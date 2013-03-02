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

#ifndef BNXDRIVER_H
#define BNXDRIVER_H

#include <cstddef>
#include <string>
#include <vector>
#include "IniFile.h"
#include "BnxBot.h"

class BnxDriver {
public:
	typedef std::vector<BnxBot *>::const_iterator BotIterator;

	static BnxDriver & GetInstance() {
		static BnxDriver clDriver;
		return clDriver;
	}

	~BnxDriver();

	void SetConfigFile(const std::string &strConfigFile) {
		m_strConfigFile = strConfigFile;
	}

	void Usage();
	bool ParseArgs(int argc, char *argv[]);
	bool Load();
	bool Run();
	void Shutdown();
	void Reset();

	BotIterator BotBegin() const {
		return m_vBots.begin();
	}

	BotIterator BotEnd() const {
		return m_vBots.end();
	}

	BnxBot * GetBot(const std::string &strProfile) const;

private:
	BnxDriver()
	: m_strConfigFile("bot.ini") { }

	// Disabled
	BnxDriver(const BnxDriver &);

	std::string m_strConfigFile;
	std::vector<BnxBot *> m_vBots;

	void LoadBot(const IniFile::Section &clSection);

	// Disabled
	BnxDriver & operator=(const BnxDriver &);
};

#endif // !BNXDRIVER_H

