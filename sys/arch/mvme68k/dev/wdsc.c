/*	$NetBSD: wdsc.c,v 1.32.30.1 2019/11/14 16:04:31 martin Exp $	*/

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 *  @(#)wdsc.c
 */

/*-
 * Copyright (c) 1996-2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford.
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
__KERNEL_RCSID(0, "$NetBSD: wdsc.c,v 1.32.30.1 2019/11/14 16:04:31 martin Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/autoconf.h>

#include <mvme68k/dev/dmavar.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/sbicreg.h>
#include <mvme68k/dev/sbicvar.h>
#include <mvme68k/dev/wdscreg.h>

#include "ioconf.h"

void    wdsc_pcc_attach(device_t, device_t, void *);
int     wdsc_pcc_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(wdsc_pcc, sizeof(struct sbic_softc),
    wdsc_pcc_match, wdsc_pcc_attach, NULL, NULL);

void    wdsc_enintr(struct sbic_softc *);
int     wdsc_dmago(struct sbic_softc *, char *, int, int);
int     wdsc_dmanext(struct sbic_softc *);
void    wdsc_dmastop(struct sbic_softc *);
int     wdsc_dmaintr(void *);
int     wdsc_scsiintr(void *);

/*
 * Match for SCSI devices on the onboard WD33C93 chip
 */
int
wdsc_pcc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct pcc_attach_args *pa = aux;

	if (strcmp(pa->pa_name, wdsc_cd.cd_name))
		return 0;

	pa->pa_ipl = cf->pcccf_ipl;
	return 1;
}

/*
 * Attach the wdsc driver
 */
void
wdsc_pcc_attach(device_t parent, device_t self, void *aux)
{
	struct sbic_softc *sc;
	struct pcc_attach_args *pa;
	bus_space_handle_t bush;
	static struct evcnt evcnt;	/* XXXSCW: Temporary hack */

	sc = device_private(self);
	sc->sc_dev = self;
	pa = aux;

	bus_space_map(pa->pa_bust, pa->pa_offset, 0x20, 0, &bush);

	/*
	 * XXXSCW: We *need* an MI, bus_spaced WD33C93 driver...
	 */
	sc->sc_sbicp = (sbic_regmap_p) bush;

	sc->sc_driver  = (void *) &evcnt;
	sc->sc_enintr  = wdsc_enintr;
	sc->sc_dmago   = wdsc_dmago;
	sc->sc_dmanext = wdsc_dmanext;
	sc->sc_dmastop = wdsc_dmastop;
	sc->sc_dmacmd  = 0;

	sc->sc_adapter.adapt_dev = self;
	sc->sc_adapter.adapt_nchannels = 1;
	sc->sc_adapter.adapt_openings = 7; 
	sc->sc_adapter.adapt_max_periph = 1;
	sc->sc_adapter.adapt_ioctl = NULL; 
	sc->sc_adapter.adapt_minphys = sbic_minphys;
	sc->sc_adapter.adapt_request = sbic_scsi_request;

	sc->sc_channel.chan_adapter = &sc->sc_adapter;
	sc->sc_channel.chan_bustype = &scsi_bustype;
	sc->sc_channel.chan_channel = 0;
	sc->sc_channel.chan_ntargets = 8;
	sc->sc_channel.chan_nluns = 8;
	sc->sc_channel.chan_id = 7;

	printf(": WD33C93 SCSI, target %d\n", sc->sc_channel.chan_id);

	/*
	 * Everything is a valid DMA address.
	 */
	sc->sc_dmamask = 0;

	/*
	 * The onboard WD33C93 of the '147 is usually clocked at 10MHz...
	 * (We use 10 times this for accuracy in later calculations)
	 */
	sc->sc_clkfreq = 100;

	/*
	 * Initialise the hardware
	 */
	sbicinit(sc);

	/*
	 * Fix up the interrupts
	 */
	sc->sc_ipl = pa->pa_ipl & PCC_IMASK;

	pcc_reg_write(sys_pcc, PCCREG_SCSI_INTR_CTRL, PCC_ICLEAR);
	pcc_reg_write(sys_pcc, PCCREG_DMA_INTR_CTRL, PCC_ICLEAR);
	pcc_reg_write(sys_pcc, PCCREG_DMA_CONTROL, 0);

	evcnt_attach_dynamic(&evcnt, EVCNT_TYPE_INTR, pccintr_evcnt(sc->sc_ipl),
	    "disk", device_xname(self));
	pccintr_establish(PCCV_DMA, wdsc_dmaintr,  sc->sc_ipl, sc, &evcnt);
	pccintr_establish(PCCV_SCSI, wdsc_scsiintr, sc->sc_ipl, sc, &evcnt);
	pcc_reg_write(sys_pcc, PCCREG_SCSI_INTR_CTRL,
            sc->sc_ipl | PCC_IENABLE | PCC_ICLEAR);

	(void)config_found(self, &sc->sc_channel, scsiprint);
}

