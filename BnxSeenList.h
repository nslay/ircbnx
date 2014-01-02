/*-
 * Copyright (c) 2013 Nathan Lay (nslay@users.sourceforge.net)
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

#ifndef BNXSEENLIST_H
#define BNXSEENLIST_H

#include <ctime>
#include <iostream>
#include <string>
#include <map>
#include "IrcUser.h"
#include "IrcString.h"

class BnxSeenList {
public:
	enum { EXPIRE_TIME_IN_DAYS = 90 };

	class SeenInfo {
	public:
		SeenInfo() {
			Reset();
		}

		SeenInfo(const IrcUser &clUser, const std::string &strChannel) {
			Reset();
			SetUser(clUser);
			SetChannel(strChannel);
			Update();
		}

		const IrcUser & GetUser() const {
			return m_clUser;
		}

		const std::string & GetChannel() const {
			return m_strChannel;
		}

		time_t GetTimestamp() const {
			return m_timeStamp;
		}

		void SetUser(const IrcUser &clUser) {
			m_clUser = clUser;
		}

		void SetChannel(const std::string &strChannel) {
			m_strChannel = strChannel;
		}

		void SetTimestamp(time_t timeStamp) {
			m_timeStamp = timeStamp;
		}

		bool IsExpired() const {
			return (time(nullptr)-m_timeStamp) > (EXPIRE_TIME_IN_DAYS*60*60*24);
		}

		void Update() {
			time(&m_timeStamp);
		}

		void Reset() {
			m_clUser.Reset();
			m_strChannel.clear();
			m_timeStamp = 0;
		}

		bool operator==(const SeenInfo &clSeenInfo) const {
			const std::string &strNickname1 = GetUser().GetNickname();
			const std::string &strNickname2 = clSeenInfo.GetUser().GetNickname();

			return !IrcStrCaseCmp(strNickname1.c_str(), strNickname2.c_str());
		}

	private:
		IrcUser m_clUser;
		std::string m_strChannel;
		time_t m_timeStamp;
	};

	struct StringLessThan {
		bool operator()(const std::string &strString1, const std::string &strString2) const {
			return IrcStrCaseCmp(strString1.c_str(), strString2.c_str()) < 0;
		}
	};

	typedef std::map<std::string, SeenInfo, StringLessThan> MapType;
	typedef MapType::const_iterator Iterator;

	BnxSeenList() {
		m_strSeenListFile = "seen.lst";
	}

	void SetSeenListFile(const std::string &strSeenListFile) {
		m_strSeenListFile = strSeenListFile;
	}

	const std::string & GetSeenListFile() const {
		return m_strSeenListFile;
	}

	Iterator Begin() const {
		return m_mSeenMap.begin();
	}

	Iterator End() const {
		return m_mSeenMap.end();
	}

	Iterator Find(const std::string &strNickname) const {
		return m_mSeenMap.find(strNickname);
	}

	void Saw(const IrcUser &clUser, const std::string &strChannel) {
		m_mSeenMap[clUser.GetNickname()] = SeenInfo(clUser, strChannel);
	}

	bool Load();

	void Save() const;

	void Reset() {
		m_mSeenMap.clear();
	}

	void ExpireEntries();

private:
	std::string m_strSeenListFile;
	MapType m_mSeenMap;
};

inline std::ostream & operator<<(std::ostream &os, const BnxSeenList::SeenInfo &clSeenInfo) {
	return os << clSeenInfo.GetUser() << ' ' << clSeenInfo.GetChannel() << ' ' << clSeenInfo.GetTimestamp();
}

inline std::istream & operator>>(std::istream &is, BnxSeenList::SeenInfo &clSeenInfo) {
	IrcUser clUser;
	std::string strChannel;
	time_t timeStamp;

	is >> clUser >> strChannel >> timeStamp;

	clSeenInfo.SetUser(clUser);
	clSeenInfo.SetChannel(strChannel);
	clSeenInfo.SetTimestamp(timeStamp);

	return is;
}

#endif // !BNXSEENLIST_H

