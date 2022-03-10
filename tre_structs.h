#pragma once

namespace Tre_navigator
{
#pragma pack(push, 1)
	struct Header
	{
		char file_type[4];
		char file_version[4];
		uint32_t resource_count;
		uint32_t info_offset;
		uint32_t info_compression;
		uint32_t info_compressed_size;
		uint32_t name_compression;
		uint32_t name_compressed_size;
		uint32_t name_uncompressed_size;
	};

	struct Resource_info
	{
		uint32_t checksum;
		uint32_t data_size;
		uint32_t data_offset;
		uint32_t data_compression;
		uint32_t data_compressed_size;
		uint32_t name_offset;
	};
#pragma pack(pop)
}