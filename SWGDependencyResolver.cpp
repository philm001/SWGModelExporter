#include "stdafx.h"
#include "SWGMainObject.h"

void SWGMainObject::resolveDependecies()
{
	// p_CompleteModels is the "master" vector that stores the data
// This code is suppose to be iteratering through this data structure and making
// modifications to what is inside of it
	std::vector<std::vector<Animated_mesh>>& ModelCopy = p_CompleteModels;
	Context& referenceContext = p_Context;

	std::cout << "Resolving dependencies..." << std::endl;
	std::for_each(p_Context.object_list.begin(), p_Context.object_list.end(),
		[&referenceContext, &ModelCopy](const std::pair<std::string, std::shared_ptr<Base_object>>& item)
		{
			std::cout << "Object : " << item.first;
			if (item.first.find("mgn") != std::string::npos)
			{
				for (auto& listIterator : ModelCopy)
				{
					if (listIterator.at(0).get_object_name() == item.first)
					{
						for (auto& modelIterator : listIterator)
						{
							auto it = referenceContext.opened_by.find(modelIterator.get_object_name());
							std::string openedByName;

							while (it != referenceContext.opened_by.end())
							{
								openedByName = it->second;// Note: Stops on lmg nd not Sat
								it = referenceContext.opened_by.find(openedByName);
							}

							// I want to edit the shaders that are inside the data structure.
							// There are multiple shaders for each model. So we also need to iterate through them
							auto shaderPointer = modelIterator.ShaderPointer();// Just getting the pointer that is pointing to the vector containing the shaders used

							for (auto& shaderIterator : modelIterator.getShaders())
							{
								auto obj_it = referenceContext.object_list.find(shaderIterator.get_name());
								if (obj_it != referenceContext.object_list.end())
								{
									if (std::dynamic_pointer_cast<Shader>(obj_it->second))
									{
										// Once checks are passed, we need to set the shader definition.
										// This is a where everything gets tricky.
										// Function takes in a shared pointer and sets
										// that equal to the definition object stored within the shader
										// However, when viewing from the scope of the p_CompleteModels,
										// the definition object is still empty
										shaderIterator.set_definition(std::dynamic_pointer_cast<Shader>(obj_it->second));
									}
								}
							}


							auto bad_shaders = std::any_of(modelIterator.getShaders().begin(), modelIterator.getShaders().end(),
								[](const Animated_mesh::Shader_appliance& shader) { return shader.get_definition() == nullptr; });

							if (bad_shaders)
							{

								std::vector<uint8_t> counters(modelIterator.get_vertices().size(), 0);
								for (auto& shader : modelIterator.getShaders())
								{
									for (auto& vert_idx : shader.get_pos_indexes())
									{
										if (vert_idx >= counters.size())
											break;
										counters[vert_idx]++;
									}


									if (shader.get_definition() == nullptr)
									{
										for (auto& vert_idx : shader.get_pos_indexes())
										{
											if (vert_idx >= counters.size())
												break;
											counters[vert_idx]--;
										}
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
									std::vector<Animated_mesh::Vertex> vertexMesh = modelIterator.get_vertices();

									vertexMesh.erase(vertexMesh.begin() + idx, vertexMesh.end());

									for (size_t idx = 0; idx < modelIterator.get_vertices().size(); ++idx)
									{
										if (modelIterator.getShaders().at(idx).get_definition() == nullptr)
										{
											modelIterator.getShaders().erase(modelIterator.getShaders().begin() + idx);
											break;
										}
									}
								}
							}

							auto mesh_description = std::dynamic_pointer_cast<Animated_object_descriptor>(referenceContext.object_list.find(openedByName)->second);

							// get skeletons
							size_t skel_idx = 0;
							for (auto it_skel = modelIterator.getSkeletonNames().begin(); it_skel != modelIterator.getSkeletonNames().end(); ++it_skel, ++skel_idx)
							{
								auto skel_name = *it_skel;
								// check name against name in description
								if (mesh_description)
								{
									auto descr_skel_name = mesh_description->get_skeleton_name(skel_idx);
									if (!boost::iequals(skel_name, descr_skel_name))
										skel_name = descr_skel_name;
								}
								auto obj_it = referenceContext.object_list.find(skel_name);
								if (obj_it != referenceContext.object_list.end() && std::dynamic_pointer_cast<Skeleton>(obj_it->second))
									modelIterator.getSkeleton().emplace_back(skel_name, std::dynamic_pointer_cast<Skeleton>(obj_it->second)->clone());
							}

							if (!openedByName.empty() && modelIterator.getSkeleton().size() > 1 && mesh_description)
							{
								// check if we opened through SAT and have multiple skeleton definitions
								//  then we have unite them to one, so we need SAT to get join information from it's skeleton info;
								auto skel_count = mesh_description->get_skeletons_count();
								std::shared_ptr<Skeleton> root_skeleton;
								for (uint32_t skel_idx = 0; skel_idx < skel_count; ++skel_idx)
								{
									auto skel_name = mesh_description->get_skeleton_name(skel_idx);
									auto point_name = mesh_description->get_skeleton_attach_point(skel_idx);

									auto this_it = std::find_if(modelIterator.getSkeleton().begin(), modelIterator.getSkeleton().end(),
										[&skel_name](const std::pair<std::string, std::shared_ptr<Skeleton>>& info)
										{
											return (boost::iequals(skel_name, info.first));
										});

									if (point_name.empty() && this_it != modelIterator.getSkeleton().end())
										// mark this skeleton as root
										root_skeleton = this_it->second;
									else if (this_it != modelIterator.getSkeleton().end())
									{
										// attach skeleton to root
										root_skeleton->join_skeleton_to_point(point_name, this_it->second);
										// remove it from used list
										modelIterator.getSkeleton().erase(this_it);
									}
								}
							}
						}


					}
				}
			}
			else
			{
				item.second->resolve_dependencies(referenceContext);
			}
			std::cout << " done." << std::endl;
		});
}