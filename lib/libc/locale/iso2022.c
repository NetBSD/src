/*-
 * Copyright (c)1999 Citrus Project,
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
 *
 *	$Id: iso2022.c,v 1.1.2.2 2000/06/23 16:58:56 minoura Exp $
 */

#if defined(XPG4) || defined(DLRUNEMOD)
/*
 * mappings:
 * ASCII (ESC ( B)		00000000 00000000 00000000 0xxxxxxx
 * iso-8859-1 (ESC , A)		00000000 00000000 00000000 1xxxxxxx
 * 94 charset (ESC ( F)		0fffffff 00000000 00000000 0xxxxxxx
 * 94 charset (ESC ( M F)	0fffffff 1mmmmmmm 00000000 0xxxxxxx
 * 96 charset (ESC , F)		0fffffff 00000000 00000000 1xxxxxxx
 * 96 charset (ESC , M F)	0fffffff 1mmmmmmm 00000000 1xxxxxxx
 * 94x94 charset (ESC $ ( F)	0fffffff 00000000 0xxxxxxx 0xxxxxxx
 * 96x96 charset (ESC $ , F)	0fffffff 00000000 0xxxxxxx 1xxxxxxx
 * 94x94 charset (ESC & V ESC $ ( F)
 *				0fffffff 1vvvvvvv 0xxxxxxx 0xxxxxxx
 * 94x94x94 charset (ESC $ ( F)	0fffffff 0xxxxxxx 0xxxxxxx 0xxxxxxx
 * 96x96x96 charset (ESC $ , F)	0fffffff 0xxxxxxx 0xxxxxxx 1xxxxxxx
 */
#include <sys/types.h>
#include <machine/limits.h>

#include <errno.h>
#if !defined(__FreeBSD__)
#define _BSD_RUNE_T_    int
#define _BSD_CT_RUNE_T_ _rune_t
#include "rune.h"
#else
#include <rune.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(__FreeBSD__)
typedef u_int8_t __uint8_t;
#endif

_rune_t _ISO2022_sgetrune __P((const char *, size_t, char const **, void *));
int _ISO2022_sputrune __P((_rune_t, char *, size_t, char **, void *));
void _ISO2022_initstate __P((void *));
void _ISO2022_packstate __P((mbstate_t *, void *));
void _ISO2022_unpackstate __P((void *, const mbstate_t *));

#include "iso2022.h"

typedef struct {
	int	maxcharset;
	_Iso2022Charset	*recommend[4];
	size_t	recommendsize[4];
	_Iso2022Charset	initg[4];
	int	flags;
#define	F_8BIT	0x0001
#define	F_NOOLD	0x0002
#define	F_SI	0x0010	/*0F*/
#define	F_SO	0x0020	/*0E*/
#define	F_LS0	0x0010	/*0F*/
#define	F_LS1	0x0020	/*0E*/
#define	F_LS2	0x0040	/*ESC n*/
#define	F_LS3	0x0080	/*ESC o*/
#define	F_LS1R	0x0100	/*ESC ~*/
#define	F_LS2R	0x0200	/*ESC }*/
#define	F_LS3R	0x0400	/*ESC |*/
#define	F_SS2	0x0800	/*ESC N*/
#define	F_SS3	0x1000	/*ESC O*/
#define	F_SS2R	0x2000	/*8E*/
#define	F_SS3R	0x4000	/*8F*/
} _Iso2022Info;

typedef struct {
	_Iso2022Charset	g[4];
	size_t	gl;
	size_t	gr;
	u_char vers;
} _Iso2022State;

static _RuneState _ISO2022_RuneState = {
	sizeof(_Iso2022State),		/* sizestate */
	_ISO2022_initstate,		/* initstate */
	_ISO2022_packstate,		/* packstate */
	_ISO2022_unpackstate		/* unpackstate */
};

