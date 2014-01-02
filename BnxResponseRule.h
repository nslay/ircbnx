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
#include <regex>

class BnxResponseRule {
public:
	bool AddRule(const std::string &strRegex) {
		using namespace std::regex_constants;

		m_vRegexRules.push_back(std::make_pair(strRegex, std::regex(strRegex, ECMAScript | icase)));

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

	std::vector<std::string> GetRules() const {
		std::vector<std::string> vRegexRules;
		vRegexRules.reserve(m_vRegexRules.size());

		for (auto &rulePair : m_vRegexRules)
			vRegexRules.push_back(rulePair.first);

		return vRegexRules;
	}

	void Reset() {
		m_vRegexRules.clear();
		m_vResponses.clear();
	}

	bool operator==(const std::string &strMessage) const {
		for (auto &rulePair : m_vRegexRules) {
			if (std::regex_search(strMessage, rulePair.second))
				return true;
		}

		return false;
	}

	bool operator!=(const std::string &strMessage) const {
		return !(*this == strMessage);
	}

private:
	std::vector<std::pair<std::string, std::regex> > m_vRegexRules;
	std::vector<std::string> m_vResponses;
};

inline bool operator==(const std::string &strMessage, const BnxResponseRule &clRule) {
	return clRule == strMessage;
}

inline bool operator!=(const std::string &strMessage, const BnxResponseRule &clRule) {
	return clRule != strMessage;
}

#endif // !BNXRESPONSERULE_H

