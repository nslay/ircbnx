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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sstream>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
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
	if (m_socket != -1)
		Disconnect();

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (m_socket == -1) {
		perror("socket");
		return false;
	}

	int flags = fcntl(m_socket, F_GETFL);

	if (fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("fcntl");

		close(m_socket);
		m_socket = -1;

		return false;
	}

	struct addrinfo hints, *pResults = NULL;
	memset(&hints, 0, sizeof(hints));

	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int e = getaddrinfo(strServer.c_str(), strPort.c_str(), &hints, &pResults);
	if (e != 0) {
		fprintf(stderr, "%s\n", gai_strerror(e));

		close(m_socket);
		m_socket = -1;

		return false;
	}

	e = connect(m_socket, pResults->ai_addr, pResults->ai_addrlen);

	if (e == -1 && errno != EINPROGRESS) {
		perror("connect");

		freeaddrinfo(pResults);

		close(m_socket);
		m_socket = -1;

		return false;
	}

	freeaddrinfo(pResults);

	// XXX: Handle errors?
	m_pWriteEvent = event_new(m_pEventBase, m_socket, EV_WRITE, &Dispatch<&IrcClient::OnWrite>, this);
	m_pReadEvent = event_new(m_pEventBase, m_socket, EV_READ | EV_PERSIST, &Dispatch<&IrcClient::OnRead>, this);

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

	// This is REALLY important since this can be called while OnRead() is still processing messages
	m_stagingBufferSize = 0;

	if (m_socket != -1) {
		close(m_socket);
		m_socket = -1;
	}

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
}

void IrcClient::Send(const char *pFormat, ...) {
	char buff[513];

	va_list ap;
	va_start(ap, pFormat);
	int buffSize = vsnprintf(buff, sizeof(buff), pFormat, ap);
	va_end(ap);

	printf("-> %s", buff);

	SendRaw(buff, buffSize);
}

void IrcClient::SendRaw(const void *pData, size_t dataSize) {
	if (m_socket == -1)
		return;

	send(m_socket, pData, dataSize, 0);
}

void IrcClient::OnConnect() {
	Send("NICK %s\r\n", m_strNickname.c_str());
	Send("USER %s localhost localhost :%s\r\n", m_strUsername.c_str(), m_strRealName.c_str());

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
	Send("PONG :%s\r\n", pServer);
}

void IrcClient::OnPong(const char *pServer1, const char *pServer2) {
}

void IrcClient::OnError(const char *pMessage) {

}

void IrcClient::OnWallops(const char *pSource, const char *pMessage) {

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

void IrcClient::OnWrite(int fd, short what) {
	event_add(m_pReadEvent, NULL);

	OnConnect();
}

void IrcClient::OnRead(int fd, short what) {
	ssize_t readSize = recv(m_socket, m_stagingBuffer + m_stagingBufferSize, sizeof(m_stagingBuffer)-1-m_stagingBufferSize,0);

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

