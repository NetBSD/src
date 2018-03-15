/*	$NetBSD: h_ifunc_static.c,v 1.2.2.2 2018/03/15 09:12:07 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Joerg Sonnenberger.
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

#if defined(__arm__) || defined(__i386__) || defined(__x86_64__) || defined(__powerpc__) || defined(__sparc__)
#include <stdlib.h>
#include <string.h>

static long long
ifunc1(void)
{
	return 0xdeadbeefll;
}

static long long
ifunc2(void)
{
	return 0xbeefdeadll;
}

static __attribute__((used))
long long (*resolve_ifunc(void))(void)
{
	const char *e = getenv("USE_IFUNC2");
	return e && strcmp(e, "1") == 0 ? ifunc2 : ifunc1;
}

__ifunc(ifunc, resolve_ifunc);
extern long long ifunc(void);

int
main(int argc, char **argv)
{

	if (argc != 2)
		return 1;
	return atoll(argv[1]) != ifunc();
}
#else
int
main(int argc, char **argv)
{
	return 127;
}
#endif
