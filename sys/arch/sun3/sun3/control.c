/*	$NetBSD: control.c,v 1.23.44.1 2014/08/20 00:03:26 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Adam Glass and Gordon W. Ross.
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
__KERNEL_RCSID(0, "$NetBSD: control.c,v 1.23.44.1 2014/08/20 00:03:26 tls Exp $");

#include <sys/param.h>

#include <machine/pte.h>
#include <sun3/sun3/control.h>

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
	return (get_control_word(CONTROL_ADDR_BUILD(PGMAP_BASE, va)));
}

void
set_pte(vaddr_t va, u_int pte)
{
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
