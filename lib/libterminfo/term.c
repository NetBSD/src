/* $NetBSD: term.c,v 1.28.8.1 2018/10/20 06:58:22 pgoyette Exp $ */

/*
 * Copyright (c) 2009, 2010, 2011 The NetBSD Foundation, Inc.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Roy Marples.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: term.c,v 1.28.8.1 2018/10/20 06:58:22 pgoyette Exp $");

#include <sys/stat.h>

#include <assert.h>
#include <cdbr.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term_private.h>
#include <term.h>

#define _PATH_TERMINFO		"/usr/share/misc/terminfo"

static char __ti_database[PATH_MAX];
const char *_ti_database;

/* Include a generated list of pre-compiled terminfo descriptions. */
#include "compiled_terms.c"

static int
allocset(void *pp, int init, size_t nelem, size_t elemsize)
{
	void **p = pp;
	if (*p) {
		memset(*p, init, nelem * elemsize);
		return 0;
	}

	if ((*p = calloc(nelem, elemsize)) == NULL)
		return -1;

	if (init != 0)
		memset(*p, init, nelem * elemsize);
	return 0;
}

static int
_ti_readterm(TERMINAL *term, const char *cap, size_t caplen, int flags)
{
	char ver;
	uint16_t ind, num;
	size_t len;
	TERMUSERDEF *ud;

	if (caplen == 0)
		goto out;
	ver = *cap++;
	caplen--;
	/* Only read version 1 structures */
	if (ver != 1)
		goto out;

	if (allocset(&term->flags, 0, TIFLAGMAX+1, sizeof(*term->flags)) == -1)
		return -1;

	if (allocset(&term->nums, -1, TINUMMAX+1, sizeof(*term->nums)) == -1)
		return -1;

	if (allocset(&term->strs, 0, TISTRMAX+1, sizeof(*term->strs)) == -1)
		return -1;

	if (term->_arealen != caplen) {
		term->_arealen = caplen;
		term->_area = realloc(term->_area, term->_arealen);
		if (term->_area == NULL)
			return -1;
	}
	memcpy(term->_area, cap, term->_arealen);

	cap = term->_area;
	len = le16dec(cap);
	cap += sizeof(uint16_t);
	term->name = cap;
	cap += len;
	len = le16dec(cap);
	cap += sizeof(uint16_t);
	if (len == 0)
		term->_alias = NULL;
	else {
		term->_alias = cap;
		cap += len;
	}
	len = le16dec(cap);
	cap += sizeof(uint16_t);
	if (len == 0)
		term->desc = NULL;
	else {
		term->desc = cap;
		cap += len;
	}

	num = le16dec(cap);
	cap += sizeof(uint16_t);
	if (num != 0) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		for (; num != 0; num--) {
			ind = le16dec(cap);
			cap += sizeof(uint16_t);
			term->flags[ind] = *cap++;
			if (flags == 0 && !VALID_BOOLEAN(term->flags[ind]))
				term->flags[ind] = 0;
		}
	}

	num = le16dec(cap);
	cap += sizeof(uint16_t);
	if (num != 0) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		for (; num != 0; num--) {
			ind = le16dec(cap);
			cap += sizeof(uint16_t);
			term->nums[ind] = (short)le16dec(cap);
			if (flags == 0 && !VALID_NUMERIC(term->nums[ind]))
				term->nums[ind] = ABSENT_NUMERIC;
			cap += sizeof(uint16_t);
		}
	}

	num = le16dec(cap);
	cap += sizeof(uint16_t);
	if (num != 0) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		for (; num != 0; num--) {
			ind = le16dec(cap);
			cap += sizeof(uint16_t);
			len = le16dec(cap);
			cap += sizeof(uint16_t);
			if (len > 0)
				term->strs[ind] = cap;
			else if (flags == 0)
				term->strs[ind] = ABSENT_STRING;
			else
				term->strs[ind] = CANCELLED_STRING;
			cap += len;
		}
	}

	num = le16dec(cap);
	cap += sizeof(uint16_t);
	if (num != 0) {
		num = le16dec(cap);
		cap += sizeof(uint16_t);
		if (num != term->_nuserdefs) {
			free(term->_userdefs);
			term->_userdefs = NULL;
			term->_nuserdefs = num;
		}
		if (allocset(&term->_userdefs, 0, term->_nuserdefs,
		    sizeof(*term->_userdefs)) == -1)
			return -1;
		for (num = 0; num < term->_nuserdefs; num++) {
			ud = &term->_userdefs[num];
			len = le16dec(cap);
			cap += sizeof(uint16_t);
			ud->id = cap;
			cap += len;
			ud->type = *cap++;
			switch (ud->type) {
			case 'f':
				ud->flag = *cap++;
				if (flags == 0 &&
				    !VALID_BOOLEAN(ud->flag))
					ud->flag = 0;
				ud->num = ABSENT_NUMERIC;
				ud->str = ABSENT_STRING;
				break;
			case 'n':
				ud->flag = ABSENT_BOOLEAN;
				ud->num = (short)le16dec(cap);
				if (flags == 0 &&
				    !VALID_NUMERIC(ud->num))
					ud->num = ABSENT_NUMERIC;
				ud->str = ABSENT_STRING;
				cap += sizeof(uint16_t);
				break;
			case 's':
				ud->flag = ABSENT_BOOLEAN;
				ud->num = ABSENT_NUMERIC;
				len = le16dec(cap);
				cap += sizeof(uint16_t);
				if (len > 0)
					ud->str = cap;
				else if (flags == 0)
					ud->str = ABSENT_STRING;
				else
					ud->str = CANCELLED_STRING;
				cap += len;
				break;
			default:
				goto out;
			}
		}
	} else {
		term->_nuserdefs = 0;
		if (term->_userdefs) {
			free(term->_userdefs);
			term->_userdefs = NULL;
		}
	}

	return 1;
