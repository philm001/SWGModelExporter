#include "stdafx.h"
#include "DebugConfig.h"

// Initialize global verbose mode flag to false (minimal logging by default)
bool DebugConfig::g_verbose_mode = false;

// Initialize global LOD mode flag to false (export all LODs by default)
bool DebugConfig::g_no_lod_mode = false;

// Initialize global LOD animation mode flag to false (all LODs animated by default)
bool DebugConfig::g_no_lod_animation_mode = false;

// ===== VERBOSE MODE IMPLEMENTATION =====
void DebugConfig::setVerboseMode(bool enabled)
{
    g_verbose_mode = enabled;
}

bool DebugConfig::isVerboseEnabled()
{
    return g_verbose_mode;
}

bool DebugConfig::shouldLog()
{
    return g_verbose_mode;
}

// ===== LOD MODE IMPLEMENTATION =====
void DebugConfig::setNoLODMode(bool enabled)
{
    g_no_lod_mode = enabled;
}

bool DebugConfig::isNoLODEnabled()
{
    return g_no_lod_mode;
}

// ===== LOD ANIMATION MODE IMPLEMENTATION =====
void DebugConfig::setNoLODAnimationMode(bool enabled)
{
    g_no_lod_animation_mode = enabled;
}

bool DebugConfig::isNoLODAnimationEnabled()
{
    return g_no_lod_animation_mode;
}
