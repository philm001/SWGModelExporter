find_path(FBXSDK_INCLUDE_DIR
  NAMES fbxsdk.h
  HINTS
    ${FBXSDK_ROOT}
    ${FBXSDK_DIR}
    ENV FBXSDK_ROOT
    ENV FBXSDK_DIR
  PATH_SUFFIXES
    include
)

if(WIN32)
  if(SWGME_FBXSDK_SHARED)
    set(_fbxsdk_library_names libfbxsdk libfbxsdk-md libfbxsdk-mt fbxsdk)
    find_file(FBXSDK_RUNTIME_LIBRARY
      NAMES libfbxsdk.dll fbxsdk.dll
      HINTS
        ${FBXSDK_ROOT}
        ${FBXSDK_DIR}
        ENV FBXSDK_ROOT
        ENV FBXSDK_DIR
      PATH_SUFFIXES
        bin
        lib/x64/release
        lib/x64/debug
        lib/vs2019/x64/release
        lib/vs2019/x64/debug
        lib/vs2022/x64/release
        lib/vs2022/x64/debug
    )
  else()
    set(_fbxsdk_library_names libfbxsdk-mt libfbxsdk-md libfbxsdk fbxsdk)
  endif()
else()
  set(_fbxsdk_library_names fbxsdk libfbxsdk)
endif()

find_library(FBXSDK_LIBRARY
  NAMES ${_fbxsdk_library_names}
  HINTS
    ${FBXSDK_ROOT}
    ${FBXSDK_DIR}
    ENV FBXSDK_ROOT
    ENV FBXSDK_DIR
  PATH_SUFFIXES
    lib
    lib64
    lib/release
    lib/debug
    lib/gcc/x64/release
    lib/gcc/x64/debug
    lib/clang/release
    lib/clang/debug
    lib/x64/release
    lib/x64/debug
    lib/vs2019/x64/release
    lib/vs2019/x64/debug
    lib/vs2022/x64/release
    lib/vs2022/x64/debug
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FBXSDK
  REQUIRED_VARS FBXSDK_INCLUDE_DIR FBXSDK_LIBRARY
  FAIL_MESSAGE "Could NOT find Autodesk FBX SDK. Set FBXSDK_ROOT or FBXSDK_DIR to the extracted SDK directory."
)

if(FBXSDK_FOUND AND NOT TARGET FBXSDK::FBXSDK)
  add_library(FBXSDK::FBXSDK UNKNOWN IMPORTED)
  set_target_properties(FBXSDK::FBXSDK PROPERTIES
    IMPORTED_LOCATION "${FBXSDK_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${FBXSDK_INCLUDE_DIR}"
  )

  if(WIN32)
    if(SWGME_FBXSDK_SHARED)
      set_property(TARGET FBXSDK::FBXSDK APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS FBXSDK_SHARED)
    else()
      set(_fbxsdk_libxml_names libxml2)
      if(FBXSDK_LIBRARY MATCHES "-mt\\.lib$")
        set(_fbxsdk_libxml_names libxml2-mt libxml2)
      elseif(FBXSDK_LIBRARY MATCHES "-md\\.lib$")
        set(_fbxsdk_libxml_names libxml2-md libxml2)
      endif()

      find_library(FBXSDK_LIBXML2_LIBRARY
        NAMES ${_fbxsdk_libxml_names}
        HINTS
          ${FBXSDK_ROOT}
          ${FBXSDK_DIR}
          ENV FBXSDK_ROOT
          ENV FBXSDK_DIR
        PATH_SUFFIXES
          lib
          lib64
          lib/release
          lib/debug
          lib/gcc/x64/release
          lib/gcc/x64/debug
          lib/clang/release
          lib/clang/debug
          lib/x64/release
          lib/x64/debug
          lib/vs2019/x64/release
          lib/vs2019/x64/debug
          lib/vs2022/x64/release
          lib/vs2022/x64/debug
      )

      if(FBXSDK_LIBXML2_LIBRARY)
        set_property(TARGET FBXSDK::FBXSDK APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${FBXSDK_LIBXML2_LIBRARY}")
        set_property(TARGET FBXSDK::FBXSDK APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS LIBXML_STATIC)
      endif()
    endif()
  else()
    set_property(TARGET FBXSDK::FBXSDK APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${CMAKE_DL_LIBS};Threads::Threads")
  endif()
endif()

mark_as_advanced(FBXSDK_INCLUDE_DIR FBXSDK_LIBRARY FBXSDK_RUNTIME_LIBRARY FBXSDK_LIBXML2_LIBRARY)
