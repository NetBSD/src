/*	$NetBSD: io.c,v 1.1.62.1 2006/04/22 11:37:54 simonb Exp $	*/

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
#include <sys/bswap.h>
#include "boot.h"

#define PCI_NSLOTS	8
#define PCI_NREGS	5
#define PCI_DEVID	0
#define PCI_CMD		1
#define PCI_CLASS	2
#define PCI_MEMBASE	4


volatile u_char *ISA_io  = (u_char *)0x80000000;
volatile u_char *ISA_mem = (u_char *)0xc0000000;
volatile char *videomem = (char *)0xc00b8000; /* + vram offset */

struct PCI_cinfo {
	u_long *config_addr;
	u_long regs[PCI_NREGS];
} PCI_slots[PCI_NSLOTS] = {
	{ (u_long *)0x80808000, {0xDEADBEEF,} },
	{ (u_long *)0x80800800, {0xDEADBEEF,} },
	{ (u_long *)0x80801000, {0xDEADBEEF,} },
	{ (u_long *)0x80802000, {0xDEADBEEF,} },
	{ (u_long *)0x80804000, {0xDEADBEEF,} },
	{ (u_long *)0x80810000, {0xDEADBEEF,} },
	{ (u_long *)0x80820000, {0xDEADBEEF,} },
	{ (u_long *)0x80840000, {0xDEADBEEF,} },
};

void
outb(int port, char val)
{

	ISA_io[port] = val;
}

inline void
outw(int port, u_int16_t val)
{
        outb(port, val>>8);
        outb(port+1, val);
}

u_char
inb(int port)
{

	return (ISA_io[port]);
}

u_long
local_to_PCI(u_long addr)
{

	return ((addr & 0x7FFFFFFF) | 0x80000000);
}

void
unlockVideo(int slot)
{
	volatile u_int8_t *ppci;

	ppci = (u_int8_t *)PCI_slots[slot].config_addr;
	ppci[4] = 0x0003;	/* enable memory and IO Access */
	ppci[0x10] = 0x00000;	/* Turn off memory mapping */
	ppci[0x11] = 0x00000;	/* mem base = 0 */
	ppci[0x12] = 0x00000;
	ppci[0x13] = 0x00000;
	__asm__ volatile("eieio");

	outb(0x3d4, 0x11);
	outb(0x3d5, 0x0e);   /* unlock CR0-CR7 */
}

int
scan_PCI(int start)
{
	int slot, r;
	struct PCI_cinfo *pslot;
	int VGAslot = -1;
	int highVGAslot = 0;

	for (slot = start + 1; slot < PCI_NSLOTS; slot++) {
		pslot = &PCI_slots[slot];
		for (r = 0; r < PCI_NREGS; r++)
			pslot->regs[r] = bswap32(pslot->config_addr[r]);
		if (pslot->regs[PCI_DEVID] != 0xffffffff) {
			/* we have a card */
			if (((pslot->regs[PCI_CLASS] & 0xffffff00) == 
				0x03000000) ||
			    ((pslot->regs[PCI_CLASS] & 0xffffff00) ==
				0x00010000)) {
				/* it's a VGA card */
				highVGAslot = slot;
				if ((pslot->regs[PCI_CMD] & 0x03)) {
					/* fW enabled it */
					VGAslot = slot;
					break;
				}
			}
		}
	}
	return VGAslot;
}

int
PCI_vendor(int slotnum)
{
	struct PCI_cinfo *pslot = &PCI_slots[slotnum];

	return (pslot->regs[PCI_DEVID] & 0xffff);
}
