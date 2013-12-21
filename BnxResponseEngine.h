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

#ifndef BNXRESPONSEENGINE_H
#define BNXRESPONSEENGINE_H

#include <cstdlib>
#include <algorithm>
#include <iostream>
#include "BnxResponseRule.h"

class BnxResponseEngine {
public:
	BnxResponseEngine() {
		AddDefaultStatementResponse("Please go on.");
		AddDefaultStatementResponse("You must be joking!");
		AddDefaultStatementResponse("Are you talkin' to me?");
		AddDefaultStatementResponse("Get outta town!");

		AddDefaultQuestionResponse("Why do you ask?");
		AddDefaultQuestionResponse("How should I know?");
	}

	bool LoadFromStream(std::istream &is);
	void SaveToStream(std::ostream &os) const;

	void AddDefaultStatementResponse(const std::string &strMessage) {
		m_vDefaultStatementResponses.push_back(strMessage);
	}

	void AddDefaultQuestionResponse(const std::string &strMessage) {
		m_vDefaultQuestionResponses.push_back(strMessage);
	}

	const std::string & ComputeResponse(const std::string &strMessage) const {
		std::vector<BnxResponseRule>::const_iterator itr;

		itr = std::find(m_vRules.begin(), m_vRules.end(), strMessage);

		if (itr == m_vRules.end()) {
			const std::vector<std::string> &vDefaultResponses = (*strMessage.rbegin() == '?') ? 
										m_vDefaultQuestionResponses : 
										m_vDefaultStatementResponses;

			size_t responseIndex = (size_t)(vDefaultResponses.size() * (rand()/(float)RAND_MAX));
			responseIndex = std::min(responseIndex, vDefaultResponses.size()-1);

			return vDefaultResponses[responseIndex];

		}

		return itr->ComputeResponse();
	}

	void Reset() {
		m_vRules.clear();
	}

private:
	std::vector<BnxResponseRule> m_vRules;
	std::vector<std::string> m_vDefaultStatementResponses, m_vDefaultQuestionResponses;
};

#endif

