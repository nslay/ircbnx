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

#include <fstream>
#include <sstream>
#include "BnxShitList.h"

bool BnxShitList::Load() {
	Reset();

	std::ifstream listStream(GetShitListFile().c_str());

	if (!listStream)
		return false;

	std::string strLine;
	while (std::getline(listStream, strLine)) {
		if (strLine.empty() || strLine[0] == ';')
			continue;

		std::stringstream lineStream(strLine);

		IrcUser clMask;

		if (!(lineStream >> clMask)) {
			Reset();
			return false;
		}

		AddMask(clMask);
	}

	return true;
}

void BnxShitList::Save() const {
	std::ifstream inListStream(GetShitListFile().c_str());

	if (inListStream) {
		std::stringstream tmpListStream;
		std::vector<IrcUser> vMasksWritten;

		std::string strLine;

		while (std::getline(inListStream, strLine)) {
			if (strLine.empty() || strLine[0] == ';') {
				tmpListStream << strLine << std::endl;
				continue;
			}

			std::stringstream lineStream(strLine);

			IrcUser clMask;
			if (!(lineStream >> clMask))
				continue;

			if (std::find(m_vHostmasks.begin(), m_vHostmasks.end(), clMask) != m_vHostmasks.end()) {
				tmpListStream << clMask << std::endl;
				vMasksWritten.push_back(clMask);
			}
		}

		if (!strLine.empty())
			tmpListStream << std::endl;

		inListStream.close();

		std::ofstream outListStream(GetShitListFile().c_str());

		outListStream << tmpListStream.str();

		for (size_t i = 0; i < m_vHostmasks.size(); ++i) {
			if (std::find(vMasksWritten.begin(), vMasksWritten.end(), 
					m_vHostmasks[i]) == vMasksWritten.end()) {
				outListStream << m_vHostmasks[i] << std::endl;
			}
		}
	}
	else {
		std::ofstream outListStream(GetShitListFile().c_str());

		for (size_t i = 0; i < m_vHostmasks.size(); ++i)
			outListStream << m_vHostmasks[i] << std::endl;
	}

}

bool BnxShitList::AddMask(const IrcUser &clMask) {
	if (GetMask(clMask) == End())  {
		m_vHostmasks.push_back(clMask);
		return true;
	}

	return false;
}

bool BnxShitList::DeleteMask(const IrcUser &clMask) {
	Iterator itr = GetMask(clMask);

	if (itr != End()) {
		DeleteMask(itr);
		return true;
	}

	return false;
}

