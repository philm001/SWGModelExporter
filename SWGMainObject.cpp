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
					// Might need to check for duplicates here
					m_bones.at(m_lod_level).insert(m_bones.at(m_lod_level).end(), item.second->getBonesatLOD(0).begin(), item.second->getBonesatLOD(0).end());
				}
			});
	}

	BoneInfoList = generateSkeletonInScene(scene_ptr, mesh_node_ptr);

	// build morph targets
	// prepare base vector
	auto total_vertices = m_vertices.size();

	FbxBlendShape* blend_shape_ptr = FbxBlendShape::Create(scene_ptr, "BlendShapes");
	for (const auto& morph : m_morphs)
	{
		FbxBlendShapeChannel* morph_channel = FbxBlendShapeChannel::Create(scene_ptr, morph.get_name().c_str());
		FbxShape* shape = FbxShape::Create(scene_ptr, morph_channel->GetName());

		shape->InitControlPoints(static_cast<int>(total_vertices));
		auto shape_vertices = shape->GetControlPoints();

		// copy base vertices to shape vertices
		for (size_t idx = 0; idx < total_vertices; ++idx)
		{
			auto& pos = m_vertices[idx].get_position();
			shape_vertices[idx].Set(pos.x, pos.y, pos.z);
		}

		// apply morph
		for (auto& morph_pt : morph.get_positions())
		{
			size_t idx = morph_pt.first;
			auto& offset = morph_pt.second;
			if (idx >= total_vertices)
				continue;
			auto& pos = m_vertices[idx].get_position();
			shape_vertices[idx].Set(pos.x + offset.x, pos.y + offset.y, pos.z + offset.z);
		}

		if (!normal_indexes.empty() && !context.batch_mode)
		{
			// get normals
			auto normal_element = shape->CreateElementNormal();
			normal_element->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
			normal_element->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

			auto& direct_array = normal_element->GetDirectArray();
			// set a base normals
			for (auto modelIterator : p_CompleteModels.at(0))
			{
				std::for_each(modelIterator.getNormals().begin(), modelIterator.getNormals().end(),
					[&direct_array](const Geometry::Vector3& elem)
					{
						direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
					});
			}
			

			for (auto& morph_normal : morph.get_normals())
			{
				uint32_t idx = morph_normal.first;
				auto& offset = morph_normal.second;
				auto& base = m_normals[idx];
				direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
			}

			auto& index_array = normal_element->GetIndexArray();
			std::for_each(normal_indexes.begin(), normal_indexes.end(),
				[&index_array](const uint32_t& idx) { index_array.Add(idx); });
		}

		if (!tangents_idxs.empty() && !context.batch_mode)
		{
			// get tangents
			auto tangents_ptr = shape->CreateElementTangent();
			tangents_ptr->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
			tangents_ptr->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

			auto& direct_array = tangents_ptr->GetDirectArray();
			std::for_each(m_lighting_normals.begin(), m_lighting_normals.end(),
				[&direct_array](const Geometry::Vector4& elem)
				{
					direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
				});

			for (auto& morph_tangent : morph.get_tangents())
			{
				uint32_t idx = morph_tangent.first;
				auto& offset = morph_tangent.second;
				auto& base = m_lighting_normals[idx];
				direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
			}

			auto& index_array = tangents_ptr->GetIndexArray();
			std::for_each(tangents_idxs.begin(), tangents_idxs.end(),
				[&index_array](const uint32_t& idx) { index_array.Add(idx); });
		}

		auto success = morph_channel->AddTargetShape(shape);
		success = blend_shape_ptr->AddBlendShapeChannel(morph_channel);
	}
	mesh_node_ptr->GetGeometry()->AddDeformer(blend_shape_ptr);
	
}

