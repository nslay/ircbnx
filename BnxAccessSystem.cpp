#include <sstream>
#include "BnxAccessSystem.h"

bool BnxAccessSystem::LoadFromStream(std::istream &is) {
	if (!is) {
		std::cerr << "Bad stream." << std::endl;
		return false;
	}

	Reset();

	std::string strLine;
	while (std::getline(is, strLine)) {
		if (strLine.empty() || strLine[0] == ';')
			continue;

		std::stringstream ss;

		ss.str(strLine);

		std::string strHostmask, strPassword;
		int iAccessLevel;

		if (!(ss >> strHostmask >> iAccessLevel >> strPassword)) {
			std::cerr << "Could not read host mask, access level or password." << std::endl;
			Reset();
			return false;
		}

		AddUser(IrcUser(strHostmask), strPassword, iAccessLevel);
	}

	return true;
}

void BnxAccessSystem::SaveToStream(std::ostream &os) {
	for (size_t i = 0; i < m_vUserEntries.size(); ++i) {
		UserEntry &clEntry = m_vUserEntries[i];
		os << clEntry.GetHostmask().GetHostmask() << " " << clEntry.GetAccessLevel() << " " << clEntry.GetPassword() << std::endl;
	}
}

void BnxAccessSystem::AddUser(const IrcUser &clHostmask, const std::string &strPassword, int iAccessLevel) {
	std::vector<UserEntry>::iterator itr;

	itr = std::find(m_vUserEntries.begin(), m_vUserEntries.end(), clHostmask);

	if (itr == m_vUserEntries.end())
		m_vUserEntries.push_back(UserEntry(clHostmask, strPassword, iAccessLevel));
	else
		*itr = UserEntry(clHostmask, strPassword, iAccessLevel);
}

bool BnxAccessSystem::DeleteUser(const IrcUser &clHostmask) {
	std::vector<UserEntry>::iterator itr;

	itr = std::find(m_vUserEntries.begin(), m_vUserEntries.end(), clHostmask);

	if (itr == m_vUserEntries.end())
		return false;

	m_vUserEntries.erase(itr);

	return true;
}

bool BnxAccessSystem::Login(const IrcUser &clUser, const std::string &strPassword) {
	std::vector<UserEntry>::const_iterator itr;

	itr = std::find_if(m_vUserEntries.begin(), m_vUserEntries.end(), EntryMatches(clUser));

	if (itr == m_vUserEntries.end() || !itr->CheckPassword(strPassword))
		return false;

	if (std::find(m_vUserSessions.begin(), m_vUserSessions.end(), clUser) == m_vUserSessions.end())
		m_vUserSessions.push_back(UserSession(clUser,itr->GetAccessLevel()));

	return true;
}

void BnxAccessSystem::Logout(const IrcUser &clUser) {
	std::vector<UserSession>::iterator itr;

	itr = std::find(m_vUserSessions.begin(), m_vUserSessions.end(), clUser);

	if (itr != m_vUserSessions.end())
		m_vUserSessions.erase(itr);
}

BnxAccessSystem::UserSession * BnxAccessSystem::GetSession(const IrcUser &clUser) {
	std::vector<UserSession>::iterator itr;

	itr = std::find(m_vUserSessions.begin(), m_vUserSessions.end(), clUser);

	if (itr == m_vUserSessions.end())
		return NULL;

	if (itr->LastAccessTime() >= TIMEOUT) {
		m_vUserSessions.erase(itr);
		return NULL;
	}

	itr->Update();

	return &(*itr);
}

void BnxAccessSystem::TimeoutSessions() {
	std::vector<UserSession>::iterator itr;
		
	itr = m_vUserSessions.begin();

	while (itr != m_vUserSessions.end()) {
		if (itr->LastAccessTime() >= TIMEOUT)
			itr = m_vUserSessions.erase(itr);
		else
			++itr;
	}
}

