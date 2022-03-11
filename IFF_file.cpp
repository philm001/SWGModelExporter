#include "stdafx.h"
#include "IFF_file.h"


IFF_file::IFF_file(uint8_t* data_ptr, size_t data_size) : m_buffer(data_ptr, data_size)
{ }

IFF_file::IFF_file(const std::vector<uint8_t>& data) : m_buffer(data.data(), data.size())
{ }

IFF_file::~IFF_file()
{ }

void IFF_file::combinedObjectProcess(std::shared_ptr<IFF_visitor> visitor)
{
	// go to begin of stream
	m_buffer.set_position(0);
	std::stack<size_t> positions;
	uint32_t depth = 0;

	auto buffer_size = m_buffer.get_size();
	while (!m_buffer.end_of_buffer())
	{
		if (!positions.empty() && m_buffer.get_position() == positions.top())
		{
			positions.pop();
			depth--;
			if (visitor)
				visitor->section_end(depth);
		}
		else
		{
			auto name = _get_iff_name();

			uint32_t size = IFF_utility::swap_bytes(m_buffer.read_uint32());
			if (size >= 4 && _is_form(name))
			{
				positions.push(m_buffer.get_position() + size);
				if (visitor)
					visitor->section_begin(name, m_buffer.raw().data() + m_buffer.get_position(), size, depth);
				depth++;
			}
			else
			{
				auto next_pos = m_buffer.get_position() + size;
				if (visitor)
					visitor->parse_data(name, m_buffer.raw().data() + m_buffer.get_position(), size);
				m_buffer.set_position(next_pos);
			}
		}
	}
}

void IFF_file::full_process(std::shared_ptr<IFF_visitor> visitor)
{
	// go to begin of stream
	m_buffer.set_position(0);
	std::stack<size_t> positions;
	uint32_t depth = 0;

	auto buffer_size = m_buffer.get_size();
	while (!m_buffer.end_of_buffer())
	{
		if (!positions.empty() && m_buffer.get_position() == positions.top())
		{
			positions.pop();
			depth--;
			if (visitor)
				visitor->section_end(depth);
		}
		else
		{
			auto name = _get_iff_name();

			uint32_t size = IFF_utility::swap_bytes(m_buffer.read_uint32());
			if (size >= 4 && _is_form(name))
			{
				positions.push(m_buffer.get_position() + size);
				if (visitor)
					visitor->section_begin(name, m_buffer.raw().data() + m_buffer.get_position(), size, depth);
				depth++;
			}
			else
			{
				auto next_pos = m_buffer.get_position() + size;
				if (visitor)
					visitor->parse_data(name, m_buffer.raw().data() + m_buffer.get_position(), size);
				m_buffer.set_position(next_pos);
			}
		}
	}
}

std::string IFF_file::_get_iff_name()
{
	std::locale loc;
	uint64_t name_buf = m_buffer.read_uint64();
	auto position = m_buffer.get_position();
	const char* name_buf_ptr = reinterpret_cast<const char*>(&name_buf);

	auto length = std::count_if(name_buf_ptr, name_buf_ptr + 8,
		[&loc](const char ch) { return std::isalnum(ch, loc) || std::isspace(ch, loc); });

	if (length < 4)
	{
		length = 0;
		m_buffer.set_position(position - 8);
	}
	else if (length < 8)
	{
		length = 4;
		m_buffer.set_position(position - 4);
	}
	else
		length = 8;
	return std::string(name_buf_ptr, length);
}

bool IFF_file::_is_form(const std::string& name)
{
	return (name.substr(0, 4) == "FORM" || name.substr(4, 4) == "FORM");
}

uint16_t IFF_utility::swap_bytes(uint16_t value)
{
	uint16_t new_value = (value >> 8) | (value << 8);
	return new_value;
}

uint32_t IFF_utility::swap_bytes(uint32_t value)
{
	uint32_t new_value = (value >> 24) |
		((value & 0x00FF0000) >> 8) |
		((value & 0x0000FF00) << 8) |
		(value << 24);
	return new_value;
}

uint64_t IFF_utility::swap_bytes(uint64_t value)
{
	uint64_t new_value =
		(value >> 56) |
		((value & 0x00FF000000000000) >> 40) |
		((value & 0x0000FF0000000000) >> 24) |
		((value & 0x000000FF00000000) >> 8) |
		((value & 0x00000000FF000000) << 8) |
		((value & 0x0000000000FF0000) << 24) |
		((value & 0x000000000000FF00) << 40) |
		(value << 56);
	return new_value;
}
