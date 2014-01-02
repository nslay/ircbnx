/*-
 * Copyright (c) 2012-2013 Nathan Lay (nslay@users.sourceforge.net)
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
#include <sstream>
#include <iostream>
#include <memory>
#include "event2/event.h"
#include "IniFile.h"
#include "BnxBot.h"

class BnxDriver {
public:
	typedef std::vector<std::shared_ptr<BnxBot> >::const_iterator BotIterator;

	static BnxDriver & GetInstance();

	BnxDriver() {
		m_strConfigFile = "bot.ini";
		m_strLogFile = "bot.log";
		m_pEventBase.reset(event_base_new());
	}

	virtual ~BnxDriver() { }

	virtual void SetConfigFile(const std::string &strConfigFile) {
		m_strConfigFile = strConfigFile;
	}

	virtual void Usage();
	virtual bool ParseArgs(int argc, char *argv[]);
	virtual bool Load();
	virtual bool Run();
	virtual void Shutdown();
	virtual void Reset();

	BotIterator BotBegin() const {
		return m_vBots.begin();
	}

	BotIterator BotEnd() const {
		return m_vBots.end();
	}

	std::shared_ptr<BnxBot> GetBot(const std::string &strProfile) const;

protected:
	struct event_base * GetEventBase() const {
		return m_pEventBase.get();
	}

private:
	struct EventBaseDeleter {
		void operator()(struct event_base *pEventBase) const {
			if (pEventBase != nullptr)
				event_base_free(pEventBase);
		}
	};

	std::string m_strConfigFile, m_strLogFile;
	std::vector<std::shared_ptr<BnxBot> > m_vBots;
	std::unique_ptr<struct event_base, EventBaseDeleter> m_pEventBase;

	// Disabled
	BnxDriver(const BnxDriver &) = delete;

	// Disabled
	BnxDriver & operator=(const BnxDriver &) = delete;

	void LoadBot(const IniFile::Section &clSection);
};

#endif // !BNXDRIVER_H

