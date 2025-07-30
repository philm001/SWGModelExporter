# SWG Model Exporter - Project Summary

## Project Overview

The **SWG Model Exporter** is a C++20 application designed to extract and convert 3D models, animations, and assets from Star Wars Galaxies (SWG) game files into modern FBX format for use in 3D modeling software like Maya, Blender, or 3ds Max.

### Key Technologies
- **Language**: C++20 
- **Graphics Framework**: Autodesk FBX SDK 2020.3.7
- **File System**: Boost.Filesystem
- **Threading**: C++11 threading (std::thread, std::future, std::mutex)
- **Build System**: Visual Studio 2022 / MSBuild

---

## File Structure & Purpose

### Core Application Files

#### `SWGModelExporter.cpp` - Main Entry Point
- **Purpose**: Console application entry point with command line argument parsing
- **Functionality**:
  - Parses command line arguments (SWG path, object name, output path, overwrite settings)
  - Includes debug mode with hardcoded paths for development
  - Initializes COM for DirectX operations
  - Supports batch processing mode (e.g., `batch:pob` to extract all POB files)
  - Creates TRE library and orchestrates the extraction process

#### `SWGMainObject.h/.cpp` - Main Processing Engine
- **Purpose**: Central coordinator class that manages the entire extraction pipeline
- **Key Features**:
  - **Multi-threaded Processing**: Uses threading for performance optimization
  - **FBX Export**: Handles complex FBX scene creation with proper bind poses
  - **Animation Processing**: Converts SWG animations to FBX animation curves
  - **Dependency Resolution**: Manages object dependencies and references
  - **Bone System**: Advanced skeletal animation support with proper bone hierarchies
  - **Template Methods**: Includes template functions for flexible bone animation processing

### Modular Processing Files (Recent Refactoring)

#### `SWGAnimationParsing.cpp` - Animation Export Logic
- **Purpose**: Handles the complete FBX animation export process
- **Key Features**:
  - **FBX Animation Stacks**: Creates properly structured animation stacks and layers
  - **Bone Animation Curves**: Processes translation and rotation curves for each bone
  - **Quaternion Decompression**: Handles SWG's compressed quaternion animation data
  - **Frame Rate Management**: Supports various animation frame rates and time modes
  - **Euler Angle Conversion**: Converts quaternions to Euler angles with gimbal lock handling
  - **Animation Validation**: Includes NaN and infinite value checking to prevent corruption

#### `SWGSkeletonExport.cpp` - Skeleton & Bind Pose Management  
- **Purpose**: Manages skeleton creation, bone hierarchies, and FBX bind poses
- **Key Features**:
  - **Single Rotation System**: Combines pre/post/bind rotations to prevent conflicts
  - **FBX Skin Clusters**: Proper vertex weight assignment to bones
  - **Bind Pose Creation**: Correctly structured bind poses for FBX compatibility
  - **Template Animation Data**: Contains `calculateBoneAnimationData<T>` template function
  - **Explicit Template Instantiation**: Resolves linker issues with template specialization

#### `SWGDependencyResolver.cpp` - Asset Dependency Management
- **Purpose**: Resolves dependencies between assets (shaders, textures, etc.)
- **Features**:
  - **Shader Resolution**: Links mesh objects to their material definitions
  - **Object Reference Tracking**: Manages complex asset interdependencies
  - **Validation**: Detects and reports missing or invalid shader references

#### `SWGFileAccess.cpp` - File I/O Operations
- **Purpose**: Manages file access and export operations
- **Features**:
  - **Sequential Export**: Ensures thread-safe file writing operations
  - **Parallel Processing**: Internal computations parallelized while maintaining sequential disk access
  - **MGN Export**: Specialized handling for mesh geometry files

### Archive & Library System

#### `tre_library.cpp/.h` - Game Asset Archive Reader
- **Purpose**: Reads and manages SWG's TRE (game archive) files
- **Functionality**:
  - Loads all .tre files from SWG installation directory
  - Provides object lookup and versioning system
  - Handles file extraction from compressed archives
  - Supports partial name matching and batch operations
  - Error handling for corrupted or missing archive files

### Parser System

#### `IFF_file.cpp/.h` - IFF Format Parser
- **Purpose**: Parses SWG's IFF (Interchange File Format) binary files
- **Features**:
  - Hierarchical chunk-based parsing
  - Stack-based depth tracking for nested structures
  - Specialized processing for different object types (MGN, CAT, etc.)

#### Parser Modules (`parsers/` directory):
- **`cat_parser.h`**: Parses CAT files (Character/Object descriptors)
- **`lmg_parser.h`**: Parses LMG files (Level of Detail lists)  
- **`mgn_parser.cpp/.h`**: Parses MGN files (3D mesh geometry)
- **`skt_parser.cpp`**: Parses SKT files (Skeleton definitions)
- **`parser_selector.h`**: Routes files to appropriate parsers based on format

### Object System

