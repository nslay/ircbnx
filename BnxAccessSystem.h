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

#ifndef BNXACCESSSYSTEM_H
#define BNXACCESSSYSTEM_H

#include <ctime>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <vector>
#include "BnxListIo.h"
#include "IrcUser.h"

class BnxAccessSystem {
public:
	// Original BNX appears to have a 10 minute timeout
	enum { TIMEOUT = 600 };

	class UserEntry {
	public:
		UserEntry()
		: m_iAccessLevel(0) { }

		UserEntry(const IrcUser &clHostmask, int iLevel, const std::string &strPassword)
		: m_clHostmask(clHostmask), m_strPassword(strPassword), m_iAccessLevel(iLevel) { }

		const IrcUser & GetHostmask() const {
			return m_clHostmask;
		}

		const std::string & GetPassword() const {
			return m_strPassword;
		}

		int GetAccessLevel() const {
			return m_iAccessLevel;
		}

		bool CheckPassword(const std::string &strPassword) const {
			return m_strPassword == strPassword;
		}

		bool operator==(const IrcUser &clUser) const {
			return m_clHostmask == clUser;
		}

		bool operator!=(const IrcUser &clUser) const {
			return !(*this == clUser);
		}

		bool operator==(const UserEntry &clEntry) const {
			return clEntry == m_clHostmask;
		}

		bool operator!=(const UserEntry &clEntry) const {
			return !(*this == clEntry);
		}

		void SetHostmask(const IrcUser &clMask) {
			m_clHostmask = clMask;
		}

		void SetPassword(const std::string &strPassword) {
			m_strPassword = strPassword;
		}

		void SetAccessLevel(int iAccessLevel) {
			m_iAccessLevel = iAccessLevel;
		}

	private:
		IrcUser m_clHostmask;
		int m_iAccessLevel;
		std::string m_strPassword;
	};

	typedef std::vector<UserEntry>::iterator EntryIterator;
	typedef std::vector<UserEntry>::const_iterator ConstEntryIterator;

	class UserSession {
	public:
		UserSession()
		: m_iAccessLevel(0), m_lastAccessTime(0) { }

		UserSession(const IrcUser &clUser, int iAccessLevel)
		: m_clUser(clUser), m_iAccessLevel(iAccessLevel), m_lastAccessTime(time(nullptr)) { }

		const IrcUser & GetUser() const {
			return m_clUser;
		}

		int GetAccessLevel() const {
			return m_iAccessLevel;
		}

		time_t LastAccessTime() const {
			return time(nullptr)-m_lastAccessTime;
		}

		void Update() {
			m_lastAccessTime = time(nullptr);
		}

		bool operator==(const IrcUser &clUser) const {
			return m_clUser == clUser;
		}

		bool operator!=(const IrcUser &clUser) const {
			return !(*this == clUser);
		}
		
	private:
		IrcUser m_clUser;
		int m_iAccessLevel;
		time_t m_lastAccessTime;
	};

	typedef std::vector<UserSession>::iterator SessionIterator;
	typedef std::vector<UserSession>::const_iterator ConstSessionIterator;

	BnxAccessSystem()
	: m_strAccessListFile("access.lst") { }

	EntryIterator EntryBegin() {
		return m_vUserEntries.begin();
	}

	ConstEntryIterator EntryBegin() const {
		return m_vUserEntries.begin();
	}

	EntryIterator EntryEnd() {
		return m_vUserEntries.end();
	}

	ConstEntryIterator EntryEnd() const {
		return m_vUserEntries.end();
	}

	SessionIterator SessionBegin() {
		return m_vUserSessions.begin();
	}

	ConstSessionIterator SessionBegin() const {
		return m_vUserSessions.begin();
	}

	SessionIterator SessionEnd() {
		return m_vUserSessions.end();
	}

	ConstSessionIterator SessionEnd() const {
		return m_vUserSessions.end();
	}

	void SetAccessListFile(const std::string &strAccessListFile) {
		m_strAccessListFile = strAccessListFile;
	}

	const std::string & GetAccessListFile() const {
		return m_strAccessListFile;
	}

	bool Load() {
		Reset();
		return BnxLoadList(m_strAccessListFile.c_str(), m_vUserEntries);
	}

	void Save() const {
		BnxSaveList(m_strAccessListFile.c_str(), m_vUserEntries);
	}

	void AddUser(const UserEntry &clEntry);
	void AddUser(const IrcUser &clHostmask, int iAccessLevel, const std::string &strPassword) {
		AddUser(UserEntry(clHostmask, iAccessLevel, strPassword));
	}

	bool DeleteUser(const IrcUser &clHostmask);
	EntryIterator DeleteUser(EntryIterator entryItr) {
		return m_vUserEntries.erase(entryItr);
	}

	bool Login(const IrcUser &clUser, const std::string &strPassword);
	void Logout(const IrcUser &clUser);

	SessionIterator Logout(SessionIterator sessionItr) {
		return m_vUserSessions.erase(sessionItr);
	}

	SessionIterator GetSession(const IrcUser &clUser);

	EntryIterator GetEntry(const IrcUser &clHostmask) {
		return std::find(EntryBegin(), EntryEnd(), clHostmask);
	}

	ConstEntryIterator GetEntry(const IrcUser &clHostmask) const {
		return std::find(EntryBegin(), EntryEnd(), clHostmask);
	}

	void TimeoutSessions();

	void ResetSessions() {
		m_vUserSessions.clear();
	}

	void Reset() {
		m_vUserEntries.clear();
		m_vUserSessions.clear();
	}

private:
	std::string m_strAccessListFile;
	std::vector<UserEntry> m_vUserEntries;
	std::vector<UserSession> m_vUserSessions;
};

std::istream & operator>>(std::istream &is, BnxAccessSystem::UserEntry &clEntry);
std::ostream & operator<<(std::ostream &os, const BnxAccessSystem::UserEntry &clEntry);

#endif // !BNXACCESSSYSTEM_H

