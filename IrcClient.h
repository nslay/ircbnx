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

#ifndef IRCCLIENT_H
#define IRCCLIENT_H

#include <string>
#include <deque>
#include <cstdarg>
#include "IrcTraits.h"
#include "IrcCounter.h"
#include "event2/event.h"

class IrcClient {
public:
	enum WhenType { AUTO = 0, NOW, LATER };

	IrcClient()
	: m_socket(INVALID_SOCKET), m_strUsername("IrcClient"), m_strRealName("IrcClient"), 
	m_stagingBufferSize(0), m_lastRecvTime(0), m_pEventBase(NULL), m_pReadEvent(NULL), 
	m_pWriteEvent(NULL), m_pSendTimer(NULL) { }

	virtual ~IrcClient();

	void SetEventBase(struct event_base *pEventBase);
	struct event_base * GetEventBase() const;

	const std::string & GetNickname() const;

	const std::string & GetCurrentNickname() const;
	const std::string & GetCurrentServer() const;
	const std::string & GetCurrentPort() const;
	const IrcTraits & GetIrcTraits() const;
	bool IsRegistered() const;
	time_t GetLastRecvTime() const;

	virtual void SetNickname(const std::string &strNickname);
	virtual void SetUsername(const std::string &strUsername);
	virtual void SetRealName(const std::string &strRealName);

	virtual bool Connect(const std::string &server, const std::string &port = "6667");
	virtual bool Reconnect();
	virtual void Disconnect();

protected:
	virtual void Log(const char *pFormat, ...);
	virtual void Send(WhenType eWhen, const char *pFormat, ...);
	virtual void SendRaw(const void *pData, size_t dataSize);

	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnRegistered();
	virtual void OnNumeric(const char *pSource, int numeric, const char *pParams[], unsigned int numParams);
	virtual void OnNick(const char *pSource, const char *pNewNick);
	virtual void OnQuit(const char *pSource, const char *pReason);
	virtual void OnJoin(const char *pSource, const char *pChannel);
	virtual void OnPart(const char *pSource, const char *pChannel, const char *pReason);
	virtual void OnMode(const char *pSource, const char *pTarget, const char *pMode, const char *pParams[], unsigned int numParams);
	virtual void OnTopic(const char *pSource, const char *pChannel, const char *pTopic);
	virtual void OnInvite(const char *pSource, const char *pChannel);
	virtual void OnKick(const char *pSource, const char *pChannel, const char *pUser, const char *pReason);
	virtual void OnPrivmsg(const char *pSource, const char *pTarget, const char *pMessage);
	virtual void OnNotice(const char *pSource, const char *pTarget, const char *pMessage);
	virtual void OnPing(const char *pServer);
	virtual void OnPong(const char *pServer1, const char *pServer2);
	virtual void OnError(const char *pMessage);
	virtual void OnWallops(const char *pSource, const char *pMessage);

private:
#ifdef _WIN32
	typedef SOCKET SocketType;
#else
	typedef int SocketType;
	enum { INVALID_SOCKET = -1 };
#endif

	SocketType m_socket;
	std::string m_strNickname, m_strUsername, m_strRealName, m_strCurrentNickname, 
		m_strCurrentServer, m_strCurrentPort;

	IrcTraits m_clIrcTraits;
	IrcCounter m_clSendCounter;
	std::deque<std::string> m_dqSendQueue;

	char m_stagingBuffer[4096];
	size_t m_stagingBufferSize;
	time_t m_lastRecvTime;

	struct event_base *m_pEventBase;
	struct event *m_pReadEvent, *m_pWriteEvent, *m_pSendTimer;

	static char * PopToken(char *&pStr);

	template<void (IrcClient::*Method)(evutil_socket_t, short)>
	static void Dispatch(evutil_socket_t fd, short what, void *arg) {
		IrcClient *pObject = (IrcClient *)arg;
		(pObject->*Method)(fd, what);
	}

	void CloseSocket();

	void ProcessLine(char *pLine);

	// Libevent callbacks
	void OnWrite(evutil_socket_t fd, short what);
	void OnRead(evutil_socket_t fd, short what);
	void OnSendTimer(evutil_socket_t fd, short what);

};

#endif // !IRCCLIENT_H

