/* $NetBSD: rc7500_promsys.s,v 1.1 1996/08/29 22:35:46 mark Exp $ */

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
 * Copyright (c) 1995 Michael L. Hitch
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Michael L. Hitch.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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


#include <machine/asm.h>

sp	.req	r13
lr	.req	r14
pc	.req	r15

ENTRY(prom_reboot)
	stmdb	sp!, {lr}
	SWI	0x06
	ldmia	sp!, {pc}

ENTRY(prom_getenv)
	stmdb	sp!, {lr}
	SWI	0x07
	ldmia	sp!, {pc}

ENTRY(prom_putchar)
	stmdb	sp!, {lr}
	SWI	0x08
	ldmia	sp!, {pc}

ENTRY(prom_puts)
	stmdb	sp!, {r0, lr}
	SWI	0x09
	ldmia	sp!, {r0, pc}

ENTRY(prom_getchar)
	stmdb	sp!, {lr}
	SWI	0x0A
	ldmia	sp!, {pc}

ENTRY(prom_open)
	stmdb	sp!, {lr}
	SWI	0x0B
	ldmia	sp!, {pc}

ENTRY(prom_close)
	stmdb	sp!, {lr}
	SWI	0x0C
	ldmia	sp!, {pc}

ENTRY(prom_read)
	stmdb	sp!, {lr}
	SWI	0x0D
	ldmia	sp!, {pc}

