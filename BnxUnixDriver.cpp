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

#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <cstdio>

#include "BnxDriver.h"
#include "BnxStreams.h"
#include "BnxUnixDriver.h"

bool BnxUnixDriver::Run() {
	if (!Daemonize())
		return false;

	m_clSigTerm.New(GetEventBase(), SIGTERM, EV_SIGNAL|EV_PERSIST);
	m_clSigInt.New(GetEventBase(), SIGINT, EV_SIGNAL|EV_PERSIST);
	m_clSigAbrt.New(GetEventBase(), SIGABRT, EV_SIGNAL|EV_PERSIST);
	m_clSigQuit.New(GetEventBase(), SIGQUIT, EV_SIGNAL|EV_PERSIST);

	m_clSigTerm.Add();
	m_clSigInt.Add();
	m_clSigAbrt.Add();
	m_clSigQuit.Add();

	bool bRet = BnxDriver::Run();

	m_clSigTerm.Free();
	m_clSigInt.Free();
	m_clSigAbrt.Free();
	m_clSigQuit.Free();

	return bRet;
}

void BnxUnixDriver::Shutdown() {
	BnxDriver::Shutdown();

	m_clSigTerm.Delete();
	m_clSigInt.Delete();
	m_clSigAbrt.Delete();
	m_clSigQuit.Delete();
}

bool BnxUnixDriver::Daemonize() {
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

	// Re-initialize after fork
	event_reinit(GetEventBase());
	
	return true;
}

