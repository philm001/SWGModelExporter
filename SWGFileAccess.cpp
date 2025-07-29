#include "stdafx.h"
#include "SWGMainObject.h"

void SWGMainObject::storeObject(const std::string& path)
{
	std::vector<std::vector<Animated_mesh>>& ModelCopy = p_CompleteModels;
	Context& referenceContext = p_Context;

	std::for_each(p_Context.object_list.begin(), p_Context.object_list.end(),
		[&referenceContext, &ModelCopy, &path, this](const std::pair<std::string, std::shared_ptr<Base_object>>& item)
		{
			std::cout << "Object : " << item.first;
			if (item.first.find("mgn") != std::string::npos)
			{
				for (auto& listIterator : ModelCopy)
				{
					//std::vector<Animated_mesh> listIterator = ModelCopy[0];
					for (Animated_mesh meshIterator : listIterator)
					{
						if (meshIterator.get_object_name() == item.first)
						{
							std::vector<Animated_mesh> tempVector;// hacky way to get this to work on single meshes. There was a time where this program could combine multiple meshes into a signle mesh. This has since been removed.
							tempVector.push_back(meshIterator);
							storeMGN(path, tempVector);
						}
					}
				}

			}
			else
			{
				item.second->store(path, referenceContext);
			}

			std::cout << " done." << std::endl;
		});

}

void SWGMainObject::storeObjectParallel(const std::string& path)
{
	// Keep disk access sequential while parallelizing internal processing
	std::cout << "Exporting assets (sequential disk access with parallel processing)..." << std::endl;

	std::vector<std::vector<Animated_mesh>>& ModelCopy = p_CompleteModels;
	Context& referenceContext = p_Context;

	std::for_each(p_Context.object_list.begin(), p_Context.object_list.end(),
		[&referenceContext, &ModelCopy, &path, this](const std::pair<std::string, std::shared_ptr<Base_object>>& item)
		{
			std::cout << "Object : " << item.first;
			if (item.first.find("mgn") != std::string::npos)
			{
				// Process MGN with parallel internal operations, but sequential file writing
				exportMGNWithParallelProcessing(path, item);
			}
			else
			{
				// Sequential export for other objects
				item.second->store(path, referenceContext);
			}
			std::cout << " done." << std::endl;
		});
}

void SWGMainObject::exportMGNWithParallelProcessing(const std::string& path, const std::pair<std::string, std::shared_ptr<Base_object>>& item)
{
	for (auto& listIterator : p_CompleteModels)
	{
		for (Animated_mesh meshIterator : listIterator)
		{
			if (meshIterator.get_object_name() == item.first)
			{
				std::vector<Animated_mesh> tempVector;
				tempVector.push_back(meshIterator);

				// Build FBX scene with parallel internal processing, then write sequentially
				storeMGN(path, tempVector);
				break;
			}
		}
	}
}