# SWGModelExporter-master

# About

Tool reads StarWars Galaxies game resource database and extracts mesh (static and animated)
into FBX format for further processing.

# Installing

Leaving a zipped copy of the DirectxTex library here for new installs.
1) Extract the contents of this zipped file into the directory above the solution directory.
2) Also rename the folder by removing the wording "-main"

Install the FBXSDK
Make sure in the project settings under Additional Directories under C/C++->General, the include directories are pointing to the installed FBX include headers.

Installing everything else:
1) Create a folder in the main solution directory called Boost
2) Extract the contents of boost.7z into this new folder
3) Extract teh contents of the lib.7z folder to the solution directory

# Additional Notes

Please remember to check all include directory paths for any changes. This version used FBX SDK 2020.1

Another thing to note is that you might get linker error: 
error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MDd_DynamicDebug' doesn't match value 'MTd_StaticDebug'

See this forum post for a fix:
https://stackoverflow.com/questions/14714877/mismatch-detected-for-runtimelibrary

Also there may be two versions of the FBX 2020 installers floating around. One installed to 2020.0.1 and another 2020.1. Right now, the program is configured for 2020.0.1. If you get 2020.1, then you will need to change settings for the C/C++ include directories, Linker Directories, and stdafx.h

# Resources

Autodesk FBS SDX - https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-0

DirectXTex - https://github.com/Microsoft/DirectXTex/

# Concluding Words

If you run into bugs, please submit a bug report to the GitHub page. If you want additional feature, you can still submit a request. There are a few known bugs that need to be solved which are already listed.

# Special Thanks

I would like to personally thank the following members for assistance on this project:

Synter
Borrie BoBaka
bhtrail - the initial creator of the project. Link to original project: https://github.com/bhtrail/SWGModelExporter
 