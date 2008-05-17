/*	$NetBSD: lcnumeric.c,v 1.1 2008/05/17 03:49:54 ginsbach Exp $	*/
/*
 * Copyright (c) 2008, The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Brian Ginsbach.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: lcnumeric.c,v 1.1 2008/05/17 03:49:54 ginsbach Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/localedef.h>

#include <assert.h>

#include "localeio.h"
#include "lcnumeric.h"

#define NSTRINGS (sizeof(_NumericLocale)/sizeof(const char **))

int
__loadnumeric(const char *filename)
{
	const char **gpp;

	_DIAGASSERT(filename != NULL);

	if (!__loadlocale(filename, NSTRINGS, 0, sizeof(_NumericLocale),
			  &_CurrentNumericLocale, &_DefaultNumericLocale)) {
		return 0;
	}

	gpp = __UNCONST(&_CurrentNumericLocale->grouping);
	*gpp = __convertgrouping(_CurrentNumericLocale->grouping);

	return 1;
}

#ifdef LOCALE_DEBUG

#include "namespace.h"

#include <stdio.h>

void
__dumpnumericlocale()
{
	const char *gp;

	(void)printf("decimal_point = \"%s\"\n",
		     _CurrentNumericLocale->decimal_point);
	(void)printf("thousands_sep = \"%s\"\n",
		     _CurrentNumericLocale->thousands_sep);
	(void)printf("grouping = ");
	for (gp = _CurrentNumericLocale->grouping; *gp != '\0'; gp++) {
		if (gp != _CurrentNumericLocale->grouping)
			(void)fputc(';', stdout);
		(void)printf("\\%03o", *gp);
	}
}
#endif	/* LOCALE_DEBUG */
