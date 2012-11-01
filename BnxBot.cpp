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

void BnxBot::AddHomeChannel(const std::string &strChannel) {
	if (std::find(m_vHomeChannels.begin(), m_vHomeChannels.end(), strChannel) != m_vHomeChannels.end())
		return;

	m_vHomeChannels.push_back(strChannel);
}

void BnxBot::DeleteHomeChannel(const std::string &strChannel) {
	std::vector<std::string>::iterator itr = std::find(m_vHomeChannels.begin(), m_vHomeChannels.end(), strChannel);

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
	m_clAccessSystem.SetAccessListFile(strFilename);

	return m_clAccessSystem.Load();
}

bool BnxBot::LoadShitList(const std::string &strFilename) {
	m_clShitList.SetShitListFile(strFilename);

	return m_clShitList.Load();
}

void BnxBot::StartUp() {
	if (m_pConnectTimer != NULL)
		return;

	m_pConnectTimer = evtimer_new(GetEventBase(), &Dispatch<&BnxBot::OnConnectTimer>, this);
	m_pFloodTimer = event_new(GetEventBase(), -1, EV_PERSIST, &Dispatch<&BnxBot::OnFloodTimer>, this);

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

	if (m_pFloodTimer != NULL) {
		event_del(m_pFloodTimer);
		event_free(m_pFloodTimer);
		m_pFloodTimer = NULL;
	}
}

void BnxBot::Disconnect() {
	IrcClient::Disconnect();

	if (m_pFloodTimer != NULL)
		event_del(m_pFloodTimer);

	m_vCurrentChannels.clear();
	m_vSquelchedUsers.clear();
	m_clAccessSystem.ResetSessions();
	m_clFloodDetector.Reset();
}

BnxBot::ChannelIterator BnxBot::ChannelBegin() {
	return m_vCurrentChannels.begin();
}

BnxBot::ChannelIterator BnxBot::ChannelEnd() {
	return m_vCurrentChannels.end();
}

