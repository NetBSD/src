/*	$NetBSD: citrus_iso2022.c,v 1.1 2002/03/17 22:14:23 tshiozak Exp $	*/

/*-
 * Copyright (c)1999, 2002 Citrus Project,
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
 *	$Citrus: xpg4dl/FreeBSD/lib/libc/locale/iso2022.c,v 1.23 2001/06/21 01:51:44 yamt Exp $
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_iso2022.c,v 1.1 2002/03/17 22:14:23 tshiozak Exp $");
#endif /* LIBC_SCCS and not lint */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <locale.h>
#include <wchar.h>
#include <sys/types.h>
#include <limits.h>
#include "citrus_module.h"
#include "citrus_ctype.h"
#include "citrus_iso2022.h"


/* ----------------------------------------------------------------------
 * private stuffs used by templates
 */


/*
 * wchar_t mappings:
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

typedef struct {
	u_char	type;
#define	CS94		(0U)
#define	CS96		(1U)
#define	CS94MULTI	(2U)
#define	CS96MULTI	(3U)

	u_char	final;
	u_char	interm;
	u_char	vers;
} _ISO2022Charset;

typedef struct {
	_ISO2022Charset	g[4];
	/* need 3 bits to hold -1, 0, ..., 3 */
	int	gl:3,
		gr:3,
		singlegl:3,
		singlegr:3;
	char ch[7];	/* longest escape sequence (ESC & V ESC $ ( F) */
	int chlen;
} _ISO2022State __attribute__((__packed__));

typedef struct {
	_ISO2022Charset	*recommend[4];
	size_t	recommendsize[4];
	_ISO2022Charset	initg[4];
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
} _ISO2022EncodingInfo;
typedef struct {
	_ISO2022EncodingInfo ei;
	struct {
		/* for future multi-locale facility */
		_ISO2022State	s_mblen;
		_ISO2022State	s_mbrlen;
		_ISO2022State	s_mbrtowc;
		_ISO2022State	s_mbtowc;
		_ISO2022State	s_mbsrtowcs;
		_ISO2022State	s_wcrtomb;
		_ISO2022State	s_wcsrtombs;
		_ISO2022State	s_wcstombs;
		_ISO2022State	s_wctomb;
	} states;
} _ISO2022CTypeInfo;

#define	_TO_EI(_cl_)			((_ISO2022EncodingInfo *)(_cl_))
#define	_TO_CEI(_cl_)			((_ISO2022CTypeInfo *)(_cl_))
#define _TO_STATE(_ps_)			((_ISO2022State *)(_ps_))
#define _CEI_TO_EI(_cei_)		(&(_cei_)->ei)
#define _CEI_TO_STATE(_cei_, _func_)	(_cei_)->states.s_##_func_

#define _FUNCNAME(m)			_citrus_ISO2022_##m
#define _ENCODING_INFO			_ISO2022EncodingInfo
#define _CTYPE_INFO			_ISO2022CTypeInfo
#define _ENCODING_STATE			_ISO2022State
#define _ENCODING_MB_CUR_MAX(_cl_)	MB_LEN_MAX
#define _ENCODING_IS_STATE_DEPENDENT	1


#define _ISO2022INVALID (wchar_t)-1

static __inline int isc0(__uint8_t x) { return ((x & 0x1f) == x); }
static __inline int isc1(__uint8_t x) { return (0x80 <= x && x <= 0x9f); }
static __inline int iscntl(__uint8_t x) { return (isc0(x) || isc1(x) || x == 0x7f); }
static __inline int is94(__uint8_t x) { return (0x21 <= x && x <= 0x7e); }
static __inline int is96(__uint8_t x) { return (0x20 <= x && x <= 0x7f); }
static __inline int isecma(__uint8_t x) { return (0x30 <= x && x <= 0x7f); }
static __inline int isinterm(__uint8_t x) { return (0x20 <= x && x <= 0x2f); }
static __inline int isthree(__uint8_t x) { return (0x60 <= x && x <= 0x6f); }

