/*	$NetBSD: bonito_iobc.c,v 1.2 2003/07/15 02:43:35 lukem Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * Code to manipulate the I/O Buffer Cache on the BONITO.
 *
 * The BONITO snoops uncached access to memory (e.g. via KSEG1); we only
 * need to deal with the IOBC for DMA to cached memory.
 *
 * Note: This only applies to the 32-bit BONITO; BONITO64's IOBC
 * is coherent.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bonito_iobc.c,v 1.2 2003/07/15 02:43:35 lukem Exp $");

#include <sys/param.h>

#include <machine/locore.h>
#include <machine/intr.h>

#include <mips/bonito/bonitoreg.h>
#include <mips/bonito/bonitovar.h>

#define	CACHECMD_INVAL		0
#define	CACHECMD_WBINV		1
#define	CACHECMD_RDTAG		2
#define	CACHECMD_WQFLUSH	3

#define	IOBC_LINESIZE		32
#define	IOBC_LINESHIFT		5
#define	IOBC_NLINES		4

#define	TAG_LOCK		0x80000000
#define	TAG_WBACK		0x40000000
#define	TAG_PFPEND		0x20000000
#define	TAG_PEND		0x10000000
#define	TAG_MOD			0x08000000
#define	TAG_PFDVAL		0x04000000
#define	TAG_DVAL		0x02000000
#define	TAG_AVAL		0x01000000
#define	TAG_ADDR		0x00ffffff

#define	IOBC_LOCK(s)		(s) = splhigh()
#define	IOBC_UNLOCK(s)		splx((s))

static void
bonito_iobc_cmd(uint32_t cmd, uint32_t line)
{
	uint32_t ctrl;

	ctrl = (cmd << BONITO_PCICACHECTRL_CACHECMD_SHIFT) |
	       (line << BONITO_PCICACHECTRL_CACHECMDLINE_SHIFT);

	REGVAL(BONITO_PCICACHECTRL) = ctrl;
	wbflush();

	REGVAL(BONITO_PCICACHECTRL) = ctrl | BONITO_PCICACHECTRL_CMDEXEC;
	wbflush();

	while (REGVAL(BONITO_PCICACHECTRL) & BONITO_PCICACHECTRL_CMDEXEC)
		/* spin */ ;

	REGVAL(BONITO_PCICACHECTRL) = ctrl;
	wbflush();
}

/*
 * bonito_iobc_wbinv_range:
 *
 *	Write-back and invalidate the specified range in
 *	the BONITO IOBC.
 */
void
bonito_iobc_wbinv_range(paddr_t pa, psize_t size)
{
	uint32_t line, tag;
	paddr_t tagaddr;
	int s;

	IOBC_LOCK(s);

	for (line = 0; line < IOBC_NLINES; line++) {
		bonito_iobc_cmd(CACHECMD_RDTAG, line);
		tag = REGVAL(BONITO_PCICACHETAG);
		if (tag & TAG_AVAL) {
			tagaddr = (tag & TAG_ADDR) << IOBC_LINESHIFT;
			if (tagaddr < (pa + size) &&
			    (tagaddr + IOBC_LINESIZE) > pa)
				bonito_iobc_cmd(CACHECMD_WBINV, line);
		}
	}
	bonito_iobc_cmd(CACHECMD_WQFLUSH, 0);

	IOBC_UNLOCK(s);
}

/*
 * bonito_iobc_inv_range:
 *
 *	Invalidate the specified range in the BONITO IOBC.
 */
void
bonito_iobc_inv_range(paddr_t pa, psize_t size)
{
	uint32_t line, tag;
	paddr_t tagaddr;
	int s;

	IOBC_LOCK(s);

	for (line = 0; line < IOBC_NLINES; line++) {
		bonito_iobc_cmd(CACHECMD_RDTAG, line);
		tag = REGVAL(BONITO_PCICACHETAG);
		if (tag & TAG_AVAL) {
			tagaddr = (tag & TAG_ADDR) << IOBC_LINESHIFT; 
			if (tagaddr < (pa + size) &&
			    (tagaddr + IOBC_LINESIZE) > pa) 
				bonito_iobc_cmd(CACHECMD_INVAL, line);
		}
	}
	bonito_iobc_cmd(CACHECMD_WQFLUSH, 0);  

	IOBC_UNLOCK(s);
}
