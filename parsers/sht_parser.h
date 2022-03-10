#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class sht_parser : public IFF_visitor
{
public:
  // Inherited via IFF_visitor
  virtual void section_begin(const std::string & name, uint8_t * data_ptr, size_t data_size, uint32_t depth) override;
  virtual void parse_data(const std::string & name, uint8_t * data_ptr, size_t data_size) override;

  virtual std::shared_ptr<Base_object> get_parsed_object() const override
  {
    return std::dynamic_pointer_cast<Base_object>(m_shader);
  }
  virtual bool is_object_parsed() const { return m_shader != nullptr; }
private:
  bool m_material_mode = false;
  bool m_texture_mode = false;

  std::string material_tag;
  Shader::Texture this_texture;

  std::shared_ptr<Shader> m_shader;
};
