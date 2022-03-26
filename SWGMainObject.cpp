#include "stdafx.h"
#include "SWGMainObject.h"


void SWGMainObject::beginParsingProcess(std::queue<std::string> queueArray)
{
	p_queueArray = queueArray;
	
	while (!p_queueArray.empty())
	{
		std::string assetName;
		assetName = p_queueArray.front();
		p_queueArray.pop();

		// normalize path
		std::replace_if(assetName.begin(), assetName.end(), [](const char& value) { return value == '\\'; }, '/');
		std::string ext = assetName.substr(assetName.length() - 3);
		boost::to_lower(ext);

		// skip already parsed object
		if (p_Context.object_list.find(assetName) != p_Context.object_list.end())
			continue;

		std::cout << "Processing : " << assetName << std::endl;
		// do not try find object on this step
		std::vector<uint8_t> buffer;
		
		if (!p_Library->get_object(assetName, buffer))
			continue;

		//special processing for pure binary files (WAV, DDS, TGA, etc)
		if (ext == "dds")
		{
			auto texture = DDS_Texture::construct(assetName, buffer.data(), buffer.size());
			if (texture)
				p_Context.object_list.insert(std::make_pair(assetName, std::dynamic_pointer_cast<Base_object>(texture)));

			continue;
		}
		p_Buffer = IFF_file(buffer);

		
		p_Buffer.combinedObjectProcess(m_selected_parser);

		if (m_selected_parser->is_object_parsed())
		{
			auto object = m_selected_parser->get_parsed_object();
			if (object)
			{
				object->set_object_name(assetName);

				p_Context.object_list.insert(make_pair(assetName, object));

				if (ext == "mgn")
				{
					std::shared_ptr meshPtr = std::dynamic_pointer_cast<Animated_mesh>(object);
					Animated_mesh meshStore = *meshPtr;
					uint32_t level = meshStore.getLodLevel();

					switch (level)
					{
						case 0:
						{
							if (p_CompleteModels.size() == 0)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(0).push_back(meshStore);
							break;
						}
						case 1:
						{
							if (p_CompleteModels.size() == 1)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(1).push_back(meshStore);
							break;
						}
						case 2:
						{
							if (p_CompleteModels.size() == 2)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(2).push_back(meshStore);
							break;
						}
						case 3:
						{
							if (p_CompleteModels.size() == 3)
							{
								std::vector<Animated_mesh> firstOne;
								p_CompleteModels.push_back(firstOne);
							}
							p_CompleteModels.at(3).push_back(meshStore);
							break;
						}
						default:
							break;
					}

					int a = 0;
					a++;
				}

				if (ext == "mgn")
				{
					std::cout << "Here";
				}
				std::set<std::string> references_objects = object->get_referenced_objects();
				std::queue<std::string>& referenceArray = p_queueArray;
				Context& referenceContext = p_Context;

				std::for_each(references_objects.begin(), references_objects.end(),
					[&referenceContext, &assetName, &referenceArray](const std::string& object_name)
					{
						if (referenceContext.object_list.find(object_name) == referenceContext.object_list.end() &&
							referenceContext.unknown.find(object_name) == referenceContext.unknown.end())
						{
							referenceContext.opened_by[object_name] = assetName;
							referenceArray.push(object_name);
						}
					}
				);


			}
		}
		else
		{
			std::cout << "Objects of this type could not be converted at this time. Sorry!" << std::endl;
			p_Context.unknown.insert(p_fullName);
		}


	}

}


