#include "stdafx.h"
#include "SWGMainObject.h"

std::vector<Skeleton::Bone> SWGMainObject::generateSkeletonInScene(FbxScene* scene_ptr, FbxNode* parent_ptr, std::vector<Animated_mesh>& mesh)
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
		Skeleton::Bone& bone = getBone(boneCounter, 0);

		FbxNode* node_ptr = FbxNode::Create(scene_ptr, bone.name.c_str());
		FbxSkeleton* skeleton_ptr = FbxSkeleton::Create(scene_ptr, bone.name.c_str());

		if (bone.parent_idx == -1)
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eRoot);
		else
			skeleton_ptr->SetSkeletonType(FbxSkeleton::eLimbNode);

		node_ptr->SetNodeAttribute(skeleton_ptr);

		// ✅ FIXED: Use single rotation system to avoid conflicts
		FbxQuaternion pre_rot_quat{ bone.pre_rot_quaternion.x, bone.pre_rot_quaternion.y, bone.pre_rot_quaternion.z, bone.pre_rot_quaternion.a };
		FbxQuaternion post_rot_quat{ bone.post_rot_quaternion.x, bone.post_rot_quaternion.y, bone.post_rot_quaternion.z, bone.post_rot_quaternion.a };
		FbxQuaternion bind_rot_quat{ bone.bind_pose_rotation.x, bone.bind_pose_rotation.y, bone.bind_pose_rotation.z, bone.bind_pose_rotation.a };

		// ✅ FIXED: Combine all rotations into single local rotation (prevents conflicts)
		auto combined_rot = post_rot_quat * bind_rot_quat * pre_rot_quat;

		// ✅ DEBUG: Log significant rotation corrections
		auto euler_rot = combined_rot.DecomposeSphericalXYZ();
		if (std::abs(euler_rot[0]) > 170.0 || std::abs(euler_rot[1]) > 170.0 || std::abs(euler_rot[2]) > 170.0) {
			std::cout << "Info: Large rotation detected for bone '" << bone.name << "': "
				<< euler_rot[0] << ", " << euler_rot[1] << ", " << euler_rot[2] << " degrees" << std::endl;
		}

		// ✅ FIXED: Use ONLY local rotation (no pre/post rotation conflicts)
		node_ptr->LclRotation.Set(combined_rot.DecomposeSphericalXYZ());
		node_ptr->LclTranslation.Set(FbxDouble3{ bone.bind_pose_transform.x, bone.bind_pose_transform.y, bone.bind_pose_transform.z });

		// ❌ REMOVED: These conflicting rotation setups that cause 180° issues
		// node_ptr->SetPreRotation(FbxNode::eSourcePivot, pre_rot_quat.DecomposeSphericalXYZ());
		// node_ptr->SetPostTargetRotation(post_rot_quat.DecomposeSphericalXYZ());

		FbxMatrix lTransformMatrix;
		FbxVector4 lT, lR, lS;
		lT = FbxVector4(node_ptr->LclTranslation.Get());
		lR = FbxVector4(node_ptr->LclRotation.Get());
		lS = FbxVector4(node_ptr->LclScaling.Get());

		lTransformMatrix.SetTRS(lT, lR, lS);

		boneListing.push_back(bone);
		nodes[boneCounter] = node_ptr;
		bone.boneNodeptr = node_ptr;
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
	FbxSkin* skin = FbxSkin::Create(scene_ptr, parent_ptr->GetName());
	skin->SetSkinningType(FbxSkin::EType::eRigid);

	auto xmatr = parent_ptr->EvaluateGlobalTransform();
	FbxAMatrix link_transform;

	// create clusters
	// create vertex index arrays for clusters
	std::map<std::string, std::vector<std::pair<uint32_t, float>>> cluster_vertices;
	uint32_t counter = 0;

	for (int cc = 0; cc < mesh.size(); cc++)
	{
		auto& modelIterator = mesh.at(cc);

		const auto& vertices = modelIterator.get_vertices();
		const auto& mesh_joint_names = modelIterator.get_joint_names();
		uint32_t sizeValue = static_cast<uint32_t>(vertices.size());

		for (uint32_t vertex_num = 0; vertex_num < vertices.size(); ++vertex_num)
		{
			uint32_t finalVertexNum = vertex_num + counter;
			const auto& vertex = vertices[vertex_num];
			for (const auto& weight : vertex.get_weights())
			{
				auto joint_name = mesh_joint_names[weight.first];
				boost::to_lower(joint_name);
				cluster_vertices[joint_name].emplace_back(finalVertexNum, weight.second);
			}
		}

		counter += static_cast<uint32_t>(modelIterator.get_vertices().size());
	}

	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
		cluster->SetLink(nodes[bone_num]);
		cluster->SetLinkMode(FbxCluster::eNormalize);

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

