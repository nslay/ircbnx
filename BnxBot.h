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

#ifndef BNXBOT_H
#define BNXBOT_H

#include <string>
#include <utility>
#include <vector>
#include "BnxResponseEngine.h"
#include "BnxAccessSystem.h"
#include "BnxShitList.h"
#include "BnxChannel.h"
#include "BnxFloodDetector.h"
#include "IrcClient.h"
#include "IrcUser.h"

class BnxBot : public IrcClient {
public:
	typedef BnxAccessSystem::UserSession UserSession;

	BnxBot()
	: m_pConnectTimer(NULL), m_pFloodTimer(NULL), m_bChatter(true) { }

	virtual ~BnxBot();

	void SetServerAndPort(const std::string &strServer, const std::string &strPort = "6667");
	void AddHomeChannel(const std::string &channel);
	void DeleteHomeChannel(const std::string &channel);
	bool LoadResponseRules(const std::string &strFileName);
	bool LoadAccessList(const std::string &strFilename);
	bool LoadShitList(const std::string &strFilename);

	void StartUp();
	void Shutdown();

	virtual void Disconnect();

protected:
	virtual bool ProcessCommand(const char *pSource, const char *pTarget, const char *pMessage);
	virtual void ProcessMessage(const char *pSource, const char *pTarget, const char *pMessage);
	virtual void Say(const char *pTarget, const char *pMessage);
	BnxChannel * GetChannel(const char *pChannel);
	bool IsSquelched(const IrcUser &clUser);

	// IRC events
	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnRegistered();
	virtual void OnNumeric(const char *pSource, int numeric, const char *pParams[], unsigned int numParams);
	virtual void OnNick(const char *pSource, const char *pNewNick);
	virtual void OnKick(const char *pSource, const char *pChannel, const char *pUser, const char *pReason);
	virtual void OnPrivmsg(const char *pSource, const char *pTarget, const char *pMessage);
	virtual void OnJoin(const char *pSource, const char *pChannel);
	virtual void OnPart(const char *pSource, const char *pChannel, const char *pReason);
	virtual void OnMode(const char *pSource, const char *pTarget, const char *pMode, const char *pParams[], unsigned int numParams);
	virtual void OnQuit(const char *pSource, const char *pReason);

	// CTCP events
	virtual void OnCtcpAction(const char *pSource, const char *pTarget, const char *pMessage);
	virtual void OnCtcpVersion(const char *pSource, const char *pTarget);

	// User command events
	virtual bool OnCommandLogin(const IrcUser &clUser, const std::string &strPassword);
	virtual bool OnCommandLogout(UserSession &clSession);
	virtual bool OnCommandSay(UserSession &clSession, const std::string &strTarget, const std::string &strMessage);
	virtual bool OnCommandChatter(UserSession &clSession);
	virtual bool OnCommandShutUp(UserSession &clSession);
	virtual bool OnCommandJoin(UserSession &clSession, const std::string &strChannels);
	virtual bool OnCommandPart(UserSession &clSession, const std::string &strChannels);
	virtual bool OnCommandShutdown(UserSession &clSession);
	virtual bool OnCommandUserList(UserSession &clSession);
	virtual bool OnCommandWhere(UserSession &clSession);
	virtual bool OnCommandKick(UserSession &clSession, const std::string &strChannel, 
					const std::string &strHostmask, const std::string &strReason);
	virtual bool OnCommandSquelch(UserSession &clSession, const std::string &strHostmask);
	virtual bool OnCommandUnsquelch(UserSession &clSession, const std::string &strHostmask);
	virtual bool OnCommandUserAdd(UserSession &clSession, const std::string &strHostmask, 
					int iAccessLevel, const std::string &strPassword);
	virtual bool OnCommandUserDel(UserSession &clSession, const std::string &strHostmask);
	virtual bool OnCommandShitAdd(UserSession &clSession, const std::string &strHostmask);
	virtual bool OnCommandShitDel(UserSession &clSession, const std::string &strHostmask);
	virtual bool OnCommandShitList(UserSession &clSession);
	virtual bool OnCommandNick(UserSession &clSession, const std::string &strNickname);

private:
	struct MaskMatches {
		MaskMatches(const IrcUser &clUser_)
		: clUser(clUser_) { }

		bool operator()(const IrcUser &clMask) const {
			return clMask.Matches(clUser);
		}

		const IrcUser &clUser;
	};

	std::string m_strServer, m_strPort;
	struct event *m_pConnectTimer, *m_pFloodTimer;

	bool m_bChatter;

	std::vector<std::string> m_vHomeChannels;
	std::vector<IrcUser> m_vSquelchedUsers;
	std::vector<BnxChannel> m_vCurrentChannels;
	BnxResponseEngine m_clResponseEngine;
	BnxAccessSystem m_clAccessSystem;
	BnxShitList m_clShitList;
	BnxFloodDetector m_clFloodDetector;

	template<void (BnxBot::*Method)(int, short)>
	static void Dispatch(int fd, short what, void *arg) {
		BnxBot *pObject = (BnxBot *)arg;
		(pObject->*Method)(fd, what);
	}

	void AddChannel(const char *pChannel);
	void DeleteChannel(const char *pChannel);
	void Squelch(const IrcUser &clUser);
	void Unsquelch(const IrcUser &clser);

	void OnConnectTimer(int fd, short what);
	void OnFloodTimer(int fd, short what);
};

#endif // !BNXBOT_H

