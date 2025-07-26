// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _SCL_SECURE_NO_WARNINGS
#define NOMINMAX


#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <assert.h>


// TODO: reference additional headers your program requires here
#include <Windows.h>

// STL
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <cstdint>
#include <exception>
#include <functional>
#include <regex>
#include <stack>
#include <locale>
#include <bitset>
#include <array>
#include <queue>

// ZLIB
#include <zconf.h>
#include <zlib.h>

// boost
#include <boost/progress.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

// FBX SDK
//#pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2016.1.2\\lib\\vs2015\\x64\\debug\\libfbxsdk-mt.lib")
////#include "C:\Program Files\Autodesk\FBX\FBX SDK\2016.1.2\include\fbxsdk.h"
//#pragma comment(lib, "C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2020.0.1\\lib\\vs2017\\x64\\debug\\libfbxsdk-mt.lib")
#include "C:\Program Files\Autodesk\FBX\FBX SDK\2020.3.7\include\fbxsdk.h"

// DirectXTex
#include <DirectXTex.h>
