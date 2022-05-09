#include "stdafx.h"
#include "mgn_parser.h"

using namespace std;

void mgn_parser::section_begin(const string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "SKMGFORM")
	{
		m_section_received[skmg] = true;
		if(!getMeshState())
			m_object = make_shared<Animated_mesh>();
	}
	else
	{
		string form_type(data_ptr, data_ptr + 4);
		if (form_type == "BLTS")
			m_in_blts = true;
		else if (form_type == "PSDT")
			m_in_psdt = true;

		if (m_in_blts && m_in_psdt)
			throw runtime_error("Invalid state of parser");
	}
}

void mgn_parser::section_end(uint32_t depth)
{
	if (depth == 2)
	{
		if (m_in_blts)
			m_in_blts = false;
		if (m_in_psdt)
			m_in_psdt = false;
	}
}

void mgn_parser::parse_data(const string& name, uint8_t* data_ptr, size_t data_size)
{
	// workaround of zero sized buffer
	if (data_size == 0)
		return;

	base_buffer buffer(data_ptr, data_size);

	if ((name == "0004INFO" || name == "0003INFO") && m_section_received[skmg])
	{
		read_mesh_info_(buffer);
	}
	else if (name == "SKTM" && m_section_received[info] && m_object->get_info().num_skeletons > 0)
	{
		read_skeletons_list_(buffer);
	}
	else if (name == "XFNM" && m_section_received[sktm])
	{
		read_joints_names_(buffer);
	}
	else if (name == "POSN" && m_section_received[xfnm] && !(m_in_blts || m_in_psdt))
	{
		read_vertices_list_(buffer);
	}
	else if (name == "TWHD" && m_section_received[posn])
	{
		read_weight_counters_(buffer);
	}
	else if (name == "TWDT" && m_section_received[twhd])
	{
		read_vertex_weights_(buffer);
	}
	else if (name == "NORM" && m_section_received[twdt] && !(m_in_blts || m_in_psdt))
	{
		read_normals_(buffer);
	}
	else if (name == "DOT3" && m_section_received[norm] && !(m_in_blts || m_in_psdt))
	{
		read_lighting_normals_(buffer);
	}
	else if (name == "PSDTNAME"
		&& (m_section_received[dot3] || m_section_received[norm] || m_section_received[psdt] || m_section_received[zto]))
	{
		read_shader_name_(buffer);
	}
	else if (name == "PIDX" && (m_section_received[psdt_name]))
	{
		read_shader_vertex_indexes_(buffer);
	}
	else if (name == "NIDX" && (m_section_received[psdt_pidx]))
	{
		read_shader_normals_indexes_(buffer);
	}
	else if (name == "DOT3" && (m_section_received[psdt_nidx]))
	{
		read_shader_lighting_indexes_(buffer);
	}
	else if (name == "TXCI" && (m_section_received[psdt_dot3] || m_section_received[psdt_nidx]))
	{
		// just read it
		uint32_t coord_per_vetext = buffer.read_uint32();
		uint32_t dimenstion_per_coord = buffer.read_uint32();
		m_section_received[psdt_txci] = buffer.end_of_buffer();
	}
	else if (name == "TCSFTCSD" && m_section_received[psdt_txci])
	{
		read_shader_texels_(buffer);
	}
	else if (name == "PRIMINFO" && m_section_received[psdt_tcsf_tcsd])
	{
		uint32_t num_primitives = buffer.read_uint32();
		m_section_received[psdt_prim_info] = buffer.end_of_buffer();
	}
	else if ((name == "ITL " || name == "OITL") && m_section_received[psdt_prim_info])
	{
		read_shader_triangles_(name, buffer);
	}
	else if (name == "BLT INFO" && m_section_received[dot3] && m_in_blts)
	{
		auto pos_altered = buffer.read_uint32();
		auto normals_altered = buffer.read_uint32();
		auto blend_name = buffer.read_stringz();

		clear_blt_flags_();
		m_object->add_new_morph(blend_name);
		m_section_received[blts_blt_info] = buffer.end_of_buffer();
	}
	else if (name == "POSN" && m_section_received[blts_blt_info] && m_in_blts)
	{
		auto& morph = m_object->get_current_morph();
		while (!buffer.end_of_buffer())
		{
			uint32_t index = buffer.read_uint32();
			float x = buffer.read_float();
			float y = buffer.read_float();
			float z = buffer.read_float();

			morph.get_positions().emplace_back(index, Geometry::Vector3{ x, y, z });
		}
		m_section_received[blts_blt_posn] = buffer.end_of_buffer();
	}
	else if (name == "NORM" && m_section_received[blts_blt_posn] && m_in_blts)
	{
		auto& morph = m_object->get_current_morph();
		while (!buffer.end_of_buffer())
		{
			uint32_t index = buffer.read_uint32();
			float x = buffer.read_float();
			float y = buffer.read_float();
			float z = buffer.read_float();

			morph.get_normals().emplace_back(index, Geometry::Vector3{ x, y, z });
		}
		m_section_received[blts_blt_norm] = buffer.end_of_buffer();
	}
	else if (name == "DOT3" && m_section_received[blts_blt_norm] && m_in_blts)
	{
		auto& morph = m_object->get_current_morph();
		uint32_t size = buffer.read_uint32();
		while (!buffer.end_of_buffer())
		{
			uint32_t index = buffer.read_uint32();
			float x = buffer.read_float();
			float y = buffer.read_float();
			float z = buffer.read_float();

			morph.get_tangents().emplace_back(index, Geometry::Vector3{ x, y, z });
		}
		m_section_received[blts_blt_dot3] = buffer.end_of_buffer();
	}
}

