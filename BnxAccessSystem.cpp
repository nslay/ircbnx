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
#include "BnxAccessSystem.h"

bool BnxAccessSystem::LoadFromStream(std::istream &is) {
	if (!is) {
		std::cerr << "Bad stream." << std::endl;
		return false;
	}

	Reset();

	std::string strLine;
	while (std::getline(is, strLine)) {
		if (strLine.empty() || strLine[0] == ';')
			continue;

		std::stringstream ss;

		ss.str(strLine);

		std::string strHostmask, strPassword;
		int iAccessLevel;

		if (!(ss >> strHostmask >> iAccessLevel >> strPassword)) {
			std::cerr << "Could not read host mask, access level or password." << std::endl;
			Reset();
			return false;
		}

		AddUser(IrcUser(strHostmask), iAccessLevel, strPassword);
	}

	return true;
}

void BnxAccessSystem::SaveToStream(std::ostream &os) {
	for (size_t i = 0; i < m_vUserEntries.size(); ++i) {
		UserEntry &clEntry = m_vUserEntries[i];
		os << clEntry.GetHostmask().GetHostmask() << " " << clEntry.GetAccessLevel() << " " << clEntry.GetPassword() << std::endl;
	}
}

void BnxAccessSystem::AddUser(const IrcUser &clHostmask, int iAccessLevel, const std::string &strPassword) {
	std::vector<UserEntry>::iterator itr;

	itr = std::find(m_vUserEntries.begin(), m_vUserEntries.end(), clHostmask);

	if (itr == m_vUserEntries.end())
		m_vUserEntries.push_back(UserEntry(clHostmask, strPassword, iAccessLevel));
	else
		*itr = UserEntry(clHostmask, strPassword, iAccessLevel);
}

bool BnxAccessSystem::DeleteUser(const IrcUser &clHostmask) {
	std::vector<UserEntry>::iterator itr;

	itr = std::find(m_vUserEntries.begin(), m_vUserEntries.end(), clHostmask);

	if (itr == m_vUserEntries.end())
		return false;

	m_vUserEntries.erase(itr);

	return true;
}

bool BnxAccessSystem::Login(const IrcUser &clUser, const std::string &strPassword) {
	std::vector<UserEntry>::const_iterator itr;

	itr = std::find_if(m_vUserEntries.begin(), m_vUserEntries.end(), EntryMatches(clUser));

	if (itr == m_vUserEntries.end() || !itr->CheckPassword(strPassword))
		return false;

	std::vector<UserSession>::iterator sessionItr;
	sessionItr = std::find(m_vUserSessions.begin(), m_vUserSessions.end(), clUser);

	if (sessionItr == m_vUserSessions.end())
		m_vUserSessions.push_back(UserSession(clUser,itr->GetAccessLevel()));
	else
		*sessionItr = UserSession(clUser,itr->GetAccessLevel()); // The access level may have been updated

	return true;
}

void BnxAccessSystem::Logout(const IrcUser &clUser) {
	std::vector<UserSession>::iterator itr;

	itr = std::find(m_vUserSessions.begin(), m_vUserSessions.end(), clUser);

	if (itr != m_vUserSessions.end())
		m_vUserSessions.erase(itr);
}

BnxAccessSystem::UserSession * BnxAccessSystem::GetSession(const IrcUser &clUser) {
	std::vector<UserSession>::iterator itr;

	itr = std::find(m_vUserSessions.begin(), m_vUserSessions.end(), clUser);

	if (itr == m_vUserSessions.end())
		return NULL;

	if (itr->LastAccessTime() >= TIMEOUT) {
		m_vUserSessions.erase(itr);
		return NULL;
	}

	itr->Update();

	return &(*itr);
}

BnxAccessSystem::UserEntry * BnxAccessSystem::GetEntry(const IrcUser &clHostmask) {
	std::vector<UserEntry>::iterator itr;

	itr = std::find(m_vUserEntries.begin(), m_vUserEntries.end(), clHostmask);

	if (itr == m_vUserEntries.end())
		return NULL;

	return &(*itr);
}

void BnxAccessSystem::TimeoutSessions() {
	std::vector<UserSession>::iterator itr;
		
	itr = m_vUserSessions.begin();

	while (itr != m_vUserSessions.end()) {
		if (itr->LastAccessTime() >= TIMEOUT)
			itr = m_vUserSessions.erase(itr);
		else
			++itr;
	}
}

