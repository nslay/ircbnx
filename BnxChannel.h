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

#ifndef BNXCHANNEL_H
#define BNXCHANNEL_H

#include <ctime>
#include <string>
#include <vector>
#include "BnxFloodDetector.h"
#include "IrcString.h"
#include "IrcUser.h"

// Special channel for BnxBot and its functionality

class BnxChannel {
public:
	enum { VOTEBAN_TIMEOUT = 30, WARNING_TIMEOUT = 600 };

	class Member {
	public:
		Member(const IrcUser &clUser) {
			Reset();
			m_clUser = clUser;
		}

		Member() {
			Reset();
		}

		const IrcUser & GetUser() const {
			return m_clUser;
		}

		IrcUser & GetUser() {
			return m_clUser;
		}

		time_t GetTimeStamp() const {
			return m_timeStamp;
		}

		void VoteYay() {
			m_iVote = 1;
		}

		void VoteNay() {
			m_iVote = -1;
		}

		int GetVote() const {
			return m_iVote;
		}

		void ResetVote() {
			m_iVote = 0;
		}

		void Reset() {
			m_clUser.Reset();
			m_timeStamp = time(NULL);
			ResetVote();
		}

		bool operator==(const std::string &strNickname) const {
			return !IrcStrCaseCmp(m_clUser.GetNickname().c_str(), strNickname.c_str());
		}

		bool operator!=(const std::string &strNickname) const {
			return !(*this == strNickname);
		}

	private:
		IrcUser m_clUser;
		time_t m_timeStamp;

		// For voteban
		int m_iVote;
	};

	class WarningEntry {
	public:
		WarningEntry(){
			Reset();
		}

		WarningEntry(const std::string &strHostname) {
			Reset();
			m_strHostname = strHostname;
		}

		const std::string & GetHostname() const {
			return m_strHostname;
		}

		time_t GetTimeStamp() const {
			return m_timeStamp;
		}

		unsigned int GetCount() const {
			return m_uiCount;
		}

		void Warn() {
			++m_uiCount;
			m_timeStamp = time(NULL);
		}

		void Reset() {
			m_strHostname.clear();
			m_timeStamp = time(NULL);
			m_uiCount = 0;
		}

		bool IsExpired() const {
			return time(NULL)-m_timeStamp > WARNING_TIMEOUT;
		}

		bool operator==(const std::string &strHostname) const {
			return !IrcStrCaseCmp(m_strHostname.c_str(), strHostname.c_str());
		}

		bool operator!=(const std::string &strHostname) const {
			return !(*this == strHostname);
		}

	private:
		std::string m_strHostname;
		time_t m_timeStamp;
		unsigned int m_uiCount;
	};

	typedef std::vector<Member>::iterator MemberIterator;
	typedef std::vector<Member>::const_iterator ConstMemberIterator;
	typedef std::vector<WarningEntry>::iterator WarningIterator;
	typedef std::vector<WarningEntry>::const_iterator ConstWarningIterator;

	BnxChannel() {
		Reset();
	}

	BnxChannel(const std::string &strName) {
		Reset();
		m_strName = strName;
	}

	MemberIterator MemberBegin() {
		return m_vMembers.begin();
	}

	ConstMemberIterator MemberBegin() const {
		return m_vMembers.begin();
	}

	MemberIterator MemberEnd() {
		return m_vMembers.end();
	}

	ConstMemberIterator MemberEnd() const {
		return m_vMembers.end();
	}

	void AddMember(const IrcUser &clUser);

	MemberIterator GetMember(const std::string &strNickname) {
		return std::find(MemberBegin(), MemberEnd(), strNickname);
	}

	ConstMemberIterator GetMember(const std::string &strNickname) const {
		return std::find(MemberBegin(), MemberEnd(), strNickname);
	}

	void DeleteMember(const std::string &strNickname);

	MemberIterator DeleteMember(MemberIterator memberItr) {
		return m_vMembers.erase(memberItr);
	}

	void UpdateMember(const std::string &strNick, const std::string &strNewNick);

	WarningIterator WarningBegin() {
		return m_vWarnings.begin();
	}

	ConstWarningIterator WarningBegin() const {
		return m_vWarnings.begin();
	}

	WarningIterator WarningEnd() {
		return m_vWarnings.end();
	}

	ConstWarningIterator WarningEnd() const {
		return m_vWarnings.end();
	}

	WarningIterator GetWarningEntry(const std::string &strHostname) {
		return std::find(WarningBegin(), WarningEnd(), strHostname);
	}

	ConstWarningIterator GetWarningEntry(const std::string &strHostname) const {
		return std::find(WarningBegin(), WarningEnd(), strHostname);
	}

	WarningIterator Warn(const std::string &strHostname);

	void DeleteWarningEntry(const std::string &strHostname);

	WarningIterator DeleteWarningEntry(WarningIterator warningItr) {
		return m_vWarnings.erase(warningItr);
	}

	void ExpireWarningEntries();

	bool IsOperator() const {
		return m_bIsOperator;
	}

	void SetOperator(bool bIsOperator) {
		m_bIsOperator = bIsOperator;
	}

	void VoteBan(const IrcUser &clUser);

	void VoteYay(const std::string &strNickname);
	void VoteNay(const std::string &strNickname);

	int TallyVote() const {
		return m_iVoteCount;
	}

	bool IsVoteBanInProgress() const {
		return m_bVoteBan;
	}

	const IrcUser & GetVoteBanMask() const {
		return m_clVoteBanMask;
	}

	time_t GetVoteBanTime() const {
		return m_voteBanTime;
	}

	bool VoteBanExpired() const {
		return time(NULL) - GetVoteBanTime() >= VOTEBAN_TIMEOUT;
	}

	void ResetVoteBan();
	void Reset();

	const std::string & GetName() const {
		return m_strName;
	}

	size_t GetSize() const {
		return m_vMembers.size();
	}

	BnxFloodDetector & GetFloodDetector() {
		return m_clFloodDetector;
	}

	bool operator==(const std::string &strName) const {
		return !IrcStrCaseCmp(m_strName.c_str(), strName.c_str());
	}

	bool operator!=(const std::string &strName) const {
		return !(*this == strName);
	}

private:
	std::string m_strName;
	std::vector<Member> m_vMembers;
	std::vector<WarningEntry> m_vWarnings;
	bool m_bIsOperator;
	BnxFloodDetector m_clFloodDetector;

	// For voteban
	bool m_bVoteBan;
	IrcUser m_clVoteBanMask;
	time_t m_voteBanTime;
	int m_iVoteCount;
};

#endif // !BNXCHANNEL_H