void mgn_parser::read_shader_triangles_(const string& name, base_buffer& buffer)
{
	bool oitl = (name == "OITL");
	auto& curr_shader = m_object->get_current_shader();
	curr_shader.add_primitive();
	uint32_t num_triangles = buffer.read_uint32();
	uint32_t tri_readed = 0;
	while (!buffer.end_of_buffer() && (tri_readed < num_triangles))
	{
		if (oitl)
			uint16_t dummy = buffer.read_uint16();
		uint32_t v1 = buffer.read_uint32();
		uint32_t v2 = buffer.read_uint32();
		uint32_t v3 = buffer.read_uint32();
		curr_shader.get_triangles().emplace_back(Graphics::Triangle_indexed{ v1, v2, v3 });
		tri_readed++;
	}
	curr_shader.close_primitive();
	bool section_ok = buffer.end_of_buffer() && tri_readed == num_triangles;
	auto section_to_set = (oitl) ? psdt_prim_oitl : psdt_prim_itl;
	m_section_received[section_to_set] = section_ok;
	m_section_received[psdt] = is_psdt_correct_();
}

void mgn_parser::read_shader_texels_(base_buffer& buffer)
{
	auto& curr_shader = m_object->get_current_shader();
	uint32_t num_vertices = static_cast<uint32_t>(curr_shader.get_pos_indexes().size());
	uint32_t vertex_idx = 0;
	while (!buffer.end_of_buffer() && vertex_idx < num_vertices)
	{
		float u = buffer.read_float();
		float v = buffer.read_float();

		curr_shader.get_texels().emplace_back(u, 1.0f - v);
		vertex_idx++;
	}
	m_section_received[psdt_tcsf_tcsd] = buffer.end_of_buffer() && (vertex_idx == num_vertices);
}

void mgn_parser::read_shader_lighting_indexes_(base_buffer& buffer)
{
	auto& curr_shader = m_object->get_current_shader();
	while (!buffer.end_of_buffer())
	{
		uint32_t light_idx = buffer.read_uint32();
		curr_shader.get_light_indexes().emplace_back(light_idx);
	}
	m_section_received[psdt_dot3] = buffer.end_of_buffer();
}

void mgn_parser::read_shader_normals_indexes_(base_buffer& buffer)
{
	auto& curr_shader = m_object->get_current_shader();
	while (!buffer.end_of_buffer())
	{
		uint32_t normal_index = buffer.read_uint32();
		curr_shader.get_normal_indexes().emplace_back(normal_index);
	}
	m_section_received[psdt_nidx] = buffer.end_of_buffer();
}

void mgn_parser::read_shader_vertex_indexes_(base_buffer& buffer)
{
	auto& curr_shader = m_object->get_current_shader();
	uint32_t num_vertices = buffer.read_uint32();
	uint32_t vertices_read = 0;
	while (!buffer.end_of_buffer() && vertices_read < num_vertices)
	{
		uint32_t vert_idx = buffer.read_uint32();
		curr_shader.get_pos_indexes().emplace_back(vert_idx);
		vertices_read++;
	}
	m_section_received[psdt_pidx] = buffer.end_of_buffer() && (vertices_read == num_vertices);
}

void mgn_parser::read_shader_name_(base_buffer& buffer)
{
	// begin of new PSDT section
	string shader_name = buffer.read_stringz();
	m_object->add_new_shader(shader_name);
	clear_psdt_flags_();
	m_section_received[psdt_name] = buffer.end_of_buffer();
}

void mgn_parser::read_lighting_normals_(base_buffer& buffer)
{
	const auto& mesh_info = m_object->get_info();
	uint32_t light_readed = 0;
	uint32_t lights_to_read = buffer.read_uint32();
	while (!buffer.end_of_buffer() && light_readed < lights_to_read)
	{
		Geometry::Vector4 vec;
		vec.x = buffer.read_float();
		vec.y = buffer.read_float();
		vec.z = buffer.read_float();
		vec.a = buffer.read_float();

		m_object->add_lighting_normal(vec);
		light_readed++;
	}
	m_section_received[dot3] = buffer.end_of_buffer() && light_readed == lights_to_read;
}

void mgn_parser::read_normals_(base_buffer& buffer)
{
	const auto& mesh_info = m_object->get_info();
	uint32_t norm_readed = 0;
	while (!buffer.end_of_buffer() && norm_readed < mesh_info.normals_count)
	{
		Geometry::Vector3 vec;
		vec.x = buffer.read_float();
		vec.y = buffer.read_float();
		vec.z = buffer.read_float();

		m_object->add_normal(vec);
		norm_readed++;
	}
	m_section_received[norm] = buffer.end_of_buffer() && norm_readed == mesh_info.normals_count;
}

