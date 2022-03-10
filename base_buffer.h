#pragma once

class base_buffer
{
public:
  base_buffer();
  base_buffer(const uint8_t* data_ptr, size_t data_size);

  // size and position functions
  size_t get_size() const { return m_actual_data_size; }
  size_t get_position() const { return m_position; }
  size_t set_position(size_t new_position);
  bool end_of_buffer() const { return m_position >= m_actual_data_size; }

  // readers 
  uint8_t read_uint8() { return _read<uint8_t>(); }
  uint16_t read_uint16() { return _read<uint16_t>(); }
  uint32_t read_uint32() { return _read<uint32_t>(); }
  uint64_t read_uint64() { return _read<uint64_t>(); }
  float read_float() { return _read<float>(); }
  double read_double() { return _read<double>(); }
  std::string read_string();
  std::string read_string(size_t max_size);
  std::string read_stringz();
  std::wstring read_wstring();

  void read_buffer(std::vector<uint8_t>& buffer, size_t bytes_to_read);
  void read_buffer(uint8_t *data_ptr, size_t bytes_to_read);

  // writers
  void write_uint8(uint8_t value) { _write<uint8_t>(value); }
  void write_uint16(uint16_t value) { _write<uint16_t>(value); }
  void write_uint32(uint32_t value) { _write<uint32_t>(value); }
  void write_uint64(uint64_t value) { _write<uint64_t>(value); }
  void write_float(float value) { _write<float>(value); }
  void write_double(float value) { _write<double>(value); }
  void write_string(const std::string& str);
  void write_wstring(const std::wstring& wstr);

  // buffer
  void write_buffer(const uint8_t* buffer, size_t size);
  void write_buffer(const std::vector<uint8_t>& buffer);
  void write_buffer(const base_buffer& buffer);

  // raw data (to convert as asio buffers etc)
  //const std::vector<uint8_t>& raw() const { return m_data; }
  std::vector<uint8_t>& raw() { return m_data; }

private:
  template<typename _PODType> _PODType _read();
  template<typename _PODType> void _write(_PODType value);
  void _reallocate(size_t new_size = 0); // zero indicate that we increase buffer size to one allocation unit no more
  void _check_size()
  {
    if (m_position > m_actual_data_size)
      m_actual_data_size = m_position;
  }

  std::vector<uint8_t> m_data;
  size_t m_position;
  size_t m_actual_data_size;
};

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
template <typename _PODType>
inline _PODType base_buffer::_read()
{
  if ((m_position + sizeof(_PODType)) > m_actual_data_size)
    throw std::range_error("result position is out of valid range");

  auto pod_data = reinterpret_cast<_PODType *>(m_data.data() + m_position);
  m_position = m_position + sizeof(_PODType);
  _check_size();
  return *pod_data;
}

template <typename _PODType>
inline void base_buffer::_write(_PODType value)
{
  if ((m_position + sizeof(_PODType)) >= m_data.size())
    _reallocate();

  auto pod_data = reinterpret_cast<_PODType *>(m_data.data() + m_position);
  *pod_data = value;
  m_position = m_position + sizeof(_PODType);
  _check_size();
}