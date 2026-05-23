#include "stdafx.h"
#include "SWGMainObject.h"
#include <iomanip>

std::vector<Skeleton::Bone> SWGMainObject::generateSkeletonInScene(FbxScene* scene_ptr, FbxNode* parent_ptr, FbxNode* skeleton_parent_ptr, std::vector<Animated_mesh>& mesh)
{
	assert(parent_ptr != nullptr && scene_ptr != nullptr);
	std::vector<Skeleton::Bone> boneListing;

	uint32_t boneCount = get_bones_count(0); // Turn this into a loop??

	// 🔍 CONDENSED DEBUG: Only for first animation frame equivalent (skeleton generation happens once)
	bool isLOD0 = mesh.size() > 0 && mesh[0].getLodLevel() == 0;
	if (DebugConfig::shouldLog() && isLOD0) {
		LOG_SKELETON("LOD 0 generation with " << boneCount << " bones");
	}

	std::vector<FbxNode*> nodes(boneCount, nullptr);
	std::vector<FbxCluster*> clusters(boneCount, nullptr);

	for (uint32_t boneCounter = 0; boneCounter < boneCount; boneCounter++)
	{
		Skeleton::Bone& bone = getBone(boneCounter, 0);


		// 🔍 CONDENSED DEBUG: Only show critical bone rotation issues
		bool isTargetBone = (bone.name == "r_f_leg3" || bone.name == "r_f_leg_finger" || 
			bone.name == "l_f_leg3" || bone.name == "l_f_leg_finger" ||
			bone.name.find("leg") != std::string::npos);

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

		// ✅ CRITICAL FIX: Use proper quaternion normalization and combination
		// Normalize all quaternions to prevent accumulation errors
		pre_rot_quat.Normalize();
		post_rot_quat.Normalize();
		bind_rot_quat.Normalize();
		
		// ✅ FIXED: Use the SAME transformation order as the animation system
		auto combined_rot = post_rot_quat * bind_rot_quat * pre_rot_quat;
		combined_rot.Normalize(); // Ensure final result is normalized
		
		// ✅ CRITICAL FIX: Handle Euler angle ambiguity for large rotations
		// The issue is not quaternion sign, but Euler angle representation choice
		// FBX DecomposeSphericalXYZ can give large angles when smaller equivalents exist
		auto test_euler = combined_rot.DecomposeSphericalXYZ();
		
		// Apply Euler angle normalization for bones with large rotations
		FbxVector4 corrected_euler = test_euler;
		bool applied_correction = false;
		
		// Normalize each axis to [-180, 180] range and find equivalent smaller angles
		for (int i = 0; i < 3; i++) {
			double angle = test_euler[i];
			
			// Method 1: Normalize to [-180, 180] range
			while (angle > 180.0) angle -= 360.0;
			while (angle < -180.0) angle += 360.0;
			
			// Method 2: For angles near ±180°, try the equivalent smaller angle
			if (std::abs(angle) > 170.0) {
				double alt_angle = (angle > 0) ? angle - 360.0 : angle + 360.0;
				if (std::abs(alt_angle) < std::abs(angle)) {
					angle = alt_angle;
					applied_correction = true;
				}
			}
			
			corrected_euler[i] = angle;
		}
		
		// Apply correction if significant improvement
		if (applied_correction) {
			// Create corrected quaternion from the normalized Euler angles
			FbxQuaternion corrected_quat;
			corrected_quat.ComposeSphericalXYZ(corrected_euler);
			combined_rot = corrected_quat;
		}

		// 🔍 CONDENSED DEBUG: Show only critical rotation corrections for target bones
		if (DebugConfig::shouldLog() && isLOD0 && isTargetBone && (std::abs(test_euler[0]) > 100.0 || std::abs(test_euler[1]) > 100.0 || std::abs(test_euler[2]) > 100.0)) {
			double original_magnitude = sqrt(test_euler[0]*test_euler[0] + test_euler[1]*test_euler[1] + test_euler[2]*test_euler[2]);
			LOG_SKELETON(bone.name << " | Large rotation magnitude: " << std::fixed << std::setprecision(1) << original_magnitude << "°");
			if (applied_correction) {
				double corrected_magnitude = sqrt(corrected_euler[0]*corrected_euler[0] + corrected_euler[1]*corrected_euler[1] + corrected_euler[2]*corrected_euler[2]);
				LOG_SKELETON(" -> " << corrected_magnitude << "° (corrected)");
			}
		}

		// ✅ FIXED: Use ONLY local rotation (no pre/post rotation conflicts)
		node_ptr->LclRotation.Set(combined_rot.DecomposeSphericalXYZ());
		node_ptr->LclTranslation.Set(FbxDouble3{ bone.bind_pose_transform.x, bone.bind_pose_transform.y, bone.bind_pose_transform.z });

		// 🔍 BIND POSE DEBUG: Verify rotation application for target bones
		if (DebugConfig::shouldLog() && isLOD0 && isTargetBone) {
			FbxVector4 applied_rotation = node_ptr->LclRotation.Get();
			FbxVector4 applied_translation = node_ptr->LclTranslation.Get();
			LOG_SKELETON("🦴 BIND POSE " << bone.name 
				<< " | Rot: (" << std::fixed << std::setprecision(1) 
				<< applied_rotation[0] << "," << applied_rotation[1] << "," << applied_rotation[2] << ")"
				<< " | Trans: (" << std::setprecision(3) 
				<< applied_translation[0] << "," << applied_translation[1] << "," << applied_translation[2] << ")");
			
			// Verify the rotation was actually applied
			FbxQuaternion verify_quat;
			verify_quat.ComposeSphericalXYZ(applied_rotation);
			double verify_dot = verify_quat.DotProduct(combined_rot);
			LOG_SKELETON("   Verification dot product: " << std::setprecision(6) << verify_dot 
				<< (std::abs(verify_dot) > 0.99 ? " [GOOD]" : " [BAD]"));
		}

		boneListing.push_back(bone);
		nodes[boneCounter] = node_ptr;
		bone.boneNodeptr = node_ptr;
	}

	// build hierarchy - remove verbose debug output
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto idx_parent = bone.parent_idx;
		
		if (idx_parent == -1)
			skeleton_parent_ptr->AddChild(nodes[bone_num]); // child of armature, not mesh_node
		else
		{
			auto& parent = nodes[idx_parent];
			parent->AddChild(nodes[bone_num]);
		}
	}

	// build bind pose
	auto mesh_attr = reinterpret_cast<FbxGeometry*>(parent_ptr->GetNodeAttribute());
	FbxSkin* skin = FbxSkin::Create(scene_ptr, parent_ptr->GetName());
	skin->SetSkinningType(FbxSkin::EType::eLinear);

	auto xmatr = parent_ptr->EvaluateGlobalTransform();
	FbxAMatrix link_transform;

	// create clusters - remove verbose debug output
	std::map<std::string, std::vector<std::pair<uint32_t, float>>> cluster_vertices;
	uint32_t counter = 0;

	for (int cc = 0; cc < mesh.size(); cc++)
	{
		auto& modelIterator = mesh.at(cc);
		const auto& vertices = modelIterator.get_vertices();
		const auto& mesh_joint_names = modelIterator.get_joint_names();

		for (uint32_t vertex_num = 0; vertex_num < vertices.size(); ++vertex_num)
		{
			uint32_t finalVertexNum = vertex_num + counter;
			const auto& vertex = vertices[vertex_num];
			
			for (const auto& weight : vertex.get_weights())
			{
				auto joint_name = mesh_joint_names[weight.first];
				std::string joint_name_lower = joint_name;
				boost::to_lower(joint_name_lower);
				cluster_vertices[joint_name_lower].emplace_back(finalVertexNum, weight.second);
			}
		}

		counter += static_cast<uint32_t>(modelIterator.get_vertices().size());
	}

	// create FBX clusters - remove verbose debug output
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
		cluster->SetLink(nodes[bone_num]);
		cluster->SetLinkMode(FbxCluster::eNormalize);

		auto bone_name = bone.name;
		boost::to_lower(bone_name);

		bool isTargetBone = (bone_name == "r_f_leg3" || bone_name == "r_f_leg_finger" || 
			bone_name == "l_f_leg3" || bone_name == "l_f_leg_finger" ||
			bone_name.find("leg") != std::string::npos);

		if (cluster_vertices.find(bone_name) != cluster_vertices.end())
		{
			auto& cluster_vertex_array = cluster_vertices[bone_name];
			
			// 🔍 CONDENSED DEBUG: Show cluster info only for target bones with issues
			if (DebugConfig::shouldLog() && isLOD0 && isTargetBone && cluster_vertex_array.size() < 5) {
				LOG_SKELETON(bone.name << " has only " << cluster_vertex_array.size() << " weighted vertices [LOW]");
			}
			
			for (const auto& vertex_weight : cluster_vertex_array)
				cluster->AddControlPointIndex(vertex_weight.first, vertex_weight.second);
		}
		else if (isLOD0 && isTargetBone) {
			LOG_ERROR(bone.name << " has NO weighted vertices [ERROR]");
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

	// create bind pose
	// The bind pose must contain the mesh node, the armature node, and every
	// bone node in the deformation chain.  Do NOT include the FBX scene root.
	FbxAMatrix matrix;
	pose_ptr->Add(parent_ptr, parent_ptr->EvaluateGlobalTransform());
	pose_ptr->Add(skeleton_parent_ptr, skeleton_parent_ptr->EvaluateGlobalTransform());
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		matrix = nodes[bone_num]->EvaluateGlobalTransform();
		pose_ptr->Add(nodes[bone_num], matrix);
	}
	scene_ptr->AddPose(pose_ptr);

	if (DebugConfig::shouldLog() && isLOD0) {
		LOG_SKELETON("Generation complete");
	}

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

	// 🔍 CONDENSED DEBUG: Focus on problematic r_f_leg bones - first animation only
	bool isTargetBone = (boneNameLower == "r_f_leg3" || boneNameLower == "r_f_leg_finger" || 
		boneNameLower == "l_f_leg3" || boneNameLower == "l_f_leg_finger" ||
		boneNameLower.find("leg") != std::string::npos);

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

	// 🔍 CONDENSED DEBUG: Single line for target bones
	if (isTargetBone) {
		LOG_ANIMATION(animatedBoneIterator.name 
			<< " | Frames: " << animationObject->get_info().frame_count 
			<< " | Rotations: " << (animatedBoneIterator.has_rotations ? "YES" : "NO") 
			<< " | Compressed: " << (!animationObject->checkIsUnCompressed() ? "YES" : "NO"));
	}

	QuatExpand::UncompressQuaternion decompressValues;
	decompressValues.install();

	// All mathematical computations here - no disk I/O
	for (int frameCounter = 0; frameCounter < animationObject->get_info().frame_count + 1; frameCounter++)
	{
		FbxVector4 TranslationVector;
		FbxVector4 RotationVector;

		// Translation Extraction - condensed
		if (animatedBoneIterator.hasXAnimatedTranslation) {
			std::vector<float> translationValues = animationObject->getCHNLValues().at(animatedBoneIterator.x_translation_channel_index);
			float translationValue = translationValues.at(frameCounter);
			TranslationVector.mData[0] = (translationValue != -1000) ? translationValue : -1000.0;
		} else {
			float translationValue = animationObject->getStaticTranslationValues().at(animatedBoneIterator.x_translation_channel_index);
			TranslationVector.mData[0] = translationValue;
		}

		if (animatedBoneIterator.hasYAnimatedTranslation) {
			std::vector<float> translationValues = animationObject->getCHNLValues().at(animatedBoneIterator.y_translation_channel_index);
			float translationValue = translationValues.at(frameCounter);
			TranslationVector.mData[1] = (translationValue != -1000) ? translationValue : -1000.0;
		} else {
			float translationValue = animationObject->getStaticTranslationValues().at(animatedBoneIterator.y_translation_channel_index);
			TranslationVector.mData[1] = translationValue;
		}

		if (animatedBoneIterator.hasZAnimatedTranslation) {
			std::vector<float> translationValues = animationObject->getCHNLValues().at(animatedBoneIterator.z_translation_channel_index);
			float translationValue = translationValues.at(frameCounter);
			TranslationVector.mData[2] = (translationValue != -1000) ? translationValue : -1000.0;
		} else {
			float translationValue = animationObject->getStaticTranslationValues().at(animatedBoneIterator.z_translation_channel_index);
			TranslationVector.mData[2] = translationValue;
		}

		// Rotation extraction - condensed
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