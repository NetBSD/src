/*	$NetBSD: iso2022.c,v 1.11.2.1 2001/10/08 20:19:49 nathanw Exp $	*/

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
 * OR SERVICES(rl); LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Citrus: xpg4dl/FreeBSD/lib/libc/locale/iso2022.c,v 1.23 2001/06/21 01:51:44 yamt Exp $
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: iso2022.c,v 1.11.2.1 2001/10/08 20:19:49 nathanw Exp $");
#endif /* LIBC_SCCS and not lint */

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

#include <assert.h>
#include <errno.h>
#include "rune.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <machine/int_types.h>

#include "iso2022.h"

static int getcs __P((char *, _Iso2022Charset *));
const char *_ISO2022_magic __P((void));
int _ISO2022_init __P((_RuneLocale *));
int _ISO2022_init_stream __P((_RuneLocale *));
struct seqtable;
static int seqmatch __P((const char *, size_t, const struct seqtable *));
static rune_t _ISO2022_sgetrune __P((_RuneLocale *, const char *, size_t,
	char const **, void *));
static int recommendation __P((_RuneLocale *, _Iso2022Charset *));
static int _ISO2022_sputrune __P((_RuneLocale *, rune_t, char *, size_t,
	char **, void *));
size_t	_ISO2022_mbrtowc __P((struct _RuneLocale *, rune_t *, const char *,
	size_t, void *));
size_t	_ISO2022_wcrtomb __P((struct _RuneLocale *, char *, size_t,
	const rune_t, void *));
void _ISO2022_initstate __P((_RuneLocale *, void *));
void _ISO2022_packstate __P((_RuneLocale *, mbstate_t *, void *));
void _ISO2022_unpackstate __P((_RuneLocale *, void *, const mbstate_t *));

typedef struct {
	_Iso2022Charset	*recommend[4];
	size_t	recommendsize[4];
	_Iso2022Charset	initg[4];
	int	maxcharset;
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
	void *runelocale;	/* reserved for future thread-safeness */
	_Iso2022Charset	g[4];
	/* need 3 bits to hold -1, 0, ..., 3 */
	int	gl:3,
		gr:3,
		singlegl:3,
		singlegr:3;
	char ch[7];	/* longest escape sequence (ESC & V ESC $ ( F) */
	int chlen;
} _Iso2022State __attribute__((__packed__));

static _RuneState _ISO2022_RuneState = {
	sizeof(_Iso2022State),		/* sizestate */
	_ISO2022_initstate,		/* initstate */
	_ISO2022_packstate,		/* packstate */
	_ISO2022_unpackstate		/* unpackstate */
};

#define isc0(x)		(((x) & 0x1f) == ((x) & 0xff))
#define isc1(x)		(0x80 <= (__uint8_t)(x) && (__uint8_t)(x) <= 0x9f)
#define	iscntl(x)	(isc0((x)) || isc1((x)) || (x) == 0x7f)
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

	_DIAGASSERT(p != NULL);
	_DIAGASSERT(cs != NULL);

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

const char *
_ISO2022_magic()
{

	return _RUNE_MODULE_1("LC_CTYPE");
}

int
_ISO2022_init(rl)
	_RuneLocale *rl;
{
	_Iso2022Info *ii;
	char *v, *e;
	int i;
	size_t s;
	const char * const tags[] = {
"DUMMY","8BIT",	"NOOLD",
"SI",	"SO",	"LS0",	"LS1",	"LS2",	"LS3",
"LS1R",	"LS2R",	"LS3R",	"SS2",	"SS3",	"SS2R",	"SS3R",	NULL };
	const int flags[] = {
0,	F_8BIT,	F_NOOLD,
F_SI,	F_SO,	F_LS0,	F_LS1,	F_LS2,	F_LS3,
F_LS1R,	F_LS2R,	F_LS3R,	F_SS2,	F_SS3,	F_SS2R,	F_SS3R,	0 };
	_Iso2022Charset cs;

	_DIAGASSERT(rl != NULL);

	/* sanity check to avoid overruns */
	if (sizeof(_Iso2022State) > sizeof(mbstate_t))
		return (EINVAL);

	rl->__rune_mbrtowc = _ISO2022_mbrtowc;
	rl->__rune_wcrtomb = _ISO2022_wcrtomb;

	/*
	 * parse VARIABLE section.
	 */
	if (!rl->__rune_variable)
		return (EFTYPE);
	v = (char *) rl->__rune_variable;

	if ((ii = malloc(sizeof(_Iso2022Info))) == NULL)
		return (ENOMEM);
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

		p = rl->__rune_variable;
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
		rl->__rune_variable = ii;
	rl->__variable_len = s;

	rl->__rune_RuneState = &_ISO2022_RuneState;
	rl->__rune_mb_cur_max = MB_LEN_MAX;

	return 0;
}

