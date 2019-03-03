/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * main.cpp is part of Sajtkukac.
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

 // Sajtkukac.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "main.h"
#include "scopeexit/ScopeExit.h"

#define MAX_LOADSTRING (100)
#define WM_USER_SHELLICON (WM_USER + 1)

// Global Variables:
HINSTANCE hInst;                                // current instance
NOTIFYICONDATA nidApp;                          // tray icon
HMENU hPopMenu;                                 // right click context menu
WCHAR szTitle[MAX_LOADSTRING];                  // the title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
UINT uPercentage;                               // position of the icons
UINT uRefreshRate;                              // refresh rate in milliseconds
WORKER_DETAILS *wdDetails = nullptr;            // struct to communicate with worker
HANDLE hMutex = nullptr;                        // single instance mutex

// TODO: fall back to system icons until someone makes one :)
#ifndef IDI_SAJTKUKAC
#define SHELL_ICON_ID (3)
HINSTANCE hShell32 = ::LoadLibrary(L"shell32.dll");
HICON hShellIcon = ::LoadIcon(hShell32, MAKEINTRESOURCE(SHELL_ICON_ID));
HICON hSmallShellIcon = ::LoadIcon(hShell32, MAKEINTRESOURCE(SHELL_ICON_ID));
#endif

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Settings(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
VOID                ShowContextMenu(HWND);
VOID                TerminateApplication(HWND);
BOOL                InitWorker();


int APIENTRY wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	// Detect already running instance
	hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE,
		L"Global\\SajtkukacSingleInstance");
	if (hMutex != nullptr) return 1;
	hMutex = ::CreateMutex(nullptr, FALSE,
		L"Global\\SajtkukacSingleInstance");
	SCOPE_EXIT{ ::ReleaseMutex(hMutex); };

	// Initialize globals
	::LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	::LoadString(hInstance, IDC_SAJTKUKAC, szWindowClass, MAX_LOADSTRING);
	::MyRegisterClass(hInstance);
	if (::wcslen(lpCmdLine) > 0)
	{
		if (INT ret = ::_wtoi(lpCmdLine); ret < 0 || 100 < ret)
		{
			uPercentage = 50;
		}
		else
		{
			uPercentage = ret;
		}
		if (auto ptr = ::wcschr(lpCmdLine, L' '); ptr != nullptr)
		{
			if (INT ret = ::_wtoi(ptr); ret < 50 || 10000 < ret)
			{
				uRefreshRate = 500;
			}
			else
			{
				uRefreshRate = ret;
			}
		}
	}
	else
	{
		uPercentage = 50;
		uRefreshRate = 500;
	}

	// Perform application initialization:
	if (!::InitInstance(hInstance, nCmdShow) || !::InitWorker())
	{
		return 1;
	}

	HACCEL hAccelTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SAJTKUKAC));
	MSG msg;

	// Main message loop:
	while (::GetMessage(&msg, nullptr, 0, 0))
	{
		if (!::TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
#ifdef IDI_SAJTKUKAC
	wcex.hIcon = ::LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SAJTKUKAC));
	wcex.hIconSm = ::LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
#else
	wcex.hIcon = hShellIcon;
	wcex.hIconSm = hSmallShellIcon;
#endif
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SAJTKUKAC);
	wcex.lpszClassName = szWindowClass;

	return ::RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	UNREFERENCED_PARAMETER(nCmdShow);

	hInst = hInstance; // Store instance handle in our global variable

	HWND hWnd = ::CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	nidApp.cbSize = sizeof(NOTIFYICONDATA);

#ifdef IDI_SAJTKUKAC
	HICON hMainIcon = ::LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_SAJTKUKAC));
	nidApp.uID = IDI_SAJTKUKAC;
	nidApp.hIcon = hMainIcon;
#else
	nidApp.uID = SHELL_ICON_ID;
	nidApp.hIcon = hShellIcon;
#endif

	nidApp.hWnd = hWnd;
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nidApp.uCallbackMessage = WM_USER_SHELLICON;
	::LoadString(hInstance, IDS_APP_TITLE, nidApp.szTip, MAX_LOADSTRING);
	::Shell_NotifyIcon(NIM_ADD, &nidApp);

	return TRUE;
}

BOOL InitWorker()
{
	return Init(nidApp.hWnd, &uPercentage, &uRefreshRate, &wdDetails);
}

