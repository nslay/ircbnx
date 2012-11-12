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

#include <sstream>
#include "event2/event.h"
#include "BnxDriver.h"

BnxDriver BnxDriver::ms_clDriver;

BnxDriver::~BnxDriver() {
	Reset();
}

void BnxDriver::Usage() {

}

bool BnxDriver::ParseArgs(int argc, char *argv[]) {
	return true;
}

bool BnxDriver::Load() {
	Reset();

	IniFile clConfig;

	if (!clConfig.Load(m_strConfigFile))
		return false;

	const IniFile::Section &clGlobal = clConfig.GetSection("global");

	std::string strEntries = clGlobal.GetValue<std::string>("profiles", "");
	if (strEntries.empty())
		return false;

	std::stringstream profileStream(strEntries);

	std::string strProfile;
	while (std::getline(profileStream,strProfile,',')) {
		const IniFile::Section &clSection = clConfig.GetSection(strProfile);
		LoadBot(clSection);
	}

	return !m_vBots.empty();
}

bool BnxDriver::Run() {
	if (!Load())
		return false;

	struct event_base *pEventBase;

	pEventBase = event_base_new();

	for (size_t i = 0; i < m_vBots.size(); ++i) {
		m_vBots[i]->SetEventBase(pEventBase);
		m_vBots[i]->StartUp();
	}

	event_base_dispatch(pEventBase);

	event_base_free(pEventBase);

	return true;
}

void BnxDriver::Shutdown() {
	for (size_t i = 0; i < m_vBots.size(); ++i)
		m_vBots[i]->Shutdown();
}

void BnxDriver::Reset() {
	for (size_t i = 0; i < m_vBots.size(); ++i)
		delete m_vBots[i];

	m_vBots.clear();
}

void BnxDriver::LoadBot(const IniFile::Section &clSection) {
	bool bEnabled = clSection.GetValue<bool>("enabled", true);

	if (!bEnabled)
		return;

	std::string strServer = clSection.GetValue<std::string>("server", "");
	std::string strNickname = clSection.GetValue<std::string>("nickname", "");

	if (strServer.empty() || strNickname.empty())
		return;

	BnxBot *pclBot = GetBot(clSection.GetName());

	if (pclBot == NULL) {
		pclBot = new BnxBot();
		pclBot->SetProfileName(clSection.GetName());

		m_vBots.push_back(pclBot);
	}

	std::string strPort = clSection.GetValue<std::string>("port", "6667");
	std::string strUsername = clSection.GetValue<std::string>("username", "BnxBot");
	std::string strRealName = clSection.GetValue<std::string>("realname", "BnxBot");
	std::string strAccessList = clSection.GetValue<std::string>("accesslist", "access.lst");
	std::string strShitList = clSection.GetValue<std::string>("shitlist", "shit.lst");
	std::string strResponseRules = clSection.GetValue<std::string>("responserules", "response.txt");
	std::string strHomeChannels = clSection.GetValue<std::string>("homechannels", "");
	std::string strNickServ = clSection.GetValue<std::string>("nickserv", "");
	std::string strNickServPassword = clSection.GetValue<std::string>("nickservpassword", "");
	
	pclBot->SetServerAndPort(strServer, strPort);
	pclBot->SetNickServAndPassword(strNickServ, strNickServPassword);
	pclBot->SetNickname(strNickname);
	pclBot->SetUsername(strUsername);
	pclBot->SetRealName(strRealName);
	pclBot->LoadResponseRules(strResponseRules);
	pclBot->LoadAccessList(strAccessList);
	pclBot->LoadShitList(strShitList);
	pclBot->SetHomeChannels(strHomeChannels);
}

BnxBot * BnxDriver::GetBot(const std::string &strProfile) const {
	for (size_t i = 0; i < m_vBots.size(); ++i) {
		if (strProfile == m_vBots[i]->GetProfileName())
			return m_vBots[i];
	}

	return NULL;
}