#define is94(x)		(0x21 <= (__uint8_t)(x) && (__uint8_t)(x) <= 0x7e)
#define is96(x)		(0x20 <= (__uint8_t)(x) && (__uint8_t)(x) <= 0x7f)
#define isecma(x)	(0x30 <= (__uint8_t)(x) && (__uint8_t)(x) <= 0x7f)
#define isinterm(x)	(0x20 <= (__uint8_t)(x) && (__uint8_t)(x) <= 0x2f)
#define	isthree(x)	(0x60 <= (__uint8_t)(x) && (__uint8_t)(x) <= 0x6f)

static int
getcs(p, cs)
	char *p;
	_Iso2022Charset *cs;
{
	if (!strncmp(p, "94$", 3) && p[3] && !p[4]) {
		cs->final = (u_char)(p[3] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS94MULTI;
		return 0;
	}
	if (!strncmp(p, "96$", 3) && p[3] && !p[4]) {
		cs->final = (u_char)(p[3] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS96MULTI;
		return 0;
	}
	if (!strncmp(p, "94", 2) && p[2] && !p[3]) {
		cs->final = (u_char)(p[2] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS94;
		return 0;
	}
	if (!strncmp(p, "96", 2) && p[2] && !p[3]) {
		cs->final = (u_char )(p[2] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS96;
		return 0;
	}

	return 1;
}

int
_ISO2022_init(rl)
	_RuneLocale *rl;
{
	_Iso2022Info *ii;
	char *v, *e;
	int i;
	int s;
	char *tags[] = {
"DUMMY","8BIT",	"NOOLD",
"SI",	"SO",	"LS0",	"LS1",	"LS2",	"LS3",
"LS1R",	"LS2R",	"LS3R",	"SS2",	"SS3",	"SS2R",	"SS3R",	NULL };
	int flags[] = {
0,	F_8BIT,	F_NOOLD,
F_SI,	F_SO,	F_LS0,	F_LS1,	F_LS2,	F_LS3,
F_LS1R,	F_LS2R,	F_LS3R,	F_SS2,	F_SS3,	F_SS2R,	F_SS3R,	0 };
	_Iso2022Charset cs;
	_Iso2022State *pst;

	rl->___sgetrune = _ISO2022_sgetrune;
	rl->___sputrune = _ISO2022_sputrune;

	/*
	 * parse VARIABLE section.
	 */
	if (!rl->__variable) {
		free(rl);
		return (EFTYPE);
	}
	v = (char *) rl->__variable;

	if ((ii = malloc(sizeof(_Iso2022Info))) == NULL) {
		free(rl);
		return (ENOMEM);
	}
	ii->maxcharset = 0;
	for (i = 0; i < 4; i++) {
		ii->recommend[i] = NULL;
		ii->recommendsize[i] = 0;
	}
	ii->flags = 0;

	while (*v) {
		while (*v == ' ' || *v == '\t')
			++v;

		/* find the token */
		e = v;
		while (*e && *e != ' ' && *e != '\t')
			++e;
		if (*e) {
			*e = '\0';
			++e;
		}

		if (strchr("0123", v[0]) && v[1] == '=') {
			if (getcs(&v[2], &cs) == 0)
				;
			else if (!strcmp(&v[2], "94")) {
				cs.final = (u_char)(v[4]);
				cs.interm = '\0';
				cs.vers = '\0';
				cs.type = CS94;
			} else if (!strcmp(&v[2], "96")) {
				cs.final = (u_char)(v[4]);
				cs.interm = '\0';
				cs.vers = '\0';
				cs.type = CS96;
			} else if (!strcmp(&v[2], "94$")) {
				cs.final = (u_char)(v[5]);
				cs.interm = '\0';
				cs.vers = '\0';
				cs.type = CS94MULTI;
			} else if (!strcmp(&v[2], "96$")) {
				cs.final = (u_char)(v[5]);
				cs.interm = '\0';
				cs.vers = '\0';
				cs.type = CS96MULTI;
			} else {
parsefail:
				free(rl);
				free(ii->recommend[0]);
				free(ii->recommend[1]);
				free(ii->recommend[2]);
				free(ii->recommend[3]);
				free(ii);
				return (EFTYPE);
			}

			i = v[0] - '0';
			ii->recommendsize[i] += 1;
			if (!ii->recommend[i]) {
				ii->recommend[i]
					= malloc(sizeof(_Iso2022Charset));
			} else {
				ii->recommend[i] = realloc(ii->recommend[i],
					sizeof(_Iso2022Charset) * (ii->recommendsize[i]));
			}
			if (!ii->recommend[i])
				goto parsefail;

			(ii->recommend[i] + (ii->recommendsize[i] - 1))->final
				= cs.final;
			(ii->recommend[i] + (ii->recommendsize[i] - 1))->interm
				= cs.interm;
			(ii->recommend[i] + (ii->recommendsize[i] - 1))->vers
				= cs.vers;
			(ii->recommend[i] + (ii->recommendsize[i] - 1))->type
				= cs.type;

okey:
			v = e;
			continue;
		} else if (!strncmp("INIT", &v[0], 4) && strchr("0123", v[4])
		 && v[5] == '=') {
			if (getcs(&v[6], &cs) != 0)
				goto parsefail;
			ii->initg[v[4] - '0'].type = cs.type;
			ii->initg[v[4] - '0'].final = cs.final;
			ii->initg[v[4] - '0'].interm = cs.interm;
			ii->initg[v[4] - '0'].vers = cs.vers;
			goto okey;
		} else if (!strcmp(v, "MAX1")) {
			ii->maxcharset = 1;
			goto okey;
		} else if (!strcmp(v, "MAX2")) {
			ii->maxcharset = 2;
			goto okey;
		} else if (!strcmp(v, "MAX3")) {
			ii->maxcharset = 3;
			goto okey;
		} else {
			for (i = 0; tags[i]; i++) {
				if (!strcmp(v, tags[i])) {
					ii->flags |= flags[i];
					goto okey;
				}
			}
		}

		goto parsefail;
	}

	s = sizeof(_Iso2022Info);
	for (i = 0; i < 4; i++)
		s += sizeof(_Iso2022Charset) * ii->recommendsize[i];
	if (s <= rl->__variable_len) {
		char *p;

		p = rl->__variable;
		memcpy(p, ii, sizeof(_Iso2022Info));
		p += sizeof(_Iso2022Info);

		for (i = 0; i < 4; i++) {
			if (!ii->recommend[i])
				continue;
			s = sizeof(_Iso2022Charset) * ii->recommendsize[i];
			memcpy(p, ii->recommend[i], s);
			ii->recommend[i] = (_Iso2022Charset *)p;
			p += s;
		}
	} else
		rl->__variable = ii;
	rl->__variable_len = s;

	_CurrentRuneLocale = rl;
	_CurrentRuneState = &_ISO2022_RuneState;
	__mb_cur_max = MB_LEN_MAX;

	s = FOPEN_MAX * sizeof(void *) + sizeof(_Iso2022State);
	_StreamStateTable = malloc(s);
	memset(_StreamStateTable, s, 0);
	pst = (_Iso2022State *)
		(((u_char *)_StreamStateTable) + (FOPEN_MAX * sizeof(void *)));
	for (i = 0; i < FOPEN_MAX; i++) {
		((void **)_StreamStateTable)[i] = &pst[i];
		(*__rune_initstate)(&pst[i]);
	}

	return (0);
}

#define	CEI	((_Iso2022Info *)(_CurrentRuneLocale->__variable))
#define	CES	((_Iso2022State *)(state))

_rune_t
_ISO2022_sgetrune(string, n, result, state)
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	_rune_t rune = 0;
	int singlegr, singlegl;
	int cur;

	singlegr = singlegl = -1;

	while (1) {
		/* G0 94MULTI special */
		if (3 <= n && string[0] == '\033' && string[1] == '$'
		 && strchr("@AB", string[2])) {
			CES->g[0].type = CS94MULTI;
			CES->g[0].final = string[2];
			CES->g[0].interm = '\0';
			CES->g[0].vers = '\0';
			string += 3;
			continue;
		}
		/* G0 94MULTI special with version specification */
		if (6 <= n && string[0] == '\033' && string[1] == '&'
		 && isecma(string[2]) && string[3] == '\033' && string[4] == '$'
		 && strchr("@AB", string[5])) {
			CES->g[0].type = CS94MULTI;
			CES->g[0].final = string[5];
			CES->g[0].interm = '\0';
			CES->g[0].vers = string[2];
			string += 6;
			continue;
		}
		/* G? 94 */
		if (3 <= n && string[0] == '\033'
		 && string[1] && strchr("()*+", string[1])
		 && isecma(string[2])) {
			CES->g[string[1] - '('].type = CS94;
			CES->g[string[1] - '('].final = string[2];
			CES->g[string[1] - '('].interm = '\0';
			CES->g[string[1] - '('].vers = '\0';
			string += 3;
			continue;
		}
		/* G? 94 with 2nd intermediate char */
		if (4 <= n && string[0] == '\033'
		 && string[1] && strchr("()*+", string[1])
		 && isinterm(string[2]) && isecma(string[3])) {
			CES->g[string[1] - '('].type = CS94;
			CES->g[string[1] - '('].final = string[3];
			CES->g[string[1] - '('].interm = string[2];
			CES->g[string[1] - '('].vers = '\0';
			string += 4;
			continue;
		}
		/* G? 96 */
		if (3 <= n && string[0] == '\033'
		 && string[1] && strchr(",-./", string[1])
		 && isecma(string[2])) {
			CES->g[string[1] - ','].type = CS96;
			CES->g[string[1] - ','].final = string[2];
			CES->g[string[1] - ','].interm = '\0';
			CES->g[string[1] - ','].vers = '\0';
			string += 3;
			continue;
		}
		/* G? 96 with 2nd intermediate char */
		if (4 <= n && string[0] == '\033'
		 && string[1] && strchr(",-./", string[1])
		 && isinterm(string[2]) && isecma(string[3])) {
			CES->g[string[1] - ','].type = CS96;
			CES->g[string[1] - ','].final = string[3];
			CES->g[string[1] - ','].interm = string[2];
			CES->g[string[1] - ','].vers = '\0';
			string += 4;
			continue;
		}
		/* G? 94MULTI */
		if (4 <= n && string[0] == '\033' && string[1] == '$'
		 && string[2] && strchr("()*+", string[2])
		 && isecma(string[3])) {
			CES->g[string[2] - '('].type = CS94MULTI;
			CES->g[string[2] - '('].final = string[3];
			CES->g[string[2] - '('].interm = '\0';
			CES->g[string[2] - '('].vers = '\0';
			string += 4;
			continue;
		}
		/* G? 96MULTI */
		if (4 <= n && string[0] == '\033' && string[1] == '$'
		 && string[2] && strchr(",-./", string[2])
		 && isecma(string[3])) {
			CES->g[string[2] - ','].type = CS96MULTI;
			CES->g[string[2] - ','].final = string[3];
			CES->g[string[2] - ','].interm = '\0';
			CES->g[string[2] - ','].vers = '\0';
			string += 4;
			continue;
		}
		/* G? 94MULTI with version specification */
		if (7 <= n && string[0] == '\033' && string[1] == '&'
		 && isecma(string[2]) && string[3] == '\033' && string[4] == '$'
		 && string[5] && strchr("()*+", string[5])
		 && isecma(string[6])) {
			CES->g[string[5] - '('].type = CS94MULTI;
			CES->g[string[5] - '('].final = string[6];
			CES->g[string[5] - '('].interm = '\0';
			CES->g[string[5] - '('].vers = string[2];
			string += 4;
			continue;
		}

		/* SI/SO */
		if (1 <= n && string[0] == '\017') {
			CES->gl = 0;
			string++;
			continue;
		}
		if (1 <= n && string[0] == '\016') {
			CES->gl = 1;
			string++;
			continue;
		}

		/* LS2/3 */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("no", string[1])) {
			CES->gl = string[1] - 'n' + 2;
			string += 2;
			continue;
		}

		/* LS1/2/3R */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("~}|", string[1])) {
			CES->gr = 3 - (string[1] - '|');
			string += 2;
			continue;
		}

		/* SS2/3 */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("NO", string[1])) {
			singlegl = (string[1] - 'N') + 2;
			string += 2;
			continue;
		}

		/* SS2/3R */
		if (1 <= n && string[0] && strchr("\217\216", string[0])) {
			singlegl = singlegr = (string[0] - '\216') + 2;
			string++;
			continue;
		}

		break;
	}

	/* normal chars. */
	if (*string & 0x80)
		cur = (singlegr == -1) ? CES->gr : singlegr;
	else
		cur = (singlegl == -1) ? CES->gl : singlegl;

	if (cur == -1) {
asis:
		rune = *string++ & 0xff;
		if (result)
			*result = string;
		return rune;
	}

	/* length error check */
	switch (CES->g[cur].type) {
	case CS94MULTI:
	case CS96MULTI:
		if (!isthree(CES->g[cur].final)) {
			if (2 <= n
			 && (string[0] & 0x80) == (string[1] & 0x80))
				break;
		} else {
			if (3 <= n
			 && (string[0] & 0x80) == (string[1] & 0x80)
			 && (string[0] & 0x80) == (string[2] & 0x80))
				break;
		}

		/* we still need to wait for more characters to come */
		if (result)
			*result = string;
		return (_INVALID_RUNE);

	case CS94:
	case CS96:
		if (1 <= n)
			break;

		/* we still need to wait for more characters to come */
		if (result)
			*result = string;
		return (_INVALID_RUNE);
	}

	/* range check */
	switch (CES->g[cur].type) {
	case CS94:
		if (!(is94(string[0] & 0x7f)))
			goto asis;
	case CS96:
		if (!(is96(string[0] & 0x7f)))
			goto asis;
		break;
	case CS94MULTI:
		if (!(is94(string[0] & 0x7f) && is94(string[1] & 0x7f)))
			goto asis;
		break;
	case CS96MULTI:
		if (!(is96(string[0] & 0x7f) && is96(string[1] & 0x7f)))
			goto asis;
		break;
	}

	/* extract the character. */
	switch (CES->g[cur].type) {
	case CS94:
		/* special case for ASCII. */
		if (CES->g[cur].final == 'B' && !CES->g[cur].interm) {
			rune = *string++;
			rune &= 0x7f;
			break;
		}
		rune = CES->g[cur].final;
		rune = (rune << 8);
		rune |= (CES->g[cur].interm ? (0x80 | CES->g[cur].interm) : 0);
		rune = (rune << 8);
		rune = (rune << 8) | (*string++ & 0x7f);
		break;
	case CS96:
		/* special case for ISO-8859-1. */
		if (CES->g[cur].final == 'A' && !CES->g[cur].interm) {
			rune = *string++;
			rune &= 0x7f;
			rune |= 0x80;
			break;
		}
		rune = CES->g[cur].final;
		rune = (rune << 8);
		rune |= (CES->g[cur].interm ? (0x80 | CES->g[cur].interm) : 0);
		rune = (rune << 8);
		rune = (rune << 8) | (*string++ & 0x7f);
		rune |= 0x80;
		break;
	case CS94MULTI:
	case CS96MULTI:
		rune = CES->g[cur].final;
		rune = (rune << 8);
		if (isthree(CES->g[cur].final))
			rune |= (*string++ & 0x7f);
		rune = (rune << 8) | (*string++ & 0x7f);
		rune = (rune << 8) | (*string++ & 0x7f);
		if (CES->g[cur].type == CS96MULTI)
			rune |= 0x80;
		break;
	}

	if (result)
		*result = string;
	return rune;
}

