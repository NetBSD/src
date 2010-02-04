/* $NetBSD: termcap.c,v 1.2 2010/02/04 09:46:26 roy Exp $ */

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
__RCSID("$NetBSD: termcap.c,v 1.2 2010/02/04 09:46:26 roy Exp $");

#include <assert.h>
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
