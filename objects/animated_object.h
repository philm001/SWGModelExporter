#pragma once
#include "UncompressQuaternion.h"
#include "objects/base_object.h"
#include "geometry_common.h"


//
// Animated object descriptor, builded from SAT files
//
class Animated_object_descriptor : public Base_object
{
public:
	Animated_object_descriptor()
	{ }

	// getters
	// meshes
	size_t get_mesh_count() const { return m_mesh_names.size(); }
	std::string get_mesh_name(size_t idx) { return m_mesh_names[idx]; }
	void setMeshNames(std::vector<std::string> names)
	{
		m_mesh_names = names;
	}

	std::vector<std::string> getMeshNames()
	{
		return m_mesh_names;
	}

	// skeletons
	size_t get_skeletons_count() const { return m_skeleton_info.size(); }
	std::string get_skeleton_name(size_t idx) { return m_skeleton_info[idx].first; }
	std::string get_skeleton_attach_point(size_t idx) { return m_skeleton_info[idx].second; }

	// anim maps
	size_t get_animation_maps() const { return m_animations_map.size(); }
	std::string get_animation_for_skeleton(const std::string& skt_name) { return m_animations_map[skt_name]; }

	// fillers
	void add_mesh_name(const std::string& mesh_name) { m_mesh_names.push_back(mesh_name); }
	void add_skeleton_info(const std::string& skt_name, const std::string& slot)
	{
		m_skeleton_info.push_back(make_pair(skt_name, slot));
	}
	void add_anim_map(const std::string& skt_name, const std::string& anim_map)
	{
		m_animations_map[skt_name] = anim_map;
	}

	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override { };
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }

private:
	std::vector<std::string> m_mesh_names;
	std::vector<std::pair<std::string, std::string>> m_skeleton_info;
	std::unordered_map<std::string, std::string> m_animations_map;

	std::string m_name;
};

class Animated_lod_list : public Base_object
{
public:
	Animated_lod_list() { }

	void add_lod_name(const std::string& name) { m_lod_names.emplace_back(name); }
	// overrides
	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override { };
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }
private:
	std::vector<std::string> m_lod_names;
	std::string m_name;
};

class Animated_file_list : public Base_object
{
public:
	Animated_file_list() { }

	void add_animation(const std::string& name)
	{
		auto returnValue = std::find(m_animation_list.begin(), m_animation_list.end(), name);
		if (returnValue == m_animation_list.end() || m_animation_list.empty())
			m_animation_list.push_back(name); // Make sure there are duplicates
	}
	// overrides
	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override { };
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }
private:
	std::vector<std::string> m_animation_list;
	std::string m_name;
};

class DDS_Texture : public Base_object
{
public:
	static std::shared_ptr<DDS_Texture> construct(const std::string& name, const uint8_t* buffer, size_t buf_size);

	// Inherited via Base_object
	virtual bool is_object_correct() const override { return (m_name.empty() == false && m_image); }
	virtual void store(const std::string& path, const Context& context) override;
	virtual std::set<std::string> get_referenced_objects() const override { return std::set<std::string>(); }
	virtual void resolve_dependencies(const Context& context) override { }
	virtual void set_object_name(const std::string& obj_name) override { m_name = obj_name; }
	virtual std::string get_object_name() const override { return m_name; }

private:
	std::string m_name;
	std::shared_ptr<DirectX::ScratchImage> m_image = nullptr;
};

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

class Animated_mesh;

class Skeleton : public Base_object
{
public:
	struct Bone
	{
		Bone(const std::string& bone_name)
			: name(bone_name), parent_idx(-1)
		{
		}
		std::string name;
		int32_t parent_idx; // -1 means no parent, i.e. root
		Geometry::Vector4 pre_rot_quaternion;
		Geometry::Vector4 post_rot_quaternion;
		Geometry::Vector3 bind_pose_transform;
		Geometry::Vector4 bind_pose_rotation;
		uint32_t rotation_order;
	};

public:
	Skeleton() : m_current_lod(0) { }
	Skeleton(const Skeleton& skt) : m_skeleton_name(skt.m_skeleton_name), m_bones(skt.m_bones), m_current_lod(skt.m_current_lod) { }

