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
#include "IrcString.h"
#include "IrcUser.h"

// Special channel for BnxBot and its functionality

class BnxChannel {
public:
	class Member {
	public:
		Member(const IrcUser &clUser) {
			Reset();
			m_clUser = clUser;
		}

		Member() {
			Reset();
		}

		void AddPrefix(char prefix);
		void RemovePrefix(char prefix);

		void SetPrefix(const std::string &strPrefix) {
			m_strPrefix = strPrefix;
		}

		const std::string & GetPrefix() const {
			return m_strPrefix;
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
			m_strPrefix.clear();
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
		std::string m_strPrefix;
		IrcUser m_clUser;
		time_t m_timeStamp;

		// For voteban
		int m_iVote;
	};

	BnxChannel() {
		Reset();
	}

	BnxChannel(const std::string &strName) {
		Reset();
		m_strName = strName;
	}

	void AddMembers(const std::string &strNameReply);

	void AddMember(const IrcUser &clUser);

	Member * GetMember(const std::string &strNickname);

	void DeleteMember(const std::string &strNickname);

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

	void ResetVoteBan();
	void Reset();

	const std::string & GetName() const {
		return m_strName;
	}

	size_t GetSize() const {
		return m_vMembers.size();
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

	// For voteban
	bool m_bVoteBan;
	IrcUser m_clVoteBanMask;
	time_t m_voteBanTime;
	int m_iVoteCount;
};

#endif // !BNXCHANNEL_H

