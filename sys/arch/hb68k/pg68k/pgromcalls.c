/*	$NetBSD: pgromcalls.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $	*/

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: pgromcalls.c,v 1.1 2026/07/19 01:48:24 thorpej Exp $");

#include <sys/param.h>

#include <machine/vectors.h>

#include <dev/cons.h>

#include <hb68k/pg68k/pgromcalls.h>

/*
 * Make sure this ends up in the .data segment so that if we have to
 * re-initialize the .bss segment, this won't get clobbered.
 */
static const struct pgromcalls *pgromvec = NULL;

#define	VERSCHECK(x)	(pgromvec != NULL && pgromvec->rv1_version >= (x))

static int
pgromcall_cngetc(dev_t dev)
{
	int rv = 0;

	if (VERSCHECK(1)) {
		while ((rv = (*pgromvec->rv1_cnpollc)()) == -1) {
			/* loop */;
		}
	}
	return rv;
}

static void
pgromcall_cnputc(dev_t dev, int c)
{
	if (VERSCHECK(1)) {
		(*pgromvec->rv1_cnputc)(c);
	}
}

static struct consdev pgromcall_consdev = {
	.cn_getc  = pgromcall_cngetc,
	.cn_putc  = pgromcall_cnputc,
	.cn_pollc = nullcnpollc,
};

void
pgromcall_init(void)
{
	u_long *addr;

	/*
	 * The pg68k firmware puts the ROM call vector immediately
	 * following the firmware's vector table.  We assume we're
	 * still on the firmware's vectors when this is called.
	 */

	__asm volatile("movc %%vbr,%0" : "=r" (addr));
	pgromvec = (void *)(&addr[NVECTORS]);

	cn_set_tab(&pgromcall_consdev);
	printf("PG68K ROM call vector initialized.\n");
}

void
pgromcall_reboot(void)
{
	if (VERSCHECK(1)) {
		(*pgromvec->rv1_reboot)();
	}
}

void
pgromcall_halt(void)
{
	if (VERSCHECK(1)) {
		(*pgromvec->rv1_halt)();
	}
}

void
pgromcall_poweroff(void)
{
	if (VERSCHECK(1)) {
		(*pgromvec->rv1_poweroff)();
	}
}

void
pgromcall_diag(uint8_t upper, uint8_t lower)
{
	if (VERSCHECK(1)) {
		(*pgromvec->rv1_diag)(upper, lower);
	}
}
