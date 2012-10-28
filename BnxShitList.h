#ifndef BNXSHITLIST_H
#define BNXSHITLIST_H

#include <algorithm>
#include <vector>
#include <string>
#include "IrcUser.h"

class BnxShitList {
public:

	void SetShitListFile(const std::string &strShitListFile) {
		m_strShitListFile = strShitListFile;
	}

	const std::string & GetShitListFile() const {
		return m_strShitListFile;
	}

	bool Load();
	void Save() const;

	bool AddMask(const IrcUser &clMask);
	bool DeleteMask(const IrcUser &clMask);

	const std::vector<IrcUser> & GetMasks() const {
		return m_vHostmasks;
	}

	bool IsListed(const IrcUser &clUser) const {
		return std::find_if(m_vHostmasks.begin(), m_vHostmasks.end(), MaskMatches(clUser)) != m_vHostmasks.end();
	}

	void Reset() {
		m_vHostmasks.clear();
	}

private:
	struct MaskMatches {
		MaskMatches(const IrcUser &clUser_)
		: clUser(clUser_) { }

		bool operator()(const IrcUser &clMask) const {
			return clMask.Matches(clUser);
		}

		const IrcUser &clUser;
	};

	std::string m_strShitListFile;
	std::vector<IrcUser> m_vHostmasks;
};

#endif // !BNXSHITLIST_H

