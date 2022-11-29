#include "stdafx.h"

#include "animated_object.h"
#include "tre_library.h"
#include <fbxsdk.h>
#include <cmath>
#include <math.h>


using namespace std;
namespace fs = boost::filesystem;

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

set<string> Animated_object_descriptor::get_referenced_objects() const
{
	set<string> names;

	// get mesh names
	names.insert(m_mesh_names.begin(), m_mesh_names.end());

	// get skeleton names
	std::for_each(m_skeleton_info.begin(), m_skeleton_info.end(),
		[&names](const pair<string, string>& item)
		{
			names.insert(item.first);
		});
	// get anim maps names;
	std::for_each(m_animations_map.begin(), m_animations_map.end(),
		[&names](const pair<string, string>& item)
		{
			names.insert(item.second);
		});
	return move(names);
}

set<string> Animated_lod_list::get_referenced_objects() const
{
	set<string> names;

	names.insert(m_lod_names.begin(), m_lod_names.end());

	return move(names);
}

set<string> Animated_file_list::get_referenced_objects() const
{
	set<string> names;

	names.insert(m_animation_list.begin(), m_animation_list.end());

	return move(names);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void Animated_mesh::set_info(const Info& info)
{
	m_mesh_info.num_skeletons = (m_mesh_info.num_skeletons > info.num_skeletons) ? m_mesh_info.num_skeletons : info.num_skeletons;
	m_mesh_info.num_joints += info.num_joints;
	m_mesh_info.position_counts += info.position_counts;
	m_mesh_info.joint_weight_data_count += info.joint_weight_data_count;
	m_mesh_info.normals_count += info.normals_count;
	m_mesh_info.num_shaders += info.num_shaders;
	m_mesh_info.blend_targets += info.blend_targets;
	m_mesh_info.occlusion_zones += info.occlusion_zones;
	m_mesh_info.occlusion_zones_combinations += info.occlusion_zones_combinations;
	m_mesh_info.occluded_zones += info.occluded_zones;
	m_mesh_info.occlusion_layer += info.occlusion_layer;

	m_vertices.reserve(m_mesh_info.position_counts);
	m_normals.reserve(m_mesh_info.normals_count);
	m_joint_names.reserve(m_mesh_info.num_joints);
	m_shaders.reserve(m_mesh_info.num_shaders);
}

bool Animated_mesh::is_object_correct() const
{
	return !m_skeletons_names.empty()
		&& !m_joint_names.empty()
		&& !m_vertices.empty()
		&& !m_normals.empty()
		&& !m_shaders.empty();
}

Animated_mesh::EulerAngles Animated_mesh::ConvertCombineCompressQuat(Geometry::Vector4 DecompressedQuaterion, Skeleton::Bone BoneReference, bool isStatic)
{
	const double pi = 3.14159265358979323846;
	double rotationFactor = 180.0 / pi;

	Geometry::Vector4 Quat;
	EulerAngles angles;

	FbxQuaternion AnimationQuat = FbxQuaternion(DecompressedQuaterion.x, DecompressedQuaterion.y, DecompressedQuaterion.z, DecompressedQuaterion.a);
	FbxQuaternion bind_rot_quat{ BoneReference.bind_pose_rotation.x, BoneReference.bind_pose_rotation.y, BoneReference.bind_pose_rotation.z, BoneReference.bind_pose_rotation.a };
	FbxQuaternion pre_rot_quat{ BoneReference.pre_rot_quaternion.x, BoneReference.pre_rot_quaternion.y, BoneReference.pre_rot_quaternion.z, BoneReference.pre_rot_quaternion.a };
	FbxQuaternion post_rot_quat{ BoneReference.post_rot_quaternion.x, BoneReference.post_rot_quaternion.y, BoneReference.post_rot_quaternion.z, BoneReference.post_rot_quaternion.a };

	auto full_rot = post_rot_quat * (AnimationQuat * bind_rot_quat) * pre_rot_quat;
	Quat = Geometry::Vector4(full_rot.mData[0], full_rot.mData[1], full_rot.mData[2], full_rot.mData[3]);

	double test = Quat.x * Quat.z - Quat.y * Quat.a;
	double sqx = Quat.x * Quat.x;
	double sqy = Quat.y * Quat.y;
	double sqz = Quat.z * Quat.z;
	double sqa = Quat.a * Quat.a;
	double unit = sqx + sqy + sqz + sqa;

	angles.yaw = std::atan2(2.0 * (Quat.x * Quat.y + Quat.z * Quat.a), sqx - sqy - sqz + sqa); // heading
	angles.pitch = std::asin(-2.0 * test / unit); // attitude
	angles.roll = std::atan2(2.0 * (Quat.y * Quat.z + Quat.x * Quat.a), -sqx - sqy + sqz + sqa); // bank

	/*double test = Quat.x * Quat.y + Quat.z * Quat.a;
	if (test < 0.499)
	{
		angles.yaw = 2.0 * std::atan2(Quat.x, Quat.a);
		angles.pitch = pi / 2.0;
		angles.roll = 0;
	}
	else if (test < -0.499)
	{
		angles.yaw = -2.0 * std::atan2(Quat.x, Quat.a);
		angles.pitch = -pi / 2.0;
		angles.roll = 0;
	}
	else
	{
		double sqx = Quat.x * Quat.x;
		double sqy = Quat.y * Quat.y;
		double sqz = Quat.z * Quat.z;

		angles.yaw = std::atan2(2.0 * Quat.y * Quat.a - 2.0 * Quat.x * Quat.z, 1.0 - 2.0 * sqy - 2.0 * sqz);
		angles.pitch = std::asin(2.0 * test);
		angles.roll = std::atan2(2.0 * Quat.x * Quat.a - 2.0 * Quat.y * Quat.z, 1.0 - 2.0 * sqx - 2.0 * sqz);
	}*/


	angles.roll *= rotationFactor;
	angles.pitch *= rotationFactor;
	angles.yaw *= rotationFactor;

	return angles;
}

void Animated_mesh::store(const std::string& path, const Context& context)
{
	// extract object name and make full path name
	boost::filesystem::path obj_name(m_object_name);
	boost::filesystem::path target_path(path);
	target_path /= obj_name.filename();

	target_path.replace_extension("fbx");

	// check directory existance
	auto directory = target_path.parent_path();
	if (!boost::filesystem::exists(directory))
		boost::filesystem::create_directories(directory);

	if (boost::filesystem::exists(target_path))
	{
		return;// For now, skip the file if it already exists
		// boost::filesystem::remove(target_path); // Might add an option to override in final release
	}
		

	// get lod level (by _lX end of file name). If there is no such pattern - lod level will be zero.
	int lodLevel =0;
	obj_name = obj_name.filename();
	obj_name.replace_extension();
	std::string name = obj_name.string();
	char last_char = name.back();
	if (isdigit(last_char))
		m_lod_level = (last_char - '0');

	// init FBX manager
	FbxManager* fbx_manager_ptr = FbxManager::Create();
	if (fbx_manager_ptr == nullptr)
		return;

	FbxIOSettings* ios_ptr = FbxIOSettings::Create(fbx_manager_ptr, IOSROOT);
	ios_ptr->SetIntProp(EXP_FBX_EXPORT_FILE_VERSION, FBX_FILE_VERSION_7700);
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
	FbxScene* scene_ptr = FbxScene::Create(fbx_manager_ptr, m_object_name.c_str());
	if (!scene_ptr)
		return;

	scene_ptr->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::m);
	auto scale_factor = scene_ptr->GetGlobalSettings().GetSystemUnit().GetScaleFactor();
	FbxNode* mesh_node_ptr = FbxNode::Create(scene_ptr, "mesh_node"); //skeleton root?
	FbxMesh* mesh_ptr = FbxMesh::Create(scene_ptr, "mesh");

	mesh_node_ptr->SetNodeAttribute(mesh_ptr);
	scene_ptr->GetRootNode()->AddChild(mesh_node_ptr);

	// prepare vertices
	uint32_t vertices_num = static_cast<uint32_t>(m_vertices.size());
	uint32_t normals_num = static_cast<uint32_t>(m_normals.size());
	mesh_ptr->SetControlPointCount(vertices_num);
	auto mesh_vertices = mesh_ptr->GetControlPoints();

	for (uint32_t vertex_idx = 0; vertex_idx < vertices_num; ++vertex_idx)
	{
		const auto& pt = m_vertices[vertex_idx].get_position();
		mesh_vertices[vertex_idx] = FbxVector4(pt.x, pt.y, pt.z);
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

	uint32_t normalCounter = 0;
	//for (int cc = 0; cc < mesh.size(); cc++)
	{
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

						auto remapped_normal_idx = normals[tri.points[i]] + normalCounter;
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
	std::vector<Skeleton::Bone> BoneInfoList;
	// build skeletons
	std::for_each(m_used_skeletons.begin(), m_used_skeletons.end(),
		[&scene_ptr, &mesh_node_ptr, &BoneInfoList, this](const pair<string, shared_ptr<Skeleton>>& item)
		{
			if (item.second->get_lod_count() > m_lod_level)
			{
				item.second->set_current_lod(m_lod_level);
				BoneInfoList = item.second->generate_skeleton_in_scene(scene_ptr, mesh_node_ptr, this);
			}
		});

	// build morph targets
	// prepare base vector
	FbxBlendShape* blend_shape_ptr = FbxBlendShape::Create(scene_ptr, "BlendShapes");
	//for (int cc = 0; cc < mesh.size(); cc++)
	{

		auto total_vertices = m_vertices.size();
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

		/*	if (!normal_indexes.empty() && !context.batch_mode)
			{
				// get normals
				auto normal_element = shape->CreateElementNormal();
				normal_element->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
				normal_element->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

				auto& direct_array = normal_element->GetDirectArray();
				// set a base normals
				std::for_each(m_normals.begin(), m_normals.end(),
					[&direct_array](const Geometry::Vector3& elem)
					{
						direct_array.Add(FbxVector4(elem.x, elem.y, elem.z));
					});


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
			}*/

			if (!tangents_idxs.empty() && context.batch_mode)
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
					auto& base = m_lighting_normals.at(idx);
					direct_array[idx].Set(base.x + offset.x, base.y + offset.y, base.z + offset.z);
				}

				auto& index_array = tangents_ptr->GetIndexArray();
				std::for_each(tangents_idxs.begin(), tangents_idxs.end(),
					[&index_array](const uint32_t& idx) { index_array.Add(idx); });
			}

			auto success = morph_channel->AddTargetShape(shape);
			success = blend_shape_ptr->AddBlendShapeChannel(morph_channel);
		}
	}
	mesh_node_ptr->GetGeometry()->AddDeformer(blend_shape_ptr);

	// Build Animations (Note: Not sure if this is the best place to put them. But should be after the skeletons
	std::vector<std::shared_ptr<Animation>> animationList;

	// First sort out the animation objects first
	for (auto baseObjectiterator : context.object_list)
	{
		if (baseObjectiterator.second->get_object_name().find("ans") != std::string::npos)
		{
			std::shared_ptr<Animation> inputObject = std::dynamic_pointer_cast<Animation>(baseObjectiterator.second);
			animationList.push_back(inputObject);
		}
	}

	QuatExpand::UncompressQuaternion decompressValues;
	decompressValues.install();


	//FbxAxisSystem max; // we desire to convert the scene from Y-Up to Z-Up
	//max.ConvertScene(scene_ptr);
	FbxAxisSystem::MayaZUp.ConvertScene(scene_ptr);
	// Next loop through the entire animation list
	for (int i = 0; i < animationList.size(); i++)// This method is esy for debugging
	{
		auto animationObject = animationList.at(i);

		if (animationObject)
		{

			std::string stackName = animationObject->get_object_name();
			std::string firstErase = "appearance/animation/";
			std::string secondErase = ".ans";

			size_t pos = stackName.find(firstErase);
			if (pos != std::string::npos)
				stackName.erase(pos, firstErase.length());

			size_t pos2 = stackName.find(secondErase);
			if (pos2 != std::string::npos)
				stackName.erase(pos2, secondErase.length());

			FbxString animationStackName = FbxString(stackName.c_str());
			fbxsdk::FbxAnimStack* animationStack = fbxsdk::FbxAnimStack::Create(scene_ptr, animationStackName);

			FbxAnimLayer* animationLayer = FbxAnimLayer::Create(scene_ptr, "Base Layer");
			animationStack->AddMember(animationLayer);

			FbxGlobalSettings& sceneGlobaleSettings = scene_ptr->GetGlobalSettings();
			double currentFrameRate = FbxTime::GetFrameRate(sceneGlobaleSettings.GetTimeMode());
			if (animationObject->get_info().FPS != currentFrameRate)
			{
				FbxTime::EMode computeTimeMode = FbxTime::ConvertFrameRateToTimeMode(animationObject->get_info().FPS);
				FbxTime::SetGlobalTimeMode(computeTimeMode, computeTimeMode == FbxTime::eCustom ? animationObject->get_info().FPS : 0.0);
				sceneGlobaleSettings.SetTimeMode(computeTimeMode);
				if (computeTimeMode == FbxTime::eCustom)
				{
					sceneGlobaleSettings.SetCustomFrameRate(animationObject->get_info().FPS);
				}
			}

			FbxTime exportedStartTime, exportedStopTime;
			exportedStartTime.SetSecondDouble(0.0f);
			exportedStopTime.SetSecondDouble((double)animationObject->get_info().frame_count / animationObject->get_info().FPS);

			FbxTimeSpan exportedTimeSpan;
			exportedTimeSpan.Set(exportedStartTime, exportedStopTime);
			animationStack->SetLocalTimeSpan(exportedTimeSpan);

			for (auto& boneIterator : animationObject->get_bones())
			{
				std::vector<FbxNode*> treeBranch;
				treeBranch.push_back(mesh_node_ptr->GetChild(0));// The first node to start with is the child of the root node
				std::string boneName = boneIterator.name;

				for (int i = 0; i < treeBranch.size(); i++)// Loop through the branches of the tree
				{
					std::string treeBranchName = treeBranch.at(i)->GetName();// Grab the name of the current element Also for debugging

					boost::to_lower(treeBranchName);
					boost::to_lower(boneName);

					if (treeBranchName == boneName)// Found a match between the selected 
					{
						Skeleton::Bone skeletonBone = Skeleton::Bone("test");
						for (auto& boneInfoIterator : BoneInfoList)
						{
							std::string searchBoneName = boneInfoIterator.name;
							boost::to_lower(searchBoneName);

							if (searchBoneName == boneName)
							{
								skeletonBone = boneInfoIterator;
								break;
							}
						}

						// For easy access placing pointers to the bones
						FbxNode* rootSkeleton = mesh_node_ptr;
						FbxNode* boneToUse = treeBranch.at(i);
						fbxsdk::FbxAMatrix& globalNode = boneToUse->EvaluateLocalTransform();
						// The bone iteratator will have all the animation info for the specific bone
						fbxsdk::FbxAnimCurve* Curves[9];

						Curves[0] = boneToUse->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
						Curves[1] = boneToUse->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
						Curves[2] = boneToUse->LclTranslation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

						Curves[3] = boneToUse->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
						Curves[4] = boneToUse->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
						Curves[5] = boneToUse->LclRotation.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);

						Curves[6] = boneToUse->LclScaling.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_X, true);
						Curves[7] = boneToUse->LclScaling.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Y, true);
						Curves[8] = boneToUse->LclScaling.GetCurve(animationLayer, FBXSDK_CURVENODE_COMPONENT_Z, true);


						for (fbxsdk::FbxAnimCurve* Curve : Curves)
						{
							Curve->KeyModifyBegin();
						}

						for (int frameCounter = 0; frameCounter < animationObject->get_info().frame_count + 1; frameCounter++)
						{
							// For each frame, we need to build the translation vector and the rotation vector
							FbxVector4 TranslationVector;
							FbxVector4 RotationVector;
							// -------------------------------------- Translation Extraction -------------------------------------
							if (boneIterator.hasXAnimatedTranslation)
							{

								std::vector<float> translationValues = animationObject->getCHNLValues().at(boneIterator.x_translation_channel_index);

								if (translationValues.size() != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								float translationValue = translationValues.at(frameCounter);
								if (translationValue > -1000.0)
								{
									TranslationVector.mData[0] = translationValue;
								}
								else
								{
									TranslationVector.mData[0] = -1000.0;
								}
							}
							else
							{
								float translationValue = animationObject->getStaticTranslationValues().at(boneIterator.x_translation_channel_index);
								TranslationVector.mData[0] = translationValue;
							}

							if (boneIterator.hasYAnimatedTranslation)
							{
								std::vector<float> translationValues = animationObject->getCHNLValues().at(boneIterator.y_translation_channel_index);

								if (translationValues.size() != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								float translationValue = translationValues.at(frameCounter);
								if (translationValue > -1000.0)
								{
									TranslationVector.mData[1] = translationValue;
								}
								else
								{
									//Need to do something here....
									//cout << "Frame Skipped";
									TranslationVector.mData[1] = -1000.0;
								}
							}
							else
							{
								float translationValue = animationObject->getStaticTranslationValues().at(boneIterator.y_translation_channel_index);
								TranslationVector.mData[1] = translationValue;
							}

							if (boneIterator.hasZAnimatedTranslation)
							{
								std::vector<float> translationValues = animationObject->getCHNLValues().at(boneIterator.z_translation_channel_index);

								if (translationValues.size() != animationObject->get_info().frame_count + 1)
								{
									std::cout << "Mis-match size";
								}

								float translationValue = translationValues.at(frameCounter);
								if (translationValue > -1000.0)
								{
									TranslationVector.mData[2] = translationValue;
								}
								else
								{
									//Need to do something here....
									//cout << "Frame Skipped";
									TranslationVector.mData[2] = -1000.0;
								}
							}
							else
							{
								float translationValue = animationObject->getStaticTranslationValues().at(boneIterator.z_translation_channel_index);
								TranslationVector.mData[2] = translationValue;
							}

							// --------------------------------------------------- Rotation extraction --------------------------------------

							if (boneIterator.has_rotations)
							{
								EulerAngles result;

								if (!animationObject->checkIsUnCompressed())
								{
									// Compressed format
									std::vector<uint32_t> compressedValues = animationObject->getQCHNValues().at(boneIterator.rotation_channel_index);
									std::vector<uint8_t> FormatValues;

									if (compressedValues.size() - 1 != animationObject->get_info().frame_count + 1)
									{
										std::cout << "Mis-match size";
									}

									uint32_t formatValue = compressedValues.at(0);
									uint32_t compressedValue = compressedValues.at(frameCounter + 1);

									FormatValues.push_back((formatValue & (((1 << 8) - 1) << 16)) >> 16);
									FormatValues.push_back((formatValue & (((1 << 8) - 1) << 8)) >> 8);
									FormatValues.push_back((formatValue & (((1 << 8) - 1))));

									if (compressedValue != 100)
									{
										Geometry::Vector4 Quat = decompressValues.ExpandCompressedValue(compressedValue, FormatValues[0], FormatValues[1], FormatValues[2]);
										result = ConvertCombineCompressQuat(Quat, skeletonBone);
										RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
									}
									else
									{
										RotationVector = FbxVector4(-1000.0, -1000.0, -1000.0);
									}
								}
								else
								{
									// uncompressed format
									auto uncompressedValues = animationObject->getKFATQCHNValues().at(boneIterator.rotation_channel_index);
									std::vector<float> QuatValues = uncompressedValues.at(frameCounter);
									if (QuatValues.at(0) != 100)
									{
										Geometry::Vector4 Quat = { QuatValues.at(1), QuatValues.at(2), QuatValues.at(3), QuatValues.at(0) };
										result = ConvertCombineCompressQuat(Quat, skeletonBone);
										RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
									}
									else
									{
										RotationVector = FbxVector4(-1000.0, -1000.0, -1000.0);
									}

								}

							}
							else
							{
								if (!animationObject->checkIsUnCompressed())
								{
									// compressed format
									uint32_t staticValue = animationObject->getStaticRotationValues().at(boneIterator.rotation_channel_index);
									std::vector<uint8_t> formatValues = animationObject->getStaticROTFormats().at(boneIterator.rotation_channel_index);

									Geometry::Vector4 Quat = decompressValues.ExpandCompressedValue(staticValue, formatValues[0], formatValues[1], formatValues[2]);

									EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);

									RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
								}
								else
								{
									std::vector<float> uncompressedValues = animationObject->getStaticKFATRotationValues().at(boneIterator.rotation_channel_index);
									Geometry::Vector4 Quat = { uncompressedValues.at(1), uncompressedValues.at(2), uncompressedValues.at(3), uncompressedValues.at(0) };
									EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);
									RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
								}

							}

							// ---------------------------------- Matrix setup ---------------------------------

							FbxVector4 ScalingVector(1.0, 1.0, 1.0);

							FbxVector4 Vectors[3] = { TranslationVector, RotationVector, ScalingVector };

							double timeValue = (double)frameCounter * ((double)1.0 / (double)animationObject->get_info().FPS);
							FbxVector4 finalVector[3] = { globalNode.GetT() + TranslationVector , RotationVector, ScalingVector };

							for (int curveIndex = 0; curveIndex < 2; curveIndex++)
							{
								for (int coordinateIndex = 0; coordinateIndex < 3; coordinateIndex++)
								{
									if (Vectors[curveIndex][coordinateIndex] != -1000.0)// If the coordinate value is -1000 then this means we need to skip it since it is not a key frame
									{
										int offsetCurveIndex = (curveIndex * 3) + coordinateIndex;
										FbxTime setTime;
										setTime.SetSecondDouble(timeValue);
										uint32_t keyIndex = Curves[offsetCurveIndex]->KeyAdd(setTime);
										Curves[offsetCurveIndex]->KeySet(keyIndex, setTime, finalVector[curveIndex][coordinateIndex], frameCounter == animationObject->get_info().frame_count ? FbxAnimCurveDef::eInterpolationConstant : FbxAnimCurveDef::eInterpolationCubic);
										if (frameCounter == animationObject->get_info().frame_count)
										{
											Curves[offsetCurveIndex]->KeySetConstantMode(keyIndex, FbxAnimCurveDef::eConstantStandard);
										}
									}
									else
									{
										//cout << "Frame Skipped" << endl;
									}
								}
							}
						}

						for (fbxsdk::FbxAnimCurve* Curve : Curves)
						{
							Curve->KeyModifyEnd();
						}
					}
					else
					{
						// If not found, then we will add the children to the treeBranch vector so that we can 
						// check the children also
						if (treeBranch.at(i)->GetChildCount() > 0)
						{
							for (int ii = 0; ii < treeBranch.at(i)->GetChildCount(); ii++)
							{
								treeBranch.push_back(treeBranch.at(i)->GetChild(ii));
							}
						}
					}
				}
			}
		}
	}

	exporter_ptr->Export(scene_ptr);
	// cleanup
	fbx_manager_ptr->Destroy();
}