VOID ShowContextMenu(HWND hWnd)
{
	hPopMenu = ::CreatePopupMenu();

#define MAKEMENUITEM(a, b, c) do {             \
	WCHAR m[MAX_LOADSTRING];                   \
	::LoadString(hInst, a, m, MAX_LOADSTRING); \
	BSTR m_bstr = SysAllocString(m);           \
	::InsertMenu(hPopMenu, -1, b, c, m_bstr);  \
	::SysFreeString(m_bstr);                   \
} while(FALSE)

	MAKEMENUITEM(IDS_TRAY_SETTINGS,  MF_BYPOSITION | MF_STRING, IDM_SETTINGS);
	MAKEMENUITEM(IDS_TRAY_RELOAD,    MF_BYPOSITION | MF_STRING, IDM_RELOAD);
	MAKEMENUITEM(IDS_TRAY_SEPARATOR, MF_SEPARATOR,              IDM_SEP);
	MAKEMENUITEM(IDS_TRAY_ABOUT,     MF_BYPOSITION | MF_STRING, IDM_ABOUT);
	MAKEMENUITEM(IDS_TRAY_SEPARATOR, MF_SEPARATOR,              IDM_SEP);
	MAKEMENUITEM(IDS_TRAY_EXIT,      MF_BYPOSITION | MF_STRING, IDM_EXIT);

#undef MAKEMENU

	::SetForegroundWindow(hWnd);

	POINT lpClickPoint;
	::GetCursorPos(&lpClickPoint);
	::TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN,
		lpClickPoint.x, lpClickPoint.y, 0, hWnd, nullptr);
}

VOID TerminateApplication(HWND hWnd)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (::Process32First(snapshot, &entry) == TRUE)
	{
		while (::Process32Next(snapshot, &entry) == TRUE)
		{
			if (::wcscmp(L"explorer.exe", entry.szExeFile) != 0) continue;
			HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
			::TerminateProcess(hProcess, 1);
			::CloseHandle(hProcess);
		}
	}

	CloseHandle(snapshot);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	::ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	::ZeroMemory(&pi, sizeof(pi));
	WCHAR command[MAX_LOADSTRING];
	::LoadString(hInst, IDS_EXPLORER_COMMAND, command, MAX_LOADSTRING);
	::CreateProcess(nullptr, command, nullptr, nullptr,
		FALSE, 0, nullptr, nullptr, &si, &pi);

	::Shell_NotifyIcon(NIM_DELETE, &nidApp);
	::DestroyWindow(hWnd);
}

VOID ShowSuccessBalloon(VOID)
{
	NOTIFYICONDATA data;

	data.cbSize = sizeof(data);

	data.hWnd = nidApp.hWnd;
#ifdef IDI_SAJTKUKAC
	data.uID = IDI_SAJTKUKAC;
#else
	data.uID = SHELL_ICON_ID;
#endif
	data.uFlags = NIF_INFO;
	data.dwInfoFlags = NIIF_INFO;
	data.uTimeout = 10000;

	::LoadString(hInst, IDS_APP_TITLE, data.szInfoTitle, MAX_LOADSTRING);
	::LoadString(hInst, IDS_WORKER_RELOADED, data.szInfo, MAX_LOADSTRING);

	::Shell_NotifyIcon(NIM_MODIFY, &data);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_USER_SHELLICON:
	{
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONDOWN:
			::ShowContextMenu(hWnd);
			return TRUE;
		}
	}
	break;
	case WM_USER_WORKER:
	{
		switch (wParam)
		{
		case WM_USER_WORKER_FAILURE:
		{
			WCHAR buffer[256];
			::StringCbPrintf(buffer, sizeof(buffer),
				L"Worker thread error 0x%X", (HRESULT)lParam);
			::MessageBox(hWnd, buffer, L"Sajtkukac - Critical Error",
				MB_ICONERROR | MB_TOPMOST | MB_SYSTEMMODAL);
			::TerminateApplication(hWnd);
		}
		break;
		case WM_USER_WORKER_RELOAD:
			if (::InitWorker())
			{
				::ShowSuccessBalloon();
			}
			else
			{
				::MessageBox(hWnd, L"Failed to reload the worker thread",
					L"Sajtkukac - Critical Error",
					MB_ICONERROR | MB_TOPMOST | MB_SYSTEMMODAL);
				::TerminateApplication(hWnd);
			}
			break;
		}
	}
	break;
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_SETTINGS:
			::DialogBox(hInst, MAKEINTRESOURCE(IDD_SETTINGSBOX), hWnd, Settings);
			break;
		case IDM_RELOAD:
			if (wdDetails->reload) break;
			wdDetails->reload = TRUE;
			break;
		case IDM_ABOUT:
			::DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			::TerminateApplication(hWnd);
			break;
		default:
			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
	default:
		return ::DefWindowProc(hWnd, message, wParam, lParam);
	}
	return FALSE;
}

