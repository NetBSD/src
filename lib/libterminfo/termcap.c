/* $NetBSD: termcap.c,v 1.4 2010/03/01 11:02:31 roy Exp $ */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: termcap.c,v 1.4 2010/03/01 11:02:31 roy Exp $");

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <term_private.h>
#include <term.h>
#include <termcap.h>
#include <unistd.h>
#include <stdio.h>

#include "termcap_map.c"
#include "termcap_hash.c"

char *UP;
char *BC;

/* ARGSUSED */
int
tgetent(__unused char *bp, const char *name)
{
	int errret;
	static TERMINAL *last = NULL;

	_DIAGASSERT(name != NULL);

	/* Free the old term */
	if (last != NULL) {
		del_curterm(last);
		last = NULL;
	}
	errret = -1;
	if (setupterm(name, STDOUT_FILENO, &errret) != 0)
		return errret;
	last = cur_term;

	if (pad_char != NULL)
		PC = pad_char[0];
	UP = __UNCONST(cursor_up);
	BC = __UNCONST(cursor_left);
	return 1;
}

int
tgetflag(const char *id)
{
	uint32_t ind;
	size_t i;
	TERMUSERDEF *ud;

	_DIAGASSERT(id != NULL);

	if (cur_term == NULL)
		return 0;

	ind = _t_flaghash((const unsigned char *)id, strlen(id));
	if (ind <= __arraycount(_ti_cap_flagids)) {
		if (strcmp(id, _ti_cap_flagids[ind].id) == 0)
			return cur_term->flags[_ti_cap_flagids[ind].ti];
	}
	for (i = 0; i < cur_term->_nuserdefs; i++) {
		ud = &cur_term->_userdefs[i];
		if (ud->type == 'f' && strcmp(ud->id, id) == 0)
			return ud->flag;
	}
	return 0;
}

int
tgetnum(const char *id)
{
	uint32_t ind;
	size_t i;
	TERMUSERDEF *ud;
	const TENTRY *te;

	_DIAGASSERT(id != NULL);

	if (cur_term == NULL)
		return -1;

	ind = _t_numhash((const unsigned char *)id, strlen(id));
	if (ind <= __arraycount(_ti_cap_numids)) {
		te = &_ti_cap_numids[ind];
		if (strcmp(id, te->id) == 0) {
			if (!VALID_NUMERIC(cur_term->nums[te->ti]))
				return ABSENT_NUMERIC;
			return cur_term->nums[te->ti];
		}
	}
	for (i = 0; i < cur_term->_nuserdefs; i++) {
		ud = &cur_term->_userdefs[i];
		if (ud->type == 'n' && strcmp(ud->id, id) == 0) {
			if (!VALID_NUMERIC(ud->num))
				return ABSENT_NUMERIC;
			return ud->num;
		}
	}
	return -1;
}

char *
tgetstr(const char *id, __unused char **area)
{
	uint32_t ind;
	size_t i;
	TERMUSERDEF *ud;
	const char *str;

	_DIAGASSERT(id != NULL);

	if (cur_term == NULL)
		return NULL;

	str = NULL;
	ind = _t_strhash((const unsigned char *)id, strlen(id));
	if (ind <= __arraycount(_ti_cap_strids)) {
		if (strcmp(id, _ti_cap_strids[ind].id) == 0) {
			str = cur_term->strs[_ti_cap_strids[ind].ti];
			if (str == NULL)
				return NULL;
		}
	}
	if (str != NULL)
		for (i = 0; i < cur_term->_nuserdefs; i++) {
			ud = &cur_term->_userdefs[i];
			if (ud->type == 's' && strcmp(ud->id, id) == 0)
				str = ud->str;
		}

	/* XXX: FXIXME
	 * We should fix sgr0(me) as it has a slightly different meaning
	 * for termcap. */

	if (str != NULL && area != NULL && *area != NULL) {
		char *s;
		s = *area;
		strcpy(*area, str);
		*area += strlen(*area) + 1;
		return s;
	}

	return __UNCONST(str);
}

char *
tgoto(const char *cm, int destcol, int destline)
{
	
	_DIAGASSERT(cm != NULL);
	return vtparm(cm, destline, destcol);
}

