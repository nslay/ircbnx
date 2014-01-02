/*-
 * Copyright (c) 2012-2013 Nathan Lay (nslay@users.sourceforge.net)
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

#include <iostream>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "BnxChannel.h"

void BnxChannel::AddMember(const IrcUser &clUser) {
	if (GetMember(clUser.GetNickname()) != MemberEnd())
		return;

	m_vMembers.push_back(Member(clUser));
}

BnxChannel::MemberIterator BnxChannel::GetMember(const std::string &strNickname) {
	MemberIterator memberItr;

	for (memberItr = MemberBegin(); memberItr != MemberEnd(); ++memberItr) {
		const IrcUser &clUser = memberItr->GetUser();

		if (!IrcStrCaseCmp(strNickname.c_str(), clUser.GetNickname().c_str(), m_eCaseMapping))
			return memberItr;
	}

	return MemberEnd();
}

BnxChannel::ConstMemberIterator BnxChannel::GetMember(const std::string &strNickname) const {
	ConstMemberIterator memberItr;

	for (memberItr = MemberBegin(); memberItr != MemberEnd(); ++memberItr) {
		const IrcUser &clUser = memberItr->GetUser();

		if (!IrcStrCaseCmp(strNickname.c_str(), clUser.GetNickname().c_str(), m_eCaseMapping))
			return memberItr;
	}

	return MemberEnd();
}

void BnxChannel::DeleteMember(const std::string &strNickname) {
	MemberIterator memberItr = GetMember(strNickname);

	if (memberItr != MemberEnd())
		DeleteMember(memberItr);
}

void BnxChannel::UpdateMember(const std::string &strNick, const std::string &strNewNick) {
	MemberIterator memberItr = GetMember(strNick);

	if (memberItr != MemberEnd())
		memberItr->GetUser().SetNickname(strNewNick);

	if (IsVoteBanInProgress() && !IrcStrCaseCmp(m_clVoteBanMask.GetNickname().c_str(), strNick.c_str()))
		m_clVoteBanMask.SetNickname(strNewNick);
}

BnxChannel::WarningIterator BnxChannel::Warn(const std::string &strHostname) {
	WarningIterator warningItr = GetWarningEntry(strHostname);

	if (warningItr == WarningEnd()) 
		warningItr = m_vWarnings.insert(m_vWarnings.end(), WarningEntry(strHostname));

	warningItr->Warn();

	return warningItr;
}

void BnxChannel::DeleteWarningEntry(const std::string &strHostname) {
	WarningIterator warningItr = GetWarningEntry(strHostname);

	if (warningItr != WarningEnd())
		DeleteWarningEntry(warningItr);
}

void BnxChannel::ExpireWarningEntries() {
	WarningIterator warningItr;

	warningItr = WarningBegin();
	while (warningItr != WarningEnd()) {
		if (warningItr->IsExpired())
			warningItr = DeleteWarningEntry(warningItr);
		else
			++warningItr;
	}
}

void BnxChannel::ResetVoteBan() {
	m_bVoteBan = false;
	m_clVoteBanMask.Reset();
	m_voteBanTime = 0;
	m_iVoteCount = 0;

	for (size_t i = 0; i < m_vMembers.size(); ++i)
		m_vMembers[i].ResetVote();
}

void BnxChannel::Reset() {
	m_strName.clear();
	m_eCaseMapping = RFC1459;
	m_vMembers.clear();
	m_vWarnings.clear();
	m_bIsOperator = false;
	m_clFloodDetector.Reset();

	ResetVoteBan();
}

void BnxChannel::VoteBan(const IrcUser &clUser) {
	ResetVoteBan();

	m_bVoteBan = true;
	m_clVoteBanMask = clUser;
	m_voteBanTime = time(nullptr);
}

void BnxChannel::VoteYay(const std::string &strNickname) {
	MemberIterator memberItr = GetMember(strNickname);

	// Only allow those members who were present prior to the voteban
	if (memberItr == MemberEnd() || memberItr->GetTimeStamp() > m_voteBanTime)
		return;

	int iOldVote = memberItr->GetVote();

	memberItr->VoteYay();

	m_iVoteCount += memberItr->GetVote() - iOldVote;
}

void BnxChannel::VoteNay(const std::string &strNickname) {
	MemberIterator memberItr = GetMember(strNickname);

	// Only allow those members who were present prior to the voteban
	if (memberItr == MemberEnd() || memberItr->GetTimeStamp() > m_voteBanTime)
		return;

	int iOldVote = memberItr->GetVote();

	memberItr->VoteNay();

	m_iVoteCount += memberItr->GetVote() - iOldVote;
}

