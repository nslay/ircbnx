cmake_minimum_required(VERSION 2.8)
PROJECT(ircbnx CXX C)

OPTION(USE_PCRE "Use PCRE instead of POSIX regex" TRUE)
SET(LIBEVENT2_INCLUDE "/usr/local/include" CACHE PATH "libevent2 include directory")
SET(LIBEVENT2_LIB "/usr/local/lib/event2" CACHE PATH "libevent2 library directory")
SET(PCRE_INCLUDE "/usr/local/include" CACHE PATH "PCRE include directory")
SET(PCRE_LIB "/usr/local/lib" CACHE PATH "PCRE library directory")

SET(LINK_LIBS "event" CACHE INTERNAL "")

INCLUDE_DIRECTORIES(${LIBEVENT2_INCLUDE})
LINK_DIRECTORIES(${LIBEVENT2_LIB})

IF (USE_PCRE)
	ADD_DEFINITIONS(-DUSE_PCRE)
	INCLUDE_DIRECTORIES(${PCRE_INCLUDE})
	LINK_DIRECTORIES(${PCRE_LIB})
	SET(LINK_LIBS ${LINK_LIBS} pcreposix pcre)
ENDIF (USE_PCRE)

ADD_EXECUTABLE(ircbnx Main.cpp Irc.h 
	IrcString.h IrcString.cpp
	Ctcp.h Ctcp.cpp
	IrcUser.h IrcUser.cpp
	IrcTraits.h IrcTraits.cpp
	IrcClient.h IrcClient.cpp 
	BnxBot.h BnxBot.cpp 
	BnxResponseRule.h 
	BnxResponseEngine.h BnxResponseEngine.cpp
	BnxAccessSystem.h BnxAccessSystem.cpp)

TARGET_LINK_LIBRARIES(ircbnx ${LINK_LIBS}) 