std::vector<Skeleton::Bone> SWGMainObject::generateSkeletonInScene(FbxScene* scene_ptr, FbxNode* parent_ptr)
{
	assert(parent_ptr != nullptr && scene_ptr != nullptr);
	std::vector<Skeleton::Bone> boneListing;

	uint32_t boneCount = get_bones_count(0); // Turn this into a loop??

	std::vector<FbxNode*> nodes(boneCount, nullptr);
	std::vector<FbxCluster*> clusters(boneCount, nullptr);

	auto pose_ptr2 = FbxPose::Create(scene_ptr, "Rest Pose"); // Also create the binding pose
	pose_ptr2->SetIsBindPose(true);

	for (uint32_t boneCounter = 0; boneCounter < boneCount; boneCounter++)
	{
		Skeleton::Bone& bone = getBone(boneCount, 0);

		FbxNode* node_ptr = FbxNode::Create(scene_ptr, bone.name.c_str());
		FbxSkeleton* skeleton_ptr = FbxSkeleton::Create(scene_ptr, bone.name.c_str());

		if (bone.parent_idx == -1)
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eRoot);
		else
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eLimbNode);

		FbxQuaternion pre_rot_quat{ bone.pre_rot_quaternion.x, bone.pre_rot_quaternion.y, bone.pre_rot_quaternion.z, bone.pre_rot_quaternion.a };
		FbxQuaternion post_rot_quat{ bone.post_rot_quaternion.x, bone.post_rot_quaternion.y, bone.post_rot_quaternion.z, bone.post_rot_quaternion.a };
		FbxQuaternion bind_rot_quat{ bone.bind_pose_rotation.x, bone.bind_pose_rotation.y, bone.bind_pose_rotation.z, bone.bind_pose_rotation.a };

		auto full_rot = post_rot_quat * bind_rot_quat * pre_rot_quat;

		node_ptr->SetPreRotation(FbxNode::eSourcePivot, pre_rot_quat.DecomposeSphericalXYZ());

		node_ptr->SetPostTargetRotation(post_rot_quat.DecomposeSphericalXYZ());

		FbxMatrix  lTransformMatrix;
		node_ptr->LclRotation.Set(full_rot.DecomposeSphericalXYZ());
		node_ptr->LclTranslation.Set(FbxDouble3{ bone.bind_pose_transform.x, bone.bind_pose_transform.y, bone.bind_pose_transform.z });
		FbxVector4 lT, lR, lS;
		lT = FbxVector4(node_ptr->LclTranslation.Get());
		lR = FbxVector4(node_ptr->LclRotation.Get());
		lS = FbxVector4(node_ptr->LclScaling.Get());

		lTransformMatrix.SetTRS(lT, lR, lS);

		boneListing.push_back(bone);
		nodes[boneCounter] = node_ptr;
	}

	// build hierarchy
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto idx_parent = bone.parent_idx;
		if (idx_parent == -1)
			parent_ptr->AddChild(nodes[bone_num]);
		else
		{
			auto& parent = nodes[idx_parent];
			parent->AddChild(nodes[bone_num]);
		}
	}

	// build bind pose
	auto mesh_attr = reinterpret_cast<FbxGeometry*>(parent_ptr->GetNodeAttribute());
	auto skin = FbxSkin::Create(scene_ptr, parent_ptr->GetName());
	auto xmatr = parent_ptr->EvaluateGlobalTransform();
	FbxAMatrix link_transform;

	// create clusters
	// create vertex index arrays for clusters
	const auto& vertices = SourceMesh.get_vertices();
	std::map<std::string, std::vector<std::pair<uint32_t, float>>> cluster_vertices;
	const auto& mesh_joint_names = SourceMesh.get_joint_names();
	for (uint32_t vertex_num = 0; vertex_num < vertices.size(); ++vertex_num)
	{
		const auto& vertex = vertices[vertex_num];
		for (const auto& weight : vertex.get_weights())
		{
			auto joint_name = mesh_joint_names[weight.first];
			boost::to_lower(joint_name);
			cluster_vertices[joint_name].emplace_back(vertex_num, weight.second);
		}
	}

	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
		cluster->SetLink(nodes[bone_num]);
		cluster->SetLinkMode(FbxCluster::eAdditive);

		auto bone_name = bone.name;
		boost::to_lower(bone_name);

		if (cluster_vertices.find(bone_name) != cluster_vertices.end())
		{
			auto& cluster_vertex_array = cluster_vertices[bone_name];
			for (const auto& vertex_weight : cluster_vertex_array)
				cluster->AddControlPointIndex(vertex_weight.first, vertex_weight.second);
		}

		cluster->SetTransformMatrix(xmatr);
		link_transform = nodes[bone_num]->EvaluateGlobalTransform();
		cluster->SetTransformLinkMatrix(link_transform);

		clusters[bone_num] = cluster;
		skin->AddCluster(cluster);
	}

	mesh_attr->AddDeformer(skin);

	auto pose_ptr = FbxPose::Create(scene_ptr, parent_ptr->GetName());
	pose_ptr->SetIsBindPose(true);

	FbxAMatrix matrix;
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		matrix = nodes[bone_num]->EvaluateGlobalTransform();
		FbxVector4 TranVec = matrix.GetT();
		FbxVector4 RotVec = matrix.GetR();

		pose_ptr->Add(nodes[bone_num], matrix);
	}
	scene_ptr->AddPose(pose_ptr);
	return boneListing;
}
