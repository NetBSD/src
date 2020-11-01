/*	$NetBSD: mca_machdep.c,v 1.4 2011/07/18 17:26:56 dyoung Exp $	*/

/*-
 * Copyright (c) 2000, 2001 The NetBSD Foundation, Inc.
 * Copyright (c) 1996-1999 Scott D. Telford.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Scott Telford <s.telford@ed.ac.uk> and Jaromir Dolecek
 * <jdolecek@NetBSD.org>.
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
 * Machine-specific functions for MCA autoconfiguration.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mca_machdep.c,v 1.4 2011/07/18 17:26:56 dyoung Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <powerpc/pio.h>
#define _POWERPC_BUS_DMA_PRIVATE
#include <sys/bus.h>

#include <dev/mca/mcavar.h>
#include <dev/mca/mcareg.h>

#include "opt_mcaverbose.h"

#ifdef UNUSED
static void	_mca_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);
#endif

/*
 * For now, we use MCA DMA to 0-16M always. Some IBM PS/2 have 32bit MCA bus,
 * but majority of them have 24bit only.
 */
#define	MCA_DMA_BOUNCE_THRESHOLD	(16 * 1024 * 1024)

/* Updated in mca_busprobe() if appropriate. */
int MCA_system = 0;

//static bus_space_handle_t dmaiot, dmacmdh, dmaexech;

#define MAX_SLAVE_CHANNELS	8
#define MAX_DMA_CHANNELS	16

#define INIT_DMA_CHN_BITMASK()	(0xFFFFFFFF << (32 - MAX_DMA_CHANS))
#define INIT_SLAVE_CHN_BITMASK(slaves)	(0xFFFFFFFF << (32 - (slaves))

#define DMA_AVAIL(chn, bitmask)	((bitmask) & (1 << (31 - (chn))))
#define DMA_ALLOC(chn, bitmask) ((bitmask) &= ~(1 << (31 - (chn))))
#define DMA_FREE(chn, bitmask)	((bitmask) |= (1 << (31 - (chn))))

/*
 * MCA DMA controller commands. The exact sense of individual bits
 * are from Tymm Twillman <tymm@computer.org>, who worked on Linux MCA DMA
 * support.
 */
#define DMACMD_SET_IO		0x00	/* set port (16bit) for i/o transfer */
#define DMACMD_SET_ADDR		0x20	/* set addr (24bit) for i/o transfer */
#define DMACMD_GET_ADDR		0x30	/* get addr (24bit) for i/o transfer */
#define	DMACMD_SET_CNT		0x40	/* set memory size for DMA (16b) */
#define DMACMD_GET_CNT		0x50	/* get count of remaining bytes in DMA*/
#define DMACMD_GET_STATUS	0x60	/* ?? */
#define DMACMD_SET_MODE		0x70	/* set DMA mode */
# define DMACMD_MODE_XFER	0x04	/* do transfer, read by default */
# define DMACMD_MODE_READ	0x08	/* read transfer */
# define DMACMD_MODE_WRITE	0x00	/* write transfer */
# define DMACMD_MODE_IOPORT	0x01	/* DMA from/to IO register */
# define DMACMD_MODE_16BIT	0x40	/* 16bit transfers (default 8bit) */
#define DMACMD_SET_ARBUS	0x80	/* ?? */
#define DMACMD_MASK		0x90	/* command mask */
#define DMACMD_RESET_MASK	0xA0	/* reset */
#define DMACMD_MASTER_CLEAR	0xD0	/* ?? */

const struct evcnt *
mca_intr_evcnt(mca_chipset_tag_t ic, int irq)
{
	/* XXX for now, no evcnt parent reported */
	return NULL;
}

/*
 * Map the MCA DMA controller registers.
 */
void
mca_attach_hook(device_t parent, device_t self, struct mcabus_attach_args *mba)
{
#if 0
	dmaiot = mba->mba_iot;

	if (bus_space_map(dmaiot, DMA_CMD, 1, 0, &dmacmdh)
	    || bus_space_map(dmaiot, DMA_EXEC, 1, 0, &dmaexech))
		panic("mca: couldn't map DMA registers");
#endif
}

/*
 * Read value of MCA POS register "reg" in slot "slot".
 */

int
mca_conf_read(mca_chipset_tag_t mc, int slot, int reg)
{
	int	data;

	slot &= 15;	/* slot must be in range 0-15 */
	data = inb(RS6000_BUS_SPACE_IO + MCA_POS_REG(reg) + (slot<<16));
	return data;
}


/*
 * Write "data" to MCA POS register "reg" in slot "slot".
 */

