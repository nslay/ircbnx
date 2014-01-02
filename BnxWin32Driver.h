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
#include "IrcEvent.h"
#include "IniFile.h"
#include "BnxBot.h"
#include <Windows.h>

class BnxWin32Driver : public BnxDriver {
public:
	static bool RegisterWindowClass();
	static bool AddNotificationIcon(HWND hWnd);
	static bool DeleteNotificationIcon(HWND hWnd);
	static void ShowContextMenu(HWND hWnd);

	BnxWin32Driver() {
		m_hWnd = nullptr;
		m_hLock = CreateMutex(nullptr, 0, nullptr);
		m_bRun = false;
		m_clCheckShutdownTimer = IrcEvent::Bind<BnxWin32Driver, &BnxWin32Driver::OnCheckShutdownTimer>(this);
	}

	virtual ~BnxWin32Driver() {
		CleanUpWindow();

		if (m_hLock != nullptr) {
			CloseHandle(m_hLock);
			m_hLock = nullptr;
		}
	}

	virtual bool Run();
	virtual void Shutdown();

	DWORD RunBase();

private:
	HWND m_hWnd;
	HANDLE m_hLock;
	bool m_bRun;
	IrcEvent m_clCheckShutdownTimer;

	// Disabled
	BnxWin32Driver(const BnxWin32Driver &);

	// Disabled
	BnxWin32Driver & operator=(const BnxWin32Driver &);

	bool MakeWindow();
	void CleanUpWindow();

	void OnCheckShutdownTimer(evutil_socket_t fd, short what);
};

#endif // !BNXWIN32DRIVER_H

