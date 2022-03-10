#include "stdafx.h"
#include "base_buffer.h"

namespace
{
	const size_t allocation_unit = 64;    // minimum allocation unit, used to minimize reallocation of internal buffer
}

base_buffer::base_buffer() : m_position(0), m_actual_data_size(0)
{
	m_data.resize(allocation_unit);
}

base_buffer::base_buffer(const uint8_t* data_ptr, size_t data_size) : m_position(0), m_actual_data_size(data_size)
{
	assert(data_ptr != nullptr);
	if (data_size == 0)
	{
		return;
		int i = 0;
		i++;
	}
	assert(data_size > 0);

	auto units = data_size / allocation_unit;
	if (data_size % allocation_unit)
		units += 1;

	m_data.resize(units * allocation_unit);
	std::copy(data_ptr, data_ptr + data_size, m_data.begin());
}

size_t base_buffer::set_position(size_t new_position)
{
	assert(m_actual_data_size <= m_data.size());
	if (new_position <= m_actual_data_size)
		m_position = new_position;
	return m_position;
}

std::string base_buffer::read_string()
{
	uint16_t string_size = read_uint16();
	return std::move(read_string(string_size));
}

std::string base_buffer::read_string(size_t size)
{
	if (m_position + size > m_actual_data_size)
		throw std::range_error("result outside of buffer size");

	auto it_begin = m_data.begin() + m_position;
	auto it_end = it_begin + size;
	std::string result(it_begin, it_end);
	m_position += size;
	return std::move(result);
}

std::string base_buffer::read_stringz()
{
	auto this_position = m_position;
	uint8_t this_char = m_data[this_position];
	while (this_char != 0 && this_position <= m_actual_data_size)
	{
		this_position += 1;
		this_char = m_data[this_position];
	}

	auto st_beg = m_data.begin() + m_position;
	auto st_end = m_data.begin() + this_position;

	// set position to next to position of trailing zero
	set_position(this_position + 1);
	return std::string(st_beg, st_end);
}

std::wstring base_buffer::read_wstring()
{
	uint16_t symbol_count = read_uint16();
	uint16_t byte_size = symbol_count * sizeof(wchar_t);
	if ((m_position + byte_size) > m_actual_data_size)
		throw std::out_of_range("result outside of buffer size");

	auto it_begin = reinterpret_cast<wchar_t*>(m_data.data() + m_position);
	auto it_end = it_begin + symbol_count;
	m_position += byte_size;
	return std::wstring(it_begin, it_end);
}

void base_buffer::read_buffer(std::vector<uint8_t>& buffer, size_t bytes_to_read)
{
	if ((m_position + bytes_to_read) > m_actual_data_size)
		throw std::out_of_range("result outside of buffer size");

	if (bytes_to_read > buffer.size())
		throw std::invalid_argument("buffer have no sufficient space to receive data");

	auto it_begin = m_data.begin() + m_position;

	std::copy(it_begin, it_begin + bytes_to_read, buffer.begin());
	m_position += bytes_to_read;
}

void base_buffer::read_buffer(uint8_t* buffer_ptr, size_t bytes_to_read)
{
	if ((m_position + bytes_to_read) > m_actual_data_size)
		throw std::out_of_range("result outside of buffer size");

	auto it_begin = m_data.begin() + m_position;
	std::copy(it_begin, it_begin + bytes_to_read, buffer_ptr);
	m_position += bytes_to_read;
}

void base_buffer::write_string(const std::string& str)
{
	auto new_size = m_position + str.size() + sizeof(uint16_t);
	if (new_size > m_data.size())
		_reallocate();

	write_uint16(static_cast<uint16_t>(str.size()));
	auto data_ptr = m_data.data() + m_position;
	for (auto chr : str)
	{
		*data_ptr++ = chr;
	}
	m_position += str.size();
	_check_size();
}

void base_buffer::write_wstring(const std::wstring& wstr)
{
	auto new_size = m_position + wstr.size() * sizeof(wchar_t) + sizeof(uint16_t);
	if (new_size > m_data.size())
		_reallocate();

	write_uint16(static_cast<uint16_t>(wstr.size()));
	auto data_ptr = reinterpret_cast<wchar_t*>(m_data.size() + m_position);
	for (auto chr : wstr)
	{
		*data_ptr++ = chr;
	}
	m_position += wstr.size() * sizeof(wchar_t);
	_check_size();
}

void base_buffer::write_buffer(const uint8_t* buffer, size_t size)
{
	auto new_size = m_position + size;
	if (new_size > m_data.size())
		_reallocate(new_size);

	std::copy(buffer, buffer + size, m_data.begin() + m_position);
	m_position += size;
	_check_size();
}

void base_buffer::write_buffer(const std::vector<uint8_t>& buffer)
{
	auto new_size = m_position + buffer.size();
	if (new_size > m_data.size())
		_reallocate(new_size);

	std::copy(buffer.begin(), buffer.end(), m_data.begin() + m_position);
	m_position += buffer.size();
	_check_size();
}

void base_buffer::write_buffer(const base_buffer& buffer)
{
	auto new_size = m_position + buffer.get_size();
	if (new_size > m_data.size())
		_reallocate(new_size);

	std::copy(buffer.m_data.cbegin(), buffer.m_data.cend(), m_data.begin() + m_position);
	m_position += buffer.get_size();
	_check_size();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void base_buffer::_reallocate(size_t new_size)
{
	if (new_size == 0)
		new_size = m_data.size() + allocation_unit;
	else
	{
		// pad new_size to allocation_unit border
		auto units = new_size / allocation_unit;
		if (new_size % allocation_unit)
			units++;

		new_size = units * allocation_unit;
	}
	m_data.reserve(new_size);
}