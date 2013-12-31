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

#include <cstddef>
#include <cctype>
#include <cstring>
#include "IrcString.h"

int IrcToUpper(int c, IrcCaseMapping mapping) {
	switch (mapping) {
	case RFC1459:
		switch (c) {
		case '{':
			return '[';
		case '}':
			return ']';
		case '|':
			return '\\';
		case '^':
			return '~';
		}
		break;
	case STRICT_RFC1459:
		switch (c) {
		case '{':
			return '[';
		case '}':
			return ']';
		case '|':
			return '\\';
		}
		break;
	case ASCII:
		break;
	}

	return toupper(c);
}

int IrcToLower(int c, IrcCaseMapping mapping) {
	switch (mapping) {
	case RFC1459:
		switch (c) {
		case '[':
			return '{';
		case ']':
			return '}';
		case '\\':
			return '|';
		case '~':
			return '^';
		}
		break;
	case STRICT_RFC1459:
		switch (c) {
		case '[':
			return '{';
		case ']':
			return '}';
		case '\\':
			return '|';
		}
		break;
	case ASCII:
		break;
	}

	return tolower(c);
}

int IrcIsPunct(int c, IrcCaseMapping mapping) {
	switch(mapping) {
	case RFC1459:
		switch (c) {
		case '[':
		case '{':
		case ']':
		case '}':
		case '\\':
		case '|':
		case '~':
		case '^':
			return 0;
		}
		break;
	case STRICT_RFC1459:
		switch (c) {
		case '[':
		case '{':
		case ']':
		case '}':
		case '\\':
		case '|':
			return 0;
		}
		break;
	case ASCII:
		break;
	}

	return ispunct(c);
}

int IrcIsSpecial(int c) {
	switch (c) {
	case '[':
	case ']':
	case '\\':
	case '`':
	case '_':
	case '^':
	case '{':
	case '|':
	case '}':
		return 1;
	}

	return 0;
}

bool IrcIsHostmask(const char *pString) {
	if (pString == NULL || *pString == '!')
		return false;

	pString = strchr(pString,'!');

	if (pString == NULL || *(++pString) == '@')
		return false;

	pString = strchr(pString,'@');

	if (pString == NULL || *(++pString) == '\0')
		return false;

	return true;
}

bool IrcIsNickname(const char *pString) {
	if (!isalpha(*pString) && !IrcIsSpecial(*pString))
		return false;

	++pString;

	for ( ; isalnum(*pString) || IrcIsSpecial(*pString) || *pString == '-'; ++pString);

	return *pString == '\0';
}

int IrcStrCaseCmp(const char *pString1, const char *pString2, IrcCaseMapping mapping) {
	for ( ; *pString1 != '\0' && 
		*pString2 != '\0' && 
		IrcToLower(*pString1, mapping) == IrcToLower(*pString2, mapping); ++pString1, ++pString2);

	return IrcToLower(*pString1, mapping)-IrcToLower(*pString2, mapping);
}

char * IrcStrCaseStr(const char *pBig, const char *pLittle, IrcCaseMapping mapping) {
	const char *pMatch = NULL, *pLittleCurrent = pLittle;

	for ( ; *pBig != '\0' && *pLittleCurrent != '\0'; ++pBig) {
		if (IrcToLower(*pLittleCurrent, mapping) != IrcToLower(*pBig, mapping)) {
			pMatch = NULL;
			pLittleCurrent = pLittle;
		}

		if (IrcToLower(*pLittleCurrent, mapping) == IrcToLower(*pBig, mapping)) {
			if (pMatch == NULL)
				pMatch = pBig;

			++pLittleCurrent;
		}
	}

	return *pLittleCurrent != '\0' ? NULL : (char *)pMatch;
}

char * IrcStrCaseWord(const char *pBig, const char *pWord, IrcCaseMapping mapping) {
	while (*pBig != '\0') {
		for ( ; *pBig != '\0' && (IrcIsPunct(*pBig, mapping) || isspace(*pBig)); ++pBig);

		const char *pMatch = pBig;
		const char *pWordCurrent = pWord;

		for ( ; *pBig != '\0' && *pWordCurrent != '\0' && IrcToLower(*pWordCurrent, mapping) == IrcToLower(*pBig, mapping); ++pBig, ++pWordCurrent);

		if (*pWordCurrent == '\0' && (*pBig == '\0' || IrcIsPunct(*pBig, mapping) || isspace(*pBig)))
			return (char *)pMatch;

		for ( ; *pBig != '\0' && !IrcIsPunct(*pBig, mapping) && !isspace(*pBig); ++pBig);
	}

	return NULL;
}

bool IrcMatch(const char *pPattern, const char *pString, IrcCaseMapping mapping) {
	if (pPattern == NULL || pString == NULL)
		return false;

	const char *pPatternLast = NULL, *pStringLast = NULL;

	do {
		if (*pPattern == '*') {
			for ( ; *pPattern == '*'; ++pPattern);
			pPatternLast = pPattern;
			pStringLast = pString;
		}
		

		if (*pPattern == '?') {
			if (*pString == '\0') {
				if (pPatternLast == NULL || *pStringLast == '\0')
					return false;

				pPattern = pPatternLast;
				pString = ++pStringLast;
				continue;
			}
		}
		else {
			if (*pPattern == '\\')
				++pPattern;

			if (IrcToLower(*pPattern, mapping) != IrcToLower(*pString, mapping)) {
				if (pPatternLast == NULL || *pStringLast == '\0')
					return false;

				pPattern = pPatternLast;
				pString = ++pStringLast;
				continue;
			}
		}

		if (*pPattern != '\0')
			++pPattern;

		if (*pString != '\0')
			++pString;

	} while (*pString != '\0');

	for ( ; *pPattern == '*'; ++pPattern);

	return *pPattern == '\0' && *pString == '\0';
}

