/*	$NetBSD: pcctwo_68k.c,v 1.8 2008/01/12 09:54:24 tsutsui Exp $	*/

/*-
 * Copyright (c) 1999, 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	      This product includes software developed by the NetBSD
 *	      Foundation, Inc. and its contributors.
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
 * PCCchip2 and MCchip Mvme68k Front End Driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcctwo_68k.c,v 1.8 2008/01/12 09:54:24 tsutsui Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/mvme68k/isr.h>

#include <dev/mvme/pcctworeg.h>
#include <dev/mvme/pcctwovar.h>

#include "ioconf.h"

/*
 * Autoconfiguration stuff.
 */
void pcctwoattach(struct device *, struct device *, void *);
int pcctwomatch(struct device *, struct cfdata *, void *);

CFATTACH_DECL(pcctwo, sizeof(struct pcctwo_softc),
    pcctwomatch, pcctwoattach, NULL, NULL);


#if defined(MVME167) || defined(MVME177)
/*
 * Devices that live on the PCCchip2, attached in this order.
 */
static const struct pcctwo_device pcctwo_devices[] = {
	{"clock", 0},
	{"clmpcc", PCCTWO_SCC_OFF},
	{"ie", PCCTWO_IE_OFF},
	{"osiop", PCCTWO_NCRSC_OFF},
	{"lpt", PCCTWO_LPT_OFF},
	{NULL, 0}
};

