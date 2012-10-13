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

#include <algorithm>
#include <fstream>
#include "Ctcp.h"
#include "IrcString.h"
#include "IrcUser.h"
#include "BnxBot.h"

BnxBot::~BnxBot() {
	Shutdown();
}

void BnxBot::SetServerAndPort(const std::string &strServer, const std::string &strPort) {
	m_strServer = strServer;
	m_strPort = strPort;
}

void BnxBot::AddHomeChannel(const std::string &channel) {
	if (std::find(m_homeChannels.begin(), m_homeChannels.end(), channel) != m_homeChannels.end())
		return;

	m_homeChannels.push_back(channel);
}

void BnxBot::DeleteHomeChannel(const std::string &channel) {
	std::vector<std::string>::iterator itr = std::find(m_homeChannels.begin(), m_homeChannels.end(), channel);

	if (itr != m_homeChannels.end())
		m_homeChannels.erase(itr);
}

bool BnxBot::LoadResponseRules(const std::string &strFilename) {
	std::ifstream responseStream(strFilename.c_str());

	if (!responseStream)
		return false;

	return m_clResponseEngine.LoadFromStream(responseStream);
}

void BnxBot::StartUp() {
	if (m_pConnectTimer != NULL)
		return;

	m_pConnectTimer = evtimer_new(GetEventBase(), &Dispatch<&BnxBot::OnConnectTimer>, this);

	struct timeval tv;
	tv.tv_sec = tv.tv_usec = 0;

	evtimer_add(m_pConnectTimer, &tv);
}

void BnxBot::Shutdown() {
	Disconnect();

	if (m_pConnectTimer != NULL) {
		evtimer_del(m_pConnectTimer);
		event_free(m_pConnectTimer);
		m_pConnectTimer = NULL;
	}
}

void BnxBot::ProcessMessage(const char *pSource, const char *pTarget, const char *pMessage) {
	char aResponseBuffer[512] = "";

	IrcUser clUser(pSource);
	const std::string &strSourceNick = clUser.GetNickname();

	if (pTarget == GetCurrentNickname()) {
		const std::string &strResponse = m_clResponseEngine.ComputeResponse(pMessage);
		snprintf(aResponseBuffer, sizeof(aResponseBuffer), strResponse.c_str(), strSourceNick.c_str());
		Send("PRIVMSG %s :%s\r\n", strSourceNick.c_str(), aResponseBuffer);
	}
	else if (IrcStrCaseStr(pMessage,GetCurrentNickname().c_str()) != NULL) {
		const std::string &strResponse = m_clResponseEngine.ComputeResponse(pMessage);
		snprintf(aResponseBuffer, sizeof(aResponseBuffer), strResponse.c_str(), strSourceNick.c_str());
		Send("PRIVMSG %s :%s: %s\r\n", pTarget, strSourceNick.c_str(), aResponseBuffer);
	}
}

void BnxBot::OnConnect() {
	std::cout << "OnConnect" << std::endl;
	IrcClient::OnConnect();
}

void BnxBot::OnDisconnect() {
	std::cout << "OnDisconnect" << std::endl;
	IrcClient::OnDisconnect();


	if (m_pConnectTimer == NULL)
		return;

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	evtimer_add(m_pConnectTimer, &tv);
}

void BnxBot::OnRegistered() {
	IrcClient::OnRegistered();

	for (size_t i = 0; i < m_homeChannels.size(); ++i) {
		Send("JOIN %s\r\n", m_homeChannels[i].c_str());
	}
}

void BnxBot::OnPrivmsg(const char *pSource, const char *pTarget, const char *pMessage) {
	IrcClient::OnPrivmsg(pSource, pTarget, pMessage);

	CtcpDecoder clDecoder(pMessage);
	CtcpMessage message;

	while (clDecoder.Decode(message)) {
		// Ignore empty CTCP messages
		if (message.tagSize == 0 && message.dataSize == 0)
			continue;

		// Let's just process the first one
		break;
	}

	if (message.tagSize > 0) {
		const char *pTag = (const char *)message.tag;

		if (!strcmp(pTag,"VERSION")) {
			OnCtcpVersion(pSource, pTarget);
		}
		else if (!strcmp(pTag,"ACTION")) {
			if (message.dataSize > 0)
				OnCtcpAction(pSource, pTarget, (const char *)message.data);
		}

		return;
	}

	if (message.dataSize == 0)
		return;

	pMessage = (const char *)message.data;

	// Finally, process the message text
	ProcessMessage(pSource, pTarget, pMessage);
}

void BnxBot::OnCtcpAction(const char *pSource, const char *pTarget, const char *pMessage) {
	ProcessMessage(pSource, pTarget, pMessage);
}

void BnxBot::OnCtcpVersion(const char *pSource, const char *pTarget) {
	CtcpEncoder clEncoder;

	clEncoder.Encode(MakeCtcpMessage("VERSION", "IRCBNX Chatterbot"));

	IrcUser clUser(pSource);
	const std::string &strSourceNick = clUser.GetNickname();

	Send("NOTICE %s :%s\r\n", strSourceNick.c_str(), clEncoder.GetRaw());
}

void BnxBot::OnConnectTimer(int fd, short what) {
	std::cout << "OnConnectTimer" << std::endl;
	Connect(m_strServer, m_strPort);
}

