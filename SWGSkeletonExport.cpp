#include "stdafx.h"
#include "SWGMainObject.h"

std::vector<Skeleton::Bone> SWGMainObject::generateSkeletonInScene(FbxScene* scene_ptr, FbxNode* parent_ptr, std::vector<Animated_mesh>& mesh)
{
	assert(parent_ptr != nullptr && scene_ptr != nullptr);
	std::vector<Skeleton::Bone> boneListing;

	uint32_t boneCount = get_bones_count(0); // Turn this into a loop??

	// 🔍 DEBUG: Only print for LOD 0 models to reduce console spam
	bool isLOD0 = mesh.size() > 0 && mesh[0].getLodLevel() == 0;
	if (isLOD0) {
		std::cout << "\n=== SKELETON GENERATION DEBUG (LOD 0) ===\n";
		std::cout << "Total bones: " << boneCount << std::endl;
	}

	std::vector<FbxNode*> nodes(boneCount, nullptr);
	std::vector<FbxCluster*> clusters(boneCount, nullptr);

	auto pose_ptr2 = FbxPose::Create(scene_ptr, "Rest Pose"); // Also create the binding pose
	pose_ptr2->SetIsBindPose(true);

	for (uint32_t boneCounter = 0; boneCounter < boneCount; boneCounter++)
	{
		Skeleton::Bone& bone = getBone(boneCounter, 0);

		// 🔍 DEBUG: Focus on problematic leg bones (both left and right)
		bool isTargetBone = (bone.name == "r_f_leg" || bone.name == "r_f_leg2" || bone.name == "r_f_leg3" ||
			bone.name == "l_f_leg" || bone.name == "l_f_leg2" || bone.name == "l_f_leg3" ||
			bone.name == "l_m_leg" || bone.name == "l_m_leg2" || bone.name == "l_m_leg3") ||
			(boneCounter >= 18 && boneCounter <= 25); // Expand range to include more bones
		
		if (isLOD0 && isTargetBone) {
			std::cout << "\n--- BONE DEBUG [" << boneCounter << "] " << bone.name << " ---\n";
			std::cout << "Parent index: " << bone.parent_idx << std::endl;
			
			// Raw quaternion data
			std::cout << "Pre-rotation quaternion: (" 
				<< bone.pre_rot_quaternion.x << ", " << bone.pre_rot_quaternion.y << ", " 
				<< bone.pre_rot_quaternion.z << ", " << bone.pre_rot_quaternion.a << ")\n";
			std::cout << "Post-rotation quaternion: (" 
				<< bone.post_rot_quaternion.x << ", " << bone.post_rot_quaternion.y << ", " 
				<< bone.post_rot_quaternion.z << ", " << bone.post_rot_quaternion.a << ")\n";
			std::cout << "Bind-pose rotation quaternion: (" 
				<< bone.bind_pose_rotation.x << ", " << bone.bind_pose_rotation.y << ", " 
				<< bone.bind_pose_rotation.z << ", " << bone.bind_pose_rotation.a << ")\n";
			std::cout << "Bind-pose translation: (" 
				<< bone.bind_pose_transform.x << ", " << bone.bind_pose_transform.y << ", " 
				<< bone.bind_pose_transform.z << ")\n";
		}

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
		
		// 🔍 DEBUG: Show magnitude comparison for all bones with large rotations
		if (isLOD0 && (std::abs(test_euler[0]) > 100.0 || std::abs(test_euler[1]) > 100.0 || std::abs(test_euler[2]) > 100.0)) {
			double original_magnitude = sqrt(test_euler[0]*test_euler[0] + test_euler[1]*test_euler[1] + test_euler[2]*test_euler[2]);
			std::cout << "EULER CORRECTION CHECK for " << bone.name << ": Original magnitude=" << original_magnitude << "\n";
			std::cout << "  Original Euler: (" << test_euler[0] << ", " << test_euler[1] << ", " << test_euler[2] << ")\n";
		}
		
		// ✅ CRITICAL FIX: Apply Euler angle normalization for bones with large rotations
		// Convert large angles to their equivalent smaller representations
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
			double original_magnitude = sqrt(test_euler[0]*test_euler[0] + test_euler[1]*test_euler[1] + test_euler[2]*test_euler[2]);
			double corrected_magnitude = sqrt(corrected_euler[0]*corrected_euler[0] + corrected_euler[1]*corrected_euler[1] + corrected_euler[2]*corrected_euler[2]);
			
			if (isLOD0 && isTargetBone) {
				std::cout << "APPLYING EULER ANGLE CORRECTION for " << bone.name << "\n";
				std::cout << "  Original: (" << test_euler[0] << ", " << test_euler[1] << ", " << test_euler[2] << ") mag=" << original_magnitude << "\n";
				std::cout << "  Corrected: (" << corrected_euler[0] << ", " << corrected_euler[1] << ", " << corrected_euler[2] << ") mag=" << corrected_magnitude << "\n";
			}
			
			// Create corrected quaternion from the normalized Euler angles
			FbxQuaternion corrected_quat;
			corrected_quat.ComposeSphericalXYZ(corrected_euler);
			combined_rot = corrected_quat;
		}

		// 🔍 DEBUG: Detailed rotation analysis for target bones
		if (isLOD0 && isTargetBone) {
			auto pre_euler = pre_rot_quat.DecomposeSphericalXYZ();
			auto post_euler = post_rot_quat.DecomposeSphericalXYZ();
			auto bind_euler = bind_rot_quat.DecomposeSphericalXYZ();
			auto combined_euler = combined_rot.DecomposeSphericalXYZ();
			
			std::cout << "Pre-rotation Euler (deg): (" 
				<< pre_euler[0] << ", " << pre_euler[1] << ", " << pre_euler[2] << ")\n";
			std::cout << "Post-rotation Euler (deg): (" 
				<< post_euler[0] << ", " << post_euler[1] << ", " << post_euler[2] << ")\n";
			std::cout << "Bind-pose Euler (deg): (" 
				<< bind_euler[0] << ", " << bind_euler[1] << ", " << bind_euler[2] << ")\n";
			std::cout << "COMBINED rotation Euler (deg): (" 
				<< combined_euler[0] << ", " << combined_euler[1] << ", " << combined_euler[2] << ")\n";
			
			// 🔍 ADVANCED DEBUG: Check quaternion magnitudes and intermediate steps
			double pre_mag = sqrt(pre_rot_quat.mData[0]*pre_rot_quat.mData[0] + pre_rot_quat.mData[1]*pre_rot_quat.mData[1] + 
				pre_rot_quat.mData[2]*pre_rot_quat.mData[2] + pre_rot_quat.mData[3]*pre_rot_quat.mData[3]);
			double post_mag = sqrt(post_rot_quat.mData[0]*post_rot_quat.mData[0] + post_rot_quat.mData[1]*post_rot_quat.mData[1] + 
				post_rot_quat.mData[2]*post_rot_quat.mData[2] + post_rot_quat.mData[3]*post_rot_quat.mData[3]);
			double bind_mag = sqrt(bind_rot_quat.mData[0]*bind_rot_quat.mData[0] + bind_rot_quat.mData[1]*bind_rot_quat.mData[1] + 
				bind_rot_quat.mData[2]*bind_rot_quat.mData[2] + bind_rot_quat.mData[3]*bind_rot_quat.mData[3]);
			double combined_mag = sqrt(combined_rot.mData[0]*combined_rot.mData[0] + combined_rot.mData[1]*combined_rot.mData[1] + 
				combined_rot.mData[2]*combined_rot.mData[2] + combined_rot.mData[3]*combined_rot.mData[3]);
				
			std::cout << "Quaternion magnitudes - Pre: " << pre_mag << ", Post: " << post_mag 
				<< ", Bind: " << bind_mag << ", Combined: " << combined_mag << "\n";
			
			// 🔍 ADVANCED DEBUG: Test different quaternion combination orders
			auto alt_combined1 = pre_rot_quat * bind_rot_quat * post_rot_quat;
			auto alt_combined2 = bind_rot_quat * post_rot_quat * pre_rot_quat;
			auto alt_euler1 = alt_combined1.DecomposeSphericalXYZ();
			auto alt_euler2 = alt_combined2.DecomposeSphericalXYZ();
			
			std::cout << "ALT ORDER 1 (pre*bind*post): (" << alt_euler1[0] << ", " << alt_euler1[1] << ", " << alt_euler1[2] << ")\n";
			std::cout << "ALT ORDER 2 (bind*post*pre): (" << alt_euler2[0] << ", " << alt_euler2[1] << ", " << alt_euler2[2] << ")\n";
			
			// 🔍 ADVANCED DEBUG: Check for quaternion flipping (w component sign)
			std::cout << "Quat W components - Pre: " << pre_rot_quat.mData[3] << ", Post: " << post_rot_quat.mData[3] 
				<< ", Bind: " << bind_rot_quat.mData[3] << ", Combined: " << combined_rot.mData[3] << "\n";
		}

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

		// 🔍 DEBUG: Show final FBX node transforms for target bones
		if (isLOD0 && isTargetBone) {
			std::cout << "Final FBX Local Translation: (" << lT[0] << ", " << lT[1] << ", " << lT[2] << ")\n";
			std::cout << "Final FBX Local Rotation: (" << lR[0] << ", " << lR[1] << ", " << lR[2] << ")\n";
			std::cout << "Final FBX Local Scale: (" << lS[0] << ", " << lS[1] << ", " << lS[2] << ")\n";
		}

		boneListing.push_back(bone);
		nodes[boneCounter] = node_ptr;
		bone.boneNodeptr = node_ptr;
	}

	// build hierarchy
	if (isLOD0) {
		std::cout << "\n=== BUILDING BONE HIERARCHY ===\n";
	}
	
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto idx_parent = bone.parent_idx;
		
		// 🔍 DEBUG: Track hierarchy for target bones
		bool isTargetBone = (bone.name == "r_f_leg" || bone.name == "r_f_leg2" || bone.name == "r_f_leg3" ||
			bone.name == "l_f_leg" || bone.name == "l_f_leg2" || bone.name == "l_f_leg3" ||
			bone.name == "l_m_leg" || bone.name == "l_m_leg2" || bone.name == "l_m_leg3") ||
			(bone_num >= 18 && bone_num <= 25);
		
		if (isLOD0 && isTargetBone) {
			if (idx_parent == -1) {
				std::cout << "Bone [" << bone_num << "] " << bone.name << " -> ROOT\n";
			} else {
				Skeleton::Bone& parentBone = getBone(idx_parent, 0);
				std::cout << "Bone [" << bone_num << "] " << bone.name << " -> Parent [" << idx_parent << "] " << parentBone.name << "\n";
			}
		}
		
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

	if (isLOD0) {
		std::cout << "\n=== VERTEX WEIGHT CLUSTERING ===\n";
		std::cout << "Mesh parent global transform:\n";
		for (int i = 0; i < 4; i++) {
			std::cout << "  [" << xmatr.Get(i, 0) << ", " << xmatr.Get(i, 1) << ", " << xmatr.Get(i, 2) << ", " << xmatr.Get(i, 3) << "]\n";
		}
	}

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

		// 🔍 DEBUG: Track vertex weight assignments for target bones
		if (isLOD0) {
			std::cout << "Processing mesh " << cc << " with " << vertices.size() << " vertices\n";
		}

		for (uint32_t vertex_num = 0; vertex_num < vertices.size(); ++vertex_num)
		{
			uint32_t finalVertexNum = vertex_num + counter;
			const auto& vertex = vertices[vertex_num];
			
			// 🔍 DEBUG: Track vertices that are weighted to target bones
			bool hasTargetBoneWeight = false;
			std::string targetBoneWeights = "";
			
			for (const auto& weight : vertex.get_weights())
			{
				auto joint_name = mesh_joint_names[weight.first];
				std::string joint_name_lower = joint_name;
				boost::to_lower(joint_name_lower);
				
				// Check if this vertex is weighted to a target bone
				if (isLOD0 && (joint_name_lower == "r_f_leg" || joint_name_lower == "r_f_leg2" || joint_name_lower == "r_f_leg3" ||
					joint_name_lower == "l_f_leg" || joint_name_lower == "l_f_leg2" || joint_name_lower == "l_f_leg3" ||
					joint_name_lower == "l_m_leg" || joint_name_lower == "l_m_leg2" || joint_name_lower == "l_m_leg3")) {
					hasTargetBoneWeight = true;
					targetBoneWeights += joint_name + "(" + std::to_string(weight.second) + ") ";
				}
				
				cluster_vertices[joint_name_lower].emplace_back(finalVertexNum, weight.second);
			}
			
			// 🔍 DEBUG: Log vertices with target bone weights (sample only first 10)
			if (isLOD0 && hasTargetBoneWeight && finalVertexNum < 10) {
				auto pos = vertex.get_position();
				std::cout << "Vertex " << finalVertexNum << " at (" << pos.x << ", " << pos.y << ", " << pos.z 
					<< ") weighted to: " << targetBoneWeights << "\n";
			}
		}

		counter += static_cast<uint32_t>(modelIterator.get_vertices().size());
	}

	if (isLOD0) {
		std::cout << "\n=== CREATING FBX CLUSTERS ===\n";
	}

	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		Skeleton::Bone& bone = getBone(bone_num, 0);
		auto cluster = FbxCluster::Create(scene_ptr, bone.name.c_str());
		cluster->SetLink(nodes[bone_num]);
		cluster->SetLinkMode(FbxCluster::eNormalize);

		auto bone_name = bone.name;
		boost::to_lower(bone_name);

		bool isTargetBone = (bone_name == "r_f_leg" || bone_name == "r_f_leg2" || bone_name == "r_f_leg3" ||
			bone_name == "l_f_leg" || bone_name == "l_f_leg2" || bone_name == "l_f_leg3" ||
			bone_name == "l_m_leg" || bone_name == "l_m_leg2" || bone_name == "l_m_leg3") ||
			(bone_num >= 18 && bone_num <= 25);

		if (cluster_vertices.find(bone_name) != cluster_vertices.end())
		{
			auto& cluster_vertex_array = cluster_vertices[bone_name];
			
			// 🔍 DEBUG: Show cluster info for target bones
			if (isLOD0 && isTargetBone) {
				std::cout << "Cluster for bone [" << bone_num << "] " << bone.name 
					<< " has " << cluster_vertex_array.size() << " weighted vertices\n";
				
				// Show first few vertex weights
				for (size_t i = 0; i < std::min(size_t(5), cluster_vertex_array.size()); i++) {
					std::cout << "  Vertex " << cluster_vertex_array[i].first 
						<< " weight: " << cluster_vertex_array[i].second << "\n";
				}
			}
			
			for (const auto& vertex_weight : cluster_vertex_array)
				cluster->AddControlPointIndex(vertex_weight.first, vertex_weight.second);
		}
		else if (isLOD0 && isTargetBone) {
			std::cout << "WARNING: No vertices found for target bone " << bone.name << "\n";
		}

		cluster->SetTransformMatrix(xmatr);
		link_transform = nodes[bone_num]->EvaluateGlobalTransform();
		cluster->SetTransformLinkMatrix(link_transform);

		// 🔍 DEBUG: Show transform matrices for target bones
		if (isLOD0 && isTargetBone) {
			std::cout << "Bone [" << bone_num << "] " << bone.name << " global transform:\n";
			for (int i = 0; i < 4; i++) {
				std::cout << "  [" << link_transform.Get(i, 0) << ", " << link_transform.Get(i, 1) 
					<< ", " << link_transform.Get(i, 2) << ", " << link_transform.Get(i, 3) << "]\n";
			}
			
			// 🔍 ADVANCED DEBUG: Verify transform matrix properties
			FbxVector4 translation = link_transform.GetT();
			FbxVector4 rotation = link_transform.GetR();
			FbxVector4 scale = link_transform.GetS();
			
			std::cout << "Matrix decomposition - Translation: (" << translation[0] << ", " << translation[1] << ", " << translation[2] << ")\n";
			std::cout << "Matrix decomposition - Rotation: (" << rotation[0] << ", " << rotation[1] << ", " << rotation[2] << ")\n";
			std::cout << "Matrix decomposition - Scale: (" << scale[0] << ", " << scale[1] << ", " << scale[2] << ")\n";
			
			// 🔍 ADVANCED DEBUG: Calculate determinant manually to check for reflection
			double det = link_transform.Determinant();
			std::cout << "Transform matrix determinant: " << det << " (should be ~1.0 for proper rotation)\n";
			
			// Check for negative determinant (reflection)
			if (det < 0) {
				std::cout << "WARNING: Negative determinant detected - indicates reflection/mirroring!\n";
			}
			
			// 🔍 ADVANCED DEBUG: Compare with expected parent-relative transform
			if (bone.parent_idx != -1) {
				FbxAMatrix parent_global = nodes[bone.parent_idx]->EvaluateGlobalTransform();
				FbxAMatrix local_expected = parent_global.Inverse() * link_transform;
				auto local_expected_rot = local_expected.GetR();
				
				// Get the actual local rotation that was set on this node
				FbxVector4 actual_local_rot = FbxVector4(nodes[bone_num]->LclRotation.Get());
				
				std::cout << "Expected local rotation from matrices: (" << local_expected_rot[0] 
					<< ", " << local_expected_rot[1] << ", " << local_expected_rot[2] << ")\n";
				std::cout << "Actual local rotation from node: (" << actual_local_rot[0] << ", " << actual_local_rot[1] << ", " << actual_local_rot[2] << ")\n";
				
				// Check if they match within tolerance
				double rot_diff = sqrt(pow(local_expected_rot[0] - actual_local_rot[0], 2) + 
					pow(local_expected_rot[1] - actual_local_rot[1], 2) + pow(local_expected_rot[2] - actual_local_rot[2], 2));
				std::cout << "Local rotation difference: " << rot_diff << " degrees\n";
				
				if (rot_diff > 1.0) {
					std::cout << "WARNING: Large discrepancy between expected and actual local rotations!\n";
				}
			}
		}

		clusters[bone_num] = cluster;
		skin->AddCluster(cluster);
	}

	mesh_attr->AddDeformer(skin);

	auto pose_ptr = FbxPose::Create(scene_ptr, parent_ptr->GetName());
	pose_ptr->SetIsBindPose(true);

	if (isLOD0) {
		std::cout << "\n=== CREATING BIND POSE ===\n";
	}

	FbxAMatrix matrix;
	for (uint32_t bone_num = 0; bone_num < boneCount; ++bone_num)
	{
		matrix = nodes[bone_num]->EvaluateGlobalTransform();
		FbxVector4 TranVec = matrix.GetT();
		FbxVector4 RotVec = matrix.GetR();

		bool isTargetBone = (bone_num >= 18 && bone_num <= 25);
		if (isLOD0 && isTargetBone) {
			Skeleton::Bone& bone = getBone(bone_num, 0);
			std::cout << "Bind pose for bone [" << bone_num << "] " << bone.name << ":\n";
			std::cout << "  Translation: (" << TranVec[0] << ", " << TranVec[1] << ", " << TranVec[2] << ")\n";
			std::cout << "  Rotation: (" << RotVec[0] << ", " << RotVec[1] << ", " << RotVec[2] << ")\n";
		}

		pose_ptr->Add(nodes[bone_num], matrix);
	}
	scene_ptr->AddPose(pose_ptr);
	
	if (isLOD0) {
		std::cout << "=== SKELETON GENERATION COMPLETE ===\n\n";
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

	// 🔍 DEBUG: Focus on problematic r_f_leg bones
	bool isTargetBone = (boneNameLower == "r_f_leg" || boneNameLower == "r_f_leg2" || boneNameLower == "r_f_leg3");
	
	if (isTargetBone) {
		std::cout << "\n=== ANIMATION DEBUG: " << animatedBoneIterator.name << " ===\n";
		std::cout << "Processing animation data for target bone\n";
		std::cout << "Has rotations: " << (animatedBoneIterator.has_rotations ? "YES" : "NO") << "\n";
		std::cout << "Animation compressed: " << (!animationObject->checkIsUnCompressed() ? "YES" : "NO") << "\n";
		std::cout << "Frame count: " << animationObject->get_info().frame_count << "\n";
	}

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
		if (isTargetBone) {
			std::cout << "ERROR: Could not find skeleton bone for " << animatedBoneIterator.name << "\n";
		}
		return results; // Invalid bone
	}

	if (isTargetBone) {
		std::cout << "Found skeleton bone - processing animation frames...\n";
		std::cout << "Skeleton bone bind data:\n";
		std::cout << "  Bind pose transform: (" << skeletonBone.bind_pose_transform.x << ", " 
			<< skeletonBone.bind_pose_transform.y << ", " << skeletonBone.bind_pose_transform.z << ")\n";
		std::cout << "  Bind pose rotation: (" << skeletonBone.bind_pose_rotation.x << ", " 
			<< skeletonBone.bind_pose_rotation.y << ", " << skeletonBone.bind_pose_rotation.z 
			<< ", " << skeletonBone.bind_pose_rotation.a << ")\n";
		std::cout << "  Pre-rotation: (" << skeletonBone.pre_rot_quaternion.x << ", " 
			<< skeletonBone.pre_rot_quaternion.y << ", " << skeletonBone.pre_rot_quaternion.z 
			<< ", " << skeletonBone.pre_rot_quaternion.a << ")\n";
		std::cout << "  Post-rotation: (" << skeletonBone.post_rot_quaternion.x << ", " 
			<< skeletonBone.post_rot_quaternion.y << ", " << skeletonBone.post_rot_quaternion.z 
			<< ", " << skeletonBone.post_rot_quaternion.a << ")\n";
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
					
					// 🔍 DEBUG: Show decompressed quaternion for target bones
					if (isTargetBone && frameCounter < 3) { // Only first 3 frames to avoid spam
						std::cout << "Frame " << frameCounter << " decompressed quat: (" 
							<< Quat.x << ", " << Quat.y << ", " << Quat.z << ", " << Quat.a << ")\n";
					}
					
					result = ConvertCombineCompressQuat(Quat, skeletonBone);
					RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
					
					// 🔍 DEBUG: Show final Euler angles for target bones
					if (isTargetBone && frameCounter < 3) {
						std::cout << "Frame " << frameCounter << " final Euler: (" 
							<< result.roll << ", " << result.pitch << ", " << result.yaw << ")\n";
					}
				}
				else
				{
					RotationVector = FbxVector4(-1000.0, -1000.0, -1000.0);
					if (isTargetBone && frameCounter < 3) {
						std::cout << "Frame " << frameCounter << " - skipped (compressed value = 100)\n";
					}
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
					
					// 🔍 DEBUG: Show uncompressed quaternion for target bones
					if (isTargetBone && frameCounter < 3) {
						std::cout << "Frame " << frameCounter << " uncompressed quat: (" 
							<< Quat.x << ", " << Quat.y << ", " << Quat.z << ", " << Quat.a << ")\n";
					}
					
					result = ConvertCombineCompressQuat(Quat, skeletonBone);
					RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
					
					// 🔍 DEBUG: Show final Euler angles for target bones
					if (isTargetBone && frameCounter < 3) {
						std::cout << "Frame " << frameCounter << " final Euler: (" 
							<< result.roll << ", " << result.pitch << ", " << result.yaw << ")\n";
					}
				}
				else
				{
					RotationVector = FbxVector4(-1000.0, -1000.0, -1000.0);
					if (isTargetBone && frameCounter < 3) {
						std::cout << "Frame " << frameCounter << " - skipped (uncompressed value = 100)\n";
					}
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
				
				// 🔍 DEBUG: Show static rotation data for target bones
				if (isTargetBone) {
					std::cout << "Static compressed quat: (" << Quat.x << ", " << Quat.y << ", " << Quat.z << ", " << Quat.a << ")\n";
				}
				
				EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);
				RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
				
				if (isTargetBone) {
					std::cout << "Static final Euler: (" << result.roll << ", " << result.pitch << ", " << result.yaw << ")\n";
				}
			}
			else
			{
				std::vector<float> uncompressedValues = animationObject->getStaticKFATRotationValues().at(animatedBoneIterator.rotation_channel_index);
				Geometry::Vector4 Quat = { uncompressedValues.at(1), uncompressedValues.at(2), uncompressedValues.at(3), uncompressedValues.at(0) };
				
				// 🔍 DEBUG: Show static rotation data for target bones
				if (isTargetBone) {
					std::cout << "Static uncompressed quat: (" << Quat.x << ", " << Quat.y << ", " << Quat.z << ", " << Quat.a << ")\n";
				}
				
				EulerAngles result = ConvertCombineCompressQuat(Quat, skeletonBone, true);
				RotationVector = FbxVector4(result.roll, result.pitch, result.yaw);
				
				if (isTargetBone) {
					std::cout << "Static final Euler: (" << result.roll << ", " << result.pitch << ", " << result.yaw << ")\n";
				}
			}
		}

		// 🔍 DEBUG: Show translation data for target bones (first few frames)
		if (isTargetBone && frameCounter < 3) {
			std::cout << "Frame " << frameCounter << " translation: (" 
				<< TranslationVector.mData[0] << ", " << TranslationVector.mData[1] << ", " << TranslationVector.mData[2] << ")\n";
		}

		AnimationCurveData data;
		data.frameIndex = frameCounter;
		data.translation = TranslationVector;
		data.rotation = RotationVector;
		data.boneName = skeletonBone.name;
		results.push_back(data);
	}

	if (isTargetBone) {
		std::cout << "Animation processing complete for " << animatedBoneIterator.name 
			<< " - " << results.size() << " frames processed\n";
		std::cout << "=== END ANIMATION DEBUG ===\n\n";
	}

	return results;
}

// Explicit template instantiation to resolve linker errors
template std::vector<SWGMainObject::AnimationCurveData> SWGMainObject::calculateBoneAnimationData<Animation::Bone_info>(
	const Animation::Bone_info& animatedBoneIterator,
	std::shared_ptr<Animation> animationObject);