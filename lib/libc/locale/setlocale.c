/*	$NetBSD: setlocale.c,v 1.34.2.4 2002/03/22 20:42:18 nathanw Exp $	*/

/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)setlocale.c	8.1 (Berkeley) 7/4/93";
#else
__RCSID("$NetBSD: setlocale.c,v 1.34.2.4 2002/03/22 20:42:18 nathanw Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#define _CTYPE_PRIVATE

#include "namespace.h"
#include <sys/localedef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <limits.h>
#include <ctype.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef WITH_RUNE
#include "rune.h"
#include "rune_local.h"
#else
#include "ctypeio.h"
#endif

/*
 * Category names for getenv()
 */
static const char *const categories[_LC_LAST] = {
    "LC_ALL",
    "LC_COLLATE",
    "LC_CTYPE",
    "LC_MONETARY",
    "LC_NUMERIC",
    "LC_TIME",
    "LC_MESSAGES"
};

/*
 * Current locales for each category
 */
static char current_categories[_LC_LAST][32] = {
    "C",
    "C",
    "C",
    "C",
    "C",
    "C",
    "C"
};

/*
 * The locales we are going to try and load
 */
static char new_categories[_LC_LAST][32];

/*
 * Backup area to back out changes on failure
 */
static char saved_categories[_LC_LAST][32];

static char current_locale_string[_LC_LAST * 33];
char *_PathLocale;

static char *currentlocale __P((void));
static char *loadlocale __P((int));
static const char *__get_locale_env __P((int));

char *
__setlocale(category, locale)
	int category;
	const char *locale;
{
	int i, loadlocale_success;
	size_t len;
	const char *env, *r;

	if (issetugid() ||
	    (!_PathLocale && !(_PathLocale = getenv("PATH_LOCALE"))))
		_PathLocale = _PATH_LOCALE;

	if (category < 0 || category >= _LC_LAST)
		return (NULL);

	if (!locale)
		return (category ?
		    current_categories[category] : currentlocale());

	/*
	 * Default to the current locale for everything.
	 */
	for (i = 1; i < _LC_LAST; ++i)
		(void)strlcpy(new_categories[i], current_categories[i],
		    sizeof(new_categories[i]));

	/*
	 * Now go fill up new_categories from the locale argument
	 */
	if (!*locale) {
		if (category == LC_ALL) {
			for (i = 1; i < _LC_LAST; ++i) {
				env = __get_locale_env(i);
				(void)strlcpy(new_categories[i], env,
				    sizeof(new_categories[i]));
			}
		}
		else {
			env = __get_locale_env(category);
			(void)strlcpy(new_categories[category], env,
				sizeof(new_categories[category]));
		}
	} else if (category) {
		(void)strlcpy(new_categories[category], locale,
		    sizeof(new_categories[category]));
	} else {
		if ((r = strchr(locale, '/')) == 0) {
			for (i = 1; i < _LC_LAST; ++i) {
				(void)strlcpy(new_categories[i], locale,
				    sizeof(new_categories[i]));
			}
		} else {
			for (i = 1; r[1] == '/'; ++r)
				;
			if (!r[1])
				return (NULL);	/* Hmm, just slashes... */
			do {
				len = r - locale > sizeof(new_categories[i]) - 1
					? sizeof(new_categories[i]) - 1
					: r - locale;
				(void)strncpy(new_categories[i++], locale, len);
				new_categories[i++][len] = 0;
				locale = r;
				while (*locale == '/')
				    ++locale;
				while (*++r && *r != '/');
			} while (*locale);
			while (i < _LC_LAST)
				(void)strlcpy(new_categories[i],
				    new_categories[i - 1],
				    sizeof(new_categories[i]));
		}
	}

	if (category)
		return (loadlocale(category));

	loadlocale_success = 0;
	for (i = 1; i < _LC_LAST; ++i) {
		(void)strlcpy(saved_categories[i], current_categories[i],
		    sizeof(saved_categories[i]));
		if (loadlocale(i) != NULL)
			loadlocale_success = 1;
	}

	/*
	 * If all categories failed, return NULL; we don't need to back
	 * changes off, since none happened.
	 */
	if (!loadlocale_success)
		return NULL;

	return (currentlocale());
}

static char *
currentlocale()
{
	int i;

	(void)strlcpy(current_locale_string, current_categories[1],
	    sizeof(current_locale_string));

	for (i = 2; i < _LC_LAST; ++i)
		if (strcmp(current_categories[1], current_categories[i])) {
			(void)snprintf(current_locale_string,
			    sizeof(current_locale_string), "%s/%s/%s/%s/%s/%s",
			    current_categories[1], current_categories[2],
			    current_categories[3], current_categories[4],
			    current_categories[5], current_categories[6]);
			break;
		}
	return (current_locale_string);
}

static char *
loadlocale(category)
	int category;
{
	char name[PATH_MAX];

	_DIAGASSERT(0 < category && category < _LC_LAST);

	if (strcmp(new_categories[category], current_categories[category]) == 0)
		return (current_categories[category]);

	if (!strcmp(new_categories[category], "C") ||
	    !strcmp(new_categories[category], "POSIX")) {

		switch (category) {
		case LC_CTYPE:
#ifdef WITH_RUNE
			(void)_xpg4_setrunelocale("C");
			(void)__runetable_to_netbsd_ctype("C");
#else
			if (_ctype_ != _C_ctype_) {
				/* LINTED const castaway */
				free((void *)_ctype_);
				_ctype_ = _C_ctype_;
			}
			if (_toupper_tab_ != _C_toupper_) {
				/* LINTED const castaway */
				free((void *)_toupper_tab_);
				_toupper_tab_ = _C_toupper_;
			}
			if (_tolower_tab_ != _C_tolower_) {
				/* LINTED const castaway */
				free((void *)_tolower_tab_);
				_tolower_tab_ = _C_tolower_;
			}
#endif
		}

		(void)strlcpy(current_categories[category],
		    new_categories[category],
		    sizeof(current_categories[category]));
		return current_categories[category];
	}

	(void)snprintf(name, sizeof(name), "%s/%s/%s",
	    _PathLocale, new_categories[category], categories[category]);

	switch (category) {
	case LC_CTYPE:
#ifdef WITH_RUNE
		if (_xpg4_setrunelocale(new_categories[category]))
			return NULL;
		if (__runetable_to_netbsd_ctype(new_categories[category])) {
			/* very unfortunate, but need to go to "C" locale */
			(void)_xpg4_setrunelocale("C");
			(void)__runetable_to_netbsd_ctype("C");
			return NULL;
		}
#else
		if (!__loadctype(name))
			return NULL;
#endif
		break;

	case LC_MESSAGES:
		/*
		 * XXX we don't have LC_MESSAGES support yet,
		 * but catopen may use the value of LC_MESSAGES category.
		 * so return successfully if locale directory is present.
		 */
		(void)snprintf(name, sizeof(name), "%s/%s",
			_PathLocale, new_categories[category]);
		/* local */
		{
			struct stat st;
			if (stat(name, &st) < 0)
				return NULL;
			if (!S_ISDIR(st.st_mode))
				return NULL;
		}
		break;

	case LC_COLLATE:
	case LC_MONETARY:
	case LC_NUMERIC:
	case LC_TIME:
		return NULL;
	}

	(void)strlcpy(current_categories[category],
		new_categories[category],
		sizeof(current_categories[category]));
	return current_categories[category];
}

static const char *
__get_locale_env(category)
	int category;
{
	const char *env;

	_DIAGASSERT(category != LC_ALL);

	/* 1. check LC_ALL. */
	env = getenv(categories[0]);

	/* 2. check LC_* */
	if (!env || !*env)
		env = getenv(categories[category]);

	/* 3. check LANG */
	if (!env || !*env)
		env = getenv("LANG");

	/* 4. if none is set, fall to "C" */
	if (!env || !*env || strchr(env, '/'))
		env = "C";

	return env;
}
