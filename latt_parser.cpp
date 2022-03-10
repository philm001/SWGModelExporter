#include "stdafx.h"
#include "latt_parser.h"

void latt_parser::section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "LATTFORM")
	{
		m_object = std::make_shared<Animated_file_list>();
	}
	else if (name == "PXATFORM")
	{
		p_PXATFORM_found = true;
	}
}

void latt_parser::parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size)
{
	static int animationLegendCounter = 0;

	if (name == "ANIMINFO")
	{
		animationLegendCounter++;
		if (p_PXATFORM_found)
		{
			p_PXATFORM_found = false;// Reset this back to false in the event the PXAT was not found
		}
	}
	else if (name == "0000INFO")
	{
		if (p_PXATFORM_found)
		{
			base_buffer buffer(data_ptr, data_size);
			std::string animationString = buffer.read_stringz();
			m_object->add_animation(animationString);
			p_PXATFORM_found = false;
		}
	}
}

bool latt_parser::is_object_parsed() const
{
	return true;
}