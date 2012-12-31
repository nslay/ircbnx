#######################################################################
# Introduction                                                        #
#######################################################################

#######################################################################
# Installing                                                          #
#######################################################################

If a precompiled version is available for your operating system,
either extract the archive where it best suits you, or copy the
executable, response.txt, and bot.ini to the desired location.

Once installed and configured, simply run "ircbnx" from the command 
line.

NOTE: It will NOT produce output, but it should be working properly.
      Be sure to check "bot.log" for additional information.

#######################################################################
# Configuration                                                       #
#######################################################################

For simplicity and flexibility, the configuration file format is INI.
Every IRCBNX config file must have a "global" section and a "profiles"
key specifying a comma delimited list of bot configurations to examine.

For example:

[global]
profiles=bot1,bot2

In this example, two bots, "bot1" and "bot2" are to be examined.

Each bot profile listed in "profiles" references a section with 
the following configurations

enabled - Whether the configuration is to be loaded or not (use 0 or 1).
server - The hostname or IP address of the IRC server.
port - The service or port number to use.
nickname - The nickname to use.
username - The username to use when registering the connection.
realname - The real name to use when registering the connection.
homechannels - A comma delimited list of channels to join.
nickserv - The nickserv service name (if any).
nickservpassword - The password to identify with the nickserv (if any).
shitlist - The shit list file to use.
accesslist - The access list file to use.

For example:

[bot1]
server=some.irc.network.com
port=6667
nickname=enslay
username=nslay
realname=Nathan Lay
homechannels=#ircbnx,#freebsd

This example corresponds to the configuration of bot profile "bot1" as 
mentioned above.

#######################################################################
# Access List Format                                                  #
#######################################################################

The access list format is a line oriented list with

<hostmask> <access level> <password>

on each line. For example:

; This is a comment
nslay!*@*.myisp.net 100 passwd
enslay!*@*.theirisp.net 90 giraffe

In this example, the first line allows anyone with nickname nslay
and subdomains of myisp.net to authenticate with password "passwd". The
second line allows anyone with nickname enslay and subdomains of
theirisp.net to authenticate with password "giraffe".

Access levels range from 0-100 where 100 is the bot owner. The
granularity of control this sceheme offers is probably inappropriate
but was copied from the original Battle.net BNX implementation. See
section "Commands" for commands and their required access levels.

Passwords are one word (i.e. no white space). Any additional words 
specified are ignored.

Comments are limited. A line MUST begin with ';' to be considered a
comment. Any other use is an error.

#######################################################################
# Shit List Format                                                    #
#######################################################################

The shit list format is a line oriented list with

<hostmask>

on each line. For example:

; This is a comment
*!*@*.badisp.net
*!*@*.anotherisp.net

When the bot has channel operator status, it will ban anyone with
subdomains of badisp.net and anotherisp.net automatically.

Comments are limited. A line MUST begin with ';' to be considered a
comment. Any other use is an error.

#######################################################################
# Response Rule Format                                                #
#######################################################################

Response rules are groupings of regular expressions and possible
responses. The format is as follows

; This is a comment
P <rule1>
P <rule2>
; Matching either of these rules produces one of these three responses
R <response1>
R <response2>
R <response3>

For example:
P \bwho +are +you\b
R Who I am is not important...
P \bwhat\b.*\bare\b.*\byou\b
P \bare\b.*\byou\b.*\bbot\b
R I'm a BNX Chatterbot v1.0

In this example, anyone who asks it, "who are you?" will be met with
the one possible response, "Who I am is not important..." If anyone
asks it either, "What are you?" or, "Are you a bot?" will be met with
the one possible response, "I'm a BNX Chatterbot v1.0"

Comments are limited. A line MUST begin with ';' to be considered a
comment. Any other use is an error.

#######################################################################
# Commands                                                            #
#######################################################################

#######################################################################
# Miscellaneous                                                       #
#######################################################################

IRCBNX exhibits some of the following additional behaviors:
- It will produce responses either when whispered or addressed in the
  channel (i.e. using its nickname in a message).
- It will say "Hi!" when a second user joins the channel.
- It will produce automated responses when there are only two users in
  the channel (including itself) regardless of being addressed.
- It will ignore users who tell it to shut up (or variants of shutup).
- It will ban users who address it with messages including "f**k" in
  the channel.
- It will refuse to voteban itself.
- It will ignore users who try to abuse its automated responses.
- It will periodically try to join channels it was kicked or banned
  from.

#######################################################################
# Building from Source                                                #
#######################################################################

To build IRCBNX from source, you will need CMake, a C++ compiler, and 
the following dependencies:
- PCRE (required for Windows, optional for Unix-like systems)
http://www.pcre.org/

- libevent2
http://libevent.org

First extract the IRCBNX source somewhere. Next create a separate
directory elsewhere. This will be the build directory. Run CMake and 
configure the source and build directories. More specifically

On Windows:
- Run cmake-gui (Look in Start Menu) and proceed from there.

On Unix-like systems:
- From a terminal, change directory to the build directory and then
run:

ccmake /path/to/source/directory

In both cases, "configure" and then specify whether you want to enable
PCRE (recommended), the location of the libevent headers and
libraries, and the location of the PCRE headers and libraries. Next
"generate" to generate the build system.

Visual Studio:
- Open the solution in the build directory you created and proceed.

Unix-like systems:
- Run make(1).

IRCBNX is known to build in the following environments:

FreeBSD 9.1:
- GCC 4.2
- Clang 3.1

Windows 7:
- MingW64 4.5.3
- Visual Studio 2010

