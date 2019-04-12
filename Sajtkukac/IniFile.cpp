/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * IniFile.cpp is part of Sajtkukac.
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

#include "stdafx.h"

namespace
{

const auto iniPath = (::std::experimental::filesystem::current_path()
	/ "Sajtkukac.ini").native();

UINT Load(LPCWSTR key, UINT def)
{
	return ::GetPrivateProfileIntW(L"Sajtkukac", key, def,
		iniPath.data());
}

VOID Store(LPCWSTR key, UINT value)
{
	WCHAR buffer[256];
	::StringCbPrintf(buffer, sizeof(buffer), L"%u", value);
	::WritePrivateProfileStringW(L"Sajtkukac", key, buffer,
		iniPath.data());
}

} // namespace

VOID ReadIni(UINT &uPerc, UINT &uRate, UINT &uEase)
{
	uPerc = ::Load(L"Percentage", 50);
	uRate = ::Load(L"RefreshRate", 500);
	uEase = ::Load(L"EasingFunction", 25);
}

VOID WriteIni(UINT uPerc, UINT uRate, UINT uEase)
{
	::Store(L"Percentage", uPerc);
	::Store(L"RefreshRate", uRate);
	::Store(L"EasingFunction", uEase);
}
