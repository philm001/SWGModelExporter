#include "stdafx.h"
#include "tre_reader.h"

namespace Tre_navigator
{
  Tre_reader::Tre_reader(const std::string & filename)
    : m_filename(filename)
  {
    // Check if the filename is empty
    if (filename.empty())
    {
      throw std::runtime_error("TRE filename cannot be empty");
    }

    // Try to open the file
    m_stream.open(filename, std::ios_base::binary);
    
    // Check if the file was opened successfully
    if (!m_stream.is_open())
    {
      throw std::runtime_error("Failed to open TRE file: " + filename + " (file may not exist or access denied)");
    }

    if (!m_stream.good())
    {
      throw std::runtime_error("TRE file stream is in bad state after opening: " + filename);
    }

    try
    {
      _read_header();
      _read_resource_block();
      _read_names_block();
      _build_lookup_table();
    }
    catch (const std::exception& e)
    {
      // Close the stream if any initialization step fails
      if (m_stream.is_open())
      {
        m_stream.close();
      }
      std::string someError = e.what();
      // Re-throw with additional context
      throw std::runtime_error("Failed to initialize TRE reader for file '" + filename + "': " + someError);
    }
  }

  Tre_reader::~Tre_reader()
  {
    // Ensure the file stream is properly closed
    if (m_stream.is_open())
    {
      m_stream.close();
    }
  }

  bool Tre_reader::is_resource_present(const std::string & res_name)
  {
    auto result = m_lookup.find(res_name);
    return (result != m_lookup.end());
  }

  std::string Tre_reader::get_resource_name(uint32_t index)
  {
    if (index >= m_resources.size())
    {
      throw std::runtime_error("Resource index " + std::to_string(index) + 
                              " is out of bounds (max: " + std::to_string(m_resources.size()) + 
                              ") in file: " + m_filename);
    }

    const auto& resource = m_resources[index];
    
    // Validate name offset
    if (resource.name_offset >= m_names.size())
    {
      throw std::runtime_error("Resource " + std::to_string(index) + 
                              " has invalid name offset " + std::to_string(resource.name_offset) + 
                              " (names buffer size: " + std::to_string(m_names.size()) + 
                              ") in file: " + m_filename);
    }

    // Find the end of the string (null terminator)
    size_t string_end = resource.name_offset;
    while (string_end < m_names.size() && m_names[string_end] != '\0')
    {
      string_end++;
    }

    // Check if we found a null terminator
    if (string_end >= m_names.size())
    {
      throw std::runtime_error("Resource " + std::to_string(index) + 
                              " name is not null-terminated in file: " + m_filename);
    }

    // Create string from the validated range
    std::string name(m_names.data() + resource.name_offset, string_end - resource.name_offset);
    return name;
  }

  bool Tre_reader::get_resource(uint32_t index, std::vector<uint8_t>& buffer)
  {
    if (index >= m_resources.size())
    {
      throw std::runtime_error("Resource index " + std::to_string(index) + 
                              " is out of bounds (max: " + std::to_string(m_resources.size()) + 
                              ") in file: " + m_filename);
    }

    const auto& res_info = m_resources[index];

    if (res_info.data_size > 0)
    {
      try
      {
        if (buffer.size() < res_info.data_size)
        {
          buffer.resize(res_info.data_size);
        }
      }
      catch (const std::bad_alloc& e)
      {
        throw std::runtime_error("Failed to allocate buffer of size " + std::to_string(res_info.data_size) + 
                                " bytes for resource " + std::to_string(index) + 
                                " in file: " + m_filename + " (" + e.what() + ")");
      }

      _read_block(res_info.data_offset,
                  res_info.data_compression,
                  res_info.data_compressed_size,
                  res_info.data_size,
                  buffer.data());
    }
    else
    {
      // Resource has no data, clear the buffer
      buffer.clear();
    }
    
    return true;
  }

  bool Tre_reader::get_resource(const std::string & res_name, std::vector<uint8_t>& buffer)
  {
    if (res_name.empty())
    {
      throw std::runtime_error("Resource name cannot be empty");
    }

    auto index = _get_resourece_index(res_name);
    if (index == size_t(-1))
    {
      return false; // Resource not found - this is not an error, just return false
    }

    return get_resource(static_cast<uint32_t>(index), buffer);
  }

