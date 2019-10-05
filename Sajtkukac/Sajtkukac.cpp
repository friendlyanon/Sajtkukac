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
#include "Easing.h"
#include "simple_match/simple_match.hpp"

namespace simple_match::customization {
	template<>
	struct tuple_adapter<RECT> {
		enum { tuple_len = 4 };

		template<size_t I, class T>
		static auto get(T&& t) {
			return std::get<I>(std::tie(t.left, t.top, t.right, t.bottom));
		}
	};
}

using TRAYMAP = std::unordered_map<
	IUIAutomationElement*,
	IUIAutomationElement*>;

static DWORD dwThreadID = 0;
static HANDLE hThread = nullptr;
static Z_PTR(IUIAutomation, g_pUI);
static Z_PTR(IUIAutomationTreeWalker, g_pControlWalker);
static BSTR g_bstrs[3] = {
	::SysAllocString(L"Shell_TrayWnd"),
	::SysAllocString(L"Shell_SecondaryTrayWnd"),
	::SysAllocString(L"MSTaskListWClass")
};

HRESULT GetWidthOfChildren(BOOL vertical, IUIAutomationElement *taskList, UINT &result)
{
	static const long buttonVariants[] = {
		UIA_ButtonControlTypeId,
		UIA_MenuItemControlTypeId
	};

	IUIAutomationCondition* conditions[2]{ nullptr, nullptr };
	SCOPE_EXIT{ for (int i = 0; i < 2; ++i) RELEASE(conditions[i]); };
	for (int i = 0; i < 2; ++i)
	{
		Z_PTR(IUIAutomationCondition, prop);
		CHK_HR(g_pUI->CreatePropertyCondition(
			UIA_ControlTypePropertyId, CComVariant{ buttonVariants[i] },
			&prop));
		conditions[i] = prop;
	}

	CComPtr<IUIAutomationCondition> orCondition;
	CHK_HR(g_pUI->CreateOrConditionFromNativeArray(
		conditions, 2, &orCondition));

	CComPtr<IUIAutomationElementArray> children;
	CHK_HR(taskList->FindAll(TreeScope_Children,
		orCondition, &children));

	INT length;
	CHK_HR(children->get_Length(&length));

	if (length <= 0) return S_OK;

	CComPtr<IUIAutomationElement> first;
	CHK_HR(children->GetElement(0, &first));
	RECT fRect;
	CHK_HR(first->get_CurrentBoundingRectangle(&fRect));

	CComPtr<IUIAutomationElement> last;
	CHK_HR(children->GetElement(length - 1, &last));
	RECT lRect;
	CHK_HR(last->get_CurrentBoundingRectangle(&lRect));

	result = vertical
		? lRect.bottom - fRect.top
		: lRect.right - fRect.left;

	return S_OK;
}

static constexpr INT animationIterations = 30;
static constexpr INT animationWait = 10;
static constexpr DOUBLE animationDuration =
	(DOUBLE)(animationIterations * animationWait);

