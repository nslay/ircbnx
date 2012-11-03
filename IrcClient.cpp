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

#ifdef _MSC_VER
// Disable vsnprintf warnings
#pragma warning(disable:4996)
#endif // _MSC_VER

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <sstream>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <iostream>
#include "IrcClient.h"
#include "IrcUser.h"
#include "Irc.h"

char * IrcClient::PopToken(char *&pStr) {
	for ( ; *pStr == ' '; ++pStr);

	char *pTmp = pStr;

	for ( ; *pStr != ' ' && *pStr != '\0'; ++pStr);

	if (*pStr != '\0') {
		*pStr++ = '\0';
		for ( ; *pStr == ' '; ++pStr);
	}

	return pTmp;
}


IrcClient::~IrcClient() {
	Disconnect();
}

void IrcClient::SetEventBase(struct event_base *pEventBase) {
	m_pEventBase = pEventBase;
}

struct event_base * IrcClient::GetEventBase() const {
	return m_pEventBase;
}

const std::string & IrcClient::GetCurrentNickname() const {
	return m_strCurrentNickname;
}

const std::string & IrcClient::GetCurrentServer() const {
	return m_strCurrentServer;
}

const std::string & IrcClient::GetCurrentPort() const {
	return m_strCurrentPort;
}

const IrcTraits & IrcClient::GetIrcTraits() const {
	return m_clIrcTraits;
}

bool IrcClient::IsRegistered() const {
	return !m_strCurrentNickname.empty();
}

void IrcClient::SetNickname(const std::string &strNickname) {
	m_strNickname = strNickname;
}

void IrcClient::SetUsername(const std::string &strUsername) {
	m_strUsername = strUsername;
}

void IrcClient::SetRealName(const std::string &strRealName) {
	m_strRealName = strRealName;
}

bool IrcClient::Connect(const std::string &strServer, const std::string &strPort) {
	if (m_socket != INVALID_SOCKET)
		Disconnect();

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == INVALID_SOCKET) {
		perror("socket");
		return false;
	}

#ifdef _WIN32
	u_long opt = 1;
	if (ioctlsocket(m_socket, FIONBIO, &opt) != 0) {
		perror("ioctlsocket");

		CloseSocket();

		return false;
	}
#else // _WIN32
	int flags = fcntl(m_socket, F_GETFL);

	if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl");

		CloseSocket();

		return false;
	}
#endif // _WIN32

	struct addrinfo hints, *pResults = NULL;
	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int e = getaddrinfo(strServer.c_str(), strPort.c_str(), &hints, &pResults);
	if (e != 0) {
		fprintf(stderr, "%s\n", gai_strerror(e));
		CloseSocket();
		return false;
	}

	// NOTE: Cast to socklen_t since Windows make ai_addrlen size_t
	e = connect(m_socket, pResults->ai_addr, (socklen_t)pResults->ai_addrlen);

#ifdef _WIN32
	if (e != 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
		perror("connect");
		CloseSocket();
		freeaddrinfo(pResults);
		return false;
	}
#else // _WIN32
	if (e != 0 && errno != EINPROGRESS) {
		perror("connect");
		CloseSocket();
		freeaddrinfo(pResults);
		return false;
	}
#endif // !_WIN32

	freeaddrinfo(pResults);

	// XXX: Handle errors?
	m_pWriteEvent = event_new(m_pEventBase, m_socket, EV_WRITE, &Dispatch<&IrcClient::OnWrite>, this);
	m_pReadEvent = event_new(m_pEventBase, m_socket, EV_READ | EV_PERSIST, &Dispatch<&IrcClient::OnRead>, this);
	m_pSendTimer = event_new(m_pEventBase, -1, EV_PERSIST, &Dispatch<&IrcClient::OnSendTimer>, this);

	event_add(m_pWriteEvent, NULL);

	m_strCurrentServer = strServer;
	m_strCurrentPort = strPort;

	return true;
}

bool IrcClient::Reconnect() {
	// Copy these over since Disconnect() clears them
	std::string strServer = m_strCurrentServer,
		strPort = m_strCurrentPort;

	return Connect(strServer,strPort);
}

void IrcClient::Disconnect() {
	m_strCurrentServer.clear();
	m_strCurrentPort.clear();
	m_strCurrentNickname.clear();

	m_clIrcTraits.Reset();
	m_clSendCounter.Reset();

	m_dqSendQueue.clear();

	// This is REALLY important since this can be called while OnRead() is still processing messages
	m_stagingBufferSize = 0;

	CloseSocket();

	if (m_pWriteEvent != NULL) {
		event_del(m_pWriteEvent);
		event_free(m_pWriteEvent);
		m_pWriteEvent = NULL;
	}

	if (m_pReadEvent != NULL) {
		event_del(m_pReadEvent);
		event_free(m_pReadEvent);
		m_pReadEvent = NULL;
	}

	if (m_pSendTimer != NULL) {
		event_del(m_pSendTimer);
		event_free(m_pSendTimer);
		m_pSendTimer = NULL;
	}
}