fbxsdk::FbxAMatrix Animated_mesh::GetGlobalDefaultPosition(fbxsdk::FbxNode* pNode)
{
	FbxAMatrix lLocalPosition;
	FbxAMatrix lGlobalPosition;
	FbxAMatrix lParentGlobalPosition;

	lLocalPosition.SetT(pNode->LclTranslation.Get());
	lLocalPosition.SetR(pNode->LclRotation.Get());
	lLocalPosition.SetS(pNode->LclScaling.Get());

	if (pNode->GetParent())
	{
		lParentGlobalPosition = GetGlobalDefaultPosition(pNode->GetParent());
		lGlobalPosition = lParentGlobalPosition * lLocalPosition;
	}
	else
	{
		lGlobalPosition = lLocalPosition;
	}

	return lGlobalPosition;
}


float Animated_mesh::GetTranslationAnimationValue(int frame, std::vector<float>& translationValues)
{
	return translationValues.at(frame);
}

void Animated_mesh::SetTranslationAnimation(FbxAnimCurve* linearCurve, std::vector<float>& translationValues, std::shared_ptr<Animation> animationObject)
{
/*	linearCurve->KeyModifyBegin();
	int indexCounter = 0;
	int keyIndex = 0;

	for (auto transValue : translationValues)
	{
		if (transValue == -1000.0)
		{
			indexCounter++;
			continue;
		}

		FbxTime lTime;
		double timeValue = (double)indexCounter * ((double)1.0 / (double)animationObject->get_info().FPS);
		lTime.SetSecondDouble(timeValue);
		
		keyIndex = linearCurve->KeyAdd(lTime);

		linearCurve->KeySetValue(keyIndex, transValue);

		linearCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);

		indexCounter++;

	}

	linearCurve->KeyModifyEnd();
	*/
}


