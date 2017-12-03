/*	$NetBSD: if_we_vme.c,v 1.3.12.1 2017/12/03 11:35:58 jdolecek Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 * adapters.
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 */

/*
 * Device driver for the SMC Elite Ultra (8216) with SMC_TT VME-ISA bridge.
 * Based on:
 *	NetBSD: if_we_isa.c,v 1.20 2008/04/28 20:23:52 martin Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_we_vme.c,v 1.3.12.1 2017/12/03 11:35:58 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_media.h>

#include <net/if_ether.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <machine/cpu.h>
#include <machine/iomap.h>
#include <machine/scu.h>

#include <atari/vme/vmevar.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/wereg.h>
#include <dev/ic/wevar.h>

/* #define WE_DEBUG */
#ifdef WE_DEBUG
#define DPRINTF(x)	printf x
#else
#define DPRINTF(x)	/**/
#endif

/* VME space mapped by SMC_TT VME-ISA bridge */
#define SMCTT_MEM_BASE	0xFE000000	/* base for shared memory space */
#define SMCTT_IOE_BASE	0xFE200000	/* base for I/O ports at even address */
#define SMCTT_IOO_BASE	0xFE300000	/* base for I/O ports at odd address */

#define SMCTT_IO_OFFSET	(SMCTT_IOO_BASE - SMCTT_IOE_BASE)

/* default SMC8216 settings for SMC_TT specified by a jumper switch at No.2 */
#define SMCTT_MEM_ADDR	0xD0000
#define SMCTT_IO_ADDR	0x280

/* SMC_TT uses IRQ4 on VME, IRQ3 on ISA, and interrupt vector 0xAA */
#define SMCTT_VME_IRQ	4
#define SMCTT_ISA_IRQ	3
#define SMCTT_VECTOR	0xAA

static int we_vme_probe(device_t, cfdata_t , void *);
static void we_vme_attach(device_t, device_t, void *);

static uint8_t smctt_bus_space_read_1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t);
static void    smctt_bus_space_write_1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t, uint8_t);
static int     smctt_bus_space_peek_1(bus_space_tag_t, bus_space_handle_t,
    bus_size_t);

struct we_vme_softc {
	struct we_softc	sc_we;
	struct atari_bus_space sc_bs;
};

CFATTACH_DECL_NEW(we_vme, sizeof(struct we_vme_softc),
    we_vme_probe, we_vme_attach, NULL, NULL);

static const int we_790_irq[] = {
	-1, 9, 3, 5, 7, 10, 11, 15,
};

