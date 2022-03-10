#include "stdafx.h"
#include "cat_parser.h"
#include "objects/animated_object.h"

void cat_parser::reset()
{
	m_section_received.reset();
	m_object == nullptr;
	m_meshes_count = 0;
	m_skeletons_count = 0;
	m_latx_present = false;
}
void cat_parser::section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "SMATFORM")
	{
		m_section_received[smat] = true;
		m_object = std::make_shared<Animated_object_descriptor>();
	}
}

void cat_parser::parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size)
{
	if (name == "0003INFO" && m_section_received[smat])
	{
		base_buffer buffer(data_ptr, data_size);
		m_meshes_count = buffer.read_uint32();
		m_skeletons_count = buffer.read_uint32();
		m_latx_present = (buffer.read_uint8() == 1);
		m_section_received[info] = buffer.end_of_buffer();
	}
	else if (name == "MSGN" && m_section_received[info] && m_meshes_count > 0)
	{
		base_buffer buffer(data_ptr, data_size);
		std::string mesh_name;
		for (uint32_t idx = 0; idx < m_meshes_count; ++idx)
		{
			mesh_name = buffer.read_stringz();
			m_object->add_mesh_name(mesh_name);
		}
		m_section_received[msgn] = buffer.end_of_buffer();
	}
	else if (name == "SKTI" && m_section_received[msgn] && m_skeletons_count > 0)
	{
		base_buffer buffer(data_ptr, data_size);
		std::string slot;
		std::string skeleton_name;
		for (uint32_t idx = 0; idx < m_skeletons_count; ++idx)
		{
			skeleton_name = buffer.read_stringz();
			slot = buffer.read_stringz();

			m_object->add_skeleton_info(skeleton_name, slot);
		}
		m_section_received[skti] = buffer.end_of_buffer();
	}
	else if (name == "LATX" && m_section_received[skti] && m_latx_present)
	{
		base_buffer buffer(data_ptr, data_size);
		uint16_t map_count = buffer.read_uint16();
		std::string skeleton;
		std::string anim_file;
		for (uint16_t idx = 0; idx < map_count; ++idx)
		{
			skeleton = buffer.read_stringz();
			anim_file = buffer.read_stringz();
			m_object->add_anim_map(skeleton, anim_file);
		}
		m_section_received[latx] = buffer.end_of_buffer();
	}
}

bool cat_parser::is_object_parsed() const
{
	return m_section_received[smat]
		&& m_section_received[info]
		&& m_section_received[msgn]
		&& m_section_received[skti]
		&& ((m_latx_present) ? m_section_received[latx] : true);
}
