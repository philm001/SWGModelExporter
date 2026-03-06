#pragma once

/// <summary>
/// Global debug configuration for verbose logging and LOD export control.
/// Verbose mode is controlled by the --verbose command-line flag.
/// When disabled (default), only minimal logging is output.
/// When enabled, comprehensive debug information is logged across all systems.
/// 
/// LOD mode is controlled by the --no-lod command-line flag.
/// When enabled, only LOD 0 (highest detail) meshes are exported.
/// When disabled (default), all LOD levels are exported.
/// </summary>
class DebugConfig
{
public:
    DebugConfig() = delete;

    // ===== VERBOSE MODE =====
    /// <summary>
    /// Set verbose mode from command line flag.
    /// Must be called early in program initialization.
    /// </summary>
    /// <param name="enabled">true to enable verbose logging, false for minimal logging</param>
    static void setVerboseMode(bool enabled);

    /// <summary>
    /// Check if verbose mode is currently enabled.
    /// </summary>
    /// <returns>true if verbose logging is enabled, false otherwise</returns>
    static bool isVerboseEnabled();

    /// <summary>
    /// Helper function to check if a debug category should log.
    /// This respects the global verbose mode flag.
    /// </summary>
    /// <returns>true if verbose mode is enabled, false otherwise</returns>
    static bool shouldLog();

    // ===== LOD MODE =====
    /// <summary>
    /// Set LOD export mode from command line flag.
    /// When enabled, only LOD 0 (highest detail) meshes are exported.
    /// When disabled (default), all LOD levels are exported.
    /// Must be called early in program initialization.
    /// </summary>
    /// <param name="enabled">true to export LOD 0 only, false to export all LODs</param>
    static void setNoLODMode(bool enabled);

    /// <summary>
    /// Check if LOD 0-only mode is currently enabled.
    /// </summary>
    /// <returns>true if only LOD 0 should be exported, false to export all LODs</returns>
    static bool isNoLODEnabled();

    // ===== LOD ANIMATION MODE =====
    /// <summary>
    /// Set LOD animation mode from command line flag.
    /// When enabled, only LOD 0 (highest detail) gets animations.
    /// LOD 1, 2, 3 meshes export without animation data (static pose).
    /// When disabled (default), all LODs get animations.
    /// Must be called early in program initialization.
    /// </summary>
    /// <param name="enabled">true to skip animations for LOD > 0, false to export all animations</param>
    static void setNoLODAnimationMode(bool enabled);

    /// <summary>
    /// Check if LOD animation mode is currently enabled.
    /// </summary>
    /// <returns>true if LOD 1/2/3 should skip animations, false to animate all LODs</returns>
    static bool isNoLODAnimationEnabled();

private:
    static bool g_verbose_mode;
    static bool g_no_lod_mode;
    static bool g_no_lod_animation_mode;
};
