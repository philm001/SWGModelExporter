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

## Current Issues Being Addressed

### ?? Primary Issue: Bind Pose Problems

The project has been experiencing **bind pose issues** in exported FBX files, where skeletal meshes appear incorrectly positioned or deformed when imported into 3D software.

#### Root Causes Identified:
1. **Duplicate Skeleton Generation**: The `storeMGN()` function was calling `generateSkeletonInScene()` twice, causing conflicts
2. **FBX Bind Pose Setup**: Missing proper mesh node inclusion in bind pose data
3. **Transform Matrix Issues**: Incorrect calculation of bone transform matrices
4. **Memory Management**: Bone node pointers not being preserved correctly

#### Fixes Implemented:
- ? **Removed duplicate skeleton generation calls**
- ? **Fixed bind pose creation** by adding mesh node first (FBX SDK requirement)
- ? **Enhanced cluster setup** with proper skin deformation settings
- ? **Added extensive debug logging** for troubleshooting bone data integrity
- ? **Improved bone validation** with position and hierarchy checks

### ?? Animation System Fixes (Previously Resolved)

The animation system had several critical bugs that were successfully fixed:

#### Issues Fixed:
1. **180-Degree Bone Rotation Bug**: Mathematical errors in quaternion to Euler conversion
2. **FBX Curve Order**: Wrong curve assignment (Translation/Rotation/Scale order)
3. **Gimbal Lock**: Missing gimbal lock handling causing animation flips
4. **Quaternion Normalization**: Precision drift from unnormalized quaternions

#### Solutions Applied:
- ? **Rewrote quaternion conversion** with proper gimbal lock handling
- ? **Fixed FBX curve order** to match FBX SDK expectations
- ? **Added quaternion normalization** to prevent precision errors
- ? **Improved angle normalization** to prevent 180° jumps

---

## Technical Challenges

### Complexity Areas:
1. **Proprietary Format Reverse Engineering**: SWG uses custom binary formats requiring careful parsing
2. **FBX SDK Integration**: Complex API with strict requirements for bind poses and animation curves  
3. **Multi-threading**: Balancing performance with FBX SDK thread safety limitations
4. **Memory Management**: Large meshes and animations require careful resource handling
5. **Coordinate System Conversion**: Transforming from SWG's coordinate system to FBX standards

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

### ? Working Features:
- TRE archive reading and object extraction
- Static mesh export (MSH files)
- Texture conversion (DDS to TGA)
- Animation export with proper bone rotations
- Batch processing capabilities
- Multi-threaded performance optimizations

### ?? Recently Fixed:
- Animation 180-degree rotation bug
- FBX curve ordering issues
- Bind pose generation problems
- Duplicate skeleton creation

### ?? Under Investigation:
- Final bind pose validation and testing
- Performance optimization for large batches
- Memory usage optimization for complex models

The project represents a significant reverse-engineering effort to preserve and convert assets from Star Wars Galaxies, enabling continued use of these 3D assets in modern development workflows.