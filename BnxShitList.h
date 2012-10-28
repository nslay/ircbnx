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

#ifndef BNXSHITLIST_H
#define BNXSHITLIST_H

#include <algorithm>
#include <vector>
#include <string>
#include "IrcUser.h"

class BnxShitList {
public:

	void SetShitListFile(const std::string &strShitListFile) {
		m_strShitListFile = strShitListFile;
	}

	const std::string & GetShitListFile() const {
		return m_strShitListFile;
	}

	bool Load();
	void Save() const;

	bool AddMask(const IrcUser &clMask);
	bool DeleteMask(const IrcUser &clMask);

	const std::vector<IrcUser> & GetMasks() const {
		return m_vHostmasks;
	}

	bool IsListed(const IrcUser &clUser) const {
		return std::find_if(m_vHostmasks.begin(), m_vHostmasks.end(), MaskMatches(clUser)) != m_vHostmasks.end();
	}

	void Reset() {
		m_vHostmasks.clear();
	}

private:
	struct MaskMatches {
		MaskMatches(const IrcUser &clUser_)
		: clUser(clUser_) { }

		bool operator()(const IrcUser &clMask) const {
			return clMask.Matches(clUser);
		}

		const IrcUser &clUser;
	};

	std::string m_strShitListFile;
	std::vector<IrcUser> m_vHostmasks;
};

#endif // !BNXSHITLIST_H

