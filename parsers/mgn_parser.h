#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class mgn_parser : public IFF_visitor
{
public:
	mgn_parser() : m_in_blts(false), m_in_psdt(false) { this->setMGNParserState(true); }
	// Inherited via IFF_visitor
	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;
	virtual void section_end(uint32_t depth) override;

	virtual bool is_object_parsed() const { return m_object->is_object_correct(); }
	virtual std::shared_ptr<Base_object> get_parsed_object() const override
	{
		return std::dynamic_pointer_cast<Base_object>(m_object);
	}

	

	bool checkMeshStat()
	{
		return m_object != NULL;
	}

	void combinedObjectProcess()
	{
		//p_Bufferfile.combinedObjectProcessMGN(std::make_shared<mgn_parser>(this));
	}
	void setBuffer(IFF_file buffer) { p_Bufferfile = buffer; }
private:
	void read_normals_(base_buffer& buffer);
	void read_vertex_weights_(base_buffer& buffer);
	void read_weight_counters_(base_buffer& buffer);
	void read_vertices_list_(base_buffer& buffer);
	void read_joints_names_(base_buffer& buffer);
	void read_skeletons_list_(base_buffer& buffer);
	void read_mesh_info_(base_buffer& buffer);
	void read_shader_triangles_(const std::string& name, base_buffer& buffer);
	void read_shader_texels_(base_buffer& buffer);
	void read_shader_lighting_indexes_(base_buffer& buffer);
	void read_shader_normals_indexes_(base_buffer& buffer);
	void read_shader_vertex_indexes_(base_buffer& buffer);
	void read_shader_name_(base_buffer& buffer);
	void read_lighting_normals_(base_buffer& buffer);

	void clear_psdt_flags_();
	bool is_psdt_correct_();

	void clear_blt_flags_();
	bool is_blt_correct_();

	IFF_file p_Bufferfile;

private:
	enum section_received
	{
		skmg = 30,
		info = 0,
		sktm = 1,
		xfnm = 2,
		posn = 3,
		twhd = 4,
		twdt = 5,
		norm = 6,
		dot3 = 7,
		blts = 8, blts_blt = 9, blts_blt_info = 10, blts_blt_posn = 11, blts_blt_norm = 12, blts_blt_dot3 = 13,
		ozn = 14,
		fozc = 15,
		ozc = 16,
		zto = 17,
		psdt = 18, psdt_name = 19, psdt_pidx = 20, psdt_nidx = 21, psdt_dot3 = 22, psdt_txci = 23,
		psdt_tcsf = 24, psdt_tcsf_tcsd = 25,
		psdt_prim = 26, psdt_prim_info = 27, psdt_prim_itl = 28, psdt_prim_oitl = 29
	};

	std::bitset<31> m_section_received;
	std::shared_ptr<Animated_mesh> m_object;
	bool m_in_blts;
	bool m_in_psdt;
};

