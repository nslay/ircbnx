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

#include <limits>
#include <sstream>
#include "IrcTraits.h"

bool IrcTraits::Parse(const std::string &strParam) {
	std::stringstream paramStream(strParam);

	if (paramStream.peek() == '-')
		paramStream.get();

	std::string strVariable;

	if (!std::getline(paramStream,strVariable,'='))
		return false;

	if (strVariable == "CASEMAPPING") {
		std::string strValue;

		if (!(paramStream >> strValue))
			return false;

		return ParseCaseMapping(strValue);
	}
	else if (strVariable == "CHANLIMIT") {
		std::string strValue;

		if (!(paramStream >> strValue))
			return false;

		return ParseChanLimit(strValue);
	}
	else if (strVariable == "CHANMODES") {
		std::string strValue;

		if (!(paramStream >> strValue))
			return false;

		return ParseChanModes(strValue);
	}
	else if (strVariable == "CHANNELLEN") {
		if (!(paramStream >> m_channelLen))
			return false;
	}
	else if (strVariable == "CHANTYPES") {
		if (!(paramStream >> m_strChanTypes))
			return false;
	}
	else if (strVariable == "EXCEPTS") {
		if (!(paramStream >> m_excepts))
			m_excepts = 'e';
	}
	else if (strVariable == "IDCHAN") {
		// This is stupid
	}
	else if (strVariable == "INVEX") {
		if (!(paramStream >> m_invex))
			m_invex = 'I';
	}
	else if (strVariable == "KICKLEN") {
		if (!(paramStream >> m_kickLen))
			m_kickLen = 510;
	}
	else if (strVariable == "MAXLIST") {
		std::string strValue;

		if (!(paramStream >> strValue))
			return false;

		return ParseMaxList(strValue);
	}
	else if (strVariable == "MODES") {
		if (!(paramStream >> m_modes))
			m_modes = std::numeric_limits<unsigned int>::max();

	}
	else if (strVariable == "NETWORK") {
		if (!(paramStream >> m_strNetwork))
			return false;

	}
	else if (strVariable == "NICKLEN") {
		if (!(paramStream >> m_nickLen))
			return false;

	}
	else if (strVariable == "PREFIX") {
		std::string strValue;

		// No prefixes is possible
		paramStream >> strValue;

		return ParsePrefix(strValue);
	}
	else if (strVariable == "SAFELIST") {
		m_safeList = true;
	}
	else if (strVariable == "STATUSMSG") {
		if (!(paramStream >> m_strStatusMsg))
			return false;
	}
	else if (strVariable == "STD") {
		// This is stupid
	}
	else if (strVariable == "TARGMAX") {
		// This is stupid
	}
	else if (strVariable == "TOPICLEN") {
		if (!(paramStream >> m_topicLen))
			m_topicLen = 510;
	}

	return true;
}

void IrcTraits::Reset() {
	m_caseMapping = RFC1459;
	m_vChanLimit.clear();
	m_strChanModes[0].clear();
	m_strChanModes[1].clear();
	m_strChanModes[2].clear();
	m_strChanModes[3].clear();
	m_channelLen = 200;
	m_strChanTypes = "#&";
	m_excepts = '\0';
	m_invex = '\0';
	m_kickLen = 510;
	m_vMaxList.clear();
	m_modes = 3;
	m_strNetwork.clear();
	m_nickLen = 9;
	m_prefix = std::make_pair(std::string("ov"),std::string("@+"));
	m_safeList = false;
	m_strStatusMsg.clear();
	m_topicLen = 510;
}

IrcTraits::LimitIterator IrcTraits::GetChanLimit(char prefix) const {
	for (LimitIterator itr = ChanLimitBegin(); itr != ChanLimitEnd(); ++itr) {
		const std::string &strPrefixes = itr->first;

		if (strPrefixes.find(prefix) != std::string::npos)
			return itr;
	}

	return ChanLimitEnd();
}

IrcTraits::ChanModesType IrcTraits::ClassifyChanMode(char mode) const {
	for (int i = 0; i < 4; ++i) {
		if (m_strChanModes[i].find(mode) != std::string::npos)
			return (ChanModesType)i;
	}

	return TYPE_B;
}

IrcTraits::LimitIterator IrcTraits::GetMaxList(char mode) const {
	for (LimitIterator itr = MaxListBegin(); itr != MaxListEnd(); ++itr) {
		const std::string &strModes = itr->first;

		if (strModes.find(mode) != std::string::npos)
			return itr;
	}

	return MaxListEnd();
}

char IrcTraits::GetPrefixByMode(char mode) const {
	size_t p = m_prefix.first.find(mode);

	if (p != std::string::npos)
		return m_prefix.second[p];

	return '\0';
}

char IrcTraits::GetModeByPrefix(char prefix) const {
	size_t p = m_prefix.second.find(prefix);

	if (p != std::string::npos)
		return m_prefix.first[p];

	return '\0';
}

bool IrcTraits::ParseCaseMapping(const std::string &strValue) {
	m_caseMapping = RFC1459;

	if (strValue == "rfc1459")
		m_caseMapping = RFC1459;
	else if (strValue == "strict-rfc1459")
		m_caseMapping = STRICT_RFC1459;
	else if (strValue == "ascii")
		m_caseMapping = ASCII;
	else
		return false;

	return true;
}

bool IrcTraits::ParseChanLimit(const std::string &strValue) {
	m_vChanLimit.clear();

	if (strValue.empty())
		return false;

	std::stringstream limitStream(strValue), paramStream;

	std::string strParam, strPrefixes;
	while (std::getline(limitStream,strParam,',')) {
		paramStream.clear();
		paramStream.str(strParam);

		if (!std::getline(paramStream, strPrefixes, ':'))
			return false;

		unsigned int maxChannels;
		if (!(paramStream >> maxChannels))
			maxChannels = std::numeric_limits<unsigned int>::max();

		m_vChanLimit.push_back(std::make_pair(strPrefixes, maxChannels));
	}

	return true;
}

bool IrcTraits::ParseChanModes(const std::string &strValue) {
	m_strChanModes[0].clear();
	m_strChanModes[1].clear();
	m_strChanModes[2].clear();
	m_strChanModes[3].clear();

	if (strValue.empty())
		return false;

	std::stringstream paramStream(strValue);

	for (int i = 0; i < 4; ++i) {
		if (!std::getline(paramStream,m_strChanModes[i],','))
			return false;
	}

	return true;
}

bool IrcTraits::ParseMaxList(const std::string &strValue) {
	m_vMaxList.clear();

	if (strValue.empty())
		return false;

	std::stringstream limitStream(strValue), paramStream;

	std::string strParam, strModes;
	while (std::getline(limitStream,strParam,',')) {
		paramStream.clear();
		paramStream.str(strParam);

		if (!std::getline(paramStream, strModes, ':'))
			return false;

		unsigned int maxModes;
		if (!(paramStream >> maxModes))
			return false;

		m_vMaxList.push_back(std::make_pair(strModes, maxModes));
	}

	return true;
}

bool IrcTraits::ParsePrefix(const std::string &strValue) {
	m_prefix.first.clear();
	m_prefix.second.clear();

	// No prefixes apparently
	if (strValue.empty())
		return true;

	std::stringstream paramStream(strValue);

	if (paramStream.get() != '(')
		return false;

	std::string strModes, strPrefixes;

	if (!std::getline(paramStream,strModes,')') || !(paramStream >> strPrefixes))
		return false;

	if (strModes.size() != strPrefixes.size())
		return false;

	m_prefix = std::make_pair(strModes, strPrefixes);

	return true;
}

