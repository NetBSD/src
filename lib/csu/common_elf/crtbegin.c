/*	$NetBSD: crtbegin.c,v 1.12 2001/07/17 13:28:05 mrg Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg and Ross Harvey.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * XXX EVENTUALLY SHOULD BE MERGED BACK WITH c++rt0.c
 */

/*
 * Run-time module which handles constructors and destructors,
 * and NetBSD identification.
 *
 * The linker constructs the following arrays of pointers to global
 * constructors and destructors. The first element contains the
 * number of pointers in each or -1 to indicate that the run-time
 * code should figure out how many there are.  The tables are also
 * null-terminated.
 */

#include <sys/param.h>		/* sysident.h requires `NetBSD' constant */
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <stdlib.h>

#include "sysident.h"
#include "dot_init.h"

static void (*__CTOR_LIST__[1])(void)
    __attribute__((section(".ctors"))) = { (void *)-1 };	/* XXX */
static void (*__DTOR_LIST__[1])(void)
    __attribute__((section(".dtors"))) = { (void *)-1 };	/* XXX */

static void __dtors(void);
static void __ctors(void);

INIT_FALLTHRU_DECL;
FINI_FALLTHRU_DECL;

extern void _init(void)   __attribute__((section(".init")));
extern void _fini(void)   __attribute__((section(".fini")));
static void __ctors(void) __attribute__((section(".init")));
static void __dtors(void) __attribute__((section(".fini")));

static void
__ctors()
{
	void (**p)(void) = __CTOR_LIST__ + 1;

	while (*p)
		(**p++)();
}

static void
__dtors()
{
	unsigned long i = (unsigned long) __DTOR_LIST__[0];
	void (**p)(void);

	if (i == -1)  {
		for (i = 1; __DTOR_LIST__[i] != NULL; i++)
			;
		i--;
	}
	p = __DTOR_LIST__ + i;
	while (i--)
		(**p--)();
}

void
_init()
{
	static int initialized = 0;

	/*
	 * Call global constructors.
	 * Arrange to call global destructors at exit.
	 */
	INIT_FALLTHRU();
	if (!initialized) {
		initialized = 1;
		__ctors();
	}
}

void
_fini()
{

	/*
	 * Call global destructors.
	 */
	/* prevent function pointer constant propagation */
	__dtors();
	FINI_FALLTHRU();
}

MD_INIT_SECTION_PROLOGUE;

MD_FINI_SECTION_PROLOGUE;
