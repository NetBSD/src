/*	$NetBSD: crt0.c,v 1.4 2003/04/01 10:20:38 scw Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "common.h"

/*
 * SH5 needs no assembler trampoline for __start as the kernel sets up
 * all the necessary parameters.
 */
void	__start(int argc, char **argv, char **envp, void (*cleanup)(void),
	    const Obj_Entry *obj, struct ps_strings *ps_strings);

/*
 * The SuperH SH5 linker has a bug which causes statically linked binaries
 * to SEGV in crt0.o due to bogus SHmedia GOT offsets. (crt0.o is always
 * compiled PIC).
 *
 * The following gross hack works around it for now (at the expense of
 * a few more runtime relocations in dynamically linked binaries).
 */
static int		(*hack_atexit)(void (*)(void)) = atexit;
#define	atexit		hack_atexit
static void		(*hack__init)(void) = _init;
#define	_init		hack__init
static void		(*hack__fini)(void) = _fini;
#define	_fini		hack__fini
static void		(*hack_exit)(int) = exit;
#define	exit		hack_exit
static int		(*hack_main)(int, char **, char **) = main;
#define	main		hack_main
#ifdef MCRT0
static void		(*hack_monstartup)(u_long, u_long) = monstartup;
#define	monstartup	hack_monstartup
#endif

void
__start(int argc, char **argv, char **envp, void (*cleanup)(void),
    const Obj_Entry *obj, struct ps_strings *ps_strings)
{
	char *pn;

	environ = envp;

	if ((pn = argv[0]) != NULL) {
		if ((pn = _strrchr(argv[0], '/')) == NULL)
			pn = argv[0];
		else
			pn++;
	}

	__progname = pn;

	if (ps_strings)
		__ps_strings = ps_strings;

#ifdef DYNAMIC
	if (&_DYNAMIC != NULL)
		_rtld_setup(cleanup, obj);
#endif

#ifdef MCRT0
	atexit(_mcleanup);
	monstartup((u_long)&_eprol, (u_long)&_etext);
#endif

	atexit(_fini);
	_init();

	exit(main(argc, argv, environ));
}

/*
 * NOTE: Leave the RCS ID _after_ _start(), in case it gets placed in .text.
  */
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: crt0.c,v 1.4 2003/04/01 10:20:38 scw Exp $");
#endif /* LIBC_SCCS and not lint */

#include "common.c"
