// SWGModelExporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tre_reader.h"
#include "tre_library.h"
#include "IFF_file.h"
#include "parsers/parser_selector.h"
#include "objects/animated_object.h"
#include "SWGMainObject.h"
#include "DebugConfig.h"
#include <cwctype>

/*
Dependency Files — Resolved Automatically, No Need to Batch
These are pulled in by the entry points above via get_referenced_objects() cascading through the queue. You only need to batch these directly if you want to extract a specific type in isolation.
Command	SWG File Type	Resolved By
batch:lmg	LOD Mesh Group	sat / cat
batch:sat	Skeletal Mesh Geometry	lmg → sat
batch:msh	Static Mesh	apt
batch:skt	Skeleton	sat / cat
batch:sht	Shader Template	mgn / msh
batch:dds	DirectX Texture	sht
batch:ans	Compressed Animation	sat
batch:kat	Keyframe Animation (uncompressed)	sat
batch:att	Animation Table (.latt files)	sat — note: select_objects_by_ext matches last 3 chars, so att matches .latt
---
TL;DR — To get everything in one go, you need three runs:
1.	batch:apt — static models
2.	batch:sat — animated models
3.	batch:pob — buildings (if needed)
*/

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace Tre_navigator;

#ifdef _WIN32
static std::string utf8_from_wide(const std::wstring& value)
{
	if (value.empty())
		return {};

	const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
	if (size <= 0)
		throw std::runtime_error("Failed to convert command line to UTF-8");

	std::string utf8(size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), utf8.data(), size, nullptr, nullptr);
	return utf8;
}

static std::wstring parse_windows_argument(const std::wstring& command_line, size_t& index)
{
	std::wstring argument;
	bool in_quotes = false;

	while (index < command_line.size())
	{
		const wchar_t ch = command_line[index];

		if (!in_quotes && std::iswspace(ch))
			break;

		if (ch == L'"')
		{
			in_quotes = !in_quotes;
			++index;
			continue;
		}

		if (in_quotes && ch == L'\\')
		{
			size_t slash_start = index;
			while (index < command_line.size() && command_line[index] == L'\\')
				++index;

			const size_t slash_count = index - slash_start;
			if (index < command_line.size() && command_line[index] == L'"')
			{
				const bool closes_argument = (index + 1 == command_line.size()) || std::iswspace(command_line[index + 1]);
				if (closes_argument)
				{
					argument.append(slash_count, L'\\');
					in_quotes = false;
					++index;
					continue;
				}
			}

			argument.append(slash_count, L'\\');
			continue;
		}

		argument.push_back(ch);
		++index;
	}

	return argument;
}

static std::vector<std::string> get_windows_command_line_args_utf8()
{
	const std::wstring command_line = GetCommandLineW();
	std::vector<std::string> args;
	size_t index = 0;

	const auto skip_whitespace = [&]()
	{
		while (index < command_line.size() && std::iswspace(command_line[index]))
			++index;
	};

	skip_whitespace();
	if (index < command_line.size())
	{
		parse_windows_argument(command_line, index);
	}

	while (index < command_line.size())
	{
		skip_whitespace();
		if (index >= command_line.size())
			break;

		args.push_back(utf8_from_wide(parse_windows_argument(command_line, index)));
	}

	return args;
}
#endif

class Progress_display
{
public:
	explicit Progress_display(unsigned long total) : m_total(total) {}

	void increment()
	{
		++m_current;
		std::cout << "\rLoaded " << m_current << "/" << m_total << " TRE files" << std::flush;
		if (m_current >= m_total)
			std::cout << std::endl;
	}

private:
	unsigned long m_total = 0;
	unsigned long m_current = 0;
};

class File_read_callback : public Tre_library_reader_callback
{
public:
	File_read_callback()
	{ }
	virtual void number_of_files(size_t file_num) override
	{
		if (m_display == nullptr)
			m_display = make_shared<Progress_display>(static_cast<unsigned long>(file_num));
	}
	virtual void file_read() override
	{
		if (m_display)
			m_display->increment();
	}

private:
	std::shared_ptr<Progress_display> m_display;
};

