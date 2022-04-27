#include "stdafx.h"
#include "anim_parser.h"

using namespace std;

void anim_parser::section_begin(const string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth)
{
	if (name == "CKATFORM")
	{
		p_boneCounter = 0;
		m_animation = make_shared<Animation>();
	}
	if (name == "KFATFORM" || name == "KFAT")
	{
		std::cout << "Found uncompress";
	}

}

void anim_parser::parse_data(const string& name, uint8_t* data_ptr, size_t data_size)
{
	if (!m_animation)
		return;

	base_buffer buffer(data_ptr, data_size);

	if (name == "0001INFO")
	{
		Animation::Info info;
		info.FPS = buffer.read_float();
		info.frame_count = buffer.read_uint16();
		info.transform_count = buffer.read_uint16();
		info.rotation_channel_count = buffer.read_uint16();
		info.static_rotation_count = buffer.read_uint16();
		info.translation_channel_count = buffer.read_uint16();
		info.static_translation_count = buffer.read_uint16();

		m_animation->set_info(info);
	}
	else if (name == "XFRMXFIN" || name == "XFIN")
	{
		Animation::Bone_info bone;

		bone.name = buffer.read_stringz();

		bone.has_rotations = buffer.read_uint8() == 1;
		bone.rotation_channel_index = buffer.read_uint16();
		bone.translation_mask = buffer.read_uint8();

		// Need to check if the X, Y, Z coordinates are to be animated on translated
		bone.hasZAnimatedTranslation = bone.translation_mask & 0x20;
		bone.hasYAnimatedTranslation = bone.translation_mask & 0x10;
		bone.hasXAnimatedTranslation = bone.translation_mask & 0x08;

		// This part may not be necessary but adding in just in case. Same as above but for rotation
		bone.hasZAnimatedRotatation = bone.translation_mask & 0x04;
		bone.hasYAnimatedRotatation = bone.translation_mask & 0x02;
		bone.hasXAnimatedRotatation = bone.translation_mask & 0x01;

		bone.x_translation_channel_index = buffer.read_uint16();
		bone.y_translation_channel_index = buffer.read_uint16();
		bone.z_translation_channel_index = buffer.read_uint16();

		m_animation->get_bones().push_back(bone);
	}
	else if (name == "AROTQCHN" || name == "QCHN")
	{
		auto key_count = buffer.read_uint16();
		auto x_format = buffer.read_uint8();
		auto y_format = buffer.read_uint8();
		auto z_format = buffer.read_uint8();

		std::vector<uint8_t> vec{ x_format, y_format, z_format };
		uint32_t valueFormats = uint32_t(x_format) << 16;
		valueFormats |= uint32_t(y_format) << 8;
		valueFormats |= uint32_t(z_format);

		uint8_t Xprime = (valueFormats & (((1 << 8) - 1) << 16)) >> 16;
		uint8_t Yprime = (valueFormats & (((1 << 8) - 1) << 8)) >> 8;
		uint8_t Zprime = (valueFormats & (((1 << 8) - 1)));

		std::vector<uint32_t> frameRotations;
		//m_animation->getFormatValues().push_back(vec);
		frameRotations.push_back(valueFormats);
		
		auto loopCounter = key_count;
		for (uint16_t count = 0; count < loopCounter; count++)
		{
			auto frame_num = buffer.read_uint16();// This may need to get passed to the exporter
			while (frame_num != count)
			{
				frameRotations.push_back(100);
				//m_animation->getFormatValues().push_back(vec); // Redundent but need to keep the arrays in sync with each other
				count++;
				loopCounter++;
			}

			auto floatRead = buffer.read_uint32();

			//m_animation->getFormatValues().push_back(vec); // Redundent but need to keep the arrays in sync with each other
			frameRotations.push_back(floatRead);
		} 
		m_animation->getQCHNValues().push_back(frameRotations);
		int k = 0;
		k++;
	}
	else if (name == "SROT" || name == "AROTSROT")
	{
		uint16_t dataCounterSize = data_size / 7;
		for (int i = 0; i < dataCounterSize; i++)
		{
			auto xFormat = buffer.read_uint8();
			auto yFormat = buffer.read_uint8();
			auto zFormat = buffer.read_uint8();

			uint32_t value = buffer.read_uint32();

			std::vector<uint8_t> formats = { xFormat, yFormat, zFormat };
			
			m_animation->getStaticROTFormats().push_back(formats);
			m_animation->getStaticRotationValues().push_back(value);
		}
	}
	else if (name == "ATRNCHNL" || name == "CHNL")
	{
		auto key_count = buffer.read_uint16();// This is general frame count
		std::vector<float> frameTranslations;
		auto loopCounter = key_count;
		auto counter = 0;

		// This is not compressed quats becuase you only need to know the direction of the transform since only 1 component will be animated at a time
		while (buffer.get_position() < buffer.get_size())
		{
			auto frame_num = buffer.read_uint16();
			while (counter != frame_num)
			{
				frameTranslations.push_back(-1000.0);// If the frame is out of snyc then insert a false frame to let the exporter know to skip this one
				counter++;
			}
			auto translationValue = buffer.read_float();
			frameTranslations.push_back(translationValue);
			counter++;
		}

		m_animation->getCHNLValues().push_back(frameTranslations);
	}
	else if (name == "STRN" || name == "ATRNSTRN")
	{
		uint16_t dataCounterSize = data_size / sizeof(float);

		for (int i = 0; i < dataCounterSize; i++)
		{
			auto staticValue = buffer.read_float();

			m_animation->getStaticTranslationValues().push_back(staticValue);
		}
	}
}