// Message handler for settings box.
INT_PTR CALLBACK Settings(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	static BOOL bInitialized = FALSE;
	if (!bInitialized && message != WM_INITDIALOG) return (INT_PTR)FALSE;
	switch (message)
	{
	case WM_INITDIALOG:
		bInitialized = TRUE;
		::SendDlgItemMessage(hDlg, IDC_PERCENTAGE_SPIN,
			UDM_SETRANGE, 0, MAKELPARAM(100, 0));
		::SendDlgItemMessage(hDlg, IDC_REFRESH_SPIN,
			UDM_SETRANGE, 0, MAKELPARAM(10000, 50));
		::SendDlgItemMessage(hDlg, IDC_PERCENTAGE_SLIDER,
			TBM_SETRANGE, 0, MAKELONG(0, 100));
		::SendDlgItemMessage(hDlg, IDC_PERCENTAGE_SLIDER,
			TBM_SETPAGESIZE, 0, 1);
		::SendDlgItemMessage(hDlg, IDC_PERCENTAGE_SLIDER,
			TBM_SETPOS, TRUE, uPercentage);
		::SetDlgItemInt(hDlg, IDC_PERCENTAGE_EDIT,
			uPercentage, TRUE);
		::SetDlgItemInt(hDlg, IDC_REFRESH_EDIT,
			uRefreshRate, TRUE);
		return (INT_PTR)TRUE;
	case WM_COMMAND:
	{
		WORD wNotifyCode = HIWORD(wParam);
		WORD wID = LOWORD(wParam);
		HWND hwndCtl = (HWND)lParam;
		switch (wID)
		{
		case IDC_PERCENTAGE_EDIT:
		{
			if (wNotifyCode != EN_CHANGE) break;
			BOOL success;
			WORD percentage = ::GetDlgItemInt(hDlg, wID, &success, FALSE);
			if (success && 0 <= percentage && percentage <= 100)
			{
				::SendDlgItemMessage(hDlg, IDC_PERCENTAGE_SLIDER,
					TBM_SETPOS, TRUE, uPercentage = percentage);
			}
			break;
		}
		break;
		case IDC_REFRESH_EDIT:
		{
			if (wNotifyCode != EN_CHANGE) break;
			BOOL success;
			WORD refreshRate = ::GetDlgItemInt(hDlg, wID, &success, FALSE);
			if (success && 50 <= refreshRate && refreshRate <= 10000)
			{
				uRefreshRate = refreshRate;
			}
			break;
		}
		break;
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, 0);
		}
	}
	break;
	case WM_HSCROLL:
	{
		if (::GetDlgItem(hDlg, IDC_PERCENTAGE_SLIDER) != (HWND)lParam)
			break;
		::SetDlgItemInt(hDlg, IDC_PERCENTAGE_EDIT, uPercentage =
			::SendDlgItemMessage(hDlg, IDC_PERCENTAGE_SLIDER,
				TBM_GETPOS, 0, 0) & 0xFF, FALSE);
	}
	break;
	case WM_DESTROY:
		bInitialized = FALSE;
	}
	return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
	{
		WORD wNotifyCode = HIWORD(wParam);
		WORD wID = LOWORD(wParam);
		HWND hwndCtl = (HWND)lParam;
		switch (wID)
		{
		case IDC_BUTTON1:
			::ShellExecute(hDlg, nullptr,
				L"https://github.com/friendlyanon/Sajtkukac#donation",
				nullptr, nullptr, SW_SHOW);
			break;
		case IDC_BUTTON2:
			::ShellExecute(hDlg, nullptr,
				L"https://github.com/friendlyanon/Sajtkukac",
				nullptr, nullptr, SW_SHOW);
			break;
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
	}
	break;
	}
	return (INT_PTR)FALSE;
}