bool BnxBot::ProcessCommand(const char *pSource, const char *pTarget, const char *pMessage) {
	if (pTarget != GetCurrentNickname())
		return false;

	IrcUser clUser(pSource);

	std::stringstream messageStream;
	messageStream.str(pMessage);

	std::string strCommand;

	if (!(messageStream >> strCommand))
		return false;

	if (strCommand == "login") {
		std::string strPassword;

		return (messageStream >> strPassword) &&
			OnCommandLogin(clUser, strPassword);
	}

	BnxAccessSystem::SessionIterator sessionItr = m_clAccessSystem.GetSession(clUser);

	if (sessionItr == m_clAccessSystem.SessionEnd())
		return false;

	if (strCommand == "logout") {
		return OnCommandLogout(*sessionItr);
	}

	if (strCommand == "say") {
		std::string strSayTarget, strMessage;
		
		if (!(messageStream >> strSayTarget)) 
			return false;

		messageStream.get();

		if (!std::getline(messageStream, strMessage))
			return false;

		return OnCommandSay(*sessionItr, strSayTarget, strMessage);
	}

	if (strCommand == "chatter") {
		return OnCommandChatter(*sessionItr);
	}

	if (strCommand == "shutup") {
		return OnCommandShutUp(*sessionItr);
	}

	if (strCommand == "join") {
		std::string strChannels;
		return (messageStream >> strChannels) &&
			OnCommandJoin(*sessionItr, strChannels);
	}

	if (strCommand == "part") {
		std::string strChannels;
		return (messageStream >> strChannels) &&
			OnCommandPart(*sessionItr, strChannels);
	}

	if (strCommand == "shutdown") {
		return OnCommandShutdown(*sessionItr);
	}

	if (strCommand == "userlist") {
		return OnCommandUserList(*sessionItr);
	}

	if (strCommand == "where") {
		return OnCommandWhere(*sessionItr);
	}

	if (strCommand == "kick") {
		std::string strChannel, strHostmask, strReason;

		if (!(messageStream >> strChannel >> strHostmask))
			return false;

		messageStream.get();

		std::getline(messageStream, strReason);

		return OnCommandKick(*sessionItr, strChannel, strHostmask, strReason);
	}

	if (strCommand == "squelch" || strCommand == "ignore") {
		std::string strHostmask;

		return (messageStream >> strHostmask) &&
			OnCommandSquelch(*sessionItr, strHostmask);
	}

	// Original BNX didn't seem to have an unignore command despite having an ignore command
	if (strCommand == "unsquelch") {
		std::string strHostmask;

		return (messageStream >> strHostmask) &&
			OnCommandUnsquelch(*sessionItr, strHostmask);
	}

	if (strCommand == "useradd") {
		std::string strHostmask, strPassword;
		int iAccessLevel;
		
		return (messageStream >> strHostmask >> iAccessLevel >> strPassword) &&
			OnCommandUserAdd(*sessionItr, strHostmask, iAccessLevel, strPassword);
	}

	if (strCommand == "userdel") {
		std::string strHostmask;

		return (messageStream >> strHostmask) &&
			OnCommandUserDel(*sessionItr, strHostmask);
	}

	if (strCommand == "shitadd") {
		std::string strHostmask;

		return (messageStream >> strHostmask) &&
			OnCommandShitAdd(*sessionItr, strHostmask);

	}

	if (strCommand == "shitdel") {
		std::string strHostmask;

		return (messageStream >> strHostmask) &&
			OnCommandShitDel(*sessionItr, strHostmask);

	}

	if (strCommand == "shitlist") {
		return OnCommandShitList(*sessionItr);
	}

	if (strCommand == "nick") {
		std::string strNickname;

		return (messageStream >> strNickname) &&
			OnCommandNick(*sessionItr, strNickname);
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
		ChannelIterator channelItr = GetChannel(pTarget);

		if ((channelItr != ChannelEnd() && channelItr->GetSize() != 2) && 
			IrcStrCaseStr(pMessage,GetCurrentNickname().c_str()) == NULL) {
			return;
		}

		pReplyTo = pTarget;
		strPrefix = strSourceNick;
		strPrefix += ": ";
	}

	if (IsSquelched(clUser))
		return;

	if (IrcMatch("*shut*up*", pMessage)) {
		Squelch(IrcUser("*", "*", clUser.GetHostname()));

		Send("PRIVMSG %s :%sOK, I won't talk to you anymore.\r\n", pReplyTo, strPrefix.c_str());
		return;
	}

	m_clFloodDetector.Hit(clUser);

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

		if (strCommand == "/me" || strCommand == "/action") {

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

BnxBot::ChannelIterator BnxBot::GetChannel(const char *pChannel) {
	return std::find(ChannelBegin(), ChannelEnd(), pChannel);
}

bool BnxBot::IsSquelched(const IrcUser &clUser) {
	return std::find_if(m_vSquelchedUsers.begin(), 
				m_vSquelchedUsers.end(), 
				MaskMatches(clUser)) != m_vSquelchedUsers.end();
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

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// TODO: Make this configurable
	m_clFloodDetector.SetThreshold(3.0f);
	m_clFloodDetector.SetTimeStep(1.0f);

	event_add(m_pFloodTimer, &tv);

	for (size_t i = 0; i < m_vHomeChannels.size(); ++i) {
		Send("JOIN %s\r\n", m_vHomeChannels[i].c_str());
	}
}

void BnxBot::OnNumeric(const char *pSource, int numeric, const char *pParams[], unsigned int numParams) {
	IrcClient::OnNumeric(pSource, numeric, pParams, numParams);

	const char *pChannel = NULL, *pTrailing = NULL, *pUsername = NULL, 
		*pHostname = NULL, *pNickname = NULL, *pMode = NULL;
	ChannelIterator channelItr;

	switch (numeric) {
	case RPL_NAMEREPLY:
		// Reference by last parameter since RFC2812 adds extra parameter
		pTrailing = pParams[numParams-1];
		pChannel = pParams[numParams-2];

		if (GetChannel(pChannel) != ChannelEnd())
			return;

		// Check if we joined a channel

		// RFC1459 only guarantees RPL_NAMEREPLY or RPL_TOPIC (with which there isn't always a topic)
		{
			std::stringstream nameStream;
			nameStream.str(pTrailing);

			std::string strNickname;

			while (nameStream >> strNickname) {
				size_t p;
				for (p = 0; p < strNickname.size() &&
						!isalpha(strNickname[p]) &&
						!IrcIsSpecial(strNickname[p]); ++p);

				if (p < strNickname.size() && GetCurrentNickname() == strNickname.substr(p)) {
					AddChannel(pChannel);

					// Now really collect useful information
					Send("WHO %s\r\n", pChannel);
					return;
				}
			}
		}

		break;
	case RPL_WHOREPLY:
		pChannel = pParams[1];
		pUsername = pParams[2];
		pHostname = pParams[3];
		pNickname = pParams[5];
		pMode = pParams[6];

		channelItr = GetChannel(pChannel);

		if (channelItr == ChannelEnd())
			return;

		channelItr->AddMember(IrcUser(pNickname,pUsername,pHostname));

		if (GetCurrentNickname() == pNickname && strchr(pMode,'@') != NULL)
			channelItr->SetOperator(true);

		break;
	}
}

void BnxBot::OnNick(const char *pSource, const char *pNewNick) {
	IrcClient::OnNick(pSource, pNewNick);

	IrcUser clUser(pSource);

	for (size_t i = 0; i < m_vCurrentChannels.size(); ++i) {
		BnxChannel::Iterator itr = m_vCurrentChannels[i].GetMember(clUser.GetNickname());

		if (itr != m_vCurrentChannels[i].End())
			itr->GetUser().SetNickname(pNewNick);
	}
}

void BnxBot::OnKick(const char *pSource, const char *pChannel, const char *pUser, const char *pReason) {
	IrcClient::OnKick(pSource, pChannel, pUser, pReason);

	ChannelIterator channelItr = GetChannel(pChannel);

	// What?
	if (channelItr == ChannelEnd())
		return;

	IrcUser clUser(pSource);

	if (pUser == GetCurrentNickname()) {
		m_clShitList.AddMask(IrcUser("*","*",clUser.GetHostname()));
		m_clShitList.Save();

		// TODO: Log that this user was shitlisted

		DeleteChannel(channelItr);
		return;
	}

	channelItr->DeleteMember(pUser);
}

void BnxBot::OnPrivmsg(const char *pSource, const char *pTarget, const char *pMessage) {
	IrcClient::OnPrivmsg(pSource, pTarget, pMessage);

	CtcpDecoder clDecoder(pMessage);
	CtcpMessage message;

	// Get first non-empty message
	while (clDecoder.Decode(message) && 
		(message.tagSize == 0 && message.dataSize == 0));

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
	}
	else if (message.dataSize > 0) {
		pMessage = (const char *)message.data;

		if (ProcessCommand(pSource, pTarget, pMessage))
			return;

		// Finally, process the message text
		ProcessMessage(pSource, pTarget, pMessage);
	}
}

void BnxBot::OnJoin(const char *pSource, const char *pChannel) {
	IrcClient::OnJoin(pSource, pChannel);

	ChannelIterator channelItr = GetChannel(pChannel);

	// What? Must be me on RFC2812
	if (channelItr == ChannelEnd())
		return;

	IrcUser clUser(pSource);

	channelItr->AddMember(clUser);

	if (channelItr->GetSize() == 2)
		Send("PRIVMSG %s :Hi!\r\n", pChannel);
}

void BnxBot::OnPart(const char *pSource, const char *pChannel, const char *pReason) {
	IrcClient::OnPart(pSource, pChannel, pReason);

	IrcUser clUser(pSource);

	if (clUser.GetNickname() == GetCurrentNickname()) {
		DeleteChannel(pChannel);
		return;
	}

	ChannelIterator channelItr = GetChannel(pChannel);

	// What?
	if (channelItr == ChannelEnd())
		return;

	channelItr->DeleteMember(clUser.GetNickname());
}

void BnxBot::OnMode(const char *pSource, const char *pTarget, const char *pMode, const char *pParams[], unsigned int numParams) {
	IrcClient::OnMode(pSource, pTarget, pMode, pParams, numParams);

	if (pTarget != GetCurrentNickname()) {
		// Targetting channel

		ChannelIterator channelItr = GetChannel(pTarget);

		// What?
		if (channelItr == ChannelEnd())
			return;

		// Get traits from IRC (needed to process modes)
		const IrcTraits &clTraits = GetIrcTraits();

		bool bSetMode = (*pMode++ != '-');

		for ( ;*pMode != '\0'; ++pMode) {
			switch (*pMode) {
			case 'o':
				if (*pParams == GetCurrentNickname())
					channelItr->SetOperator(bSetMode);

				++pParams;
				--numParams;
				break;
			default:
				// Process all other modes
				switch (clTraits.ClassifyChanMode(*pMode)) {
				case IrcTraits::TYPE_A:
				case IrcTraits::TYPE_B:
					++pParams;
					--numParams;
					break;
				case IrcTraits::TYPE_C:
					if (bSetMode) {
						++pParams;
						--numParams;
					}
					break;
				case IrcTraits::TYPE_D:
					break;
				}
			}
		}

		if (numParams != 0) {
			std::cerr << "Didn't process modes correctly!" << std::endl;
		}
	}
}

void BnxBot::OnQuit(const char *pSource, const char *pReason) {
	IrcClient::OnQuit(pSource, pReason);

	IrcUser clUser(pSource);

	for (size_t i = 0; i < m_vCurrentChannels.size(); ++i)
		m_vCurrentChannels[i].DeleteMember(clUser.GetNickname());
}

void BnxBot::OnCtcpAction(const char *pSource, const char *pTarget, const char *pMessage) {
	ProcessMessage(pSource, pTarget, pMessage);
}

void BnxBot::OnCtcpVersion(const char *pSource, const char *pTarget) {
	IrcUser clUser(pSource);

	if (IsSquelched(clUser))
		return;

	m_clFloodDetector.Hit(clUser);

	CtcpEncoder clEncoder;
	clEncoder.Encode(MakeCtcpMessage("VERSION", "IRCBNX Chatterbot"));

	Send("NOTICE %s :%s\r\n", clUser.GetNickname().c_str(), clEncoder.GetRaw());
}

bool BnxBot::OnCommandLogin(const IrcUser &clUser, const std::string &strPassword) {

	if (m_clAccessSystem.Login(clUser, strPassword)) {
		Send("PRIVMSG %s :Your wish is my command, master.\r\n", clUser.GetNickname().c_str());

		return true;
	}

	return false;
}

bool BnxBot::OnCommandLogout(UserSession &clSession) {
	const IrcUser &clUser = clSession.GetUser();

	Send("PRIVMSG %s :Fare the well...\r\n", clUser.GetNickname().c_str());

	m_clAccessSystem.Logout(clUser);

	// XXX: clUser and clSession are dead references now!

	return true;
}

bool BnxBot::OnCommandSay(UserSession &clSession, const std::string &strTarget, const std::string &strMessage) {

	Say(strTarget.c_str(), strMessage.c_str());

	return true;
}

bool BnxBot::OnCommandChatter(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	m_bChatter = true;

	// The original BNX would ignore users indefinitely, here we'll clear the list on "chatter"
	m_vSquelchedUsers.clear();

	Send("PRIVMSG %s :Permission to speak freely, sir?\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandShutUp(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	m_bChatter = false;

	Send("PRIVMSG %s :Aww... Why can't I talk anymore?\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandJoin(UserSession &clSession, const std::string &strChannels) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	Send("JOIN %s\r\n", strChannels.c_str());

	return true;
}

bool BnxBot::OnCommandPart(UserSession &clSession, const std::string &strChannels) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	Send("PART %s\r\n", strChannels.c_str());

	return true;
}

bool BnxBot::OnCommandShutdown(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	SendNow("PRIVMSG %s :Sir, if you don't mind, I'll close down for a while...\r\n", clUser.GetNickname().c_str());

	Shutdown();

	return true;
}

bool BnxBot::OnCommandUserList(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	BnxAccessSystem::ConstEntryIterator entryItr;

	Send("PRIVMSG %s :Access List:\r\n", clUser.GetNickname().c_str());

	for (entryItr = m_clAccessSystem.EntryBegin(); entryItr != m_clAccessSystem.EntryEnd(); ++entryItr) {
		const IrcUser &clMask = entryItr->GetHostmask();
		std::string strMask = clMask.GetHostmask();
		int iAccessLevel = entryItr->GetAccessLevel();

		Send("PRIVMSG %s :%s %d\r\n", clUser.GetNickname().c_str(), strMask.c_str(), iAccessLevel);
	}

	Send("PRIVMSG %s :End of Access List.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandWhere(UserSession &clSession) {
	const IrcUser &clUser = clSession.GetUser();

	std::string strChannels;

	if (m_vCurrentChannels.empty()) {
		// NOTE: The Void was a no-where channel on Battle.net
		strChannels = "The Void";
	}
	else {
		strChannels = m_vCurrentChannels[0].GetName();

		for (size_t i = 1; i < m_vCurrentChannels.size(); ++i) {
			strChannels += ", ";
			strChannels += m_vCurrentChannels[i].GetName();
		}
	}

	Send("PRIVMSG %s :I am in channels: %s\r\n", clUser.GetNickname().c_str(), strChannels.c_str());

	return true;
}

bool BnxBot::OnCommandKick(UserSession &clSession, const std::string &strChannel, 
				const std::string &strHostmask, const std::string &strReason) {
	if (clSession.GetAccessLevel() < 60)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	if (!channelItr->IsOperator()) {
		Send("PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	if (strReason.empty())
		Send("KICK %s %s\r\n", strChannel.c_str(), strHostmask.c_str());
	else
		Send("KICK %s %s :%s\r\n", strChannel.c_str(), strHostmask.c_str(), strReason.c_str());

	Send("PRIVMSG %s :OK, kicked his ass.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandSquelch(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Squelch(IrcUser(strHostmask));

	Send("PRIVMSG %s :OK, ignoring...\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandUnsquelch(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Unsquelch(IrcUser(strHostmask));

	Send("PRIVMSG %s :OK, unignoring...\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandUserAdd(UserSession &clSession, const std::string &strHostmask, 
				int iAccessLevel, const std::string &strPassword) {
	if(clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	IrcUser clMask(strHostmask);

	m_clAccessSystem.AddUser(clMask, iAccessLevel, strPassword);
	m_clAccessSystem.Save();

	Send("PRIVMSG %s :Added %s, level %d, pass %s\r\n", clUser.GetNickname().c_str(), 
		clMask.GetHostmask().c_str(), iAccessLevel, strPassword.c_str());

	return true;
}

bool BnxBot::OnCommandUserDel(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();
	IrcUser clMask(strHostmask);

	BnxAccessSystem::EntryIterator entryItr = m_clAccessSystem.GetEntry(clMask);

	if (entryItr == m_clAccessSystem.EntryEnd()) {
		Send("PRIVMSG %s :Cannot delete %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());
		return true;
	}

	if (clSession.GetAccessLevel() <= entryItr->GetAccessLevel()) {
		Send("PRIVMSG %s :Insufficient privilege.\r\n", clUser.GetNickname().c_str());
		return true;
	}

	m_clAccessSystem.DeleteUser(entryItr);
	m_clAccessSystem.Save();

	Send("PRIVMSG %s :Deleted %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());

	return true;
}

bool BnxBot::OnCommandShitAdd(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	IrcUser clMask(strHostmask);

	m_clShitList.AddMask(clMask);
	m_clShitList.Save();

	Send("PRIVMSG %s :%s shitlisted.\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());

	return true;
}

bool BnxBot::OnCommandShitDel(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	IrcUser clMask(strHostmask);

	if (m_clShitList.DeleteMask(clMask)) {
		Send("PRIVMSG %s :Deleted %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());
		m_clShitList.Save();
	}
	else {
		Send("PRIVMSG %s :Cannot delete %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());
	}

	return true;
}

bool BnxBot::OnCommandShitList(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Send("PRIVMSG %s :Shit List:\r\n", clUser.GetNickname().c_str());

	BnxShitList::ConstIterator itr;

	for (itr = m_clShitList.Begin(); itr != m_clShitList.End(); ++itr)
		Send("PRIVMSG %s :%s\r\n", clUser.GetNickname().c_str(), itr->GetHostmask().c_str());

	Send("PRIVMSG %s :End of Shit List.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandNick(UserSession &clSession, const std::string &strNickname) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Send("NICK %s\r\n", strNickname.c_str());

	return true;
}

void BnxBot::AddChannel(const char *pChannel) {
	if (GetChannel(pChannel) != ChannelEnd())
		return;

	m_vCurrentChannels.push_back(BnxChannel(pChannel));
}

void BnxBot::DeleteChannel(const char *pChannel) {
	ChannelIterator itr = GetChannel(pChannel);

	if (itr != ChannelEnd())
		DeleteChannel(itr);
}

BnxBot::ChannelIterator BnxBot::DeleteChannel(ChannelIterator channelItr) {
	return m_vCurrentChannels.erase(channelItr);
}

void BnxBot::Squelch(const IrcUser &clUser) {
	if (!IsSquelched(clUser))
		m_vSquelchedUsers.push_back(clUser);
}

void BnxBot::Unsquelch(const IrcUser &clUser) {
	std::vector<IrcUser>::iterator itr;

	itr = std::find(m_vSquelchedUsers.begin(), m_vSquelchedUsers.end(), clUser);

	if (itr != m_vSquelchedUsers.end())
		m_vSquelchedUsers.erase(itr);
}

void BnxBot::OnConnectTimer(int fd, short what) {
	std::cout << "OnConnectTimer" << std::endl;
	Connect(m_strServer, m_strPort);
}

void BnxBot::OnFloodTimer(int fd, short what) {
	std::vector<IrcUser> vFlooders;

	m_clFloodDetector.Detect(vFlooders);

	for (size_t i = 0; i < vFlooders.size(); ++i)
		Squelch(IrcUser("*","*",vFlooders[i].GetHostname()));
}

