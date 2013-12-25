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

#include <cstdio>
#include <sstream>
#include "getopt.h"
#include "BnxDriver.h"
#include "BnxStreams.h"

#ifdef _WIN32
#include <Windows.h>
#include <ShellAPI.h>
#else // !_WIN32
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#endif // _WIN32

#ifdef USE_PCRE
// Include for pcre_version()
#include "pcre.h"
#endif // USE_PCRE

BnxDriver::~BnxDriver() {
	Reset();
}

void BnxDriver::Usage() {
	// TODO: Get program name
	BnxErrorStream << "Usage: ircbnx [-hv] [-c config.ini]" << BnxEndl;
}

bool BnxDriver::ParseArgs(int argc, char *argv[]) {
	int c;

	while ((c = getopt(argc, argv, "c:hv")) != -1) {
		switch (c) {
		case 'c':
			SetConfigFile(optarg);
			break;
		case 'v':
			BnxOutStream << BnxBot::GetVersionString() << "\n\n";
			BnxOutStream << "libevent: " << event_get_version() << '\n';
#ifdef USE_PCRE
			BnxOutStream << "pcre: " << pcre_version() << BnxEndl;
#endif // USE_PCRE
			return false;
		case 'h':
		default:
			Usage();
			return false;
		}
	}

	return true;
}

bool BnxDriver::Load() {
	Reset();

	IniFile clConfig;

	if (!clConfig.Load(m_strConfigFile))
		return false;

	const IniFile::Section &clGlobal = clConfig.GetSection("global");

	std::string strEntries = clGlobal.GetValue<std::string>("profiles", "");
	m_strLogFile = clGlobal.GetValue<std::string>("logfile", m_strLogFile);

	if (strEntries.empty())
		return false;

	std::stringstream profileStream(strEntries);

	std::string strProfile;
	while (std::getline(profileStream,strProfile,',')) {
		const IniFile::Section &clSection = clConfig.GetSection(strProfile);
		LoadBot(clSection);
	}

	return !m_vBots.empty();
}

bool BnxDriver::Run() {
	if (!Load())
		return false;

	if (!Daemonize())
		return false;

	struct event_base *pEventBase;

	pEventBase = event_base_new();

#ifdef __unix__
	m_pSigTerm = event_new(pEventBase, SIGTERM, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxDriver::OnSignal>, this);
	m_pSigInt = event_new(pEventBase, SIGINT, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxDriver::OnSignal>, this);
	m_pSigAbrt = event_new(pEventBase, SIGABRT, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxDriver::OnSignal>, this);
	m_pSigQuit = event_new(pEventBase, SIGQUIT, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxDriver::OnSignal>, this);

	event_add(m_pSigTerm, NULL);
	event_add(m_pSigInt, NULL);
	event_add(m_pSigAbrt, NULL);
	event_add(m_pSigQuit, NULL);
#endif // __unix__

	for (size_t i = 0; i < m_vBots.size(); ++i) {
		m_vBots[i]->SetEventBase(pEventBase);
		m_vBots[i]->StartUp();
	}

	event_base_dispatch(pEventBase);

#ifdef __unix__
	event_free(m_pSigTerm);
	event_free(m_pSigInt);
	event_free(m_pSigAbrt);
	event_free(m_pSigQuit);

	m_pSigTerm = m_pSigInt = m_pSigAbrt = m_pSigQuit = NULL;
#endif // __unix__

	event_base_free(pEventBase);

	return true;
}

void BnxDriver::Shutdown() {
	for (size_t i = 0; i < m_vBots.size(); ++i)
		m_vBots[i]->Shutdown();

#ifdef __unix__
	if (m_pSigTerm != NULL)
		event_del(m_pSigTerm);

	if (m_pSigInt != NULL)
		event_del(m_pSigInt);
	
	if (m_pSigAbrt != NULL)
		event_del(m_pSigAbrt);

	if (m_pSigQuit != NULL)
		event_del(m_pSigQuit);
#endif // __unix__
}

