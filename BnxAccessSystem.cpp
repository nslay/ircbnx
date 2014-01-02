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
	IrcUser clMask;
	int iAccessLevel;
	std::string strPassword;

	if (is >> clMask >> iAccessLevel >> strPassword)
		clEntry = BnxAccessSystem::UserEntry(clMask, iAccessLevel, strPassword);

	return is;
}

std::ostream & operator<<(std::ostream &os, const BnxAccessSystem::UserEntry &clEntry) {
	return os << clEntry.GetHostmask() << ' ' << 
			clEntry.GetAccessLevel() << ' ' <<
			clEntry.GetPassword();
			
}

void BnxAccessSystem::AddUser(const UserEntry &clNewEntry) {
	EntryIterator entryItr = GetEntry(clNewEntry.GetHostmask());

	if (entryItr == EntryEnd())
		m_vUserEntries.push_back(clNewEntry);
	else
		*entryItr = clNewEntry;
}

bool BnxAccessSystem::DeleteUser(const IrcUser &clHostmask) {
	EntryIterator entryItr = GetEntry(clHostmask);

	if (entryItr == EntryEnd())
		return false;

	DeleteUser(entryItr);

	return true;
}

bool BnxAccessSystem::Login(const IrcUser &clUser, const std::string &strPassword) {
	ConstEntryIterator entryItr;

	entryItr = std::find_if(EntryBegin(), EntryEnd(), [&clUser](const UserEntry &clEntry) {
		return clEntry.GetHostmask().Matches(clUser);
	});

	if (entryItr == EntryEnd() || !entryItr->CheckPassword(strPassword))
		return false;

	SessionIterator sessionItr = GetSession(clUser);

	if (sessionItr == SessionEnd())
		m_vUserSessions.push_back(UserSession(clUser,entryItr->GetAccessLevel()));
	else
		*sessionItr = UserSession(clUser,entryItr->GetAccessLevel()); // The access level may have been updated

	return true;
}

void BnxAccessSystem::Logout(const IrcUser &clUser) {
	SessionIterator itr = GetSession(clUser);

	if (itr != SessionEnd())
		Logout(itr);
}

BnxAccessSystem::SessionIterator BnxAccessSystem::GetSession(const IrcUser &clUser) {
	SessionIterator itr = std::find(SessionBegin(), SessionEnd(), clUser);

	if (itr == SessionEnd())
		return SessionEnd();

	if (itr->LastAccessTime() >= TIMEOUT) {
		Logout(itr);
		return SessionEnd();
	}

	itr->Update();

	return itr;
}

void BnxAccessSystem::TimeoutSessions() {
	SessionIterator itr = m_vUserSessions.begin();

	while (itr != SessionEnd()) {
		if (itr->LastAccessTime() >= TIMEOUT)
			itr = Logout(itr);
		else
			++itr;
	}
}

