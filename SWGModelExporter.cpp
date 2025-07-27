// SWGModelExporter.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "tre_reader.h"
#include "tre_library.h"
#include "IFF_file.h"
#include "parsers/parser_selector.h"
#include "objects/animated_object.h"
#include "SWGMainObject.h"

using namespace std;
namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace Tre_navigator;

class File_read_callback : public Tre_library_reader_callback
{
public:
	File_read_callback()
	{ }
	virtual void number_of_files(size_t file_num) override
	{
		if (m_display == nullptr)
			m_display = make_shared<boost::progress_display>(static_cast<unsigned long>(file_num));
	}
	virtual void file_read() override
	{
		if (m_display)
			m_display->operator++();
	}

private:
	std::shared_ptr<boost::progress_display> m_display;
};

int _tmain(int argc, _TCHAR* argv[])
{
	std::string swg_path;
	std::string object_name;
	std::string output_pathname;
	bool overwriteResult = true;

#ifdef _DEBUG
	// Debug mode: Use hardcoded values for development
	swg_path = "C:\\SWG";
	output_pathname = "C:\\extraction";
	
	// Uncomment one of these lines for testing different objects:
	/* These animations have known bugs and will not export correctly */
	//object_name = "dress_s06_f.sat";// bug here
	//object_name = "bikini_s01_f.sat";// This one still has issue
	
	/* These animations export. However, the animations themselves have a couple of minor bugs (all animations do) */
	//object_name = "bageraset.sat";
	object_name = "acklay.sat";
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
	//object_name = "batch:pob";
	
	std::cout << "Debug Mode - Using hardcoded values:" << std::endl;
	std::cout << "SWG Path: " << swg_path << std::endl;
	std::cout << "Output Path: " << output_pathname << std::endl;
	std::cout << "Object: " << object_name << std::endl;
	std::cout << std::endl;
#else
	// Release mode: Use command line arguments
	po::options_description flags("Program options");
	flags.add_options()
		("help", "get this help message")
		("swg-path", po::value<string>(&swg_path)->required(), "Path to Star Wars Galaxies. Ex: C:\\swg\\SWGEmu")
		("object", po::value<string>(&object_name)->required(), "Name of object to extract. use batch:<ext> to extract all files of given ext. Ex: asteroid_acid_large_s01.apt")
		("output-path", po::value<string>(&output_pathname)->required(), "Path to output location Ex: C:\\extraction\\test")
		("overwrite-result", po::value<bool>(&overwriteResult)->required(), "Set to 1 to overwrite current file. Default is to skip file if it exists Ex: 1");

	try
	{
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, flags), vm);
		po::notify(vm);
	}
	catch (...)
	{
		std::cout << flags << std::endl;
		return -1;
	}
#endif

	CoInitialize(NULL);

	fs::path output_path(output_pathname);

	File_read_callback read_callback;
	std::cout << "Loading TRE library..." << std::endl;

	std::shared_ptr<Tre_library> library = make_shared<Tre_library>(swg_path, &read_callback);

	string full_name;
	std::cout << "Looking for object" << endl;

	queue<queue<std::string>> objects_to_process;

	// check if we in batch mode by given object name special look
	boost::char_separator<char> separators(":");
	boost::tokenizer<boost::char_separator<char>> object_name_tokens(object_name, separators);
	vector<string> tokens(object_name_tokens.begin(), object_name_tokens.end());
	if (tokens.size() > 1)
	{
		if (tokens[0] != "batch")
		{
			cout << "Incorrect format for batch mode" << endl;
			return 0;
		}

		auto filetype = tokens[1];

		vector<string> selected_objects;
		if (library->select_objects_by_ext(filetype, selected_objects))
		{
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
			cout << "no object selected for batch - extension is wrong?";
			return 0;
		}
	}
	else
	{
		// normalize filename
		replace_if(object_name.begin(), object_name.end(), [](const char& value) { return value == '\\'; }, '/');
		if (library->is_object_present(object_name))
		{
			queue<std::string> singleVector;
			singleVector.push(object_name);
			objects_to_process.push(singleVector);
		}
		else if (library->get_object_name(object_name, full_name))
		{
			queue<std::string> singleVector;
			singleVector.push(full_name);
			objects_to_process.push(singleVector);
		}
		else
			std::cout << "Object with name \"" << object_name << "\" has not been found" << std::endl;
	}


	while (objects_to_process.empty() == false)
	{
		queue<std::string> frontValue = objects_to_process.front();
		objects_to_process.pop();

		SWGMainObject SWGObject;
		SWGObject.SetLibrary(library);

		SWGObject.beginParsingProcess(frontValue, output_pathname, overwriteResult);
		SWGObject.resolveDependecies();
		SWGObject.storeObject(output_pathname);
	}

	CoUninitialize();
	return 0;
}
