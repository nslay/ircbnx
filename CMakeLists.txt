cmake_minimum_required(VERSION 2.6)
PROJECT(ircbnx CXX C)

SET(LIBEVENT2_INCLUDE "/usr/local/include" CACHE PATH "libevent2 include directory")
SET(LIBEVENT2_LIB "/usr/local/lib/event2" CACHE PATH "libevent2 library directory")

IF (WIN32)
	SET(EXECUTABLE_TYPE "WIN32")
	SET(LINK_LIBS event ws2_32)
	SET(PLATFORM_SRC BnxWin32Driver.h BnxWin32Driver.cpp Resource.h ircbnx.rc)
ELSE (WIN32)
	SET(LINK_LIBS event)
ENDIF (WIN32)

IF (UNIX)
	SET(PLATFORM_SRC BnxUnixDriver.h BnxUnixDriver.cpp)
ENDIF (UNIX)

INCLUDE_DIRECTORIES(${LIBEVENT2_INCLUDE})
LINK_DIRECTORIES(${LIBEVENT2_LIB})

ADD_EXECUTABLE(ircbnx ${EXECUTABLE_TYPE} Main.cpp Irc.h 
	IrcString.h IrcString.cpp
	IrcUser.h IrcUser.cpp
	IrcTraits.h IrcTraits.cpp
	IrcEvent.h IrcEvent.cpp
	IrcClient.h IrcClient.cpp 
	IrcCounter.h
	Ctcp.h Ctcp.cpp
	IniFile.h IniFile.cpp
	BnxListIo.h
	BnxDriver.h BnxDriver.cpp
	BnxBot.h BnxBot.cpp 
	BnxResponseRule.h 
	BnxResponseEngine.h BnxResponseEngine.cpp
	BnxAccessSystem.h BnxAccessSystem.cpp
	BnxChannel.h BnxChannel.cpp
	BnxShitList.h BnxShitList.cpp
	BnxFloodDetector.h BnxFloodDetector.cpp
	BnxSeenList.h BnxSeenList.cpp
	BnxStreams.h BnxStreams.cpp
	getopt.h getopt.c
	${PLATFORM_SRC}
	)

TARGET_LINK_LIBRARIES(ircbnx ${LINK_LIBS}) 
