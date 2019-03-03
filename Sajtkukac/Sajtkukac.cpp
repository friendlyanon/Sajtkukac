/* Sajtkukac - customizes the position of icons on your Windows taskbar
 * Copyright (C) 2019 friendlyanon <skype1234@waifu.club>
 * Sajtkukac.cpp is part of Sajtkukac.
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
#include "Sajtkukac.h"
#include "scopeexit/ScopeExit.h"

namespace simple_match::customization {
	template<>
	struct tuple_adapter<RECT> {
		enum { tuple_len = 4 };

		template<size_t I, class T>
		static decltype(auto) get(T&& t) {
			return std::get<I>(std::tie(t.left, t.top, t.right, t.bottom));
		}
	};
}

enum class TrayPosition
{
	UNKNOWN,
	BOTTOM,
	RIGHT,
	LEFT,
	TOP
};

// return if result is not null
#define _C(x) if (auto r = x; r) return r
// release if x is not nullptr
#define _R(x) if (auto r = x; r != nullptr) r->Release()
// declare variable x with type y and init with nullptr
#define _P(y, x) y * x = nullptr
// release x on scope exit using RAII
#define _E(x) SCOPE_EXIT{ if (x != nullptr && !x->Release()) x = nullptr; }
// declare scoped variable with release scope exit function
#define _S(x, y) _P(x, y); _E(y)

typedef std::unordered_map<
	IUIAutomationElement*,
	IUIAutomationElement*> TRAYMAP;

DWORD dwThreadID;
HANDLE hThread = nullptr;
_P(IUIAutomation, g_pUI);
_P(IUIAutomationTreeWalker, g_pControlWalker);
BSTR g_bstrs[3] = {
	SysAllocString(L"Shell_TrayWnd"),
	SysAllocString(L"Shell_SecondaryTrayWnd"),
	SysAllocString(L"MSTaskListWClass")
};

HRESULT GetWidthOfChildren(BOOL vertical, IUIAutomationElement *taskList, PUINT result)
{
	_S(IUIAutomationCondition, buttonCond);
	{
		VARIANT var{ VT_I4 };
		var.lVal = UIA_ButtonControlTypeId;
		_C(g_pUI->CreatePropertyCondition(UIA_ControlTypePropertyId,
			var, &buttonCond));
	}

	_S(IUIAutomationElementArray, children);
	_C(taskList->FindAll(TreeScope_Children,
		buttonCond, &children));

	INT length;
	_C(children->get_Length(&length));

	if (length <= 0) return S_OK;

	_S(IUIAutomationElement, first);
	_C(children->GetElement(0, &first));
	RECT fRect;
	_C(first->get_CurrentBoundingRectangle(&fRect));

	_S(IUIAutomationElement, last);
	_C(children->GetElement(length - 1, &last));
	RECT lRect;
	_C(last->get_CurrentBoundingRectangle(&lRect));

	*result = vertical
		? lRect.bottom - fRect.top
		: lRect.right - fRect.left;

	return S_OK;
}

#define ANIMATION_ITERATIONS (15)
#define ANIMATION_WAIT (10)
#define ANIMATION_DURATION (ANIMATION_ITERATIONS * ANIMATION_WAIT)
#define POS_SETTER(x, y) \
	if (auto r = ::SetWindowPos((HWND)hWnd, nullptr, \
		(INT)x, (INT)y, 0, 0, flags); r == 0) \
		return ::GetLastError();
	
HRESULT MoveTaskList(BOOL vertical, UINT percentage,
	RECT dRect, IUIAutomationElement *taskList)
{
	RECT tRect;
	_C(taskList->get_CurrentBoundingRectangle(&tRect));
	UINT width = 0;
	_C(::GetWidthOfChildren(vertical, taskList, &width));

	DOUBLE dPerc = percentage / 100.0;
	DOUBLE dTarget = (vertical ? dRect.bottom : dRect.right) * dPerc;
	DOUBLE tTarget = width * dPerc;

	DOUBLE b = vertical ? tRect.top : tRect.left;
	DOUBLE offset = b + tTarget;
	DOUBLE distance = dTarget - offset;

	if (::abs(distance) < 5.0) return S_OK;

	UIA_HWND hWnd;
	_C(taskList->get_CurrentNativeWindowHandle(&hWnd));

	UINT flags = SWP_ASYNCWINDOWPOS | SWP_NOSENDCHANGING |
		SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE;

	// TODO: SetWindowPos is super slow, can't animate. What gives?
#if (0)
	for (int i = 0; i < ANIMATION_ITERATIONS; ++i)
	{
		DOUBLE t = i / ANIMATION_ITERATIONS * 10;
		DOUBLE step = ::EaseOutBack(t, b, 1, ANIMATION_DURATION);
		if (vertical)
		{
			POS_SETTER(0, step);
		}
		else
		{
			POS_SETTER(step, 0);
		}
		Sleep(ANIMATION_WAIT);
	}
#else
	if (vertical)
	{
		POS_SETTER(0, dTarget - tTarget);
	}
	else
	{
		POS_SETTER(dTarget - tTarget, 0);
	}
#endif

	return S_OK;
}

HRESULT PopulateTrayMap(IUIAutomationElement *pDesktop, TRAYMAP *pTrayMap)
{
	IUIAutomationCondition *conditions[2]{ nullptr, nullptr };
	SCOPE_EXIT{ for (int i = 0; i < 2; ++i) _R(conditions[i]); };
	for (int i = 0; i < 2; ++i)
	{
		_P(IUIAutomationCondition, prop);
		VARIANT var{ VT_BSTR };
		var.bstrVal = g_bstrs[i];
		_C(g_pUI->CreatePropertyCondition(
			UIA_ClassNamePropertyId, var, &prop));
		conditions[i] = prop;
	}

	_S(IUIAutomationCondition, orCondition);
	_C(g_pUI->CreateOrConditionFromNativeArray(
		conditions, 2, &orCondition));

	_S(IUIAutomationElementArray, sysTrays);
	_C(pDesktop->FindAll(TreeScope_Children,
		orCondition, &sysTrays));
	{
		int length;
		_C(sysTrays->get_Length(&length));
		if (!length) return S_OK;
		for (int i = 0; i < length; ++i)
		{
			_P(IUIAutomationElement, sysTray);
			_C(sysTrays->GetElement(i, &sysTray));

			_P(IUIAutomationElement, taskList);
			_S(IUIAutomationCondition, prop);
			{
				VARIANT var{ VT_BSTR };
				var.bstrVal = g_bstrs[2];
				_C(g_pUI->CreatePropertyCondition(
					UIA_ClassNamePropertyId, var, &prop));
			}
			_C(sysTray->FindFirst(TreeScope_Descendants,
				prop, &taskList));
			(*pTrayMap)[sysTray] = taskList;
		}
	}
	return S_OK;
}

HRESULT SajtkukacImpl(UINT percentage)
{
	using namespace simple_match;
	using namespace simple_match::placeholders;

	_S(IUIAutomationElement, pDesktop);
	_C(g_pUI->GetRootElement(&pDesktop));

	TRAYMAP trayMap;
	SCOPE_EXIT{
		for (auto [sysTray, taskList] : trayMap)
		{
			_R(sysTray);
			_R(taskList);
		}
		trayMap.clear();
	};
	_C(::PopulateTrayMap(pDesktop, &trayMap));

	RECT dRect;
	_C(pDesktop->get_CurrentBoundingRectangle(&dRect));

	for (auto [sysTray, taskList] : trayMap)
	{
		RECT sRect;
		_C(sysTray->get_CurrentBoundingRectangle(&sRect));

		TrayPosition pos = match(dRect,
			ds(_, sRect.top, sRect.right, sRect.bottom),
			[] { return TrayPosition::RIGHT; },
			ds(sRect.left, _, sRect.right, sRect.bottom),
			[] { return TrayPosition::BOTTOM; },
			ds(sRect.left, sRect.top, _, sRect.bottom),
			[] { return TrayPosition::LEFT; },
			ds(sRect.left, sRect.top, sRect.right, _),
			[] { return TrayPosition::TOP; },
			// TODO: there might be unusual setups?
			otherwise,
			[] { return TrayPosition::UNKNOWN; }
		);

		switch (pos)
		{
		case TrayPosition::LEFT:
		case TrayPosition::RIGHT:
			_C(::MoveTaskList(TRUE, percentage, dRect, taskList));
			break;
		case TrayPosition::TOP:
		case TrayPosition::BOTTOM:
			_C(::MoveTaskList(FALSE, percentage, dRect, taskList));
			break;
		}
	}
	return S_OK;
}

HRESULT CreateControlWalker(VOID)
{
	_P(IUIAutomationCondition, propCond);
	{
		VARIANT var{ VT_BOOL };
		var.boolVal = FALSE;
		_C(g_pUI->CreatePropertyCondition(
			UIA_IsControlElementPropertyId, var, &propCond));
	}
	_P(IUIAutomationCondition, notCondition);
	_C(g_pUI->CreateNotCondition(propCond, &notCondition));
	_C(g_pUI->CreateTreeWalker(notCondition, &g_pControlWalker));
	return S_OK;
}

HRESULT SajtkukacInit(VOID)
{
	_C(::CoInitialize(nullptr));
	_C(::CoCreateInstance(__uuidof(CUIAutomation),
		nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation),
		(LPVOID *)&g_pUI));
	_C(::CreateControlWalker());
	return S_OK;
}

DWORD WINAPI Sajtkukac(PVOID pParam)
{
	auto details = (WORKER_DETAILS **)pParam;
	HWND hWnd = (*details)->hWnd;
	SCOPE_EXIT{
		delete *details;
		*details = nullptr;
		::SendMessage(hWnd, WM_USER_WORKER,
			WM_USER_WORKER_RELOAD, 0);
	};

	HRESULT returnCode = S_OK;
	SCOPE_EXIT{
		if (returnCode == S_OK) return;
		::SendMessage(hWnd, WM_USER_WORKER,
			WM_USER_WORKER_FAILURE, returnCode);
	};

	_C(returnCode = ::SajtkukacInit());
	SCOPE_EXIT{
		_R(g_pControlWalker);
		g_pControlWalker = nullptr;
		_R(g_pUI);
		g_pUI = nullptr;
		::CoUninitialize();
	};

	volatile auto reload = &(*details)->reload;
	volatile auto refreshRate = (*details)->refreshRate;
	for (volatile auto p = (*details)->percentage;
		!*reload && !(returnCode = ::SajtkukacImpl(*p));
		::Sleep(*refreshRate));

	return returnCode;
}

BOOL Init(HWND hWnd, PUINT pPercentage, PUINT pRefreshRate, WORKER_DETAILS **wdDetails)
{
	if (*wdDetails != nullptr)
	{
		if (!(*wdDetails)->reload) return FALSE;
		if (!::CloseHandle(hThread))
		{
			return FALSE;
		}
	}
	
	*wdDetails = new (std::nothrow) WORKER_DETAILS{ pPercentage, pRefreshRate, hWnd };
	hThread = ::CreateThread(0, 0, (LPTHREAD_START_ROUTINE)::Sajtkukac,
		wdDetails, 0, &dwThreadID);

	return hThread ? TRUE : FALSE;
}