static int pcctwo_vec2icsr_1x7[] = {
	VEC2ICSR(PCC2REG_PRT_BUSY_ICSR,  PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_PRT_PE_ICSR,    PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_PRT_SEL_ICSR,   PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_PRT_FAULT_ICSR, PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_PRT_ACK_ICSR,   PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_SCSI_ICSR,      0),
	VEC2ICSR(PCC2REG_ETH_ICSR,       PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_ETH_ICSR,       PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_TIMER2_ICSR,    PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_TIMER1_ICSR,    PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_GPIO_ICSR,      PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	-1,
	VEC2ICSR(PCC2REG_SCC_RX_ICSR,    0),
	VEC2ICSR(PCC2REG_SCC_MODEM_ICSR, 0),
	VEC2ICSR(PCC2REG_SCC_TX_ICSR,    0),
	VEC2ICSR(PCC2REG_SCC_RX_ICSR,    0)
};
#endif

#if defined(MVME162) || defined(MVME172)
/*
 * Devices that live on the MCchip, attached in this order.
 */
static const struct pcctwo_device mcchip_devices[] = {
	{"clock", 0},
	{"zsc", MCCHIP_ZS0_OFF},
	{"zsc", MCCHIP_ZS1_OFF},
	{"ie", PCCTWO_IE_OFF},
	{"osiop", PCCTWO_NCRSC_OFF},
	{NULL, 0}
};

static int pcctwo_vec2icsr_1x2[] = {
	-1,
	-1,
	-1,
	VEC2ICSR(MCCHIPREG_TIMER4_ICSR, PCCTWO_ICR_ICLR),
	VEC2ICSR(MCCHIPREG_TIMER3_ICSR, PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_SCSI_ICSR,     0),
	VEC2ICSR(PCC2REG_ETH_ICSR,      PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_ETH_ICSR,      PCCTWO_ICR_EDGE | PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_TIMER2_ICSR,   PCCTWO_ICR_ICLR),
	VEC2ICSR(PCC2REG_TIMER1_ICSR,   PCCTWO_ICR_ICLR),
	-1,
	VEC2ICSR(MCCHIPREG_PARERR_ICSR, PCCTWO_ICR_ICLR),
	VEC2ICSR(MCCHIPREG_SCC_ICSR,    0),
	VEC2ICSR(MCCHIPREG_SCC_ICSR,    0),
	VEC2ICSR(MCCHIPREG_ABORT_ICSR,  PCCTWO_ICR_ICLR),
	-1
};

static	int pcctwoabortintr(void *);
void	pcctwosoftintrinit(void);
static	int pcctwosoftintr(void *);
#ifdef notyet
static	void pcctwosoftintrassert(void);
#endif
#endif

static void pcctwoisrlink(void *, int (*)(void *), void *,
		int, int, struct evcnt *);
static void pcctwoisrunlink(void *, int);
static struct evcnt *pcctwoisrevcnt(void *, int);


/* ARGSUSED */
int
pcctwomatch(struct device *parent, struct cfdata *cf, void *args)
{
	struct mainbus_attach_args *ma;
	bus_space_handle_t bh;
	uint8_t cid;

	ma = args;

	/* There can be only one. */
	if (sys_pcctwo || strcmp(ma->ma_name, pcctwo_cd.cd_name))
		return 0;

	/*
	 * Grab the Chip's ID
	 */
	bus_space_map(ma->ma_bust, PCCTWO_REG_OFF + ma->ma_offset,
	    PCC2REG_SIZE, 0, &bh);
	cid = bus_space_read_1(ma->ma_bust, bh, PCC2REG_CHIP_ID);
	bus_space_unmap(ma->ma_bust, bh, PCC2REG_SIZE);

#if defined(MVME167) || defined(MVME177)
	if ((machineid == MVME_167 || machineid == MVME_177) &&
	    cid == PCCTWO_CHIP_ID_PCC2)
		return 1;
#endif
#if defined(MVME162) || defined(MVME172)
	if ((machineid == MVME_162 || machineid == MVME_172) &&
	    cid == PCCTWO_CHIP_ID_MCCHIP)
		return 1;
#endif

	return 0;
}

/* ARGSUSED */
void
pcctwoattach(struct device *parent, struct device *self, void *args)
{
	struct mainbus_attach_args *ma;
	struct pcctwo_softc *sc;
	const struct pcctwo_device *pd = NULL;
	uint8_t cid;

	ma = args;
	sc = sys_pcctwo = (struct pcctwo_softc *)self;

	/* Get a handle to the PCCChip2's registers */
	sc->sc_bust = ma->ma_bust;
	sc->sc_dmat = ma->ma_dmat;
	bus_space_map(sc->sc_bust, PCCTWO_REG_OFF + ma->ma_offset,
	    PCC2REG_SIZE, 0, &sc->sc_bush);

	sc->sc_vecbase = PCCTWO_VECBASE;
	sc->sc_isrlink = pcctwoisrlink;
	sc->sc_isrevcnt = pcctwoisrevcnt;
	sc->sc_isrunlink = pcctwoisrunlink;

	cid = pcc2_reg_read(sc, PCC2REG_CHIP_ID);

#if defined(MVME167) || defined(MVME177)
	if (cid == PCCTWO_CHIP_ID_PCC2) {
		pd = pcctwo_devices;
		sc->sc_vec2icsr = pcctwo_vec2icsr_1x7;
	}
#endif
#if defined(MVME162) || defined(MVME172)
	if (cid == PCCTWO_CHIP_ID_MCCHIP) {
		pd = mcchip_devices;
		sc->sc_vec2icsr = pcctwo_vec2icsr_1x2;
	}
#endif

	/* Finish initialisation in common code */
	pcctwo_init(sc, pd, ma->ma_offset);

#if defined(MVME162) || defined(MVME172)
	if (cid == PCCTWO_CHIP_ID_MCCHIP) {
		evcnt_attach_dynamic(&sc->sc_evcnt, EVCNT_TYPE_INTR,
		    isrlink_evcnt(7), "nmi", "abort sw");
		pcctwointr_establish(MCCHIPV_ABORT, pcctwoabortintr, 7, NULL,
		    &sc->sc_evcnt);
	}
#endif
}

/* ARGSUSED */
static void
pcctwoisrlink(void *cookie, int (*fn)(void *), void *arg, int ipl, int vec,
    struct evcnt *evcnt)
{

	isrlink_vectored(fn, arg, ipl, vec, evcnt);
}

/* ARGSUSED */
static void
pcctwoisrunlink(void *cookie, int vec)
{

	isrunlink_vectored(vec);
}

/* ARGSUSED */
static struct evcnt *
pcctwoisrevcnt(void *cookie, int ipl)
{

	return isrlink_evcnt(ipl);
}

#if defined(MVME162) || defined(MVME172)
static int
pcctwoabortintr(void *frame)
{

	pcc2_reg_write(sys_pcctwo, MCCHIPREG_ABORT_ICSR, PCCTWO_ICR_ICLR |
	    pcc2_reg_read(sys_pcctwo, MCCHIPREG_ABORT_ICSR));

	return nmihand(frame);
}

void
pcctwosoftintrinit(void)
{

	/*
	 * Since the VMEChip2 is normally used to generate
	 * software interrupts to the CPU, we have to deal
	 * with 162/172 boards which have the "No VMEChip2"
	 * build option.
	 *
	 * When such a board is found, the VMEChip2 probe code
	 * calls this function to implement software interrupts
	 * the hard way; using tick timer 4 ...
	 */
	pcctwointr_establish(MCCHIPV_TIMER4, pcctwosoftintr,
	    1, sys_pcctwo, &sys_pcctwo->sc_evcnt);
	pcc2_reg_write(sys_pcctwo, MCCHIPREG_TIMER4_CTRL, 0);
	pcc2_reg_write32(sys_pcctwo, MCCHIPREG_TIMER4_COMP, 1);
	pcc2_reg_write32(sys_pcctwo, MCCHIPREG_TIMER4_CNTR, 0);
#ifdef notyet
	_softintr_chipset_assert = pcctwosoftintrassert;
#endif
}

static int
pcctwosoftintr(void *arg)
{
	struct pcctwo_softc *sc = arg;

	pcc2_reg_write32(sc, MCCHIPREG_TIMER4_CNTR, 0);
	pcc2_reg_write(sc, MCCHIPREG_TIMER4_CTRL, 0);
	pcc2_reg_write(sc, MCCHIPREG_TIMER4_ICSR,
	    PCCTWO_ICR_ICLR | PCCTWO_ICR_IEN | 1);

#ifdef notyet
	softintr_dispatch();
#endif

	return 1;
}

#ifdef notyet
static void
pcctwosoftintrassert(void)
{

	/*
	 * Schedule a timer interrupt to happen in ~1uS.
	 * This is more than adequate on any available m68k platform
	 * for simulating software interrupts.
	 */
	pcc2_reg_write(sys_pcctwo, MCCHIPREG_TIMER4_CTRL, PCCTWO_TT_CTRL_CEN);
}
#endif
#endif