  void Tre_reader::_read_header()
  {
    // Check if the stream is open and in a good state
    if (!m_stream.is_open())
    {
      throw std::runtime_error("Failed to open TRE file: " + m_filename);
    }

    if (!m_stream.good())
    {
      throw std::runtime_error("TRE file stream is in bad state: " + m_filename);
    }

    // Check if we can read the header size
    m_stream.seekg(0, std::ios_base::end);
    std::streamsize file_size = m_stream.tellg();
    m_stream.seekg(0, std::ios_base::beg);

    if (file_size < static_cast<std::streamsize>(sizeof(m_header)))
    {
      throw std::runtime_error("TRE file too small to contain valid header: " + m_filename + 
                              " (size: " + std::to_string(file_size) + 
                              " bytes, expected at least: " + std::to_string(sizeof(m_header)) + " bytes)");
    }

    // Clear the header before reading
    memset(&m_header, 0, sizeof(m_header));

    // Read the header
    m_stream.read(reinterpret_cast<char *>(&m_header), sizeof(m_header));

    // Check if the read operation was successful
    if (m_stream.fail() || m_stream.gcount() != sizeof(m_header))
    {
      throw std::runtime_error("Failed to read TRE file header from: " + m_filename + 
                              " (read " + std::to_string(m_stream.gcount()) + 
                              " bytes, expected " + std::to_string(sizeof(m_header)) + " bytes)");
    }

    // Validate file type
    if (strncmp(m_header.file_type, "EERT", 4) != 0)
    {
      // Create a readable string for the actual file type
      std::string actual_type(m_header.file_type, 4);
      // Replace any non-printable characters with '?'
      for (char& c : actual_type)
      {
        if (c < 32 || c > 126) c = '?';
      }
      
      throw std::runtime_error("Invalid TRE file format - wrong file type signature in: " + m_filename + 
                              " (expected 'EERT', got '" + actual_type + "')");
    }

    // Validate file version
    if (strncmp(m_header.file_version, "5000", 4) != 0)
    {
      // Create a readable string for the actual version
      std::string actual_version(m_header.file_version, 4);
      // Replace any non-printable characters with '?'
      for (char& c : actual_version)
      {
        if (c < 32 || c > 126) c = '?';
      }
      
      throw std::runtime_error("Unsupported TRE file version in: " + m_filename + 
                              " (expected '5000', got '" + actual_version + "')");
    }

    // Basic validation of header values
    if (m_header.resource_count == 0)
    {
      throw std::runtime_error("TRE file contains no resources: " + m_filename);
    }

    if (m_header.info_offset >= file_size)
    {
      throw std::runtime_error("Invalid info_offset in TRE file header: " + m_filename + 
                              " (offset " + std::to_string(m_header.info_offset) + 
                              " >= file size " + std::to_string(file_size) + ")");
    }

    // Check if the resource count seems reasonable (prevent potential memory issues)
    const uint32_t MAX_REASONABLE_RESOURCES = 1000000; // 1 million resources should be more than enough
    if (m_header.resource_count > MAX_REASONABLE_RESOURCES)
    {
      throw std::runtime_error("TRE file claims unreasonably high resource count: " + m_filename + 
                              " (" + std::to_string(m_header.resource_count) + " resources)");
    }
  }

  void Tre_reader::_read_resource_block()
  {
    if (m_header.resource_count == 0)
    {
      return; // No resources to read
    }

    try
    {
      m_resources.resize(m_header.resource_count);
    }
    catch (const std::bad_alloc& e)
    {
      throw std::runtime_error("Failed to allocate memory for " + std::to_string(m_header.resource_count) + 
                              " resources in file: " + m_filename + " (" + e.what() + ")");
    }

    uint32_t uncomp_size = m_header.resource_count * sizeof(Resource_info);

    // Check for potential integer overflow
    if (m_header.resource_count > UINT32_MAX / sizeof(Resource_info))
    {
      throw std::runtime_error("Resource count too large, would cause integer overflow in file: " + m_filename);
    }

    _read_block(m_header.info_offset,
                m_header.info_compression,
                m_header.info_compressed_size,
                uncomp_size,
                reinterpret_cast<uint8_t *>(m_resources.data()));
  }

  void Tre_reader::_read_names_block()
  {
    if (m_header.name_uncompressed_size == 0)
    {
      return; // No names to read
    }

    try
    {
      m_names.resize(m_header.name_uncompressed_size);
    }
    catch (const std::bad_alloc& e)
    {
      throw std::runtime_error("Failed to allocate memory for names block of size " + 
                              std::to_string(m_header.name_uncompressed_size) + 
                              " bytes in file: " + m_filename + " (" + e.what() + ")");
    }

    auto name_offset = m_header.info_offset + m_header.info_compressed_size;

    // Check for potential integer overflow
    if (m_header.info_offset > UINT32_MAX - m_header.info_compressed_size)
    {
      throw std::runtime_error("Name offset calculation would overflow in file: " + m_filename);
    }

    _read_block(
      name_offset,
      m_header.name_compression,
      m_header.name_compressed_size,
      m_header.name_uncompressed_size,
      reinterpret_cast<uint8_t *>(m_names.data()));
  }

  void Tre_reader::_build_lookup_table()
  {
    for (size_t index = 0; index < m_resources.size(); ++index)
    {
      std::string resource_name(get_resource_name(static_cast<uint32_t>(index)));
      m_lookup.insert(std::make_pair(resource_name, index));
    }
  }

