#pragma once

/// <summary>
/// Global debug configuration constants.
/// Set a flag to true to enable that category of debug logging, false to suppress it.
/// This class is non-instantiable; all members are static constexpr.
/// </summary>
class DebugConfig
{
public:
    DebugConfig() = delete;

    /// <summary>
    /// Enables detailed animation export debug logging in SWGAnimationParsing.cpp.
    /// Covers per-animation headers, bone node checks, compressed/uncompressed quaternion
    /// decompression, Euler conversion results, FBX curve assignments, and skip markers.
    /// </summary>
    static constexpr bool ANIM_DEBUG_LOGGING = false;

    /// <summary>
    /// Enables bone rotation debug logging in SWGMainObject.cpp (ConvertCombineCompressQuat).
    /// Prints input quaternion, dot products, Euler angles, and round-trip validation
    /// for bones matching the target bone filter (e.g. leg bones).
    /// </summary>
    static constexpr bool BONE_DEBUG_LOGGING = false;

    /// <summary>
    /// Enables skeleton generation debug logging in SWGSkeletonExport.cpp.
    /// Covers LOD 0 bone count, large rotation corrections, bind pose application,
    /// verification dot products, and low vertex-count warnings for target bones.
    /// </summary>
    static constexpr bool SKELETON_DEBUG_LOGGING = false;
};
