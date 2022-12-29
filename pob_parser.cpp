#include "stdafx.h"
#include "pob_parser.h"


void pob_parser::section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "PRTOFORM")
	{
		m_object = std::make_shared<LODFileList>();
	}
}

void pob_parser::parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size)
{
	if (name == "0005DATA")
	{
		base_buffer buffer(data_ptr, data_size);
		
		// Throw away 5 bytes
		buffer.read_uint32();
		buffer.read_uint8();

		// Throw away 1 string
		std::string temp = buffer.read_stringz();

		// Now read the data that we need
		std::string LODString = buffer.read_stringz();
		m_object->AddLODFile(LODString);
	}
}

bool pob_parser::is_object_parsed() const
{
	return true;
}