template<typename BoneType>
std::vector<SWGMainObject::AnimationCurveData> SWGMainObject::calculateBoneAnimationData(
	const BoneType& animatedBoneIterator,
	std::shared_ptr<Animation> animationObject)
{
	std::vector<AnimationCurveData> results;
	results.reserve(animationObject->get_info().frame_count + 1);

	// Find the corresponding skeleton bone
	Skeleton::Bone skeletonBone = Skeleton::Bone("test");
	std::string boneNameLower = animatedBoneIterator.name;
	boost::to_lower(boneNameLower);

	for (auto& boneIterator : m_bones.at(0))
	{
		std::string boneName = boneIterator.name;
		boost::to_lower(boneName);
		if (boneName == boneNameLower)
		{
			skeletonBone = boneIterator;
			break;
		}
	}

	if (skeletonBone.name == "test") {
		return results; // Invalid bone
	}

	QuatExpand::UncompressQuaternion decompressValues;
	decompressValues.install();

	// All mathematical computations here - no disk I/O
	for (int frameCounter = 0; frameCounter < animationObject->get_info().frame_count + 1; frameCounter++)
	{
		FbxVector4 TranslationVector;
		FbxVector4 RotationVector;

		// Translation Extraction
		if (animatedBoneIterator.hasXAnimatedTranslation)  // ✅ FIXED: Use correct translation flag
		{
			std::vector<float> translationValues = animationObject->getCHNLValues().at(animatedBoneIterator.x_translation_channel_index);
			float translationValue = translationValues.at(frameCounter);
			TranslationVector.mData[0] = (translationValue != -1000) ? translationValue : -1000.0;
		}
		else
		{
			float translationValue = animationObject->getStaticTranslationValues().at(animatedBoneIterator.x_translation_channel_index);
			TranslationVector.mData[0] = translationValue;
		}

		if (animatedBoneIterator.hasYAnimatedTranslation)  // ✅ FIXED: Use correct translation flag
		{
			std::vector<float> translationValues = animationObject->getCHNLValues().at(animatedBoneIterator.y_translation_channel_index);
			float translationValue = translationValues.at(frameCounter);
			TranslationVector.mData[1] = (translationValue != -1000) ? translationValue : -1000.0;
		}
		else
		{
			float translationValue = animationObject->getStaticTranslationValues().at(animatedBoneIterator.y_translation_channel_index);
			TranslationVector.mData[1] = translationValue;
		}

		if (animatedBoneIterator.hasZAnimatedTranslation)  // ✅ FIXED: Use correct translation flag
		{
			std::vector<float> translationValues = animationObject->getCHNLValues().at(animatedBoneIterator.z_translation_channel_index);
			float translationValue = translationValues.at(frameCounter);
			TranslationVector.mData[2] = (translationValue != -1000) ? translationValue : -1000.0;
		}
		else
		{
			float translationValue = animationObject->getStaticTranslationValues().at(animatedBoneIterator.z_translation_channel_index);
			TranslationVector.mData[2] = translationValue;
		}
		// --------------------------------------------------- Rotation extraction --------------------------------------
		if (animatedBoneIterator.has_rotations)
		{
			EulerAngles result;
			if (!animationObject->checkIsUnCompressed())
			{
				std::vector<uint32_t> compressedValues = animationObject->getQCHNValues().at(animatedBoneIterator.rotation_channel_index);
				std::vector<uint8_t> FormatValues;
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
				// For objects that are not compressed
				auto uncompressedValues = animationObject->getKFATQCHNValues().at(animatedBoneIterator.rotation_channel_index);
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
			// Static rotations
			if (!animationObject->checkIsUnCompressed())
			{
				// compressed format
				uint32_t staticValue = animationObject->getStaticRotationValues().at(animatedBoneIterator.rotation_channel_index);
				std::vector<uint8_t> formatValues = animationObject->getStaticROTFormats().at(animatedBoneIterator.rotation_channel_index);

				Geometry::Vector4 Quat = decompressValues.ExpandCompressedValue(staticValue, formatValues[0], formatValues[1], formatValues[2]);
				EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);
				RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
			}
			else
			{
				std::vector<float> uncompressedValues = animationObject->getStaticKFATRotationValues().at(animatedBoneIterator.rotation_channel_index);
				Geometry::Vector4 Quat = { uncompressedValues.at(1), uncompressedValues.at(2), uncompressedValues.at(3), uncompressedValues.at(0) };
				EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);
				RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
			}
		}

		AnimationCurveData data;
		data.frameIndex = frameCounter;
		data.translation = TranslationVector;
		data.rotation = RotationVector;
		data.boneName = skeletonBone.name;
		results.push_back(data);
	}

	return results;
}

// Explicit template instantiation to resolve linker errors
template std::vector<SWGMainObject::AnimationCurveData> SWGMainObject::calculateBoneAnimationData<Animation::Bone_info>(
	const Animation::Bone_info& animatedBoneIterator,
	std::shared_ptr<Animation> animationObject);