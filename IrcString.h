/*-
 * Copyright (c) 2012 Nathan Lay (nslay@users.sourceforge.net)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IRCSTRING_H
#define IRCSTRING_H

enum IrcCaseMapping { RFC1459 = 0, STRICT_RFC1459, ASCII };

int IrcToUpper(int c, IrcCaseMapping mapping = ASCII);

int IrcToLower(int c, IrcCaseMapping mapping = ASCII);

int IrcIsSpecial(int c);

bool IrcIsHostmask(const char *pString);

bool IrcIsNickname(const char *pString);

int IrcStrCaseCmp(const char *pString1, const char *pString2, IrcCaseMapping mapping = ASCII);

char * IrcStrCaseStr(const char *pBig, const char *pLittle, IrcCaseMapping mapping = ASCII);

bool IrcMatch(const char *pPattern, const char *pString, IrcCaseMapping mapping = ASCII);

#endif // !IRCSTRING_H

