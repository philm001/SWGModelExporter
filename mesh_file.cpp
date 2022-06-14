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
			uint32_t indexValue = buffer.read_uint32();// Confirm this is the correct order

			SortedIndex AdditionalIndex;
			AdditionalIndex.CoordinateDirection = Coordinate;
			AdditionalIndex.IndexValue = indexValue;

			// After reading index value will need to read through for indice values
			for (int i = 0; i < indexValue; i++)
			{
				AdditionalIndex.Indicie.push_back(buffer.read_uint16());
			}

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
	boost::filesystem::path obj_name(m_name);
	boost::filesystem::path target_path(path);
	target_path /= obj_name.filename();
	target_path.replace_extension("fbx");

	// init FBX manager
	FbxManager* fbx_manager_ptr = FbxManager::Create();
	if (fbx_manager_ptr == nullptr)
		return;

	FbxIOSettings* ios_ptr = FbxIOSettings::Create(fbx_manager_ptr, IOSROOT);
	ios_ptr->SetIntProp(EXP_FBX_EXPORT_FILE_VERSION, FBX_FILE_VERSION_7400);
	fbx_manager_ptr->SetIOSettings(ios_ptr);

	FbxExporter* exporter_ptr = FbxExporter::Create(fbx_manager_ptr, "");
	if (!exporter_ptr)
		return;

	exporter_ptr->SetFileExportVersion(FBX_2014_00_COMPATIBLE);
	bool result = exporter_ptr->Initialize(target_path.string().c_str(), -1, fbx_manager_ptr->GetIOSettings());
	if (!result)
	{
		auto status = exporter_ptr->GetStatus();
		std::cout << "FBX error: " << status.GetErrorString() << std::endl;
		return;
	}
	FbxScene* scene_ptr = FbxScene::Create(fbx_manager_ptr, m_name.c_str());
	if (!scene_ptr)
		return;

	scene_ptr->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
	auto scale_factor = scene_ptr->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
	FbxNode* mesh_node_ptr = FbxNode::Create(scene_ptr, "mesh_node"); //skeleton root?
	FbxMesh* mesh_ptr = FbxMesh::Create(scene_ptr, "mesh");

	mesh_node_ptr->SetNodeAttribute(mesh_ptr);
	scene_ptr->GetRootNode()->AddChild(mesh_node_ptr);

	// prepare vertices
	uint32_t vertices_num = static_cast<uint32_t>(m_triangleVertices.size());
	uint32_t normals_num = static_cast<uint32_t>(m_Normals.size());
	mesh_ptr->SetControlPointCount(vertices_num);
	auto mesh_vertices = mesh_ptr->GetControlPoints();

	for (uint32_t vertex_idx = 0; vertex_idx < vertices_num; ++vertex_idx)
	{
		const auto& pt = m_triangleVertices.at(vertex_idx);
		mesh_vertices[vertex_idx] = FbxVector4(pt.at(0), pt.at(1), pt.at(2));
	}

	// add material layer
	auto material_layer = mesh_ptr->CreateElementMaterial();
	material_layer->SetMappingMode(FbxLayerElement::eByPolygon);
	material_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);

	// process polygons
	std::vector<uint32_t> normal_indexes;
	std::vector<uint32_t> tangents_idxs;
	std::vector<Graphics::Tex_coord> uvs;
	std::vector<uint32_t> uv_indexes;

	for (uint32_t shader_idx = 0; shader_idx < m_shaders.size(); ++shader_idx)
	{
		auto& shader = m_shaders[shader_idx];
		if (shader.get_definition())
		{
			auto material_ptr = FbxSurfacePhong::Create(scene_ptr, shader.get_name().c_str());
			material_ptr->ShadingModel.Set("Phong");

			auto& material = shader.get_definition()->material();
			auto& textures = shader.get_definition()->textures();

			// create material for this shader
			material_ptr->Ambient.Set(FbxDouble3(material.ambient.r, material.ambient.g, material.ambient.g));
			material_ptr->Diffuse.Set(FbxDouble3(material.diffuse.r, material.diffuse.g, material.diffuse.g));
			material_ptr->Emissive.Set(FbxDouble3(material.emissive.r, material.emissive.g, material.emissive.g));
			material_ptr->Specular.Set(FbxDouble3(material.specular.r, material.specular.g, material.specular.g));

			// add texture definitions
			for (auto& texture_def : textures)
			{
				FbxFileTexture* texture = FbxFileTexture::Create(scene_ptr, texture_def.tex_file_name.c_str());
				boost::filesystem::path tex_path(path);
				tex_path /= texture_def.tex_file_name;
				tex_path.replace_extension("tga");

				texture->SetFileName(tex_path.string().c_str());
				texture->SetTextureUse(FbxTexture::eStandard);
				texture->SetMaterialUse(FbxFileTexture::eModelMaterial);
				texture->SetMappingType(FbxTexture::eUV);
				texture->SetWrapMode(FbxTexture::eRepeat, FbxTexture::eRepeat);
				texture->SetTranslation(0.0, 0.0);
				texture->SetScale(1.0, 1.0);
				texture->SetTranslation(0.0, 0.0);
				switch (texture_def.texture_type)
				{
				case Shader::texture_type::main:
					material_ptr->Diffuse.ConnectSrcObject(texture);
					break;
				case Shader::texture_type::normal:
					material_ptr->Bump.ConnectSrcObject(texture);
					break;
				case Shader::texture_type::specular:
					material_ptr->Specular.ConnectSrcObject(texture);
					break;
				}
			}

			mesh_ptr->GetNode()->AddMaterial(material_ptr);

			// get geometry element
			auto& triangles = shader.get_triangles();
			auto& positions = shader.get_pos_indexes();
			auto& normals = shader.get_normal_indexes();
			auto& tangents = shader.get_light_indexes();

			auto idx_offset = static_cast<uint32_t>(uvs.size());
			copy(shader.get_texels().begin(), shader.get_texels().end(), back_inserter(uvs));

			normal_indexes.reserve(normal_indexes.size() + normals.size());
			tangents_idxs.reserve(tangents_idxs.size() + tangents.size());

			for (uint32_t tri_idx = 0; tri_idx < triangles.size(); ++tri_idx)
			{
				auto& tri = triangles[tri_idx];
				mesh_ptr->BeginPolygon(shader_idx, -1, shader_idx, false);
				for (size_t i = 0; i < 3; ++i)
				{
					auto remapped_pos_idx = positions[tri.points[i]];
					mesh_ptr->AddPolygon(remapped_pos_idx);

					auto remapped_normal_idx = normals[tri.points[i]];
					normal_indexes.emplace_back(remapped_normal_idx);

					if (!tangents.empty())
					{
						auto remapped_tangent = tangents[tri.points[i]];
						tangents_idxs.emplace_back(remapped_tangent);
					}
					uv_indexes.emplace_back(idx_offset + tri.points[i]);
				}
				mesh_ptr->EndPolygon();
			}
		}
	}
	// add UVs
	FbxGeometryElementUV* uv_ptr = mesh_ptr->CreateElementUV("UVSet1");
	uv_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	uv_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	std::for_each(uvs.begin(), uvs.end(), [&uv_ptr](const Graphics::Tex_coord& coord)
		{
			uv_ptr->GetDirectArray().Add(FbxVector2(coord.u, coord.v));
		});
	std::for_each(uv_indexes.begin(), uv_indexes.end(), [&uv_ptr](const uint32_t idx)
		{
			uv_ptr->GetIndexArray().Add(idx);
		});

	// add normals
	if (!normal_indexes.empty())
	{
		FbxGeometryElementNormal* normals_ptr = mesh_ptr->CreateElementNormal();
		normals_ptr->SetMappingMode(FbxGeometryElementNormal::eByPolygonVertex);
		normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		auto& direct_array = normals_ptr->GetDirectArray();
		std::for_each(m_normals.begin(), m_normals.end(),
			[&direct_array](const Geometry::Vector3& elem)
			{
				direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
			});

		auto& index_array = normals_ptr->GetIndexArray();
		std::for_each(normal_indexes.begin(), normal_indexes.end(),
			[&index_array](const uint32_t& idx) { index_array.Add(idx); });
	}

	// add tangents
	if (!tangents_idxs.empty())
	{
		FbxGeometryElementTangent* normals_ptr = mesh_ptr->CreateElementTangent();
		normals_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		auto& direct_array = normals_ptr->GetDirectArray();
		std::for_each(m_lighting_normals.begin(), m_lighting_normals.end(),
			[&direct_array](const Geometry::Vector4& elem)
			{
				direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
			});

		auto& index_array = normals_ptr->GetIndexArray();
		std::for_each(tangents_idxs.begin(), tangents_idxs.end(),
			[&index_array](const uint32_t& idx) { index_array.Add(idx); });
	}

	mesh_ptr->BuildMeshEdgeArray();
}