static int
recommendation(cs)
	_Iso2022Charset *cs;
{
	int i, j;
	_Iso2022Charset *recommend;

	/* first, try a exact match. */
	for (i = 0; i < 4; i++) {
		recommend = CEI->recommend[i];
		for (j = 0; j < CEI->recommendsize[i]; j++) {
			if (cs->type != recommend[j].type)
				continue;
			if (cs->final != recommend[j].final)
				continue;
			if (cs->interm != recommend[j].interm)
				continue;

			return i;
		}
	}

	/* then, try a wildcard match over final char. */
	for (i = 0; i < 4; i++) {
		recommend = CEI->recommend[i];
		for (j = 0; j < CEI->recommendsize[i]; j++) {
			if (cs->type != recommend[j].type)
				continue;
			if (cs->final
			 && (cs->final != recommend[j].final))
				continue;
			if (cs->interm
			 && (cs->interm != recommend[j].interm))
				continue;

			return i;
		}
	}

	/* there's no recommendation. make a guess. */
	if (CEI->maxcharset == 0) {
		return 0;
	} else {
		switch (cs->type) {
		case CS94:
		case CS94MULTI:
			return 0;
		case CS96:
		case CS96MULTI:
			return 1;
		}
	}
	return 0;
}

int
_ISO2022_sputrune(c, string, n, result, state)
	_rune_t c;
	char *string, **result;
	size_t n;
	void *state;
{
	int i = 0, len;
	_Iso2022Charset cs;
	char *p;
	char tmp[MB_LEN_MAX];
	int target;
	u_char mask;
	int singlegl, singlegr;
	int bit8;

	if (!(c & ~0xff)) {
		if (c & 0x80) {
			/* special treatment for ISO-8859-1 */
			cs.type = CS96;
			cs.final = 'A';
			cs.interm = '\0';
		} else {
			/* special treatment for ASCII */
			cs.type = CS94;
			cs.final = 'B';
			cs.interm = '\0';
		}
	} else {
		cs.final = (c >> 24) & 0x7f;
		if ((c >> 16) & 0x80)
			cs.interm = (c >> 16) & 0x7f;
		else
			cs.interm = '\0';
		if (c & 0x80)
			cs.type = (c & 0x00007f00) ? CS96MULTI : CS96;
		else
			cs.type = (c & 0x00007f00) ? CS94MULTI : CS94;
	}
	target = recommendation(&cs);
	p = tmp;
	singlegl = singlegr = -1;
	bit8 = CEI->flags & F_8BIT;

	/* designate the charset onto the target plane(G0/1/2/3). */
	if (CES->g[target].type == cs.type
	 && CES->g[target].final == cs.final
	 && CES->g[target].interm == cs.interm)
		goto planeok;

	*p++ = '\033';
	if (cs.type == CS94MULTI || cs.type == CS96MULTI)
		*p++ = '$';
	if (target == 0 && cs.type == CS94MULTI && strchr("@AB", cs.final)
	 && !cs.interm && !(CEI->flags & F_NOOLD))
		;
	else if (cs.type == CS94 || cs.type == CS94MULTI)
		*p++ = "()*+"[target];
	else
		*p++ = ",-./"[target];
	if (cs.interm)
		*p++ = cs.interm;
	*p++ = cs.final;

	CES->g[target].type = cs.type;
	CES->g[target].final = cs.final;
	CES->g[target].interm = cs.interm;

planeok:
	 
	/* invoke the plane onto GL or GR. */
	if (CES->gl == target)
		goto sideok;
	if (bit8 && CES->gr == target)
		goto sideok;

	if (target == 0 && (CEI->flags & F_LS0)) {
		*p++ = '\017';
		CES->gl = 0;
	} else if (target == 1 && (CEI->flags & F_LS1)) {
		*p++ = '\016';
		CES->gl = 1;
	} else if (target == 2 && (CEI->flags & F_LS2)) {
		*p++ = '\033';
		*p++ = 'n';
		CES->gl = 2;
	} else if (target == 3 && (CEI->flags & F_LS3)) {
		*p++ = '\033';
		*p++ = 'o';
		CES->gl = 3;
	} else if (bit8 && target == 1 && (CEI->flags & F_LS1R)) {
		*p++ = '\033';
		*p++ = '~';
		CES->gr = 1;
	} else if (bit8 && target == 2 && (CEI->flags & F_LS2R)) {
		*p++ = '\033';
		*p++ = '}';
		CES->gr = 2;
	} else if (bit8 && target == 3 && (CEI->flags & F_LS3R)) {
		*p++ = '\033';
		*p++ = '|';
		CES->gr = 3;
	} else if (target == 2 && (CEI->flags & F_SS2)) {
		*p++ = '\033';
		*p++ = 'N';
		singlegl = 2;
	} else if (target == 3 && (CEI->flags & F_SS3)) {
		*p++ = '\033';
		*p++ = 'O';
		singlegl = 3;
	} else if (bit8 && target == 2 && (CEI->flags & F_SS2R)) {
		*p++ = '\216';
		*p++ = 'N';
		singlegl = singlegr = 2;
	} else if (bit8 && target == 3 && (CEI->flags & F_SS3R)) {
		*p++ = '\217';
		*p++ = 'O';
		singlegl = singlegr = 3;
	} else
		abort();

sideok:
	if (singlegl == target)
		mask = 0x00;
	else if (singlegr == target)
		mask = 0x80;
	else if (CES->gl == target)
		mask = 0x00;
	else if ((CEI->flags & F_8BIT) && CES->gr == target)
		mask = 0x80;
	else
		abort();

	switch (cs.type) {
	case CS94:
	case CS96:
		i = 1;
		break;
	case CS94MULTI:
	case CS96MULTI:
		i = isthree(cs.final) ? 3 : 2;
		break;
	}
	while (i-- > 0)
		*p++ = ((c >> (i << 3)) & 0x7f) | mask;

	len = p - tmp;
	if (n < len) {
		if (result)
			*result = (char *)0;
	} else {
		if (result)
			*result = string + len;
		memcpy(string, tmp, len);
	}
	return len;
}

