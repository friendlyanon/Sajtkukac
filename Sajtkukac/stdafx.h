/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * stdafx.h is part of Sajtkukac.
 *
 * Sajtkukac is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Sajtkukac is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Sajtkukac. If not, see <https://www.gnu.org/licenses/>.
 */

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define _HAS_EXCEPTIONS 0
// Windows Header Files
#include <windows.h>
#include <windowsx.h>
#include <Windef.h>
#include <ShlDisp.h>
#include <shellapi.h>
#include <commctrl.h>
#include <ole2.h>
#include <fileapi.h>
#include <UIAutomation.h>
#include <Strsafe.h>
#include <tlhelp32.h>
#include <wininet.h>
#include <urlmon.h>
#if (NTDDI_VERSION >= NTDDI_WIN8)
#include <Pathcch.h>
#endif

// C RunTime Header Files
#include <malloc.h>
#include <memory.h>
#include <wchar.h>

// C++ Runtime Header Files
#include <array>
#include <cstdlib>
#include <simple_match/simple_match.hpp>
#include <unordered_map>
#include <utility>
#include <vector>
