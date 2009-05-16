/*	$NetBSD: mca_machdep.c,v 1.34.18.3 2009/05/16 10:41:15 yamt Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: mca_machdep.c,v 1.34.18.3 2009/05/16 10:41:15 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <machine/bioscall.h>
#include <machine/psl.h>
#include <machine/bus.h>
#include <machine/bus_private.h>
#include <machine/pio.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isareg.h>
#include <dev/mca/mcavar.h>
#include <dev/mca/mcareg.h>

#include "isa.h"
#include "opt_mcaverbose.h"

/* System Configuration Block - this info is returned by the BIOS call */
struct bios_config {
	uint16_t	count;
	uint8_t		model;
	uint8_t		submodel;
	uint8_t		bios_rev;
	uint8_t		feature1;
#define FEATURE_MCAISA	0x01	/* Machine contains both MCA and ISA bus */
#define FEATURE_MCABUS	0x02	/* Machine has MCA bus instead of ISA	*/
#define FEATURE_EBDA	0x04	/* Extended BIOS data area allocated	*/
#define FEATURE_WAITEV	0x08	/* Wait for external event is supported	*/
#define FEATURE_KBDINT	0x10	/* Keyboard intercept called by Int 09h	*/
#define FEATURE_RTC	0x20	/* Real-time clock present		*/
#define FEATURE_IC2	0x40	/* Second interrupt chip present	*/
#define FEATURE_DMA3	0x80	/* DMA channel 3 used by hard disk BIOS	*/
	uint8_t		feature2;
	uint8_t		pad[9];
} __packed;

/*
 * Used to encode DMA channel into ISA DMA cookie. We use upper 4 bits of
 * ISA DMA cookie id_flags, it's unused.
 */
struct x86_isa_dma_cookie {
	int id_flags;
	/* We don't care about rest */
};

#ifdef UNUSED
static void	_mca_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);
#endif

/*
 * For now, we use MCA DMA to 0-16M always. Some IBM PS/2 have 32bit MCA bus,
 * but majority of them have 24bit only.
 */
#define	MCA_DMA_BOUNCE_THRESHOLD	(16 * 1024 * 1024)

struct x86_bus_dma_tag mca_bus_dma_tag = {
	0,
	MCA_DMA_BOUNCE_THRESHOLD,		/* _bounce_thresh */
	0,					/* _bounce_alloc_lo */
	MCA_DMA_BOUNCE_THRESHOLD,		/* _bounce_alloc_hi */
	NULL,					/* _may_bounce */
	_bus_dmamap_create,
	_bus_dmamap_destroy,
	_bus_dmamap_load,
	_bus_dmamap_load_mbuf,
	_bus_dmamap_load_uio,
	_bus_dmamap_load_raw,
	_bus_dmamap_unload,
	_bus_dmamap_sync,
	_bus_dmamem_alloc,
	_bus_dmamem_free,
	_bus_dmamem_map,
	_bus_dmamem_unmap,
	_bus_dmamem_mmap,
	_bus_dmatag_subregion,
	_bus_dmatag_destroy,
};

/* Updated in mca_busprobe() if appropriate. */
int MCA_system = 0;

/* Used to kick MCA DMA controller */
#define DMA_CMD		0x18		/* command the controller */
#define DMA_EXEC	0x1A		/* tell controller how to do things */
static bus_space_handle_t dmaiot, dmacmdh, dmaexech;

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

/*
 * Map the MCA DMA controller registers.
 */
void
mca_attach_hook(device_t parent, device_t self,
    struct mcabus_attach_args *mba)
{
	dmaiot = mba->mba_iot;

	if (bus_space_map(dmaiot, DMA_CMD, 1, 0, &dmacmdh)
	    || bus_space_map(dmaiot, DMA_EXEC, 1, 0, &dmaexech))
		panic("mca: couldn't map DMA registers");
}

/*
 * Read value of MCA POS register "reg" in slot "slot".
 */

int
mca_conf_read(mca_chipset_tag_t mc, int slot, int reg)
{
	int	data;

	slot &= 7;	/* slot must be in range 0-7 */	
	outb(MCA_MB_SETUP_REG, 0xff); /* ensure m/board setup is disabled */
	outb(MCA_ADAP_SETUP_REG, slot | MCA_ADAP_SET);
	data = inb(MCA_POS_REG(reg));
	outb(MCA_ADAP_SETUP_REG, 0);
	return data;
}


/*
 * Write "data" to MCA POS register "reg" in slot "slot".
 */

