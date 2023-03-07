#include "stdafx.h"
#include "UncompressQuaternion.h"

struct FormatPrecisionInfo
{
	//-- Specified directly.
	uint8_t  formatId;
	uint8_t  baseIndexMask;
	int    baseCount;
	float  baseSeparation;

	//-- Calculated.
	float  compressFactorElevenBit;
	float  expandFactorElevenBit;

	float  compressFactorTenBit;
	float  expandFactorTenBit;
};

FormatPrecisionInfo  s_formatPrecisionInfo[] =
{
	MAKE_PRECISION_INFO(BINARY2(1111, 1110), BINARY2(0000, 0000), 0),
	MAKE_PRECISION_INFO(BINARY2(1111, 1100), BINARY2(0000, 0001), 1),
	MAKE_PRECISION_INFO(BINARY2(1111, 1000), BINARY2(0000, 0011), 2),
	MAKE_PRECISION_INFO(BINARY2(1111, 0000), BINARY2(0000, 0111), 3),
	MAKE_PRECISION_INFO(BINARY2(1110, 0000), BINARY2(0000, 1111), 4),
	MAKE_PRECISION_INFO(BINARY2(1100, 0000), BINARY2(0001, 1111), 5),
	MAKE_PRECISION_INFO(BINARY2(1000, 0000), BINARY2(0011, 1111), 6)
};

// 11-bit compressed format
const uint32_t cs_valueMaskElevenBit = BINARY3(0011, 1111, 1111);
const uint32_t cs_signBitElevenBit = BINARY3(0100, 0000, 0000);

// 10-bit compressed format
const uint32_t cs_valueMaskTenBit = BINARY3(0001, 1111, 1111);
const uint32_t cs_signBitTenBit = BINARY3(0010, 0000, 0000);

const int    cs_minBaseShiftCount = 0;
const int    cs_maxBaseShiftCount = static_cast<int>(sizeof(s_formatPrecisionInfo) / sizeof(s_formatPrecisionInfo[0])) - 1;



float QuatExpand::FormatData::expandTenBit(uint32_t compressedValue) const
{
	if ((compressedValue & cs_signBitTenBit) != 0)
		return m_baseValue - (static_cast<float>(compressedValue & cs_valueMaskTenBit) * s_formatPrecisionInfo[m_formatPrecisionIndex].expandFactorTenBit);
	else
		return m_baseValue + (static_cast<float>(compressedValue & cs_valueMaskTenBit) * s_formatPrecisionInfo[m_formatPrecisionIndex].expandFactorTenBit);
}

float QuatExpand::FormatData::expandElevenBit(uint32_t compressedValue) const
{
	if ((compressedValue & cs_signBitElevenBit) != 0)
		return m_baseValue - (static_cast<float>(compressedValue & cs_valueMaskElevenBit) * s_formatPrecisionInfo[m_formatPrecisionIndex].expandFactorElevenBit);
	else
		return m_baseValue + (static_cast<float>(compressedValue & cs_valueMaskElevenBit) * s_formatPrecisionInfo[m_formatPrecisionIndex].expandFactorElevenBit);
}

void QuatExpand::UncompressQuaternion::install()
{
	for (int baseShiftCount = 0; baseShiftCount <= cs_maxBaseShiftCount; ++baseShiftCount)
	{
		float const baseSeparation = s_formatPrecisionInfo[baseShiftCount].baseSeparation;
		float const halfRange = 0.5f * calculateRange(baseShiftCount);

		// compression factor is : uncompressedUnits * (total compressedUnits/ total uncompressedUnits) = compressedUnits
		s_formatPrecisionInfo[baseShiftCount].compressFactorElevenBit = static_cast<float>(BINARY3(0011, 1111, 1111)) / halfRange;
		s_formatPrecisionInfo[baseShiftCount].expandFactorElevenBit = halfRange / static_cast<float>(BINARY3(0011, 1111, 1111));

		s_formatPrecisionInfo[baseShiftCount].compressFactorTenBit = static_cast<float>(BINARY3(0001, 1111, 1111)) / halfRange;
		s_formatPrecisionInfo[baseShiftCount].expandFactorTenBit = halfRange / static_cast<float>(BINARY3(0001, 1111, 1111));

		uint8_t const formatId = s_formatPrecisionInfo[baseShiftCount].formatId;

		int const baseCount = s_formatPrecisionInfo[baseShiftCount].baseCount;

		for (int i = 0; i < baseCount; ++i)
		{
			uint8_t const formatIndex = static_cast<uint8_t>(formatId | static_cast<uint8_t>(i));
			float const baseValue = -1.0f + (i + 1) * baseSeparation;

			s_formatData[formatIndex].install(baseValue, static_cast<uint8_t>(baseShiftCount));
		}
	}
}

Geometry::Vector4 QuatExpand::UncompressQuaternion::ExpandCompressedValue(uint32_t value, uint8_t xFormat, uint8_t yFormat, uint8_t zFormat)
{
	float w;
	float x;
	float y;
	float z;

	x = s_formatData[xFormat].expandElevenBit(value >> cs_xShift);
	y = s_formatData[yFormat].expandElevenBit(value >> cs_yShift);
	z = s_formatData[zFormat].expandTenBit(value);
	w = sqrt(1.0 - (x * x + y * y + z * z));

	Geometry::Vector4 vec{ x, y, z, w };
	return vec;
}