void SWGMainObject::CombineMeshProcess(std::shared_ptr<Animated_mesh> mainMesh, std::shared_ptr<Animated_mesh> secondaryMesh)
{
	std::shared_ptr<Animated_mesh> first = mainMesh;
	std::shared_ptr<Animated_mesh> second = secondaryMesh;

	std::vector<std::string> jointsToAdd;
	std::vector<Animated_mesh::Vertex>	verticiesToAdd;
	std::vector<Geometry::Vector3> normalsToAdd;
	std::vector<Geometry::Vector4> lightingNormalsToAdd;
	std::vector<Animated_mesh::Shader_appliance> shaderToAdd;

	for (auto jointIterator : secondaryMesh->get_joint_names())
	{
		bool jointFound = false;
		for (auto firstJointInterator : mainMesh->get_joint_names())
		{
			if (jointIterator == firstJointInterator)
			{
				jointFound = true;
				break;
			}
		}

		if (!jointFound)
		{
			jointsToAdd.push_back(jointIterator);
		}
	}

	for (auto addJoint : jointsToAdd)
	{
		mainMesh->add_joint_name(addJoint);
	}

	for (auto secondVertexIterator : secondaryMesh->get_vertices())
	{
		bool vertexFound = false;
		for (auto firstVertexInterator : mainMesh->get_vertices())
		{
			if (secondVertexIterator.isEqual(firstVertexInterator))
			{
				vertexFound = true;
				break;
			}
		}

		if (!vertexFound)
		{
			verticiesToAdd.push_back(secondVertexIterator);
		}
	}

	for (auto vertexAdd : verticiesToAdd)
	{
		mainMesh->addVertex(vertexAdd);
	}


	for (auto secondNormals : secondaryMesh->getNormals())
	{
		bool normalFound = false;
		for (auto firstNormalInterator : mainMesh->getNormals())
		{
			if (secondNormals == firstNormalInterator)
			{
				normalFound = true;
				break;
			}
		}

		if (!normalFound)
		{
			normalsToAdd.push_back(secondNormals);
		}
	}

	for (auto normalAdd : normalsToAdd)
	{
		mainMesh->add_normal(normalAdd);
	}

	for (auto lightingNormals : secondaryMesh->getNormalLighting())
	{
		bool normalFound = false;
		for (auto firstNormalInterator : mainMesh->getNormalLighting())
		{
			if (lightingNormals == firstNormalInterator)
			{
				normalFound = true;
				break;
			}
		}

		if (!normalFound)
		{
			lightingNormalsToAdd.push_back(lightingNormals);
		}
	}

	for (auto normalAdd : lightingNormalsToAdd)
	{
		mainMesh->add_lighting_normal(normalAdd);
	}

	for (auto secondaryShaders : secondaryMesh->getShaders())
	{
		bool shadersFound = false;
		for (auto firstShader : mainMesh->getShaders())
		{
			if (firstShader.get_name() == secondaryShaders.get_name())
			{
				shadersFound = true;
				break;
			}
		}

		if (!shadersFound)
		{
			shaderToAdd.push_back(secondaryShaders);
		}
	}

	for (auto shadersAdd : shaderToAdd)
	{
		mainMesh->addShader(shadersAdd);
	}

	int a = 0;
	a++;
}