out:
	errno = EINVAL;
	return -1;
}

static int
_ti_checkname(const char *name, const char *termname, const char *termalias)
{
	const char *alias, *s;
	size_t len, l;

	/* Check terminal name matches. */
	if (strcmp(termname, name) == 0)
		return 1;

	/* Check terminal aliases match. */
	if (termalias == NULL)
		return 0;

	len = strlen(name);
	alias = termalias;
	while (*alias != '\0') {
		s = strchr(alias, '|');
		if (s == NULL)
			l = strlen(alias);
		else
			l = (size_t)(s - alias);
		if (len == l && memcmp(alias, name, l) == 0)
			return 1;
		if (s == NULL)
			break;
		alias = s + 1;
	}

	/* No match. */
	return 0;
}

static int
_ti_dbgetterm(TERMINAL *term, const char *path, const char *name, int flags)
{
	struct cdbr *db;
	const void *data;
	const uint8_t *data8;
	size_t len, klen;
	int r;

	r = snprintf(__ti_database, sizeof(__ti_database), "%s.cdb", path);
	if (r < 0 || (size_t)r > sizeof(__ti_database)) {
		db = NULL;
		errno = ENOENT; /* To fall back to a non extension. */
	} else
		db = cdbr_open(__ti_database, CDBR_DEFAULT);

	/* Target file *may* be a cdb file without the extension. */
	if (db == NULL && errno == ENOENT) {
		len = strlcpy(__ti_database, path, sizeof(__ti_database));
		if (len < sizeof(__ti_database))
			db = cdbr_open(__ti_database, CDBR_DEFAULT);
	}
	if (db == NULL)
		return -1;

	r = 0;
	klen = strlen(name) + 1;
	if (cdbr_find(db, name, klen, &data, &len) == -1)
		goto out;
	data8 = data;
	if (len == 0)
		goto out;

	/* If the entry is an alias, load the indexed terminfo description. */
	if (data8[0] == 2) {
		if (cdbr_get(db, le32dec(data8 + 1), &data, &len))
			goto out;
		data8 = data;
	}

	r = _ti_readterm(term, data, len, flags);
	/* Ensure that this is the right terminfo description. */
        if (r == 1)
                r = _ti_checkname(name, term->name, term->_alias);
	/* Remember the database we read. */
        if (r == 1)
                _ti_database = __ti_database;

out:
	cdbr_close(db);
	return r;
}

