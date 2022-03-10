#pragma once
#include <defines.h>
#include "objects/geometry_common.h"

// packed format: [MSB] x-11-bit  y-11-bit  z-10-bit
const uint32_t cs_xShift = 21;
const uint32_t cs_yShift = 10;

// Acceptable error in given w calculation from real w calculation.
const float cs_xAcceptableEpsilon = 0.001f;
const float cs_yAcceptableEpsilon = 0.001f;
const float cs_zAcceptableEpsilon = 2.0f * cs_xAcceptableEpsilon;
const float cs_wAcceptableEpsilon = 0.1f;

const int    cs_minFormatValue = 0;
const int    cs_maxFormatValue = 254;

namespace QuatExpand
{
	class FormatData
	{
	public:
		FormatData()
		{

		}

		void install(float baseValue, uint8_t formatPrecisionIndex)
		{
			m_baseValue = baseValue;
			m_formatPrecisionIndex = formatPrecisionIndex;
		}

		float expandTenBit(uint32_t compressedValue) const;

		float expandElevenBit(uint32_t compressedValue) const;

	private:
		float m_baseValue;
		uint8_t m_formatPrecisionIndex;
	};


	class UncompressQuaternion
	{
	private:
		FormatData s_formatData[cs_maxFormatValue + 1];

		int convertShiftToCount(int shift)
		{
			return (0x01 << static_cast<uint8_t>(shift));
		}

		float calculateRange(int baseShiftCount)
		{
			return 4.0f / static_cast<float>(convertShiftToCount(baseShiftCount) + 1);
		}
	public:
		void install();
		Geometry::Vector4 ExpandCompressedValue(uint32_t value, uint8_t xFormat, uint8_t yFormat, uint8_t zFormat);
	};
}