int
_ISO2022_init_stream(rl)
	_RuneLocale *rl;
{
#if 0
	int i;
	size_t s;
	_Iso2022State *pst;

	_DIAGASSERT(rl != NULL);

	s = FOPEN_MAX * 2 * (sizeof(void *) + sizeof(_Iso2022State));
	_StreamStateTable = malloc(s);
	if (!_StreamStateTable)
		return ENOBUFS;
	memset(_StreamStateTable, s, 0);
	pst = (_Iso2022State *)
		(((u_char *)_StreamStateTable) + (FOPEN_MAX * sizeof(void *)));
	for (i = 0; i < FOPEN_MAX; i++) {
		((void **)_StreamStateTable)[i] = &pst[i];
		(*___rune_initstate(rl))(rl, &pst[i]);
	}
#endif

	return (0);
}

#define	CEI(rl)	((_Iso2022Info *)((rl)->__rune_variable))
#define	CES(rl)	((_Iso2022State *)(state))

#define	ESC	'\033'
#define	ECMA	-1
#define	INTERM	-2
#define	OECMA	-3
static struct seqtable {
	int type;
	int csoff;
	int finaloff;
	int intermoff;
	int versoff;
	int len;
	int chars[10];
} seqtable[] = {
	/* G0 94MULTI special */
	{ CS94MULTI, -1, 2, -1, -1,	3, { ESC, '$', OECMA }, },
	/* G0 94MULTI special with version identification */
	{ CS94MULTI, -1, 5, -1, 2,	6, { ESC, '&', ECMA, ESC, '$', OECMA }, },
	/* G? 94 */
	{ CS94, 1, 2, -1, -1,		3, { ESC, CS94, ECMA, }, },
	/* G? 94 with 2nd intermediate char */
	{ CS94, 1, 3, 2, -1,		4, { ESC, CS94, INTERM, ECMA, }, },
	/* G? 96 */
	{ CS96, 1, 2, -1, -1,		3, { ESC, CS96, ECMA, }, },
	/* G? 96 with 2nd intermediate char */
	{ CS96, 1, 3, 2, -1,		4, { ESC, CS96, INTERM, ECMA, }, },
	/* G? 94MULTI */
	{ CS94MULTI, 2, 3, -1, -1,	4, { ESC, '$', CS94, ECMA, }, },
	/* G? 96MULTI */
	{ CS96MULTI, 2, 3, -1, -1,	4, { ESC, '$', CS96, ECMA, }, },
	/* G? 94MULTI with version specification */
	{ CS94MULTI, 5, 6, -1, 2,	7, { ESC, '&', ECMA, ESC, '$', CS94, ECMA, }, },
	/* LS2/3 */
	{ -1, -1, -1, -1, -1,		2, { ESC, 'n', }, },
	{ -1, -1, -1, -1, -1,		2, { ESC, 'o', }, },
	/* LS1/2/3R */
	{ -1, -1, -1, -1, -1,		2, { ESC, '~', }, },
	{ -1, -1, -1, -1, -1,		2, { ESC, /*{*/ '}', }, },
	{ -1, -1, -1, -1, -1,		2, { ESC, '|', }, },
	/* SS2/3 */
	{ -1, -1, -1, -1, -1,		2, { ESC, 'N', }, },
	{ -1, -1, -1, -1, -1,		2, { ESC, 'O', }, },
	/* end of records */
	{ 0, }
};