void IrcClient::Send(const char *pFormat, ...) {
	if (m_socket == -1)
		return;

	char buff[513];

	va_list ap;
	va_start(ap, pFormat);
	int buffSize = vsnprintf(buff, sizeof(buff), pFormat, ap);
	va_end(ap);

	// TODO: Tunable for bursting
	if (!m_dqSendQueue.empty() || m_clSendCounter.GetCurrentCount() > 4) {
		m_dqSendQueue.push_back(buff);
		return;
	}

	SendRaw(buff, buffSize);
}

void IrcClient::SendNow(const char *pFormat, ...) {
	if (m_socket == -1)
		return;

	char buff[513];

	va_list ap;
	va_start(ap, pFormat);
	int buffSize = vsnprintf(buff, sizeof(buff), pFormat, ap);
	va_end(ap);

	SendRaw(buff, buffSize);
}

void IrcClient::SendLater(const char *pFormat, ...) {
	if (m_socket == -1)
		return;

	char buff[513];

	va_list ap;
	va_start(ap, pFormat);
	int buffSize = vsnprintf(buff, sizeof(buff), pFormat, ap);
	va_end(ap);

	m_dqSendQueue.push_back(buff);
}

void IrcClient::SendRaw(const void *pData, size_t dataSize) {
	if (m_socket == -1)
		return;

	printf("-> %s", (const char *)pData);

	m_clSendCounter.Hit();

#ifdef _WIN32
	send(m_socket, (const char *)pData, (int)dataSize, 0);
#else // _WIN32
	send(m_socket, pData, dataSize, 0);
#endif // !_WIN32
}

void IrcClient::OnConnect() {
	SendNow("NICK %s\r\n", m_strNickname.c_str());
	SendNow("USER %s localhost localhost :%s\r\n", m_strUsername.c_str(), m_strRealName.c_str());

	m_stagingBufferSize = 0;
}

void IrcClient::OnDisconnect() {
	Disconnect();
}

void IrcClient::OnRegistered() {

}

void IrcClient::OnNumeric(const char *pPrefix, int numeric, const char **pParams, unsigned int numParams) {

	switch (numeric) {
	case RPL_ISUPPORT:
		for (unsigned int i = 1; i < numParams; ++i) {
			//printf("Parsing: %s\n", pParams[i]);
			if (!m_clIrcTraits.Parse(pParams[i]))
				fprintf(stderr, "Failed to parse '%s'\n", pParams[i]);
		}
		break;
	case RPL_LUSERCLIENT:
		if (!IsRegistered()) {
			m_strCurrentNickname = pParams[0];
			OnRegistered();
		}
		break;
	case ERR_NICKNAMEINUSE:
		if (!IsRegistered())
		{
			std::stringstream ss;
			ss << m_strNickname << (rand() % 1000);

			Send("NICK %s\r\n", ss.str().c_str());
		}
		break;
	}
}

void IrcClient::OnNick(const char *pSource, const char *pNewNick) {
	IrcUser clUser(pSource);

	const std::string &strNickname = clUser.GetNickname();

	if (m_strCurrentNickname == strNickname)
		m_strCurrentNickname = pNewNick;
}

void IrcClient::OnQuit(const char *pSource, const char *pReason) {

}

void IrcClient::OnJoin(const char *pSource, const char *pChannel) {

}

void IrcClient::OnPart(const char *pSource, const char *pChannel, const char *pReason) {

}

void IrcClient::OnMode(const char *pSource, const char *pTarget, const char *pMode, const char *pParams[], unsigned int numParams) {

}

void IrcClient::OnTopic(const char *pSource, const char *pChannel, const char *pTopic) {

}

void IrcClient::OnInvite(const char *pSource, const char *pChannel) {

}

void IrcClient::OnKick(const char *pSource, const char *pChannel, const char *pUser, const char *pReason) {

}

void IrcClient::OnPrivmsg(const char *pSource, const char *pTarget, const char *pMessage) {

}

void IrcClient::OnNotice(const char *pSource, const char *pTarget, const char *pMessage) {

}

void IrcClient::OnPing(const char *pServer) {
	SendNow("PONG :%s\r\n", pServer);
}

void IrcClient::OnPong(const char *pServer1, const char *pServer2) {
}

void IrcClient::OnError(const char *pMessage) {

}

void IrcClient::OnWallops(const char *pSource, const char *pMessage) {

}

