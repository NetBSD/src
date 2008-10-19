/* $Id: crti.c,v 1.1.1.1.6.2 2008/10/19 22:40:22 haad Exp $ */
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

asm(	"	.section .init		\n"
	"	.globl _init	\n"
	"	.align 16	\n"
	"_init:			\n"
	"	push %ebp	\n"
	"	mov %esp,%ebp	\n"
	"	.previous	\n");

asm(	"	.section .fini	\n"
	"	.globl _fini	\n"
	"	.align 16	\n"
	"_fini:			\n"
	"	push %ebp	\n"
	"	mov %esp,%ebp	\n"
	"	.previous	\n");

IDENT("$Id: crti.c,v 1.1.1.1.6.2 2008/10/19 22:40:22 haad Exp $");
