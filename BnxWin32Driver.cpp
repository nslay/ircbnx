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

#include <Windows.h>
#include <ShellAPI.h>

bool BnxWin32Driver::Run() {
	if (!MakeWindow()) {
		BnxErrorStream << "Error: Could not create window." << BnxEndl;
		return false;
	}

	DWORD threadId = 0;
	HANDLE threadHandle = CreateThread(NULL, 0, &DispatchRun, this, 0, &threadId);

	if (threadHandle == NULL) {
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

	WaitForSingleObject(threadHandle, INFINITE);

	CloseHandle(threadHandle);

	CleanUpWindow();

	return true;
}

void BnxWin32Driver::Shutdown() {
	WaitForSingleObject(m_hLock, INFINITE);

	BnxDriver::Shutdown();

	if (m_hWnd != NULL)
		PostMessage(m_hWnd, WM_QUIT, 0, 0);

	ReleaseMutex(m_hLock);
}

LRESULT BnxWin32Driver::OnWindowEvent(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	switch (uMsg) {
	case TRAY_ICON_MESSAGE:
		switch(lParam) {
		case WM_COMMAND:
			break;
		}
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD BnxWin32Driver::DispatchRun(LPVOID arg) {
	BnxWin32Driver * const pObject = (BnxWin32Driver *)arg;
	return (pObject->BnxDriver::Run)();
}


bool BnxWin32Driver::RegisterWindowClass() {
	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASSEX stWndClass;
	memset(&stWndClass, 0, sizeof(stWndClass));

	stWndClass.cbSize = sizeof(stWndClass);
	stWndClass.lpfnWndProc = (WNDPROC)&OnWindowEvent;
	stWndClass.hInstance = hInst;
	stWndClass.lpszClassName = "ircbnx";

	RegisterClassEx(&stWndClass);

	return true;
}

bool BnxWin32Driver::AddNotificationIcon() {
	if (m_hWnd == NULL)
		return false;

	NOTIFYICONDATA stIconData;
	memset(&stIconData, 0, sizeof(stIconData));

	stIconData.cbSize = sizeof(stIconData);
	stIconData.hWnd = m_hWnd;
	stIconData.uID = 0;
	stIconData.uVersion = NOTIFYICON_VERSION;
	stIconData.uCallbackMessage = (UINT)TRAY_ICON_MESSAGE;
	strcpy(stIconData.szTip, "ircbnx");

	stIconData.uFlags = NIF_MESSAGE|NIF_TIP;

	if (Shell_NotifyIcon(NIM_ADD, &stIconData) == FALSE)
		return false;

	return true;
}

bool BnxWin32Driver::DeleteNotificationIcon() {
	NOTIFYICONDATA stIconData;
	memset(&stIconData, 0, sizeof(stIconData));

	stIconData.cbSize = sizeof(stIconData);
	stIconData.hWnd = m_hWnd;

	Shell_NotifyIcon(NIM_DELETE, &stIconData);

	return true;
}

bool BnxWin32Driver::MakeWindow() {
	if (m_hWnd != NULL)
		return true;

	if (!RegisterWindowClass())
		return false;

	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

	m_hWnd = CreateWindow("ircbnx", "ircbnx", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	if (m_hWnd == NULL)
		return false;

	if (!AddNotificationIcon()) {
		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
		return false;
	}

	return true;
}

void BnxWin32Driver::CleanUpWindow() {
	if (m_hWnd == NULL)
		return;

	DeleteNotificationIcon();
	DestroyWindow(m_hWnd);
	m_hWnd = NULL;
}

