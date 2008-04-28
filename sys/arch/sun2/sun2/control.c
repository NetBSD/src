/*	$NetBSD: control.c,v 1.6 2008/04/28 20:23:37 martin Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass, Gordon W. Ross, and Matthew Fredette.
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
__KERNEL_RCSID(0, "$NetBSD: control.c,v 1.6 2008/04/28 20:23:37 martin Exp $");

#include <sys/param.h>

#include <machine/pte.h>
#include <sun2/sun2/control.h>

int 
get_context(void)
{
	return (get_control_byte(CONTEXT_REG) & CONTEXT_MASK);
}

void 
set_context(int c)
{
	set_control_byte(CONTEXT_REG, (c & CONTEXT_MASK));
}

u_int 
get_pte(vaddr_t va)
{
	u_int pte;

	pte = get_control_word(CONTROL_ADDR_BUILD(PGMAP_BASE, va));
	if (pte & PG_VALID) {
		/* 
		 * This clears bit 30 (the kernel readable bit, which
		 * should always be set), bit 28 (which should always
		 * be set), bit 26 (the user writable bit, which we
		 * always have tracking the kernel writable bit), and
		 * bit 25 (the fill-on-demand bit, which should always
		 * be set).  In the protection, this leaves bit 29
		 * (the kernel writable bit) and bit 27 (the user
		 * readable bit).  See pte.h for more about this
		 * hack.
		 */
		pte &= ~(0x56000000);
		/*
		 * Flip bit 27 (the user readable bit) to become bit
		 * 27 (the PG_SYSTEM bit).
		 */
		pte ^= (PG_SYSTEM);
	}
	return (pte);
}

void 
set_pte(vaddr_t va, u_int pte)
{
	if (pte & PG_VALID) {
		/* Clear bit 26 (the user writable bit).  */
		pte &= (~0x04000000);
		/*
		 * Flip bit 27 (the PG_SYSTEM bit) to become bit 27
		 * (the user readable bit).
		 */
		pte ^= (PG_SYSTEM);
		/*
		 * Always set bits 30 (the kernel readable bit), bit
		 * 28, and bit 25 (the fill-on-demand bit), and set
		 * bit 26 (the user writable bit) iff bit 29 (the
		 * kernel writable bit) is set *and* bit 27 (the user
		 * readable bit) is set.  This latter bit of logic is
		 * expressed in the bizarre second term below, chosen
		 * because it needs no branches.
		 */
#if (PG_WRITE >> 2) != PG_SYSTEM
#error	"PG_WRITE and PG_SYSTEM definitions don't match!"
#endif
		pte |= 0x52000000
		    | ((((pte & PG_WRITE) >> 2) & pte) >> 1);
	}
	set_control_word(CONTROL_ADDR_BUILD(PGMAP_BASE, va), pte);
}

int 
get_segmap(vaddr_t va)
{
	return (get_control_byte(CONTROL_ADDR_BUILD(SEGMAP_BASE, va)));
}

void 
set_segmap(vaddr_t va, int sme)
{
	set_control_byte(CONTROL_ADDR_BUILD(SEGMAP_BASE, va), sme);
}
