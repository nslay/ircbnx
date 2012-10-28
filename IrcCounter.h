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

#ifndef IRCCOUNTER_H
#define IRCCOUNTER_H

class IrcCounter {
public:
	IrcCounter(float fTimeStep = 1.0f) {
		SetTimeStep(fTimeStep);
		Reset();
	}

	void Hit() {
		++m_iCounts[1];
	}

	int GetCurrentCount() const {
		return m_iCounts[1];
	}

	float GetCurrentRate() const {
		return m_fRate;
	}

	float GetTimeStep() const {
		return m_fTimeStep;
	}

	void SetTimeStep(float fTimeStep) {
		m_fTimeStep = fTimeStep;
	}

	float SampleRate() {
		if (m_iCounts[0] < 0) {
			// Forward difference
			m_fRate = m_iCounts[1]/m_fTimeStep;
		}
		else {
			// Center difference
			m_fRate = 0.5f*(m_iCounts[0]+m_iCounts[1])/m_fTimeStep;
		}

		m_iCounts[0] = m_iCounts[1];
		m_iCounts[1] = 0;

		return m_fRate;
	}

	void Reset() {
		m_iCounts[0] = -1;
		m_iCounts[1] = 0;
		m_fRate = 0.0f;
	}

private:
	int m_iCounts[2];
	float m_fTimeStep, m_fRate;
};

#endif // !IRCCOUNTER_H