static int
seqmatch(s, n, sp)
	const char *s;
	size_t n;
	const struct seqtable *sp;
{
	const int *p;

	_DIAGASSERT(s != NULL);
	_DIAGASSERT(sp != NULL);

	p = sp->chars;
	while (p - sp->chars < n && p - sp->chars < sp->len) {
		switch (*p) {
		case ECMA:
			if (!isecma(*s))
				goto terminate;
			break;
		case OECMA:
			if (*s && strchr("@AB", *s))
				break;
			else
				goto terminate;
		case INTERM:
			if (!isinterm(*s))
				goto terminate;
			break;
		case CS94:
			if (*s && strchr("()*+", *s))
				break;
			else
				goto terminate;
		case CS96:
			if (*s && strchr(",-./", *s))
				break;
			else
				goto terminate;
		default:
			if (*s != *p)
				goto terminate;
			break;
		}

		p++;
		s++;
	}

terminate:
	return p - sp->chars;
}

static rune_t
_ISO2022_sgetrune(rl, string, n, result, state)
	_RuneLocale *rl;
	const char *string;
	size_t n;
	char const **result;
	void *state;
{
	rune_t rune = 0;
	int cur;
	struct seqtable *sp;
	int nmatch;
	int i;

	_DIAGASSERT(rl != NULL);
	_DIAGASSERT(string != NULL);
	/* result may be NULL */

	while (1) {
		/* SI/SO */
		if (1 <= n && string[0] == '\017') {
			CES(rl)->gl = 0;
			string++;
			n--;
			continue;
		}
		if (1 <= n && string[0] == '\016') {
			CES(rl)->gl = 1;
			string++;
			n--;
			continue;
		}

		/* SS2/3R */
		if (1 <= n && string[0] && strchr("\217\216", string[0])) {
			CES(rl)->singlegl = CES(rl)->singlegr =
			    (string[0] - '\216') + 2;
			string++;
			n--;
			continue;
		}

		/* eat the letter if this is not ESC */
		if (1 <= n && string[0] != '\033')
			break;

		/* look for a perfect match from escape sequences */
		for (sp = &seqtable[0]; sp->len; sp++) {
			nmatch = seqmatch(string, n, sp);
			if (sp->len == nmatch && n >= sp->len)
				break;
		}

		if (!sp->len)
			goto notseq;

		if (sp->type != -1) {
			if (sp->csoff == -1)
				i = 0;
			else {
				switch (sp->type) {
				case CS94:
				case CS94MULTI:
					i = string[sp->csoff] - '(';
					break;
				case CS96:
				case CS96MULTI:
					i = string[sp->csoff] - ',';
					break;
				}
			}
			CES(rl)->g[i].type = sp->type;
			CES(rl)->g[i].final = '\0';
			CES(rl)->g[i].interm = '\0';
			CES(rl)->g[i].vers = '\0';
			/* sp->finaloff must not be -1 */
			if (sp->finaloff != -1)
				CES(rl)->g[i].final = string[sp->finaloff];
			if (sp->intermoff != -1)
				CES(rl)->g[i].interm = string[sp->intermoff];
			if (sp->versoff != -1)
				CES(rl)->g[i].vers = string[sp->versoff];

			string += sp->len;
			n -= sp->len;
			continue;
		}

		/* LS2/3 */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("no", string[1])) {
			CES(rl)->gl = string[1] - 'n' + 2;
			string += 2;
			n -= 2;
			continue;
		}

		/* LS1/2/3R */
			/* XXX: { for vi showmatch */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("~}|", string[1])) {
			CES(rl)->gr = 3 - (string[1] - '|');
			string += 2;
			n -= 2;
			continue;
		}

		/* SS2/3 */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("NO", string[1])) {
			CES(rl)->singlegl = (string[1] - 'N') + 2;
			string += 2;
			n -= 2;
			continue;
		}

	notseq:
		/*
		 * if we've got an unknown escape sequence, eat the ESC at the
		 * head.  otherwise, wait till full escape sequence comes.
		 */
		for (sp = &seqtable[0]; sp->len; sp++) {
			nmatch = seqmatch(string, n, sp);
			if (!nmatch)
				continue;

			/*
			 * if we are in the middle of escape sequence,
			 * we still need to wait for more characters to come
			 */
			if (n < sp->len) {
				if (nmatch == n) {
					if (result)
						*result = string;
					return (___INVALID_RUNE(rl));
				}
			} else {
				if (nmatch == sp->len) {
					/* this case should not happen */
					goto eat;
				}
			}
		}

		break;
	}

