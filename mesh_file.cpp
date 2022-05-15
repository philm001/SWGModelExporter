#include "stdafx.h"
#include "mesh_file.h"


void LODParser::section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "DTLAFORM")
	{
		m_object = std::make_shared<LODObject>();
	}
}

void LODParser::parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size)
{
	base_buffer buffer(data_ptr, data_size);

	if (name == "DATACHLD" || name == "CHLD")
	{
		std::string LODString;
		while (!buffer.end_of_buffer())
		{
			LODString = buffer.read_stringz();
		}

		m_object->AddLODName(LODString);
	}
	else if (name == "BOX ")
	{
		std::vector<float> maxValues;
		std::vector<float> minValues;

		float maxX = buffer.read_float(); // confirm these are in correct order
		float maxY = buffer.read_float();
		float maxZ = buffer.read_float();

		maxValues.push_back(maxX);
		maxValues.push_back(maxY);
		maxValues.push_back(maxZ);

		float minX = buffer.read_float(); // confirm these are in correct order
		float minY = buffer.read_float();
		float minZ = buffer.read_float();

		minValues.push_back(minX);
		minValues.push_back(minY);
		minValues.push_back(minZ);

		m_object->AddBoundBox(maxValues);
		m_object->AddBoundBox(minValues);
	}
	else if (name == "0001SPHR")
	{
		std::vector<float> CenterPoint;

		float cX = buffer.read_float(); // confirm these are in correct order
		float cY = buffer.read_float();
		float cZ = buffer.read_float();

		CenterPoint.push_back(cX);
		CenterPoint.push_back(cY);
		CenterPoint.push_back(cZ);

		float radius = buffer.read_float();

		m_object->AddCenterPoint(CenterPoint);
		m_object->AddRadius(radius);
	}
	else if (name == "FLORDATA")
	{
		// do nothing for now
	}
	else if (name == "RADRINFO")
	{

	}
	else if (name == "TESTINFO")
	{

	}
	else if (name == "WRITEINFO")
	{

	}
	else if (name == "0000VERT")
	{
		// Triangle vetexs
		while (!buffer.end_of_buffer())
		{
			std::vector<float> triPoints;

			float x = buffer.read_float();
			float y = buffer.read_float();
			float z = buffer.read_float();

			triPoints.push_back(x); // confirm the order
			triPoints.push_back(y);
			triPoints.push_back(z);
			                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
			m_object->AddVertex(triPoints);
		}
	}
	else if (name == "INDX")
	{
		// Triangles INdicies?
		while (!buffer.end_of_buffer())
		{
			uint32_t readValue = buffer.read_uint32();
			m_object->AddIndex(readValue);
		}
	}
}

bool LODParser::is_object_parsed() const
{
	return true;
}


void meshParser::section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "MESHFORM")
	{
		m_object = std::make_shared<meshObject>();
	}
}

void meshParser::parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size)
{
	base_buffer buffer(data_ptr, data_size);
}

bool meshParser::is_object_parsed() const
{
	return true;
}