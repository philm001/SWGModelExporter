#include "stdafx.h"
#include "sht_parser.h"

using namespace std;

void sht_parser::section_begin(const string & name, uint8_t * data_ptr, size_t data_size, uint32_t depth)
{
  if (name == "MATSFORM")
  {
    m_shader = make_shared<Shader>();
    m_material_mode = true;
  }
  else if (name == "TXMSFORM")
  {
    m_texture_mode = true;
  }
  else if ((name == "TXM FORM" || name == "TXM ") && m_texture_mode)
  {
    this_texture.texture_tag.clear();
    this_texture.tex_file_name.clear();
  }
}

void sht_parser::parse_data(const string & name, uint8_t * data_ptr, size_t data_size)
{
  base_buffer buffer(data_ptr, data_size);
  if (name == "0000TAG " && m_material_mode)
  {
    string rtag = buffer.read_string(4);
    material_tag.assign(rtag.rbegin(), rtag.rend());
  }
  else if (name == "MATL" && m_material_mode)
  {
    if (material_tag.empty())
      throw runtime_error("MATL section before TAG");

    auto& material = m_shader->material();

    auto read_argb = [&buffer](Graphics::Color& color)
    {
      color.a = buffer.read_float();
      color.r = buffer.read_float();
      color.g = buffer.read_float();
      color.b = buffer.read_float();
    };
    // read ambient
    read_argb(material.ambient);
    read_argb(material.diffuse);
    read_argb(material.emissive);
    read_argb(material.specular);

    material.strength = buffer.read_float();
    m_material_mode = false;
  }
  else if (name == "0001DATA" && m_texture_mode)
  {
    string tag = buffer.read_string(4);
    this_texture.texture_tag.assign(tag.rbegin(), tag.rend());
    this_texture.placeholder = (buffer.read_uint8() == 0);
    this_texture.address_u = Shader::texture_mode(buffer.read_uint8());
    this_texture.address_v = Shader::texture_mode(buffer.read_uint8());
    this_texture.address_z = Shader::texture_mode(buffer.read_uint8());
    this_texture.mipmap = Shader::filter_mode(buffer.read_uint8());
    this_texture.minification = Shader::filter_mode(buffer.read_uint8());
    this_texture.magnification = Shader::filter_mode(buffer.read_uint8());
    this_texture.texture_type = Shader::get_texture_type(this_texture.texture_tag);
  }
  else if (name == "NAME" && m_texture_mode)
  {
    this_texture.tex_file_name = buffer.read_stringz();
    m_shader->textures().push_back(this_texture);
  }
}
