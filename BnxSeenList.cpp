#include "BnxSeenList.h"
#include "BnxListIo.h"

bool BnxSeenList::Load() {
	Reset();
	std::vector<SeenInfo> vSeenInfo;
	if (!BnxLoadList(GetSeenListFile().c_str(), vSeenInfo))
		return false;

	for (size_t i = 0; i < vSeenInfo.size(); ++i) {
		const std::string &strNick = vSeenInfo[i].GetUser().GetNickname();
		m_mSeenMap[strNick] = vSeenInfo[i];
	}

	return true;
}

void BnxSeenList::Save() const {
	std::vector<SeenInfo> vSeenInfo;

	for (Iterator itr = Begin(); itr != End(); ++itr)
		vSeenInfo.push_back(itr->second);

	BnxSaveList(GetSeenListFile().c_str(), vSeenInfo);
}

void BnxSeenList::ExpireEntries() {
	MapType::iterator itr = m_mSeenMap.begin();

	while (itr != m_mSeenMap.end()) {
		if (itr->second.IsExpired()) {
			m_mSeenMap.erase(itr); // erase() does not return an iterator in standard C++
			itr = m_mSeenMap.begin();
		}
		else {
			++itr;
		}
	}
}

