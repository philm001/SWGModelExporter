#pragma once
#include "objects/base_object.h"
#include "objects/animated_object.h"
#include "IFF_file.h"
#include "parsers/parser_selector.h"
#include "tre_library.h"
#include "parsers/mgn_parser.h"
#include <fbxsdk.h>
#include "UncompressQuaternion.h"
#include "objects/animated_object.h"

#define COORDINATE_SYSTEM FbxAxisSystem::Max


enum class ParseSelector
{
	CAT_PARSER,
	LMG_PARSER,
	MGN_PARSER,
	SKT_PARSER,
	SHT_PARSER,
	ANIM_PARSER,
	LATT_PARSER
};

class SWGMainObject : public Base_object
{
private:
	std::shared_ptr<IFF_visitor> m_selected_parser = std::make_shared<Parser_selector>();;
	ParseSelector p_CurrentParser;
	IFF_file p_Buffer;
	std::string p_ObjectName;
	std::set<std::string> p_emptySet;
	Context p_Context;
	std::queue<std::string> p_queueArray;
	std::string p_fullName;
	std::shared_ptr<Tre_navigator::Tre_library> p_Library;
	std::vector<std::vector<Animated_mesh>> p_CompleteModels; // level of detail goes here
	std::vector<std::vector<Skeleton::Bone>> m_bones;



	std::vector<Animation> p_CompleteAnimations;

	std::vector<mgn_parser> p_CompleteParsers;
	std::vector<std::string> p_CompleteNames;

	Animated_mesh SourceMesh;

	struct EulerAngles
	{
		double roll;
		double pitch;
		double yaw;
	};

	EulerAngles ConvertCombineCompressQuat(Geometry::Vector4 DecompressedQuaterion, Skeleton::Bone BoneReference, bool isStatic = false);

	// ------ Section that works on the bones -----
	std::vector<Skeleton::Bone> generateSkeletonInScene(FbxScene* scene_ptr, FbxNode* parent_ptr, std::vector<Animated_mesh>& mesh);
	uint32_t get_bones_count(int LODLevel) { return m_bones.at(LODLevel).size(); }
	Skeleton::Bone& getBone(uint32_t index, int LODLevel) { return m_bones.at(LODLevel).at(index); }
public:
	void SetLibrary(std::shared_ptr<Tre_navigator::Tre_library> library)
	{
		p_Library = library;
	}

	void beginParsingProcess(std::queue<std::string> queueArray, std::string output_pathname, bool overwrite);

	void StoreObject(const std::string& path)
	{
		this->store(path, p_Context);
	}

	virtual bool is_object_correct() const override { return true; } // For now
	
	virtual std::set<std::string> get_referenced_objects() const override { return p_emptySet; }
	virtual void set_object_name(const std::string& obj_name) override { p_ObjectName = obj_name; }// object will have no name??
	virtual std::string get_object_name() const override { return p_ObjectName; }
	void resolveDependecies();
	void storeObject(const std::string& path);

	void storeMGN(const std::string& path, std::vector<Animated_mesh>& mesh);

	// Do not use these functions, they are only here to satisfy the virtual functions
	virtual void resolve_dependencies(const Context& context) override;
	virtual void store(const std::string& path, const Context& context) override;

	
};