	std::shared_ptr<Skeleton> clone();
	void add_lod_level()
	{
		m_bones.push_back(std::vector<Bone>());
		m_current_lod = static_cast<uint32_t>(m_bones.size() - 1);
	}
	uint32_t get_lod_count() const { return static_cast<uint32_t>(m_bones.size()); }
	uint32_t get_current_lod() const { return m_current_lod; }
	void set_current_lod(uint32_t lod) { assert(lod < m_bones.size()); m_current_lod = lod; }

	void add_bone(const std::string& bone_name) { m_bones[m_current_lod].emplace_back(bone_name); }
	Bone& get_bone(uint32_t idx) { return m_bones[m_current_lod][idx]; }
	uint32_t get_bones_count() const { return static_cast<uint32_t>(m_bones[m_current_lod].size()); }

	std::vector<Skeleton::Bone> generate_skeleton_in_scene(FbxScene* scene_ptr, FbxNode* parent_ptr, Animated_mesh* source_mesh);
	void join_skeleton_to_point(const std::string& attach_point, const std::shared_ptr<Skeleton>& skel_to_join);

	// Inherited via Base_object
	virtual bool is_object_correct() const override;
	virtual void store(const std::string& path, const Context& context) override { };
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context& context) override;
	virtual void set_object_name(const std::string& obj_name) override;
	virtual std::string get_object_name() const override { return m_skeleton_name; }

private:
	fbxsdk::FbxAMatrix GetGlobalDefaultPosition(fbxsdk::FbxNode* pNode);
	std::string m_skeleton_name;
	std::vector<std::vector<Bone>> m_bones;
	uint32_t m_current_lod;
};

class Animation : public Base_object
{
public:
#pragma pack(push, 1)
	struct Info
	{
		float FPS = 0;
		uint16_t frame_count;
		uint16_t transform_count;
		uint16_t rotation_channel_count;
		uint16_t static_rotation_count;
		uint16_t translation_channel_count;
		uint16_t static_translation_count;
	};

	struct Bone_info
	{
		std::string name;
		bool has_rotations;
		bool hasXAnimatedTranslation = false;
		bool hasYAnimatedTranslation = false;
		bool hasZAnimatedTranslation = false;
		bool hasXAnimatedRotatation = false;
		bool hasYAnimatedRotatation = false;
		bool hasZAnimatedRotatation = false;
		uint16_t rotation_channel_index;
		uint8_t translation_mask;
		uint16_t x_translation_channel_index;
		uint16_t y_translation_channel_index;
		uint16_t z_translation_channel_index;
		std::vector<Geometry::Vector4> BoneMovement;
	};
#pragma pack(pop)
	Animation() { };

	// Inherited via Base_object
	virtual bool is_object_correct() const override;
	virtual void store(const std::string& path, const Context& context) override;
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context& context) override;
	virtual void set_object_name(const std::string& obj_name) override;
	virtual std::string get_object_name() const override;
	bool hasFPS() { return m_info.FPS != 0; }

	// getters/setters
	void set_info(const Info& info) { m_info = info; }
	const Info& get_info() const { return m_info; }

	std::vector<Bone_info>& get_bones() { return m_animated_bones; } // This is the XFIN

	std::vector< std::vector<uint32_t>>& getQCHNValues() { return m_QCHNValues; }// This is where the AROT is stored
	std::vector<std::vector<float>>& getCHNLValues() { return m_CHNLValues; } // This is where the ATRN is stored

	std::vector<uint32_t>& getStaticRotationValues() { return m_staticRotationValues; } // This is where the SROT is stored
	std::vector<std::vector<uint8_t>>& getStaticROTFormats() { return m_staticFormatValues; }

	std::vector<float>& getStaticTranslationValues() { return m_staticTranslationValues; } // This is where the ATRN is stored

	std::vector<std::vector<uint8_t>>& getFormatValues() { return CoordinateFormatValue; }

