cmake_minimum_required(VERSION 3.25)

project(SWGModelExporter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(SWGME_FBXSDK_SHARED "Link against the shared Autodesk FBX SDK on Windows" OFF)
option(SWGME_ENABLE_DEBUG_MODE "Build with the hardcoded developer debug inputs enabled" OFF)

find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENTS filesystem program_options)
set(SWGME_FREEIMAGE_TARGET FreeImage::FreeImage)
find_package(freeimage CONFIG QUIET)
if(TARGET freeimage::FreeImage)
  set(SWGME_FREEIMAGE_TARGET freeimage::FreeImage)
else()
  find_package(FreeImage REQUIRED)
endif()
find_package(FBXSDK REQUIRED)

set(SWG_MODEL_EXPORTER_SOURCES
  DebugConfig.cpp
  IFF_file.cpp
  Logger.cpp
  SWGAnimationParsing.cpp
  SWGDependencyResolver.cpp
  SWGFileAccess.cpp
  SWGMainObject.cpp
  SWGModelExporter.cpp
  SWGSkeletonExport.cpp
  UncompressQuaternion.cpp
  base_buffer.cpp
  mesh_file.cpp
  objects/animated_object.cpp
  objects/static_object.cpp
  parsers/anim_parser.cpp
  parsers/apt_parser.cpp
  parsers/cat_parser.cpp
  parsers/latt_parser.cpp
  parsers/lmg_parser.cpp
  parsers/mgn_parser.cpp
  parsers/parser_selector.cpp
  parsers/pob_parser.cpp
  parsers/sht_parser.cpp
  parsers/skt_parser.cpp
  stdafx.cpp
  tre_library.cpp
  tre_reader.cpp
)

add_executable(SWGModelExporter ${SWG_MODEL_EXPORTER_SOURCES})

target_include_directories(SWGModelExporter
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/objects"
    "${CMAKE_CURRENT_SOURCE_DIR}/parsers"
)

target_compile_definitions(SWGModelExporter
  PRIVATE
    $<$<BOOL:${SWGME_ENABLE_DEBUG_MODE}>:SWGME_DEBUG_MODE>
)

if(MSVC)
  target_compile_definitions(SWGModelExporter PRIVATE _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
  set_property(TARGET SWGModelExporter PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )
else()
  target_compile_options(SWGModelExporter PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_link_libraries(SWGModelExporter
  PRIVATE
    Boost::filesystem
    Boost::program_options
    FBXSDK::FBXSDK
    ${SWGME_FREEIMAGE_TARGET}
    ZLIB::ZLIB
)

if(WIN32)
  target_link_libraries(SWGModelExporter PRIVATE wininet)
else()
  target_link_libraries(SWGModelExporter PRIVATE Threads::Threads)
endif()

install(TARGETS SWGModelExporter RUNTIME DESTINATION .)

if(WIN32)
  if(DEFINED FBXSDK_RUNTIME_LIBRARY)
    install(FILES "${FBXSDK_RUNTIME_LIBRARY}" DESTINATION . OPTIONAL)
  endif()
  if(DEFINED FreeImage_RUNTIME_LIBRARY)
    install(FILES "${FreeImage_RUNTIME_LIBRARY}" DESTINATION . OPTIONAL)
  endif()
endif()
