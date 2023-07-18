/* 	$NetBSD: initfini.c,v 1.16 2023/07/18 11:44:32 riastradh Exp $	 */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
__RCSID("$NetBSD: initfini.c,v 1.16 2023/07/18 11:44:32 riastradh Exp $");

#ifdef _LIBC
#include "namespace.h"
#endif

#include <sys/param.h>
#include <sys/exec.h>
#include <sys/tls.h>
#include <stdbool.h>
#include "csu-common.h"

void	_libc_init(void) __attribute__((__constructor__, __used__));

void	__guard_setup(void);
void	__libc_thr_init(void);
void	__libc_atomic_init(void);
void	__libc_atexit_init(void);
void	__libc_env_init(void);

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
__dso_hidden void	__libc_static_tls_setup(void);
#endif

#ifdef __weak_alias
__weak_alias(_dlauxinfo,___dlauxinfo)
static void *__libc_dlauxinfo;

void *___dlauxinfo(void) __pure;
__weakref_visible void * real_dlauxinfo(void) __weak_reference(_dlauxinfo);

void *
___dlauxinfo(void)
{
	return __libc_dlauxinfo;
}
#endif

static bool libc_initialised;

void _libc_init(void);

/*
 * Declare as common symbol to allow new libc with older binaries to
 * not trigger an undefined reference.
 */
struct ps_strings *__ps_strings __common;
char *__progname __common;
char **environ __common;

/*
 * _libc_init is called twice.  One call comes explicitly from crt0.o
 * (for newer versions) and the other is via global constructor handling.
 *
 * In static binaries the explicit call is first; in dynamically linked
 * binaries the global constructors of libc are called from ld.elf_so
 * before crt0.o is reached.
 *
 * Note that __ps_strings is set by crt0.o. So in the dynamic case, it
 * hasn't been set yet when we get here, and __libc_dlauxinfo is not
 * (ever) assigned. But this is ok because __libc_dlauxinfo is only
 * used in static binaries, because it's there to substitute for the
 * dynamic linker. In static binaries __ps_strings will have been set
 * up when we get here and we get a valid __libc_dlauxinfo.
 *
 * This code causes problems for Emacs because Emacs's undump
 * mechanism saves the __ps_strings value from the startup execution;
 * then running the resulting binary it gets here before crt0 has
 * assigned the current execution's value to __ps_strings, and in an
 * environment with ASLR this can cause the assignment of
 * __libc_dlauxinfo to receive SIGSEGV.
 */
void __section(".text.startup")
_libc_init(void)
{

	if (libc_initialised)
		return;

	libc_initialised = 1;

	/* Only initialize _dlauxinfo for static binaries. */
	if (__ps_strings != NULL && real_dlauxinfo == ___dlauxinfo)
		__libc_dlauxinfo = __ps_strings->ps_argvstr +
		    __ps_strings->ps_nargvstr + __ps_strings->ps_nenvstr + 2;

	/* For -fstack-protector */
	__guard_setup();

	/* Atomic operations */
	__libc_atomic_init();

#if defined(__HAVE_TLS_VARIANT_I) || defined(__HAVE_TLS_VARIANT_II)
	/* Initialize TLS for statically linked programs. */
	__libc_static_tls_setup();
#endif

	/* Threads */
	__libc_thr_init();

	/* Initialize the atexit mutexes */
	__libc_atexit_init();
}
