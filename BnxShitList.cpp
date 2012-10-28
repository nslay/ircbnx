#include <fstream>
#include <sstream>
#include "BnxShitList.h"

bool BnxShitList::Load() {
	Reset();

	std::ifstream listStream(GetShitListFile().c_str());

	if (!listStream)
		return false;

	std::string strLine;
	while (std::getline(listStream, strLine)) {
		if (strLine.empty() || strLine[0] == ';')
			continue;

		std::stringstream lineStream;

		lineStream.str(strLine);

		std::string strHostmask;

		if (!(lineStream >> strHostmask)) {
			Reset();
			return false;
		}

		AddMask(IrcUser(strLine));
	}

	return true;
}

void BnxShitList::Save() const {
	std::ifstream inListStream(GetShitListFile().c_str());

	if (inListStream) {
		std::stringstream tmpListStream;
		std::vector<IrcUser> vMasksWritten;

		std::string strLine;

		while (std::getline(inListStream, strLine)) {
			if (strLine.empty() || strLine[0] == ';') {
				tmpListStream << strLine << std::endl;
				continue;
			}

			std::stringstream lineStream;
			lineStream.str(strLine);

			std::string strHostmask;
			if (!(lineStream >> strHostmask))
				continue;

			IrcUser clMask(strHostmask);
			if (std::find(m_vHostmasks.begin(), m_vHostmasks.end(), clMask) != m_vHostmasks.end()) {
				tmpListStream << clMask.GetHostmask() << std::endl;
				vMasksWritten.push_back(clMask);
			}
		}

		if (!strLine.empty())
			tmpListStream << std::endl;

		inListStream.close();

		std::ofstream outListStream(GetShitListFile().c_str());

		outListStream << tmpListStream.str();

		for (size_t i = 0; i < m_vHostmasks.size(); ++i) {
			if (std::find(vMasksWritten.begin(), vMasksWritten.end(), 
					m_vHostmasks[i]) == vMasksWritten.end()) {
				outListStream << m_vHostmasks[i].GetHostmask() << std::endl;
			}
		}
	}
	else {
		std::ofstream outListStream(GetShitListFile().c_str());

		for (size_t i = 0; i < m_vHostmasks.size(); ++i)
			outListStream << m_vHostmasks[i].GetHostmask() << std::endl;
	}

}

bool BnxShitList::AddMask(const IrcUser &clMask) {
	if (std::find(m_vHostmasks.begin(), m_vHostmasks.end(), clMask) == m_vHostmasks.end()) {
		m_vHostmasks.push_back(clMask);
		return true;
	}

	return false;
}

bool BnxShitList::DeleteMask(const IrcUser &clMask) {
	std::vector<IrcUser>::iterator itr;

	itr = std::find(m_vHostmasks.begin(), m_vHostmasks.end(), clMask);

	if (itr != m_vHostmasks.end()) {
		m_vHostmasks.erase(itr);
		return true;
	}

	return false;
}

