cmake_minimum_required(VERSION 2.8)
PROJECT(ircbnx CXX C)

OPTION(USE_PCRE "Use PCRE instead of POSIX regex" TRUE)
SET(LIBEVENT2_INCLUDE "/usr/local/include" CACHE PATH "libevent2 include directory")
SET(LIBEVENT2_LIB "/usr/local/lib/event2" CACHE PATH "libevent2 library directory")
SET(PCRE_INCLUDE "/usr/local/include" CACHE PATH "PCRE include directory")
SET(PCRE_LIB "/usr/local/lib" CACHE PATH "PCRE library directory")

IF (WIN32)
	SET(LINK_LIBS event ws2_32)
	ADD_DEFINITIONS(-DPCRE_STATIC=1)
ELSE (WIN32)
	SET(LINK_LIBS event)
ENDIF (WIN32)

INCLUDE_DIRECTORIES(${LIBEVENT2_INCLUDE})
LINK_DIRECTORIES(${LIBEVENT2_LIB})

IF (USE_PCRE)
	ADD_DEFINITIONS(-DUSE_PCRE=1)
	INCLUDE_DIRECTORIES(${PCRE_INCLUDE})
	LINK_DIRECTORIES(${PCRE_LIB})
	SET(LINK_LIBS ${LINK_LIBS} pcreposix pcre)
ENDIF (USE_PCRE)

ADD_EXECUTABLE(ircbnx Main.cpp Irc.h 
	IrcString.h IrcString.cpp
	IrcUser.h IrcUser.cpp
	IrcTraits.h IrcTraits.cpp
	IrcClient.h IrcClient.cpp 
	IrcCounter.h
	Ctcp.h Ctcp.cpp
	IniFile.h IniFile.cpp
	BnxDriver.h BnxDriver.cpp
	BnxBot.h BnxBot.cpp 
	BnxResponseRule.h 
	BnxResponseEngine.h BnxResponseEngine.cpp
	BnxAccessSystem.h BnxAccessSystem.cpp
	BnxChannel.h BnxChannel.cpp
	BnxShitList.h BnxShitList.cpp
	BnxFloodDetector.h BnxFloodDetector.cpp
	getopt.h getopt.c)

TARGET_LINK_LIBRARIES(ircbnx ${LINK_LIBS}) 

