/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
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

#include <stdlib.h>
#include <string.h>

static int
ifunc1(void)
{
	return 0xdeadbeef;
}

static int
ifunc2(void)
{
	return 0xbeefdead;
}

static __attribute__((used))
int (*resolve_ifunc(void))(void)
{
	const char *e = getenv("USE_IFUNC2");
	return e && strcmp(e, "1") == 0 ? ifunc2 : ifunc1;
}

static __attribute__((used))
int (*resolve_ifunc2(void))(void)
{
	const char *e = getenv("USE_IFUNC2");
	return e && strcmp(e, "1") == 0 ? ifunc1 : ifunc2;
}

__ifunc(ifunc, resolve_ifunc);
__hidden_ifunc(ifunc_hidden, resolve_ifunc2);

int ifunc_hidden(void);
int ifunc_plt(void);

int ifunc_plt(void)
{
	return ifunc_hidden();
}

int (*ifunc_indirect(void))(void);

int (*ifunc_indirect(void))(void)
{
	return ifunc_hidden;
}