void mgn_parser::read_vertex_weights_(base_buffer& buffer)
{
	const auto& mesh_info = m_object->get_info();
	uint32_t vertex_pos = 0;
	uint32_t weights_readed = 0;
	while (!buffer.end_of_buffer() && vertex_pos < mesh_info.position_counts)
	{
		auto& vertex = m_object->get_vertex(vertex_pos);

		auto num_weights = vertex.get_weights().size();
		size_t weight_pos = 0;

		while (num_weights > 0)
		{
			uint32_t joint_idx = buffer.read_uint32();
			float joint_weight = buffer.read_float();

			vertex.get_weights()[weight_pos++] = make_pair(joint_idx, joint_weight);

			num_weights--;
			weights_readed++;
		}

		vertex_pos++;
	}
	m_section_received[twdt] = buffer.end_of_buffer()
		&& weights_readed == mesh_info.joint_weight_data_count
		&& vertex_pos == mesh_info.position_counts;
}

void mgn_parser::read_weight_counters_(base_buffer& buffer)
{
	const auto& mesh_info = m_object->get_info();
	uint32_t counters_readed = 0;
	while (!buffer.end_of_buffer() && counters_readed < mesh_info.position_counts)
	{
		if (counters_readed == 156)
		{
			int a = 0;
			a++;
		}
		if (counters_readed >= m_object->get_vertices().size())
		{
			break;
		}
		auto& vertex = m_object->get_vertex(counters_readed);
		uint32_t num_weights = buffer.read_uint32();
		vertex.get_weights().resize(num_weights);
		counters_readed++;
	}
	m_section_received[twhd] = buffer.end_of_buffer() && counters_readed == mesh_info.position_counts;
}

void mgn_parser::read_vertices_list_(base_buffer& buffer)
{
	const auto& mesh_info = m_object->get_info();
	uint32_t points_readed = 0;
	while (!buffer.end_of_buffer() && points_readed < mesh_info.position_counts)
	{
		Geometry::Point pt;
		pt.x = buffer.read_float();
		pt.y = buffer.read_float();
		pt.z = buffer.read_float();

		m_object->add_vertex_position(pt);
		points_readed++;
	}
	m_section_received[posn] = buffer.end_of_buffer() && points_readed == mesh_info.position_counts;
}

void mgn_parser::read_joints_names_(base_buffer& buffer)
{
	const auto& mesh_info = m_object->get_info();
	uint32_t joints_readed = 0;
	while (!buffer.end_of_buffer() && joints_readed < mesh_info.num_joints)
	{
		string joint_name = buffer.read_stringz();
		m_object->add_joint_name(joint_name);
		joints_readed++;
	}
	m_section_received[xfnm] = buffer.end_of_buffer() && (joints_readed == mesh_info.num_joints);
}

void mgn_parser::read_skeletons_list_(base_buffer& buffer)
{
	// read list of skeletals
	const auto& mesh_info = m_object->get_info();

	uint32_t skels_readed = 0;
	while (!buffer.end_of_buffer() && skels_readed < mesh_info.num_skeletons)
	{
		string skel_name = buffer.read_stringz();
		m_object->add_skeleton_name(skel_name);
		skels_readed++;
	}
	m_section_received[sktm] = buffer.end_of_buffer() && (skels_readed == mesh_info.num_skeletons);
}

void mgn_parser::read_mesh_info_(base_buffer& buffer)
{
	// read info
	Animated_mesh::Info mesh_info;

	auto max_tranforms_per_vector = buffer.read_uint32();
	auto max_tranforms_per_shader = buffer.read_uint32();

	buffer.read_buffer(reinterpret_cast<uint8_t*>(&mesh_info), sizeof(mesh_info));

	m_object->set_info(mesh_info);
	m_section_received[info] = buffer.end_of_buffer();
}

void mgn_parser::clear_psdt_flags_()
{
	for (uint8_t flag = psdt; flag <= psdt_prim_oitl; flag++)
		m_section_received[flag] = false;
}

bool mgn_parser::is_psdt_correct_()
{
	return m_section_received[psdt_name] &&
		m_section_received[psdt_pidx] &&
		m_section_received[psdt_nidx] &&
		m_section_received[psdt_dot3] &&
		m_section_received[psdt_txci] &&
		m_section_received[psdt_tcsf_tcsd] &&
		m_section_received[psdt_prim_info] &&
		(m_section_received[psdt_prim_itl] || m_section_received[psdt_prim_oitl])
		;
}

void mgn_parser::clear_blt_flags_()
{
	for (uint8_t flags = blts; flags <= blts_blt_dot3; flags++)
		m_section_received[flags] = false;
}

bool mgn_parser::is_blt_correct_()
{
	return m_section_received[blts_blt_info] && m_section_received[blts_blt_posn]
		&& m_section_received[blts_blt_norm] && m_section_received[blts_blt_dot3];
}



