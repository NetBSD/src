/*	$NetBSD: leds.c,v 1.19.2.1 2011/06/06 09:05:37 jruoho Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1986, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: machdep.c 1.74 92/12/20$
 *
 *	@(#)machdep.c	8.10 (Berkeley) 4/20/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: leds.c,v 1.19.2.1 2011/06/06 09:05:37 jruoho Exp $");

#include <sys/param.h>

#include <uvm/uvm_extern.h>

#include <hp300/hp300/leds.h>

static vaddr_t	ledbase;	/* kva of LED page */
uint8_t		*ledaddr;	/* actual address of LEDs */
static volatile uint8_t currentleds; /* current LED status */

/*
 * Map the LED page and setup the KVA to access it.
 */
void
ledinit(void)
{

	ledbase = uvm_km_alloc(kernel_map, PAGE_SIZE, 0, UVM_KMF_VAONLY);
	pmap_enter(pmap_kernel(), ledbase, (paddr_t)LED_ADDR,
	    VM_PROT_READ|VM_PROT_WRITE, VM_PROT_READ|VM_PROT_WRITE|PMAP_WIRED);
	pmap_update(pmap_kernel());
	ledaddr = (uint8_t *)(ledbase | (LED_ADDR & PGOFSET));
}

/*
 * Do lights:
 *	`ons' is a mask of LEDs to turn on,
 *	`offs' is a mask of LEDs to turn off,
 *	`togs' is a mask of LEDs to toggle.
 * Note that we don't use splclock/splx for mutual exclusion.  They are
 * expensive and we really don't need to be that precise.  Besides, we
 * would like to be able to profile this routine.
 */
void
ledcontrol(int ons, int offs, int togs)
{

	__asm volatile (
		"orb	%1,%0	;\n"
		"andb	%2,%0	;\n"
		"eorb	%3,%0"
	    : "=m" (currentleds)
	    : "d" (ons), "d" (~offs), "d" (togs));
	*ledaddr = ~currentleds;
}
