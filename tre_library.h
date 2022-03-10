#pragma once

#include "tre_reader.h"

namespace Tre_navigator
{
  class Tre_library_reader_callback
  {
  public:
    virtual void number_of_files(size_t file_num) = 0;
    virtual void file_read() = 0;
  };

  class Tre_library
  {
  public:
    Tre_library() = delete;
    Tre_library(const std::string& path, Tre_library_reader_callback * callback_ptr = nullptr);

    bool is_object_present(const std::string& name);
    bool get_object_name(const std::string& partial_name, std::string& full_name);

    size_t number_of_object_versions(const std::string& name);
    std::vector<std::weak_ptr<Tre_reader>> get_versioned_readers(const std::string& name);
    bool get_object(const std::string& name, std::vector<uint8_t> &buffer, size_t version_num = 0, bool search = false);
    bool select_objects_by_ext(const std::string& ext, std::vector<std::string>& result);
  private:
    std::vector<std::shared_ptr<Tre_reader>> m_readers;
    std::unordered_multimap<std::string, std::weak_ptr<Tre_reader>> m_reader_lookup;
  };
} // Tre_navigator