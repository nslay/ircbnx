cmake_minimum_required(VERSION 2.8)
PROJECT(ircbnx CXX C)

SET(LIBEVENT2_INCLUDE "/usr/local/include" CACHE PATH "libevent2 include directory")
SET(LIBEVENT2_LIB "/usr/local/lib/event2" CACHE PATH "libevent2 library directory")

INCLUDE_DIRECTORIES(${LIBEVENT2_INCLUDE})
LINK_DIRECTORIES(${LIBEVENT2_LIB})

ADD_EXECUTABLE(ircbnx Main.cpp Irc.h 
	IrcString.h IrcString.cpp
	Ctcp.h Ctcp.cpp
	IrcUser.h IrcUser.cpp
	IrcClient.h IrcClient.cpp 
	BnxBot.h BnxBot.cpp 
	BnxResponseRule.h 
	BnxResponseEngine.h BnxResponseEngine.cpp
	BnxAccessSystem.h BnxAccessSystem.cpp)

TARGET_LINK_LIBRARIES(ircbnx event)