void
mca_conf_write(mca_chipset_tag_t mc, int slot, int reg, int data)
{
	slot &= 15;	/* slot must be in range 0-15 */
	outb(RS6000_BUS_SPACE_IO + MCA_POS_REG(reg) + (slot<<16), data);
}


void *
mca_intr_establish(mca_chipset_tag_t mc, mca_intr_handle_t ih,
    int level, int (*func)(void *), void *arg)
{
	if (ih == 0 || ih >= ICU_LEN)
		panic("mca_intr_establish: bogus handle 0x%x", ih);

	/* MCA interrupts are always level-triggered */
	return intr_establish(ih, IST_LEVEL, level, func, arg);
}

void
mca_intr_disestablish(mca_chipset_tag_t mc, void *cookie)
{
	intr_disestablish(cookie);
}
	

/*
 * Handle a NMI.
 * return true to panic system, false to ignore.
 */
int
mca_nmi(void)
{
	/*
	* PS/2 MCA devices can generate NMIs - we can find out which
	* slot generated it from the POS registers.
	*/
 
	int 	slot, mcanmi=0;

	/* if there is no MCA bus, call x86_nmi() */
	if (!MCA_system)
		goto out;

	/* ensure motherboard setup is disabled */
	outb(MCA_MB_SETUP_REG, 0xff);

	/* find if an MCA slot has the CHCK bit asserted (low) in POS 5 */
	for(slot=0; slot<MCA_MAX_SLOTS; slot++) {
		outb(MCA_ADAP_SETUP_REG, slot | MCA_ADAP_SET);
		if ((inb(MCA_POS_REG(5)) & MCA_POS5_CHCK) == 0) {
			mcanmi = 1;
			/* find if CHCK status is available in POS 6/7 */
			if((inb(MCA_POS_REG(5)) & MCA_POS5_CHCK_STAT) == 0)
				log(LOG_CRIT, "MCA NMI: slot %d, POS6=0x%02x, POS7=0x%02x\n",
					slot+1, inb(MCA_POS_REG(6)),
						inb(MCA_POS_REG(7)));
			else
				log(LOG_CRIT, "MCA NMI: slot %d\n", slot+1);
		}
	}
	outb(MCA_ADAP_SETUP_REG, 0);

   out:
	if (!mcanmi) {
		/* no CHCK bits asserted, assume ISA NMI */
		//return (x86_nmi());
		return 0;
	} else
		return(0);
}

/*
 * Realistically, we should probe for the presence of an MCA bus here, and
 * return a reasonable value.  However, this port is never expected to run
 * on anything other than MCA, so rather than write a bunch of complex code
 * to find that we indeed have a bus, lets just assume we do.
 */
void
mca_busprobe(void)
{
	MCA_system = 1;
}

#define PORT_DISKLED	0x92
#define DISKLED_ON	0x40

/*
 * Light disk busy LED on IBM PS/2.
 */
void
mca_disk_busy(void)
{
	outb(PORT_DISKLED, inb(PORT_DISKLED) | DISKLED_ON);
}

/*
 * Turn off disk LED on IBM PS/2.
 */
void
mca_disk_unbusy(void)
{
	outb(PORT_DISKLED, inb(PORT_DISKLED) & ~DISKLED_ON);
}

/*
 * -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * MCA DMA specific stuff. We use ISA routines for bulk of the work,
 * since MCA shares much of the charasteristics with it. We just hook
 * the DMA channel initialization and kick MCA DMA controller appropriately.
 * -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 */

#ifdef NOTYET
/*
 * Synchronize a MCA DMA map.
 */
