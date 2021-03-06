/*-
 * Copyright (c) 2013 Nathan Lay (nslay@users.sourceforge.net)
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

#include "BnxSeenList.h"
#include "BnxListIo.h"

bool BnxSeenList::Load() {
	Reset();
	std::vector<SeenInfo> vSeenInfo;
	if (!BnxLoadList(GetSeenListFile().c_str(), vSeenInfo))
		return false;

	for (size_t i = 0; i < vSeenInfo.size(); ++i) {
		const std::string &strNick = vSeenInfo[i].GetUser().GetNickname();
		m_mSeenMap[strNick] = vSeenInfo[i];
	}

	return true;
}

void BnxSeenList::Save() const {
	std::vector<SeenInfo> vSeenInfo;

	for (Iterator itr = Begin(); itr != End(); ++itr)
		vSeenInfo.push_back(itr->second);

	BnxSaveList(GetSeenListFile().c_str(), vSeenInfo);
}

void BnxSeenList::ExpireEntries() {
	MapType::iterator itr = m_mSeenMap.begin();

	while (itr != m_mSeenMap.end()) {
		if (itr->second.IsExpired()) {
			m_mSeenMap.erase(itr); // erase() does not return an iterator in standard C++
			itr = m_mSeenMap.begin();
		}
		else {
			++itr;
		}
	}
}

