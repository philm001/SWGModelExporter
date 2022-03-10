#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class skt_parser : public IFF_visitor
{
public:
  skt_parser() : m_lods_in_file(0), m_joints_in_lod(0) { }
  // Inherited via IFF_visitor
  virtual void section_begin(const std::string & name, uint8_t * data_ptr, size_t data_size, uint32_t depth) override;
  virtual void parse_data(const std::string & name, uint8_t * data_ptr, size_t data_size) override;

  virtual bool is_object_parsed() const 
  {
    return m_skeleton->is_object_correct() && (m_lods_in_file == m_skeleton->get_lod_count());
  }
  virtual std::shared_ptr<Base_object> get_parsed_object() const override
  {
    return std::dynamic_pointer_cast<Base_object>(m_skeleton);
  }
private:
  enum sections
  {
    info = 0,
    name = 1,
    prnt = 2,
    rpre = 3,
    rpst = 4,
    bptr = 5,
    bpro = 6,
    jror = 7
  };
  std::bitset<8> m_sections_received;
  uint32_t m_lods_in_file;
  uint32_t m_joints_in_lod;
  std::shared_ptr<Skeleton> m_skeleton;
};