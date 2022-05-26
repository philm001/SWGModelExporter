#pragma once
#include "objects/base_object.h"
#include "IFF_file.h"
#include "PrimitiveType.h"


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


class LODObject : public Base_object // This object contains mesh object
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
	std::vector<std::string> m_LODName;
	std::string m_name;

	std::vector<std::vector<float>> m_BoundingBox;
	std::vector<std::vector<float>> m_centerPoints;
	std::vector<float> m_sphereRadius;
	std::vector<std::vector<float>> m_triangleVertices;
	std::vector<uint32_t> m_indexList;
};


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


class meshObject : public Base_object // This object contains shader data
{
public:
	meshObject() { }

	// overrides
	virtual bool is_object_correct() const override { return true; }
	virtual void store(const std::string& path, const Context& context) override { };

	void setShaderName(std::string shaderName)
	{
		m_shaderName = shaderName;
	}

	void setCenter(std::vector<float> centerPoint)
	{
		m_centerPoint = centerPoint;
	}

	void setRadiusPoint(float radisuPoint)
	{
		m_radius = radisuPoint;
	}

	void setMaxPoint(std::vector<float> point)
	{
		m_maxPoint = point;
	}

	void setMinPoint(std::vector<float> point)
	{
		m_minPoint = point;
	}

	void AddVertex(std::vector<float> vec3)
	{
		m_triangleVertices.push_back(vec3);
	}

	virtual std::set<std::string> get_referenced_objects() const override
	{
		std::set<std::string> names;

		names.insert(m_shaderName);

		return move(names);
	};

	void setFlags(uint32_t flagSet)
	{
		m_flags = flagSet;
	}

	int GetNumTextureCoordinateSets()
	{
		return (m_flags >> 8) & 0xF;
	}

	int getCoordinateSet(int textureCoordinateSet)
	{
		int shiftValue = 12 + (textureCoordinateSet * 2);
		return ((m_flags >> shiftValue) & 0x3) + 1;
	}

	bool hasPosition()
	{
		return (m_flags & 0x00000001) != 0;
	}

	bool isTransformed()
	{
		return (m_flags & 0x00000002) != 0;
	}

	bool hasNormals()
	{
		return(m_flags & 0x00000004) != 0;
	}

	bool hasPointSize()
	{
		return (m_flags & 0x00000020) != 0;
	}

	bool hasColor0()
	{
		return (m_flags & 0x00000008) != 0;
	}

	bool hasColor1()
	{
		return (m_flags & 0x00000010) != 0;
	}

	void addNormal(std::vector<float> normal)
	{
		m_Normals.push_back(normal);
	}

	void SetPrimitiveType(ShaderPrimitiveType type)
	{
		m_primitiveType = type;
	}

	ShaderPrimitiveType getPrimitiveType()
	{
		return m_primitiveType;
	}

	void setIndiciesState(bool state)
	{
		m_hasIndicies = state;
	}

	bool getIndiciesState()
	{
		return m_hasIndicies;
	}

	void setIndiciesSortedState(bool state)
	{
		m_sortedIndicies = state;
	}

	bool getSortIndiciesState()
	{
		return m_sortedIndicies;
	}

	std::vector<uint16_t>& getIndexArray()
	{
		return m_indexArray;
	}

	virtual void resolve_dependencies(const Context&) override { }
	virtual void set_object_name(const std::string& name) override { m_name = name; }
	virtual std::string get_object_name() const override { return m_name; }
private:
	std::string m_name;
	std::vector<std::string> m_LODName;
	std::string m_shaderName;
	std::vector<float> m_maxPoint;
	std::vector<float> m_minPoint;

	std::vector<uint16_t> m_indexArray;

	std::vector<float> m_centerPoint;
	float m_radius = 0;

	std::vector<std::vector<float>> m_triangleVertices;
	std::vector<std::vector<float>> m_Normals;

	uint32_t m_flags;

	ShaderPrimitiveType m_primitiveType;

	bool m_hasIndicies = false;
	bool m_sortedIndicies = false;
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