static int
_ti_dbgettermp(TERMINAL *term, const char *path, const char *name, int flags)
{
	const char *p;
	char pathbuf[PATH_MAX];
	size_t l;
	int r, e;

	e = -1;
	r = 0;
	do {
		for (p = path; *path != '\0' && *path != ':'; path++)
			continue;
		l = (size_t)(path - p);
		if (l != 0 && l + 1 < sizeof(pathbuf)) {
			memcpy(pathbuf, p, l);
			pathbuf[l] = '\0';
			r = _ti_dbgetterm(term, pathbuf, name, flags);
			if (r == 1)
				return 1;
			if (r == 0)
				e = 0;
		}
	} while (*path++ == ':');
	return e;
}

static int
_ti_findterm(TERMINAL *term, const char *name, int flags)
{
	int r;
	char *c, *e;

	_DIAGASSERT(term != NULL);
	_DIAGASSERT(name != NULL);

	_ti_database = NULL;
	r = 0;

	if ((e = getenv("TERMINFO")) != NULL && *e != '\0') {
		if (e[0] == '/')
			return _ti_dbgetterm(term, e, name, flags);
	}

	c = NULL;
	if (e == NULL && (c = getenv("TERMCAP")) != NULL) {
		if (*c != '\0' && *c != '/') {
			c = strdup(c);
			if (c != NULL) {
				e = captoinfo(c);
				free(c);
			}
		}
	}

	if (e != NULL) {
		TIC *tic;

		if (c == NULL)
			e = strdup(e); /* So we don't destroy env */
		if (e == NULL)
			tic = NULL;
		else {
			tic = _ti_compile(e, TIC_WARNING |
			    TIC_ALIAS | TIC_DESCRIPTION | TIC_EXTRA);
			free(e);
		}
		if (tic != NULL &&
		    _ti_checkname(name, tic->name, tic->alias) == 1)
		{
			uint8_t *f;
			ssize_t len;

			len = _ti_flatten(&f, tic);
			if (len != -1) {
				r = _ti_readterm(term, (char *)f, (size_t)len,
				    flags);
				free(f);
			}
		}
		_ti_freetic(tic);
		if (r == 1) {
			if (c == NULL)
				_ti_database = "$TERMINFO";
			else
				_ti_database = "$TERMCAP";
			return r;
		}
	}

	if ((e = getenv("TERMINFO_DIRS")) != NULL)
		return _ti_dbgettermp(term, e, name, flags);

	if ((e = getenv("HOME")) != NULL) {
		char homepath[PATH_MAX];

		if (snprintf(homepath, sizeof(homepath), "%s/.terminfo", e) > 0)
			r = _ti_dbgetterm(term, homepath, name, flags);
	}
	if (r != 1)
		r = _ti_dbgettermp(term, _PATH_TERMINFO, name, flags);

	return r;
}

int
_ti_getterm(TERMINAL *term, const char *name, int flags)
{
	int r;
	size_t i;
	const struct compiled_term *t;

	r = _ti_findterm(term, name, flags);
	if (r == 1)
		return r;

	for (i = 0; i < __arraycount(compiled_terms); i++) {
		t = &compiled_terms[i];
		if (strcmp(name, t->name) == 0) {
			r = _ti_readterm(term, t->cap, t->caplen, flags);
			break;
		}
	}

	return r;
}
