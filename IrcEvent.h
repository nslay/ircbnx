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

#include <functional>
#include <memory>
#include "event2/event.h"

class IrcEvent {
public:
	typedef std::function<void (evutil_socket_t, short)> CallbackType;

	template<typename ObjectType>
	static IrcEvent Bind(void (ObjectType::*Method)(evutil_socket_t, short), ObjectType *p_clObject) {
		using namespace std::placeholders;

		return IrcEvent(std::bind(Method, p_clObject, _1, _2));
	}

	static IrcEvent Bind(void (*callback)(evutil_socket_t, short, void *), void *pArg) {
		using namespace std::placeholders;

		return IrcEvent(std::bind(callback, _1, _2, pArg));
	}

	IrcEvent() = default;

	IrcEvent(const IrcEvent &clEvent) {
		*this = clEvent;
	}

	explicit IrcEvent(const CallbackType &clCallback) {
		m_clCallback = clCallback;
	}

	bool New(struct event_base *pBase, evutil_socket_t fd, short sWhat);

	bool NewTimer(struct event_base *pBase, short sWhat) {
		return New(pBase, -1, sWhat);
	}

	bool Add(struct timeval *p_stTv = nullptr) const {
		return m_pEvent && event_add(m_pEvent.get(), p_stTv) == 0;
	}

	bool Delete() const {
		return m_pEvent && event_del(m_pEvent.get()) == 0;
	}

	void Free() {
		m_pEvent.reset();
	}

	void operator()(evutil_socket_t fd, short sWhat) const {
		m_clCallback(fd, sWhat);
	}

	IrcEvent & operator=(const IrcEvent &clEvent) & {
		if (this != &clEvent)
			m_clCallback = clEvent.m_clCallback;

		return *this;
	}

	explicit operator bool() const {
		return m_pEvent != nullptr;
	}

private:
	struct EventDeleter {
		void operator()(struct event *pEvent) const {
			if (pEvent != nullptr)
				event_free(pEvent);
		}
	};

	std::unique_ptr<struct event, EventDeleter> m_pEvent;
	CallbackType m_clCallback;
};

#endif // !IRCEVENT_H