void
_ISO2022_initstate(s)
	void *s;
{
	_Iso2022State *state;
	size_t i;

	if (!s)
		return;
	state = s;
	memset(state, sizeof(_Iso2022State), 0);

	state->gl = 0;
	state->gr = (CEI->flags & F_8BIT) ? 1 : -1;

	for (i = 0; i < 4; i++) {
		if (CEI->initg[i].final) {
			CES->g[i].type = CEI->initg[i].type;
			CES->g[i].final = CEI->initg[i].final;
			CES->g[i].interm = CEI->initg[i].interm;
		}
	}
}

void
_ISO2022_packstate(dst, src)
	mbstate_t *dst;
	void* src;
{
	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_Iso2022State));
	return;
#if 0
	_Iso2022State *state;
	mbstate_t t;

	if (!(state = src)) {
		*dst = 0;
		return;
	}

	t = 0;

#if 0
	/* 7x4 + 7x4 + 2x4 + 2x2 = 68 bits... */
	t = (state->g[0].final & 0x7f);	t = (t << 7);
	t |= (state->g[1].final & 0x7f);	t = (t << 7);
	t |= (state->g[2].final & 0x7f);	t = (t << 7);
	t |= (state->g[3].final & 0x7f);	t = (t << 7);
	t |= (state->g[0].interm & 0x7f);	t = (t << 7);
	t |= (state->g[1].interm & 0x7f);	t = (t << 7);
	t |= (state->g[2].interm & 0x7f);	t = (t << 7);
	t |= (state->g[3].interm & 0x7f);	t = (t << 7);
	t |= (state->g[0].type & 0x03);	t = (t << 2);
	t |= (state->g[1].type & 0x03);	t = (t << 2);
	t |= (state->g[2].type & 0x03);	t = (t << 2);
	t |= (state->g[3].type & 0x03);	t = (t << 2);
	t |= (state->gl & 0x03);	t = (t << 2);
	t |= (state->gr & 0x03);
