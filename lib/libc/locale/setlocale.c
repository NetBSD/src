/* $NetBSD: setlocale.c,v 1.56 2009/01/11 02:46:29 christos Exp $ */

/*-
 * Copyright (c)2008 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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
__RCSID("$NetBSD: setlocale.c,v 1.56 2009/01/11 02:46:29 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <langinfo.h>
#define __SETLOCALE_SOURCE__
#include <locale.h>
#include <paths.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "setlocale_local.h"

const char *_PathLocale = NULL;

__link_set_decl(all_categories, _locale_category_t);

#ifndef __lint__
static
#endif
const _locale_category_t dummy = {
    .category = _LC_LAST, /* XXX */
    .setlocale = NULL,
};

__link_set_add_data(all_categories, dummy);

_locale_category_t *
_find_category(int category)
{
	_locale_category_t * const *p;

	__link_set_foreach(p, all_categories) {
		if ((*p)->category == category)
			return *p;
	}
	return NULL;
}

const char *
_get_locale_env(const char *category)
{
	const char *name;

	/* 1. check LC_ALL */
	name = (const char *)getenv("LC_ALL");
	if (name == NULL || *name == '\0') {
		/* 2. check LC_* */
		name = (const char *)getenv(category);
		if (name == NULL || *name == '\0') {
			/* 3. check LANG */
			name = getenv("LANG");
		}
	}
	if (name == NULL || *name == '\0' || strchr(name, '/'))
		/* 4. if none is set, fall to "C" */
		name = _C_LOCALE;
	return name;
}

char *
__setlocale(int category, const char *name)
{
	_locale_category_t *l;
	struct _locale_impl_t *impl;

	if (category >= LC_ALL && category < _LC_LAST) {
		l = _find_category(category);
		if (l != NULL) {
			if (issetugid() || ((_PathLocale == NULL &&
			    (_PathLocale = getenv("PATH_LOCALE")) == NULL) ||
			    *_PathLocale == '\0'))
				_PathLocale = _PATH_LOCALE;
			impl = *_current_locale();
			return __UNCONST((*l->setlocale)(name, impl));
		}
	}
	return NULL;
}
