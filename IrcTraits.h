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

#ifndef IRCTRAITS_H
#define IRCTRAITS_H

#include <string>
#include <utility>
#include <vector>
#include "IrcString.h"

// Support for RPL_ISUPPORT
class IrcTraits {
public:
	enum ChanModesType { TYPE_A = 0, TYPE_B, TYPE_C, TYPE_D };

	IrcTraits() {
		Reset();
	}

	bool Parse(const std::string &strParam);

	void Reset();

	IrcCaseMapping GetCaseMapping() const {
		return m_caseMapping;
	}

	const std::pair<std::string, unsigned int> * GetChanLimit(char prefix) const;

	const std::vector<std::pair<std::string, unsigned int> > & GetChanLimit() const {
		return m_vChanLimit;
	}

	const std::string & GetChanModes(ChanModesType type) const {
		return m_strChanModes[type];
	}

	ChanModesType ClassifyChanMode(char mode) const;

	unsigned int GetChannelLen() const {
		return m_channelLen;
	}

	const std::string & GetChanTypes() const {
		return m_strChanTypes;
	}

	char GetExcepts() const {
		return m_excepts;
	}

	char GetInvex() const {
		return m_invex;
	}

	unsigned int GetKickLen() const {
		return m_kickLen;
	}

	const std::pair<std::string, unsigned int> * GetMaxList(char mode) const;

	const std::vector<std::pair<std::string, unsigned int> > & GetMaxList() const {
		return m_vMaxList;
	}

	unsigned int GetModes() const {
		return m_modes;
	}

	const std::string & GetNetwork() const {
		return m_strNetwork;
	}

	unsigned int GetNickLen() const {
		return m_nickLen;
	}

	const std::pair<std::string, std::string> & GetPrefix() const {
		return m_prefix;
	}

	char GetPrefixByMode(char mode) const;
	char GetModeByPrefix(char prefix) const;

	bool GetSafeList() const {
		return m_safeList;
	}

	const std::string & GetStatusMsg() const {
		return m_strStatusMsg;
	}

	unsigned int GetTopicLen() const {
		return m_topicLen;
	}

private:
	IrcCaseMapping m_caseMapping;
	std::string m_strChanTypes, m_strNetwork, m_strStatusMsg, m_strChanModes[4];
	std::vector<std::pair<std::string, unsigned int> > m_vChanLimit, m_vMaxList;
	unsigned int m_channelLen, m_kickLen, m_modes, m_nickLen, m_topicLen;
	char m_excepts, m_invex;
	std::pair<std::string, std::string> m_prefix;
	bool m_safeList;

	bool ParseCaseMapping(const std::string &strValue);
	bool ParseChanLimit(const std::string &strValue);
	bool ParseChanModes(const std::string &strValue);
	bool ParseMaxList(const std::string &strValue);
	bool ParsePrefix(const std::string &strValue);
};

#endif // !IRCTRAITS_H

