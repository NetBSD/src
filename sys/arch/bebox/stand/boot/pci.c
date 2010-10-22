/*	$NetBSD: pci.c,v 1.4.96.1 2010/10/22 07:21:08 uebayasi Exp $	*/

/*
 * Copyright (C) 1995-1997 Gary Thomas (gdt@linuxppc.org)
 * All rights reserved.
 *
 * Adapted from a program by:
 *                                      Steve Sellgren
 *                                      San Francisco Indigo Company
 *                                      sfindigo!sellgren@uunet.uu.net
 * Adapted for Moto boxes by:
 *                                      Pat Kane & Mark Scott, 1996
 * Fixed for IBM/PowerStack II          Pat Kane 1997
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
#include <dev/pci/pcireg.h>
#include "boot.h"

#define NSLOTS 5
#define NPCIREGS  10


/*
 * should use devfunc number/indirect method to be totally safe on
 * all machines, this works for now on 3 slot Moto boxes
 */

#define PCI_CONFIG_SPACE_BASE	0x80800000
#define PCI_CONFIG_SPACE(d, f)	\
		(u_long *)(PCI_CONFIG_SPACE_BASE | (1 << (d)) | ((f) << 8))

struct PCI_ConfigInfo {
	u_long *config_addr;
	u_long regs[NPCIREGS];
} PCI_slots [NSLOTS] = {
	{ PCI_CONFIG_SPACE(11, 0), { 0xDE, 0xAD, 0xBE, 0xEF } },
	{ PCI_CONFIG_SPACE(12, 0), { 0xDE, 0xAD, 0xBE, 0xEF } },
	{ PCI_CONFIG_SPACE(13, 0), { 0xDE, 0xAD, 0xBE, 0xEF } },
	{ PCI_CONFIG_SPACE(14, 0), { 0xDE, 0xAD, 0xBE, 0xEF } },
	{ PCI_CONFIG_SPACE(15, 0), { 0xDE, 0xAD, 0xBE, 0xEF } },
};


#define DEVID		(PCI_ID_REG >> 2)
#define CMD		(PCI_COMMAND_STATUS_REG >> 2)
#define CLASS		(PCI_CLASS_REG >> 2)
#define BAR_BASE	(PCI_MAPREG_START >> 2)

/*
 * The following code modifies the PCI Command register
 * to enable memory and I/O accesses.
 */
void
enablePCI(int slot, int io, int mem, int master)
{
	volatile u_char *ppci;
	u_char enable = 0;

	if (io)
		enable |= PCI_COMMAND_IO_ENABLE;
	if (mem)
		enable |= PCI_COMMAND_MEM_ENABLE;
	if (master)
		enable |= PCI_COMMAND_MASTER_ENABLE;

	ppci = (u_char *)&PCI_slots[slot].config_addr[CMD];
	*ppci = enable;
	__asm volatile("eieio");
}

void
scanPCI(void)
{
	struct PCI_ConfigInfo *pslot;
	int slt, r;

	for (slt = 0; slt < NSLOTS; slt++) {
		pslot = &PCI_slots[slt];
		for (r = 0; r < NPCIREGS; r++)
			pslot->regs[r] = bswap32(pslot->config_addr[r]);
	}
}

int
findPCIVga(void)
{
	struct PCI_ConfigInfo *pslot;
	int theSlot = -1;
	int highVgaSlot = -1;
	int slt;

	for (slt = 0; slt < NSLOTS; slt++) {
		pslot = &PCI_slots[slt];
		if (pslot->regs[DEVID] != 0xffffffff) {	/* card in slot ? */
			if (PCI_CLASS(pslot->regs[CLASS]) ==
			    PCI_CLASS_DISPLAY) {
				highVgaSlot = slt;
				if ((pslot->regs[CMD] & 0x03)) { /* did firmware enable it ? */
					theSlot = slt;
				}
			}
		}
	}
	if (theSlot == -1)
		theSlot = highVgaSlot;

	return theSlot;
}

int
PCISlotnum(u_int bus, u_int dev, u_int func)
{
	u_long *tag;
	int i;

	if (bus != 0 ||
	    dev < 11 || dev > 15 ||
	    func > 7)
		return -1;

	tag = PCI_CONFIG_SPACE(dev, func);
	for (i = 0; i < sizeof(PCI_slots) / sizeof(struct PCI_ConfigInfo); i++)
		if (tag == PCI_slots[i].config_addr)
			return i;
	return -1;
}

/* return Vendor ID of card in the slot */
int
PCIVendor(int slotnum)
{
	struct PCI_ConfigInfo *pslot;

	pslot = &PCI_slots[slotnum];

	return pslot->regs[DEVID] & 0xffff;
}

/* return mapped address for I/O or Memory */
u_long
PCIAddress(int slotnum, u_int bar, int type)
{
	struct PCI_ConfigInfo *pslot;

	if (bar >= 6)
		return 0xffffffff;

	pslot = &PCI_slots[slotnum];

	if (pslot->regs[DEVID] == 0xffffffff ||
	    PCI_MAPREG_TYPE(pslot->regs[BAR_BASE + bar]) != type)
		return 0xffffffff;

	return PCI_MAPREG_MEM_ADDR(pslot->regs[BAR_BASE + bar]);
}

#ifdef DEBUG
void
printPCIslots(void)
{
	int i;
	for (i = 0; i < NSLOTS; i++) {
		printf("PCI Slot number: %d", i);
		printf(" Vendor ID: 0x%x\n", PCIVendor(i));
	}
}
#endif /* DEBUG */