private:
	std::string m_object_name;
	Info m_info;
	std::vector<Bone_info> m_animated_bones;

	std::vector< std::vector<uint32_t>> m_QCHNValues;
	std::vector< std::vector<float>> m_CHNLValues;

	std::vector<uint32_t> m_staticRotationValues;
	std::vector<std::vector<uint8_t>> m_staticFormatValues;

	std::vector<float> m_staticTranslationValues;

	std::vector<std::vector<uint8_t>> CoordinateFormatValue;
	
	std::map<std::vector<uint8_t>, uint32_t> m_staticRotateValues;
};

class Animated_mesh : public Base_object
{
public:
#pragma pack(push, 1)
	struct Info
	{
		uint32_t num_skeletons;
		uint32_t num_joints;
		uint32_t position_counts;
		uint32_t joint_weight_data_count;
		uint32_t normals_count;
		uint32_t num_shaders;
		uint32_t blend_targets;
		uint16_t occlusion_zones;
		uint16_t occlusion_zones_combinations;
		uint16_t occluded_zones;
		int16_t occlusion_layer;
	};
#pragma pack(pop)

	struct Bone_Info
	{
		std::string Bonename = "";
		FbxQuaternion Pre_Quat;
		FbxQuaternion Post_Quat;
		FbxQuaternion Bind_Pose;
	};

	class Vertex
	{
	public:
		Vertex(const Geometry::Point& position) : m_position(position) { }
		void set_position(const Geometry::Point& point) { m_position = point; }
		const Geometry::Point& get_position() const { return m_position; }
		std::vector<std::pair<uint32_t, float>>& get_weights() { return m_weights; }
		const std::vector<std::pair<uint32_t, float>>& get_weights() const { return m_weights; }
		bool isEqual(const Vertex& compareVertex)
		{
			bool firstCheck = false;
			bool secondCheck = false;
			bool thirdCheck = true;

			if (compareVertex.get_position().x == m_position.x &&
				compareVertex.get_position().y == m_position.y &&
				compareVertex.get_position().z == m_position.z)
				firstCheck = true;

			if (compareVertex.get_weights().size() == m_weights.size())
				secondCheck = true;

			if (secondCheck)
			{
				for (auto weightIterator : compareVertex.get_weights())
				{
					bool isEqual = false;
					for (auto otherWeightIterator : m_weights)
					{
						if (weightIterator == otherWeightIterator)
						{
							isEqual = true;
							break;
						}
					}

					if (!isEqual)
					{
						thirdCheck = false;
						break;
					}
				}
			}
			else
				thirdCheck = false;

			
			return firstCheck && secondCheck && thirdCheck;
		}

	private:
		Geometry::Point m_position;
		std::vector<std::pair<uint32_t, float>> m_weights;
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
		void set_definition(const std::shared_ptr<Shader>& shader_def) { m_shader_definition = shader_def; }
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

	class Morph_target
	{
	public:
		Morph_target(const std::string& name) : m_name(name) { }
		std::vector<std::pair<uint32_t, Geometry::Vector3>>& get_positions() { return m_position_offsets; }
		std::vector<std::pair<uint32_t, Geometry::Vector3>>& get_normals() { return m_normal_offsets; }
		std::vector<std::pair<uint32_t, Geometry::Vector3>>& get_tangents() { return m_tangent_offsets; }

		const std::vector<std::pair<uint32_t, Geometry::Vector3>>& get_positions() const { return m_position_offsets; }
		const std::vector<std::pair<uint32_t, Geometry::Vector3>>& get_normals() const { return m_normal_offsets; }
		const std::vector<std::pair<uint32_t, Geometry::Vector3>>& get_tangents() const { return m_tangent_offsets; }

		const std::string get_name() const { return m_name; }
	private:
		std::string m_name;
		std::vector<std::pair<uint32_t, Geometry::Vector3>> m_position_offsets;
		std::vector<std::pair<uint32_t, Geometry::Vector3>> m_normal_offsets;
		std::vector<std::pair<uint32_t, Geometry::Vector3>> m_tangent_offsets;
	};

	struct EulerAngles
	{
		double roll;
		double pitch;
		double yaw;
	};

public:
	Animated_mesh() : m_lod_level(0) 
	{ 
		m_mesh_info.num_skeletons = 0;
		m_mesh_info.num_joints = 0;
		m_mesh_info.position_counts = 0;
		m_mesh_info.joint_weight_data_count = 0;
		m_mesh_info.normals_count = 0;
		m_mesh_info.num_shaders = 0;
		m_mesh_info.blend_targets = 0;
		m_mesh_info.occlusion_zones = 0;
		m_mesh_info.occlusion_zones_combinations = 0;
		m_mesh_info.occluded_zones = 0;
		m_mesh_info.occlusion_layer = 0;
	}

