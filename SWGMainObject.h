#pragma once
#include "objects/base_object.h"
#include "objects/animated_object.h"
#include "IFF_file.h"

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
	std::shared_ptr<IFF_visitor> m_selected_parser;
	ParseSelector p_CurrentParser;
	IFF_file* p_Buffer;
	std::string p_ObjectName;
	std::set<std::string> p_emptySet;

public:
	void SetIFFBuffer(IFF_file* SWGBuffer)
	{
		p_Buffer = SWGBuffer;
	}

	void beginParsingProcess();

	virtual bool is_object_correct() const override { return true; } // For now
	virtual void store(const std::string& path, const Context& context) override { return; }
	virtual std::set<std::string> get_referenced_objects() const override { return p_emptySet; }
	virtual void resolve_dependencies(const Context& context) override { return; }
	virtual void set_object_name(const std::string& obj_name) override { p_ObjectName = obj_name; }// object will have no name??
	virtual std::string get_object_name() const override { return p_ObjectName; }
};

