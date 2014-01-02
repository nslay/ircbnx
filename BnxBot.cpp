/*-
 * Copyright (c) 2012-2013 Nathan Lay (nslay@users.sourceforge.net)
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
#include <functional>
#include <fstream>
#include <sstream>
#include <ctime>
#include "Ctcp.h"
#include "Irc.h"
#include "IrcString.h"
#include "IrcUser.h"
#include "BnxBot.h"
#include "BnxDriver.h"

#ifdef _MSC_VER
// Disable warnings about safe string functions
#pragma warning(disable:4996)
#endif // _MSC_VER

std::string BnxBot::GetVersionString() {
	std::stringstream versionStream;

	versionStream << "IRCBNX Chatterbot " << MAJOR_VERSION << '.' << MINOR_VERSION << '.' << PATCH_LEVEL  << " - " <<
		"http://sourceforge.net/projects/ircbnx/";

	return versionStream.str();
}

BnxBot::BnxBot() {
	m_strLogFile = "bot.log";
	m_bChatter = true;

	m_clConnectTimer = IrcEvent::Bind(&BnxBot::OnConnectTimer, this);
	m_clFloodTimer = IrcEvent::Bind(&BnxBot::OnFloodTimer, this);
	m_clVoteBanTimer = IrcEvent::Bind(&BnxBot::OnVoteBanTimer, this);
	m_clChannelsTimer = IrcEvent::Bind(&BnxBot::OnChannelsTimer, this);
	m_clAntiIdleTimer = IrcEvent::Bind(&BnxBot::OnAntiIdleTimer, this);
	m_clSeenListTimer = IrcEvent::Bind(&BnxBot::OnSeenListTimer, this);
}

BnxBot::~BnxBot() {
	Shutdown();
}

void BnxBot::SetProfileName(const std::string &strProfileName) {
	m_strProfileName = strProfileName;
}

const std::string & BnxBot::GetProfileName() const {
	return m_strProfileName;
}

void BnxBot::SetServerAndPort(const std::string &strServer, const std::string &strPort) {
	m_strServer = strServer;
	m_strPort = strPort;
}

void BnxBot::SetNickServAndPassword(const std::string &strNickServ, const std::string &strPassword) {
	m_strNickServ = strNickServ;
	m_strNickServPassword = strPassword;
}

void BnxBot::SetHomeChannels(const std::string &strChannels) {
	m_vHomeChannels.clear();
	AddHomeChannels(strChannels);
}

void BnxBot::AddHomeChannels(const std::string &strChannels) {
	std::stringstream channelStream(strChannels);

	std::string strChannel;
	while (std::getline(channelStream,strChannel,',')) {
		if (strChannel.empty())
			continue;

		if (std::find_if(m_vHomeChannels.cbegin(), m_vHomeChannels.cend(),
				[&strChannel](const std::string &strOther) {
					return !IrcStrCaseCmp(strChannel.c_str(), strOther.c_str());
				}) == m_vHomeChannels.end()) {
			m_vHomeChannels.push_back(strChannel);
		}
	}
}

void BnxBot::DeleteHomeChannels(const std::string &strChannels) {
	std::stringstream channelStream(strChannels);

	std::string strChannel;
	while (std::getline(channelStream,strChannel,',')) {
		auto itr = std::find_if(m_vHomeChannels.begin(), m_vHomeChannels.end(), 
					[&strChannel](const std::string &strOther) {
						return !IrcStrCaseCmp(strChannel.c_str(), strOther.c_str());
					});

		if (itr != m_vHomeChannels.end())
			m_vHomeChannels.erase(itr);
	}
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

bool BnxBot::LoadSeenList(const std::string &strSeenList) {
	m_clSeenList.SetSeenListFile(strSeenList);

	return m_clSeenList.Load();
}

void BnxBot::SetLogFile(const std::string &strLogFile) {
	m_strLogFile = strLogFile;
}

void BnxBot::StartUp() {
	if (m_clConnectTimer)
		return;

	Log("Starting up ...");
	Log("Version: %s", GetVersionString().c_str());

	m_clConnectTimer.NewTimer(GetEventBase(), 0);
	m_clFloodTimer.NewTimer(GetEventBase(), EV_PERSIST);
	m_clVoteBanTimer.NewTimer(GetEventBase(), EV_PERSIST);
	m_clChannelsTimer.NewTimer(GetEventBase(), EV_PERSIST);
	m_clAntiIdleTimer.NewTimer(GetEventBase(), EV_PERSIST);
	m_clSeenListTimer.NewTimer(GetEventBase(), EV_PERSIST);

	struct timeval tv;
	tv.tv_sec = tv.tv_usec = 0;

	m_clConnectTimer.Add(&tv);
}

void BnxBot::Shutdown() {
	Send(NOW, "QUIT :Shutting down ...\r\n");
	Log("Shutting down ...");

	Disconnect();

	m_clConnectTimer.Free();
	m_clFloodTimer.Free();
	m_clVoteBanTimer.Free();
	m_clChannelsTimer.Free();
	m_clAntiIdleTimer.Free();
	m_clSeenListTimer.Free();
}

void BnxBot::Disconnect() {
	IrcClient::Disconnect();

	m_clFloodTimer.Delete();
	m_clVoteBanTimer.Delete();
	m_clChannelsTimer.Delete();
	m_clAntiIdleTimer.Delete();
	m_clSeenListTimer.Delete();

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

void BnxBot::Log(const char *pFormat, ...) {
	FILE *pFile = fopen(m_strLogFile.c_str(), "a");

	if (pFile == nullptr)
		return;

	time_t rawTime = 0;
	time(&rawTime);
	
	// XXX: Not thread-safe
	struct tm *pLocalTime = localtime(&rawTime);

	char aBuff[128] = "";
	strftime(aBuff, sizeof(aBuff), "%c ", pLocalTime);

	fputs(aBuff, pFile);
	fputs(GetProfileName().c_str(), pFile);

	fputc(' ', pFile);

	va_list ap;

	va_start(ap, pFormat);
	vfprintf(pFile, pFormat, ap);
	va_end(ap);

	fputc('\n', pFile);

	fclose(pFile);
}

void BnxBot::ProcessFlood(const char *pSource, const char *pTarget, const char *pMessage) {
	IrcUser clUser(pSource);

	if (!IsSquelched(clUser))
		m_clFloodDetector.Hit(clUser);

	if (!IsMe(pTarget)) {
		ChannelIterator channelItr = GetChannel(pTarget);

		if (channelItr != ChannelEnd() && channelItr->IsOperator())
			channelItr->GetFloodDetector().Hit(clUser);
	}

}

bool BnxBot::ProcessCommand(const char *pSource, const char *pTarget, const char *pMessage) {
	if (!IsMe(pTarget))
		return false;

	IrcUser clUser(pSource);

	std::stringstream messageStream(pMessage);

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

	if (strCommand == "ban") {
		std::string strChannel, strHostmask, strReason;

		if (!(messageStream >> strChannel >> strHostmask))
			return false;

		messageStream.get();

		std::getline(messageStream, strReason);

		return OnCommandBan(*sessionItr, strChannel, strHostmask, strReason);
	}

	if (strCommand == "unban") {
		std::string strChannel, strHostmask;

		if (!(messageStream >> strChannel >> strHostmask))
			return false;

		return OnCommandUnban(*sessionItr, strChannel, strHostmask);
	}

	if (strCommand == "splatterkick") {
		std::string strChannel, strNickname;

		if (!(messageStream >> strChannel >> strNickname))
			return false;

		return OnCommandSplatterKick(*sessionItr, strChannel, strNickname);
	}

	if (strCommand == "voteban") {
		std::string strChannel, strNickname;

		if (!(messageStream >> strChannel >> strNickname))
			return false;

		return OnCommandVoteBan(*sessionItr, strChannel, strNickname);
	}

	if (strCommand == "op") {
		std::string strChannel, strNickname;

		if (!(messageStream >> strChannel >> strNickname))
			return false;

		return OnCommandOp(*sessionItr, strChannel, strNickname);
	}

	if (strCommand == "deop") {
		std::string strChannel, strNickname;

		if (!(messageStream >> strChannel >> strNickname))
			return false;

		return OnCommandDeOp(*sessionItr, strChannel, strNickname);
	}

	if (strCommand == "rejoin") {
		std::string strChannel;

		if (!(messageStream >> strChannel))
			return false;

		return OnCommandRejoin(*sessionItr, strChannel);
	}

	if (strCommand == "who") {
		std::string strChannel;

		if (!(messageStream >> strChannel))
			return false;

		return OnCommandWho(*sessionItr, strChannel);
	}

	if (strCommand == "seen") {
		std::string strNickname;

		if (!(messageStream >> strNickname))
			return false;

		return OnCommandSeen(*sessionItr, strNickname);
	}

	if (strCommand == "lastseen") {
		std::string strChannel;
		int iDays = 1;

		if (!(messageStream >> strChannel))
			return false;

		if (!(messageStream >> iDays))
			iDays = 1;

		return OnCommandLastSeen(*sessionItr, strChannel, iDays);
	}

	if (strCommand == "reconnect") {
		return OnCommandReconnect(*sessionItr);
	}

	return false;
}

bool BnxBot::ProcessVoteBan(const char *pSource, const char *pTarget, const char *pMessage) {
	if (IsMe(pTarget))
		return false;

	ChannelIterator channelItr = GetChannel(pTarget);

	if (channelItr == ChannelEnd() || !channelItr->IsVoteBanInProgress())
		return false;

	IrcUser clUser(pSource);

	if (!IrcStrCaseCmp(pMessage,"yea")) {
		channelItr->VoteYay(clUser.GetNickname());
		return true;
	}
	else if (!IrcStrCaseCmp(pMessage,"nay")) {
		channelItr->VoteNay(clUser.GetNickname());
		return true;
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
	if (IsMe(strSourceNick))
		return;

	const char *pReplyTo = strSourceNick.c_str();
	std::string strPrefix;

	if (!IsMe(pTarget)) {
		ChannelIterator channelItr = GetChannel(pTarget);

		// Uhh?
		if (channelItr == ChannelEnd())
			return;

		if (channelItr->GetSize() != 2 && 
			IrcStrCaseNick(pMessage,GetCurrentNickname().c_str()) == nullptr) {
			return;
		}

		// BNX seems to only consider "fuck" inappropriate
		// Due to the multi-channel nature of IRC, we can only do this in channel
		// The original would also ban for whispered profanity
		if (channelItr->IsOperator() && IrcStrCaseStr(pMessage,"fuck") != nullptr) {
			Send(LATER, "PRIVMSG %s :I don't appreciate being spoken to in that manner, %s.\r\n", 
					pTarget, clUser.GetNickname().c_str());
			Send(LATER, "MODE %s +b %s\r\n", pTarget, clUser.GetBanMask().c_str());
			Send(LATER, "KICK %s %s :for inappropriate language\r\n", pTarget, 
					clUser.GetNickname().c_str());

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

		Send(AUTO, "PRIVMSG %s :%sOK, I won't talk to you anymore.\r\n", pReplyTo, strPrefix.c_str());
		return;
	}

	std::string strResponse = m_clResponseEngine.ComputeResponse(pMessage);

	if (strResponse[0] == '/')
		strPrefix.clear();

	size_t findPos = 0;
	while ((findPos = strResponse.find("%s", findPos)) != std::string::npos) {
		strResponse.replace(findPos, 2, strSourceNick);
		findPos += strSourceNick.size();
	}
	
	Say(AUTO, pReplyTo, "%s%s", strPrefix.c_str(), strResponse.c_str());
}

void BnxBot::Say(WhenType eWhen, const char *pTarget, const char *pFormat, ...) {
	CtcpEncoder clEncoder;
	char aBuff[513];
	va_list ap;

	va_start(ap, pFormat);

	vsnprintf(aBuff, sizeof(aBuff), pFormat, ap);

	va_end(ap);

	const char *pMessage = aBuff;

	if (pMessage[0] == '/') {
		std::stringstream commandStream(pMessage);

		std::string strCommand;

		commandStream >> strCommand;

		if (strCommand == "/me" || strCommand == "/action") {
			std::string strLine;

			commandStream.get();

			if (!std::getline(commandStream,strLine))
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

	Send(eWhen, "PRIVMSG %s :%s\r\n", pTarget, pMessage);
}

BnxBot::ChannelIterator BnxBot::GetChannel(const char *pChannel) {
	return std::find(ChannelBegin(), ChannelEnd(), pChannel);
}

bool BnxBot::IsSquelched(const IrcUser &clUser) {
	return std::find_if(m_vSquelchedUsers.cbegin(), 
				m_vSquelchedUsers.cend(), 
				[&clUser](const IrcUser &clMask) {
					return clMask.Matches(clUser);
				}) != m_vSquelchedUsers.end();
}

void BnxBot::SplatterKick(const char *pChannel, const IrcUser &clUser) {
	const std::string &strNickname = clUser.GetNickname();

	switch(rand() % 12) {
	case 0:
		Say(LATER, pChannel, "Congratulations, %s - you're the lucky winner of a one-way trip to The Void!", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :don't forget to write!\r\n", pChannel, strNickname.c_str());
		break;
	case 1:
		Say(LATER, pChannel, "%s: What is your real name?", strNickname.c_str());
		Say(LATER, pChannel, "%s: What is your quest?", strNickname.c_str());
		Say(LATER, pChannel, "%s: What is the average velocity of a coconut-laden swallow?", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s\r\n", pChannel, strNickname.c_str());
		Say(LATER, pChannel, "I guess he didn't know!");
		break;
	case 2:
		Say(LATER, pChannel, "/me smells something bad...");
		Say(LATER, pChannel, "/me looks at %s...", strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :Ah! Smell's gone!!\r\n", pChannel, strNickname.c_str());
		break;
	case 3:
		Say(LATER, pChannel, "/me says \"YER OUTTA HERE, PAL!\"");
		Say(LATER, pChannel, "/me takes %s by the balls and throws him into The Void.", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :AND STAY OUT!!\r\n", pChannel, strNickname.c_str());
		break;
	case 4:
		Say(LATER, pChannel, "It's April, the season of growing, and the F-ing weeds are popping up everywhere.");
		Say(LATER, pChannel, "/me spots a weed in %s", pChannel);
		Say(LATER, pChannel, "/me grabs a bottle of Round-Up and spritzes %s liberally.", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :FSSST! Weed's gone!\r\n", pChannel, strNickname.c_str());
		break;
	case 5:
		Say(LATER, pChannel, "/me pulls out his portable chalkboard.");
		Say(LATER, pChannel, "/me shows %s the function of relativity for chaos mathematics.", 
				strNickname.c_str());
		Say(LATER, pChannel, "/me watches as %s's brain shorts out with a puff of putrid smoke!", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :zzzzzttttt!!!!\r\n", pChannel, strNickname.c_str());
		break;
	case 6:
		Say(LATER, pChannel, "/me bashes %s's head in with a baseball bat *BOK*!!", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s\r\n", pChannel, strNickname.c_str());
		Say(LATER, pChannel, "/me wipes the blood off on %s's hair.", strNickname.c_str());
		break;
	case 7:
		Say(LATER, pChannel, "/me gags %s, stuffs him into a cow suit, then tosses him into a corral with a horny bull.", 
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :Moooo!!!!!!!\r\n", pChannel, strNickname.c_str());
		break;
	case 8:
		Say(LATER, pChannel, "/me grabs %s by the hair and jams his face into the toilet.", 
				strNickname.c_str());
		Say(LATER, pChannel, "/me does the royal flush.");
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :KA-WIIIISSSHHHHHHHHH!!!\r\n", pChannel, strNickname.c_str());
		break;
	case 9:
		Say(LATER, pChannel, "/me casts a Fireball that goes streaking across the channel at %s", 
				strNickname.c_str());
		Say(LATER, pChannel, "/me watches as %s's corporeal form is enveloped in flame!",
				strNickname.c_str());
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :poof!!\r\n", pChannel, strNickname.c_str());
		break;
	case 10:
		Say(LATER, pChannel, "/me grabs %s's tongue and pulls it waaaaay out.", 
			strNickname.c_str());
		Say(LATER, pChannel, "/me takes out the locking ring and loops it through %s's tongue.",
			strNickname.c_str());
		Say(LATER, pChannel, "/me then fastens the ring to the bumper of his Porsche and drives off.");
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s :what a drag!\r\n", pChannel, strNickname.c_str());
		break;
	case 11:
		Say(LATER, pChannel, "/me pulls down the switch on the electric chair.");
		Send(LATER, "MODE %s +b %s\r\n", pChannel, clUser.GetBanMask().c_str());
		Send(LATER, "KICK %s %s\r\n", pChannel, strNickname.c_str());
		Say(LATER, pChannel, "/me makes an omelette with %s's brains.", strNickname.c_str());
		break;
	}
}

void BnxBot::OnConnect() {
	IrcClient::OnConnect();
}

void BnxBot::OnDisconnect() {
	IrcClient::OnDisconnect();

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	m_clConnectTimer.Add(&tv);
}

void BnxBot::OnRegistered() {
	IrcClient::OnRegistered();

	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// TODO: Make this configurable
	m_clFloodDetector.SetThreshold(2.0f);
	m_clFloodDetector.SetTimeStep(1.0f);

	m_clFloodTimer.Add(&tv);
	m_clVoteBanTimer.Add(&tv);

	tv.tv_sec = 10;
	m_clChannelsTimer.Add(&tv);

	tv.tv_sec = 60;
	m_clAntiIdleTimer.Add(&tv);

	tv.tv_sec = 300;
	m_clSeenListTimer.Add(&tv);

	if (IsMe(GetNickname()) &&
		!m_strNickServ.empty() && !m_strNickServPassword.empty()) {
		Send(AUTO, "PRIVMSG %s :identify %s\r\n", m_strNickServ.c_str(), 
			m_strNickServPassword.c_str());
	}

	for (size_t i = 0; i < m_vHomeChannels.size(); ++i) {
		Send(AUTO, "JOIN %s\r\n", m_vHomeChannels[i].c_str());
	}
}

void BnxBot::OnNumeric(const char *pSource, int numeric, const char *pParams[], unsigned int numParams) {
	IrcClient::OnNumeric(pSource, numeric, pParams, numParams);

	const char *pChannel = nullptr, *pTrailing = nullptr, *pUsername = nullptr, 
		*pHostname = nullptr, *pNickname = nullptr, *pMode = nullptr;
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
			std::stringstream nameStream(pTrailing);

			std::string strNickname;

			while (nameStream >> strNickname) {
				size_t p;
				for (p = 0; p < strNickname.size() &&
						!isalpha(strNickname[p]) &&
						!IrcIsSpecial(strNickname[p]); ++p);

				if (p < strNickname.size() && IsMe(strNickname.substr(p))) {
					AddChannel(pChannel);

					// Now really collect useful information
					Send(AUTO, "WHO %s\r\n", pChannel);
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

		{
			IrcUser clUser(pNickname,pUsername,pHostname);
			channelItr->AddMember(clUser);
			m_clSeenList.Saw(clUser, pChannel);
		}

		if (IsMe(pNickname) && strchr(pMode,'@') != nullptr)
			channelItr->SetOperator(true);

		break;
	case ERR_NOSUCHCHANNEL:
		pChannel = pParams[1];
		DeleteHomeChannels(pChannel);
		break;
	case ERR_ERRONEUSNICKNAME:
		// If this happens, the bot never even registers ... 
		if (!IsRegistered())
			BnxDriver::GetInstance().Shutdown();
		break;
	}
}

void BnxBot::OnNick(const char *pSource, const char *pNewNick) {
	IrcClient::OnNick(pSource, pNewNick);

	IrcUser clUser(pSource);

	for (size_t i = 0; i < m_vCurrentChannels.size(); ++i) 
		m_vCurrentChannels[i].UpdateMember(clUser.GetNickname(), pNewNick);
}

void BnxBot::OnKick(const char *pSource, const char *pChannel, const char *pUser, const char *pReason) {
	IrcClient::OnKick(pSource, pChannel, pUser, pReason);

	ChannelIterator channelItr = GetChannel(pChannel);

	// What?
	if (channelItr == ChannelEnd())
		return;

	IrcUser clUser(pSource);

	if (IsMe(pUser)) {
		m_clShitList.AddMask(IrcUser("*","*",clUser.GetHostname()));
		m_clShitList.Save();

		Log("%s kicked me out of %s, so I shitlisted him.", pSource, pChannel);

		DeleteChannel(channelItr);
		return;
	}

	channelItr->DeleteMember(pUser);
}

void BnxBot::OnPrivmsg(const char *pSource, const char *pTarget, const char *pMessage) {
	IrcClient::OnPrivmsg(pSource, pTarget, pMessage);

	if (!IsMe(pTarget))
		m_clSeenList.Saw(IrcUser(pSource), pTarget);

	ProcessFlood(pSource, pTarget, pMessage);

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
		else if (!strcmp(pTag,"TIME")) {
			OnCtcpTime(pSource, pTarget);
		}
		else if (!strcmp(pTag,"ACTION")) {
			if (message.dataSize > 0)
				OnCtcpAction(pSource, pTarget, pMessage);
		}
		else if (!strcmp(pTag,"PING")) {
			if (message.dataSize > 0)
				OnCtcpPing(pSource, pTarget, pMessage);
		}
	}
	else {
		// Don't use the decoded non-tagged data since it strips lone backslash

		if (ProcessCommand(pSource, pTarget, pMessage))
			return;

		if (ProcessVoteBan(pSource, pTarget, pMessage))
			return;

		// Finally, process the message text
		ProcessMessage(pSource, pTarget, pMessage);
	}
}

void BnxBot::OnNotice(const char *pSource, const char *pTarget, const char *pMessage) {
	IrcClient::OnNotice(pSource, pTarget, pMessage);

	// Sometimes servers send notices
	if (IrcIsHostmask(pSource))
		ProcessFlood(pSource, pTarget, pMessage);
}

void BnxBot::OnJoin(const char *pSource, const char *pChannel) {
	IrcClient::OnJoin(pSource, pChannel);

	ChannelIterator channelItr = GetChannel(pChannel);

	// What? Must be me on RFC2812
	if (channelItr == ChannelEnd())
		return;

	IrcUser clUser(pSource);

	channelItr->AddMember(clUser);
	m_clSeenList.Saw(clUser, pChannel);

	if (channelItr->IsOperator()) {
		BnxShitList::ConstIterator shitItr = m_clShitList.FindMatch(clUser);

		if (shitItr != m_clShitList.End()) {
			Send(AUTO, "MODE %s +b %s\r\n", pChannel, shitItr->GetHostmask().c_str());
			Send(AUTO, "KICK %s %s :because I don't like you\r\n", pChannel, 
				clUser.GetNickname().c_str());
			return;
		}
	}

	if (channelItr->GetSize() == 2)
		Send(AUTO, "PRIVMSG %s :Hi!\r\n", pChannel);
}

void BnxBot::OnPart(const char *pSource, const char *pChannel, const char *pReason) {
	IrcClient::OnPart(pSource, pChannel, pReason);

	IrcUser clUser(pSource);

	if (IsMe(clUser.GetNickname())) {
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

	if (!IsMe(pTarget)) {
		// Targetting channel

		ChannelIterator channelItr = GetChannel(pTarget);

		// What?
		if (channelItr == ChannelEnd())
			return;

		// Get traits from IRC (needed to process modes)
		const IrcTraits &clTraits = GetIrcTraits();

		const char *pModeString = pMode;

		bool bSetMode = true;

		for ( ;*pMode != '\0'; ++pMode) {
			switch (*pMode) {
			case 'o':
				if (IsMe(*pParams))
					channelItr->SetOperator(bSetMode);

				++pParams;
				--numParams;
				break;
			case '-':
				bSetMode = false;
				break;
			case '+':
				bSetMode = true;
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
			Log("Didn't process modes correctly: %s", pModeString);
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

void BnxBot::OnCtcpPing(const char *pSource, const char *pTarget, const char *pMessage) {
	IrcUser clUser(pSource);

	if (IsSquelched(clUser))
		return;

	CtcpEncoder clEncoder;
	clEncoder.Encode(MakeCtcpMessage("PING", pMessage));

	Send(AUTO, "NOTICE %s :%s\r\n", clUser.GetNickname().c_str(), clEncoder.GetRaw());
}

void BnxBot::OnCtcpVersion(const char *pSource, const char *pTarget) {
	IrcUser clUser(pSource);

	if (IsSquelched(clUser))
		return;

	// Actually, this can be used inline below (as before) even though it seems like it might be destructed.
	// One might be tempted to think that it is destructs when MakeCtcpMessage() has finished,
	//  but apparently this is not the case and it destructs when the entire expression has
	//  finished executing.
	std::string strVersion = GetVersionString();

	CtcpEncoder clEncoder;
	clEncoder.Encode(MakeCtcpMessage("VERSION", strVersion.c_str()));

	Send(AUTO, "NOTICE %s :%s\r\n", clUser.GetNickname().c_str(), clEncoder.GetRaw());
}

void BnxBot::OnCtcpTime(const char *pSource, const char *pTarget) {
	IrcUser clUser(pSource);

	if (IsSquelched(clUser))
		return;

	time_t rawTime = 0;
	time(&rawTime);

	// XXX: Not thread-safe
	struct tm *pLocalTime = localtime(&rawTime);

	char aBuff[128] = "";
	strftime(aBuff, sizeof(aBuff), "%c", pLocalTime);

	CtcpEncoder clEncoder;
	clEncoder.Encode(MakeCtcpMessage("TIME", aBuff));

	Send(AUTO, "NOTICE %s :%s\r\n", clUser.GetNickname().c_str(), clEncoder.GetRaw());
}

bool BnxBot::OnCommandLogin(const IrcUser &clUser, const std::string &strPassword) {

	if (m_clAccessSystem.Login(clUser, strPassword)) {
		Log("%s validated.", clUser.GetHostmask().c_str());
		Send(AUTO, "PRIVMSG %s :Your wish is my command, master.\r\n", clUser.GetNickname().c_str());

		return true;
	}

	Log("Login attempt by %s, pass %s failed.", clUser.GetHostmask().c_str(), strPassword.c_str());

	return false;
}

bool BnxBot::OnCommandLogout(UserSession &clSession) {
	const IrcUser &clUser = clSession.GetUser();

	Send(AUTO, "PRIVMSG %s :Fare the well...\r\n", clUser.GetNickname().c_str());

	m_clAccessSystem.Logout(clUser);

	// XXX: clUser and clSession are dead references now!

	return true;
}

bool BnxBot::OnCommandSay(UserSession &clSession, const std::string &strTarget, const std::string &strMessage) {

	Say(AUTO, strTarget.c_str(), "%s", strMessage.c_str());

	return true;
}

bool BnxBot::OnCommandChatter(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	m_bChatter = true;

	// The original BNX would ignore users indefinitely, here we'll clear the list on "chatter"
	m_vSquelchedUsers.clear();

	Send(AUTO, "PRIVMSG %s :Permission to speak freely, sir?\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandShutUp(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	m_bChatter = false;

	Send(AUTO, "PRIVMSG %s :Aww... Why can't I talk anymore?\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandJoin(UserSession &clSession, const std::string &strChannels) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	AddHomeChannels(strChannels);

	Send(AUTO, "JOIN %s\r\n", strChannels.c_str());

	return true;
}

bool BnxBot::OnCommandPart(UserSession &clSession, const std::string &strChannels) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	DeleteHomeChannels(strChannels);

	Send(AUTO, "PART %s\r\n", strChannels.c_str());

	return true;
}

bool BnxBot::OnCommandShutdown(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Send(NOW, "PRIVMSG %s :Sir, if you don't mind, I'll close down for a while...\r\n", clUser.GetNickname().c_str());

	//Shutdown();

	// Shut all bots down I guess ...
	BnxDriver &clDriver = BnxDriver::GetInstance();

	clDriver.Shutdown();

	return true;
}

bool BnxBot::OnCommandUserList(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	BnxAccessSystem::ConstEntryIterator entryItr;

	Send(AUTO, "PRIVMSG %s :Access List:\r\n", clUser.GetNickname().c_str());

	for (entryItr = m_clAccessSystem.EntryBegin(); entryItr != m_clAccessSystem.EntryEnd(); ++entryItr) {
		const IrcUser &clMask = entryItr->GetHostmask();
		std::string strMask = clMask.GetHostmask();
		int iAccessLevel = entryItr->GetAccessLevel();

		Send(AUTO, "PRIVMSG %s :%s %d\r\n", clUser.GetNickname().c_str(), strMask.c_str(), iAccessLevel);
	}

	Send(AUTO, "PRIVMSG %s :End of Access List.\r\n", clUser.GetNickname().c_str());

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

	Send(AUTO, "PRIVMSG %s :I am in channels: %s\r\n", clUser.GetNickname().c_str(), strChannels.c_str());

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
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	Send(AUTO, "KICK %s %s :%s\r\n", strChannel.c_str(), strHostmask.c_str(), strReason.c_str());

	Send(AUTO, "PRIVMSG %s :OK, kicked his ass.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandSquelch(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Squelch(IrcUser(strHostmask));

	Send(AUTO, "PRIVMSG %s :OK, ignoring...\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandUnsquelch(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Unsquelch(IrcUser(strHostmask));

	Send(AUTO, "PRIVMSG %s :OK, unignoring...\r\n", clUser.GetNickname().c_str());

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

	Send(AUTO, "PRIVMSG %s :Added %s, level %d, pass %s\r\n", clUser.GetNickname().c_str(), 
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
		Send(AUTO, "PRIVMSG %s :Cannot delete %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());
		return true;
	}

	if (clSession.GetAccessLevel() <= entryItr->GetAccessLevel()) {
		Send(AUTO, "PRIVMSG %s :Insufficient privilege.\r\n", clUser.GetNickname().c_str());
		return true;
	}

	m_clAccessSystem.DeleteUser(entryItr);
	m_clAccessSystem.Save();

	Send(AUTO, "PRIVMSG %s :Deleted %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());

	return true;
}

bool BnxBot::OnCommandShitAdd(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	IrcUser clMask(strHostmask);

	m_clShitList.AddMask(clMask);
	m_clShitList.Save();

	Send(AUTO, "PRIVMSG %s :%s shitlisted.\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());

	return true;
}

bool BnxBot::OnCommandShitDel(UserSession &clSession, const std::string &strHostmask) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	IrcUser clMask(strHostmask);

	if (m_clShitList.DeleteMask(clMask)) {
		Send(AUTO, "PRIVMSG %s :Deleted %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());
		m_clShitList.Save();
	}
	else {
		Send(AUTO, "PRIVMSG %s :Cannot delete %s\r\n", clUser.GetNickname().c_str(), clMask.GetHostmask().c_str());
	}

	return true;
}

bool BnxBot::OnCommandShitList(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Send(AUTO, "PRIVMSG %s :Shit List:\r\n", clUser.GetNickname().c_str());

	BnxShitList::ConstIterator itr;

	for (itr = m_clShitList.Begin(); itr != m_clShitList.End(); ++itr)
		Send(AUTO, "PRIVMSG %s :%s\r\n", clUser.GetNickname().c_str(), itr->GetHostmask().c_str());

	Send(AUTO, "PRIVMSG %s :End of Shit List.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandNick(UserSession &clSession, const std::string &strNickname) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	Send(AUTO, "NICK %s\r\n", strNickname.c_str());

	return true;
}

bool BnxBot::OnCommandBan(UserSession &clSession, const std::string &strChannel, 
				const std::string &strHostmask, const std::string &strReason) {
	if (clSession.GetAccessLevel() < 60)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;
	
	if (!channelItr->IsOperator()) {
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	std::string strKickNick;
	IrcUser clBanMask(strHostmask);

	if (!IrcIsHostmask(strHostmask.c_str())) {
		// Nickname
		BnxChannel::ConstMemberIterator memberItr = channelItr->GetMember(strHostmask);

		if (memberItr != channelItr->MemberEnd()) {
			const IrcUser &clMember = memberItr->GetUser();
			strKickNick = clMember.GetNickname();
			clBanMask.Set("*","*",clMember.GetHostname());
		}
	}

	Send(AUTO, "MODE %s +b %s\r\n", strChannel.c_str(), clBanMask.GetHostmask().c_str());

	if (!strKickNick.empty()) {
		Send(AUTO, "KICK %s %s :%s\r\n", strChannel.c_str(), strKickNick.c_str(), strReason.c_str());
	}

	Send(AUTO, "PRIVMSG %s :He is forever banned.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandUnban(UserSession &clSession, const std::string &strChannel, 
				const std::string &strHostmask) {

	if (clSession.GetAccessLevel() < 60)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	if (!channelItr->IsOperator()) {
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	IrcUser clMask(strHostmask);

	Send(AUTO, "MODE %s -b %s\r\n", strChannel.c_str(), clMask.GetHostmask().c_str());
	Send(AUTO, "PRIVMSG %s :Aw, do I have to let him back in?\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandSplatterKick(UserSession &clSession, const std::string &strChannel,
					const std::string &strNickname) {
	if (clSession.GetAccessLevel() < 60)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	if (!channelItr->IsOperator()) {
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	BnxChannel::ConstMemberIterator memberItr = channelItr->GetMember(strNickname);

	if (memberItr == channelItr->MemberEnd()) {
		// Original BNX doesn't check this
		Send(AUTO, "PRIVMSG %s :%s is not here for me to ban!\r\n", clUser.GetNickname().c_str(), 
			strNickname.c_str());
		return true;
	}

	SplatterKick(strChannel.c_str(), memberItr->GetUser());

	Send(AUTO, "PRIVMSG %s :Consider him splattered.\r\n", clUser.GetNickname().c_str());

	return true;
}

bool BnxBot::OnCommandVoteBan(UserSession &clSession, const std::string &strChannel,
				const std::string &strNickname) {
	if (clSession.GetAccessLevel() < 60)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	if (!channelItr->IsOperator()) {
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	if (channelItr->IsVoteBanInProgress())
		return false;

	// Apparently the original checks this here
	if (IsMe(strNickname)) {
		Send(AUTO, "PRIVMSG %s :Only a MORON thinks I would try to ban MYSELF!\r\n", 
			clUser.GetNickname().c_str());
		return true;
	}

	BnxChannel::ConstMemberIterator memberItr = channelItr->GetMember(strNickname);

	if (memberItr == channelItr->MemberEnd()) {
		Send(AUTO, "PRIVMSG %s :%s is not here for me to ban!\r\n", clUser.GetNickname().c_str(), 
			strNickname.c_str());
		return true;
	}

	Send(AUTO, "PRIVMSG %s :If you want me to ban %s, say \"Yea\" - if not, say \"Nay\"\r\n", 
		strChannel.c_str(), strNickname.c_str());

	Send(AUTO, "PRIVMSG %s :I will tally the votes in %d seconds.\r\n", strChannel.c_str(), 
		BnxChannel::VOTEBAN_TIMEOUT);

	channelItr->VoteBan(memberItr->GetUser());

	return true;
}

bool BnxBot::OnCommandOp(UserSession &clSession, const std::string &strChannel,
			const std::string &strNickname) {
	if (clSession.GetAccessLevel() < 90)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	if (!channelItr->IsOperator()) {
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	Send(AUTO, "MODE %s +o %s\r\n", strChannel.c_str(), strNickname.c_str());

	return true;
}

bool BnxBot::OnCommandDeOp(UserSession &clSession, const std::string &strChannel,
			const std::string &strNickname) {
	if (clSession.GetAccessLevel() < 90)
		return false;

	const IrcUser &clUser = clSession.GetUser();

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	if (!channelItr->IsOperator()) {
		Send(AUTO, "PRIVMSG %s :I don't have OP!\r\n", clUser.GetNickname().c_str());
		return true;
	}

	Send(AUTO, "MODE %s -o %s\r\n", strChannel.c_str(), strNickname.c_str());

	return true;
}

bool BnxBot::OnCommandRejoin(UserSession &clSession, const std::string &strChannel) {
	if (clSession.GetAccessLevel() < 75)
		return false;

	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	if (channelItr == ChannelEnd())
		return false;

	Send(AUTO, "PART %s\r\n", strChannel.c_str());
	Send(AUTO, "JOIN %s\r\n", strChannel.c_str());

	return true;
}

bool BnxBot::OnCommandWho(UserSession &clSession, const std::string &strChannel) {
	ChannelIterator channelItr = GetChannel(strChannel.c_str());

	const IrcUser &clUser = clSession.GetUser();

	if (channelItr == ChannelEnd())
		return false;

	BnxChannel::ConstMemberIterator memberItr;

	Send(AUTO, "PRIVMSG %s :Users in channel: %s (%lu)\r\n", 
		clUser.GetNickname().c_str(), strChannel.c_str(), channelItr->GetSize());

	unsigned int uiCount = 0;
	std::string strOutput;

	for (memberItr = channelItr->MemberBegin(); memberItr != channelItr->MemberEnd(); ++memberItr) {
		if (!strOutput.empty())
			strOutput += ", ";

		strOutput += memberItr->GetUser().GetNickname();

		if (++uiCount == 5) {
			Send(AUTO, "PRIVMSG %s :%s\r\n", 
				clUser.GetNickname().c_str(), strOutput.c_str());
			strOutput.clear();
			uiCount = 0;
		}
		
	}

	if (!strOutput.empty()) {
		Send(AUTO, "PRIVMSG %s :%s\r\n", 
			clUser.GetNickname().c_str(), strOutput.c_str());
	}

	return true;
}

bool BnxBot::OnCommandSeen(UserSession &clSession, const std::string &strNickname) {
	const IrcUser &clUser = clSession.GetUser();

	BnxSeenList::Iterator itr = m_clSeenList.Find(strNickname);

	if (itr == m_clSeenList.End()) {
		Send(AUTO, "PRIVMSG %s :Haven't seen %s in any channel.\r\n",
			clUser.GetNickname().c_str(), strNickname.c_str());
		return true;
	}

	const BnxSeenList::SeenInfo &clSeenInfo = itr->second;

	time_t rawTime = clSeenInfo.GetTimestamp();

	// XXX: Not thread-safe
	struct tm *pLocalTime = localtime(&rawTime);

	char aFormattedTime[128] = "";
	strftime(aFormattedTime, sizeof(aFormattedTime), "%c", pLocalTime);

	Send(AUTO, "PRIVMSG %s :Last saw %s in channel %s on %s.\r\n",
		clUser.GetNickname().c_str(), clSeenInfo.GetUser().GetHostmask().c_str(),
		clSeenInfo.GetChannel().c_str(), aFormattedTime);

	return true;
}

bool BnxBot::OnCommandLastSeen(UserSession &clSession, const std::string &strChannel, int iDays) {
	if (iDays < 1)
		return false;

	const int iMaxTime = 60*60*24*iDays;
	const IrcUser &clUser = clSession.GetUser();

	Send(AUTO, "PRIVMSG %s :Users last seen in the past %d days in channel %s:\r\n",
		clUser.GetNickname().c_str(), iDays, strChannel.c_str());

	BnxSeenList::Iterator itr;

	int iCount = 0;
	for (itr = m_clSeenList.Begin(); itr != m_clSeenList.End(); ++itr) {
		const BnxSeenList::SeenInfo &clSeenInfo = itr->second;

		if (IrcStrCaseCmp(clSeenInfo.GetChannel().c_str(), strChannel.c_str()) != 0 ||
			time(nullptr) - clSeenInfo.GetTimestamp() >= iMaxTime) {
			continue;
		}

		++iCount;

		time_t rawTime = clSeenInfo.GetTimestamp();
	
		// XXX: Not thread-safe
		struct tm *pLocalTime = localtime(&rawTime);
	
		char aFormattedTime[128] = "";
		strftime(aFormattedTime, sizeof(aFormattedTime), "%a %b %d %H:%M", pLocalTime);

		Send(AUTO, "PRIVMSG %s :%s on %s.\r\n", clUser.GetNickname().c_str(),
			clSeenInfo.GetUser().GetHostmask().c_str(), aFormattedTime);
	}

	Send(AUTO, "PRIVMSG %s :Saw a total of %d users.\r\n", clUser.GetNickname().c_str(), iCount);

	return true;
}

bool BnxBot::OnCommandReconnect(UserSession &clSession) {
	if (clSession.GetAccessLevel() < 100)
		return false;

	Send(NOW, "QUIT :Reconnecting ...\r\n");

	struct timeval tv;
	tv.tv_sec = tv.tv_usec = 0;

	m_clConnectTimer.Add(&tv);

	return true;
}

void BnxBot::AddChannel(const char *pChannel) {
	if (GetChannel(pChannel) != ChannelEnd())
		return;

	IrcCaseMapping eCaseMapping = GetIrcTraits().GetCaseMapping();

	m_vCurrentChannels.push_back(BnxChannel(pChannel, eCaseMapping));
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

void BnxBot::OnConnectTimer(evutil_socket_t fd, short what) {
	if (!Connect(m_strServer, m_strPort)) {
		// Connect failed outright so reschedule the timer
		struct timeval tv;

		tv.tv_sec = 5;
		tv.tv_usec = 0;

		m_clConnectTimer.Add(&tv);
	}
}

void BnxBot::OnFloodTimer(evutil_socket_t fd, short what) {
	std::vector<IrcUser> vFlooders;

	m_clFloodDetector.Detect(vFlooders);

	for (size_t i = 0; i < vFlooders.size(); ++i) {
		Log("Ignoring %s for flooding", vFlooders[i].GetHostmask().c_str());
		Squelch(IrcUser("*","*",vFlooders[i].GetHostname()));
	}

	// Detect floods in channels
	for (size_t i = 0; i < m_vCurrentChannels.size(); ++i) {
		BnxChannel &clChannel = m_vCurrentChannels[i];

		clChannel.ExpireWarningEntries();

		BnxFloodDetector &clDetector = clChannel.GetFloodDetector();

		vFlooders.clear();

		// This should be called regardless of channel operator status
		clDetector.Detect(vFlooders);

		if (!clChannel.IsOperator())
			continue;

		for (size_t j = 0; j < vFlooders.size(); ++j) {
			const IrcUser &clUser = vFlooders[j];

			BnxChannel::WarningIterator warningItr = clChannel.Warn(clUser.GetHostname());

			switch (warningItr->GetCount()) {
			case 0:
				break;
			case 1:
				Send(AUTO, "PRIVMSG %s :%s: Stop flooding! I'm warning you!\r\n", 
					clChannel.GetName().c_str(), clUser.GetNickname().c_str());
				break;
			case 2:
				Log("Kicking %s from %s for flooding", 
					clUser.GetHostmask().c_str(), clChannel.GetName().c_str());

				Send(AUTO, "KICK %s %s :stop flooding!\r\n", 
					clChannel.GetName().c_str(), clUser.GetNickname().c_str());
				break;
			case 3:
			default:
				Log("Banning %s from %s for flooding", 
					clUser.GetHostmask().c_str(), clChannel.GetName().c_str());

				Send(AUTO, "MODE %s +b %s\r\n", 
					clChannel.GetName().c_str(), clUser.GetBanMask().c_str());
				Send(AUTO, "KICK %s %s :for flooding\r\n", 
					clChannel.GetName().c_str(), clUser.GetNickname().c_str());

				clChannel.DeleteWarningEntry(warningItr);
			}
		}

	}

}

void BnxBot::OnVoteBanTimer(evutil_socket_t fd, short what) {
	for (size_t i = 0; i < m_vCurrentChannels.size(); ++i) {
		BnxChannel &clChannel = m_vCurrentChannels[i];

		if (clChannel.IsVoteBanInProgress() && clChannel.VoteBanExpired()) {
			const IrcUser &clUser = clChannel.GetVoteBanMask();

			if (clChannel.TallyVote() >= 0) {
				Send(AUTO, "PRIVMSG %s :Banning %s by popular request...\r\n", 
					clChannel.GetName().c_str(), clUser.GetNickname().c_str());
				SplatterKick(clChannel.GetName().c_str(), clUser);
			}
			else {
				Send(AUTO, "PRIVMSG %s :OK, %s can stay.\r\n",
					clChannel.GetName().c_str(), clUser.GetNickname().c_str());
			}

			clChannel.ResetVoteBan();
		}
	}
}

void BnxBot::OnChannelsTimer(evutil_socket_t fd, short what) {
	for (size_t i = 0; i < m_vHomeChannels.size(); ++i) {
		ChannelIterator channelItr = GetChannel(m_vHomeChannels[i].c_str());

		if (channelItr == ChannelEnd())
			Send(AUTO, "JOIN %s\r\n", m_vHomeChannels[i].c_str());
	}
}

void BnxBot::OnAntiIdleTimer(evutil_socket_t fd, short what) {
	if (time(nullptr)-GetLastRecvTime() > 30)
		Send(AUTO, "PING :%s\r\n", GetCurrentServer().c_str());
}

void BnxBot::OnSeenListTimer(evutil_socket_t fd, short what) {
	// Occasionally expire and save entries

	m_clSeenList.ExpireEntries();
	m_clSeenList.Save();
}