	const Info& get_info() const { return m_mesh_info; }
	void set_info(const Info& info);

	void add_vertex_position(const Geometry::Point& position) { m_vertices.emplace_back(position); }
	void addVertex(Vertex point) { m_vertices.push_back(point); }

	Vertex& get_vertex(const uint32_t pos_num)
	{ 
		return m_vertices.at(pos_num);
	}
	const std::vector<Vertex>& get_vertices() const { return m_vertices; }
	const std::vector<std::string>& get_joint_names() const { return m_joint_names; }

	void add_normal(const Geometry::Vector3& norm) { m_normals.emplace_back(norm); }
	std::vector<Geometry::Vector3>& getNormals() { return m_normals; }

	void add_lighting_normal(const Geometry::Vector4& light_norm) { m_lighting_normals.emplace_back(light_norm); }
	std::vector<Geometry::Vector4>& getNormalLighting() { return m_lighting_normals; }

	void add_skeleton_name(const std::string& name) { m_skeletons_names.emplace_back(name); }
	void add_joint_name(const std::string& name) { m_joint_names.emplace_back(name); }

	void add_new_shader(const std::string& name) { m_shaders.emplace_back(name); }
	void addShader(const Shader_appliance& shader) { m_shaders.push_back(shader); }
	std::vector<Shader_appliance>& getShaders() { return m_shaders; }

	std::vector<std::pair<std::string, std::shared_ptr<Skeleton>>> getSkeleton() { return m_used_skeletons; }

	void add_new_morph(const std::string& name) { m_morphs.emplace_back(name); }
	Shader_appliance& get_current_shader() { assert(m_shaders.empty() == false); return m_shaders.back(); }
	Morph_target& get_current_morph() { assert(m_morphs.empty() == false); return m_morphs.back(); }

	// Inherited via Base_object
	virtual bool is_object_correct() const override;
	virtual void store(const std::string& path, const Context& context) override;
	virtual std::set<std::string> get_referenced_objects() const override;
	virtual void resolve_dependencies(const Context& context) override;
	virtual void set_object_name(const std::string& obj_name) override { m_object_name = obj_name; }
	virtual std::string get_object_name() const override { return m_object_name; }
	uint32_t getLodLevel() 
	{ 
		std::string name = m_object_name;
		if (name.find("0") != std::string::npos)
		{
			return 0;
		}

		if (name.find("1") != std::string::npos)
		{
			return 1;
		}

		if (name.find("2") != std::string::npos)
		{
			return 2;
		}

		if (name.find("3") != std::string::npos)
		{
			return 3;
		}
		return m_lod_level;
	}
	Info m_mesh_info;
private:

	float GetTranslationAnimationValue(int frame, std::vector<float>& translationValues);

	void SetTranslationAnimation(FbxAnimCurve *linearCurve, std::vector<float> &translationValues, std::shared_ptr<Animation> animationObject);
	void setStaticTranslationAnimation(FbxAnimCurve* linearCurve, double translationValue, std::shared_ptr<Animation> animationObject);
	
	fbxsdk::FbxAMatrix GetGlobalDefaultPosition(fbxsdk::FbxNode* pNode);

	EulerAngles ConvertCombineCompressQuat(Geometry::Vector4 DecompressedQuaterion, Skeleton::Bone BoneReference, bool isStatic = false);

	std::string m_object_name;
	
	std::vector<std::string> m_skeletons_names;
	std::vector<std::string> m_joint_names;
	std::vector<Vertex> m_vertices;
	std::vector<Geometry::Vector3> m_normals;
	std::vector<Geometry::Vector4> m_lighting_normals;
	std::vector<Shader_appliance> m_shaders;
	std::vector<std::pair<std::string, std::shared_ptr<Skeleton>>> m_used_skeletons;
	std::vector<Morph_target> m_morphs;
	uint32_t m_lod_level;
	
};
