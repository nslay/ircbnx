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

#include <cstring>
#include "Ctcp.h"

bool CtcpEncoder::Encode(const CtcpMessage &message) {
	const char *pTag = (const char *)message.tag,
		*pData = (const char *)message.data;
	size_t tagSize = message.tagSize, dataSize = message.dataSize,
		expectedSize = 0;

	if (pTag != NULL) {
		// Check for \040, not allowed in tags
		if (memchr(pTag, 0040, tagSize) != NULL)
			return false;

		// Followed by two \001
		expectedSize += ComputeEncodedSize(pTag, tagSize) + 2;

		// And a space
		if (pData != NULL && dataSize > 0)
			++expectedSize;
	}

	if (pData != NULL && dataSize > 0) 
		expectedSize += ComputeEncodedSize(pData, dataSize);

	// Too big
	if (expectedSize+m_bufferSize > MESSAGE_SIZE)
		return false;

	char *pBuffer = m_aBuffer + m_bufferSize;
	if (pTag != NULL) {
		*pBuffer++ = 0001;
		pBuffer += Encode(pTag, tagSize, pBuffer);

		if (pData != NULL && dataSize > 0)
			*pBuffer++ = 0040;
	}

	if (pData != NULL && dataSize > 0) 
		pBuffer += Encode(pData, dataSize, pBuffer);

	if (pTag != NULL)
		*pBuffer++ = 0001;

	m_bufferSize += expectedSize;

	m_aBuffer[m_bufferSize] = '\0';
	
	return true;
}

size_t CtcpEncoder::ComputeEncodedSize(const void *pData, size_t dataSize) {
	const char *pDataChar = (const char *)pData;
	size_t expectedSize = dataSize;

	for (size_t i = 0; i < dataSize; ++i) {
		switch (pDataChar[i]) {
		// Low level quoting
		case '\0':
		case '\n':
		case '\r':
		case 0020:
		// High level quoting
		case 0001:
		case 0134:
			++expectedSize;
			break;
		}
	}

	return expectedSize;
}

size_t CtcpEncoder::Encode(const void *pData, size_t dataSize, char *pDest) {
	const char *pDataChar = (const char *)pData;
	size_t newSize = dataSize;

	for (size_t i = 0; i < dataSize; ++i) {
		switch (pDataChar[i]) {
		// Low level quoting
		case '\0':
			*pDest++ = 0020;
			*pDest++ = '0';
			++newSize;
			break;
		case '\n':
			*pDest++ = 0020;
			*pDest++ = 'n';
			++newSize;
			break;
		case '\r':
			*pDest++ = 0020;
			*pDest++ = 'r';
			++newSize;
			break;
		case 0020:
			*pDest++ = 0020;
			*pDest++ = 0020;
			++newSize;
			break;
		// High level quoting
		case 0001:
			*pDest++ = 0134;
			*pDest++ = 'a';
			++newSize;
			break;
		case 0134:
			*pDest++ = 0134;
			*pDest++ = 0134;
			++newSize;
			break;
		default:
			*pDest++ = pDataChar[i];
		}
	}

	return newSize;
}

bool CtcpDecoder::Decode(CtcpMessage &message) {

	message.tag = message.data = NULL;
	message.tagSize = message.dataSize = 0;

	if (m_aBuffer[m_offset] == '\0')
		return false;

	char *pBegin = m_aBuffer + m_offset, *pEnd;

	for (pEnd = pBegin; *pEnd != 0001 && *pEnd != '\0'; ++pEnd);

	m_offset += pEnd - pBegin;

	if (*pEnd != '\0') {
		*pEnd = '\0';
		++m_offset;
	}

	if (m_iTag) {
		char *p;

		for (p = pBegin; *p != 0040 && *p != '\0'; ++p);

		if (*p != '\0')
			*p++ = '\0';

		message.tagSize = Decode(pBegin);
		message.tag = pBegin;

		pBegin = p;
	}

	message.dataSize = Decode(pBegin);
	message.data = pBegin;

	m_iTag ^= 1;

	return true;
}

size_t CtcpDecoder::Decode(char *pBuffer) {
	char *pCurrent, *pDest;

	pCurrent = pDest = pBuffer;

	while (*pCurrent != '\0') {
		switch (*pCurrent) {
		case 0020:
			++pCurrent;

			switch (*pCurrent) {
			case '0':
				++pCurrent;
				*pDest++ = '\0';
				break;
			case 'r':
				++pCurrent;
				*pDest++ = '\r';
				break;
			case 'n':
				++pCurrent;
				*pDest++ = '\n';
				break;
			case '\0':
				break;
			default:
				*pDest++ = *pCurrent++;
			}
			break;
		case 0134:
			++pCurrent;

			switch (*pCurrent) {
			case 'a':
				++pCurrent;
				*pDest++ = 0001;
				break;
			case '\0':
				break;
			default:
				*pDest++ = *pCurrent++;
			}
			break;
		default:
			*pDest++ = *pCurrent++;
		}
	}

	*pDest = '\0';

	return pDest - pBuffer;
}

