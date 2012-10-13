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
		AddDefaultResponse("Please go on.");
		AddDefaultResponse("You must be joking!");
		AddDefaultResponse("Are you talkin' to me?");
		AddDefaultResponse("Get outta town!");
	}

	bool LoadFromStream(std::istream &is);
	void SaveToStream(std::ostream &os) const;

	void AddDefaultResponse(const std::string &strMessage) {
		m_vDefaultResponses.push_back(strMessage);
	}

	const std::string & ComputeResponse(const std::string &strMessage) const {
		std::vector<BnxResponseRule>::const_iterator itr;

		itr = std::find(m_vRules.begin(), m_vRules.end(), strMessage);

		if (itr == m_vRules.end())
			return m_vDefaultResponses[rand() % m_vDefaultResponses.size()];

		return itr->ComputeResponse();
	}

	void Reset() {
		m_vRules.clear();
	}

private:
	std::vector<BnxResponseRule> m_vRules;
	std::vector<std::string> m_vDefaultResponses;
};

#endif

