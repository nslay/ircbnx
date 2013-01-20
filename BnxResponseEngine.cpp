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

#include "BnxResponseEngine.h"

bool BnxResponseEngine::LoadFromStream(std::istream &is) {
	if (!is) {
		std::cerr << "Bad stream" << std::endl;
		return false;
	}

	BnxResponseRule clRule;
	char mode = '\0';

	Reset();

	std::string line;
	while (std::getline(is, line)) {
		size_t p = line.find('\r');

		if (p != std::string::npos)
			line.resize(p);

		if (line.empty())
			continue;

		char type = line[0];
		std::string value;

		if (line.size() > 2)
			value = line.substr(2);

		switch (type) {
		case ';':
			break;
		case 'P':
			if (value.empty()) {
				std::cerr << "Error: Missing regex rule." << std::endl;
				Reset();
				return false;
			}

			if (line[1] != ' ') {
				std::cerr << "Error: Expected space after type." << std::endl;
				Reset();
				return false;
			}

			if (mode == 'R') {
				// Rule completed
				m_vRules.push_back(clRule);
				clRule.Reset();
			}

			mode = 'P';

			if (!clRule.AddRule(value)) {
				std::cerr << "Error: Could not compile regex '" << value << "'." << std::endl;
				Reset();
				return false;
			}

			break;
		case 'R':
			if (mode == '\0') {
				std::cerr << "Error: No rules specified for response." << std::endl;
				Reset();
				return false;
			}

			if (line[1] != ' ') {
				std::cerr << "Error: Expected space after type." << std::endl;
				Reset();
				return false;
			}

			if (value.empty()) {
				std::cerr << "Error: Missing response." << std::endl;
				Reset();
				return false;
			}

			mode = 'R';

			clRule.AddResponse(value);
			break;
		default:
			std::cerr << "Unknown rule type '" << type << "'" << std::endl;
			Reset();
			return false;
		}
	}

	if (mode == 'P') {
		std::cerr << "Error: No responses specified!" << std::endl;
		Reset();
		return false;
	}
	else if (mode == 'R') 
		m_vRules.push_back(clRule);

	return true;
}

void BnxResponseEngine::SaveToStream(std::ostream &os) const {
	std::vector<std::string> vRegexRules;

	for (size_t i = 0; i < m_vRules.size(); ++i) {
		const std::vector<std::string> &vResponses = m_vRules[i].GetResponses();
		m_vRules[i].GetRules(vRegexRules);

		for (size_t j = 0; j < vRegexRules.size(); ++j)
			os << "P " << vRegexRules[j] << std::endl;

		for (size_t j = 0; j < vResponses.size(); ++j)
			os << "R " << vResponses[j] << std::endl;
	}
}