static void
_mca_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct rs6000_dma_cookie *cookie;
	bus_addr_t phys;
	bus_size_t cnt;
	int dmach, mode;

	_bus_dmamap_sync(t, map, offset, len, ops);

	/*
	 * Don't do anything if not using the DMA controller.
	 */
	if ((map->_dm_flags & _MCABUS_DMA_USEDMACTRL) == 0)
		return;

	/*
	 * Don't do anything if not PRE* operation, allow only
	 * one of PREREAD and PREWRITE.
	 */
	if (ops != BUS_DMASYNC_PREREAD && ops != BUS_DMASYNC_PREWRITE)
		return;

	cookie = (struct rs6000_dma_cookie *)map->_dm_cookie;
	dmach = (cookie->id_flags & 0xf0) >> 4;

	phys = map->dm_segs[0].ds_addr;
	cnt = map->dm_segs[0].ds_len;

	mode = DMACMD_MODE_XFER;
	mode |= (ops == BUS_DMASYNC_PREREAD)
			? DMACMD_MODE_READ : DMACMD_MODE_WRITE;
	if (map->_dm_flags & MCABUS_DMA_IOPORT)
		mode |= DMACMD_MODE_IOPORT;

	/* Use 16bit DMA if requested */
	if (map->_dm_flags & MCABUS_DMA_16BIT) {
#ifdef DIAGNOSTIC
		if ((cnt % 2) != 0) {
			panic("_mca_bus_dmamap_sync: 16bit DMA and cnt %lu odd",
				cnt);
		}
#endif 
		mode |= DMACMD_MODE_16BIT;
		cnt /= 2;
	}

	/*
	 * Initialize the MCA DMA controller appropriately. The exact
	 * sequence to setup the controller is taken from Minix.
	 */

	/* Disable access to DMA channel. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_MASK | dmach);

	/* Set the transfer mode. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_SET_MODE | dmach);
	bus_space_write_1(dmaiot, dmaexech, 0, mode);

	/* Set the address byte pointer. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_SET_ADDR | dmach);
	/* address bits 0..7   */
	bus_space_write_1(dmaiot, dmaexech, 0, (phys >> 0) & 0xff);
	/* address bits 8..15  */
	bus_space_write_1(dmaiot, dmaexech, 0, (phys >> 8) & 0xff);
	/* address bits 16..23  */
	bus_space_write_1(dmaiot, dmaexech, 0, (phys >> 16) & 0xff);

	/* Set the count byte pointer */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_SET_CNT | dmach);
	/* count bits 0..7     */
	bus_space_write_1(dmaiot, dmaexech, 0, ((cnt - 1) >> 0) & 0xff);        
	/* count bits 8..15    */
	bus_space_write_1(dmaiot, dmaexech, 0, ((cnt - 1) >> 8) & 0xff);        

	/* Enable access to DMA channel. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_RESET_MASK | dmach);
}
#endif

/*
 * Allocate a DMA map, and set up DMA channel.
 */
int
mca_dmamap_create(bus_dma_tag_t t, bus_size_t size, int flags,
    bus_dmamap_t *dmamp, int dmach)
{
#if 0
	int error;
	struct rs6000_dma_cookie *cookie;

#ifdef DEBUG
	/* Sanity check */
	if (dmach < 0 || dmach >= MAX_DMA_CHANNELS) {
		printf("mcadma_create: invalid DMA channel %d\n",
			dmach);
		return (EINVAL);
	}

	if (size > 65536) {
		panic("mca_dmamap_create: dmamap sz %ld > 65536",
		    (long) size);
	}
#endif

	/*
	 * MCA DMA transfer can be maximum 65536 bytes long and must
	 * be in one chunk. No specific boundary constraints are present.
	 */
	if ((error = _bus_dmamap_create(t, size, 1, 65536, 0, flags, dmamp)))
		return (error);

	cookie = (struct rs6000_dma_cookie *) (*dmamp)->_dm_cookie;

	if (cookie == NULL) {
		/*
		 * Allocate our cookie if not yet done.
		 */
		cookie = malloc(sizeof(struct rs6000_dma_cookie), M_DMAMAP,
		    ((flags & BUS_DMA_NOWAIT) ? M_NOWAIT : M_WAITOK) | M_ZERO);
		if (cookie == NULL) {
			
			return ENOMEM;
		}
		(*dmamp)->_dm_cookie = cookie;
	}


	/* Encode DMA channel */
	cookie->id_flags &= 0x0f;
	cookie->id_flags |= dmach << 4;

	/* Mark the dmamap as using DMA controller. Some devices
	 * drive DMA themselves, and don't need the MCA DMA controller.
	 * To distinguish the two, use a flag for dmamaps which use the DMA
	 * controller.
 	 */
	(*dmamp)->_dm_flags |= _MCABUS_DMA_USEDMACTRL;
#endif
	return (0);
}

/*
 * Set I/O port for DMA. Implemented separately from _mca_bus_dmamap_sync()
 * so that it's available for one-shot setup.
 */
void
mca_dma_set_ioport(int dma, uint16_t port)
{
#if 0
	/* Disable access to dma channel. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_MASK | dma);

	/* Set I/O port to use for DMA */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_SET_IO | dma);
	bus_space_write_1(dmaiot, dmaexech, 0, port & 0xff);
	bus_space_write_1(dmaiot, dmaexech, 0, (port >> 8) & 0xff);

	/* Enable access to DMA channel. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_RESET_MASK | dma);
#endif
}
