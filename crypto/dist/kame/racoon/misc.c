/*	$KAME: misc.c,v 1.23 2001/08/16 14:37:29 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: misc.c,v 1.2 2003/07/12 09:37:11 itojun Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>

#include "var.h"
#include "misc.h"
#include "debug.h"

#if 0
static int bindump __P((void *, size_t));

static int
bindump(buf0, len)
        void *buf0;
        size_t len;
{
	unsigned char *buf = (unsigned char *)buf0;
	size_t i;

	for (i = 0; i < len; i++) {
		if ((buf[i] & 0x80) || !isprint(buf[i]))
			printf("\\x%x", buf[i]);
		else
			printf("%c", buf[i]);
	}
	printf("\n");

	return 0;
}
#endif

int
hexdump(buf0, len)
	void *buf0;
	size_t len;
{
	caddr_t buf = (caddr_t)buf0;
	size_t i;

	for (i = 0; i < len; i++) {
		if (i != 0 && i % 32 == 0)
			printf("\n");
		if (i % 4 == 0)
			printf(" ");
		printf("%02x", (unsigned char)buf[i]);
	}
	printf("\n");

	return 0;
}

char *
bit2str(n, bl)
	int n, bl;
{
#define MAXBITLEN 128
	static char b[MAXBITLEN + 1];
	int i;

	if (bl > MAXBITLEN)
		return "Failed to convert.";	/* NG */
	memset(b, '0', bl);
	b[bl] = '\0';

	for (i = 0; i < bl; i++) {
		if (n & (1 << i))
			b[bl - 1 - i] = '1';
	}

	return b;
}

const char *
debug_location(file, line, func)
	const char *file;
	int line;
	const char *func;
{
	static char buf[1024];
	const char *p;

	/* truncate pathname */
	p = strrchr(file, '/');
	if (p)
		p++;
	else
		p = file;

	if (func)
		snprintf(buf, sizeof(buf), "%s:%d:%s()", p, line, func);
	else
		snprintf(buf, sizeof(buf), "%s:%d", p, line);

	return buf;
}

/*
 * get file size.
 * -1: error occured.
 */
int
getfsize(path)
	char *path;
{
        struct stat st;

        if (stat(path, &st) != 0)
                return -1;
        else
                return st.st_size;
}

/*
 * calculate the difference between two times.
 * t1: start
 * t2: end
 */
double
timedelta(t1, t2)
	struct timeval *t1, *t2;
{
	if (t2->tv_usec >= t1->tv_usec)
		return t2->tv_sec - t1->tv_sec +
			(double)(t2->tv_usec - t1->tv_usec) / 1000000;

	return t2->tv_sec - t1->tv_sec - 1 +
		(double)(1000000 + t2->tv_usec - t1->tv_usec) / 1000000;
}