void SWGMainObject::store(const std::string& path, const Context& context)
{
	Context& otherContext = p_Context;
	std::cout << "Resolve dependencies..." << std::endl;
	std::for_each(p_Context.object_list.begin(), p_Context.object_list.end(),
		[&otherContext](const std::pair<std::string, std::shared_ptr<Base_object>>& item)
		{
			std::cout << "Object : " << item.first;
			item.second->resolve_dependencies(otherContext);
			std::cout << " done." << std::endl;
		});

	// Possibly load up the mesh objects here????

	std::cout << "Store objects..." << std::endl;
	std::for_each(otherContext.object_list.begin(), otherContext.object_list.end(),
		[&path, &otherContext](const std::pair<std::string, std::shared_ptr<Base_object>>& item)
		{
			if (item.second->get_object_name().find("ans") == std::string::npos)
			{
				// Block the storage of the animated object
				std::cout << "Object : " << item.first;
				item.second->store(path, otherContext);
				std::cout << " done." << std::endl;
			}
			else
			{
				std::cout << "Does not support saving directly" << std::endl;
			}
		});

	boost::filesystem::path obj_name(p_CompleteModels.at(0).at(0).get_object_name());
	boost::filesystem::path target_path(path);
	target_path /= obj_name.filename();

	target_path.replace_extension("fbx");
	auto directory = target_path.parent_path();

	if (!boost::filesystem::exists(directory))
		boost::filesystem::create_directories(directory);

	if (boost::filesystem::exists(target_path))
		boost::filesystem::remove(target_path);

	int lodLevel = p_CompleteModels.at(0).at(0).getLodLevel();

	obj_name = obj_name.filename();
	obj_name.replace_extension();
	std::string name = obj_name.string();
	std::string mainObjectName = p_CompleteModels.at(0).at(0).get_object_name();


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
	FbxScene* scene_ptr = FbxScene::Create(fbx_manager_ptr, mainObjectName.c_str());
	if (!scene_ptr)
		return;

	scene_ptr->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
	auto scale_factor = scene_ptr->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
	FbxNode* mesh_node_ptr = FbxNode::Create(scene_ptr, "mesh_node"); //skeleton root?
	FbxMesh* mesh_ptr = FbxMesh::Create(scene_ptr, "mesh");

	mesh_node_ptr->SetNodeAttribute(mesh_ptr);
	scene_ptr->GetRootNode()->AddChild(mesh_node_ptr);

	uint32_t verticesNum = 0;
	uint32_t normalsNum = 0;

	for (auto modelIterator : p_CompleteModels.at(0))
	{
		verticesNum += modelIterator.get_vertices().size();
		normalsNum += modelIterator.getNormals().size();
	}

	mesh_ptr->SetControlPointCount(verticesNum);
	auto mesh_vertices = mesh_ptr->GetControlPoints();

	uint32_t counter = 0;
	uint32_t otherCounter = 0;

	for (auto modelIterator : p_CompleteModels.at(0))
	{
		for (uint32_t vertexCounter = 0; vertexCounter < modelIterator.get_vertices().size(); vertexCounter++)
		{
			const auto& pt = modelIterator.get_vertices()[vertexCounter].get_position();
			mesh_vertices[counter + vertexCounter] = FbxVector4(pt.x, pt.y, pt.z);
		}
		counter += modelIterator.get_vertices().size();
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
	counter = 0;

	for (auto modelIterator : p_CompleteModels.at(0))
	{
		for (uint32_t shader_idx = 0; shader_idx < modelIterator.getShaders().size(); shader_idx++)
		{
			auto& shader = modelIterator.getShaders().at(shader_idx);
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
							tangents.emplace_back(remapped_tangent);
						}
						uv_indexes.emplace_back(idx_offset + tri.points[i]);
					}
					mesh_ptr->EndPolygon();
				}
			}
		}

		counter += modelIterator.getShaders().size();
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

		for (auto modelIterator : p_CompleteModels.at(0))
		{
			std::for_each(modelIterator.getNormals().begin(), modelIterator.getNormals().end(),
				[&direct_array](const Geometry::Vector3& elem)
				{
					direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
				});
		}
		
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

		for (auto modelIterator : p_CompleteModels.at(0))
		{
			std::for_each(modelIterator.getNormalLighting().begin(), modelIterator.getNormalLighting().end(),
				[&direct_array](const Geometry::Vector4& elem)
				{
					direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
				});
		}
		
		auto& index_array = normals_ptr->GetIndexArray();
		std::for_each(tangents_idxs.begin(), tangents_idxs.end(),
			[&index_array](const uint32_t& idx) { index_array.Add(idx); });
	}

	mesh_ptr->BuildMeshEdgeArray();
	std::vector<Skeleton::Bone> BoneInfoList;

	for (auto modelIterator : p_CompleteModels.at(0))
	{

		uint32_t m_lod_level = modelIterator.getLodLevel();

		std::for_each(modelIterator.getSkeleton().begin(), modelIterator.getSkeleton().end(),
			[&scene_ptr, &mesh_node_ptr, &BoneInfoList, &m_lod_level, this](const std::pair<std::string, std::shared_ptr<Skeleton>>& item)
			{
				if (item.second->get_lod_count() > m_lod_level)
				{
					item.second->set_current_lod(m_lod_level);

					m_bones.at(m_lod_level).insert(m_bones.at(m_lod_level).end(), item.second->getBonesatLOD(0).begin(), item.second->getBonesatLOD(0).end());
				}
			});
	}


	
}

std::vector<Skeleton::Bone> generateSkeletonInScene()
{

}
