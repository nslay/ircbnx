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

#ifndef BNXLISTIO_H
#define BNXLISTIO_H

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

template<typename ElementType>
bool BnxLoadList(const char *pFileName, std::vector<ElementType> &vElements);

template<typename ElementType>
bool BnxSaveList(const char *pFileName, const std::vector<ElementType> &vElements);

template<typename ElementType>
bool BnxLoadList(const char *pFileName, std::vector<ElementType> &vElements) {
	vElements.clear();

	std::ifstream listStream(pFileName);

	if (!listStream)
		return false;

	std::string strLine;
	std::stringstream lineStream;

	while (std::getline(listStream, strLine)) {
		if (strLine.empty() || strLine[0] == ';')
			continue;

		lineStream.clear();
		lineStream.str(strLine);

		ElementType clElement;
		if (!(lineStream >> clElement)) {
			vElements.clear();
			return false;
		}

		if (std::find(vElements.begin(), vElements.end(), clElement) == vElements.end())
			vElements.push_back(clElement);
	}

	return true;
}

template<typename ElementType>
bool BnxSaveList(const char *pFileName, const std::vector<ElementType> &vElements) {
	std::ifstream inListStream(pFileName);

	if (inListStream) {
		std::stringstream tmpListStream;

		std::vector<bool> vIsWritten(vElements.size(), false);

		std::string strLine;
		std::stringstream lineStream;

		while (std::getline(inListStream, strLine)) {
			if (strLine.empty() || strLine[0] == ';') {
				tmpListStream << strLine << std::endl;
				continue;
			}

			lineStream.clear();
			lineStream.str(strLine);

			ElementType clElement;
			if (!(lineStream >> clElement)) 
				continue;

			typename std::vector<ElementType>::const_iterator itr;
			itr = std::find(vElements.begin(), vElements.end(), clElement);

			if (itr == vElements.end())
				continue;

			size_t i = itr - vElements.begin();

			if (!vIsWritten[i]) {
				tmpListStream << strLine << std::endl;
				vIsWritten[i] = true;
			}
		}

		inListStream.close();

		std::ofstream outListStream(pFileName);

		if (!outListStream)
			return false;

		outListStream << tmpListStream.str();

		for (size_t i = 0; i < vElements.size(); ++i) {
			if (!vIsWritten[i])
				outListStream << vElements[i] << std::endl;
		}
	}
	else {
		std::ofstream outListStream(pFileName);

		if (!outListStream)
			return false;

		for (size_t i = 0; i < vElements.size(); ++i)
			outListStream << vElements[i] << std::endl;
	}

	return true;
}

#endif // !BNXLISTIO_H