static int
we_vme_probe(device_t parent, cfdata_t cf, void *aux)
{
	struct vme_attach_args *va = aux;
	struct atari_bus_space t;
	bus_space_tag_t asict, memt;
	bus_space_handle_t asich, asich1, memh;
	bus_size_t memsize = 0;
	bool asich_valid, asich1_valid, memh_valid;
	int i, rv;
	uint8_t sum, reg, type, hwr;

	rv = 0;
	asich_valid = false;
	asich1_valid = false;
	memh_valid = false;

	if (va->va_iobase != IOBASEUNK &&
	    va->va_iobase != SMCTT_IOE_BASE + SMCTT_IO_ADDR)
		return 0;
	if (va->va_maddr != IOBASEUNK &&
	    va->va_maddr != SMCTT_MEM_BASE + SMCTT_MEM_ADDR)
		return 0;
	if (va->va_irq != IRQUNK &&
	    va->va_irq != SMCTT_VME_IRQ)
		return 0;

	/* SMC_TT has a bit weird I/O address mappings */
	asict = beb_alloc_bus_space_tag(&t);
	/* XXX setup only simple byte functions used in MI we(4) driver */
	asict->abs_r_1 = smctt_bus_space_read_1;
	asict->abs_w_1 = smctt_bus_space_write_1;
	asict->abs_p_1 = smctt_bus_space_peek_1;

	/*
	 * Only 16 bit accesses are allowed for memory space on SMC_TT,
	 * but MI we(4) uses them on 16 bit mode.
	 */
	memt = va->va_memt;

	/* Attempt to map the device. */
	if (bus_space_map(asict, SMCTT_IOE_BASE + SMCTT_IO_ADDR, WE_NPORTS,
	    0, &asich) != 0) {
		DPRINTF(("%s: failed to map even I/O space", __func__));
		goto out;
	}
	asich_valid = true;

	if (bus_space_map(asict, SMCTT_IOO_BASE + SMCTT_IO_ADDR, WE_NPORTS,
	    0, &asich1) != 0) {
		DPRINTF(("%s: failed to map odd I/O space", __func__));
		goto out;
	}
	asich1_valid = true;

	/* XXX abuse stride for offset of odd ports from even ones */
	asict->stride =
	    (vaddr_t)bus_space_vaddr(asict, asich1) -
	    (vaddr_t)bus_space_vaddr(asict, asich);

	/* check if register regions are valid */
	if (bus_space_peek_1(asict, asich, WE_PROM + 0) == 0 ||
	    bus_space_peek_1(asict, asich, WE_PROM + 1) == 0)
		goto out;

	/*
	 * Attempt to do a checksum over the station address PROM.
	 * If it fails, it's probably not an SMC_TT board.
	 */
	DPRINTF(("%s: WE_PROM: ", __func__));
	sum = 0;
	for (i = 0; i < 8; i++) {
		reg = bus_space_read_1(asict, asich, WE_PROM + i);
		DPRINTF(("%02x ", reg));
		sum += reg;
	}
	DPRINTF(("\n"));
	DPRINTF(("%s: WE_ROM_SUM: 0x%02x\n", __func__, sum));

	if (sum != WE_ROM_CHECKSUM_TOTAL)
		goto out;

	/*
	 * Reset the card to force it into a known state.
	 */
	bus_space_write_1(asict, asich, WE_MSR, WE_MSR_RST);
	delay(100);

	bus_space_write_1(asict, asich, WE_MSR,
	    bus_space_read_1(asict, asich, WE_MSR) & ~WE_MSR_RST);

	/* Wait in case the card is reading its EEPROM. */
	delay(5000);

	/*
	 * Check card type.
	 */
	type = bus_space_read_1(asict, asich, WE_CARD_ID);
	/* Assume SMT_TT has only 8216 */
	if (type != WE_TYPE_SMC8216C && type != WE_TYPE_SMC8216T)
		goto out;

	hwr = bus_space_read_1(asict, asich, WE790_HWR);
	bus_space_write_1(asict, asich, WE790_HWR, hwr | WE790_HWR_SWH);
	switch (bus_space_read_1(asict, asich, WE790_RAR) & WE790_RAR_SZ64) {
	case WE790_RAR_SZ64:
		memsize = 65536;
		break;
	case WE790_RAR_SZ32:
		memsize = 32768;
		break;
	case WE790_RAR_SZ16:
		memsize = 16384;
		break;
	case WE790_RAR_SZ8:
		memsize = 8192;
		break;
	default:
		memsize = 16384;
		break;
	}
	bus_space_write_1(asict, asich, WE790_HWR, hwr);

	/* Attempt to map the memory space. */
	if (bus_space_map(memt, SMCTT_MEM_BASE + SMCTT_MEM_ADDR, memsize,
	    0, &memh) != 0) {
		DPRINTF(("%s: failed to map shared memory", __func__));
		goto out;
	}
	memh_valid = true;

	/* check if memory region is valid */
	if (bus_space_peek_2(memt, memh, 0) == 0)
		goto out;

	/*
	 * Check the assigned interrupt number from the card.
	 */

	/* Assemble together the encoded interrupt number. */
	hwr = bus_space_read_1(asict, asich, WE790_HWR);
	bus_space_write_1(asict, asich, WE790_HWR, hwr | WE790_HWR_SWH);

	reg = bus_space_read_1(asict, asich, WE790_GCR);
	i = ((reg & WE790_GCR_IR2) >> 4) |
	    ((reg & (WE790_GCR_IR1|WE790_GCR_IR0)) >> 2);
	bus_space_write_1(asict, asich, WE790_HWR, hwr & ~WE790_HWR_SWH);

	if (we_790_irq[i] != SMCTT_ISA_IRQ) {
		DPRINTF(("%s: wrong IRQ (%d); check jumper settings\n",
		    __func__, we_790_irq[i]));
		goto out;
	}

	/* So, we say we've found it! */
	va->va_iobase = SMCTT_IOE_BASE + SMCTT_IO_ADDR;
	va->va_iosize = WE_NPORTS;
	va->va_maddr = SMCTT_MEM_BASE + SMCTT_MEM_ADDR;
	va->va_msize = memsize;
	va->va_irq = SMCTT_VME_IRQ;

	rv = 1;

 out:
	if (asich_valid)
		bus_space_unmap(asict, asich, WE_NPORTS);
	if (asich1_valid)
		bus_space_unmap(asict, asich1, WE_NPORTS);
	if (memh_valid)
		bus_space_unmap(memt, memh, memsize);
	return rv;
}

