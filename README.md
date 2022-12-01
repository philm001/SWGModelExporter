# SWGModelExporter-master

Leaving a zipped copy of the DirectxTex library here for new installs.
1) Extract the contents of this zipped file into the directory above the solution directory.
2) Also rename the folder by removing the wording "-main"

Install the FBXSDK
Make sure in the project settings under Additional Directories under C/C++->General, the include directories are pointing to the installed FBX include headers.

Installing everything else:
1) Create a folder in the main solution directory called Boost
2) Extract the contents of boost.7z into this new folder
3) Extract teh contents of the lib.7z folder to the solution directory

Please remember to check all include directory paths for any changes. This version used FBX SDK 2020.1

Another thing to note is that you might get linker error: 
error LNK2038 mismatch detected for 'RuntimeLibrary': value 'MDd_DynamicDebug' doesn't match value 'MTd_StaticDebug'

See this forum post for a fix:
https://stackoverflow.com/questions/14714877/mismatch-detected-for-runtimelibrary

 