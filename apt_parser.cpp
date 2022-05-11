#include "stdafx.h"
#include "apt_parser.h"


void apt_parser::section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "APT FORM")
	{
		m_object = std::make_shared<LODFileList>();
	}
}

void apt_parser::parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size)
{
	if (name == "0000" || name == "0000NAME")
	{
		base_buffer buffer(data_ptr, data_size);
		std::string LODString = buffer.read_stringz();
		m_object->AddLODFile(LODString);
	}
}

bool apt_parser::is_object_parsed() const
{
	return true;
}