/*-
 * Copyright (c) 2012-2013 Nathan Lay (nslay@users.sourceforge.net)
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

#ifndef BNXDRIVER_H
#define BNXDRIVER_H

#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "event2/event.h"
#include "IniFile.h"
#include "BnxBot.h"

class BnxDriver {
public:
	typedef std::vector<BnxBot *>::const_iterator BotIterator;

	static BnxDriver & GetInstance() {
		static BnxDriver clDriver;
		return clDriver;
	}

	~BnxDriver();

	void SetConfigFile(const std::string &strConfigFile) {
		m_strConfigFile = strConfigFile;
	}

	void Usage();
	bool ParseArgs(int argc, char *argv[]);
	bool Load();
	bool Run();
	void Shutdown();
	void Reset();

	BotIterator BotBegin() const {
		return m_vBots.begin();
	}

	BotIterator BotEnd() const {
		return m_vBots.end();
	}

	BnxBot * GetBot(const std::string &strProfile) const;

private:
	std::string m_strConfigFile, m_strLogFile;
	std::vector<BnxBot *> m_vBots;

	template<void (BnxDriver::*Method)(evutil_socket_t, short)>
	static void Dispatch(evutil_socket_t fd, short what, void *arg) {
		BnxDriver *pObject = (BnxDriver *)arg;
		(pObject->*Method)(fd, what);
	}

	BnxDriver() {
		m_strConfigFile = "bot.ini";
		m_strLogFile = "bot.log";
#ifdef __unix__
		m_pSigTerm = m_pSigInt = m_pSigAbrt = m_pSigQuit = NULL;
#endif // __unix__

#ifdef _WIN32
		m_hWnd = NULL;
#endif // _WIN32
	}

	// Disabled
	BnxDriver(const BnxDriver &);

	bool Daemonize();

	void LoadBot(const IniFile::Section &clSection);

	// Disabled
	BnxDriver & operator=(const BnxDriver &);

#ifdef __unix__
	// Unix-specific signals
	struct event *m_pSigTerm, *m_pSigInt, *m_pSigAbrt, *m_pSigQuit;

	void OnSignal(evutil_socket_t signal, short what) {
		Shutdown();
	}
#endif // __unix__

#ifdef _WIN32
	enum { TRAY_ICON_MESSAGE = 6112 };

	HWND m_hWnd;

	static LRESULT OnWindowEvent(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);
#endif // _WIN32
};

#endif // !BNXDRIVER_H

