#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class lmg_parser : public IFF_visitor
{
public:
	lmg_parser() : m_names_count(0), m_names_received(0)
	{ }
	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t date_size) override;

	virtual bool is_object_parsed() const override;
	std::shared_ptr<Base_object> get_parsed_object() const { return std::dynamic_pointer_cast<Base_object>(m_lmg_object); }

private:
	enum sections_received
	{
		mlod = 0,
		info = 1,
		name = 2
	};

	std::bitset<3> m_sections_received;

	uint16_t m_names_count;
	uint16_t m_names_received;

	std::shared_ptr<Animated_lod_list> m_lmg_object;
};
