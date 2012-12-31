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

#ifndef BNXRESPONSERULE_H
#define BNXRESPONSERULE_H

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>

#ifdef USE_PCRE
#include <pcreposix.h>
#else // USE_PCRE
#include <regex.h>
#endif // !USE_PCRE

class BnxResponseRule {
public:
	BnxResponseRule() { }

	~BnxResponseRule() {
		Reset();
	}

	BnxResponseRule(const BnxResponseRule &clRule) {
		*this = clRule;
	}

	bool AddRule(const std::string &strRegex) {
		regex_t rule;

		if (regcomp(&rule, strRegex.c_str(), REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0)
			return false;

		m_vRegexRules.push_back(std::make_pair(strRegex, rule));

		return true;
	}

	void AddResponse(const std::string &strResponse) {
		m_vResponses.push_back(strResponse);
	}

	const std::string & ComputeResponse() const {
		return m_vResponses[rand() % m_vResponses.size()];
	}

	const std::vector<std::string> & GetResponses() const {
		return m_vResponses;
	}

	void GetRules(std::vector<std::string> &vRegexRules) const {
		vRegexRules.resize(m_vRegexRules.size());

		for (size_t i = 0; i < m_vRegexRules.size(); ++i)
			vRegexRules[i] = m_vRegexRules[i].first;
	}

	void Reset() {
		for (size_t i = 0; i < m_vRegexRules.size(); ++i)
			regfree(&m_vRegexRules[i].second);

		m_vRegexRules.clear();
		m_vResponses.clear();
	}

	bool operator==(const std::string &strMessage) const {
		for (size_t i = 0; i < m_vRegexRules.size(); ++i) {
			//std::cout << "Matching: '" << strMessage << "' against '" << m_vRegexRules[i].first << "'" << std::endl;
			if (regexec(&m_vRegexRules[i].second, strMessage.c_str(), 0, NULL, 0) == 0) 
				return true;
		}

		return false;
	}

	bool operator!=(const std::string &strMessage) const {
		return !(*this == strMessage);
	}

	BnxResponseRule & operator=(const BnxResponseRule &clRule) {
		if (this == &clRule)
			return *this;

		Reset();

		// POSIX gives us no way to copy regex_t
		// So compile new ones ...

		m_vResponses = clRule.m_vResponses;
		for (size_t i = 0; i < clRule.m_vRegexRules.size(); ++i)
			AddRule(clRule.m_vRegexRules[i].first);

		return *this;
	}

private:
	std::vector<std::pair<std::string, regex_t> > m_vRegexRules;
	std::vector<std::string> m_vResponses;
};

inline bool operator==(const std::string &strMessage, const BnxResponseRule &clRule) {
	return clRule == strMessage;
}

inline bool operator!=(const std::string &strMessage, const BnxResponseRule &clRule) {
	return clRule != strMessage;
}

#endif // !BNXRESPONSERULE_H

