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

#ifndef BNXWIN32DRIVER_H
#define BNXWIN32DRIVER_H

#include <cstddef>
#include <string>
#include <vector>
#include <iostream>
#include "BnxDriver.h"
#include "IniFile.h"
#include "BnxBot.h"
#include <Windows.h>

class BnxWin32Driver : public BnxDriver {
public:
	BnxWin32Driver() {
		m_hWnd = NULL;
		m_hLock = CreateMutex(NULL, 0, NULL);
	}

	virtual ~BnxWin32Driver() {
		CleanUpWindow();

		if (m_hLock != NULL) {
			CloseHandle(m_hLock);
			m_hLock = NULL;
		}
	}

	virtual bool Run();
	virtual void Shutdown();

private:
	enum { TRAY_ICON_MESSAGE = 6112 };

	HWND m_hWnd;
	HANDLE m_hLock;

	static LRESULT OnWindowEvent(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
	static DWORD DispatchRun(LPVOID arg);

	// Disabled
	BnxWin32Driver(const BnxWin32Driver &);

	// Disabled
	BnxWin32Driver & operator=(const BnxWin32Driver &);

	bool RegisterWindowClass();
	bool AddNotificationIcon();
	bool DeleteNotificationIcon();

	bool MakeWindow();
	void CleanUpWindow();
};

#endif // !BNXWIN32DRIVER_H

