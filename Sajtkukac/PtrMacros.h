/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * PtrMacros.h is part of Sajtkukac.
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

#pragma once

#include "scopeexit/ScopeExit.h"

// return if result is not null
#define CHECK_HR(x) if (auto r = x; r) return r
// release if x is not nullptr
#define RELEASE(x) if (auto r = x; r != nullptr) r->Release()
// declare pointer and init with nullptr
#define DECLARE_PTR(type, id) \
	type * id = nullptr
// declare scoped pointer with release scope exit function
#define SCOPED_PTR(type, id) DECLARE_PTR(type, id); \
	SCOPE_EXIT{ if (id != nullptr && !id->Release()) id = nullptr; }


#define DECLARE_INET_PTR(id, val) HINTERNET id = val
#define SCOPED_INET_PTR(id, val) DECLARE_INET_PTR(id, val); \
	SCOPE_EXIT{ if (id != nullptr) { ::InternetCloseHandle(id); id = nullptr; } }