void
we_vme_attach(device_t parent, device_t self, void *aux)
{
	struct we_vme_softc *wvsc = device_private(self);
	struct we_softc *wsc = &wvsc->sc_we;
	struct dp8390_softc *sc = &wsc->sc_dp8390;
	struct vme_attach_args *va = aux;
	bus_space_tag_t nict, asict, memt;
	bus_space_handle_t nich, asich, asich1, memh;
	const char *typestr;

	aprint_normal("\n");

	sc->sc_dev = self;

	/* See comments in the above probe function */
	asict = beb_alloc_bus_space_tag(&wvsc->sc_bs);
	asict->abs_r_1 = smctt_bus_space_read_1;
	asict->abs_w_1 = smctt_bus_space_write_1;
	nict = asict;

	memt = va->va_memt;

	/* Map the device. */
	if (bus_space_map(asict, va->va_iobase, WE_NPORTS, 0, &asich) != 0) {
		aprint_error_dev(self, "can't map even I/O space\n");
		return;
	}
	if (bus_space_map(asict, va->va_iobase + SMCTT_IO_OFFSET, WE_NPORTS,
	    0, &asich1) != 0) {
		aprint_error_dev(self, "can't map odd I/O space\n");
		goto out;
	}
	asict->stride =
	    (vaddr_t)bus_space_vaddr(asict, asich1) -
	    (vaddr_t)bus_space_vaddr(asict, asich);

	if (bus_space_subregion(asict, asich, WE_NIC_OFFSET, WE_NIC_NPORTS,
	    &nich) != 0) {
		aprint_error_dev(self, "can't subregion I/O space\n");
		goto out1;
	}

	/* Map memory space. */
	if (bus_space_map(memt, va->va_maddr, va->va_msize, 0, &memh) != 0) {
		aprint_error_dev(self, "can't map shared memory\n");
		goto out1;
	}

	wsc->sc_asict = asict;
	wsc->sc_asich = asich;

	sc->sc_regt = nict;
	sc->sc_regh = nich;

	sc->sc_buft = memt;
	sc->sc_bufh = memh;

	wsc->sc_maddr = va->va_maddr & 0xfffff;
	sc->mem_size = va->va_msize;

	/* Interface is always enabled. */
	sc->sc_enabled = 1;

	/* SMC_TT assumes SMC8216 */
	sc->is790 = 1;

	/* SMC_TT supports only 16 bit access for shared memory */
	wsc->sc_flags |= WE_16BIT_ENABLE;

	/* Appeal the Atari spirit :-) */
	typestr = "SMC8216 with SMC_TT VME-ISA bridge";

	if (we_config(self, wsc, typestr) != 0)
		goto out2;

	/*
	 * Enable the configured interrupt.
	 */
	bus_space_write_1(asict, asich, WE790_ICR,
	    bus_space_read_1(asict, asich, WE790_ICR) | WE790_ICR_EIL);

	/* Establish interrupt handler. */
	wsc->sc_ih = intr_establish(SMCTT_VECTOR - 64, USER_VEC, 0,
	    (hw_ifun_t)dp8390_intr, sc);
	if (wsc->sc_ih == NULL) {
		aprint_error_dev(self, "can't establish interrupt\n");
		goto out2;
	}
	/*
	 * Unmask the VME interrupt we're on.
	 */
	if ((machineid & ATARI_TT) != 0)
		SCU->vme_mask |= 1 << va->va_irq;

	return;

 out2:
		bus_space_unmap(memt, memh, va->va_msize);
 out1:
		bus_space_unmap(asict, asich1, WE_NPORTS);
 out:
		bus_space_unmap(asict, asich, WE_NPORTS);
}

static uint8_t
smctt_bus_space_read_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg)
{
	uint8_t rv;

	if ((reg & 0x01) != 0) {
		/* odd address space */
		rv = *(volatile uint8_t *)(bh + bt->stride + (reg & ~0x01));
	} else {
		/* even address space */
		rv = *(volatile uint8_t *)(bh + reg);
	}

	return rv;
}

static void
smctt_bus_space_write_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg, uint8_t val)
{

	if ((reg & 0x01) != 0) {
		/* odd address space */
		*(volatile uint8_t *)(bh + bt->stride + (reg & ~0x01)) = val;
	} else {
		/* even address space */
		*(volatile uint8_t *)(bh + reg) = val;
	}
}

static int
smctt_bus_space_peek_1(bus_space_tag_t bt, bus_space_handle_t bh,
    bus_size_t reg)
{
	uint8_t *va;

	if ((reg & 0x01) != 0) {
		/* odd address space */
		va = (uint8_t *)(bh + bt->stride + (reg & ~0x01));
	} else {
		/* even address space */
		va = (uint8_t *)(bh + reg);
	}

	return !badbaddr(va, sizeof(uint8_t));
}
