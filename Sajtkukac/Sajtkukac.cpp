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

#if HAS_EASING_OUT_BACK
#include "easing/Easing.h"
#endif

namespace simple_match::customization {
	template<>
	struct tuple_adapter<RECT> {
		enum { tuple_len = 4 };

		template<size_t I, class T>
		static decltype(auto) get(T&& t) {
			return ::std::get<I>(::std::tie(t.left, t.top, t.right, t.bottom));
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
#define CHECK_HR(x) if (auto r = x; r) return r
// release if x is not nullptr
#define RELEASE(x) if (auto r = x; r != nullptr) r->Release()
// declare pointer and init with nullptr
#define DECLARE_PTR(type, id) \
	type * id = nullptr
// declare scoped pointer with release scope exit function
#define SCOPED_PTR(type, id) DECLARE_PTR(type, id); \
	SCOPE_EXIT{ if (id != nullptr && !id->Release()) id = nullptr; }

typedef ::std::unordered_map<
	IUIAutomationElement*,
	IUIAutomationElement*> TRAYMAP;

DWORD dwThreadID;
HANDLE hThread = nullptr;
DECLARE_PTR(IUIAutomation, g_pUI);
DECLARE_PTR(IUIAutomationTreeWalker, g_pControlWalker);
BSTR g_bstrs[3] = {
	SysAllocString(L"Shell_TrayWnd"),
	SysAllocString(L"Shell_SecondaryTrayWnd"),
	SysAllocString(L"MSTaskListWClass")
};

HRESULT GetWidthOfChildren(BOOL vertical, IUIAutomationElement *taskList, PUINT result)
{
	SCOPED_PTR(IUIAutomationCondition, buttonCond);
	{
		VARIANT var{ VT_I4 };
		var.lVal = UIA_ButtonControlTypeId;
		CHECK_HR(g_pUI->CreatePropertyCondition(UIA_ControlTypePropertyId,
			var, &buttonCond));
	}

	SCOPED_PTR(IUIAutomationElementArray, children);
	CHECK_HR(taskList->FindAll(TreeScope_Children,
		buttonCond, &children));

	INT length;
	CHECK_HR(children->get_Length(&length));

	if (length <= 0) return S_OK;

	SCOPED_PTR(IUIAutomationElement, first);
	CHECK_HR(children->GetElement(0, &first));
	RECT fRect;
	CHECK_HR(first->get_CurrentBoundingRectangle(&fRect));

	SCOPED_PTR(IUIAutomationElement, last);
	CHECK_HR(children->GetElement(length - 1, &last));
	RECT lRect;
	CHECK_HR(last->get_CurrentBoundingRectangle(&lRect));

	*result = vertical
		? lRect.bottom - fRect.top
		: lRect.right - fRect.left;

	return S_OK;
}

static constexpr INT animationIterations = 30;
static constexpr INT animationWait = 10;
static constexpr DOUBLE animationDuration = animationIterations * animationWait;

HRESULT MoveTaskList(BOOL vertical, UINT percentage,
	RECT dRect, IUIAutomationElement *taskList)
{
	RECT tRect;
	CHECK_HR(taskList->get_CurrentBoundingRectangle(&tRect));
	UINT width = 0;
	CHECK_HR(::GetWidthOfChildren(vertical, taskList, &width));

	const DOUBLE dPerc = percentage / 100.0;
	const DOUBLE desktopPoint = (vertical ? dRect.bottom : dRect.right) * dPerc;
	const DOUBLE taskListPoint = width * dPerc;

	const DOUBLE startValue = vertical ? tRect.top : tRect.left;
	const DOUBLE difference = desktopPoint - startValue - taskListPoint;

	if (::abs(difference) < 5.0) return S_OK;

	UIA_HWND hWnd;
	CHECK_HR(taskList->get_CurrentNativeWindowHandle(&hWnd));

	auto posSetter = [h = (HWND)hWnd, f = 0x4415u, vertical]
		(DOUBLE x, DOUBLE y) noexcept -> HRESULT
	{
		if (vertical) ::std::swap(x, y);
		if (auto r = ::SetWindowPos(h, nullptr,
			(INT)x, (INT)y, 0, 0, f); r == 0)
			return ::GetLastError();
		return S_OK;
	};

#if HAS_EASING_OUT_BACK
	const DOUBLE tUnit = 1.0 / animationIterations * animationDuration;
	DOUBLE currentTime = 0.0;
	for (int i = 0; i < animationIterations; ++i, currentTime += tUnit)
	{
		const DOUBLE step = ::EaseOutBack(
			currentTime, startValue, difference, animationDuration);
		CHECK_HR(posSetter(step, 0));
		::Sleep(animationWait);
	}
#endif
	CHECK_HR(posSetter(desktopPoint - taskListPoint, 0));

	return S_OK;
}

HRESULT PopulateTrayMap(IUIAutomationElement *pDesktop, TRAYMAP *pTrayMap)
{
	IUIAutomationCondition *conditions[2]{ nullptr, nullptr };
	SCOPE_EXIT{ for (int i = 0; i < 2; ++i) RELEASE(conditions[i]); };
	for (int i = 0; i < 2; ++i)
	{
		DECLARE_PTR(IUIAutomationCondition, prop);
		VARIANT var{ VT_BSTR };
		var.bstrVal = g_bstrs[i];
		CHECK_HR(g_pUI->CreatePropertyCondition(
			UIA_ClassNamePropertyId, var, &prop));
		conditions[i] = prop;
	}

	SCOPED_PTR(IUIAutomationCondition, orCondition);
	CHECK_HR(g_pUI->CreateOrConditionFromNativeArray(
		conditions, 2, &orCondition));

	SCOPED_PTR(IUIAutomationElementArray, sysTrays);
	CHECK_HR(pDesktop->FindAll(TreeScope_Children,
		orCondition, &sysTrays));
	{
		int length;
		CHECK_HR(sysTrays->get_Length(&length));
		if (!length) return S_OK;
		for (int i = 0; i < length; ++i)
		{
			DECLARE_PTR(IUIAutomationElement, sysTray);
			CHECK_HR(sysTrays->GetElement(i, &sysTray));

			DECLARE_PTR(IUIAutomationElement, taskList);
			SCOPED_PTR(IUIAutomationCondition, prop);
			{
				VARIANT var{ VT_BSTR };
				var.bstrVal = g_bstrs[2];
				CHECK_HR(g_pUI->CreatePropertyCondition(
					UIA_ClassNamePropertyId, var, &prop));
			}
			CHECK_HR(sysTray->FindFirst(TreeScope_Descendants,
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

	SCOPED_PTR(IUIAutomationElement, pDesktop);
	CHECK_HR(g_pUI->GetRootElement(&pDesktop));

	TRAYMAP trayMap;
	SCOPE_EXIT{
		for (auto [sysTray, taskList] : trayMap)
		{
			RELEASE(sysTray);
			RELEASE(taskList);
		}
		trayMap.clear();
	};
	CHECK_HR(::PopulateTrayMap(pDesktop, &trayMap));

	RECT dRect;
	CHECK_HR(pDesktop->get_CurrentBoundingRectangle(&dRect));

	for (auto [sysTray, taskList] : trayMap)
	{
		RECT sRect;
		CHECK_HR(sysTray->get_CurrentBoundingRectangle(&sRect));

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
			CHECK_HR(::MoveTaskList(TRUE, percentage, dRect, taskList));
			break;
		case TrayPosition::TOP:
		case TrayPosition::BOTTOM:
			CHECK_HR(::MoveTaskList(FALSE, percentage, dRect, taskList));
			break;
		}
	}
	return S_OK;
}

HRESULT CreateControlWalker(VOID)
{
	DECLARE_PTR(IUIAutomationCondition, propCond);
	{
		VARIANT var{ VT_BOOL };
		var.boolVal = FALSE;
		CHECK_HR(g_pUI->CreatePropertyCondition(
			UIA_IsControlElementPropertyId, var, &propCond));
	}
	DECLARE_PTR(IUIAutomationCondition, notCondition);
	CHECK_HR(g_pUI->CreateNotCondition(propCond, &notCondition));
	CHECK_HR(g_pUI->CreateTreeWalker(notCondition, &g_pControlWalker));
	return S_OK;
}

HRESULT SajtkukacInit(VOID)
{
	CHECK_HR(::CoInitialize(nullptr));
	CHECK_HR(::CoCreateInstance(__uuidof(CUIAutomation),
		nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation),
		(LPVOID *)&g_pUI));
	CHECK_HR(::CreateControlWalker());
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

	CHECK_HR(returnCode = ::SajtkukacInit());
	SCOPE_EXIT{
		RELEASE(g_pControlWalker);
		g_pControlWalker = nullptr;
		RELEASE(g_pUI);
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
	
	*wdDetails = new (::std::nothrow) WORKER_DETAILS{ pPercentage, pRefreshRate, hWnd };
	hThread = ::CreateThread(0, 0, (LPTHREAD_START_ROUTINE)::Sajtkukac,
		wdDetails, 0, &dwThreadID);

	return hThread ? TRUE : FALSE;
}
