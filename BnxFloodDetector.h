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

#ifndef BNXFLOODDETECTOR_H
#define BNXFLOODDETECTOR_H

#include <utility>
#include <vector>
#include "IrcString.h"
#include "IrcUser.h"
#include "IrcCounter.h"

class BnxFloodDetector {
public:
	BnxFloodDetector(float fThreshold = 3.0f, float fTimeStep = 1.0f) {
		SetThreshold(fThreshold);
		SetTimeStep(fTimeStep);
		Reset();
	}

	float GetTimeStep() const {
		return m_fTimeStep;
	}

	float GetThreshold() const {
		return m_fThreshold;
	}

	void SetTimeStep(float fTimeStep) {
		m_fTimeStep = fTimeStep;
	}

	void SetThreshold(float fThreshold) {
		m_fThreshold = fThreshold;
	}

	IrcCounter & GetCounter(const IrcUser &clUser);

	void Hit(const IrcUser &clUser) {
		GetCounter(clUser).Hit();
	}

	void Detect(std::vector<IrcUser> &vFlooders);

	void Reset() {
		m_vFloodCounters.clear();
	}

private:
	float m_fTimeStep, m_fThreshold;
	std::vector<std::pair<IrcUser, IrcCounter> > m_vFloodCounters;
};

#endif // !BNXFLOODDETECTOR_H

