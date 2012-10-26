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
#include "IrcUser.h"

class BnxAccessSystem {
public:
	// Original BNX appears to have a 10 minute timeout
	enum { TIMEOUT = 600 };

	class UserEntry {
	public:
		UserEntry()
		: m_iAccessLevel(0) { }

		UserEntry(const IrcUser &clHostmask, const std::string &strPassword, int iLevel)
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

	private:
		IrcUser m_clHostmask;
		std::string m_strPassword;
		int m_iAccessLevel;
	};

	class UserSession {
	public:
		UserSession()
		: m_iAccessLevel(0), m_lastAccessTime(0) { }

		UserSession(const IrcUser &clUser, int iAccessLevel)
		: m_clUser(clUser), m_iAccessLevel(iAccessLevel), m_lastAccessTime(time(NULL)) { }

		const IrcUser & GetUser() const {
			return m_clUser;
		}

		int GetAccessLevel() const {
			return m_iAccessLevel;
		}

		time_t LastAccessTime() const {
			return time(NULL)-m_lastAccessTime;
		}

		void Update() {
			m_lastAccessTime = time(NULL);
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


	bool LoadFromStream(std::istream &is);
	void SaveToStream(std::ostream &os);

	void AddUser(const IrcUser &clHostmask, int iAccessLevel, const std::string &strPassword);
	bool DeleteUser(const IrcUser &clHostmask);

	bool Login(const IrcUser &clUser, const std::string &strPassword);
	void Logout(const IrcUser &clUser);

	UserSession * GetSession(const IrcUser &clUser);
	UserEntry * GetEntry(const IrcUser &clHostmask);

	const std::vector<UserSession> & GetAllSessions() const {
		return m_vUserSessions;
	}

	const std::vector<UserEntry> & GetAllEntries() const {
		return m_vUserEntries;
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
	struct EntryMatches {
		EntryMatches(const IrcUser &clUser_)
		: clUser(clUser_) { }
	
		bool operator()(const UserEntry &clEntry) const {
			return clEntry.GetHostmask().Matches(clUser);
		}

		const IrcUser &clUser;
	};

	std::vector<UserEntry> m_vUserEntries;
	std::vector<UserSession> m_vUserSessions;

};

#endif // !BNXACCESSSYSTEM_H

