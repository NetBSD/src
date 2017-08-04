/*
 * Copyright 2011 Nexenta Systems, Inc.  All rights reserved.
 * Copyright 2012 Garrett D'Amore <garrett@damore.org>  All rights reserved.
 * Copyright 2015 John Marino <draco@marino.st>
 *
 * This source code is derived from the illumos localedef command, and
 * provided under BSD-style license terms by Nexenta Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * LC_CTYPE database generation routines for localedef.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/usr.bin/localedef/ctype.c 306782 2016-10-06 19:46:43Z bapt $");
#endif

#if HAVE_NBTOOL_CONFIG_H
# include "../../sys/sys/tree.h"
#else
# include <sys/tree.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <netinet/in.h>	/* Needed by <arpa/inet.h> on NetBSD 1.5. */
#include <arpa/inet.h>	/* Needed for htonl on POSIX systems. */
#include <wchar.h>
#include <ctype.h>
#include <wctype.h>
#include <unistd.h>
#include "localedef.h"
#include "parser.h"
#include "runetype_file.h"


/* Needed for bootstrapping, _CTYPE_N */
#ifndef _RUNETYPE_N
#define _RUNETYPE_N      0x0 /* 0x00400000L */
#endif

#define _ISUPPER	_RUNETYPE_U
#define _ISLOWER	_RUNETYPE_L
#define	_ISDIGIT	_RUNETYPE_D
#define	_ISXDIGIT	_RUNETYPE_X
#define	_ISSPACE	_RUNETYPE_S
#define	_ISBLANK	_RUNETYPE_B
#define	_ISALPHA	_RUNETYPE_A
#define	_ISPUNCT	_RUNETYPE_P
#define	_ISGRAPH	_RUNETYPE_G
#define	_ISPRINT	_RUNETYPE_R
#define	_ISCNTRL	_RUNETYPE_C
#define	_E1		_RUNETYPE_Q
#define	_E2		_RUNETYPE_I
#define	_E3		0
#define	_E4		_RUNETYPE_N
#define	_E5		_RUNETYPE_T

static wchar_t		last_ctype;
static int ctype_compare(const void *n1, const void *n2);

typedef struct ctype_node {
	wchar_t wc;
	int32_t	ctype;
	int32_t	toupper;
	int32_t	tolower;
	RB_ENTRY(ctype_node) entry;
} ctype_node_t;

static RB_HEAD(ctypes, ctype_node) ctypes;
RB_GENERATE_STATIC(ctypes, ctype_node, entry, ctype_compare);

static int
ctype_compare(const void *n1, const void *n2)
{
	const ctype_node_t *c1 = n1;
	const ctype_node_t *c2 = n2;

	return (c1->wc < c2->wc ? -1 : c1->wc > c2->wc ? 1 : 0);
}

void
init_ctype(void)
{
	RB_INIT(&ctypes);
}


static void
add_ctype_impl(ctype_node_t *ctn)
{
	switch (last_kw) {
	case T_ISUPPER:
		ctn->ctype |= (_ISUPPER | _ISALPHA | _ISGRAPH | _ISPRINT);
		break;
	case T_ISLOWER:
		ctn->ctype |= (_ISLOWER | _ISALPHA | _ISGRAPH | _ISPRINT);
		break;
	case T_ISALPHA:
		ctn->ctype |= (_ISALPHA | _ISGRAPH | _ISPRINT);
		break;
	case T_ISDIGIT:
		ctn->ctype |= (_ISDIGIT | _ISGRAPH | _ISPRINT | _ISXDIGIT | _E4);
		break;
	case T_ISSPACE:
		ctn->ctype |= _ISSPACE;
		break;
	case T_ISCNTRL:
		ctn->ctype |= _ISCNTRL;
		break;
	case T_ISGRAPH:
		ctn->ctype |= (_ISGRAPH | _ISPRINT);
		break;
	case T_ISPRINT:
		ctn->ctype |= _ISPRINT;
		break;
	case T_ISPUNCT:
		ctn->ctype |= (_ISPUNCT | _ISGRAPH | _ISPRINT);
		break;
	case T_ISXDIGIT:
		ctn->ctype |= (_ISXDIGIT | _ISPRINT);
		break;
	case T_ISBLANK:
		ctn->ctype |= (
#ifdef _ISBLANK
			       _ISBLANK |
#endif
			       _ISSPACE);
		break;
	case T_ISPHONOGRAM:
		ctn->ctype |= (_E1 | _ISPRINT | _ISGRAPH);
		break;
	case T_ISIDEOGRAM:
		ctn->ctype |= (_E2 | _ISPRINT | _ISGRAPH);
		break;
	case T_ISENGLISH:
		ctn->ctype |= (_E3 | _ISPRINT | _ISGRAPH);
		break;
	case T_ISNUMBER:
		ctn->ctype |= (_E4 | _ISPRINT | _ISGRAPH);
		break;
	case T_ISSPECIAL:
		ctn->ctype |= (_E5 | _ISPRINT | _ISGRAPH);
		break;
	case T_ISALNUM:
		/*
		 * We can't do anything with this.  The character
		 * should already be specified as a digit or alpha.
		 */
		break;
	default:
		errf("not a valid character class");
	}
}