int main(int argc, char* argv[])
{
	// Initialize logging system - this will delete any existing log file and create a new one
	INIT_LOGGER("SWGModelExporter.log");

	std::string swg_path;
	std::string object_name;
	std::string output_pathname;
	bool overwriteResult = false;
	bool verbose = false;
	bool no_lod = false;
	bool no_lod_animation = true;

#ifdef SWGME_DEBUG_MODE
	// Debug mode: Use hardcoded values for development
	swg_path = "C:\\SWG";
	output_pathname = "C:\\extraction";
	
	// Uncomment one of these lines for testing different objects:
	/* These animations have known bugs and will not export correctly */
	//object_name = "dress_s06_f.sat";// bug here
	//object_name = "bikini_s01_f.sat";// This one still has issue
	//object_name = "ackbar.lmg";
	
	/* These animations export. However, the animations themselves have a couple of minor bugs (all animations do) */
	//object_name = "bageraset.sat";
	//object_name = "lom.sat";
	//object_name = "krayt_dragon.sat";
	//object_name = "bantha_hue.sat"; // bone rotation 4th one
	
	/* For testing static meshes, test each of these 3 */
	//object_name = "asteroid_acid_large_s01.apt"; // For apt parsing
	//object_name = "asteroid_acid_large_s01_l0.msh"; // Simple case
	//object_name = "bunker_mine_car_s01_l0.msh"; // Has multiple shaders + DOT3
	//object_name = "door_jabba_backdoor.msh"; // Has multiple shaders + DOT3 + 32 bit indicies for index
	
	/* Test for pob */
	//object_name = "thm_corl_skyskraper_s01.pob";

	/* Example for batch mode */
	object_name = "batch:sat";
	//object_name = "bageraset.sat";

	// Enable verbose mode in debug builds for maximum diagnostics
	DebugConfig::setVerboseMode(verbose);

	// Enable LOD 0-only mode in debug builds
	DebugConfig::setNoLODMode(no_lod);

	// Enable LOD animation mode in debug builds (or not, based on no_lod_animation variable)
	DebugConfig::setNoLODAnimationMode(no_lod_animation);

	LOG_INFO("Debug Mode - Using hardcoded values:");
	LOG_INFO("SWG Path: " + swg_path);
	LOG_INFO("Output Path: " + output_pathname);
	LOG_INFO("Object: " + object_name);
	LOG_INFO("Verbose Mode: ENABLED (Debug build)");
	LOG_INFO("LOD Export Mode: " + std::string(no_lod ? "LOD 0 ONLY" : "ALL LODS"));
	LOG_INFO("Animation Export Mode: " + std::string(no_lod_animation ? "LOD 0 ANIMATIONS ONLY" : "ALL LODS ANIMATED"));
	LOG_INFO("");
#else
	// Release mode: Use command line arguments
	po::options_description flags("Program options");
	flags.add_options()
		("help", "get this help message")
		("swg-path", po::value<string>(&swg_path)->required(), "Path to Star Wars Galaxies. Ex: C:\\swg\\SWGEmu")
		("object", po::value<string>(&object_name)->required(), "Name of object to extract. use batch:<ext> to extract all files of given ext. Ex: asteroid_acid_large_s01.apt")
		("output-path", po::value<string>(&output_pathname)->required(), "Path to output location Ex: C:\\extraction\\test")
		("overwrite-result", po::value<bool>(&overwriteResult)->default_value(false), "Set to 1 to overwrite current file. Default is to skip file if it exists Ex: 1")
		("verbose", po::bool_switch(&verbose)->default_value(false), "Enable verbose logging output to console and log file")
		("no-lod", po::bool_switch(&no_lod)->default_value(false), "Skip LOD levels - only export LOD 0 (highest detail mesh)")
		("no-lod-animation", po::bool_switch(&no_lod_animation)->default_value(false), "Skip animations for LOD 1+ - only LOD 0 gets animations");

	try
	{
		po::variables_map vm;
#ifdef _WIN32
		try
		{
			po::store(po::parse_command_line(argc, argv, flags), vm);
		}
		catch (const po::error&)
		{
			po::store(po::command_line_parser(get_windows_command_line_args_utf8()).options(flags).run(), vm);
		}
#else
		po::store(po::parse_command_line(argc, argv, flags), vm);
#endif
		if (vm.count("help"))
		{
			std::cout << flags << std::endl;
			return 0;
		}
		po::notify(vm);

		// Set verbose mode in DebugConfig before any processing
		DebugConfig::setVerboseMode(verbose);

		// Set LOD mode in DebugConfig before any processing
		DebugConfig::setNoLODMode(no_lod);

		// Set LOD animation mode in DebugConfig before any processing
		DebugConfig::setNoLODAnimationMode(no_lod_animation);

		LOG_INFO("Release Mode - Using command line arguments:");
		LOG_INFO("SWG Path: " + swg_path);
		LOG_INFO("Output Path: " + output_pathname);
		LOG_INFO("Object: " + object_name);
		LOG_INFO("Overwrite: " + std::to_string(overwriteResult));
		LOG_INFO("Verbose Mode: " + std::string(verbose ? "ENABLED" : "DISABLED"));
		LOG_INFO("LOD Export Mode: " + std::string(no_lod ? "LOD 0 ONLY" : "ALL LODS"));
		LOG_INFO("Animation Export Mode: " + std::string(no_lod_animation ? "LOD 0 ANIMATIONS ONLY" : "ALL LODS ANIMATED"));
		LOG_INFO("");
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("Invalid command line arguments provided: " + std::string(ex.what()));
		std::cout << flags << std::endl;
		return -1;
	}
#endif

	try
	{
#ifdef _WIN32
		CoInitialize(nullptr);
#endif

		fs::path output_path(output_pathname);

		File_read_callback read_callback;
		LOG_INFO("Loading TRE library...");

		std::shared_ptr<Tre_library> library = make_shared<Tre_library>(swg_path, &read_callback);

		string full_name;
		LOG_INFO("Looking for object: " + object_name);

		queue<queue<std::string>> objects_to_process;

		// check if we in batch mode by given object name special look
		boost::char_separator<char> separators(":");
		boost::tokenizer<boost::char_separator<char>> object_name_tokens(object_name, separators);
		vector<string> tokens(object_name_tokens.begin(), object_name_tokens.end());
		if (tokens.size() > 1)
		{
			if (tokens[0] != "batch")
			{
				LOG_ERROR("Incorrect format for batch mode");
				return 0;
			}

			auto filetype = tokens[1];
			LOG_INFO("Batch mode detected for file type: " + filetype);

			vector<string> selected_objects;
			if (library->select_objects_by_ext(filetype, selected_objects))
			{
				LOG_INFO("Found " + std::to_string(selected_objects.size()) + " objects for batch processing");
				//context.batch_mode = true;
				for (const auto& obj_name : selected_objects)
				{
					queue<std::string> singleVector;
					singleVector.push(obj_name);
					objects_to_process.push(singleVector);
				}
			}
			else
			{
				LOG_ERROR("No objects selected for batch - extension is wrong?");
				return 0;
			}
		}
		else
		{
			// normalize filename
			replace_if(object_name.begin(), object_name.end(), [](const char& value) { return value == '\\'; }, '/');
			if (library->is_object_present(object_name))
			{
				LOG_INFO("Object found directly: " + object_name);
				queue<std::string> singleVector;
				singleVector.push(object_name);
				objects_to_process.push(singleVector);
			}
			else if (library->get_object_name(object_name, full_name))
			{
				LOG_INFO("Object found with full name: " + full_name);
				queue<std::string> singleVector;
				singleVector.push(full_name);
				objects_to_process.push(singleVector);
			}
			else
			{
				LOG_ERROR("Object with name \"" + object_name + "\" has not been found");
			}
		}

		LOG_INFO("Starting processing of " + std::to_string(objects_to_process.size()) + " object(s)...");

		while (objects_to_process.empty() == false)
		{
			queue<std::string> frontValue = objects_to_process.front();
			objects_to_process.pop();

			std::string name = "";

			LOG_INFO("Processing object queue with " + std::to_string(frontValue.size()) + " items");
			{
				queue<std::string> printCopy = frontValue;
				while (!printCopy.empty())
				{
					LOG_INFO("Object queued for processing: " + printCopy.front());
					name = printCopy.front();
					printCopy.pop();
				}
			}

			SWGMainObject SWGObject;
			SWGObject.SetLibrary(library);
			
			SWGObject.beginParsingProcess(frontValue, output_pathname, overwriteResult);
			SWGObject.resolveDependecies();
			SWGObject.storeObject(output_pathname);
		}

		LOG_INFO("All objects processed successfully");
#ifdef _WIN32
		CoUninitialize();
#endif
		return 0;
	}
	catch (const std::exception& e)
	{
		LOG_ERROR("Exception occurred: " + std::string(e.what()));
#ifdef _WIN32
		CoUninitialize();
#endif
		return -1;
	}
	catch (...)
	{
		LOG_ERROR("Unknown exception occurred");
#ifdef _WIN32
		CoUninitialize();
#endif
		return -1;
	}
}
