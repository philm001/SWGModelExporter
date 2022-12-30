#pragma once
#include "objects/base_object.h"
#include "IFF_file.h"
#include "PrimitiveType.h"
#include "objects/animated_object.h"
#include "objects/geometry_common.h"

#include <sstream>

#include <fbxsdk.h>

struct SortedIndex
{
	std::vector<float> CoordinateDirection;
	uint32_t IndexValue;
	std::vector<uint16_t> Indicie;
};

class LODFileList : public Base_object
{
public:
	LODFileList() { }

	void AddLODFile(const std::string& name)
	{
		auto returnValue = std::find(m_LODFile.begin(), m_LODFile.end(), name);
		if (returnValue == m_LODFile.end() || m_LODFile.empty())
			m_LODFile.push_back(name); // Make sure there are duplicates
	}
	// overrides
	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override { };

	virtual std::set<std::string> get_referenced_objects() const override
	{
		std::set<std::string> names;

		names.insert(m_LODFile.begin(), m_LODFile.end());

		return move(names);
	};



	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }
private:
	std::vector<std::string> m_LODFile;
	std::string m_name;
};


class LODObject : public Base_object // This object contains data related to the LOD file listings. The boxes inside are meant for collision
{
public:
	LODObject() { }

	void AddLODName(const std::string& name)
	{
		auto returnValue = std::find(m_LODName.begin(), m_LODName.end(), name);
		if (returnValue == m_LODName.end() || m_LODName.empty())
			m_LODName.push_back(name); // Make sure there are duplicates
	}

	void AddBoundBox(std::vector<float> vec3)
	{
		m_BoundingBox.push_back(vec3);
	}

	void AddCenterPoint(std::vector<float> vec3)
	{
		m_centerPoints.push_back(vec3);
	}

	void AddRadius(float radius)
	{
		m_sphereRadius.push_back(radius);
	}

	void AddVertex(std::vector<float> vec3)
	{
		m_triangleVertices.push_back(vec3);
	}

	void AddIndex(uint32_t index)
	{
		m_indexList.push_back(index);
	}

	// overrides
	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override { };

	virtual std::set<std::string> get_referenced_objects() const override
	{
		std::set<std::string> names;

		names.insert(m_LODName.begin(), m_LODName.end());

		return move(names);
	};

	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }
private:
	/* Some of these items, I will keep here for any future updates */
	std::vector<std::string> m_LODName;
	std::string m_name;

	std::vector<std::vector<float>> m_BoundingBox;
	std::vector<std::vector<float>> m_centerPoints;
	std::vector<float> m_sphereRadius;
	std::vector<std::vector<float>> m_triangleVertices;
	std::vector<uint32_t> m_indexList;
};

/* 
	The LOD parser is only meant for Level of detail. It does not contain any mesh data.
	The vertices and indicies that it contains are related to collision which for FBX this does not matter
*/
class LODParser : public IFF_visitor
{
public:
	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;

	virtual bool is_object_parsed() const override;
	std::shared_ptr<Base_object> get_parsed_object() const { return std::dynamic_pointer_cast<Base_object>(m_object); }

private:
	std::shared_ptr<LODObject> m_object;
};

struct StaticMeshVertexInfo
{
	std::vector<float> triangleVertices;
	std::vector<float> triangleNormals;
	std::vector<Graphics::Tex_coord> UVs;
};

class meshObject : public Base_object // This object contains shader data
{
public:
	/* First class definitions */

	class Shader_appliance
	{
	public:
		Shader_appliance()
		{
			m_texels_array.reserve(8);
		}

		Shader_appliance(const std::string& name) : m_name(name) { }
		const std::string& get_name() const { return m_name; }

		std::vector<uint32_t>& get_light_indexes() { return m_light_indexes; }
		std::vector<Graphics::Triangle_indexed>& get_triangles() { return m_triangles; }
		std::vector<std::vector<float>>& get_normals() { return m_Normals; }
		std::vector <std::vector<Graphics::Tex_coord>>& GetTexelArray() { return m_texels_array; }

		void add_lighting_normal(const Geometry::Vector4& light_norm) { m_lighting_normals.emplace_back(light_norm); }
		std::vector<Geometry::Vector4>& getNormalLighting() { return m_lighting_normals; }