#### `objects/animated_object.cpp/.h` - 3D Object Classes
- **Purpose**: Represents 3D models, animations, and related data
- **Key Classes**:
  - **`Animated_mesh`**: 3D geometry with vertex data, UV coordinates, materials
  - **`Skeleton`**: Bone hierarchy with bind poses and transformations
  - **`Shader`**: Material definitions with textures and rendering properties
  - **`Animation`**: Keyframe animation data with compression support
  - **`DDS_Texture`**: DirectX texture format handling

#### `objects/base_object.h` - Base Object Interface
- **Purpose**: Abstract base class for all game objects
- **Features**: Virtual interface for parsing, dependency resolution, and export

### Utility Files

#### `tre_reader.cpp/.h` - Low-level Archive Access
- **Purpose**: Low-level reading of individual TRE archive files
- **Features**: Binary file reading, resource indexing, data extraction

#### `UncompressQuaternion.h` - Animation Compression
- **Purpose**: Decompresses SWG's proprietary quaternion compression for animations
- **Critical for**: Proper bone rotation extraction from animation files

#### `stdafx.h` - Precompiled Headers
- **Purpose**: Common includes for Windows, STL, Boost, and FBX SDK

---

## Recent Major Improvements & Fixes

### ? **Active Investigation: Skeleton Rotation Issues (Current Problem)**

**Issue**: Acklay creature model shows severe mesh spiral distortion due to incorrect bone rotations during skeleton generation
- **Symptoms**: 
  - Mesh vertices appear twisted in spiral patterns
  - Large discrepancies between expected and actual bone rotations (up to 279° difference)
  - Bones `r_f_leg3` and `r_f_leg_finger` showing critical rotation errors
- **Target Model**: Acklay creature (`appearance/mesh/acklay_l0.mgn`) used for debugging

#### **Investigation Progress & Findings:**

##### **? Root Cause Identified:**
- **Euler Angle Ambiguity**: FBX's `DecomposeSphericalXYZ()` produces large angles when smaller equivalent representations exist
- **Mathematical Accuracy Confirmed**: All quaternion operations are mathematically correct
- **FBX Representation Issue**: Same rotation represented as (192°, -108°, -128°) vs (-168°, 72°, 52°) - both valid but FBX chooses problematic larger angles

##### **? Comprehensive Debug Infrastructure:**
- **Advanced Logging**: Detailed quaternion, Euler angle, and matrix analysis with magnitude tracking
- **Target Bone Focus**: Specialized debugging for problematic leg bones (`r_f_leg`, `l_f_leg`, `l_m_leg` series)
- **Matrix Validation**: Transform matrix determinant checking and parent-relative transform verification
- **Euler Correction Tracking**: `EULER CORRECTION CHECK` logging implemented for bones >100° rotations

##### **? Failed Solution Attempts:**
1. **Quaternion Sign Correction**: Failed because `q` and `-q` produce identical Euler angle magnitudes
2. **Quaternion Order Variations**: Different multiplication orders don't resolve Euler representation issues  
3. **Pre/Post Rotation Systems**: Created conflicts and inconsistencies

##### **?? Current Issue - Euler Correction Not Triggering:**
- **Logic Implemented**: Euler angle normalization code exists in `SWGSkeletonExport.cpp`
- **Detection Working**: System correctly identifies bones with large rotations via `EULER CORRECTION CHECK`
- **Correction Failing**: No `APPLYING EULER ANGLE CORRECTION` messages appear in debug output
- **Algorithm Issue**: The normalization condition `std::abs(angle) > 170.0` may be insufficient

#### **Latest Debug Data Analysis:**
```cpp
// Example from r_f_leg3 bone:
r_f_leg3: Original Euler: (94.6894, -108.141, -128.081) - magnitude: 192.523°
Expected local rotation: (-85.3106, -71.8594, 51.9194)
Actual local rotation: (94.6894, -108.141, -128.081)
Local rotation difference: 257.131 degrees ?? CRITICAL

// Multiple bones showing large rotations detected:
EULER CORRECTION CHECK for l_f_leg: Original magnitude=173.983
EULER CORRECTION CHECK for l_f_leg2: Original magnitude=196.234  
EULER CORRECTION CHECK for r_f_leg2: Original magnitude=196.235
EULER CORRECTION CHECK for r_f_leg3: Original magnitude=192.523
```

#### **Technical Architecture:**
- **Dual Skeleton Systems**: Both `SWGSkeletonExport.cpp` (new modular) and `objects/animated_object.cpp` (legacy) handle skeleton generation
- **Consistent Quaternion Order**: `post_rot_quat * bind_rot_quat * pre_rot_quat` verified in both systems
- **Single Rotation Approach**: Eliminated pre/post rotation conflicts by combining all rotations into local rotation

#### **Next Steps Required:**
1. **Fix Euler Correction Trigger**: Investigate why angle normalization logic doesn't activate
2. **Enhance Correction Algorithm**: Improve the 170° threshold and correction criteria  
3. **Matrix Consistency Resolution**: Address the massive rotation discrepancies in global transform matrices

### ? **Resolved: Template Linker Errors (Previously Fixed)**