static ctype_node_t *
get_ctype(wchar_t wc)
{
	ctype_node_t	srch;
	ctype_node_t	*ctn;

	srch.wc = wc;
	if ((ctn = RB_FIND(ctypes, &ctypes, &srch)) == NULL) {
		if ((ctn = calloc(1, sizeof (*ctn))) == NULL) {
			errf("out of memory");
			return (NULL);
		}
		ctn->wc = wc;

		RB_INSERT(ctypes, &ctypes, ctn);
	}
	return (ctn);
}

void
add_ctype(int val)
{
	ctype_node_t	*ctn;

	if ((ctn = get_ctype(val)) == NULL) {
		INTERR;
		return;
	}
	add_ctype_impl(ctn);
	last_ctype = ctn->wc;
}

void
add_ctype_range(wchar_t end)
{
	ctype_node_t	*ctn;
	wchar_t		cur;

	if (end < last_ctype) {
		errf("malformed character range (%u ... %u))",
		    last_ctype, end);
		return;
	}
	for (cur = last_ctype + 1; cur <= end; cur++) {
		if ((ctn = get_ctype(cur)) == NULL) {
			INTERR;
			return;
		}
		add_ctype_impl(ctn);
	}
	last_ctype = end;

}

/*
 * A word about widths: if the width mask is specified, then libc
 * unconditionally honors it.  Otherwise, it assumes printable
 * characters have width 1, and non-printable characters have width
 * -1 (except for NULL which is special with with 0).  Hence, we have
 * no need to inject defaults here -- the "default" unset value of 0
 * indicates that libc should use its own logic in wcwidth as described.
 */
void
add_width(int wc, int width)
{
	ctype_node_t	*ctn;

	if ((ctn = get_ctype(wc)) == NULL) {
		INTERR;
		return;
	}
	ctn->ctype &= ~(_RUNETYPE_SWM);
	switch (width) {
	case 0:
		ctn->ctype |= _RUNETYPE_SW0;
		break;
	case 1:
		ctn->ctype |= _RUNETYPE_SW1;
		break;
	case 2:
		ctn->ctype |= _RUNETYPE_SW2;
		break;
	case 3:
		ctn->ctype |= _RUNETYPE_SW3;
		break;
	}
}

void
add_width_range(int start, int end, int width)
{
	for (; start <= end; start++) {
		add_width(start, width);
	}
}

void
add_caseconv(int val, int wc)
{
	ctype_node_t	*ctn;

	ctn = get_ctype(val);
	if (ctn == NULL) {
		INTERR;
		return;
	}

	switch (last_kw) {
	case T_TOUPPER:
		ctn->toupper = wc;
		break;
	case T_TOLOWER:
		ctn->tolower = wc;
		break;
	default:
		INTERR;
		break;
	}
}

