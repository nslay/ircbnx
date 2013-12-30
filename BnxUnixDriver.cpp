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

	m_pSigTerm = event_new(GetEventBase(), SIGTERM, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxUnixDriver::OnSignal>, this);
	m_pSigInt = event_new(GetEventBase(), SIGINT, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxUnixDriver::OnSignal>, this);
	m_pSigAbrt = event_new(GetEventBase(), SIGABRT, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxUnixDriver::OnSignal>, this);
	m_pSigQuit = event_new(GetEventBase(), SIGQUIT, EV_SIGNAL|EV_PERSIST, &Dispatch<&BnxUnixDriver::OnSignal>, this);

	event_add(m_pSigTerm, NULL);
	event_add(m_pSigInt, NULL);
	event_add(m_pSigAbrt, NULL);
	event_add(m_pSigQuit, NULL);

	bool bRet = BnxDriver::Run();

	event_free(m_pSigTerm);
	event_free(m_pSigInt);
	event_free(m_pSigAbrt);
	event_free(m_pSigQuit);

	m_pSigTerm = m_pSigInt = m_pSigAbrt = m_pSigQuit = NULL;

	return bRet;
}

void BnxUnixDriver::Shutdown() {
	BnxDriver::Shutdown();

	if (m_pSigTerm != NULL)
		event_del(m_pSigTerm);

	if (m_pSigInt != NULL)
		event_del(m_pSigInt);
	
	if (m_pSigAbrt != NULL)
		event_del(m_pSigAbrt);

	if (m_pSigQuit != NULL)
		event_del(m_pSigQuit);
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

