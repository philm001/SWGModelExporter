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

### ? **Resolved: Template Linker Errors (Latest Fix)**

**Issue**: Unresolved external symbol for template function `calculateBoneAnimationData<Animation::Bone_info>`
- **Root Cause**: Template function declared in header but implemented in .cpp file without explicit instantiation
- **Solution**: Added explicit template instantiation in `SWGSkeletonExport.cpp`:
  ```cpp
  template std::vector<SWGMainObject::AnimationCurveData> 
  SWGMainObject::calculateBoneAnimationData<Animation::Bone_info>(
      const Animation::Bone_info& animatedBoneIterator,
      std::shared_ptr<Animation> animationObject);
  ```

### ? **Resolved: Project Build Issues**

**Issue**: Build failed due to missing source files referenced in project
- **Root Cause**: Empty .cpp files were removed but still referenced in vcxproj
- **Solution**: Cleaned up project file by removing references to:
  - `SWGAssetParser.cpp` (empty file)
  - `SWGFBXSceneBuilder.cpp` (empty file) 
  - `SWGMainObject_New.cpp` (empty file)
  - `SWGMainObject_Refactored.cpp` (empty file)

### ? **Resolved: Animation System Fixes (Previously Fixed)**

The animation system had several critical bugs that were successfully resolved:

#### Issues Fixed:
1. **180-Degree Bone Rotation Bug**: Mathematical errors in quaternion to Euler conversion
2. **FBX Curve Order**: Wrong curve assignment (Translation/Rotation/Scale order)
3. **Gimbal Lock**: Missing gimbal lock handling causing animation flips
4. **Quaternion Normalization**: Precision drift from unnormalized quaternions
5. **Animation Curve Processing**: Corrected vector order and frame processing

#### Solutions Applied:
- ? **Rewrote quaternion conversion** with proper gimbal lock handling
- ? **Fixed FBX curve order** to match FBX SDK expectations (Translation, Rotation, Scale)
- ? **Added quaternion normalization** to prevent precision errors
- ? **Improved angle normalization** to prevent 180° jumps
- ? **Enhanced frame validation** with NaN and infinite value checking

### ? **Resolved: Bind Pose Problems (Previously Fixed)**

**Issue**: Skeletal meshes appeared incorrectly positioned or deformed in exported FBX files

#### Root Causes Identified:
1. **Duplicate Skeleton Generation**: The `storeMGN()` function was calling `generateSkeletonInScene()` twice
2. **FBX Bind Pose Setup**: Missing proper mesh node inclusion in bind pose data
3. **Transform Matrix Issues**: Incorrect calculation of bone transform matrices
4. **Rotation Conflicts**: Pre/post rotation conflicts causing 180° bone issues

#### Fixes Implemented:
- ? **Removed duplicate skeleton generation calls**
- ? **Fixed bind pose creation** by adding mesh node first (FBX SDK requirement)
- ? **Enhanced cluster setup** with proper skin deformation settings
- ? **Implemented single rotation system** combining pre/post/bind rotations
- ? **Added extensive debug logging** for troubleshooting bone data integrity

---

## Code Architecture Improvements

### **Modular Refactoring**
The project has been refactored into a more modular structure:

- **`SWGAnimationParsing.cpp`**: All animation-related FBX export logic
- **`SWGSkeletonExport.cpp`**: Skeleton creation and bind pose management
- **`SWGDependencyResolver.cpp`**: Asset dependency resolution
- **`SWGFileAccess.cpp`**: File I/O and export coordination

### **Template System Enhancement**
- Added robust template support for flexible bone animation processing
- Implemented explicit template instantiation to resolve linker issues
- Enhanced type safety with template parameter validation

### **Threading Architecture**
- **Parallel Processing**: Internal mathematical computations parallelized
- **Sequential File Access**: Disk operations remain sequential for stability
- **Thread-Safe Operations**: Proper mutex usage for shared data structures

---

## Technical Challenges

### Complexity Areas:
1. **Proprietary Format Reverse Engineering**: SWG uses custom binary formats requiring careful parsing
2. **FBX SDK Integration**: Complex API with strict requirements for bind poses and animation curves  
3. **Multi-threading**: Balancing performance with FBX SDK thread safety limitations
4. **Memory Management**: Large meshes and animations require careful resource handling
5. **Coordinate System Conversion**: Transforming from SWG's coordinate system to FBX standards
6. **Template Instantiation**: C++ template linking across compilation units

### Performance Optimizations:
- **Parallel Dependency Resolution**: Multi-threaded shader and skeleton processing
- **Threaded Animation Processing**: Mathematical computations parallelized
- **Memory-Efficient Streaming**: Large files processed in chunks
- **Caching System**: Object lookup optimization for dependency resolution

---

## Build Configuration

### Dependencies:
- **Autodesk FBX SDK 2020.3.7**: For FBX file creation and manipulation
- **Boost Libraries**: Filesystem, program options, tokenizer, progress display
- **DirectXTex**: For DDS texture processing and conversion
- **Windows SDK**: For COM initialization and file operations

### Debug vs Release:
- **Debug Mode**: Uses hardcoded paths for easy development and testing
- **Release Mode**: Full command-line interface for production use
- **Test Objects**: Various SWG assets included for validation (creatures, characters, static meshes)

---

## Current Status

### ? **Fully Working Features**:
- TRE archive reading and object extraction
- Static mesh export (MSH files)  
- Texture conversion (DDS to TGA)
- Animation export with proper bone rotations
- Batch processing capabilities
- Multi-threaded performance optimizations
- Template-based animation processing
- Modular code architecture

### ? **Recently Fixed**:
- **Template linker errors** - Explicit instantiation resolved
- **Project build issues** - Cleaned up empty file references
- **Animation 180-degree rotation bug** - Mathematical fixes applied
- **FBX curve ordering issues** - Proper Translation/Rotation/Scale order
- **Bind pose generation problems** - Single rotation system implemented
- **Duplicate skeleton creation** - Removed redundant calls

### ?? **Current Development**:
- Performance optimization for large batch processing
- Memory usage optimization for complex models
- Additional template specializations for different bone types
- Enhanced error handling and logging

The project represents a significant reverse-engineering effort to preserve and convert assets from Star Wars Galaxies, enabling continued use of these 3D assets in modern development workflows. The recent modular refactoring and bug fixes have significantly improved the stability and maintainability of the codebase.