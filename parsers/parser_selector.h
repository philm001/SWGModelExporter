#pragma once

#include "IFF_file.h"
#include "objects/base_object.h"

class Parser_selector : public IFF_visitor
{
public:
	Parser_selector();

	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;
	virtual void section_end(uint32_t depth) override;

	virtual bool is_object_parsed() const override
	{
		bool returnValue = false;

		if (m_selected_parser != nullptr)
		{
			bool tempValue = m_selected_parser->is_object_parsed();
			returnValue = tempValue;
		}

		return returnValue;

	}

	virtual std::shared_ptr<Base_object> get_parsed_object() const override
	{
		return (m_selected_parser != nullptr) ? m_selected_parser->get_parsed_object() : nullptr;
	}
private:
	std::shared_ptr<IFF_visitor> m_selected_parser;
};

