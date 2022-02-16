/*	$NetBSD: io.c,v 1.8 2022/02/16 23:49:26 riastradh Exp $	*/

/*-
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
 * All rights reserved.
 *
 * PCI/ISA I/O support
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
 *      This product includes software developed by Gary Thomas.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <lib/libsa/stand.h>
#include "boot.h"

volatile u_char *PCI_mem = (u_char *)0xc0000000;
volatile u_char *ISA_io  = (u_char *)0x80000000;
volatile u_char *ISA_mem = (u_char *)0xc0000000;

static int dcache_line_size = 32;

void
outb(int port, u_char val)
{

	ISA_io[port] = val;
}

void
outw(int port, u_short val)
{

	outb(port, val >> 8);
	outb(port + 1, val);
}

u_char
inb(int port)
{

	return ISA_io[port];
}

u_short
inw(int port)
{

	return *((volatile uint16_t *)(&ISA_io[port]));
}

u_short
inwrb(int port)
{

	return le16toh(*((volatile uint16_t *)(&ISA_io[port])));
}

void
writeb(u_long addr, u_char val)
{

	PCI_mem[addr] = val;
}

void
writel(u_long addr, u_long val)
{

	*((u_long *)&PCI_mem[addr]) = htole32(val);
}

u_char
readb(u_long addr)
{

	return PCI_mem[addr];
}

u_short
readw(u_long addr)
{

	return le16toh(*((u_short *)&PCI_mem[addr]));
}

u_long
readl(u_long addr)
{

	return le32toh(*((u_long *)&PCI_mem[addr]));
}

u_long
local_to_PCI(u_long addr)
{

	return (addr & 0x7FFFFFFF) | 0x80000000;
}

void
_wbinv(uint32_t adr, uint32_t siz)
{
	uint32_t bnd;

	asm volatile("eieio" ::: "memory");
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size)
		asm volatile("dcbf 0,%0" :: "r"(adr) : "memory");
	asm volatile("sync" ::: "memory");
}

void
_inv(uint32_t adr, uint32_t siz)
{
	uint32_t bnd, off;

	off = adr & (dcache_line_size - 1);
	adr -= off;
	siz += off;
	asm volatile("eieio" ::: "memory");
	if (off != 0) {
		/* wbinv() leading unaligned dcache line */
		asm volatile("dcbf 0,%0" :: "r"(adr) : "memory");
		if (siz < dcache_line_size)
			goto done;
		adr += dcache_line_size;
		siz -= dcache_line_size;
	}
	bnd = adr + siz;
	off = bnd & (dcache_line_size - 1);
	if (off != 0) {
		/* wbinv() trailing unaligned dcache line */
		asm volatile("dcbf 0,%0" :: "r"(bnd) : "memory"); /* it's OK */
		if (siz < dcache_line_size)
			goto done;
		siz -= off;
	}
	for (bnd = adr + siz; adr < bnd; adr += dcache_line_size) {
		/* inv() intermediate dcache lines if ever */
		asm volatile("dcbi 0,%0" :: "r"(adr) : "memory");
	}
  done:
	asm volatile("sync" ::: "memory");
}
