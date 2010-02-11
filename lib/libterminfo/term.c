/* $NetBSD: term.c,v 1.5 2010/02/11 00:27:09 roy Exp $ */

/*
 * Copyright (c) 2009, 2010 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: term.c,v 1.5 2010/02/11 00:27:09 roy Exp $");

#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <ndbm.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <term_private.h>
#include <term.h>

#define TERMINFO_DIRS		"/usr/share/misc/terminfo"

static char database[PATH_MAX];
static char pathbuf[PATH_MAX];
const char *_ti_database;

/* rescue.c is generated from /usr/src/share/terminfo/terminfo
 * at build time as an array of strings.
 * static const char *rescue_terms[] = {
 *	"ansi",
 *	"\002\005\000\141\156\163\151\000\000\000\043\000\141\156\163\151\057",
 *	NULL,
 *	NULL
 * };
 */
 
#include "rescue.c"

static int
_ti_readterm(TERMINAL *term, const char *cap, size_t caplen, int flags)
{
	uint8_t ver;
	uint16_t ind, num;
	size_t len;
	TERMUSERDEF *ud;

	ver = *cap++;
	/* Only read version 1 and 2 structures */
	if (ver != 1 && ver != 2) {
		errno = EINVAL;
		return -1;
	}

	term->flags = calloc(TIFLAGMAX + 1, sizeof(char));
	if (term->flags == NULL)
		goto err;
	term->nums = malloc((TINUMMAX + 1) * sizeof(short));
	if (term->nums == NULL)
		goto err;
	memset(term->nums, (short)-1, (TINUMMAX + 1) * sizeof(short));
	term->strs = calloc(TISTRMAX + 1, sizeof(char *));
	if (term->strs == NULL)
		goto err;
	term->_arealen = caplen;
	term->_area = malloc(term->_arealen);
	if (term->_area == NULL)
		goto err;
 	memcpy(term->_area, cap, term->_arealen);

	cap = term->_area;
	len = le16dec(cap);
	cap += sizeof(uint16_t);
	term->name = cap;
	cap += len;
	if (ver == 1)
		term->_alias = NULL;
	else {
		len = le16dec(cap);
		cap += sizeof(uint16_t);
		if (len == 0)
			term->_alias = NULL;
		else {
			term->_alias = cap;
			cap += len;
		}
	}
	len = le16dec(cap);
	cap += sizeof(uint16_t);
	term->desc = cap;
	cap += len;

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
			term->nums[ind] = le16dec(cap);
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
		term->_nuserdefs = le16dec(cap);
		term->_userdefs = malloc(sizeof(*term->_userdefs) * num);
		cap += sizeof(uint16_t);
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
				ud->num = le16dec(cap);
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
				errno = EINVAL;
				goto err;
			}
		}
	}
	return 1;
	
err:
	_ti_freeterm(term);
	return -1;
}

static int
_ti_dbgetterm(TERMINAL *term, const char *path, const char *name, int flags)
{
	DBM *db;
	datum dt;
	char *p;
	int r;

	db = dbm_open(path, O_RDONLY, 0644);
	if (db == NULL)
		return -1;
	strlcpy(database, path, sizeof(database));
	_ti_database = database;
	dt.dptr = (void *)__UNCONST(name);
	dt.dsize = strlen(name);
	dt = dbm_fetch(db, dt);
	if (dt.dptr == NULL) {
		dbm_close(db);
		return 0;
	}

	for (;;) {
		p = (char *)dt.dptr;
		if (*p++ != 0) /* not alias */
			break;
		dt.dsize = le16dec(p) - 1;
		p += sizeof(uint16_t);
		dt.dptr = p;
		dt = dbm_fetch(db, dt);
		if (dt.dptr == NULL) {
			dbm_close(db);
			return 0;
		}
	}
		
	r = _ti_readterm(term, (char *)dt.dptr, dt.dsize, flags);
	dbm_close(db);
	return r;
}

static int
_ti_dbgettermp(TERMINAL *term, const char *path, const char *name, int flags)
{
	const char *p;
	size_t l;
	int r, e;

	e = -1;
	r = 0;
	do {
		for (p = path; *path != '\0' && *path != ':'; path++)
			continue;
		l = path - p;
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

int
_ti_getterm(TERMINAL *term, const char *name, int flags)
{
	int r;
	char *e, h[PATH_MAX];
	const char **p;

	_DIAGASSERT(term != NULL);
	_DIAGASSERT(name != NULL);

	database[0] = '\0';
	_ti_database = NULL;

	e = getenv("TERMINFO");
	if (e != NULL)
		return _ti_dbgetterm(term, e, name, flags);

	e = getenv("HOME");
	if (e != NULL) {
		snprintf(h, sizeof(h), "%s/.terminfo", e);
		r = _ti_dbgetterm(term, h, name, flags);
		if (r == 1)
			return 1;
	}

	r = _ti_dbgettermp(term, TERMINFO_DIRS, name, flags);
	if (r == 1)
		return 1;

	for (p = rescue_terms; *p != NULL; p++, p++)
		if (strcmp(name, *p) == 0) {
			r = _ti_readterm(term, *(p + 1), 4096, flags);
			break;
		}

	return r;
}

void
_ti_freeterm(TERMINAL *term)
{

	_DIAGASSERT(term != NULL);
	
	free(term->_area);
	term->_area = NULL;
	free(term->strs);
	term->strs = NULL;
	free(term->nums);
	term->nums = NULL;
	free(term->flags);
	term->flags = NULL;
	free(term->_userdefs);
	term->_userdefs = NULL;
}