eat:
	/* no letter to eat */
	if (n < 1) {
		if (result)
			*result = string;
		return (___INVALID_RUNE(rl));
	}

	/* normal chars.  always eat C0/C1 as is. */
	if (iscntl(*string & 0xff))
		cur = -1;
	else if (*string & 0x80) {
		cur = (CES(rl)->singlegr == -1)
			? CES(rl)->gr : CES(rl)->singlegr;
	} else {
		cur = (CES(rl)->singlegl == -1)
			? CES(rl)->gl : CES(rl)->singlegl;
	}

	if (cur == -1) {
asis:
		rune = *string++ & 0xff;
		if (result)
			*result = string;
		/* reset single shift state */
		CES(rl)->singlegr = CES(rl)->singlegl = -1;
		return rune;
	}

	/* length error check */
	switch (CES(rl)->g[cur].type) {
	case CS94MULTI:
	case CS96MULTI:
		if (!isthree(CES(rl)->g[cur].final)) {
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
		return (___INVALID_RUNE(rl));

	case CS94:
	case CS96:
		if (1 <= n)
			break;

		/* we still need to wait for more characters to come */
		if (result)
			*result = string;
		return (___INVALID_RUNE(rl));
	}

	/* range check */
	switch (CES(rl)->g[cur].type) {
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
	switch (CES(rl)->g[cur].type) {
	case CS94:
		/* special case for ASCII. */
		if (CES(rl)->g[cur].final == 'B' && !CES(rl)->g[cur].interm) {
			rune = *string++;
			rune &= 0x7f;
			break;
		}
		rune = CES(rl)->g[cur].final;
		rune = (rune << 8);
		rune |= (CES(rl)->g[cur].interm ? (0x80 | CES(rl)->g[cur].interm) : 0);
		rune = (rune << 8);
		rune = (rune << 8) | (*string++ & 0x7f);
		break;
	case CS96:
		/* special case for ISO-8859-1. */
		if (CES(rl)->g[cur].final == 'A' && !CES(rl)->g[cur].interm) {
			rune = *string++;
			rune &= 0x7f;
			rune |= 0x80;
			break;
		}
		rune = CES(rl)->g[cur].final;
		rune = (rune << 8);
		rune |= (CES(rl)->g[cur].interm ? (0x80 | CES(rl)->g[cur].interm) : 0);
		rune = (rune << 8);
		rune = (rune << 8) | (*string++ & 0x7f);
		rune |= 0x80;
		break;
	case CS94MULTI:
	case CS96MULTI:
		rune = CES(rl)->g[cur].final;
		rune = (rune << 8);
		if (isthree(CES(rl)->g[cur].final))
			rune |= (*string++ & 0x7f);
		rune = (rune << 8) | (*string++ & 0x7f);
		rune = (rune << 8) | (*string++ & 0x7f);
		if (CES(rl)->g[cur].type == CS96MULTI)
			rune |= 0x80;
		break;
	}

	if (result)
		*result = string;
	/* reset single shift state */
	CES(rl)->singlegr = CES(rl)->singlegl = -1;
	return rune;
}

static int
recommendation(rl, cs)
	_RuneLocale *rl;
	_Iso2022Charset *cs;
{
	int i, j;
	_Iso2022Charset *recommend;

	_DIAGASSERT(rl != NULL);
	_DIAGASSERT(cs != NULL);

	/* first, try a exact match. */
	for (i = 0; i < 4; i++) {
		recommend = CEI(rl)->recommend[i];
		for (j = 0; j < CEI(rl)->recommendsize[i]; j++) {
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
		recommend = CEI(rl)->recommend[i];
		for (j = 0; j < CEI(rl)->recommendsize[i]; j++) {
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
	if (CEI(rl)->maxcharset == 0) {
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

static int
_ISO2022_sputrune(rl, c, string, n, result, state)
	_RuneLocale *rl;
	rune_t c;
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
	int bit8;

	_DIAGASSERT(rl != NULL);
	_DIAGASSERT(string != NULL);
	/* result may be NULL */
	/* state appears to be unused */

	if (iscntl(c & 0xff)) {
		/* go back to ASCII on control chars */
		cs.type = CS94;
		cs.final = 'B';
		cs.interm = '\0';
	} else if (!(c & ~0xff)) {
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
	target = recommendation(rl, &cs);
	p = tmp;
	bit8 = CEI(rl)->flags & F_8BIT;

	/* designate the charset onto the target plane(G0/1/2/3). */
	if (CES(rl)->g[target].type == cs.type
	 && CES(rl)->g[target].final == cs.final
	 && CES(rl)->g[target].interm == cs.interm)
		goto planeok;

	*p++ = '\033';
	if (cs.type == CS94MULTI || cs.type == CS96MULTI)
		*p++ = '$';
	if (target == 0 && cs.type == CS94MULTI && strchr("@AB", cs.final)
	 && !cs.interm && !(CEI(rl)->flags & F_NOOLD))
		;
	else if (cs.type == CS94 || cs.type == CS94MULTI)
		*p++ = "()*+"[target];
	else
		*p++ = ",-./"[target];
	if (cs.interm)
		*p++ = cs.interm;
	*p++ = cs.final;

	CES(rl)->g[target].type = cs.type;
	CES(rl)->g[target].final = cs.final;
	CES(rl)->g[target].interm = cs.interm;

planeok:
	 
	/* invoke the plane onto GL or GR. */
	if (CES(rl)->gl == target)
		goto sideok;
	if (bit8 && CES(rl)->gr == target)
		goto sideok;

	if (target == 0 && (CEI(rl)->flags & F_LS0)) {
		*p++ = '\017';
		CES(rl)->gl = 0;
	} else if (target == 1 && (CEI(rl)->flags & F_LS1)) {
		*p++ = '\016';
		CES(rl)->gl = 1;
	} else if (target == 2 && (CEI(rl)->flags & F_LS2)) {
		*p++ = '\033';
		*p++ = 'n';
		CES(rl)->gl = 2;
	} else if (target == 3 && (CEI(rl)->flags & F_LS3)) {
		*p++ = '\033';
		*p++ = 'o';
		CES(rl)->gl = 3;
	} else if (bit8 && target == 1 && (CEI(rl)->flags & F_LS1R)) {
		*p++ = '\033';
		*p++ = '~';
		CES(rl)->gr = 1;
	} else if (bit8 && target == 2 && (CEI(rl)->flags & F_LS2R)) {
		*p++ = '\033';
		/*{*/
		*p++ = '}';
		CES(rl)->gr = 2;
	} else if (bit8 && target == 3 && (CEI(rl)->flags & F_LS3R)) {
		*p++ = '\033';
		*p++ = '|';
		CES(rl)->gr = 3;
	} else if (target == 2 && (CEI(rl)->flags & F_SS2)) {
		*p++ = '\033';
		*p++ = 'N';
		CES(rl)->singlegl = 2;
	} else if (target == 3 && (CEI(rl)->flags & F_SS3)) {
		*p++ = '\033';
		*p++ = 'O';
		CES(rl)->singlegl = 3;
	} else if (bit8 && target == 2 && (CEI(rl)->flags & F_SS2R)) {
		*p++ = '\216';
		*p++ = 'N';
		CES(rl)->singlegl = CES(rl)->singlegr = 2;
	} else if (bit8 && target == 3 && (CEI(rl)->flags & F_SS3R)) {
		*p++ = '\217';
		*p++ = 'O';
		CES(rl)->singlegl = CES(rl)->singlegr = 3;
	} else
		abort();

sideok:
	if (CES(rl)->singlegl == target)
		mask = 0x00;
	else if (CES(rl)->singlegr == target)
		mask = 0x80;
	else if (CES(rl)->gl == target)
		mask = 0x00;
	else if ((CEI(rl)->flags & F_8BIT) && CES(rl)->gr == target)
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

	/* reset single shift state */
	CES(rl)->singlegl = CES(rl)->singlegr = -1;

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

/* s is non-null */
size_t
_ISO2022_mbrtowc(rl, pwcs, s, n, state)
	_RuneLocale *rl;
	rune_t *pwcs;
	const char *s;
	size_t n;
	void *state;
{
	_Iso2022State *ps;
	rune_t rune;
	const char *p, *result;
	int c;
	int chlenbak;

	_DIAGASSERT(rl != NULL);
	/* pwcs may be NULL */
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(state != NULL);

	ps = state;
	c = 0;
	chlenbak = ps->chlen;

	/*
	 * if we have something in buffer, use that.
	 * otherwise, skip here
	 */
	if (ps->chlen < 0 || ps->chlen > sizeof(ps->ch)) {
		/* illgeal state */
		goto encoding_error;
	}
	if (ps->chlen == 0)
		goto nobuf;
	p = ps->ch;
	while (ps->chlen < sizeof(ps->ch) && n >= 0) {
		if (n > 0) {
			ps->ch[ps->chlen++] = *s++;
			n--;
		}

		rune = _ISO2022_sgetrune(rl, p, ps->chlen - (p - ps->ch),
		    &result, state);
		if (rune != _INVALID_RUNE) {
			c += result - p;
			if (ps->chlen > c)
				memmove(ps->ch, result, ps->chlen - c);
			if (ps->chlen < c)
				ps->chlen = 0;
			else
				ps->chlen -= c;
			goto output;
		}

		c += result - p;
		p = result;

		if (n == 0)
			return (size_t)-2;
	}

	/* escape sequence too long? */
	goto encoding_error;

nobuf:
	rune = _ISO2022_sgetrune(rl, s, n, &result, state);
	if (rune != _INVALID_RUNE) {
		c += result - s;
		ps->chlen = 0;
		goto output;
	}
	if (result > s && n > result - s) {
		c += (result - s);
		n -= (result - s);
		s = result;
		goto nobuf;
	}
	n += c;
	if (n < sizeof(ps->ch)) {
		memcpy(ps->ch, s - c, n);
		ps->chlen = n;
		return (size_t)-2;
	}


	/* escape sequence too long? */

encoding_error:
	ps->chlen = 0;
	return (size_t)-1;

output:
	if (pwcs)
		*pwcs = rune;
	if (!rune)
		return 0;
	else
		return c - chlenbak;
}

/* s is non-null */
size_t
_ISO2022_wcrtomb(rl, s, n, wc, state)
        _RuneLocale *rl;
        char *s;
	size_t n;
        const rune_t wc;
        void *state;
{
	char buf[MB_LEN_MAX];
	char *result;
	int len;

	_DIAGASSERT(rl != NULL);
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(state != NULL);

	/* XXX state will be modified after this operation... */
	len = _ISO2022_sputrune(rl, wc, buf, sizeof(buf), &result, state);
	if (sizeof(buf) < len || n < len) {
		/* XXX should recover state? */
		goto notenough;
	}

	memcpy(s, buf, len);
	return len;

notenough:
	/* bound check failure */
	errno = EILSEQ;	/*XXX*/
	return (size_t)-1;
}

void
_ISO2022_initstate(rl, s)
	_RuneLocale *rl;
	void *s;
{
	_Iso2022State *state;
	size_t i;

	_DIAGASSERT(rl != NULL);

	if (!s)
		return;
	state = s;
	memset(state, 0, sizeof(_Iso2022State));

	state->gl = 0;
	state->gr = (CEI(rl)->flags & F_8BIT) ? 1 : -1;

	for (i = 0; i < 4; i++) {
		if (CEI(rl)->initg[i].final) {
			CES(rl)->g[i].type = CEI(rl)->initg[i].type;
			CES(rl)->g[i].final = CEI(rl)->initg[i].final;
			CES(rl)->g[i].interm = CEI(rl)->initg[i].interm;
		}
	}
	CES(rl)->singlegl = CES(rl)->singlegr = -1;
}

void
_ISO2022_packstate(rl, dst, src)
	_RuneLocale *rl;
	mbstate_t *dst;
	void* src;
{

	/* rl appears to be unused */
	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_Iso2022State));
	return;
}

void
_ISO2022_unpackstate(rl, dst, src)
	_RuneLocale *rl;
	void* dst;
	const mbstate_t *src;
{

	/* rl appears to be unused */
	_DIAGASSERT(dst != NULL);
	_DIAGASSERT(src != NULL);

	memcpy((caddr_t)dst, (caddr_t)src, sizeof(_Iso2022State));
	return;
}