void IrcClient::CloseSocket() {
	if (m_socket == INVALID_SOCKET)
		return;

#ifdef _WIN32
	closesocket(m_socket);
#else
	close(m_socket);
#endif

	m_socket = INVALID_SOCKET;
}

void IrcClient::ProcessLine(char *pLine) {
	const char *pPrefix = NULL, *pCommand = NULL, *pParams[15] = { NULL };
	unsigned int numParams = 0;

	puts(pLine);

	if (*pLine == ':')
		pPrefix = PopToken(pLine)+1;

	pCommand = PopToken(pLine);

	while (*pLine != '\0' && numParams < 15) {
		if (*pLine == ':') {
			pParams[numParams++] = pLine+1;
			break;
		}

		pParams[numParams++] = PopToken(pLine);
	}

	// XXX: Shouldn't we check if sufficient parameters are present?
	if (isdigit(pCommand[0])) {
		// This is probably a numeric

		int numeric = strtol(pCommand,NULL,10);

		OnNumeric(pPrefix, numeric, pParams, numParams);
	}
	else if (!strcmp(pCommand, "NICK")) {
		OnNick(pPrefix, pParams[0]);
	}
	else if (!strcmp(pCommand, "QUIT")) {
		OnQuit(pPrefix, pParams[0]);
	}
	else if (!strcmp(pCommand, "JOIN")) {
		OnJoin(pPrefix, pParams[0]);
	}
	else if (!strcmp(pCommand, "PART")) {
		OnPart(pPrefix, pParams[0], pParams[1]);
	}
	else if (!strcmp(pCommand, "MODE")) {
		OnMode(pPrefix, pParams[0], pParams[1], pParams+2, numParams-2);
	}
	else if (!strcmp(pCommand, "TOPIC")) {
		OnTopic(pPrefix, pParams[0], pParams[1]);
	}
	else if (!strcmp(pCommand, "INVITE")) {
		OnInvite(pPrefix, pParams[0]);
	}
	else if (!strcmp(pCommand, "KICK")) {
		OnKick(pPrefix, pParams[0], pParams[1], pParams[2]);
	}
	else if (!strcmp(pCommand, "PRIVMSG")) {
		OnPrivmsg(pPrefix, pParams[0], pParams[1]);
	}
	else if (!strcmp(pCommand, "NOTICE")) {
		OnNotice(pPrefix, pParams[0], pParams[1]);
	}
	else if (!strcmp(pCommand, "PING")) {
		OnPing(pParams[0]);
	}
	else if (!strcmp(pCommand, "PONG")) {
		OnPong(pParams[0], pParams[1]);
	}
	else if (!strcmp(pCommand, "ERROR")) {
		OnError(pParams[0]);
	}
	else if (!strcmp(pCommand, "WALLOPS")) {
		OnWallops(pPrefix, pParams[0]);
	}

}

void IrcClient::OnWrite(evutil_socket_t fd, short what) {
	event_add(m_pReadEvent, NULL);

	// TODO: Tunable for send timer
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;

	event_add(m_pSendTimer, &tv);
	m_clSendCounter.SetTimeStep(0.5f);

	OnConnect();
}

void IrcClient::OnRead(evutil_socket_t fd, short what) {
#ifdef _WIN32
	int readSize = recv(m_socket, m_stagingBuffer + m_stagingBufferSize, 
		(int)(sizeof(m_stagingBuffer)-1-m_stagingBufferSize),0);
#else // _WIN32
	ssize_t readSize = recv(m_socket, m_stagingBuffer + m_stagingBufferSize, 
		sizeof(m_stagingBuffer)-1-m_stagingBufferSize,0);
#endif // !_WIN32

	if (readSize <= 0) {
		OnDisconnect();
		return;
	}

	m_stagingBufferSize += readSize;

	m_stagingBuffer[m_stagingBufferSize] = '\0';

	char *p, *q;

	p = q = m_stagingBuffer;

	// We check the buffer size since Disconnect() can be called somewhere in ProcessLine()
	while (m_stagingBufferSize > 0 && (q = strpbrk(p,"\r\n")) != NULL) {
		*q = '\0';

		m_stagingBufferSize -= (q-p) + 1;
		if (q != p)
			ProcessLine(p);

		p = q + 1;
	}

	memmove(m_stagingBuffer, p, m_stagingBufferSize);
}

void IrcClient::OnSendTimer(evutil_socket_t fd, short what) {
	float fRate = m_clSendCounter.SampleRate();

	// TODO: Tunable for rate
	if (fRate > 2 || m_dqSendQueue.empty())
		return;

	const std::string &strMessage = m_dqSendQueue.front();

	SendRaw(strMessage.c_str(), strMessage.size());

	m_dqSendQueue.pop_front();
}

