#pragma once
#include "IFF_file.h"
#include "objects/animated_object.h"

class latt_parser : public IFF_visitor
{
public:
	latt_parser()
	{

	}

	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;

	virtual bool is_object_parsed() const override;

	std::shared_ptr<Base_object> get_parsed_object() const { return std::dynamic_pointer_cast<Base_object>(m_object); }

private:
	bool p_PXATFORM_found = false;
	std::shared_ptr<Animated_file_list> m_object;
};

