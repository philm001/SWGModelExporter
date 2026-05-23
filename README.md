# SWGModelExporter-master

# About

Tool reads StarWars Galaxies game resource database and extracts mesh (static and animated)
into FBX format for further processing.

# Installing

The project now supports a CMake-based build for both Linux and Windows.

## Dependencies

- Autodesk FBX SDK 2020.x
- Boost (`filesystem`, `program_options`)
- zlib
- FreeImage

On Linux, the included `linux-release` preset is designed to work with a local `vcpkg` checkout at `.third_party/vcpkg` for the non-FBX dependencies.

## Linux build

1. Install or extract the Autodesk Linux FBX SDK and point `FBXSDK_ROOT` at it.
2. Configure:
   `FBXSDK_ROOT=/path/to/fbx-sdk cmake --preset linux-release`
3. Build:
   `FBXSDK_ROOT=/path/to/fbx-sdk cmake --build build-linux -j`

The checked-in build now uses command-line arguments by default. If you want the old hardcoded developer inputs, enable `SWGME_ENABLE_DEBUG_MODE=ON` when configuring.

## Windows build

1. Install the Autodesk Windows FBX SDK and point `FBXSDK_ROOT` at it.
2. Set `VCPKG_ROOT` to a `vcpkg` checkout with the `x64-windows-static` triplet available.
3. Configure:
   `cmake --preset windows-vs2022-release`
4. Build:
   `cmake --build --preset windows-vs2022-release`

## Windows cross-build from Linux

This repository also includes a Linux-hosted Windows cross-build based on `clang-cl`, `xwin`, and `lld-link`-compatible tooling.

1. Extract the Autodesk Windows FBX SDK and point `FBXSDK_ROOT` at it.
2. Extract the Windows FreeImage binary package and point `FREEIMAGE_ROOT` at it.
3. Prepare a local Windows SDK/CRT sysroot with `xwin` under `.xwin`.
4. Configure:
   `FBXSDK_ROOT=/path/to/fbx-sdk FREEIMAGE_ROOT=/path/to/freeimage cmake --preset windows-clangcl-release`
5. Build:
   `cmake --build build-windows-clangcl-release -j`
6. Install the runnable bundle:
   `cmake --install build-windows-clangcl-release --prefix out/windows-clangcl-release`

The `windows-clangcl-release` preset intentionally leaves `FBXSDK_ROOT` and `FREEIMAGE_ROOT` unset so the configure step can honor your environment or explicit `-D...` overrides. The resulting Windows bundle contains `SWGModelExporter.exe` and `FreeImage.dll`. The FBX SDK is linked statically in this configuration.

# Additional Notes

Please remember to check all include directory paths for any changes. This version used FBX SDK 2020.0.1

Another thing to note is that you might get linker error:
error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MDd_DynamicDebug' doesn't match value 'MTd_StaticDebug'

See this forum post for a fix:
https://stackoverflow.com/questions/14714877/mismatch-detected-for-runtimelibrary

Also there may be two versions of the FBX 2020 installers floating around. One installed to 2020.0.1 and another 2020.1. The current CMake finder is version-agnostic as long as `FBXSDK_ROOT` points at the installed SDK root.

# Resources

Autodesk FBS SDX - https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-0

FreeImage - https://freeimage.sourceforge.io/

# Concluding Words

If you run into bugs, please submit a bug report to the GitHub page. If you want additional feature, you can still submit a request. There are a few known bugs that need to be solved which are already listed.

# Special Thanks

I would like to personally thank the following members for assistance on this project:

1) Synter
2) Borrie BoBaka
3) bhtrail - the initial creator of the project. Link to original project: https://github.com/bhtrail/SWGModelExporter
 