void Animated_mesh::setStaticTranslationAnimation(FbxAnimCurve* linearCurve, double translationValue, std::shared_ptr<Animation> animationObject)
{
	linearCurve->KeyModifyBegin();

	for (int i = 0; i < animationObject->get_info().frame_count; i++)
	{
		FbxTime lTime;
		int keyIndex = 0;

		double frameTime = (double)i / (double)animationObject->get_info().FPS;

		lTime.SetSecondDouble(frameTime);
		keyIndex = linearCurve->KeyAdd(lTime);
		linearCurve->KeySetValue(keyIndex, translationValue);
		linearCurve->KeySetInterpolation(keyIndex, FbxAnimCurveDef::eInterpolationLinear);
	}

	linearCurve->KeyModifyEnd();
}

set<string> Animated_mesh::get_referenced_objects() const
{
	set<string> names;
	names.insert(m_skeletons_names.begin(), m_skeletons_names.end());
	std::for_each(m_shaders.begin(), m_shaders.end(),
		[&names](const Shader_appliance& shader) { names.insert(shader.get_name()); });
	return names;
}

void Animated_mesh::resolve_dependencies(const Context& context)
{
	// find object description through open by collection;
	auto it = context.opened_by.find(m_object_name);
	string opened_by_name;
	while (it != context.opened_by.end())
	{
		opened_by_name = it->second;
		it = context.opened_by.find(opened_by_name);
	}

	// get shaders
	for (auto it_shad = m_shaders.begin(); it_shad != m_shaders.end(); ++it_shad)
	{
		auto obj_it = context.object_list.find(it_shad->get_name());
		if (obj_it != context.object_list.end() && dynamic_pointer_cast<Shader>(obj_it->second))
			it_shad->set_definition(dynamic_pointer_cast<Shader>(obj_it->second));
	}

	auto bad_shaders = any_of(m_shaders.begin(), m_shaders.end(),
		[](const Shader_appliance& shader) { return shader.get_definition() == nullptr; });

	if (bad_shaders)
	{
		vector<uint8_t> counters(m_vertices.size(), 0);
		for (auto& shader : m_shaders)
		{
			for (auto& vert_idx : shader.get_pos_indexes())
			{
				if (vert_idx >= counters.size())
					break;
				counters[vert_idx]++;
			}
				

			if (shader.get_definition() == nullptr)
				for (auto& vert_idx : shader.get_pos_indexes())
				{
					if (vert_idx >= counters.size())
						break;
					counters[vert_idx]--;
				}
					
		}

		auto safe_clear = is_partitioned(counters.begin(), counters.end(),
			[](uint8_t val) { return val > 0; });
		if (safe_clear)
		{
			// we can do safe clear of trouble shader, if not - oops
			auto beg = find_if(counters.begin(), counters.end(),
				[](uint8_t val) { return val == 0; });
			auto idx = distance(counters.begin(), beg);
			m_vertices.erase(m_vertices.begin() + idx, m_vertices.end());

			for (size_t idx = 0; idx < m_shaders.size(); ++idx)
				if (m_shaders[idx].get_definition() == nullptr)
				{
					m_shaders.erase(m_shaders.begin() + idx);
					break;
				}
		}
	}

	// get object description first
	// NOTE there is a error in MGN can be encountered because wrong skt name given in skeleton sections
	auto mesh_description = dynamic_pointer_cast<Animated_object_descriptor>(context.object_list.find(opened_by_name)->second);

	// get skeletons
	size_t skel_idx = 0;
	for (auto it_skel = m_skeletons_names.begin(); it_skel != m_skeletons_names.end(); ++it_skel, ++skel_idx)
	{
		auto skel_name = *it_skel;
		// check name against name in description
		if (mesh_description)
		{
			auto descr_skel_name = mesh_description->get_skeleton_name(skel_idx);
			if (!boost::iequals(skel_name, descr_skel_name))
				skel_name = descr_skel_name;
		}
		auto obj_it = context.object_list.find(skel_name);
		if (obj_it != context.object_list.end() && dynamic_pointer_cast<Skeleton>(obj_it->second))
			m_used_skeletons.emplace_back(skel_name, dynamic_pointer_cast<Skeleton>(obj_it->second)->clone());
	}

	if (!opened_by_name.empty() && m_used_skeletons.size() > 1 && mesh_description)
	{
		// check if we opened through SAT and have multiple skeleton definitions
		//  then we have unite them to one, so we need SAT to get join information from it's skeleton info;
		auto skel_count = mesh_description->get_skeletons_count();
		shared_ptr<Skeleton> root_skeleton;
		for (uint32_t skel_idx = 0; skel_idx < skel_count; ++skel_idx)
		{
			auto skel_name = mesh_description->get_skeleton_name(skel_idx);
			auto point_name = mesh_description->get_skeleton_attach_point(skel_idx);

			auto this_it = std::find_if(m_used_skeletons.begin(), m_used_skeletons.end(),
				[&skel_name](const pair<string, shared_ptr<Skeleton>>& info)
				{
					return (boost::iequals(skel_name, info.first));
				});

			if (point_name.empty() && this_it != m_used_skeletons.end())
				// mark this skeleton as root
				root_skeleton = this_it->second;
			else if (this_it != m_used_skeletons.end())
			{
				// attach skeleton to root
				root_skeleton->join_skeleton_to_point(point_name, this_it->second);
				// remove it from used list
				m_used_skeletons.erase(this_it);
			}
		}
	}
}

