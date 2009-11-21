/*	$NetBSD: cache.c,v 1.21 2009/11/21 04:16:52 rmind Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross.
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
 * cache flush operations specific to the Sun3 custom MMU
 * all are done using writes to control space
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cache.c,v 1.21 2009/11/21 04:16:52 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/pte.h>
#include <machine/vmparam.h>

#include <sun3/sun3/cache.h>
#include <sun3/sun3/control.h>
#include <sun3/sun3/enable.h>
#include <sun3/sun3/fc.h>
#include <sun3/sun3/machdep.h>

#define	CACHE_LINE	16	/* bytes */
#define	VAC_FLUSH_INCR	512	/* bytes */
#define VADDR_MASK	0xfFFffFF	/* 28 bits */

static void cache_clear_tags(void);

void 
cache_flush_page(vaddr_t pgva)
{
	char *va, *endva;
	int old_dfc, ctl_dfc;
	int data;

	pgva &= (VADDR_MASK & ~PGOFSET);
	pgva |= VAC_FLUSH_BASE;

	/* Set up for writes to control space. */
	__asm volatile ("movc %%dfc, %0" : "=d" (old_dfc));
	ctl_dfc = FC_CONTROL;
	__asm volatile ("movc %0, %%dfc" : : "d" (ctl_dfc));

	/* Write to control space for each cache line. */
	va = (char *) pgva;
	endva = (char *) (pgva + PAGE_SIZE);
	data = VAC_FLUSH_PAGE;

	do {
		__asm volatile ("movsl %0, %1@" : : "d" (data), "a" (va));
		va += VAC_FLUSH_INCR;
	} while (va < endva);

	/* Restore destination function code. */
	__asm volatile ("movc %0, %%dfc" : : "d" (old_dfc));
}

void 
cache_flush_segment(vaddr_t sgva)
{
	char *va, *endva;
	int old_dfc, ctl_dfc;
	int data;

	sgva &= (VADDR_MASK & ~SEGOFSET);
	sgva |= VAC_FLUSH_BASE;

	/* Set up for writes to control space. */
	__asm volatile ("movc %%dfc, %0" : "=d" (old_dfc));
	ctl_dfc = FC_CONTROL;
	__asm volatile ("movc %0, %%dfc" : : "d" (ctl_dfc));

	/* Write to control space for each cache line. */
	va = (char *) sgva;
	endva = (char *) (sgva + cache_size);
	data = VAC_FLUSH_SEGMENT;

	do {
		__asm volatile ("movsl %0, %1@" : : "d" (data), "a" (va));
		va += VAC_FLUSH_INCR;
	} while (va < endva);

	/* Restore destination function code. */
	__asm volatile ("movc %0, %%dfc" : : "d" (old_dfc));
}

void 
cache_flush_context(void)
{
	char *va, *endva;
	int old_dfc, ctl_dfc;
	int data;

	/* Set up for writes to control space. */
	__asm volatile ("movc %%dfc, %0" : "=d" (old_dfc));
	ctl_dfc = FC_CONTROL;
	__asm volatile ("movc %0, %%dfc" : : "d" (ctl_dfc));

	/* Write to control space for each cache line. */
	va = (char *) VAC_FLUSH_BASE;
	endva = (char *) (VAC_FLUSH_BASE + cache_size);
	data = VAC_FLUSH_CONTEXT;

	do {
		__asm volatile ("movsl %0, %1@" : : "d" (data), "a" (va));
		va += VAC_FLUSH_INCR;
	} while (va < endva);

	/* Restore destination function code. */
	__asm volatile ("movc %0, %%dfc" : : "d" (old_dfc));
}

static void 
cache_clear_tags(void)
{
	char *va, *endva;
	int old_dfc, ctl_dfc;
	int data;

	/* Set up for writes to control space. */
	__asm volatile ("movc %%dfc, %0" : "=d" (old_dfc));
	ctl_dfc = FC_CONTROL;
	__asm volatile ("movc %0, %%dfc" : : "d" (ctl_dfc));

	/* Write to control space for each cache line. */
	va = (char *) VAC_CACHE_TAGS;
	endva = (char *) (VAC_CACHE_TAGS + cache_size);
	data = 0;	/* invalid tags */

	do {
		__asm volatile ("movsl %0, %1@" : : "d" (data), "a" (va));
		va += CACHE_LINE;
	} while (va < endva);

	/* Restore destination function code. */
	__asm volatile ("movc %0, %%dfc" : : "d" (old_dfc));
}

void 
cache_enable(void)
{
	int enab_reg;

	if (cache_size == 0)
		return;

	/* Have to invalidate all tags before enabling the cache. */
	cache_clear_tags();

	/* OK, just set the "enable" bit. */
	enab_reg = get_control_byte(SYSTEM_ENAB);
	enab_reg |= ENA_CACHE;
	set_control_byte(SYSTEM_ENAB, enab_reg);

	/* Brag... */
	printf("cache enabled\n");
}
