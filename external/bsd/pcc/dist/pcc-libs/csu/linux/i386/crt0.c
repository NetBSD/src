/*	$Id: crt0.c,v 1.1.1.1 2008/08/24 05:34:46 gmcgarry Exp $	*/
/*-
 * Copyright (c) 2008 Gregory McGarry <g.mcgarry@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "common.h"

void __start(int, char **);

asm("	.text				\n"
"	.align	4			\n"
"	.globl	_start			\n"
"_start:				\n"
"	xor %ebp,%ebp			\n"
"	pop %esi			\n"
"	mov %esp,%ecx			\n"
"	and $0xfffffff0,%esp		\n"
"	push %ecx			\n"
"	push %esi			\n"
"	call __start			\n"
"	hlt				\n");

void
__start(int argc, char *argv[])
{

#ifdef PROFILE
	atexit(_mcleanup);
	monstartup((unsigned long)&_eprol, (unsigned long)&_etext);
#endif

	_init();
	atexit(_fini);

	exit(main(argc, argv));
}

#include "common.c"

IDENT("$Id: crt0.c,v 1.1.1.1 2008/08/24 05:34:46 gmcgarry Exp $");