#else
	/* 7x4 + 4x4 + 2x4 + 2x2 = 56 bits */
	t = (state->g[0].final & 0x7f);	t = (t << 7);
	t = (state->g[1].final & 0x7f);	t = (t << 7);
	t = (state->g[2].final & 0x7f);	t = (t << 7);
	t = (state->g[3].final & 0x7f);	t = (t << 7);
	t = (state->g[0].interm & 0x0f);	t = (t << 4);
	t = (state->g[1].interm & 0x0f);	t = (t << 4);
	t = (state->g[2].interm & 0x0f);	t = (t << 4);
	t = (state->g[3].interm & 0x0f);	t = (t << 4);
	t = (state->g[0].type & 0x03);	t = (t << 2);
	t = (state->g[1].type & 0x03);	t = (t << 2);
	t = (state->g[2].type & 0x03);	t = (t << 2);
	t = (state->g[3].type & 0x03);	t = (t << 2);
	t = (state->gl & 0x03);	t = (t << 2);
	t = (state->gr & 0x03);
#endif

	*dst = t;
#endif
}

void
_ISO2022_unpackstate(dst, src)
	void* dst;
	const mbstate_t *src;
{
	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_Iso2022State));
	return;
#if 0
	_Iso2022State *state;
	mbstate_t t;

	state = (_Iso2022State *)dst;
	t = *src;
