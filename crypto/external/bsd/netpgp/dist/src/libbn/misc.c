/*-
 * Copyright (c) 2012 Alistair Crooks <agc@NetBSD.org>
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
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syslog.h>

#ifdef _KERNEL
# include <sys/kmem.h>
#else
# include <ctype.h>
# include <inttypes.h>
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <time.h>
# include <unistd.h>
#endif

#include "misc.h"

#ifndef USE_ARG
#define USE_ARG(x)	/*LINTED*/(void)&(x)
#endif

void *
netpgp_allocate(size_t n, size_t nels)
{
#ifdef _KERNEL
	return kmem_zalloc(n * nels, KM_SLEEP);
#else
	return calloc(n, nels);
#endif
}

void
netpgp_deallocate(void *ptr, size_t size)
{
#ifdef _KERNEL
	kmem_free(ptr, size);
#else
	USE_ARG(size);
	free(ptr);
#endif
}

#ifndef _KERNEL
void
logmessage(const int level, const char *fmt, ...)
{
	va_list	args;

	USE_ARG(level);
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}
#endif

#ifndef _KERNEL
#define LINELEN	16

#define PRIsize	"z"

/* show hexadecimal/ascii dump */
void 
hexdump(FILE *fp, const char *header, const uint8_t *src, size_t length)
{
	size_t	i;
	char	line[LINELEN + 1];

	(void) fprintf(fp, "%s%s", (header) ? header : "", (header) ? "\n" : "");
	(void) fprintf(fp, "[%" PRIsize "u char%s]\n", length, (length == 1) ? "" : "s");
	for (i = 0 ; i < length ; i++) {
		if (i % LINELEN == 0) {
			(void) fprintf(fp, "%.5" PRIsize "u | ", i);
		}
		(void) fprintf(fp, "%.02x ", (uint8_t)src[i]);
		line[i % LINELEN] = (isprint(src[i])) ? src[i] : '.';
		if (i % LINELEN == LINELEN - 1) {
			line[LINELEN] = 0x0;
			(void) fprintf(fp, " | %s\n", line);
		}
	}
	if (i % LINELEN != 0) {
		for ( ; i % LINELEN != 0 ; i++) {
			(void) fprintf(fp, "   ");
			line[i % LINELEN] = ' ';
		}
		line[LINELEN] = 0x0;
		(void) fprintf(fp, " | %s\n", line);
	}
}
#endif
