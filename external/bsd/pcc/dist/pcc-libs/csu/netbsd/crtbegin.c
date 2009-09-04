/*	$Id: crtbegin.c,v 1.1.1.2 2009/09/04 00:27:35 gmcgarry Exp $	*/
/*-
 * Copyright (c) 1998, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg, Ross Harvey, and Jason R. Thorpe.
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

/*
 * Run-time module which handles constructors and destructors.
 */

#include "common.h"

extern void (*__CTOR_LIST__[1])(void);
extern void (*__DTOR_LIST__[1])(void);

asm(	"	.section .ctors\n"
	"	.align 2\n"
	"__CTOR_LIST__:\n"
#if defined(__x86_64__) || (__sparc64__)
	"	.quad -1\n"
#else
	"	.long -1\n"
#endif
	"	.section .dtors\n"
	"	.align 2\n"
	"__DTOR_LIST__:\n"
#if defined(__x86_64__) || (__sparc64__)
	"	.quad -1\n"
#else
	"	.long -1\n"
#endif
);

static void
__ctors(void)
{
	unsigned long i = (unsigned long) __CTOR_LIST__[0];
	void (**p)(void);

	if (i == (unsigned long) -1)  {
		for (i = 1; __CTOR_LIST__[i]; i++)
			;
		i--;
	}
	p = __CTOR_LIST__ + i;
	while (i--)
		(**p--)();
}

static void
__dtors(void)
{
	void (**p)(void) = __DTOR_LIST__ + 1;

	while (*p)
		(**p++)();
}

void
__do_global_ctors_aux(void)
{
	static int initialized;

	if (!initialized) {
		initialized = 1;
		__ctors();
	}
}

void
__do_global_dtors_aux(void)
{
	static int finished;

	if (finished)
		return;

	__dtors();

	finished = 1;
}

#define MD_CALL_STATIC_FUNCTION(section, func)				\
void __call_##func(void);						\
void __call_##func(void)						\
{									\
	asm volatile (".section " #section);				\
	func();								\
	asm volatile (".previous");					\
}

MD_CALL_STATIC_FUNCTION(.init, __do_global_ctors_aux)
MD_CALL_STATIC_FUNCTION(.fini, __do_global_dtors_aux)

IDENT("$Id: crtbegin.c,v 1.1.1.2 2009/09/04 00:27:35 gmcgarry Exp $");