void BnxDriver::Reset() {
	for (size_t i = 0; i < m_vBots.size(); ++i)
		delete m_vBots[i];

	m_vBots.clear();

#ifdef _WIN32
	if (m_hWnd != NULL) {
		NOTIFYICONDATA stIconData;
		memset(&stIconData, 0, sizeof(stIconData));

		stIconData.cbSize = sizeof(stIconData);
		stIconData.hWnd = m_hWnd;

		Shell_NotifyIcon(NIM_DELETE, &stIconData);

		DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
#endif // _WIN32
}

#ifdef _WIN32
bool BnxDriver::Daemonize() {
	if (m_hWnd != NULL)
		return true;

	HINSTANCE hInst = (HINSTANCE)GetModuleHandle(NULL);

	WNDCLASSEX stWndClass;
	memset(&stWndClass, 0, sizeof(stWndClass));

	stWndClass.cbSize = sizeof(stWndClass);
	stWndClass.lpfnWndProc = (WNDPROC)&OnWindowEvent;
	stWndClass.hInstance = hInst;
	stWndClass.lpszClassName = "ircbnx";

	RegisterClassEx(&stWndClass);

	m_hWnd = CreateWindow("ircbnx", "ircbnx", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInst, NULL);

	if (m_hWnd == NULL) {
		BnxErrorStream << "Error: Could not create window: " << GetLastError() << BnxEndl;
		return false;
	}

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
		BnxErrorStream << "Warning: Could not add notication icon: " << GetLastError() << BnxEndl;

	return true;
}

LRESULT BnxDriver::OnWindowEvent(_In_ HWND hWnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
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

#else // !_WIN32
bool BnxDriver::Daemonize() {
	// Taken from daemon(3) (which is not standard)

	// Ignore signal from parent exiting
	signal(SIGHUP, SIG_IGN);

	switch (fork()) {
	case -1:
		perror("fork");
		return false;
	case 0:
		// Child
		break;
	default:
		// Parent
		exit(0);
	}

	setsid();

	int fd = open("/dev/null", O_RDWR, 0);

	if (fd != -1) {
		dup2(fd, STDIN_FILENO);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDERR_FILENO);

		if (fd != STDIN_FILENO && 
			fd != STDOUT_FILENO && 
			fd != STDERR_FILENO) {
			close(fd);
		}
	}
	
	return true;
}
#endif // _WIN32

void BnxDriver::LoadBot(const IniFile::Section &clSection) {
	bool bEnabled = clSection.GetValue<bool>("enabled", true);

	if (!bEnabled)
		return;

	std::string strServer = clSection.GetValue<std::string>("server", "");
	std::string strNickname = clSection.GetValue<std::string>("nickname", "");

	if (strServer.empty() || strNickname.empty())
		return;

	BnxBot *pclBot = GetBot(clSection.GetName());

	if (pclBot == NULL) {
		pclBot = new BnxBot();
		pclBot->SetProfileName(clSection.GetName());

		m_vBots.push_back(pclBot);
	}

	std::string strPort = clSection.GetValue<std::string>("port", "6667");
	std::string strUsername = clSection.GetValue<std::string>("username", "BnxBot");
	std::string strRealName = clSection.GetValue<std::string>("realname", "BnxBot");
	std::string strAccessList = clSection.GetValue<std::string>("accesslist", "access.lst");
	std::string strShitList = clSection.GetValue<std::string>("shitlist", "shit.lst");
	std::string strSeenList = clSection.GetValue<std::string>("seenlist", "seen.lst");
	std::string strResponseRules = clSection.GetValue<std::string>("responserules", "response.txt");
	std::string strHomeChannels = clSection.GetValue<std::string>("homechannels", "");
	std::string strNickServ = clSection.GetValue<std::string>("nickserv", "");
	std::string strNickServPassword = clSection.GetValue<std::string>("nickservpassword", "");
	
	pclBot->SetServerAndPort(strServer, strPort);
	pclBot->SetNickServAndPassword(strNickServ, strNickServPassword);
	pclBot->SetNickname(strNickname);
	pclBot->SetUsername(strUsername);
	pclBot->SetRealName(strRealName);
	pclBot->LoadResponseRules(strResponseRules);
	pclBot->LoadAccessList(strAccessList);
	pclBot->LoadShitList(strShitList);
	pclBot->LoadSeenList(strSeenList);
	pclBot->SetLogFile(m_strLogFile);
	pclBot->SetHomeChannels(strHomeChannels);
}

BnxBot * BnxDriver::GetBot(const std::string &strProfile) const {
	for (size_t i = 0; i < m_vBots.size(); ++i) {
		if (strProfile == m_vBots[i]->GetProfileName())
			return m_vBots[i];
	}

	return NULL;
}

