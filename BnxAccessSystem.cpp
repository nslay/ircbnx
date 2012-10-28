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

#include <fstream>
#include <sstream>
#include "BnxAccessSystem.h"

std::istream & operator>>(std::istream &is, BnxAccessSystem::UserEntry &clEntry) {
	std::string strHostmask, strPassword;
	int iAccessLevel;

	if (is >> strHostmask >> iAccessLevel >> strPassword)
		clEntry = BnxAccessSystem::UserEntry(IrcUser(strHostmask), iAccessLevel, strPassword);

	return is;
}

std::ostream & operator<<(std::ostream &os, const BnxAccessSystem::UserEntry &clEntry) {
	return os << clEntry.GetHostmask().GetHostmask() << ' ' << 
			clEntry.GetAccessLevel() << ' ' <<
			clEntry.GetPassword();
			
}

bool BnxAccessSystem::Load() {
	std::ifstream inAccessStream(GetAccessListFile().c_str());

	Reset();

	std::string strLine;
	while (std::getline(inAccessStream, strLine)) {
		if (strLine.empty() || strLine[0] == ';')
			continue;

		std::stringstream lineStream;

		lineStream.str(strLine);

		UserEntry clEntry;

		if (!(lineStream >> clEntry)) {
			std::cerr << "Could not read user entry." << std::endl;
			Reset();
			return false;
		}

		AddUser(clEntry);
	}

	return true;
}

void BnxAccessSystem::Save() {
	std::ifstream inAccessStream(GetAccessListFile().c_str());

	if (inAccessStream) {
		// Try to preserve comments and formatting
		std::stringstream tmpAccessStream;
		std::vector<IrcUser> vEntriesWritten;

		std::string strLine;

		while (std::getline(inAccessStream, strLine)) {
			if (strLine.empty() || strLine[0] == ';') {
				tmpAccessStream << strLine << std::endl;
				continue;
			}

			std::stringstream lineStream;
			lineStream.str(strLine);

			UserEntry clTmpEntry;

			if (!(lineStream >> clTmpEntry)) {
				std::cerr << "Could not read user entry ... ignoring" << std::endl;
				continue;
			}

			BnxAccessSystem::UserEntry *pclEntry = GetEntry(clTmpEntry.GetHostmask());
			if (pclEntry != NULL) {
				vEntriesWritten.push_back(pclEntry->GetHostmask());
				tmpAccessStream << *pclEntry << std::endl;
			}
		}

		// End with a comment or entry?
		if (!strLine.empty())
			tmpAccessStream << std::endl;

		inAccessStream.close();

		std::ofstream outAccessStream(GetAccessListFile().c_str());

		outAccessStream << tmpAccessStream.str();

		for (size_t i = 0; i < m_vUserEntries.size(); ++i) {
			const UserEntry &clEntry = m_vUserEntries[i];

			if (std::find(vEntriesWritten.begin(), vEntriesWritten.end(), 
					clEntry.GetHostmask()) == vEntriesWritten.end()) {
				outAccessStream << clEntry << std::endl;
			}
		}
	}
	else {
		// No such file
		std::ofstream outAccessStream(GetAccessListFile().c_str());

		for (size_t i = 0; i < m_vUserEntries.size(); ++i)
			outAccessStream << m_vUserEntries[i] << std::endl;
	}
}

void BnxAccessSystem::AddUser(const UserEntry &clNewEntry) {
	UserEntry *pclEntry = GetEntry(clNewEntry.GetHostmask());

	if (pclEntry == NULL)
		m_vUserEntries.push_back(clNewEntry);
	else
		*pclEntry = clNewEntry;
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