#if 0
	state->gr = t & 0x03;	t = (t >> 2);
	state->gl = t & 0x03;	t = (t >> 2);
	state->g[3].type = t & 0x03;	t = (t >> 2);
	state->g[2].type = t & 0x03;	t = (t >> 2);
	state->g[1].type = t & 0x03;	t = (t >> 2);
	state->g[0].type = t & 0x03;	t = (t >> 2);
	state->g[3].interm = t & 0x7f;	t = (t >> 7);
	state->g[2].interm = t & 0x7f;	t = (t >> 7);
	state->g[1].interm = t & 0x7f;	t = (t >> 7);
	state->g[0].interm = t & 0x7f;	t = (t >> 7);
	state->g[3].final = t & 0x7f;	t = (t >> 7);
	state->g[2].final = t & 0x7f;	t = (t >> 7);
	state->g[1].final = t & 0x7f;	t = (t >> 7);
	state->g[0].final = t & 0x7f;	t = (t >> 7);
#else
	state->gr = t & 0x03;	t = (t >> 2);
	state->gl = t & 0x03;	t = (t >> 2);
	state->g[3].type = t & 0x03;	t = (t >> 2);
	state->g[2].type = t & 0x03;	t = (t >> 2);
	state->g[1].type = t & 0x03;	t = (t >> 2);
	state->g[0].type = t & 0x03;	t = (t >> 2);
	state->g[3].interm = (t & 0x0f) + 0x20;	t = (t >> 4);
	state->g[2].interm = (t & 0x0f) + 0x20;	t = (t >> 4);
	state->g[1].interm = (t & 0x0f) + 0x20;	t = (t >> 4);
	state->g[0].interm = (t & 0x0f) + 0x20;	t = (t >> 4);
	state->g[3].final = t & 0x7f;	t = (t >> 7);
	state->g[2].final = t & 0x7f;	t = (t >> 7);
	state->g[1].final = t & 0x7f;	t = (t >> 7);
	state->g[0].final = t & 0x7f;	t = (t >> 7);
#endif
#endif
}
#endif  /* XPG4 || DLRUNEMOD */
