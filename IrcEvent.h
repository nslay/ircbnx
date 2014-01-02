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

#ifndef IRCEVENT_H
#define IRCEVENT_H

#include "event2/event.h"

class IrcEvent {
public:
	typedef void (*CallbackType)(evutil_socket_t, short, void *);

	IrcEvent() {
		m_pEvent = nullptr;
		m_pArg = nullptr;
		m_pCallback = nullptr;
	}

	IrcEvent(const IrcEvent &clEvent) {
		m_pEvent = nullptr;
		m_pArg = clEvent.m_pArg;
		m_pCallback = clEvent.m_pCallback;
	}

	virtual ~IrcEvent() {
		Free();
	}

	static IrcEvent Bind(CallbackType pCallback, void *pArg) {
		IrcEvent clEvent;

		clEvent.m_pArg = pArg;
		clEvent.m_pCallback = pCallback;

		return clEvent;
	}

	template<typename ObjectType, void (ObjectType::*Method)(evutil_socket_t fd, short sWhat)>
	static IrcEvent Bind(ObjectType *p_clObject) {
		return Bind(&Dispatch<ObjectType, Method>, p_clObject);
	}

	bool New(struct event_base *pBase, evutil_socket_t fd, short sWhat);

	bool NewTimer(struct event_base *pBase, short sWhat) {
		return New(pBase, -1, sWhat);
	}

	bool Add(struct timeval *p_stTv = nullptr) const {
		return m_pEvent != nullptr && event_add(m_pEvent, p_stTv) == 0;
	}

	bool Delete() const {
		return m_pEvent != nullptr && event_del(m_pEvent) == 0;
	}

	void Free() {
		if (m_pEvent != nullptr) {
			event_free(m_pEvent);
			m_pEvent = nullptr;
		}
	}

	void operator()(evutil_socket_t fd, short sWhat) const {
		(*m_pCallback)(fd, sWhat, m_pArg);
	}

	IrcEvent & operator=(const IrcEvent &clEvent) {
		if (this == &clEvent)
			return *this;

		m_pArg = clEvent.m_pArg;
		m_pCallback = clEvent.m_pCallback;

		return *this;
	}

	operator bool() const {
		return m_pEvent != nullptr;
	}

private:
	struct event *m_pEvent;
	void *m_pArg;
	CallbackType m_pCallback;

	template<class ObjectType, void (ObjectType::*Method)(evutil_socket_t fd, short sWhat)>
	static void Dispatch(evutil_socket_t fd, short sWhat, void *pArg) {
		ObjectType * const p_clObject = (ObjectType *)pArg;
		(p_clObject->*Method)(fd, sWhat);
	}
};

#endif // !IRCEVENT_H

