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

#include <algorithm>
#include "BnxFloodDetector.h"

IrcCounter & BnxFloodDetector::GetCounter(const IrcUser &clUser) {
	std::vector<std::pair<IrcUser, IrcCounter> >::iterator itr;

	itr = std::find_if(m_vFloodCounters.begin(), 
				m_vFloodCounters.end(), 
				HostnameEquals(clUser.GetHostname()));

	if (itr != m_vFloodCounters.end())
		return itr->second;

	m_vFloodCounters.push_back(std::make_pair(clUser,IrcCounter(GetTimeStep())));

	return m_vFloodCounters.back().second;
}

void BnxFloodDetector::Detect(std::vector<IrcUser> &vFlooders) {
	vFlooders.clear();

	std::vector<std::pair<IrcUser, IrcCounter> >::iterator itr;

	itr = m_vFloodCounters.begin();

	while (itr != m_vFloodCounters.end()) {
		const IrcUser &clUser = itr->first;
		IrcCounter &clCounter = itr->second;

		float fRate = clCounter.SampleRate();

		if (fRate <= 0) {
			itr = m_vFloodCounters.erase(itr);
			// XXX: clUser and clCounter are no longer valid references
		}
		else if (fRate > GetThreshold()) {
			vFlooders.push_back(clUser);
			itr = m_vFloodCounters.erase(itr);
			// XXX: clUser and clCounter are no longer valid references
		}
		else {
			++itr;
		}
	}
}

