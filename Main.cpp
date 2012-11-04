/*-
 * Copyright (c) 2012 Nathan Lay (nslay@users.sourceforge.net)
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

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif // _WIN32

#include <signal.h>
#include <iostream>
#include "BnxMain.h"

int main(int argc, char **argv) {

#ifdef _WIN32
	WORD wsaVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	int e;

	e = WSAStartup(wsaVersion, &wsaData);
	if (e != 0) {
		std::cerr << "WSAStartup failed with error: " << e << std::endl;
		return -1;
	}
#else // _WIN32
	signal(SIGPIPE, SIG_IGN);
#endif // !_WIN32

	BnxMain clBnxMain;

	clBnxMain.SetConfigFile("bot.ini");

	if (!clBnxMain.Load()) {
		std::cerr << "Could not configuration file (or no profiles enabled)." << std::endl;
		return -1;
	}

	clBnxMain.Run();

#ifdef _WIN32
	WSACleanup();
#endif // _WIN32

	return 0;
}

