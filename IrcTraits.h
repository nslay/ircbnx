#ifndef IRCTRAITS_H
#define IRCTRAITS_H

#include <string>
#include <utility>
#include <vector>

// Support for RPL_SUPPORT
class IrcTraits {
public:
	enum ChanModesType { A = 0, B, C, D };

	IrcTraits() {
		Reset();
	}

	bool Parse(const std::string &strParam);

	void Reset();

	std::pair<std::string, unsigned int> GetChanLimit(char prefix) const;

	const std::vector<std::pair<std::string, unsigned int> > & GetChanLimit() const {
		return m_vChanLimit;
	}

	const std::string & GetChanModes(ChanModesType type) const {
		return m_strChanModes[type];
	}

	ChanModesType ClassifyChanMode(char mode) const;

	unsigned int GetChannelLen() const {
		return m_channelLen;
	}

	const std::string & GetChanTypes() const {
		return m_strChanTypes;
	}

	char GetExcepts() const {
		return m_excepts;
	}

	char GetInvex() const {
		return m_invex;
	}

	std::pair<std::string, unsigned int> GetMaxList(char mode) const;

	const std::vector<std::pair<std::string, unsigned int> > & GetMaxList() const {
		return m_vMaxList;
	}

	unsigned int GetModes() const {
		return m_modes;
	}

	const std::string & GetNetwork() const {
		return m_strNetwork;
	}

	unsigned int GetNickLen() const {
		return m_nickLen;
	}

	const std::vector<std::pair<char, char> > & GetPrefix() const {
		return m_vPrefix;
	}

	char GetPrefixByMode(char mode) const;
	char GetModeByPrefix(char prefix) const;

	bool GetSafeList() const {
		return m_safeList;
	}

	const std::string & GetStatusMsg() const {
		return m_strStatusMsg;
	}

private:
	std::string m_strCaseMapping, m_strChanTypes, m_strNetwork, m_strStatusMsg, m_strChanModes[4];
	std::vector<std::pair<std::string, unsigned int> > m_vChanLimit, m_vMaxList;
	unsigned int m_channelLen, m_kickLen, m_modes, m_nickLen, m_topicLen;
	char m_excepts, m_invex;
	std::vector<std::pair<char, char> > m_vPrefix;
	bool m_safeList;

	bool ParseChanLimit(const std::string &strValue);
	bool ParseChanModes(const std::string &strValue);
	bool ParseMaxList(const std::string &strValue);
	bool ParsePrefix(const std::string &strValue);
};

#endif // !IRCTRAITS_H

