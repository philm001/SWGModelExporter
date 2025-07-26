#include "stdafx.h"
#include "tre_library.h"
#include <filesystem>

namespace Tre_navigator
{
	namespace fs = std::filesystem;
	namespace test = std::experimental::filesystem;

	using namespace std;

	Tre_library::Tre_library(const string& path, Tre_library_reader_callback* callback_ptr)
	{
		// Validate the path first
		if (path.empty())
		{
			throw std::runtime_error("TRE library path cannot be empty");
		}

		if (!fs::exists(path))
		{
			throw std::runtime_error("TRE library path does not exist: " + path);
		}

		if (!fs::is_directory(path))
		{
			throw std::runtime_error("TRE library path is not a directory: " + path);
		}

		try
		{
			fs::directory_iterator file_enum(path);
			vector<string> files;

			for (auto& file_item : file_enum)
			{
				if (fs::is_regular_file(file_item) && file_item.path().extension() == ".tre")
					files.push_back(file_item.path().string());
			}

			sort(files.begin(), files.end());
			if (callback_ptr != nullptr)
				callback_ptr->number_of_files(files.size());

			// Track failed files for reporting
			vector<string> failed_files;
			size_t successfully_loaded = 0;

			for (auto& filename : files)
			{
				try
				{
					std::cout << "Loading TRE file: " << fs::path(filename).filename().string() << std::flush;
					
					if (filename == "C:\\SWG\\patch_18_client_00.tre")
					{
						std::cout << " (known problematic file)" << std::flush;
					}
					
					auto reader = std::make_shared<Tre_reader>(filename);
					m_readers.push_back(reader);

					auto resources_num = reader->get_resource_count();
					for (uint32_t idx = 0; idx < resources_num; ++idx)
					{
						auto res_name = reader->get_resource_name(idx);
						m_reader_lookup.insert(std::make_pair(res_name, reader));
					}
					
					std::cout << " - OK (" << resources_num << " resources)" << std::endl;
					successfully_loaded++;
					if (callback_ptr != nullptr)
						callback_ptr->file_read();
				}
				catch (const std::exception& e)
				{
					// Log the failed file but continue processing others
					std::cout << " - FAILED" << std::endl;
					std::cerr << "Error details: " << e.what() << std::endl;
					failed_files.push_back(filename);
					
					if (callback_ptr != nullptr)
						callback_ptr->file_read(); // Still report progress
				}
			}

			// Report summary
			std::cout << "TRE Library loaded: " << successfully_loaded << " files successfully";
			if (!failed_files.empty())
			{
				std::cout << ", " << failed_files.size() << " files failed to load";
			}
			std::cout << std::endl;

			// If no files were loaded successfully, that's a fatal error
			if (successfully_loaded == 0)
			{
				std::string error_msg = "No TRE files could be loaded from: " + path;
				if (!failed_files.empty())
				{
					error_msg += "\nFailed files:";
					for (const auto& failed_file : failed_files)
					{
						error_msg += "\n  - " + failed_file;
					}
				}
				throw std::runtime_error(error_msg);
			}
		}
		catch (const fs::filesystem_error& e)
		{
			throw std::runtime_error("Filesystem error while accessing TRE library path '" + path + "': " + e.what());
		}
		catch (const std::exception& e)
		{
			throw std::runtime_error("Failed to initialize TRE library from path '" + path + "': " + e.what());
		}
	}

	bool Tre_library::is_object_present(const std::string& name)
	{
		auto result = m_reader_lookup.find(name);

		return result != m_reader_lookup.end();
	}

	bool Tre_library::get_object_name(const std::string& partial_name, std::string& full_name)
	{
		regex pattern("(.*)" + partial_name);
		auto result = find_if(m_reader_lookup.begin(), m_reader_lookup.end(),
			[&pattern](const pair<string, weak_ptr<Tre_reader>>& item)
			{
				return regex_match(item.first, pattern);
			});

		auto ret = result != m_reader_lookup.end();
		if (ret)
			full_name = result->first;
		return ret;
	}

	size_t Tre_library::number_of_object_versions(const string& name)
	{
		auto object = m_reader_lookup.find(name);
		if (object != m_reader_lookup.end())
		{
			auto readers_range = m_reader_lookup.equal_range(name);
			return std::distance(readers_range.first, readers_range.second);
		}

		return size_t(-1);
	}

	vector<weak_ptr<Tre_reader>> Tre_library::get_versioned_readers(const string& name)
	{
		vector<weak_ptr<Tre_reader>> result;

		auto object = m_reader_lookup.find(name);
		if (object != m_reader_lookup.end())
		{
			auto range = m_reader_lookup.equal_range(name);
			for (auto it = range.first; it != range.second; ++it)
				result.push_back(it->second);
		}
		return result;
	}

	bool Tre_library::get_object(const string& name, vector<uint8_t>& buffer, size_t version_num, bool search)
	{
		string fullname;
		if (!is_object_present(name))
		{
			if (!search || !get_object_name(name, fullname))
				return false;
		}
		else
			fullname = name;

		auto readers = get_versioned_readers(fullname);
		if (version_num > readers.size())
			return false;

		auto reader_lock = readers[version_num].lock();
		if (reader_lock)
			return reader_lock->get_resource(fullname, buffer);

		return false;
	}

	bool Tre_library::select_objects_by_ext(const string& ext, vector<string>& result)
	{
		if (ext.length() != 3)
			return false;

		for (auto& object : m_reader_lookup)
		{
			auto object_name = object.first;
			auto obj_ext = object_name.substr(object_name.length() - 3);
			if (obj_ext == ext)
				result.push_back(object_name);
		}

		sort(result.begin(), result.end());
		result.erase(unique(result.begin(), result.end()), result.end());
		return result.empty() == false;
	}
}