void Animated_mesh::Shader_appliance::add_primitive()
{
	uint32_t new_primitive_position = static_cast<uint32_t>(m_triangles.size());
	if (!m_primitives.empty() && m_primitives.back().second == 0)
		close_primitive();

	m_primitives.emplace_back(new_primitive_position, 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

shared_ptr<Skeleton> Skeleton::clone()
{
	return make_shared<Skeleton>(*this);
}

std::vector<Skeleton::Bone> Skeleton::generate_skeleton_in_scene(FbxScene* scene_ptr, FbxNode* parent_ptr, Animated_mesh* source_mesh)
{
	assert(parent_ptr != nullptr && scene_ptr != nullptr);
	std::vector<Skeleton::Bone> boneListing;

	auto bones_count = get_bones_count();
	vector<FbxNode*> nodes(bones_count, nullptr);
	vector<FbxCluster*> clusters(bones_count, nullptr);
	// Set bone attributes
	auto pose_ptr2 = FbxPose::Create(scene_ptr, "Rest Pose"); // Also create the binding pose
	pose_ptr2->SetIsBindPose(true);

	for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
	{
		auto& bone = get_bone(bone_num);

		FbxNode* node_ptr = FbxNode::Create(scene_ptr, bone.name.c_str());
		FbxSkeleton* skeleton_ptr = FbxSkeleton::Create(scene_ptr, bone.name.c_str());
		if (bone.parent_idx == -1)
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eRoot);
		else
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eLimbNode);

		node_ptr->SetNodeAttribute(skeleton_ptr);

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
		nodes[bone_num] = node_ptr;
		
	}

	//scene_ptr->AddPose(pose_ptr2);


	// build hierarchy
	for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
	{
		auto& bone = get_bone(bone_num);
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
	const auto& vertices = source_mesh->get_vertices();
	map<string, vector<pair<uint32_t, float>>> cluster_vertices;
	const auto& mesh_joint_names = source_mesh->get_joint_names();
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

	for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
	{
		auto& bone = get_bone(bone_num);
		auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
		cluster->SetLink(nodes[bone_num]);
		cluster->SetLinkMode(FbxCluster::eTotalOne);

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
	for (uint32_t bone_num = 0; bone_num < bones_count; ++bone_num)
	{
		matrix = nodes[bone_num]->EvaluateGlobalTransform();
		FbxVector4 TranVec = matrix.GetT();
		FbxVector4 RotVec = matrix.GetR();

		pose_ptr->Add(nodes[bone_num], matrix);
	}
	scene_ptr->AddPose(pose_ptr);
	return boneListing;
}

fbxsdk::FbxAMatrix Skeleton::GetGlobalDefaultPosition(fbxsdk::FbxNode* pNode)
{
	FbxAMatrix lLocalPosition;
	FbxAMatrix lGlobalPosition;
	FbxAMatrix lParentGlobalPosition;

	lLocalPosition.SetT(pNode->LclTranslation.Get());
	lLocalPosition.SetR(pNode->LclRotation.Get());
	lLocalPosition.SetS(pNode->LclScaling.Get());

	if (pNode->GetParent())
	{
		lParentGlobalPosition = GetGlobalDefaultPosition(pNode->GetParent());
		lGlobalPosition = lParentGlobalPosition * lLocalPosition;
	}
	else
	{
		lGlobalPosition = lLocalPosition;
	}

	return lGlobalPosition;
}

void Skeleton::join_skeleton_to_point(const string& attach_point, const shared_ptr<Skeleton>& skel_to_join)
{
	uint32_t attach_point_idx = numeric_limits<uint32_t>::max();
	auto lods_to_join = min(get_lod_count(), skel_to_join->get_lod_count());
	for (uint32_t lod = 0; lod < lods_to_join; ++lod)
	{
		// find attach point;
		for (uint32_t idx = 0; idx < m_bones[lod].size(); ++idx)
		{
			if (boost::iequals(attach_point, m_bones[lod][idx].name))
				attach_point_idx = idx;
		}

		// attach point found
		if (attach_point_idx < numeric_limits<uint32_t>::max())
		{
			auto& bones_to_join = skel_to_join->m_bones[lod];
			uint32_t bones_offset = static_cast<uint32_t>(m_bones[lod].size());
			for (uint32_t idx = 0; idx < bones_to_join.size(); ++idx)
			{
				auto bone = bones_to_join[idx];
				if (bone.parent_idx == -1)
					bone.parent_idx = attach_point_idx;
				else
				{
					bone.parent_idx += bones_offset;
				}
				m_bones[lod].push_back(bone);
			}
		}
	}
}


bool Skeleton::is_object_correct() const
{
	return !m_bones.empty();
}

set<string> Skeleton::get_referenced_objects() const
{
	return set<string>();
}

void Skeleton::resolve_dependencies(const Context& context)
{
}

void Skeleton::set_object_name(const std::string& obj_name)
{
	m_skeleton_name = obj_name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

Shader::texture_type Shader::get_texture_type(const std::string& texture_tag)
{
	if (texture_tag == "MAIN")
		return texture_type::main;
	else if (texture_tag == "NRML")
		return texture_type::normal;
	else if (texture_tag == "DOT3")
		return texture_type::lightmap;
	else if (texture_tag == "SPEC")
		return texture_type::specular;
	return texture_type::none_type;
}

set<string> Shader::get_referenced_objects() const
{
	set<string> result;
	std::for_each(m_textures.begin(), m_textures.end(),
		[&result](const Texture& texture) { result.insert(texture.tex_file_name); });

	std::for_each(m_palettes.begin(), m_palettes.end(),
		[&result](const Palette& palette) { result.insert(palette.palette_file); });

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

shared_ptr<DDS_Texture> DDS_Texture::construct(const string& name, const uint8_t* buffer, size_t buf_size)
{
	DirectX::TexMetadata meta;
	shared_ptr<DirectX::ScratchImage> image = make_shared<DirectX::ScratchImage>();
	HRESULT hr = DirectX::LoadFromDDSMemory(buffer, buf_size, DirectX::DDS_FLAGS_NONE, &meta, *image);
	if (SUCCEEDED(hr))
	{
		shared_ptr<DDS_Texture> ret_object = make_shared<DDS_Texture>();
		ret_object->set_object_name(name);
		ret_object->m_image = image;
		return ret_object;
	}

	return nullptr;
}

void DDS_Texture::store(const string& path, const Context& context)
{
	boost::filesystem::path out_path(path);

	out_path /= m_name;
	out_path.replace_extension("tga");
	out_path.normalize();

	auto directory = out_path.parent_path();
	if (!boost::filesystem::exists(directory))
		boost::filesystem::create_directories(directory);

	auto image = m_image->GetImage(0, 0, 0);
	const DirectX::Image* out_image = nullptr;
	DirectX::ScratchImage decompressed;
	if (DirectX::IsCompressed(image->format))
	{
		DirectX::Decompress(*image, DXGI_FORMAT_R8G8B8A8_UNORM, decompressed);
		out_image = decompressed.GetImage(0, 0, 0);
	}
	else
		out_image = image;

	DirectX::SaveToTGAFile(*out_image, out_path.wstring().c_str());
}

bool Animation::is_object_correct() const
{
	return false;
}

void Animation::store(const std::string& path, const Context& context)
{
	fs::path obj_name(m_object_name);
	fs::path target_path(path);
	target_path /= obj_name.filename();
	target_path.replace_extension("fbx");

	// check directory existance
	auto directory = target_path.parent_path();
	if (!boost::filesystem::exists(directory))
		boost::filesystem::create_directories(directory);

	if (fs::exists(target_path))
		fs::remove(target_path);
}

std::set<std::string> Animation::get_referenced_objects() const
{
	return std::set<std::string>();
}

void Animation::resolve_dependencies(const Context& context)
{
}

void Animation::set_object_name(const std::string& obj_name)
{
	m_object_name = obj_name;
}

std::string Animation::get_object_name() const
{
	return m_object_name;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////


