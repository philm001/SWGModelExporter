#pragma once
#include "objects/base_object.h"
#include "objects/geometry_common.h"
#include "string"
#include "vector"
#include <memory>
#include <set>

class Shader : public Base_object
{
public:
	struct Material
	{
		Graphics::Color ambient;
		Graphics::Color diffuse;
		Graphics::Color emissive;
		Graphics::Color specular;
		float strength; // TODO : Clarify what is it
	};

	enum texture_mode : uint8_t
	{
		wrap = 0,
		mirror = 1,
		clamp = 2,
		border = 3,
		mirror_once = 4,
		tex_mode_invalid = 5
	};

	enum filter_mode : uint8_t
	{
		none = 0,
		point = 1,
		linear = 2,
		anisotropic = 3,
		flat_cubic = 4,
		gaussian_cubic = 5,
		filter_mode_invalid = 6
	};

	enum texture_type : uint8_t
	{
		specular = 0,
		main = 1,
		normal = 2,
		lightmap = 3,
		none_type = 0xFF
	};

	struct Texture
	{
		bool placeholder; //?
		texture_mode address_u;
		texture_mode address_v;
		texture_mode address_z;
		filter_mode mipmap;
		filter_mode minification;
		filter_mode magnification;
		texture_type texture_type;
		std::string texture_tag;
		std::string tex_file_name;
	};

	struct Palette
	{
		std::string color_index;
		uint8_t private_color;
		std::string texture_tag;
		std::string palette_file;
		uint32_t palette_id;
	};

	Shader() { }
	Material& material() { return m_material; }
	const Material& material() const { return m_material; }

	std::vector<Texture>& textures() { return m_textures; }
	const std::vector<Texture>& textures() const { return m_textures; }

	std::vector<Palette>& palettes() { return m_palettes; }
	const std::vector<Palette>& palettes() const { return m_palettes; }

	static texture_type get_texture_type(const std::string& texture_tag);

	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string&, const Context& context) override { }
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const { return m_name; }

private:
	Material m_material;
	std::vector<Texture> m_textures;
	std::vector<Palette> m_palettes;

	std::string m_name;
};

class Shader_appliance
{
public:
	Shader_appliance(const std::string& name) : m_name(name) { }
	const std::string& get_name() const { return m_name; }
	std::vector<uint32_t>& get_pos_indexes() { return m_position_indexes; }
	std::vector<uint32_t>& get_normal_indexes() { return m_normal_indexes; }
	std::vector<uint32_t>& get_light_indexes() { return m_light_indexes; }
	std::vector<Graphics::Tex_coord>& get_texels() { return m_texels; }
	std::vector<Graphics::Triangle_indexed>& get_triangles() { return m_triangles; }
	std::vector<std::pair<uint32_t, uint32_t>>& get_primivites() { return m_primitives; }
	void set_definition(const std::shared_ptr<Shader> shader_def) { m_shader_definition = shader_def; }
	const std::shared_ptr<Shader>& get_definition() const { return m_shader_definition; }

	void add_primitive();
	void close_primitive() { m_primitives.back().second = static_cast<uint32_t>(m_primitives.size()); }
private:
	std::string m_name;
	std::vector<uint32_t> m_position_indexes;
	std::vector<uint32_t> m_normal_indexes;
	std::vector<uint32_t> m_light_indexes;
	std::vector<Graphics::Tex_coord> m_texels;
	std::vector<Graphics::Triangle_indexed> m_triangles;
	std::vector<std::pair<uint32_t, uint32_t>> m_primitives;
	std::shared_ptr<Shader> m_shader_definition;

};