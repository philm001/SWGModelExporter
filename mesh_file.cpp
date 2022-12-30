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
		/* Keeping here for historicas/archival purposes */
		// Collision box. This is not needed
		//std::vector<float> maxValues;
		//std::vector<float> minValues;

		//float maxX = buffer.read_float(); // confirm these are in correct order
		//float maxY = buffer.read_float();
		//float maxZ = buffer.read_float();

		//maxValues.push_back(maxX);
		//maxValues.push_back(maxY);
		//maxValues.push_back(maxZ);

		//float minX = buffer.read_float(); // confirm these are in correct order
		//float minY = buffer.read_float();
		//float minZ = buffer.read_float();

		//minValues.push_back(minX);
		//minValues.push_back(minY);
		//minValues.push_back(minZ);

		//m_object->AddBoundBox(maxValues);
		//m_object->AddBoundBox(minValues);
	}
	else if (name == "0001SPHR")
	{
		/* Keeping here for historicas/archival purposes */
		// Collision Sphere, this is also not needed in fbx
		//std::vector<float> CenterPoint;

		//float cX = buffer.read_float(); // confirm these are in correct order
		//float cY = buffer.read_float();
		//float cZ = buffer.read_float();

		//CenterPoint.push_back(cX);
		//CenterPoint.push_back(cY);
		//CenterPoint.push_back(cZ);

		//float radius = buffer.read_float();

		//m_object->AddCenterPoint(CenterPoint);
		//m_object->AddRadius(radius);
	}
	/* AT this point, none of the sections after FLORDATA is needed. */
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
		/* Keeping here for historicas/archival purposes */
		/* These are not for triangles but for collision around the mesh*/
		//while (!buffer.end_of_buffer())
		//{
		//	std::vector<float> triPoints;

		//	float x = buffer.read_float();
		//	float y = buffer.read_float();
		//	float z = buffer.read_float();

		//	triPoints.push_back(x); // confirm the order
		//	triPoints.push_back(y);
		//	triPoints.push_back(z);
		//	                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 
		//	m_object->AddVertex(triPoints);
		//}
	}
	else if (name == "INDX")
	{
		/* Keeping here for historicas/archival purposes */
		// Triangles INdicies?
		/*while (!buffer.end_of_buffer())
		{
			uint32_t readValue = buffer.read_uint32();
			m_object->AddIndex(readValue);
		}*/
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
		/* Keeping here for historicas/archival purposes */
		/* More collision stuff */
		/*float centerX = buffer.read_float();
		float centerY = buffer.read_float();
		float centerZ = buffer.read_float();

		float cRadius = buffer.read_float();

		std::vector<float> center{ centerX, centerY, centerZ };

		m_object->setRadiusPoint(cRadius);
		m_object->setCenter(center);*/
	}
	else if (name == "BOX ")
	{
		/* Keeping here for historicas/archival purposes */
		/* More collision stuff */
		//std::vector<float> maxValue;
		//std::vector<float> minValue;

		//maxValue.push_back(buffer.read_float());// X
		//maxValue.push_back(buffer.read_float());// Y
		//maxValue.push_back(buffer.read_float());// Z

		//minValue.push_back(buffer.read_float());// X
		//minValue.push_back(buffer.read_float());// Y
		//minValue.push_back(buffer.read_float());// Z

		//m_object->setMaxPoint(maxValue);
		//m_object->setMinPoint(minValue);
	}
	//else if (name.find("CNT") != -1)
	else if(name == "0001CNT ")
	{
		uint16_t shaderCount = buffer.read_uint16();
		m_object->SetShaderCount(shaderCount);
	}
	else if (name.find("NAME") != -1)
	{
		/* Grab all of the shaders */
		static uint16_t shaderLoop = 0;
		if (shaderLoop < m_object->GetShaderCount())
		{
			shaderLoop++;
		}
		std::stringstream ss;
		ss << std::setw(4) << std::setfill('0') << shaderLoop;
		std::string s = ss.str();
		s += "NAME";
		if (name == s)
		{
			std::string Shader = buffer.read_stringz();
			m_object->add_new_shader(Shader);
		}

		if (shaderLoop == m_object->GetShaderCount())
			shaderLoop = 0; // Reset the counter back to 0
	}
	else if (name == "0000INFO")
	{
		/* Note sure about this one.... */
		/*static bool FirstInfoHit = false;
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
		}*/

		/* For this one, this is a flag to indicate if we need to read the index as 32 or 16 bit ints*/
		m_object->get_current_shader().set32BitIndexState(true);

	}
	else if (name == "0003INFO")
	{
		uint32_t flags = buffer.read_uint32();
		uint32_t numVerticies = buffer.read_uint32();
		m_object->get_current_shader().SetFlags(flags);
		m_object->get_current_shader().setNumberofMeshVertices(numVerticies);
	}
	/* So far the 0002 and 0001 Info are not needed. Break points were never but keeping here just in case */
	else if (name == "0002INFO")
	{
		std::cout << "Need to account for this";
	}
	else if (name == "0001INFO")
	{
		m_object->get_current_shader().AddInfoCounter();
		if(m_object->get_current_shader().getInfoCounter() == 2)
			std::cout << "Need to account for this";
	}
	else if (name == "DATA")
	{
		bool skipDot3 = false;

		if (buffer.get_size() == 4704)
		{
			std::cout << "Break";
		}
		
		int numberOfTextureCoordinateSets = m_object->get_current_shader().GetNumTextureCoordinateSets();
		int tempValue = m_object->get_current_shader().getCoordinateSet(numberOfTextureCoordinateSets - 1);
		if (numberOfTextureCoordinateSets > 0 && m_object->get_current_shader().getCoordinateSet(numberOfTextureCoordinateSets - 1) == 4)/* Might need to add another statement for GraphicsOptionTags::get(TAG_DOT3) == false*/
		{
			numberOfTextureCoordinateSets--;
			m_object->get_current_shader().SetTextureCoordinateDim(numberOfTextureCoordinateSets, 1);
			m_object->get_current_shader().SetNumberOfTextureCoordinateSets(numberOfTextureCoordinateSets);
			skipDot3 = true;
		}

		// Triangle vetexs
		while (!buffer.end_of_buffer())
		{
			static uint32_t tempCounter = 0;
			
			if (m_object->get_current_shader().hasPosition())
			{
				std::vector<float> triPoints;

				float x = buffer.read_float();
				float y = buffer.read_float();
				float z = buffer.read_float();

				triPoints.push_back(x);
				triPoints.push_back(y);
				triPoints.push_back(z);

				m_object->get_current_shader().AddVertex(triPoints);
			}
			
			if (m_object->get_current_shader().isTransformed())
			{
				float transformFloat = buffer.read_float();
			}
				
			if (m_object->get_current_shader().hasNormals())
			{
				std::vector<float> parsedNormal;

				float xNormal = buffer.read_float();
				float yNormal = buffer.read_float();
				float zNormal = buffer.read_float();

				parsedNormal.push_back(xNormal);
				parsedNormal.push_back(yNormal);
				parsedNormal.push_back(zNormal);

				m_object->get_current_shader().AddNormal(parsedNormal);
			}
			
			if (m_object->get_current_shader().hasPointSize())
			{
				float pointSize = buffer.read_float();
			}
				

			if (m_object->get_current_shader().hasColor0())
			{
				uint32_t color0 = buffer.read_uint32();
			}
			
			if (m_object->get_current_shader().hasColor1())
			{
				uint32_t color1 = buffer.read_uint32();
			}
			
			std::vector<std::vector<float>> textureCoordinatePrime;
			std::vector<Graphics::Tex_coord> textureCoordinate;

			// This is most likely the UVs
			for (int i = 0; i < numberOfTextureCoordinateSets; i++)
			{
				std::vector<Graphics::Tex_coord> readTextureCoordinates;
				float uValue = 0;
				float vValue = 0;
				
				for (int j = 0; j < m_object->get_current_shader().getCoordinateSet(i); j++)
				{
					if (j == 0)
					{
						uValue = buffer.read_float();
					}
					else
					{
						vValue = 1.0f - buffer.read_float();
					}
				}

				Graphics::Tex_coord currentValue(uValue, vValue);

				if (m_object->GetShader().GetTexelArray().size() < i + 1)
				{
					std::vector<Graphics::Tex_coord> textureCoordinate;
					textureCoordinate.push_back(currentValue);
					m_object->GetShader().GetTexelArray().push_back(textureCoordinate);
				}
				else
				{
					m_object->GetShader().GetTexelArray().at(i).push_back(currentValue);
				}			
			}

			if (skipDot3)
			{
				/* We are going to read blank data? */
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
			/* There could be a better way of doing this*/
			if (m_object->get_current_shader().get32BitIndexState())
			{
				uint32_t v1 = buffer.read_uint32();
				uint32_t v2 = buffer.read_uint32();
				uint32_t v3 = buffer.read_uint32();
				Graphics::Triangle_indexed tempTri(v1, v2, v3);
				m_object->get_current_shader().get_triangles().emplace_back(tempTri);
			}
			else
			{
				uint16_t v1 = buffer.read_uint16();
				uint16_t v2 = buffer.read_uint16();
				uint16_t v3 = buffer.read_uint16();
				Graphics::Triangle_indexed tempTri(v1, v2, v3);
				m_object->get_current_shader().get_triangles().emplace_back(tempTri);
			}
			
		}
		int stop = 32;
	}
	else if (name == "SIDX")
	{
	/* For exporting, this does not matter. However, the code will remain here for archival purposes*/
		//uint32_t indexCount = buffer.read_uint32();
		//while (!buffer.end_of_buffer())
		//{
		//	// Read direction of buffer
		//	std::vector<float> Coordinate{ buffer.read_float(), buffer.read_float(), buffer.read_float() };
		//	// Read in index value
		//	uint32_t indexValue = buffer.read_uint32();// Confirm this is the correct order

		//	SortedIndex AdditionalIndex;
		//	AdditionalIndex.CoordinateDirection = Coordinate;
		//	AdditionalIndex.IndexValue = indexValue;

		//	// After reading index value will need to read through for indice values
		//	for (int i = 0; i < indexValue; i++)
		//	{
		//		AdditionalIndex.Indicie.push_back(buffer.read_uint16());
		//	}

		//	m_object->GetSortedIndex().push_back(AdditionalIndex);
		//}
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

	uint32_t meshVerticies = 0;
	uint32_t triangleVertices = 0;

	for (auto shader : m_shaders)
	{
		meshVerticies += shader.getNumberofMeshVertices();
		triangleVertices += shader.GetTriangleVertices().size();
	}

	mesh_ptr->SetControlPointCount(meshVerticies);
	auto mesh_vertices = mesh_ptr->GetControlPoints();

	uint32_t lastTriCount = 0;

	for (auto shader : m_shaders)
	{
		for (uint32_t vertex_id = 0; vertex_id < shader.GetTriangleVertices().size(); vertex_id++)
		{
			/*
			m_triangleVertices[vertex_id].at(0) - x
			m_triangleVertices[vertex_id].at(1) - y
			m_triangleVertices[vertex_id].at(2) - z
		*/
			mesh_vertices[vertex_id + lastTriCount] = FbxVector4(shader.GetTriangleVertices()[vertex_id].at(0), shader.GetTriangleVertices()[vertex_id].at(1), shader.GetTriangleVertices()[vertex_id].at(2));
		}
		lastTriCount += shader.GetTriangleVertices().size();
	}

	// add material layer
	auto material_layer = mesh_ptr->CreateElementMaterial();
	material_layer->SetMappingMode(FbxLayerElement::eByPolygon);
	material_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);

	// process polygons
	std::vector<uint32_t> normal_indexes;
	std::vector<std::vector<uint32_t>> tangent_idxs_prime;

	// New UV Code
	std::vector<std::vector<Graphics::Tex_coord>> primed_uvs;
	std::vector<std::vector<uint32_t>> uv_indexes;

	uint32_t triangleCounter = 0;
	uint32_t tempCounter = 0;

	for (uint32_t shader_idx = 0; shader_idx < m_shaders.size(); ++shader_idx)
	{
		auto& shader = m_shaders.at(shader_idx);
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

			for (uint32_t tri_idx = 0; tri_idx < triangles.size(); ++tri_idx)
			{
				auto& tri = triangles[tri_idx];
				mesh_ptr->BeginPolygon(shader_idx, -1, shader_idx, false);
				for (size_t i = 0; i < 3; ++i)
				{
					//auto remapped_pos_idx = positions[tri.points[i]];
					uint32_t tempValue = tri.points[i] + triangleCounter;
					mesh_ptr->AddPolygon(tri.points[i] + triangleCounter);

					//auto remapped_normal_idx = normals[tri.points[i]];
					normal_indexes.emplace_back(tri.points[i] + triangleCounter);
				

					/* So the normal lightening does not do anything but in the parser, there was a version where it gets the tangents.
						And the tangets are stored in the normal lighening. However, everything is now being stored in the UVs and the 
						normal lighening is not used. This may change at a future version so will keep the logic here but commented out.
					*/
					//if (shader.getNormalLighting().size() > 0)
					//{
					//	if (tangent_idxs_prime.size() < shader_idx + 1)
					//	{
					//		std::vector<uint32_t> initialValue;
					//		tangent_idxs_prime.push_back(initialValue);
					//	}

					//	tangent_idxs_prime.at(shader_idx).push_back(tri.points[i] + triangleCounter); // First we will try it with this one
					//}

					for (int j = 0; j < shader.GetTexelArray().size(); j++)
					{
						uint32_t offsetValue = 0;

						if (primed_uvs.size() != 0 && primed_uvs.size() > j)
						{
							offsetValue = primed_uvs.at(j).size();
						}
						
						if (uv_indexes.size() < j + 1)
						{
							std::vector<uint32_t> initialValue;
							initialValue.push_back(offsetValue + tri.points[i]);
							uv_indexes.push_back(initialValue);
						}
						else
						{
							uv_indexes.at(j).emplace_back(offsetValue + tri.points[i]);
						}
					}
				}
				mesh_ptr->EndPolygon();
			}

			for (int j = 0; j < shader.GetTexelArray().size(); j++)
			{
				if (primed_uvs.size() < j + 1)
				{
					std::vector<Graphics::Tex_coord> initialValue;
					primed_uvs.push_back(initialValue);
				}

				copy(shader.GetTexelArray().at(j).begin(), shader.GetTexelArray().at(j).end(), back_inserter(primed_uvs.at(j)));

			}

			triangleCounter += shader.getNumberofMeshVertices();
		}
	}

	triangleCounter = 0;

	// add UVs
	for (int i = 0; i < primed_uvs.size(); i++)
	{
		std::stringstream ss;
		ss << i;
		std::string str = "UVSet" + ss.str();

		FbxGeometryElementUV* uv_ptr = mesh_ptr->CreateElementUV(str.c_str());
		uv_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
		uv_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

		std::for_each(primed_uvs.at(i).begin(), primed_uvs.at(i).end(), [&uv_ptr](const Graphics::Tex_coord& coord)
			{
				uv_ptr->GetDirectArray().Add(FbxVector2(coord.u, coord.v));
			});

		std::for_each(uv_indexes.at(i).begin(), uv_indexes.at(i).end(), [&uv_ptr](const uint32_t idx)
			{
				uv_ptr->GetIndexArray().Add(idx);
			});
	}
	
	// add normals
	FbxGeometryElementNormal* normals_ptr = mesh_ptr->CreateElementNormal();
	normals_ptr->SetMappingMode(FbxGeometryElementNormal::eByPolygonVertex);
	normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
	auto& direct_array = normals_ptr->GetDirectArray();
	for (auto shader : m_shaders)
	{
		std::for_each(shader.get_normals().begin(), shader.get_normals().end(),
			[&direct_array](const std::vector<float>& elem)
			{
				direct_array.Add(FbxVector4(elem.at(0), elem.at(1), elem.at(2)));
			});
	}
	

	auto& index_array = normals_ptr->GetIndexArray();
	std::for_each(normal_indexes.begin(), normal_indexes.end(),
		[&index_array](const uint32_t& idx) { index_array.Add(idx); });

	

	// Add tangents if they are present
	/* Again commenting it out for now. Maybe added in at a later time*/
	/*if (tangent_idxs_prime.size() > 0)
	{
		for (uint32_t shader_idx = 0; shader_idx < m_shaders.size(); ++shader_idx)
		{
			auto& shader = m_shaders.at(shader_idx);
			std::stringstream ss;
			ss << shader_idx;
			std::string str = "UVSet" + ss.str();

			FbxGeometryElementTangent* normals_ptr = mesh_ptr->CreateElementTangent();
			normals_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
			normals_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
			auto& direct_array = normals_ptr->GetDirectArray();

			std::for_each(shader.getNormalLighting().begin(), shader.getNormalLighting().end(),
				[&direct_array](const Geometry::Vector4& elem)
				{
					direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
				});

			std::for_each(tangent_idxs_prime.at(shader_idx).begin(), tangent_idxs_prime.at(shader_idx).end(),
				[&index_array](const uint32_t& idx) { index_array.Add(idx); });
		}
	}*/

	mesh_ptr->BuildMeshEdgeArray();

	// Smoothing? Might need to look here: https://forums.autodesk.com/t5/fbx-forum/how-to-create-a-smoothing-group/td-p/4245673

	/*FbxLayerElementSmoothing* lLayerElementSmoothing = FbxLayerElementSmoothing::Create(mesh_ptr, "smoothing");
	lLayerElementSmoothing->SetMappingMode(FbxLayerElement::eByPolygon);
	lLayerElementSmoothing->SetReferenceMode(FbxLayerElement::eDirect);
	FbxLayer* testLayer = mesh_ptr->GetLayer(0);

	testLayer->SetSmoothing(lLayerElementSmoothing);*/

	exporter_ptr->Export(scene_ptr);
	fbx_manager_ptr->Destroy();
}


void meshObject::resolve_dependencies(const Context& context)
{
	for (auto it_shad = m_shaders.begin(); it_shad != m_shaders.end(); ++it_shad)
	{
		auto obj_it = context.object_list.find(it_shad->get_name());
		if (obj_it != context.object_list.end() && std::dynamic_pointer_cast<Shader>(obj_it->second))
			it_shad->set_definition(std::dynamic_pointer_cast<Shader>(obj_it->second));
	}

	auto bad_shaders = any_of(m_shaders.begin(), m_shaders.end(),
		[](const Shader_appliance& shader) { return shader.get_definition() == nullptr; });

	if (bad_shaders)
	{
		// So far I haven't ran into bad shaders. If this becomes an issue, look at what is being done on the skeletal mesh side of things
		std::cout << "Bad shaders breakpoint" << std::endl;
	}
}