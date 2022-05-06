#pragma once

#include "IFF_file.h"
#include "objects/animated_object.h"

class anim_parser : public IFF_visitor
{
public:
	// Inherited via IFF_visitor
	virtual void section_begin(const std::string& name, uint8_t* data_ptr, size_t data_size, uint32_t depth) override;
	virtual void parse_data(const std::string& name, uint8_t* data_ptr, size_t data_size) override;

	virtual bool is_object_parsed() const
	{
		// Add in check to see if correct data was obtained
		return m_animation->hasFPS(); // If the animation has a FPS other then 0, we are good (Note: will need to double check this if this is valid for all cases
	}
	virtual std::shared_ptr<Base_object> get_parsed_object() const override
	{
		return std::dynamic_pointer_cast<Base_object>(m_animation);
	}

private:
	std::shared_ptr<Animation> m_animation;
	uint16_t p_boneCounter = 0;
	bool p_isKFATFORM = false;
};
