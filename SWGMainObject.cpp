#include "stdafx.h"
#include "SWGMainObject.h"
#include <iomanip>

void SWGMainObject::beginParsingProcess(std::queue<std::string> queueArray, std::string output_pathname, bool overwrite)
{
	p_queueArray = queueArray;
	
	while (!p_queueArray.empty())
	{
		std::string assetName;
		assetName = p_queueArray.front();
		p_queueArray.pop();

		boost::filesystem::path obj_name(assetName);
		boost::filesystem::path target_path(output_pathname);
		target_path /= obj_name.filename();
		target_path.replace_extension("fbx");

		if (!overwrite)
		{
			if (boost::filesystem::exists(target_path))
			{
				continue;
			}
		}
		
		// normalize path
		std::replace_if(assetName.begin(), assetName.end(), [](const char& value) { return value == '\\'; }, '/');
		if (assetName.length() <= 3)
		{
			std::cout << "Invalid Name: " + assetName << std::endl;
			continue;
		}
		std::string ext = assetName.substr(assetName.length() - 3);
		boost::to_lower(ext);

		// skip already parsed object
		if (p_Context.object_list.find(assetName) != p_Context.object_list.end())
			continue;

		std::cout << "Processing : " << assetName << std::endl;
		if (assetName.find("no_render.iff") != std::string::npos ||
			assetName.find("defaultappearance") != std::string::npos 
			)// Skip this guy
		{
			std::cout << "Asset not compatible" << std::endl;
			continue;
		}

		// do not try find object on this step
		std::vector<uint8_t> buffer;
		
		if (!p_Library->get_object(assetName, buffer)) // Quick search
		{
			if(!p_Library->get_object(assetName, buffer, 0, true))// Deep search
				continue;
		}
			

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

					// === LOD FILTERING: Skip non-LOD0 meshes if --no-lod enabled ===
					if (DebugConfig::isNoLODEnabled() && level != 0) {
						if (DebugConfig::shouldLog()) {
							LOG_INFO("Skipping LOD " << level << " mesh: " << assetName << " (--no-lod enabled)");
						}
						// Skip this mesh, don't add to p_CompleteModels
					}
					else {

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

					}  // Close else block

					int a = 0;
					a++;
				}
				else
				{
					//object->store();
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

void SWGMainObject::processAnimationCurvesParallel(
	const std::vector<std::shared_ptr<Animation>>& animationList,
	FbxScene* scene_ptr,
	FbxNode* mesh_node_ptr)
{
	for (auto animationObject : animationList) {
		if (!animationObject) continue;
		
		// Parallelize bone processing (mathematical operations only)
		std::vector<std::future<std::vector<AnimationCurveData>>> bone_futures;
		
		for (auto& animatedBoneIterator : animationObject->get_bones()) {
			bone_futures.push_back(std::async(std::launch::async, [&, this]() -> std::vector<AnimationCurveData> {
				return calculateBoneAnimationData(animatedBoneIterator, animationObject);
			}));
		}
		
		// Sequential application to FBX (to avoid FBX SDK threading issues)
		for (auto& future : bone_futures) {
			auto curveData = future.get();
			if (!curveData.empty()) {
				applyAnimationDataToFBX(curveData, scene_ptr, mesh_node_ptr);
			}
		}
	}
}

void SWGMainObject::applyAnimationDataToFBX(
	const std::vector<AnimationCurveData>& curveData, 
	FbxScene* scene_ptr, 
	FbxNode* mesh_node_ptr)
{
	if (curveData.empty()) return;
	
	// Find the bone node
	std::string boneName = curveData[0].boneName;
	FbxNode* boneToUse = nullptr;
	
	for (auto& boneIterator : m_bones.at(0))
	{
		if (boneIterator.name == boneName)
		{
			boneToUse = boneIterator.boneNodeptr;
			break;
		}
	}
	
	if (!boneToUse) return;
	
	// Apply the pre-computed animation data to FBX curves
	// This is the sequential part that applies the parallel-computed data
	// Implementation would continue with the FBX curve creation logic...
}
void SWGMainObject::resolve_dependencies(const Context& context)
{
	std::cout << "Using the wrong resolve dependency function for this class. Use the other one" << std::endl;
}

void SWGMainObject::store(const std::string& path, const Context& context)
{
	std::cout << "Using the wrong resolve store object function for this class. Use the other one" << std::endl;
}

// Add the missing function implementations

SWGMainObject::EulerAngles SWGMainObject::ConvertCombineCompressQuat(Geometry::Vector4 DecompressedQuaterion, Skeleton::Bone BoneReference, bool isStatic)
{
	const double pi = 3.14159265358979323846;
	double rotationFactor = 180.0 / pi;
	EulerAngles angles;

	// 🔍 DEBUG: Focus on problematic bones
	std::string boneName = BoneReference.name;
	boost::to_lower(boneName);
	bool isTargetBone = (boneName == "r_f_leg3" || boneName == "r_f_leg_finger" || 
		boneName == "l_f_leg3" || boneName == "l_f_leg_finger" ||
		boneName.find("leg") != std::string::npos);

	FbxQuaternion anim_quat(DecompressedQuaterion.x, DecompressedQuaterion.y, DecompressedQuaterion.z, DecompressedQuaterion.a);
	FbxQuaternion pre_rot_quat(BoneReference.pre_rot_quaternion.x, BoneReference.pre_rot_quaternion.y, BoneReference.pre_rot_quaternion.z, BoneReference.pre_rot_quaternion.a);
	FbxQuaternion post_rot_quat(BoneReference.post_rot_quaternion.x, BoneReference.post_rot_quaternion.y, BoneReference.post_rot_quaternion.z, BoneReference.post_rot_quaternion.a);

	anim_quat.Normalize();
	pre_rot_quat.Normalize();
	post_rot_quat.Normalize();

	FbxQuaternion selected_quat = post_rot_quat * anim_quat * pre_rot_quat;
	selected_quat.Normalize();
	if (selected_quat[3] < 0)
		selected_quat = -selected_quat;

	// 🔍 CONDENSED DEBUG: Only show critical data for target bones
	if (DebugConfig::shouldLog() && isTargetBone) {
		std::cout << "[BONE] " << BoneReference.name << (isStatic ? " (static)" : "")
			<< " | Input: (" << std::fixed << std::setprecision(3) 
			<< DecompressedQuaterion.x << "," << DecompressedQuaterion.y << "," << DecompressedQuaterion.z << "," << DecompressedQuaterion.a << ")";
	}

	Geometry::Vector4 Quat(selected_quat[0], selected_quat[1], selected_quat[2], selected_quat[3]);

	double sinp = 2.0 * (Quat.a * Quat.y - Quat.z * Quat.x);

	if (std::abs(sinp) >= 1.0) {
		angles.pitch = std::copysign(pi / 2.0, sinp);
		angles.yaw = std::atan2(2.0 * (Quat.a * Quat.z + Quat.x * Quat.y), 1.0 - 2.0 * (Quat.y * Quat.y + Quat.z * Quat.z));
		angles.roll = 0.0;
	}
	else {
		angles.pitch = std::asin(sinp);
		angles.yaw = std::atan2(2.0 * (Quat.a * Quat.z + Quat.x * Quat.y), 1.0 - 2.0 * (Quat.y * Quat.y + Quat.z * Quat.z));
		angles.roll = std::atan2(2.0 * (Quat.a * Quat.x + Quat.y * Quat.z), 1.0 - 2.0 * (Quat.x * Quat.x + Quat.y * Quat.y));
	}

	angles.roll = std::fmod(angles.roll * rotationFactor + 180.0, 360.0) - 180.0;
	angles.pitch = std::fmod(angles.pitch * rotationFactor + 180.0, 360.0) - 180.0;
	angles.yaw = std::fmod(angles.yaw * rotationFactor + 180.0, 360.0) - 180.0;

	if (DebugConfig::shouldLog() && isTargetBone) {
		std::cout << " | Euler: (" << std::setprecision(1) << angles.roll << "," << angles.pitch << "," << angles.yaw << ")";
		std::cout << "\n";
	}

	return angles;
}





























