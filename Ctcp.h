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

#ifndef CTCP_H
#define CTCP_H

// See: http://www.irchelp.org/irchelp/rfc/ctcpspec.html

#include <cstring>

struct CtcpMessage {
	const void *tag, *data;
	size_t tagSize, dataSize;
};

inline CtcpMessage MakeCtcpMessage(const char *pData) {
	CtcpMessage message;

	message.tag = NULL;
	message.tagSize = 0;

	message.data = pData;
	message.dataSize = strlen(pData);

	return message;
}

inline CtcpMessage MakeCtcpMessage(const char *pTag, const char *pData) {
	CtcpMessage message;

	message.tag = pTag;
	message.tagSize = strlen(pTag);

	message.data = pData;
	message.dataSize = strlen(pData);

	return message;
}

class CtcpEncoder {
public:
	enum { MESSAGE_SIZE = 510 };

	CtcpEncoder() {
		Reset();
	}

	bool Encode(const CtcpMessage &message);

	const char * GetRaw() const {
		return m_aBuffer;
	}

	size_t GetSize() const {
		return m_bufferSize;
	}

	void Reset() {
		m_aBuffer[0] = '\0';
		m_bufferSize = 0;
	}

private:
	char m_aBuffer[MESSAGE_SIZE+1];
	size_t m_bufferSize;

	static size_t ComputeEncodedSize(const void *pData, size_t dataSize);
	static size_t Encode(const void *pData, size_t dataSize, char *pDest);
};

class CtcpDecoder {
public:
	enum { MESSAGE_SIZE = 510 };

	CtcpDecoder() {
		Reset();
	}

	explicit CtcpDecoder(const char *pRaw) {
		Reset();
		SetRaw(pRaw);
	}

	void Reset() {
		m_aBuffer[0] = '\0';
		m_offset = 0;
		m_iTag = 0;
	}

	bool Decode(CtcpMessage &message);

	bool SetRaw(const char *pRaw) {
		// Too big
		if (strlen(pRaw) > MESSAGE_SIZE)
			return false;

		Reset();

		strcpy(m_aBuffer, pRaw);
		return true;
	}

private:
	char m_aBuffer[MESSAGE_SIZE+1];
	size_t m_offset;
	int m_iTag;

	static size_t Decode(char *pBuffer);
};

#endif // !CTCP_H

