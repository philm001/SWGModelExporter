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
		//std::cout << "Found uncompress";
		p_isKFATFORM = true;
		m_animation = make_shared<Animation>();
	}

}

void anim_parser::parse_data(const string& name, uint8_t* data_ptr, size_t data_size)
{
	if (!m_animation)
		return;

	base_buffer buffer(data_ptr, data_size);

	if (name == "0001INFO" || name == "0003INFO")
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
		if (p_isKFATFORM)
		{
			auto key_count = buffer.read_uint32();
			std::vector<std::vector<float>> frameRotations;
			auto loopCounter = key_count;

			for (uint16_t frameCount = 0; frameCount < loopCounter; frameCount++)
			{
				auto frameNumber = buffer.read_uint32();
				while (frameNumber != frameCount)
				{
					std::vector<float> temp;
					temp.push_back(-100);
					frameRotations.push_back(temp);
					frameCount++;
					loopCounter++;
				}

				float quatX = buffer.read_float();
				float quatY = buffer.read_float();
				float quatZ = buffer.read_float();
				float quatA = buffer.read_float();

				std::vector<float> temp;

				temp.push_back(quatX);
				temp.push_back(quatY);
				temp.push_back(quatZ);
				temp.push_back(quatA);

				frameRotations.push_back(temp);
			}
			m_animation->getKFATQCHNValues().push_back(frameRotations);
		}
		else
		{
			auto key_count = buffer.read_uint16();
			auto x_format = buffer.read_uint8();
			auto y_format = buffer.read_uint8();
			auto z_format = buffer.read_uint8();

			// Compress the format values into a single 16 bit data type and transport this along with the animation data
			uint32_t valueFormats = uint32_t(x_format) << 16;
			valueFormats |= uint32_t(y_format) << 8;
			valueFormats |= uint32_t(z_format);

			std::vector<uint32_t> frameRotations;
			frameRotations.push_back(valueFormats);

			auto loopCounter = key_count;
			for (uint16_t count = 0; count < loopCounter; count++)
			{
				auto frame_num = buffer.read_uint16();// This may need to get passed to the exporter
				while (frame_num != count)
				{
					frameRotations.push_back(100);
					count++;
					loopCounter++;
				}

				auto floatRead = buffer.read_uint32();
				frameRotations.push_back(floatRead);
			}
			m_animation->getQCHNValues().push_back(frameRotations);
		}
	}
	else if (name == "SROT" || name == "AROTSROT")
	{
		if (p_isKFATFORM)
		{
			uint16_t dataCounterSize = data_size / 4;
			for (int i = 0; i < dataCounterSize; i++)
			{
				float value = buffer.read_float();
				m_animation->getStaticKFATRotationValues().push_back(value);
			}
		}
		else
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
		
	}
	else if (name == "ATRNCHNL" || name == "CHNL")
	{
		uint32_t key_count = 0;// This is general frame count

		if (p_isKFATFORM)
			key_count = buffer.read_uint32();
		else
			key_count = (uint32_t)buffer.read_uint16();

		std::vector<float> frameTranslations;
		auto loopCounter = key_count;
		auto counter = 0;

		while (buffer.get_position() < buffer.get_size())
		{
			uint32_t frame_num = 0;

			if(p_isKFATFORM)
				frame_num = buffer.read_uint32();
			else
				frame_num = (uint32_t)buffer.read_uint16();

			while (counter != frame_num)
			{
				frameTranslations.push_back(-1000.0);// If the frame is out of snyc then insert a false frame to let the exporter know to skip this one
				counter++;
			}

			float translationValue = buffer.read_float();
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