		void set_definition(const std::shared_ptr<Shader> shader_def) { m_shader_definition = shader_def; }
		const std::shared_ptr<Shader>& get_definition() const { return m_shader_definition; }

		void SetFlags(uint32_t flags) { m_flags = flags; }

		int GetNumTextureCoordinateSets() { return (m_flags >> 8) & 0x0F; }

		int getCoordinateSet(int textureCoordinateSet)
		{
			int shiftValue = 12 + (textureCoordinateSet * 2);
			return ((m_flags >> shiftValue) & 0x03) + 1;
		}

		bool hasPosition() { return (m_flags & 0x00000001) != 0; }

		bool isTransformed() { return (m_flags & 0x00000002) != 0; }

		bool hasNormals() { return(m_flags & 0x00000004) != 0; }

		bool hasPointSize() { return (m_flags & 0x00000020) != 0; }

		bool hasColor0() { return (m_flags & 0x00000008) != 0; }

		bool hasColor1() { return (m_flags & 0x00000010) != 0; }

		void SetTextureCoordinateDim(int textureCoordinateSet, int dim)
		{
			const unsigned int shift = static_cast<unsigned int>(12 + (textureCoordinateSet * 2));
			m_flags = (m_flags & ~(static_cast<unsigned int>(0x03) << shift)) | (static_cast<uint32_t>(dim - 1) << shift);
		}

		void SetNumberOfTextureCoordinateSets(int numberofTextures)
		{
			m_flags = (m_flags & ~(0b1111 << 8)) | (static_cast<uint32_t>(numberofTextures) << 8);
		}

		void setNumberofMeshVertices(uint32_t number) { m_NumberofMeshVertices = number; }
		uint32_t getNumberofMeshVertices() { return m_NumberofMeshVertices; }

		void AddVertex(std::vector<float> vec3) { m_triangleVertices.push_back(vec3); }
		void AddNormal(std::vector<float> vec3) { m_Normals.push_back(vec3); }

		std::vector<std::vector<float>>& GetTriangleVertices() { return m_triangleVertices; }

		bool get32BitIndexState() { return m_is32BitIndex; }
		void set32BitIndexState(bool state) { m_is32BitIndex = state; }

		int getInfoCounter() { return m_InfoCounter; }
		void AddInfoCounter() { m_InfoCounter++; }
	private:
		/* Some of these items, I will keep here for any future updates */
		std::string m_name;

		std::vector<uint32_t> m_light_indexes;

		std::vector <std::vector<Graphics::Tex_coord>> m_texels_array;
		std::vector<Graphics::Triangle_indexed> m_triangles;

		std::shared_ptr<Shader> m_shader_definition;
		std::vector<std::vector<float>> m_triangleVertices;
		std::vector<std::vector<float>> m_Normals;
		std::vector<Geometry::Vector4> m_lighting_normals;
		

		uint32_t m_flags;
		uint32_t m_NumberofMeshVertices = 0;

		bool m_is32BitIndex = false;
		int m_InfoCounter = 0;
	};


	/* Now functions */
	meshObject() { }

	// overrides
	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override;


	virtual std::set<std::string> get_referenced_objects() const override
	{
		std::set<std::string> names;

		std::for_each(m_shaders.begin(), m_shaders.end(),
			[&names](const Shader_appliance& shader) { names.insert(shader.get_name()); });

		return names;
	};

	virtual void resolve_dependencies(const Context& context) override;
	
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }
	Shader_appliance& get_current_shader() { assert(m_shaders.empty() == false); return m_shaders.back(); }

	void add_new_shader(const std::string& name) { m_shaders.emplace_back(name); }
	Shader_appliance &GetShader() { return m_shaders.back(); }

	void SetShaderCount(uint16_t value)
	{
		m_ShaderCount = value;
		m_shaders.reserve(value);
	}

	uint8_t GetShaderCount() { return m_ShaderCount; }
private:
	std::string m_name;
	std::vector<std::string> m_LODName;

	std::vector<Shader_appliance> m_shaders;

	int m_maxUVChannelCount = 0;

	uint16_t m_ShaderCount = 1;// all shaders static objects have a value of 1
};

class meshParser : public IFF_visitor
{
public:
	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;

	virtual bool is_object_parsed() const override;
	std::shared_ptr<Base_object> get_parsed_object() const { return std::dynamic_pointer_cast<Base_object>(m_object); }

private:
	std::shared_ptr<meshObject> m_object;
};

