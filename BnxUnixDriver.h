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

#ifndef BNXUNIXDRIVER_H
#define BNXUNIXDRIVER_H

#include <cstddef>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "event2/event.h"
#include "BnxDriver.h"
#include "BnxBot.h"

class BnxUnixDriver : public BnxDriver {
public:
	BnxUnixDriver() { 
		m_pSigTerm = m_pSigInt = m_pSigAbrt = m_pSigQuit = NULL;
	}

	virtual bool Run();
	virtual void Shutdown();

private:
	// Unix-specific signals
	struct event *m_pSigTerm, *m_pSigInt, *m_pSigAbrt, *m_pSigQuit;

	template<void (BnxUnixDriver::*Method)(evutil_socket_t, short)>
	static void Dispatch(evutil_socket_t fd, short what, void *arg) {
		BnxUnixDriver *pObject = (BnxUnixDriver *)arg;
		(pObject->*Method)(fd, what);
	}

	// Disabled
	BnxUnixDriver(const BnxUnixDriver &);

	// Disabled
	BnxUnixDriver & operator=(const BnxUnixDriver &);

	bool Daemonize();

	void OnSignal(evutil_socket_t signal, short what) {
		Shutdown();
	}
};

#endif // !BNXUNIXDRIVER_H