HRESULT MoveTaskList(BOOL vertical, UINT percentage, UINT easingIdx,
	RECT dRect, IUIAutomationElement *taskList)
{
	RECT tRect;
	CHK_HR(taskList->get_CurrentBoundingRectangle(&tRect));
	UINT width = 0;
	CHK_HR(::GetWidthOfChildren(vertical, taskList, width));

	const DOUBLE dPerc = (DOUBLE)percentage / 100.0;
	const DOUBLE desktopPoint = (vertical ? dRect.bottom : dRect.right) * dPerc;
	const DOUBLE taskListPoint = width * dPerc;

	RECT parentRect;
	{
		CComPtr<IUIAutomationTreeWalker> walker;
		CHK_HR(g_pUI->get_ControlViewWalker(&walker));
		CComPtr<IUIAutomationElement> parent;
		CHK_HR(walker->GetParentElement(taskList, &parent));
		CHK_HR(parent->get_CurrentBoundingRectangle(&parentRect));
	}

	const DOUBLE parentStart = vertical ? parentRect.top : parentRect.left;
	const DOUBLE startValue = (vertical ? tRect.top : tRect.left) - parentStart;
	const DOUBLE difference = desktopPoint - taskListPoint - startValue -
		parentStart;

	if (::abs(difference) < 5.0) return S_OK;

	UIA_HWND hWnd;
	CHK_HR(taskList->get_CurrentNativeWindowHandle(&hWnd));

	const auto posSetter = [h = (HWND)hWnd, f = 0x4415u, vertical]
		(INT x, INT y = 0) noexcept -> HRESULT
	{
		if (vertical) ::std::swap(x, y);
		return ::SetWindowPos(h, nullptr, x, y, 0, 0, f)
			? S_OK
			: ::GetLastError();
	};

	const DOUBLE tUnit = 1.0 / animationIterations * animationDuration;
	DOUBLE currentTime = 0.0;
	const auto easeFn = easingFunctions[easingIdx].second;
	
	for (int i = 0; i < animationIterations; ++i, currentTime += tUnit)
	{
		const DOUBLE step = easeFn(
			currentTime, startValue, difference, animationDuration);
		CHK_HR(posSetter(step));
		::Sleep(animationWait);
	}
	CHK_HR(posSetter(desktopPoint - taskListPoint - parentStart));

	return S_OK;
}

HRESULT PopulateTrayMap(IUIAutomationElement *pDesktop, TRAYMAP &pTrayMap)
{
	IUIAutomationCondition *conditions[2]{ nullptr, nullptr };
	SCOPE_EXIT{ for (int i = 0; i < 2; ++i) RELEASE(conditions[i]); };
	for (int i = 0; i < 2; ++i)
	{
		Z_PTR(IUIAutomationCondition, prop);
		CHK_HR(g_pUI->CreatePropertyCondition(
			UIA_ClassNamePropertyId, CComVariant{ g_bstrs[i] },
			&prop));
		conditions[i] = prop;
	}

	CComPtr<IUIAutomationCondition> orCondition;
	CHK_HR(g_pUI->CreateOrConditionFromNativeArray(
		conditions, 2, &orCondition));

	CComPtr<IUIAutomationElementArray> sysTrays;
	CHK_HR(pDesktop->FindAll(TreeScope_Children,
		orCondition, &sysTrays));
	{
		int length;
		CHK_HR(sysTrays->get_Length(&length));
		if (!length) return S_OK;
		for (int i = 0; i < length; ++i)
		{
			Z_PTR(IUIAutomationElement, sysTray);
			CHK_HR(sysTrays->GetElement(i, &sysTray));

			Z_PTR(IUIAutomationElement, taskList);
			CComPtr<IUIAutomationCondition> prop;
			CHK_HR(g_pUI->CreatePropertyCondition(
				UIA_ClassNamePropertyId, CComVariant{ g_bstrs[2] },
				&prop));
			CHK_HR(sysTray->FindFirst(TreeScope_Descendants,
				prop, &taskList));
			pTrayMap[sysTray] = taskList;
		}
	}

	return S_OK;
}

