find_path(FreeImage_INCLUDE_DIR
  NAMES FreeImage.h
  HINTS
    ${FREEIMAGE_ROOT}
    ${FREEIMAGE_DIR}
    ENV FREEIMAGE_ROOT
    ENV FREEIMAGE_DIR
  PATH_SUFFIXES
    include
    Dist/x64
    Dist/x32
    FreeImage/Dist/x64
    FreeImage/Dist/x32
)

find_library(FreeImage_LIBRARY
  NAMES freeimage FreeImage
  HINTS
    ${FREEIMAGE_ROOT}
    ${FREEIMAGE_DIR}
    ENV FREEIMAGE_ROOT
    ENV FREEIMAGE_DIR
  PATH_SUFFIXES
    lib
    lib64
    Lib
    Dist/x64
    Dist/x32
    FreeImage/Dist/x64
    FreeImage/Dist/x32
)

if(WIN32)
  find_file(FreeImage_RUNTIME_LIBRARY
    NAMES FreeImage.dll freeimage.dll
    HINTS
      ${FREEIMAGE_ROOT}
      ${FREEIMAGE_DIR}
      ENV FREEIMAGE_ROOT
      ENV FREEIMAGE_DIR
    PATH_SUFFIXES
      bin
      Bin
      lib
      Lib
      Dist/x64
      Dist/x32
      FreeImage/Dist/x64
      FreeImage/Dist/x32
  )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeImage
  REQUIRED_VARS FreeImage_INCLUDE_DIR FreeImage_LIBRARY
)

if(FreeImage_FOUND AND NOT TARGET FreeImage::FreeImage)
  add_library(FreeImage::FreeImage UNKNOWN IMPORTED)
  set_target_properties(FreeImage::FreeImage PROPERTIES
    IMPORTED_LOCATION "${FreeImage_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${FreeImage_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(FreeImage_INCLUDE_DIR FreeImage_LIBRARY FreeImage_RUNTIME_LIBRARY)
