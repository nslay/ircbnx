#######################################################################
# Introduction                                                        #
#######################################################################

IRCBNX Chatterbot is a moderation IRC bot based on the original 
Battle.net text gateway bot BNX Chatterbot. Like the original, it 
features a rule-based response system, an access system, and several 
moderation features.

#######################################################################
# Installing                                                          #
#######################################################################

If a precompiled version is available for your operating system,
either extract the archive where it best suits you, or copy the
executable, response.txt, and bot.ini to the desired location.

#######################################################################
# Usage                                                               #
#######################################################################

Once installed and configured, simply run "ircbnx" from the command 
line.

NOTE: It will NOT produce output, but it should be working properly.
      Be sure to check "bot.log" for additional information.

IRCBNX also has some command line flags:
Usage: ircbnx [-hv] [-c config.ini]

-h - Produces the above usage.
-v - Queries the version of IRCBNX.
-c - Specify a different configuration file (by default it's bot.ini).

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
responserules - The response rules file to use.

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

Commands will be grouped and documented by minimum required access 
level.

All commands are issued by directly messaging the bot. You must "login"
to the bot prior to issuing commands, see "login" under "Level 0".

*** Level 100 ***

shutdown - Have the bot quit and exit.

userlist - Display the access list.

useradd <hostmask> <accesslevel> <password> - Add a new user to command
                                              the bot.

userdel <hostmask> - Delete user from the access list.

nick <newnick> - Chane the bot's nickname.

*** Level 90 ***

op <channel> <nick> - Give operator status to a user.

deop <channel> <nick> - Remove operator status from a user.

*** Level 75 ***

chatter - Enable chattering. The bot will produce automated responses.
          This clears the squelch list.

shutup - Disable chattering. The bot will no longer produce automated
         responses.

join <channel> - Have the bot join a new home channel.

part <channel> - Have the bot part from a channel.

rejoin <channel> - Have the bot rejoin a channel.

squelch <hostmask> - Ignore all users matching a hostmask. Users can
                     still command the bot but it will not produce
                     automated or CTCP responses.

ignore <hostmask> - Same as squelch.

unsquelch <hostmask> - No longer ignore users matching the given 
                       hostmask.

shitadd <hostmask> - Add a hostmask to ban on sight.

shitdel <hostmask> - Remove a hostmask from the shitlist.

shitlist - List all shit listed hostmasks.

*** Level 60 ***

kick <channel> <nick> [<reason>] - Have the bot kick a user from a
                                   channel it has operator status in.

ban <channel> <nick|hostmask> [<reason>] - Have the bot set mode +b on
                                           a given hostmask. If a nick
                                           is provided, it will search
                                           the channel for the
                                           hostmask.

unban <channel> <hostmask> - Set mode -b on a given hostmask.

splatterkick <channel> <nick> - Ban a user with one of twelve humorous
                                sequences.

voteban <channel> <nick> - Hold a 30 second vote on whether to ban a 
                           user.

*** Level 0 ***

login <password> - Login to the bot to issue commands. 

logout - Logout of the bot. No commands can be issued until you "login"
         again.

say <target> <message> - Have the bot send a message to a user(s) or 
                         channel(s). The message may use "/me" or 
                         "/action"

where - Have the bot tell you its current channels.

who <channel> - Have the bot tell you the users in a channel it is
                currently in.

#######################################################################
# Miscellaneous                                                       #
#######################################################################

IRCBNX exhibits some of the following additional behaviors:
- It will shitlist users who kick it from a channel.
- It will progressively warn, kick, and ban users flooding the
  channel it has ops in.
- It will automatically logout users after 10 minutes of inactivity.
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
http://pcre.org/

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

CentOS 6.3:
- GCC 4.4.6

Windows 7:
- MingW64 4.5.3
- Visual Studio 2010

