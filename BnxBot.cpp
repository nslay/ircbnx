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
#include <sstream>
#include "Ctcp.h"
#include "Irc.h"
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
	if (std::find(m_vHomeChannels.begin(), m_vHomeChannels.end(), channel) != m_vHomeChannels.end())
		return;

	m_vHomeChannels.push_back(channel);
}

void BnxBot::DeleteHomeChannel(const std::string &channel) {
	std::vector<std::string>::iterator itr = std::find(m_vHomeChannels.begin(), m_vHomeChannels.end(), channel);

	if (itr != m_vHomeChannels.end())
		m_vHomeChannels.erase(itr);
}

bool BnxBot::LoadResponseRules(const std::string &strFilename) {
	std::ifstream responseStream(strFilename.c_str());

	if (!responseStream)
		return false;

	return m_clResponseEngine.LoadFromStream(responseStream);
}

bool BnxBot::LoadAccessList(const std::string &strFilename) {
	std::ifstream accessStream(strFilename.c_str());

	if (!accessStream)
		return false;

	return m_clAccessSystem.LoadFromStream(accessStream);
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

void BnxBot::Disconnect() {
	IrcClient::Disconnect();

	m_vChannels.clear();
	m_vIgnoredUsers.clear();
	m_clAccessSystem.ResetSessions();
}

bool BnxBot::ProcessCommand(const char *pSource, const char *pTarget, const char *pMessage) {
	if (pTarget != GetCurrentNickname())
		return false;

	IrcUser clUser(pSource);

	const std::string &strSourceNick = clUser.GetNickname();
	
	std::stringstream messageStream;
	messageStream.str(pMessage);

	std::string strCommand;

	if (!(messageStream >> strCommand))
		return false;

	if (strCommand == "login") {
		std::string strPassword;

		if (!(messageStream >> strPassword) || !m_clAccessSystem.Login(clUser, strPassword)) {
			return false;
		}

		Send("PRIVMSG %s :Your wish is my command, master.\r\n", strSourceNick.c_str());

		return true;
	}

	BnxAccessSystem::UserSession *pclSession = m_clAccessSystem.GetSession(clUser);

	if (pclSession == NULL)
		return false;

	// Commands for all access levels
	if (strCommand == "logout") {
		m_clAccessSystem.Logout(clUser);

		Send("PRIVMSG %s :Fare the well...\r\n", strSourceNick.c_str());

		return true;
	}

	if (strCommand == "say") {
		std::string strSayTarget, strLine;
		
		if (!(messageStream >> strSayTarget)) 
			return false;

		messageStream.get();

		if (!std::getline(messageStream, strLine))
			return false;

		Say(strSayTarget.c_str(), strLine.c_str());

		return true;
	}

	if (strCommand == "chatter") {
		m_bChatter = true;

		// The original BNX would ignore users indefinitely, here we'll clear the list on "chatter"
		m_vIgnoredUsers.clear();

		Send("PRIVMSG %s :Permission to speak freely, sir?\r\n", strSourceNick.c_str());

		return true;
	}

	if (strCommand == "shutup") {
		m_bChatter = false;

		Send("PRIVMSG %s :Aww... Why can't I talk anymore?\r\n", strSourceNick.c_str());

		return true;
	}

	if (pclSession->GetAccessLevel() >= 75) {
		if (strCommand == "join") {
			std::string strChannels;

			if (!(messageStream >> strChannels))
				return false;

			Send("JOIN %s\r\n", strChannels.c_str());

			return true;
		}

		if (strCommand == "part") {
			std::string strChannels;

			if (!(messageStream >> strChannels))
				return false;

			Send("PART %s\r\n", strChannels.c_str());

			return true;
		}
	}

	if (pclSession->GetAccessLevel() == 100) {
		if (strCommand == "shutdown") {
			Send("PRIVMSG %s :Sir, if you don't mind, I'll close down for a while...\r\n", strSourceNick.c_str());

			Shutdown();

			return true;
		}

		if (strCommand == "userlist") {
			const std::vector<BnxAccessSystem::UserEntry> &vUserEntries = m_clAccessSystem.GetAllEntries();

			Send("PRIVMSG %s :Access List:\r\n", strSourceNick.c_str());

			for (size_t i = 0; i < vUserEntries.size(); ++i) {
				const IrcUser &clMask = vUserEntries[i].GetHostmask();
				std::string strMask = clMask.GetHostmask();
				int iAccessLevel = vUserEntries[i].GetAccessLevel();

				Send("PRIVMSG %s :%s %d\r\n", strSourceNick.c_str(), strMask.c_str(), iAccessLevel);
			}

			Send("PRIVMSG %s :End of Access List.\r\n", strSourceNick.c_str());

			return true;
		}
	}


	return false;
}

void BnxBot::ProcessMessage(const char *pSource, const char *pTarget, const char *pMessage) {
	// Chatter isn't turned on
	if (!m_bChatter)
		return;

	IrcUser clUser(pSource);
	const std::string &strSourceNick = clUser.GetNickname();

	// Don't respond to self
	if (strSourceNick == GetCurrentNickname())
		return;

	const char *pReplyTo = strSourceNick.c_str();
	std::string strPrefix;

	if (pTarget != GetCurrentNickname()) {
		if (IrcStrCaseStr(pMessage,GetCurrentNickname().c_str()) == NULL)
			return;

		pReplyTo = pTarget;
		strPrefix = strSourceNick + ": ";
	}

	if (std::find_if(m_vIgnoredUsers.begin(), m_vIgnoredUsers.end(), MaskMatches(clUser)) != m_vIgnoredUsers.end())
		return;

	if (IrcMatch("*shut*up*", pMessage)) {
		m_vIgnoredUsers.push_back(IrcUser("*","*",clUser.GetHostname()));

		Send("PRIVMSG %s :%sOK, I won't talk to you anymore.\r\n", pReplyTo, strPrefix.c_str());
		return;
	}

	const std::string &strResponse = m_clResponseEngine.ComputeResponse(pMessage);

	if (strResponse[0] == '/')
		strPrefix.clear();

	std::string strFormattedResponse = strPrefix + strResponse;

	size_t findPos = 0;
	while ((findPos = strFormattedResponse.find("%s", findPos)) != std::string::npos) {
		strFormattedResponse.replace(findPos, 2, strSourceNick);
		findPos += strSourceNick.size();
	}
	
	Say(pReplyTo, strFormattedResponse.c_str());
}

void BnxBot::Say(const char *pTarget, const char *pMessage) {
	CtcpEncoder clEncoder;

	if (pMessage[0] == '/') {
		std::stringstream ss;
		ss.str(pMessage);

		std::string strCommand;

		ss >> strCommand;

		if (strCommand == "/me") {

			std::string strLine;

			ss.get();

			if (!std::getline(ss,strLine))
				return;

			if (!clEncoder.Encode(MakeCtcpMessage("ACTION",strLine.c_str())))
				return;

			pMessage = clEncoder.GetRaw();
		}
		else {
			// Unrecognized command
			return;
		}
	}

	Send("PRIVMSG %s :%s\r\n", pTarget, pMessage);
}

BnxChannel * BnxBot::GetChannel(const char *pChannel) {
	std::vector<BnxChannel>::iterator itr;
	itr = std::find(m_vChannels.begin(), m_vChannels.end(), pChannel);

	if (itr == m_vChannels.end())
		return NULL;

	return &(*itr);
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

	for (size_t i = 0; i < m_vHomeChannels.size(); ++i) {
		Send("JOIN %s\r\n", m_vHomeChannels[i].c_str());
	}
}

void BnxBot::OnNumeric(const char *pSource, int numeric, const char *pParams[], unsigned int numParams) {
	IrcClient::OnNumeric(pSource, numeric, pParams, numParams);

	const char *pChannel = NULL, *pTrailing = NULL, *pUsername = NULL, 
		*pHostname = NULL, *pNickname = NULL;
	BnxChannel *pclChannel = NULL;
	BnxChannel::Member *pclMember = NULL;

	switch (numeric) {
	case RPL_NAMEREPLY:
		if (!strcmp(pParams[1],"=") || !strcmp(pParams[1],"*") || !strcmp(pParams[1],"@")) {
			// RFC2812
			pChannel = pParams[2];
			pTrailing = pParams[3];
		}
		else {
			// RFC1459
			pChannel = pParams[1];
			pTrailing = pParams[2];
		}

		pclChannel = GetChannel(pChannel);

		if (pclChannel == NULL)
			return;

		pclChannel->AddMembers(pTrailing);
		break;
	case RPL_WHOREPLY:
		pChannel = pParams[1];
		pUsername = pParams[2];
		pHostname = pParams[3];
		pNickname = pParams[5];

		pclChannel = GetChannel(pChannel);

		if (pclChannel == NULL)
			return;

		pclMember = pclChannel->GetMember(pNickname);

		// What?
		if (pclMember == NULL)
			return;

		pclMember->GetUser().SetUsername(pUsername);
		pclMember->GetUser().SetHostname(pHostname);

		std::cout << "Updated hostmask: " << pclMember->GetUser().GetHostmask() << std::endl;

		break;
	}
}

void BnxBot::OnNick(const char *pSource, const char *pNewNick) {
	IrcClient::OnNick(pSource, pNewNick);

	IrcUser clUser(pSource);

	for (size_t i = 0; i < m_vChannels.size(); ++i) {
		BnxChannel::Member *pclMember = m_vChannels[i].GetMember(clUser.GetNickname());

		if (pclMember == NULL)
			continue;

		std::cout << "Setting nick: " << clUser.GetNickname() << " -> " << pNewNick << std::endl;

		pclMember->GetUser().SetNickname(pNewNick);
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
		pMessage = (const char *)message.data;

		if (!strcmp(pTag,"VERSION")) {
			OnCtcpVersion(pSource, pTarget);
		}
		else if (!strcmp(pTag,"ACTION")) {
			if (message.dataSize > 0)
				OnCtcpAction(pSource, pTarget, pMessage);
		}

		return;
	}

	if (message.dataSize == 0)
		return;

	pMessage = (const char *)message.data;

	if (ProcessCommand(pSource, pTarget, pMessage))
		return;

	// Finally, process the message text
	ProcessMessage(pSource, pTarget, pMessage);
}

void BnxBot::OnJoin(const char *pSource, const char *pChannel) {
	IrcClient::OnJoin(pSource, pChannel);

	IrcUser clUser(pSource);

	BnxChannel *pclChannel = NULL;

	if (clUser.GetNickname() == GetCurrentNickname()) {
		m_vChannels.push_back(BnxChannel(pChannel));
		pclChannel = &m_vChannels.back();

		// Get hostmask information
		Send("WHO %s\r\n", pChannel);
	}
	else {
		pclChannel = GetChannel(pChannel);
	}
	

	// What? Must be me on RFC2812
	if (pclChannel == NULL)
		return;
	
	pclChannel->AddMember(clUser);
}

void BnxBot::OnPart(const char *pSource, const char *pChannel, const char *pReason) {
	IrcClient::OnPart(pSource, pChannel, pReason);

	IrcUser clUser(pSource);

	BnxChannel *pclChannel = NULL;

	if (clUser.GetNickname() == GetCurrentNickname()) {
		std::vector<BnxChannel>::iterator itr;
		itr = std::find(m_vChannels.begin(), m_vChannels.end(), pChannel);

		if (itr != m_vChannels.end())
			m_vChannels.erase(itr);

		return;
	}

	// What?
	pclChannel = GetChannel(pChannel);

	if (pclChannel == NULL)
		return;

	pclChannel->DeleteMember(clUser.GetNickname());
}

void BnxBot::OnQuit(const char *pSource, const char *pReason) {
	IrcClient::OnQuit(pSource, pReason);

	IrcUser clUser(pSource);

	for (size_t i = 0; i < m_vChannels.size(); ++i)
		m_vChannels[i].DeleteMember(clUser.GetNickname());
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