/*
 * Enable DMA interrupts
 */
void
wdsc_enintr(struct sbic_softc *dev)
{

	dev->sc_flags |= SBICF_INTR;

	pcc_reg_write(sys_pcc, PCCREG_DMA_INTR_CTRL,
            dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR);
}

/*
 * Prime the hardware for a DMA transfer
 */
int
wdsc_dmago(struct sbic_softc *dev, char *addr, int count, int flags)
{

	/*
	 * Set up the command word based on flags
	 */
	if ((flags & DMAGO_READ) == 0)
		dev->sc_dmacmd = DMAC_CSR_ENABLE | DMAC_CSR_WRITE;
	else
		dev->sc_dmacmd = DMAC_CSR_ENABLE;

	dev->sc_flags |= SBICF_INTR;
	dev->sc_tcnt   = dev->sc_cur->dc_count << 1;

	/*
	 * Prime the hardware.
	 * Note, it's probably not necessary to do this here, since dmanext
	 * is called just prior to the actual transfer.
	 */
	pcc_reg_write(sys_pcc, PCCREG_DMA_CONTROL, 0);
	pcc_reg_write(sys_pcc, PCCREG_DMA_INTR_CTRL,
	    dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR);
	pcc_reg_write32(sys_pcc, PCCREG_DMA_DATA_ADDR,
	    (uint32_t) dev->sc_cur->dc_addr);
	pcc_reg_write32(sys_pcc, PCCREG_DMA_BYTE_COUNT,
	    (uint32_t) dev->sc_tcnt | (1 << 24));
	pcc_reg_write(sys_pcc, PCCREG_DMA_CONTROL, dev->sc_dmacmd);

	return dev->sc_tcnt;
}

/*
 * Prime the hardware for the next DMA transfer
 */
int
wdsc_dmanext(struct sbic_softc *dev)
{

	if (dev->sc_cur > dev->sc_last) {
		/*
		 * Shouldn't happen !!
		 */
		printf("wdsc_dmanext at end !!!\n");
		wdsc_dmastop(dev);
		return 0;
	}

	dev->sc_tcnt = dev->sc_cur->dc_count << 1;

	/* 
	 * Load the next DMA address
	 */
	pcc_reg_write(sys_pcc, PCCREG_DMA_CONTROL, 0);
	pcc_reg_write(sys_pcc, PCCREG_DMA_INTR_CTRL,
	    dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR);
	pcc_reg_write32(sys_pcc, PCCREG_DMA_DATA_ADDR,
	    (uint32_t) dev->sc_cur->dc_addr);
	pcc_reg_write32(sys_pcc, PCCREG_DMA_BYTE_COUNT,
	    (uint32_t) dev->sc_tcnt | (1 << 24));
	pcc_reg_write(sys_pcc, PCCREG_DMA_CONTROL, dev->sc_dmacmd);

	return dev->sc_tcnt;
}

/*
 * Stop DMA, and disable interrupts
 */
void
wdsc_dmastop(struct sbic_softc *dev)
{
	int s;

	s = splbio();

	pcc_reg_write(sys_pcc, PCCREG_DMA_CONTROL, 0);
	pcc_reg_write(sys_pcc, PCCREG_DMA_INTR_CTRL, dev->sc_ipl | PCC_ICLEAR);

	splx(s);
}

/*
 * Come here following a DMA interrupt
 */
int
wdsc_dmaintr(void *arg)
{
	struct sbic_softc *dev = arg;
	int found = 0;

	/*
	 * Really a DMA interrupt?
	 */
	if ((pcc_reg_read(sys_pcc, PCCREG_DMA_INTR_CTRL) & 0x80) == 0)
		return 0;

	/*
	 * Was it a completion interrupt?
	 * XXXSCW Note: Support for other DMA interrupts is required,
	 * eg. buserr
	 */
	if (pcc_reg_read(sys_pcc, PCCREG_DMA_CONTROL) & DMAC_CSR_DONE) {
		++found;

		pcc_reg_write(sys_pcc, PCCREG_DMA_INTR_CTRL,
		    dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR);
	}

	return found;
}

/*
 * Come here for SCSI interrupts
 */
int
wdsc_scsiintr(void *arg)
{
	struct sbic_softc *dev = arg;
	int found;

	/*
	 * Really a SCSI interrupt?
	 */
	if ((pcc_reg_read(sys_pcc, PCCREG_SCSI_INTR_CTRL) & 0x80) == 0)
		return 0;

	/*
	 * Go handle it
	 */
	found = sbicintr(dev);

	/*
	 * Acknowledge and clear the interrupt
	 */
	pcc_reg_write(sys_pcc, PCCREG_SCSI_INTR_CTRL,
	    dev->sc_ipl | PCC_IENABLE | PCC_ICLEAR);

	return found;
}
