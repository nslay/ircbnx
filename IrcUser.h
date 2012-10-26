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

#ifndef IRCUSER_H
#define IRCUSER_H

#include <string>
#include "IrcString.h"

class IrcUser {
public:
	IrcUser() { }

	explicit IrcUser(const std::string &strHostmask) {
		Parse(strHostmask);
	}

	IrcUser(const std::string &strNickname, const std::string &strUsername, const std::string &strHostname) {
		Set(strNickname, strUsername, strHostname);
	}

	void Parse(const std::string &strHostmask);

	void Set(const std::string &strNickname, const std::string &strUsername, const std::string &strHostname) {
		SetNickname(strNickname);
		SetUsername(strUsername);
		SetHostname(strHostname);
	}

	void SetNickname(const std::string &strNickname) {
		m_strNickname = strNickname.empty() ? "*" : strNickname;
	}

	void SetUsername(const std::string &strUsername) {
		m_strUsername = strUsername.empty() ? "*" : strUsername;
	}

	void SetHostname(const std::string &strHostname) {
		m_strHostname = strHostname.empty() ? "*" : strHostname;
	}

	const std::string & GetNickname() const {
		return m_strNickname;
	}

	const std::string & GetUsername() const {
		return m_strUsername;
	}

	const std::string & GetHostname() const {
		return m_strHostname;
	}

	const std::string GetHostmask() const {
		std::string strHostmask = GetNickname();
		strHostmask += '!';
		strHostmask += GetUsername();
		strHostmask += '@';
		strHostmask += GetHostname();
		return strHostmask;
	}

	void Reset() {
		m_strNickname = '*';
		m_strUsername = '*';
		m_strHostname = '*';
	}

	bool Matches(const IrcUser &clUser) const {
		return this == &clUser ||
			(IrcMatch(GetNickname().c_str(),clUser.GetNickname().c_str()) &&
			IrcMatch(GetUsername().c_str(),clUser.GetUsername().c_str()) &&
			IrcMatch(GetHostname().c_str(),clUser.GetHostname().c_str()));
	}

	bool operator==(const IrcUser &clUser) const {
		return this == &clUser ||
			(!IrcStrCaseCmp(GetNickname().c_str(), clUser.GetNickname().c_str()) &&
			!IrcStrCaseCmp(GetUsername().c_str(), clUser.GetUsername().c_str()) &&
			!IrcStrCaseCmp(GetHostname().c_str(), clUser.GetHostname().c_str()));
	}

	bool operator!=(const IrcUser &clUser) const {
		return !(*this == clUser);
	}

private:
	std::string m_strNickname, m_strUsername, m_strHostname;
};

#endif // !IRCUSER_H