static __inline int
getcs(const char * __restrict p, _ISO2022Charset * __restrict cs)
{

	_DIAGASSERT(p != NULL);
	_DIAGASSERT(cs != NULL);

	if (!strncmp(p, "94$", 3) && p[3] && !p[4]) {
		cs->final = (u_char)(p[3] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS94MULTI;
	} else if (!strncmp(p, "96$", 3) && p[3] && !p[4]) {
		cs->final = (u_char)(p[3] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS96MULTI;
	} else if (!strncmp(p, "94", 2) && p[2] && !p[3]) {
		cs->final = (u_char)(p[2] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS94;
	} else if (!strncmp(p, "96", 2) && p[2] && !p[3]) {
		cs->final = (u_char )(p[2] & 0xff);
		cs->interm = '\0';
		cs->vers = '\0';
		cs->type = CS96;
	} else {
		return 1;
	}

	return 0;
}


#define _NOTMATCH	0
#define _MATCH		1
#define _PARSEFAIL	2

static __inline int
get_recommend(_ISO2022EncodingInfo * __restrict ei,
	      const char * __restrict token)
{
	int i;
	_ISO2022Charset cs;

	if (!strchr("0123", token[0]) || token[1] != '=')
		return (_NOTMATCH);

	if (getcs(&token[2], &cs) == 0)
		;
	else if (!strcmp(&token[2], "94")) {
		cs.final = (u_char)(token[4]);
		cs.interm = '\0';
		cs.vers = '\0';
		cs.type = CS94;
	} else if (!strcmp(&token[2], "96")) {
		cs.final = (u_char)(token[4]);
		cs.interm = '\0';
		cs.vers = '\0';
		cs.type = CS96;
	} else if (!strcmp(&token[2], "94$")) {
		cs.final = (u_char)(token[5]);
		cs.interm = '\0';
		cs.vers = '\0';
		cs.type = CS94MULTI;
	} else if (!strcmp(&token[2], "96$")) {
		cs.final = (u_char)(token[5]);
		cs.interm = '\0';
		cs.vers = '\0';
		cs.type = CS96MULTI;
	} else {
		return (_PARSEFAIL);
	}

	i = token[0] - '0';
	ei->recommendsize[i] += 1;
	if (!ei->recommend[i]) {
		ei->recommend[i] = malloc(sizeof(_ISO2022Charset));
	} else {
		ei->recommend[i] =
		    realloc(ei->recommend[i],
			    sizeof(_ISO2022Charset)* (ei->recommendsize[i]));
	}
	if (!ei->recommend[i])
		return (_PARSEFAIL);

	(ei->recommend[i] + (ei->recommendsize[i] - 1))->final = cs.final;
	(ei->recommend[i] + (ei->recommendsize[i] - 1))->interm = cs.interm;
	(ei->recommend[i] + (ei->recommendsize[i] - 1))->vers = cs.vers;
	(ei->recommend[i] + (ei->recommendsize[i] - 1))->type = cs.type;

	return (_MATCH);
}

static __inline int
get_initg(_ISO2022EncodingInfo * __restrict ei,
	  const char * __restrict token)
{
	_ISO2022Charset cs;

	if (strncmp("INIT", &token[0], 4) ||
	    !strchr("0123", token[4]) ||
	    token[5] != '=')
		return (_NOTMATCH);

	if (getcs(&token[6], &cs) != 0)
		return (_PARSEFAIL);

	ei->initg[token[4] - '0'].type = cs.type;
	ei->initg[token[4] - '0'].final = cs.final;
	ei->initg[token[4] - '0'].interm = cs.interm;
	ei->initg[token[4] - '0'].vers = cs.vers;

	return (_MATCH);
}

static __inline int
get_max(_ISO2022EncodingInfo * __restrict ei,
	const char * __restrict token)
{
	if (!strcmp(token, "MAX1")) {
		ei->maxcharset = 1;
	} else if (!strcmp(token, "MAX2")) {
		ei->maxcharset = 2;
	} else if (!strcmp(token, "MAX3")) {
		ei->maxcharset = 3;
	} else
		return (_NOTMATCH);

	return (_MATCH);
}


static __inline int
get_flags(_ISO2022EncodingInfo * __restrict ei,
	  const char * __restrict token)
{
	int i;
	static struct {
		const char	*tag;
		int		flag;
	} const tags[] = {
		{ "DUMMY",	0	},
		{ "8BIT",	F_8BIT	},
		{ "NOOLD",	F_NOOLD	},
		{ "SI",		F_SI	},
		{ "SO",		F_SO	},
		{ "LS0",	F_LS0	},
		{ "LS1",	F_LS1	},
		{ "LS2",	F_LS2	},
		{ "LS3",	F_LS3	},
		{ "LS1R",	F_LS1R	},
		{ "LS2R",	F_LS2R	},
		{ "LS3R",	F_LS3R	},
		{ "SS2",	F_SS2	},
		{ "SS3",	F_SS3	},
		{ "SS2R",	F_SS2R	},
		{ "SS3R",	F_SS3R	},
		{ NULL,		0 }
	};

	for (i = 0; tags[i].tag; i++) {
		if (!strcmp(token, tags[i].tag)) {
			ei->flags |= tags[i].flag;
			return (_MATCH);
		}
	}

	return (_NOTMATCH);
}


static __inline int
_citrus_ISO2022_parse_variable(_ISO2022EncodingInfo * __restrict ei,
			       const void * __restrict var, size_t lenvar)
{
	char const *v, *e;
	char buf[20];
	int i, len, ret;

	_DIAGASSERT(ei != NULL);


	/*
	 * parse VARIABLE section.
	 */

	if (!var)
		return (EFTYPE);

	v = (const char *) var;

	/* initialize structure */
	ei->maxcharset = 0;
	for (i = 0; i < 4; i++) {
		ei->recommend[i] = NULL;
		ei->recommendsize[i] = 0;
	}
	ei->flags = 0;

	while (*v) {
		while (*v == ' ' || *v == '\t')
			++v;

		/* find the token */
		e = v;
		while (*e && *e != ' ' && *e != '\t')
			++e;
		if (*e) {
			len = e-v;
			if (len>=sizeof(buf))
				goto parsefail;
			sprintf(buf, "%.*s", len, v);
			++e;
		}

		if ((ret = get_recommend(ei, buf)) != _NOTMATCH)
			;
		else if ((ret = get_initg(ei, buf)) != _NOTMATCH)
			;
		else if ((ret = get_max(ei, buf)) != _NOTMATCH)
			;
		else if ((ret = get_flags(ei, buf)) != _NOTMATCH)
			;
		else
			ret = _PARSEFAIL;
		if (ret==_PARSEFAIL)
			goto parsefail;
		v = e;

	}

	return (0);

parsefail:
	free(ei->recommend[0]);
	free(ei->recommend[1]);
	free(ei->recommend[2]);
	free(ei->recommend[3]);

	return (EFTYPE);
}

static __inline void
/*ARGSUSED*/
_citrus_ISO2022_init_state(_ISO2022EncodingInfo * __restrict ei,
			   _ISO2022State * __restrict s)
{
	int i;

	memset(s, 0, sizeof(*s));
	s->gl = 0;
	s->gr = (ei->flags & F_8BIT) ? 1 : -1;

	for (i = 0; i < 4; i++) {
		if (ei->initg[i].final) {
			s->g[i].type = ei->initg[i].type;
			s->g[i].final = ei->initg[i].final;
			s->g[i].interm = ei->initg[i].interm;
		}
	}
	s->singlegl = s->singlegr = -1;
}

static __inline void
/*ARGSUSED*/
_citrus_ISO2022_pack_state(_ISO2022EncodingInfo * __restrict ei,
			   void * __restrict pspriv,
			   const _ISO2022State * __restrict s)
{
	memcpy(pspriv, (const void *)s, sizeof(*s));
}

static __inline void
/*ARGSUSED*/
_citrus_ISO2022_unpack_state(_ISO2022EncodingInfo * __restrict ei,
			     _ISO2022State * __restrict s,
			     const void * __restrict pspriv)
{
	memcpy((void *)s, pspriv, sizeof(*s));
}

static int
/*ARGSUSED*/
_citrus_ISO2022_stdencoding_init(_ISO2022EncodingInfo * __restrict ei,
				 const void * __restrict var, size_t lenvar)
{

	_DIAGASSERT(ei != NULL);

	return _citrus_ISO2022_parse_variable(ei, var, lenvar);
}

static void
/*ARGSUSED*/
_citrus_ISO2022_stdencoding_uninit(_ISO2022EncodingInfo *ei)
{
}

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
seqmatch(const char * __restrict s, size_t n,
	 const struct seqtable * __restrict sp)
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

static wchar_t
_ISO2022_sgetwchar(_ISO2022EncodingInfo * __restrict ei,
			  const char * __restrict string, size_t n,
			  const char ** __restrict result,
			  _ISO2022State * __restrict psenc)
{
	wchar_t wchar = 0;
	int cur;
	struct seqtable *sp;
	int nmatch;
	int i;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(state != NULL);
	_DIAGASSERT(string != NULL);
	/* result may be NULL */

	while (1) {
		/* SI/SO */
		if (1 <= n && string[0] == '\017') {
			psenc->gl = 0;
			string++;
			n--;
			continue;
		}
		if (1 <= n && string[0] == '\016') {
			psenc->gl = 1;
			string++;
			n--;
			continue;
		}

		/* SS2/3R */
		if (1 <= n && string[0] && strchr("\217\216", string[0])) {
			psenc->singlegl = psenc->singlegr =
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
			psenc->g[i].type = sp->type;
			psenc->g[i].final = '\0';
			psenc->g[i].interm = '\0';
			psenc->g[i].vers = '\0';
			/* sp->finaloff must not be -1 */
			if (sp->finaloff != -1)
				psenc->g[i].final = string[sp->finaloff];
			if (sp->intermoff != -1)
				psenc->g[i].interm = string[sp->intermoff];
			if (sp->versoff != -1)
				psenc->g[i].vers = string[sp->versoff];

			string += sp->len;
			n -= sp->len;
			continue;
		}

		/* LS2/3 */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("no", string[1])) {
			psenc->gl = string[1] - 'n' + 2;
			string += 2;
			n -= 2;
			continue;
		}

		/* LS1/2/3R */
			/* XXX: { for vi showmatch */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("~}|", string[1])) {
			psenc->gr = 3 - (string[1] - '|');
			string += 2;
			n -= 2;
			continue;
		}

		/* SS2/3 */
		if (2 <= n && string[0] == '\033'
		 && string[1] && strchr("NO", string[1])) {
			psenc->singlegl = (string[1] - 'N') + 2;
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
					return (_ISO2022INVALID);
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
		return (_ISO2022INVALID);
	}

	/* normal chars.  always eat C0/C1 as is. */
	if (iscntl(*string & 0xff))
		cur = -1;
	else if (*string & 0x80) {
		cur = (psenc->singlegr == -1)
			? psenc->gr : psenc->singlegr;
	} else {
		cur = (psenc->singlegl == -1)
			? psenc->gl : psenc->singlegl;
	}

	if (cur == -1) {
asis:
		wchar = *string++ & 0xff;
		if (result)
			*result = string;
		/* reset single shift state */
		psenc->singlegr = psenc->singlegl = -1;
		return wchar;
	}

	/* length error check */
	switch (psenc->g[cur].type) {
	case CS94MULTI:
	case CS96MULTI:
		if (!isthree(psenc->g[cur].final)) {
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
		return (_ISO2022INVALID);

	case CS94:
	case CS96:
		if (1 <= n)
			break;

		/* we still need to wait for more characters to come */
		if (result)
			*result = string;
		return (_ISO2022INVALID);
	}

	/* range check */
	switch (psenc->g[cur].type) {
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
	switch (psenc->g[cur].type) {
	case CS94:
		/* special case for ASCII. */
		if (psenc->g[cur].final == 'B' && !psenc->g[cur].interm) {
			wchar = *string++;
			wchar &= 0x7f;
			break;
		}
		wchar = psenc->g[cur].final;
		wchar = (wchar << 8);
		wchar |= (psenc->g[cur].interm ? (0x80 | psenc->g[cur].interm) : 0);
		wchar = (wchar << 8);
		wchar = (wchar << 8) | (*string++ & 0x7f);
		break;
	case CS96:
		/* special case for ISO-8859-1. */
		if (psenc->g[cur].final == 'A' && !psenc->g[cur].interm) {
			wchar = *string++;
			wchar &= 0x7f;
			wchar |= 0x80;
			break;
		}
		wchar = psenc->g[cur].final;
		wchar = (wchar << 8);
		wchar |= (psenc->g[cur].interm ? (0x80 | psenc->g[cur].interm) : 0);
		wchar = (wchar << 8);
		wchar = (wchar << 8) | (*string++ & 0x7f);
		wchar |= 0x80;
		break;
	case CS94MULTI:
	case CS96MULTI:
		wchar = psenc->g[cur].final;
		wchar = (wchar << 8);
		if (isthree(psenc->g[cur].final))
			wchar |= (*string++ & 0x7f);
		wchar = (wchar << 8) | (*string++ & 0x7f);
		wchar = (wchar << 8) | (*string++ & 0x7f);
		if (psenc->g[cur].type == CS96MULTI)
			wchar |= 0x80;
		break;
	}

	if (result)
		*result = string;
	/* reset single shift state */
	psenc->singlegr = psenc->singlegl = -1;
	return wchar;
}



static int
_citrus_ISO2022_mbrtowc_priv(_ISO2022EncodingInfo * __restrict ei,
			     wchar_t * __restrict pwc,
			     const char ** __restrict s,
			     size_t n, _ISO2022State * __restrict psenc,
			     size_t * __restrict nresult)
{
	wchar_t wchar;
	const char *s0, *p, *result;
	int c;
	int chlenbak;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s != NULL);

	s0 = *s;
	c = 0;
	chlenbak = psenc->chlen;

	/*
	 * if we have something in buffer, use that.
	 * otherwise, skip here
	 */
	if (psenc->chlen < 0 || psenc->chlen > sizeof(psenc->ch)) {
		/* illgeal state */
		_citrus_ISO2022_init_state(ei, psenc);
		goto encoding_error;
	}
	if (psenc->chlen == 0)
		goto emptybuf;

	/* buffer is not empty */
	p = psenc->ch;
	while (psenc->chlen < sizeof(psenc->ch) && n >= 0) {
		if (n > 0) {
			psenc->ch[psenc->chlen++] = *s0++;
			n--;
		}

		wchar = _ISO2022_sgetwchar(ei, p, psenc->chlen - (p-psenc->ch),
					   &result, psenc);
		if (wchar != _ISO2022INVALID) {
			c += result - p;
			if (psenc->chlen > c)
				memmove(psenc->ch, result, psenc->chlen - c);
			if (psenc->chlen < c)
				psenc->chlen = 0;
			else
				psenc->chlen -= c;
			goto output;
		}

		c += result - p;
		p = result;

		if (n == 0)
			goto restart;
	}

	/* escape sequence too long? */
	goto encoding_error;

emptybuf:
	wchar = _ISO2022_sgetwchar(ei, s0, n, &result, psenc);
	if (wchar != _ISO2022INVALID) {
		c += result - s0;
		psenc->chlen = 0;
		s0 = result;
		goto output;
	}
	if (result > s0 && n > result - s0) {
		c += (result - s0);
		n -= (result - s0);
		s0 = result;
		goto emptybuf;
	}
	n += c;
	if (n < sizeof(psenc->ch)) {
		memcpy(psenc->ch, s0 - c, n);
		psenc->chlen = n;
		s0 = result;
		goto restart;
	}

	/* escape sequence too long? */

encoding_error:
	psenc->chlen = 0;
	*nresult = (size_t)-1;
	return (EILSEQ);

output:
	*s = s0;
	if (pwc)
		*pwc = wchar;

	if (!wchar)
		*nresult = 0;
	else
		*nresult = c - chlenbak;

	return (0);

restart:
	*s = s0;
	*nresult = (size_t)-2;

	return (0);
}

static int
recommendation(_ISO2022EncodingInfo * __restrict ei,
	       _ISO2022Charset * __restrict cs)
{
	int i, j;
	_ISO2022Charset *recommend;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(cs != NULL);

	/* first, try a exact match. */
	for (i = 0; i < 4; i++) {
		recommend = ei->recommend[i];
		for (j = 0; j < ei->recommendsize[i]; j++) {
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
		recommend = ei->recommend[i];
		for (j = 0; j < ei->recommendsize[i]; j++) {
			if (cs->type != recommend[j].type)
				continue;
			if (cs->final && (cs->final != recommend[j].final))
				continue;
			if (cs->interm && (cs->interm != recommend[j].interm))
				continue;

			return i;
		}
	}

	/* there's no recommendation. make a guess. */
	if (ei->maxcharset == 0) {
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
_ISO2022_sputwchar(_ISO2022EncodingInfo * __restrict ei, wchar_t c,
		   char * __restrict string, size_t n,
		   char ** __restrict result,
		   _ISO2022State * __restrict psenc)
{
	int i = 0, len;
	_ISO2022Charset cs;
	char *p;
	char tmp[MB_LEN_MAX];
	int target;
	u_char mask;
	int bit8;

	_DIAGASSERT(ei != NULL);
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
	target = recommendation(ei, &cs);
	p = tmp;
	bit8 = ei->flags & F_8BIT;

	/* designate the charset onto the target plane(G0/1/2/3). */
	if (psenc->g[target].type == cs.type
	 && psenc->g[target].final == cs.final
	 && psenc->g[target].interm == cs.interm)
		goto planeok;

	*p++ = '\033';
	if (cs.type == CS94MULTI || cs.type == CS96MULTI)
		*p++ = '$';
	if (target == 0 && cs.type == CS94MULTI && strchr("@AB", cs.final)
	 && !cs.interm && !(ei->flags & F_NOOLD))
		;
	else if (cs.type == CS94 || cs.type == CS94MULTI)
		*p++ = "()*+"[target];
	else
		*p++ = ",-./"[target];
	if (cs.interm)
		*p++ = cs.interm;
	*p++ = cs.final;

	psenc->g[target].type = cs.type;
	psenc->g[target].final = cs.final;
	psenc->g[target].interm = cs.interm;

planeok:
	 
	/* invoke the plane onto GL or GR. */
	if (psenc->gl == target)
		goto sideok;
	if (bit8 && psenc->gr == target)
		goto sideok;

	if (target == 0 && (ei->flags & F_LS0)) {
		*p++ = '\017';
		psenc->gl = 0;
	} else if (target == 1 && (ei->flags & F_LS1)) {
		*p++ = '\016';
		psenc->gl = 1;
	} else if (target == 2 && (ei->flags & F_LS2)) {
		*p++ = '\033';
		*p++ = 'n';
		psenc->gl = 2;
	} else if (target == 3 && (ei->flags & F_LS3)) {
		*p++ = '\033';
		*p++ = 'o';
		psenc->gl = 3;
	} else if (bit8 && target == 1 && (ei->flags & F_LS1R)) {
		*p++ = '\033';
		*p++ = '~';
		psenc->gr = 1;
	} else if (bit8 && target == 2 && (ei->flags & F_LS2R)) {
		*p++ = '\033';
		/*{*/
		*p++ = '}';
		psenc->gr = 2;
	} else if (bit8 && target == 3 && (ei->flags & F_LS3R)) {
		*p++ = '\033';
		*p++ = '|';
		psenc->gr = 3;
	} else if (target == 2 && (ei->flags & F_SS2)) {
		*p++ = '\033';
		*p++ = 'N';
		psenc->singlegl = 2;
	} else if (target == 3 && (ei->flags & F_SS3)) {
		*p++ = '\033';
		*p++ = 'O';
		psenc->singlegl = 3;
	} else if (bit8 && target == 2 && (ei->flags & F_SS2R)) {
		*p++ = '\216';
		*p++ = 'N';
		psenc->singlegl = psenc->singlegr = 2;
	} else if (bit8 && target == 3 && (ei->flags & F_SS3R)) {
		*p++ = '\217';
		*p++ = 'O';
		psenc->singlegl = psenc->singlegr = 3;
	} else
		abort();

sideok:
	if (psenc->singlegl == target)
		mask = 0x00;
	else if (psenc->singlegr == target)
		mask = 0x80;
	else if (psenc->gl == target)
		mask = 0x00;
	else if ((ei->flags & F_8BIT) && psenc->gr == target)
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
	psenc->singlegl = psenc->singlegr = -1;

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

static int
_citrus_ISO2022_wcrtomb_priv(_ISO2022EncodingInfo * __restrict ei,
			     char * __restrict s, size_t n, wchar_t wc,
			     _ISO2022State * __restrict psenc,
			     size_t * __restrict nresult)
{
	char buf[MB_LEN_MAX];
	char *result;
	int len;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	/* XXX state will be modified after this operation... */
	len = _ISO2022_sputwchar(ei, wc, buf, sizeof(buf), &result, psenc);
	if (sizeof(buf) < len || n < len) {
		/* XXX should recover state? */
		goto ilseq;
	}

	memcpy(s, buf, len);
	*nresult = (size_t)len;
	return (0);

ilseq:
	/* bound check failure */
	*nresult = (size_t)-1;
	return (EILSEQ);
}

/* ----------------------------------------------------------------------
 * public interface for ctype
 */

_CITRUS_CTYPE_DECLS(ISO2022);
_CITRUS_CTYPE_DEF_OPS(ISO2022);

#include "citrus_ctype_template.h"
