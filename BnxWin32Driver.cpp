/*-
 * Copyright (c) 2013 Nathan Lay (nslay@users.sourceforge.net)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdio>
#include <sstream>
#include "getopt.h"
#include "BnxStreams.h"
#include "BnxWin32Driver.h"
#include "Resource.h"

#include <Windows.h>
#include <ShellAPI.h>

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif // _MSC_VER

namespace {
	extern "C" void DispatchOnCheckShutdownTimer(evutil_socket_t fd, short sWhat, void *pArg) {
		((BnxWin32Driver *)pArg)->OnCheckShutdownTimer(fd, sWhat);
	}

	extern "C" DWORD DispatchRunBase(LPVOID pArg) {
		return ((BnxWin32Driver *)pArg)->RunBase();
	}


	extern "C" LRESULT OnWindowEvent(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
		BnxWin32Driver &clDriver = (BnxWin32Driver &)BnxDriver::GetInstance();
	
		switch (uMsg) {
		case WM_CREATE:
			clDriver.AddNotificationIcon(hWnd);
			break;
		case WM_DESTROY:
			clDriver.DeleteNotificationIcon(hWnd);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDM_EXIT:
				clDriver.Shutdown();
				break;
			}
			break;
		case WMAPP_NOTIFYMESSAGE:
			switch(LOWORD(lParam)) {
			case WM_RBUTTONDOWN:
			case WM_CONTEXTMENU:
				clDriver.ShowContextMenu(hWnd);
				break;
			}
			break;
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		return 0;
	}

} // end namespace

bool BnxWin32Driver::AddNotificationIcon(HWND hWnd) {
	if (hWnd == NULL)
		return false;

	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

	NOTIFYICONDATA stIconData;
	memset(&stIconData, 0, sizeof(stIconData));

	stIconData.cbSize = sizeof(stIconData);
	stIconData.hWnd = hWnd;
	stIconData.uID = 0;
	stIconData.uCallbackMessage = WMAPP_NOTIFYMESSAGE;
	strcpy(stIconData.szTip, "ircbnx");

	stIconData.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_IRCBNXICON));

	stIconData.uFlags = NIF_MESSAGE|NIF_TIP|NIF_ICON;

	if (Shell_NotifyIcon(NIM_ADD, &stIconData) == FALSE)
		return false;

	return true;
}

bool BnxWin32Driver::DeleteNotificationIcon(HWND hWnd) {
	if (hWnd == NULL)
		return false;

	NOTIFYICONDATA stIconData;
	memset(&stIconData, 0, sizeof(stIconData));

	stIconData.cbSize = sizeof(stIconData);
	stIconData.hWnd = hWnd;
	stIconData.uCallbackMessage = WMAPP_NOTIFYMESSAGE;
	stIconData.uFlags = NIF_MESSAGE;

	Shell_NotifyIcon(NIM_DELETE, &stIconData);

	return true;
}

void BnxWin32Driver::ShowContextMenu(HWND hWnd) {
	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);
	POINT stPoint;

	GetCursorPos(&stPoint);

	HMENU hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDC_IRCBNXMENU));

	if (hMenu == NULL)
		return;

	HMENU hSubMenu = GetSubMenu(hMenu, 0);

	if (hSubMenu == NULL) {
		DestroyMenu(hMenu);
		return;
	}

	SetForegroundWindow(hWnd);

	UINT uiFlags = TPM_CENTERALIGN | TPM_LEFTBUTTON;

	TrackPopupMenuEx(hSubMenu, uiFlags, stPoint.x, stPoint.y, hWnd, NULL);

	DestroyMenu(hMenu);
}

bool BnxWin32Driver::Run() {
	if (!MakeWindow()) {
		BnxErrorStream << "Error: Could not create window." << BnxEndl;
		return false;
	}

	m_bRun = true;

	DWORD threadId = 0;
	HANDLE hThread = CreateThread(NULL, 0, &DispatchRunBase, this, 0, &threadId);

	if (hThread == NULL) {
		BnxErrorStream << "Error: Could not create thread." << BnxEndl;
		return false;
	}

	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, m_hWnd, 0, 0)) != 0) {
		if (bRet == -1) {
			// TODO: Handle this
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);

	CleanUpWindow();

	return true;
}

void BnxWin32Driver::Shutdown() {
	WaitForSingleObject(m_hLock, INFINITE);

	m_bRun = false;

	if (m_hWnd != NULL)
		PostMessage(m_hWnd, WM_QUIT, 0, 0);

	ReleaseMutex(m_hLock);
}

DWORD BnxWin32Driver::RunBase() {
	m_pCheckShutdownTimer = event_new(GetEventBase(), -1, EV_PERSIST, &DispatchOnCheckShutdownTimer, this);

	struct timeval tv;

	tv.tv_sec = 1;
	tv.tv_usec = 0;

	event_add(m_pCheckShutdownTimer, &tv);

	bool bRet = BnxDriver::Run();

	if (!bRet) {
		// If this failed for other reasons, call Shutdown() here
		event_del(m_pCheckShutdownTimer);
		Shutdown();
	}

	event_free(m_pCheckShutdownTimer);

	m_pCheckShutdownTimer = NULL;

	return (DWORD)bRet;
}

void BnxWin32Driver::OnCheckShutdownTimer(evutil_socket_t fd, short what) {
	WaitForSingleObject(m_hLock, INFINITE);

	if (!m_bRun) {
		BnxDriver::Shutdown();
		event_del(m_pCheckShutdownTimer);
	}

	ReleaseMutex(m_hLock);
}

bool BnxWin32Driver::RegisterWindowClass() {
	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASSEX stWndClass;
	memset(&stWndClass, 0, sizeof(stWndClass));

	stWndClass.cbSize = sizeof(stWndClass);
	stWndClass.lpfnWndProc = &OnWindowEvent;
	stWndClass.hInstance = hInst;
	stWndClass.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_IRCBNXICON));
	stWndClass.lpszMenuName = MAKEINTRESOURCE(IDC_IRCBNXMENU);
	stWndClass.lpszClassName = "ircbnx";

	RegisterClassEx(&stWndClass);

	return true;
}


bool BnxWin32Driver::MakeWindow() {
	if (m_hWnd != NULL)
		return true;

	if (!RegisterWindowClass())
		return false;

	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

	m_hWnd = CreateWindow("ircbnx", "ircbnx", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	return m_hWnd != NULL;
}

void BnxWin32Driver::CleanUpWindow() {
	if (m_hWnd == NULL)
		return;

	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
}

