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
	if (name == "0001SPHR")
	{
		float centerX = buffer.read_float();
		float centerY = buffer.read_float();
		float centerZ = buffer.read_float();

		float cRadius = buffer.read_float();

		std::vector<float> center{ centerX, centerY, centerZ };

		m_object->setRadiusPoint(cRadius);
		m_object->setCenter(center);
	}
	else if (name == "BOX ")
	{
		std::vector<float> maxValue;
		std::vector<float> minValue;

		maxValue.push_back(buffer.read_float());// X
		maxValue.push_back(buffer.read_float());// Y
		maxValue.push_back(buffer.read_float());// Z

		minValue.push_back(buffer.read_float());// X
		minValue.push_back(buffer.read_float());// Y
		minValue.push_back(buffer.read_float());// Z

		m_object->setMaxPoint(maxValue);
		m_object->setMinPoint(minValue);
	}
	else if (name == "0001NAME")
	{
		std::string Shader = buffer.read_stringz();
		m_object->setShaderName(Shader);
	}
	else if (name == "0001INFO")
	{
		static bool FirstInfoHit = false;
		if (!FirstInfoHit)
		{
			uint32_t flags = buffer.read_uint32();
			m_object->setFlags(flags);
			FirstInfoHit = true;
		}
		else
		{
			uint32_t primitiveType = buffer.read_uint32();

			m_object->SetPrimitiveType((ShaderPrimitiveType)primitiveType);

			bool hasIndicies = buffer.read_uint8();
			bool hasSortedIndicies = buffer.read_uint8();
		}
		
	}
	else if (name == "DATA")
	{
		bool skipDot2 = false;
		int numberOfTextureCoordinateSets = m_object->GetNumTextureCoordinateSets();
		if (numberOfTextureCoordinateSets > 0 && m_object->getCoordinateSet(numberOfTextureCoordinateSets - 1) == 4)
		{
			numberOfTextureCoordinateSets--;
			m_object->SetTextreCoordinateDim(numberOfTextureCoordinateSets, 1);
			m_object->SetNumberOfTextureCoordinateSets(numberOfTextureCoordinateSets);
			skipDot2 = true;
		}
		// Triangle vetexs
		while (!buffer.end_of_buffer())
		{
			if (m_object->hasPosition())
			{
				std::vector<float> triPoints;

				float x = buffer.read_float();
				float y = buffer.read_float();
				float z = buffer.read_float();

				triPoints.push_back(x);
				triPoints.push_back(y);
				triPoints.push_back(z);

				m_object->AddVertex(triPoints);
			}
			
			if (m_object->isTransformed())
			{
				float transformFloat = buffer.read_float();
			}
				
			if (m_object->hasNormals())
			{
				std::vector<float> parsedNormal;

				float xNormal = buffer.read_float();
				float yNormal = buffer.read_float();
				float zNormal = buffer.read_float();

				parsedNormal.push_back(xNormal);
				parsedNormal.push_back(yNormal);
				parsedNormal.push_back(zNormal);

				m_object->addNormal(parsedNormal);
			}
			
			if (m_object->hasPointSize())
			{
				float pointSize = buffer.read_float();
			}
				

			if (m_object->hasColor0())
			{
				uint32_t color0 = buffer.read_uint32();
			}
			
			if (m_object->hasColor1())
			{
				uint32_t color1 = buffer.read_uint32();
			}
			
			std::vector<std::vector<float>> textureCoordinatePrime;

			for (int i = 0; i < m_object->GetNumTextureCoordinateSets(); i++)
			{
				std::vector<float> textureCoordinateSemiPrime;
				for (int j = 0; j < m_object->getCoordinateSet(i); j++)
				{
					textureCoordinateSemiPrime.push_back(buffer.read_float());
				}
				textureCoordinatePrime.push_back(textureCoordinateSemiPrime);
			}

			if (skipDot2)
			{
				buffer.read_float();
				buffer.read_float();
				buffer.read_float();
				buffer.read_float();
			}
		}
	}
	else if (name == "INDX")
	{
		uint32_t indexCount = buffer.read_uint32();

		while (!buffer.end_of_buffer())
		{
			uint16_t indexValue = buffer.read_uint16();
			m_object->getIndexArray().push_back(indexValue);
		}
	}
	else if (name == "SIDX")
	{
		uint32_t indexCount = buffer.read_uint32();
		while (!buffer.end_of_buffer())
		{
			// Read direction of buffer
			std::vector<float> Coordinate{ buffer.read_float(), buffer.read_float(), buffer.read_float() };
			// Read in index value
			uint32_t indexValue = buffer.read_uint32();

			SortedIndex AdditionalIndex;
			AdditionalIndex.CoordinateDirection = Coordinate;
			AdditionalIndex.IndexValue = indexValue;

			m_object->GetSortedIndex().push_back(AdditionalIndex);
		}
	}
}

bool meshParser::is_object_parsed() const
{
	return true;
}


void meshObject::store(const std::string& path, const Context& context)
{

}