  size_t Tre_reader::_get_resourece_index(const std::string & name)
  {
    auto result = m_lookup.find(name);
    if (result == m_lookup.end())
      return size_t(-1);

    return result->second;
  }

  void Tre_reader::_read_block(uint32_t offset, uint32_t compression, uint32_t comp_size, uint32_t uncomp_size, uint8_t * buffer)
  {
    if (buffer == nullptr)
    {
      throw std::runtime_error("_read_block: buffer is null");
    }

    // Validate parameters
    if (uncomp_size == 0)
    {
      return; // Nothing to read
    }

    // Check file bounds
    m_stream.seekg(0, std::ios_base::end);
    std::streamsize file_size = m_stream.tellg();
    
    if (offset >= static_cast<uint32_t>(file_size))
    {
      throw std::runtime_error("_read_block: offset (" + std::to_string(offset) + 
                              ") is beyond file size (" + std::to_string(file_size) + ") in file: " + m_filename);
    }

    if (compression == 0)
    {
      // Uncompressed data
      if (offset + uncomp_size > static_cast<uint32_t>(file_size))
      {
        throw std::runtime_error("_read_block: uncompressed data extends beyond file bounds in: " + m_filename +
                                " (offset: " + std::to_string(offset) + 
                                ", size: " + std::to_string(uncomp_size) + 
                                ", file size: " + std::to_string(file_size) + ")");
      }

      m_stream.seekg(offset, std::ios_base::beg);
      if (m_stream.fail())
      {
        throw std::runtime_error("_read_block: failed to seek to offset " + std::to_string(offset) + " in file: " + m_filename);
      }

      m_stream.read(reinterpret_cast<char *>(buffer), uncomp_size);
      if (m_stream.fail() || m_stream.gcount() != static_cast<std::streamsize>(uncomp_size))
      {
        throw std::runtime_error("_read_block: failed to read uncompressed data from: " + m_filename +
                                " (requested: " + std::to_string(uncomp_size) + 
                                " bytes, read: " + std::to_string(m_stream.gcount()) + " bytes)");
      }
    }
    else if (compression == 2)
    {
      // Compressed data (zlib)
      if (comp_size == 0)
      {
        throw std::runtime_error("_read_block: compressed size is zero for compressed data in file: " + m_filename);
      }

      if (offset + comp_size > static_cast<uint32_t>(file_size))
      {
        throw std::runtime_error("_read_block: compressed data extends beyond file bounds in: " + m_filename +
                                " (offset: " + std::to_string(offset) + 
                                ", compressed size: " + std::to_string(comp_size) + 
                                ", file size: " + std::to_string(file_size) + ")");
      }

      std::vector<char> compressed_data(comp_size);

      m_stream.seekg(offset, std::ios_base::beg);
      if (m_stream.fail())
      {
        throw std::runtime_error("_read_block: failed to seek to compressed data offset " + std::to_string(offset) + " in file: " + m_filename);
      }

      m_stream.read(compressed_data.data(), comp_size);
      if (m_stream.fail() || m_stream.gcount() != static_cast<std::streamsize>(comp_size))
      {
        throw std::runtime_error("_read_block: failed to read compressed data from: " + m_filename +
                                " (requested: " + std::to_string(comp_size) + 
                                " bytes, read: " + std::to_string(m_stream.gcount()) + " bytes)");
      }

      // Initialize zlib stream
      z_stream stream;
      stream.zalloc = Z_NULL;
      stream.zfree = Z_NULL;
      stream.opaque = Z_NULL;
      stream.avail_in = 0;
      stream.next_in = Z_NULL;
      
      int result = inflateInit(&stream);
      if (result != Z_OK)
      {
        throw std::runtime_error("_read_block: zlib inflateInit failed with error code " + std::to_string(result) + " in file: " + m_filename);
      }

      // Setup for decompression
      stream.next_in = reinterpret_cast<Bytef *>(compressed_data.data());
      stream.avail_in = comp_size;
      stream.next_out = reinterpret_cast<Bytef *>(buffer);
      stream.avail_out = uncomp_size;

      // Decompress
      result = inflate(&stream, Z_FINISH);
      if (result != Z_STREAM_END && result != Z_OK)
      {
        inflateEnd(&stream);
        throw std::runtime_error("_read_block: zlib inflate failed with error code " + std::to_string(result) + " in file: " + m_filename);
      }

      // Verify we got the expected amount of data
      if (stream.total_out != uncomp_size)
      {
        inflateEnd(&stream);
        throw std::runtime_error("_read_block: decompressed size mismatch in: " + m_filename +
                                " (expected: " + std::to_string(uncomp_size) + 
                                " bytes, got: " + std::to_string(stream.total_out) + " bytes)");
      }

      inflateEnd(&stream);
    }
    else
    {
      throw std::runtime_error("_read_block: unknown compression type " + std::to_string(compression) + " in file: " + m_filename);
    }
  }
}