void
mca_conf_write(mca_chipset_tag_t mc, int slot, int reg, int data)
{
	slot&=7;	/* slot must be in range 0-7 */	
	outb(MCA_MB_SETUP_REG, 0xff); /* ensure m/board setup is disabled */
	outb(MCA_ADAP_SETUP_REG, slot | MCA_ADAP_SET);
	outb(MCA_POS_REG(reg), data);
	outb(MCA_ADAP_SETUP_REG, 0);
}

#if NISA <= 0
#error mca_intr_(dis)establish: needs ISA to be configured into kernel
#endif

#if 0
const struct evcnt *
mca_intr_establish(mca_chipset_tag_t mc, mca_intr_handle_t ih)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}
#endif

void *
mca_intr_establish(mca_chipset_tag_t mc, mca_intr_handle_t ih,
    int level, int (*func)(void *), void *arg)
{
	if (ih == 0 || ih >= NUM_LEGACY_IRQS || ih == 2)
		panic("mca_intr_establish: bogus handle 0x%x", ih);

	/* MCA interrupts are always level-triggered */
	return isa_intr_establish(NULL, ih, IST_LEVEL, level, func, arg);
}

void
mca_intr_disestablish(mca_chipset_tag_t mc, void *cookie)
{
	isa_intr_disestablish(NULL, cookie);
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
		return (x86_nmi());
	} else
		return(0);
}

/*
 * We can obtain the information about MCA bus presence via
 * GET CONFIGURATION BIOS call - int 0x15, function 0xc0.
 * The call returns a pointer to memory place with the configuration block
 * in es:bx (on AT-compatible, e.g. all we care about, computers).
 *
 * Configuration block contains block length (2 bytes), model
 * number (1 byte), submodel number (1 byte), BIOS revision
 * (1 byte) and up to 5 feature bytes. We only care about
 * first feature byte.
 */
void
mca_busprobe(void)
{
	struct bioscallregs regs;
	struct bios_config *scp;
	paddr_t             paddr;
	char buf[80];

	memset(&regs, 0, sizeof(regs));
	regs.AH = 0xc0;
	bioscall(0x15, &regs);

	if ((regs.EFLAGS & PSL_C) || regs.AH != 0) {
		aprint_verbose("BIOS CFG: Not supported. Not AT-compatible?\n");
		return;
	}

	paddr = (regs.ES << 4) + regs.BX;
	scp = (struct bios_config *)ISA_HOLE_VADDR(paddr);

	snprintb(buf, sizeof(buf),
		"\20"
		"\01MCA+ISA"
		"\02MCA"
		"\03EBDA"
		"\04WAITEV"
		"\05KBDINT"
		"\06RTC"
		"\07IC2"
		"\010DMA3B"
		"\011res"
		"\012DSTR"
		"\013n8042"
		"\014CPUF"
		"\015MMF"
		"\016GPDF"
		"\017KBDF"
		"\020DMA32\n", (scp->feature2 << 8) | scp->feature1);

	aprint_verbose("BIOS CFG: Model-SubM-Rev: %02x-%02x-%02x, 0x%s\n",
		scp->model, scp->submodel, scp->bios_rev, buf);

	MCA_system = (scp->feature1 & FEATURE_MCABUS) ? 1 : 0;
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

#ifdef UNUSED
/*
 * Synchronize a MCA DMA map.
 */
static void
_mca_bus_dmamap_sync(bus_dma_tag_t t, bus_dmamap_t map, bus_addr_t offset,
    bus_size_t len, int ops)
{
	struct x86_isa_dma_cookie *cookie;
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

	cookie = (struct x86_isa_dma_cookie *)map->_dm_cookie;
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
	int error;
	struct x86_isa_dma_cookie *cookie;

#ifdef DEBUG
	/* Sanity check */
	if (dmach < 0 || dmach >= 16) {
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

	cookie = (struct x86_isa_dma_cookie *) (*dmamp)->_dm_cookie;

	if (cookie == NULL) {
		/*
		 * Allocate our cookie if not yet done.
		 */
		cookie = malloc(sizeof(struct x86_bus_dma_cookie), M_DMAMAP,
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

	return (0);
}

/*
 * Set I/O port for DMA. Implemented separately from _mca_bus_dmamap_sync()
 * so that it's available for one-shot setup.
 */
void
mca_dma_set_ioport(int dma, uint16_t port)
{
	/* Disable access to dma channel. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_MASK | dma);

	/* Set I/O port to use for DMA */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_SET_IO | dma);
	bus_space_write_1(dmaiot, dmaexech, 0, port & 0xff);
	bus_space_write_1(dmaiot, dmaexech, 0, (port >> 8) & 0xff);

	/* Enable access to DMA channel. */
	bus_space_write_1(dmaiot, dmacmdh, 0, DMACMD_RESET_MASK | dma);
}