void
dump_ctype(void)
{
	FILE		*f;
	_FileRuneLocale	rl;
	ctype_node_t	*ctn, *last_ct, *last_lo, *last_up;
	_FileRuneEntry	*ct = NULL;
	_FileRuneEntry	*lo = NULL;
	_FileRuneEntry	*up = NULL;
	wchar_t		wc;
	int             n;
	char		variable[1024];

	(void) memset(&rl, 0, sizeof (rl));
	last_ct = NULL;
	last_lo = NULL;
	last_up = NULL;

	if ((f = open_category()) == NULL)
		return;

	(void) memcpy(rl.frl_magic, _RUNECT10_MAGIC, sizeof(rl.frl_magic));
	(void) strncpy(rl.frl_encoding, get_wide_encoding(), sizeof (rl.frl_encoding));

	/*
	 * Initialize the identity map.
	 */
	for (wc = 0; (unsigned)wc < _CTYPE_CACHE_SIZE; wc++) {
		rl.frl_maplower[wc] = htonl(wc);
		rl.frl_mapupper[wc] = htonl(wc);
	}

	RB_FOREACH(ctn, ctypes, &ctypes) {
		int conflict = 0;

		wc = ctn->wc;

		/*
		 * POSIX requires certain portable characters have
		 * certain types.  Add them if they are missing.
		 */
		if ((wc >= 1) && (wc <= 127)) {
			if ((wc >= 'A') && (wc <= 'Z'))
				ctn->ctype |= _ISUPPER;
			if ((wc >= 'a') && (wc <= 'z'))
				ctn->ctype |= _ISLOWER;
			if ((wc >= '0') && (wc <= '9'))
				ctn->ctype |= _ISDIGIT;
			if (wc == ' ')
				ctn->ctype |= _ISPRINT;
			if (strchr(" \f\n\r\t\v", (char)wc) != NULL)
				ctn->ctype |= _ISSPACE;
			if (strchr("0123456789ABCDEFabcdef", (char)wc) != NULL)
				ctn->ctype |= _ISXDIGIT;
			if (strchr(" \t", (char)wc))
				ctn->ctype |= _ISBLANK;

#ifndef ABANDON_NETBSD_COMPAT
			ctn->ctype &= ~_RUNETYPE_SWM;
			if (wc >= ' ' && wc <= '~')
				ctn->ctype |= _RUNETYPE_SW1;
			if (wc >= '0' && wc <= '9')
				ctn->ctype |= (wc - '0');
			if (wc >= 'A' && wc <= 'F')
				ctn->ctype |= (wc + 0x0A - 'A');
			if (wc >= 'a' && wc <= 'f')
				ctn->ctype |= (wc + 0x0a - 'a');
#endif

			/*
			 * Technically these settings are only
			 * required for the C locale.  However, it
			 * turns out that because of the historical
			 * version of isprint(), we need them for all
			 * locales as well.  Note that these are not
			 * necessarily valid punctation characters in
			 * the current language, but ispunct() needs
			 * to return TRUE for them.
			 */
			if (strchr("!\"'#$%&()*+,-./:;<=>?@[\\]^_`{|}~",
			    (char)wc))
				ctn->ctype |= _ISPUNCT;
		}

		/*
		 * POSIX also requires that certain types imply
		 * others.  Add any inferred types here.
		 */
		if (ctn->ctype & (_ISUPPER |_ISLOWER))
			ctn->ctype |= _ISALPHA;
		if (ctn->ctype & _ISDIGIT)
			ctn->ctype |= _ISXDIGIT;
		if (ctn->ctype & _ISBLANK)
			ctn->ctype |= _ISSPACE;
		if (ctn->ctype & (_ISALPHA|_ISDIGIT|_ISXDIGIT))
			ctn->ctype |= _ISGRAPH;
		if (ctn->ctype & _ISGRAPH)
			ctn->ctype |= _ISPRINT;

		/*
		 * Finally, POSIX requires that certain combinations
		 * are invalid.  We don't flag this as a fatal error,
		 * but we will warn about.
		 */
		if ((ctn->ctype & _ISALPHA) &&
		    (ctn->ctype & (_ISPUNCT|_ISDIGIT)))
			conflict++;
		if ((ctn->ctype & _ISPUNCT) &
		    (ctn->ctype & (_ISDIGIT|_ISALPHA|_ISXDIGIT)))
			conflict++;
		if ((ctn->ctype & _ISSPACE) && (ctn->ctype & _ISGRAPH))
			conflict++;
		if ((ctn->ctype & _ISCNTRL) & _ISPRINT)
			conflict++;
		if ((wc == ' ') && (ctn->ctype & (_ISPUNCT|_ISGRAPH)))
			conflict++;

		if (conflict) {
			warn("conflicting classes for character 0x%x (%x)",
			    wc, ctn->ctype);
		}
		/*
		 * Handle the lower 256 characters using the simple
		 * optimization.  Note that if we have not defined the
		 * upper/lower case, then we identity map it.
		 */
		if ((unsigned)wc < _CTYPE_CACHE_SIZE) {
			rl.frl_runetype[wc] = htonl(ctn->ctype);
			if (ctn->tolower)
				rl.frl_maplower[wc] = htonl(ctn->tolower);
			if (ctn->toupper)
				rl.frl_mapupper[wc] = htonl(ctn->toupper);
			continue;
		}

		if ((last_ct != NULL) && (last_ct->ctype == ctn->ctype) &&
		    (last_ct->wc + 1 == wc)) {
			ct[rl.frl_runetype_ext.frr_nranges-1].fre_max = wc;
		} else {
			rl.frl_runetype_ext.frr_nranges++;
			ct = realloc(ct,
			    sizeof (*ct) * rl.frl_runetype_ext.frr_nranges);
			ct[rl.frl_runetype_ext.frr_nranges - 1].fre_min = wc;
			ct[rl.frl_runetype_ext.frr_nranges - 1].fre_max = wc;
			ct[rl.frl_runetype_ext.frr_nranges - 1].fre_map = ctn->ctype;
		}
		last_ct = ctn;
		if (ctn->tolower == 0) {
			last_lo = NULL;
		} else if ((last_lo != NULL) &&
		    (last_lo->tolower + 1 == ctn->tolower)) {
			lo[rl.frl_maplower_ext.frr_nranges-1].fre_max = wc;
			last_lo = ctn;
		} else {
			rl.frl_maplower_ext.frr_nranges++;
			lo = realloc(lo,
			    sizeof (*lo) * rl.frl_maplower_ext.frr_nranges);
			lo[rl.frl_maplower_ext.frr_nranges - 1].fre_min = wc;
			lo[rl.frl_maplower_ext.frr_nranges - 1].fre_max = wc;
			lo[rl.frl_maplower_ext.frr_nranges - 1].fre_map = ctn->tolower;
			last_lo = ctn;
		}

		if (ctn->toupper == 0) {
			last_up = NULL;
		} else if ((last_up != NULL) &&
		    (last_up->toupper + 1 == ctn->toupper)) {
			up[rl.frl_mapupper_ext.frr_nranges-1].fre_max = wc;
			last_up = ctn;
		} else {
			rl.frl_mapupper_ext.frr_nranges++;
			up = realloc(up,
			    sizeof (*up) * rl.frl_mapupper_ext.frr_nranges);
			up[rl.frl_mapupper_ext.frr_nranges - 1].fre_min = wc;
			up[rl.frl_mapupper_ext.frr_nranges - 1].fre_max = wc;
			up[rl.frl_mapupper_ext.frr_nranges - 1].fre_map = ctn->toupper;
			last_up = ctn;
		}
	}

	/*
	 * Okay, we are now ready to write the new locale file.
	 */

	/*
	 * PART 1: The _RuneLocale structure
	 */
	if (fwrite((char *)&rl, sizeof(rl), 1, f) != 1) {
		perror("writing _FileRuneLocale");
		exit(1);
	}

	/*
	 * PART 2: The runetype_ext structures (not the actual tables)
	 */
	for (n = 0; n < rl.frl_runetype_ext.frr_nranges; n++) {
		_FileRuneEntry re;

		memset(&re, 0, sizeof(re));
		re.fre_min = htonl(ct[n].fre_min);
		re.fre_max = htonl(ct[n].fre_max);
		re.fre_map = htonl(ct[n].fre_map);

		if (fwrite((char *)&re, sizeof(re), 1, f) != 1) {
			fprintf(stderr, "writing runetype_ext #%d: %s\n", n, strerror(errno));
			exit(1);
		}
	}
	/*
	 * PART 3: The maplower_ext structures
	 */
	for (n = 0; n < rl.frl_maplower_ext.frr_nranges; n++) {
		_FileRuneEntry re;

		memset(&re, 0, sizeof(re));
		re.fre_min = htonl(lo[n].fre_min);
		re.fre_max = htonl(lo[n].fre_max);
		re.fre_map = htonl(lo[n].fre_map);

		if (fwrite((char *)&re, sizeof(re), 1, f) != 1) {
			fprintf(stderr, "error writing maplower_ext #%d: %s\n", n, strerror(errno));
			exit(1);
		}
	}
	/*
	 * PART 4: The mapupper_ext structures
	 */
	for (n = 0; n < rl.frl_mapupper_ext.frr_nranges; n++) {
		_FileRuneEntry re;

		memset(&re, 0, sizeof(re));
		re.fre_min = htonl(up[n].fre_min);
		re.fre_max = htonl(up[n].fre_max);
		re.fre_map = htonl(up[n].fre_map);

		if (fwrite((char *)&re, sizeof(re), 1, f) != 1) {
			fprintf(stderr, "writing mapupper_ext #%d: %s\n", n, strerror(errno));
			exit(1);
		}
	}
#if 0
	/*
	 * PART 5: The runetype_ext tables
	 */
	for (list = types.root, n = 0; list != NULL; list = list->next, n++) {
		for (x = 0; x < list->max - list->min + 1; ++x)
			list->types[x] = htonl(list->types[x]);

		if (!list->map) {
			if (fwrite((char *)list->types,
				   (list->max - list->min + 1) * sizeof(u_int32_t),
				   1, f) != 1) {
				fprintf(stderr, "writing runetype_ext table #%d: %s\n", n, strerror(errno));
			}
		}
	}
#endif
	/*
	 * PART 6: And finally the variable data
	 */
	snprintf(variable, sizeof(variable), "CODESET=%s", get_wide_charset());
	if (variable[8] != '\0') {
		if (fwrite(variable, strlen(variable), 1, f) != 1) {
			perror("writing variable data");
			exit(1);
		}
	}

	close_category(f);
}

void
set_variable(char *var)
{
}

