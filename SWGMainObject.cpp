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

		if (ext == "mgn")
		{
			std::shared_ptr<mgn_parser> mgnPointer;

			if (assetName.find("0") != std::string::npos)
			{
				if (p_CompleteParsers.size() == 0)
				{
					mgn_parser firstParser;
					p_CompleteParsers.push_back(firstParser);
					p_CompleteNames.push_back(assetName);
				}

				mgnPointer = std::make_shared<mgn_parser>(p_CompleteParsers.at(0));
			}
			else if (assetName.find("1") != std::string::npos)
			{
				if (p_CompleteParsers.size() == 1)
				{
					mgn_parser secondParser;
					p_CompleteParsers.push_back(secondParser);
					p_CompleteNames.push_back(assetName);
				}

				mgnPointer = std::make_shared<mgn_parser>(p_CompleteParsers.at(1));
			}
			else if (assetName.find("2") != std::string::npos)
			{
				if (p_CompleteParsers.size() == 2)
				{
					mgn_parser thirdParser;
					p_CompleteParsers.push_back(thirdParser);
					p_CompleteNames.push_back(assetName);
				}
				mgnPointer = std::make_shared<mgn_parser>(p_CompleteParsers.at(2));
			}
			else if (assetName.find("3") != std::string::npos)
			{
				if (p_CompleteParsers.size() == 3)
				{
					mgn_parser fourthParser;
					p_CompleteParsers.push_back(fourthParser);
					p_CompleteNames.push_back(assetName);
				}
				mgnPointer = std::make_shared<mgn_parser>(p_CompleteParsers.at(3));
			}

			p_Buffer.combinedObjectProcess(mgnPointer);
		}
		else
		{
			p_Buffer.combinedObjectProcess(m_selected_parser);

			if (m_selected_parser->is_object_parsed())
			{
				auto object = m_selected_parser->get_parsed_object();
				if (object)
				{
					object->set_object_name(assetName);

					if (ext != "mgn")
						p_Context.object_list.insert(make_pair(assetName, object));

					/*	if (ext == "mgn")
						{
							std::shared_ptr meehPtr = std::dynamic_pointer_cast<Animated_mesh>(object);
							Animated_mesh meshStore = *meehPtr;
							uint32_t level = meshStore.getLodLevel();

							switch (level)
							{
							case 0:
							{
								if (p_CompleteModels.size() == 0)
								{
									p_CompleteModels.push_back(meshStore);
								}
								else
								{
									std::shared_ptr<Animated_mesh> mesh_ptr = std::make_shared<Animated_mesh>(p_CompleteModels.at(0));
									CombineMeshProcess(mesh_ptr, meehPtr);
								}
								break;
							}
							case 1:
							{

								if (p_CompleteModels.size() == 1)
								{

								}
								else
								{

								}
								break;
							}
							case 2:
							{
								if (p_CompleteModels.size() == 2)
								{

								}
								else
								{

								}
								break;
							}
							case 4:
							{
								if (p_CompleteModels.size() == 3)
								{
									p_CompleteModels.push_back(meshStore);
								}
								else
								{

								}
								break;
							}
							default:
								break;
							}

							int a = 0;
							a++;
						}*/


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

	int counter = 0;
	for (auto parserIterator : p_CompleteParsers)
	{
		std::shared_ptr<mgn_parser> mgnPtr = std::make_shared<mgn_parser>(parserIterator);
		std::string assetName = p_CompleteNames.at(counter);
		if (mgnPtr->is_object_parsed())
		{
			auto object = mgnPtr->get_parsed_object();
			if (object)
			{
				object->set_object_name(assetName);
				p_Context.object_list.insert(make_pair(assetName, object));

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

		counter++;
	}

/*	for (auto modelIterator : p_CompleteModels)
	{
		p_Context.object_list.insert(make_pair(modelIterator.get_object_name(), std::make_shared<Animated_mesh>(modelIterator)));
	}
	*/
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

	std::cout << "Store objects..." << std::endl;
	std::for_each(otherContext.object_list.begin(), otherContext.object_list.end(),
		[&path, &otherContext](const std::pair<std::string, std::shared_ptr<Base_object>>& item)
		{
			if (item.second->get_object_name().find("ans") == std::string::npos)
			{
				std::cout << "Object : " << item.first;
				item.second->store(path, otherContext);
				std::cout << " done." << std::endl;
			}
			else
			{
				std::cout << "Does not support saving directly" << std::endl;
			}
		});
}