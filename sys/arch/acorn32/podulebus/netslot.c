/*	$NetBSD: netslot.c,v 1.5 2002/06/19 23:27:48 bjh21 Exp $	*/

/*
 * Copyright (c) 1994-1996 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
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
 *	This product includes software developed by Brini.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY BRINI ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL BRINI OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>

__KERNEL_RCSID(1, "$NetBSD: netslot.c,v 1.5 2002/06/19 23:27:48 bjh21 Exp $");

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <uvm/uvm_extern.h>
#include <machine/io.h>
#include <arm/arm32/katelib.h>
#include <machine/intr.h>
#include <machine/bootconfig.h>
#include <machine/pmap.h>
#include <arm/iomd/iomdreg.h>
#include <arm/iomd/iomdvar.h>
#include <dev/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>

u_int netslotread __P((u_int, int));

u_int
netslotread(address, offset)
	u_int address;
	int offset;
{
	static int netslotoffset = -1;

	offset = offset >> 2;
	if (netslotoffset == -1 || offset < netslotoffset) {
		WriteByte(address, 0);
		netslotoffset = 0;
	}
	while (netslotoffset < offset) {
		(void)ReadByte(address);
		++netslotoffset;
	}
	++netslotoffset;
	return(ReadByte(address));
}

void
netslotscan(dev)
	struct device *dev;
{
	podule_t *podule;
	volatile u_char *address;

	/* Only one netslot atm */

	/* Reset the address counter */

	WriteByte(NETSLOT_BASE, 0x00);

	address = (u_char *)NETSLOT_BASE;

	podule = &podules[MAX_PODULES];

	podule->fast_base = NETSLOT_BASE;
	podule->medium_base = NETSLOT_BASE;
	podule->slow_base = NETSLOT_BASE;
	podule->sync_base = NETSLOT_BASE;
	podule->mod_base = NETSLOT_BASE;
	podule->easi_base = 0;
	podule->attached = 0;
	podule->slottype = SLOT_NONE;
	podule->podulenum = MAX_PODULES;
	podule->interrupt = IRQ_NETSLOT;
	podule->read_rom = netslotread;
	podule->dma_channel = -1;
	podule->dma_interrupt = -1;
	podule->description[0] = 0;

	/* XXX - Really needs to be linked to a DMA manager */
	if (IOMD_ID == RPC600_IOMD_ID)
		podule->dma_channel = 0;

	/* Get information from the podule header */

	podule->flags0 = *address;
	podule->flags1 = *address;
	podule->reserved = *address;
	podule->product = *address;
	podule->product += (*address << 8);
	podule->manufacturer = *address;
	podule->manufacturer += (*address << 8);
	podule->country = *address;
	if (podule->flags1 & PODULE_FLAGS_IS) {
		podule->irq_mask = *address;
		podule->irq_addr = *address;
		podule->irq_addr += (*address << 8);
		podule->irq_addr += (*address << 16);
		podule->irq_addr += podule->slow_base;
		if (podule->irq_mask == 0)
			podule->irq_mask = 0x01;
		podule->fiq_mask = *address;
		podule->fiq_addr = *address;
		podule->fiq_addr += (*address << 8);
		podule->fiq_addr += (*address << 16);
		podule->fiq_addr += podule->slow_base;
		if (podule->fiq_mask == 0)
			podule->fiq_mask = 0x04;
	} else {
		podule->irq_addr = podule->slow_base;
		podule->irq_mask = 0x01;
		podule->fiq_addr = podule->slow_base;
		podule->fiq_mask = 0x04;
	}

	poduleexamine(podule, dev, SLOT_NET);
}

void
netslot_ea(buffer)
	u_int8_t *buffer;
{
	/* Build station address from machine ID */
	buffer[0] = 0x00;
	buffer[1] = 0x00;
	buffer[2] = 0xa4;
	buffer[3] = bootconfig.machine_id[2] + 0x10;
	buffer[4] = bootconfig.machine_id[1];
	buffer[5] = bootconfig.machine_id[0];
}