static const char *
flagname(const char *key)
{
	uint32_t idx;

	idx = _t_flaghash((const unsigned char *)key, strlen(key));
	if (idx <= __arraycount(_ti_cap_flagids) &&
	    strcmp(key, _ti_cap_flagids[idx].id) == 0)
		return _ti_flagid(_ti_cap_flagids[idx].ti);
	return key;
}

static const char *
numname(const char *key)
{
	uint32_t idx;

	idx = _t_numhash((const unsigned char *)key, strlen(key));
	if (idx <= __arraycount(_ti_cap_numids) && 
	    strcmp(key, _ti_cap_numids[idx].id) == 0)
		return _ti_numid(_ti_cap_numids[idx].ti);
	return key;
}

static const char *
strname(const char *key)
{
	uint32_t idx;

	idx = _t_strhash((const unsigned char *)key, strlen(key));
	if (idx <= __arraycount(_ti_cap_strids) &&
	    strcmp(key, _ti_cap_strids[idx].id) == 0)
		return _ti_strid(_ti_cap_strids[idx].ti);

	if (strcmp(key, "tc") == 0)
		return "use";

	return key;
}

/* We don't currently map %> %B %D
 * That means no conversion for regent100, hz1500, act4, act5, mime terms. */
static char *
strval(const char *val)
{
	char *info, *ip, c;
	int p;
	size_t len, l, n;

	len = 1024; /* no single string should be bigger */
	info = ip = malloc(len);
	if (info == NULL)
		return 0;

	l = 0;
	p = 1;
	for (; *val != '\0'; val++) {
		if (l + 2 > len)
			goto elen;
		if (*val != '%') {
			if (*val == ',') {
				if (l + 3 > len)
					goto elen;
				*ip++ = '\\';
				l++;
			}
			*ip++ = *val;
			l++;
			continue;
		}
		switch (c = *(++val)) {
		case 'd':
			if (l + 6 > len)
				goto elen;
			*ip++ = '%';
			*ip++ = 'p';
			*ip++ = '0' + p;
			*ip++ = '%';
			*ip++ = 'd';
			l += 5;
			n += 5;
			/* FALLTHROUGH */
		case 'r':
			p = 3 - p;
			break;
		default:
			/* Hope it matches a terminfo command. */
			*ip++ = '%';
			*ip++ = c;
			l += 2;
			break;
		}
	}

	*ip = '\0';
	return info;

elen:
	free(info);
	errno = ENOMEM;
	return NULL;
}

char *
captoinfo(char *cap)
{
	char *info, *ip, *token, *val, *p, tok[3];
	const char *name;
	size_t len, lp, nl, vl, rl;

	_DIAGASSERT(cap != NULL);

	len = strlen(cap) * 2;
	info = ip = malloc(len);
	if (info == NULL)
		return NULL;

	lp = 0;
	tok[2] = '\0';
	while ((token = strsep(&cap, ":")) != NULL) {
		/* Trim whitespace */
		while (isspace((unsigned char)*token))
			token++;
		if (token[0] == '\0')
			continue;
		name = token;
		val = NULL;
		if (token[1] != '\0') {
			tok[0] = token[0];
			tok[1] = token[1];
			if (token[2] == '\0') {
				name = flagname(tok);
				val = NULL;
			} else if (token[2] == '#') {
				name = numname(tok);
				val = token + 2;
			} else if (token[2] == '=') {
				name = strname(tok);
				val = strval(token + 2);
			}
		}
		nl = strlen(name);
		if (val == NULL)
			vl = 0;
		else
			vl = strlen(val);
		rl = nl + vl + 3; /* , \0 */

		if (lp + rl > len) {
			if (rl < 256)
				len += 256;
			else
				len += rl;
			p = realloc(info, len);
			if (p == NULL)
				return NULL;
			info = p;
		}

		if (ip != info) {
			*ip++ = ',';
			*ip++ = ' ';
		}

		strcpy(ip, name);
		ip += nl;
		if (val != NULL) {
			strcpy(ip, val);
			ip += vl;
			if (token[2] == '=')
				free(val);
		}
	}

	*ip = '\0';
	return info;
}

