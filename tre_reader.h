#pragma once

#include "tre_structs.h"

namespace Tre_navigator
{
  class Tre_reader
  {
  public:
    Tre_reader(const std::string &filename);

    const std::string& get_archive_name() { return m_filename; }
    uint32_t get_resource_count() { return m_header.resource_count; }
    bool is_resource_present(const std::string & res_name);
    std::string get_resource_name(uint32_t index);
    bool get_resource(uint32_t index, std::vector<uint8_t>& buffer);
    bool get_resource(const std::string & res_name, std::vector<uint8_t>& buffer);
  private:
    void _read_header();
    void _read_resource_block();
    void _read_names_block();

    void _build_lookup_table();
    size_t _get_resourece_index(const std::string& name);

    void _read_block(uint32_t offset, uint32_t compression, uint32_t comp_size, uint32_t uncomp_size, uint8_t *buffer);

    std::ifstream m_stream;                      // source stream
    std::string m_filename;                     // namefile;
    Header m_header;                             // header of file
    std::vector<Resource_info> m_resources;      // resource info block
    std::vector<char> m_names;                   // names block
    std::unordered_map<std::string, size_t> m_lookup; // table to lookup data
  };
}