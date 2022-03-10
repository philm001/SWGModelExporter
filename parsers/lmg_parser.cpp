#include "stdafx.h"
#include "lmg_parser.h"

using namespace std;

void lmg_parser::section_begin(const string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
  if (name == "MLODFORM")
  {
    m_sections_received[mlod] = true;
    m_lmg_object = make_shared<Animated_lod_list>();
  }
}

void lmg_parser::parse_data(const string& block_name, uint8_t* data_ptr, size_t data_size)
{
  base_buffer buffer(data_ptr, data_size);

  if (block_name == "0000INFO" && m_sections_received[mlod])
  {
    m_names_count = buffer.read_uint16();
    m_names_received = 0;
    m_sections_received[info] = buffer.end_of_buffer();
  }
  else if (block_name == "NAME" && m_sections_received[info] && (m_names_received < m_names_count))
  {
    string mesh_name;
    mesh_name = buffer.read_stringz();
    if (buffer.end_of_buffer())
      m_names_received++;
    else
      return;

    m_lmg_object->add_lod_name(mesh_name);

    m_sections_received[name] = (m_names_received == m_names_count);
  }
}

bool lmg_parser::is_object_parsed() const
{
  return (m_sections_received[mlod] && m_sections_received[info] && m_sections_received[name]);
}
