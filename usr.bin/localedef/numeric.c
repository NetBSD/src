/*
 * Copyright 2010 Nexenta Systems, Inc.  All rights reserved.
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
 * LC_NUMERIC database generation routines for localedef.
 */
#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if 0
__FBSDID("$FreeBSD: head/usr.bin/localedef/numeric.c 290562 2015-11-08 22:23:21Z bapt $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#if HAVE_NBTOOL_CONFIG_H
# include "../../sys/sys/localedef.h"
#else
# include <sys/localedef.h>
#endif
#include "localedef.h"
#include "parser.h"
#include "citrus_lc_numeric.h"
#include "mklocaledb.h"

static _NumericLocale numeric;

void
init_numeric(void)
{
	(void) memset(&numeric, 0, sizeof (numeric));
}

void
add_numeric_str(wchar_t *wcs)
{
	char *str;

	if ((str = to_mb_string(wcs)) == NULL) {
		INTERR;
		return;
	}
	free(wcs);

	switch (last_kw) {
	case T_DECIMAL_POINT:
		numeric.decimal_point = str;
		break;
	case T_THOUSANDS_SEP:
		numeric.thousands_sep = str;
		break;
	default:
		free(str);
		INTERR;
		break;
	}
}

void
reset_numeric_group(void)
{
	free(__UNCONST(numeric.grouping));
	numeric.grouping = NULL;
}

void
add_numeric_group(int n)
{
	char *s;

	if (numeric.grouping == NULL) {
		(void) asprintf(&s, "%d", n);
	} else {
		(void) asprintf(&s, "%s;%d", numeric.grouping, n);
	}
	if (s == NULL)
		fprintf(stderr, "out of memory");

	free(__UNCONST(numeric.grouping));
	numeric.grouping = s;
}

void
dump_numeric(void)
{
	FILE *f;
	const category_t *cat;

	if ((f = open_category()) == NULL) {
		return;
	}
	if ((cat = mklocaledb_init("NUMERIC")) == NULL) {
		return;
	}

	mklocaledb_record(cat, _CITRUS_LC_NUMERIC_SYM_DECIMAL_POINT, numeric.decimal_point);
	mklocaledb_record(cat, _CITRUS_LC_NUMERIC_SYM_THOUSANDS_SEP, numeric.thousands_sep);
	mklocaledb_record(cat, _CITRUS_LC_NUMERIC_SYM_GROUPING, numeric.grouping);

	mklocaledb_dump(cat, f);
	close_category(f);
}