HRESULT SajtkukacImpl(UINT percentage, UINT easingIdx)
{
	CComPtr<IUIAutomationElement> pDesktop;
	CHK_HR(g_pUI->GetRootElement(&pDesktop));

	TRAYMAP trayMap;
	SCOPE_EXIT{
		for (auto [sysTray, taskList] : trayMap)
		{
			RELEASE(sysTray);
			RELEASE(taskList);
		}
		trayMap.clear();
	};
	CHK_HR(::PopulateTrayMap(pDesktop, trayMap));

	RECT dRect;
	CHK_HR(pDesktop->get_CurrentBoundingRectangle(&dRect));

	for (auto [sysTray, taskList] : trayMap)
	{
		using namespace simple_match::placeholders;

		RECT sRect;
		CHK_HR(sysTray->get_CurrentBoundingRectangle(&sRect));

		enum
		{
			UNKNOWN,
			BOTTOM,
			RIGHT,
			LEFT,
			TOP
		} pos = ::simple_match::match(dRect,
			ds(_, sRect.top, sRect.right, sRect.bottom),
			[] { return RIGHT; },
			ds(sRect.left, _, sRect.right, sRect.bottom),
			[] { return BOTTOM; },
			ds(sRect.left, sRect.top, _, sRect.bottom),
			[] { return LEFT; },
			ds(sRect.left, sRect.top, sRect.right, _),
			[] { return TOP; },
			// TODO: there might be unusual setups?
			otherwise,
			[] { return UNKNOWN; }
		);

		const auto vertical = pos == LEFT || pos == RIGHT
			? TRUE
			: FALSE;
		::MoveTaskList(vertical, percentage, easingIdx, dRect, taskList);
	}
	return S_OK;
}

HRESULT CreateControlWalker(VOID)
{
	Z_PTR(IUIAutomationCondition, propCond);
	CHK_HR(g_pUI->CreatePropertyCondition(
		UIA_IsControlElementPropertyId, CComVariant{ false }, &propCond));
	Z_PTR(IUIAutomationCondition, notCondition);
	CHK_HR(g_pUI->CreateNotCondition(propCond, &notCondition));
	CHK_HR(g_pUI->CreateTreeWalker(notCondition, &g_pControlWalker));
	return S_OK;
}

HRESULT SajtkukacInit(VOID)
{
	CHK_HR(::CoInitialize(nullptr));
	CHK_HR(::CoCreateInstance(__uuidof(CUIAutomation),
		nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation),
		(LPVOID *)&g_pUI));
	CHK_HR(::CreateControlWalker());
	return S_OK;
}

DWORD WINAPI Sajtkukac(PVOID pParam)
{
	auto details = (WORKER_DETAILS **)pParam;
	HWND hWnd = (*details)->hWnd;
	SCOPE_EXIT{
		delete *details;
		*details = nullptr;
		::PostMessage(hWnd, WM_USER_WORKER,
			WM_USER_WORKER_RELOAD, 0);
	};

	HRESULT returnCode = S_OK;
	SCOPE_EXIT{
		if (returnCode == S_OK) return;
		::PostMessage(hWnd, WM_USER_WORKER,
			WM_USER_WORKER_FAILURE, returnCode);
	};

	CHK_HR(returnCode = ::SajtkukacInit());
	SCOPE_EXIT{
		RELEASE(g_pControlWalker);
		g_pControlWalker = nullptr;
		RELEASE(g_pUI);
		g_pUI = nullptr;
		::CoUninitialize();
	};

	volatile auto reload = &(*details)->reload;
	volatile auto refreshRate = (*details)->refreshRate;
	volatile auto i = (*details)->easingIdx;
	for (volatile auto p = (*details)->percentage;
		!*reload && SUCCEEDED(returnCode = ::SajtkukacImpl(*p, *i));
		::Sleep(*refreshRate));

	return returnCode;
}

BOOL Init(HWND hWnd, UINT &pPercentage, UINT &pRefreshRate,
	UINT &easingIdx, PWORKER_DETAILS &wdDetails)
{
	if (wdDetails != nullptr)
	{
		if (!wdDetails->reload) return FALSE;
		if (!::CloseHandle(hThread))
		{
			return FALSE;
		}
	}
	
	auto ptr = new (::std::nothrow) WORKER_DETAILS{
		&pPercentage, &pRefreshRate, &easingIdx, hWnd };
	if (ptr == nullptr) return FALSE;
	wdDetails = ptr;
	auto fn = (LPTHREAD_START_ROUTINE)::Sajtkukac;
	hThread = ::CreateThread(0, 0, fn, &wdDetails, 0, &dwThreadID);

	return hThread != nullptr ? TRUE : FALSE;
}
