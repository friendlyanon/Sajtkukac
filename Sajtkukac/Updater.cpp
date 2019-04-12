/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * Updater.cpp is part of Sajtkukac.
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
#include "Updater.h"

#pragma comment (lib, "Wininet.lib")
#pragma comment (lib, "urlmon.lib")

namespace
{

BOOL networking = FALSE;
DWORD dwThreadID = 0;
HANDLE hThread = nullptr;

struct UPDATER_DETAILS
{
	HWND hWnd = nullptr;
	LPARAM lParam = 0;
};

}

DWORD WINAPI CheckUpdate(LPVOID pParam)
{
#define RET(val) return returnValue = val
	enum updateStates
	{
		UPDATE_INET_ERROR,
		UPDATE_NOT_FOUND,
		UPDATE_FOUND
	} returnValue = UPDATE_NOT_FOUND;

	auto details = (UPDATER_DETAILS*)pParam;
	HWND hWnd = details->hWnd;
	LPARAM lParam = details->lParam;

	ULONG major = 0, minor = 0;

	SCOPE_EXIT{
		networking = FALSE;
		delete details;
		switch (returnValue)
		{
		case UPDATE_INET_ERROR:
			::PostMessage(hWnd, WM_USER_UPDATER,
				WM_USER_UPDATER_FAIL, ::GetLastError());
			break;
		case UPDATE_NOT_FOUND:
			::PostMessage(hWnd, WM_USER_UPDATER,
				WM_USER_UPDATER_NOT_FOUND, 0);
			break;
		case UPDATE_FOUND:
			::PostMessage(hWnd, WM_USER_UPDATER,
				WM_USER_UPDATER_FOUND, MAKELPARAM(major, minor));
			break;
		}
		::CloseHandle(hThread);
		hThread = nullptr;
	};

	CHAR userAgent[512];
	DWORD userAgentSize = 0;
	if (::ObtainUserAgentString(0, userAgent, &userAgentSize))
	{
		::sprintf_s(userAgent,
			"Mozilla/4.0 (compatible; MSIE 11.0; Windows NT 10.0)");
	}

	INET_PTR(hInet, ::InternetOpenA(
		userAgent, INTERNET_OPEN_TYPE_PRECONFIG,
		nullptr, nullptr, 0));

	if (hInet == nullptr) RET(UPDATE_INET_ERROR);

	INET_PTR(hConn, ::InternetConnectA(
		hInet, "github.com", INTERNET_DEFAULT_HTTPS_PORT,
		nullptr, nullptr, INTERNET_SERVICE_HTTP, 0,
		INTERNET_NO_CALLBACK));

	if (hConn == nullptr) RET(UPDATE_INET_ERROR);

	LPCSTR acceptTypes[]{ "*/*", nullptr };

	INET_PTR(hReq, ::HttpOpenRequestA(
		hConn, "HEAD", "/friendlyanon/Sajtkukac/releases/latest",
		"HTTP/1.1", *acceptTypes, nullptr,
		INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_AUTO_REDIRECT,
		INTERNET_NO_CALLBACK));

	if (hReq == nullptr) RET(UPDATE_INET_ERROR);

	if (!::HttpSendRequestA(hReq, nullptr, 0, nullptr, 0))
		RET(UPDATE_INET_ERROR);

	CHAR buffer[INTERNET_MAX_URL_LENGTH];
	DWORD bufferSize;
	if (!::HttpQueryInfoA(hReq, HTTP_QUERY_LOCATION,
		buffer, &bufferSize, nullptr))
	{
		if (::GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND)
			RET(UPDATE_NOT_FOUND);
	}

	if (bufferSize < 20) RET(UPDATE_NOT_FOUND);

	for (PCHAR ptr = buffer + bufferSize;;)
	{
		if (*--ptr != '/') continue;
		major = ::strtol(ptr + 2, &ptr, 10);
		minor = ::strtol(ptr + 1, nullptr, 10);
		break;
	}

	BOOL newer = major > LOWORD(lParam) ||
		major == LOWORD(lParam) && minor > HIWORD(lParam);

	RET(newer ? UPDATE_FOUND : UPDATE_NOT_FOUND);
}

DWORD WINAPI UpdateApplication(LPVOID pParam)
{
	enum updateStates
	{
		UPDATE_INET_ERROR,
		UPDATE_DONE
	} returnValue = UPDATE_DONE;

	auto details = (UPDATER_DETAILS*)pParam;
	HWND hWnd = details->hWnd;
	SCOPE_EXIT{
		networking = FALSE;
		delete details;
		switch (returnValue)
		{
		case UPDATE_INET_ERROR:
			::PostMessage(hWnd, WM_USER_UPDATER,
				WM_USER_UPDATER_FAIL, ::GetLastError());
			break;
		case UPDATE_DONE:
			::PostMessage(hWnd, WM_USER_UPDATER,
				WM_USER_UPDATER_DONE, 0);
			break;
		}
		::CloseHandle(hThread);
		hThread = nullptr;
	};

	WORD major = LOWORD(details->lParam);
	WORD minor = HIWORD(details->lParam);

	CHAR userAgent[512];
	DWORD userAgentSize = 0;
	if (::ObtainUserAgentString(0, userAgent, &userAgentSize))
	{
		::sprintf_s(userAgent,
			"Mozilla/4.0 (compatible; MSIE 11.0; Windows NT 10.0)");
	}

	INET_PTR(hInet, ::InternetOpenA(
		userAgent, INTERNET_OPEN_TYPE_DIRECT,
		nullptr, nullptr, 0));

	if (hInet == nullptr) RET(UPDATE_INET_ERROR);

	CHAR pathname[128];
	::StringCbPrintfA(pathname, sizeof(pathname),
		"https://github.com/friendlyanon/Sajtkukac/releases/download/v%d.%d/Sajtkukac_v%d.%d.zip",
		major, minor, major, minor);

	INET_PTR(hUrl, ::InternetOpenUrlA(
		hInet, pathname, nullptr, 0,
		INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0));

	HANDLE hFile = ::CreateFileA("_update.zip",
		GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
	SCOPE_EXIT{ ::CloseHandle(hFile); };

	BYTE data[1024];
	DWORD size = 0, dummy;
	do {
		BOOL result = ::InternetReadFile(
			hUrl, data, sizeof(data), &size);
		if (result == FALSE) RET(UPDATE_INET_ERROR);
		::WriteFile(hFile, data, size, &dummy, nullptr);
	} while (size > 0);

	RET(UPDATE_DONE);
}

BOOL Updater(HWND hWnd, LPARAM lParam, BOOL checkOnly)
{
	if (networking) return FALSE;

	LPTHREAD_START_ROUTINE fPtr = checkOnly
		? ::CheckUpdate
		: ::UpdateApplication;

	if (!(hThread = ::CreateThread(0, 0, fPtr,
		new UPDATER_DETAILS{ hWnd, lParam }, 0, &dwThreadID)))
	{
		return FALSE;
	}

	return networking = TRUE;
}
