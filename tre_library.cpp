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

		for (auto& filename : files)
		{
			auto reader = std::make_shared<Tre_reader>(filename);
			m_readers.push_back(reader);

			auto resources_num = reader->get_resource_count();
			for (uint32_t idx = 0; idx < resources_num; ++idx)
			{
				auto res_name = reader->get_resource_name(idx);
				m_reader_lookup.insert(std::make_pair(res_name, reader));
			}
			if (callback_ptr != nullptr)
				callback_ptr->file_read();
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
