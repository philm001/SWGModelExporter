#include "stdafx.h"
#include "SWGMainObject.h"


void SWGMainObject::beginParsingProcess()
{
	p_Buffer->getBuffer()->set_position(0);
	p_Buffer->full_process(m_selected_parser);
}