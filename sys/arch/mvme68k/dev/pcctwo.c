/*	$NetBSD: pcctwo.c,v 1.16 2001/08/12 19:16:18 scw Exp $ */

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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
 * PCCchip2 and MCchip Driver
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/dev/pcctworeg.h>
#include <mvme68k/dev/pcctwovar.h>

/*
 * Autoconfiguration stuff.
 */
void pcctwoattach __P((struct device *, struct device *, void *));
int pcctwomatch __P((struct device *, struct cfdata *, void *));
int pcctwoprint __P((void *, const char *));

struct cfattach pcctwo_ca = {
	sizeof(struct pcctwo_softc), pcctwomatch, pcctwoattach
};

extern struct cfdriver pcctwo_cd;

/*
 * Global Pointer to the PCCChip2/MCchip soft state, and chip ID
 */
struct pcctwo_softc *sys_pcctwo;

/*
 * Structure used to describe a device for autoconfiguration purposes.
 */
struct pcctwo_device {
	char *pcc_name;		/* name of device (e.g. "clock") */
	bus_addr_t pcc_offset;	/* offset from PCC2 base */
};

/*
 * Macroes to make life easy when converting vector offset to interrupt
 * control register, and how to initialise the ICSR.
 */
#define VEC2ICSR(r,v)		((r) | (((v) | PCCTWO_ICR_IEN) << 8))
#define VEC2ICSR_REG(x)		((x) & 0xff)
#define VEC2ICSR_INIT(x)	((x) >> 8)

#if defined(MVME167) || defined(MVME177)
/*
 * Devices that live on the PCCchip2, attached in this order.
 */
static struct pcctwo_device pcctwo_devices[] = {
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
static struct pcctwo_device mcchip_devices[] = {
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
static	void pcctwosoftintrassert(void);
#endif

/* ARGSUSED */
int
pcctwomatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct mainbus_attach_args *ma;
	bus_space_handle_t bh;
	u_int8_t cid;

	ma = args;

	/* There can be only one. */
	if (sys_pcctwo || strcmp(ma->ma_name, pcctwo_cd.cd_name))
		return (0);

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
		return (1);
#endif
#if defined(MVME162) || defined(MVME172)
	if ((machineid == MVME_162 || machineid == MVME_172) &&
	    cid == PCCTWO_CHIP_ID_MCCHIP)
		return (1);
#endif

	return (0);
}

/* ARGSUSED */
void
pcctwoattach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct mainbus_attach_args *ma;
	struct pcctwo_softc *sc;
	struct pcctwo_attach_args npa;
	struct pcctwo_device *pd;
	u_int8_t cid;

	ma = args;
	sc = sys_pcctwo = (struct pcctwo_softc *) self;

	/* Get a handle to the PCCChip2's registers */
	sc->sc_bust = ma->ma_bust;
	bus_space_map(sc->sc_bust, PCCTWO_REG_OFF + ma->ma_offset,
	    PCC2REG_SIZE, 0, &sc->sc_bush);

	/*
	 * Fix up the vector base for PCCChip2 Interrupts
	 */
	pcc2_reg_write(sc, PCC2REG_VECTOR_BASE, PCCTWO_VECBASE);

	/*
	 * Enable PCCChip2 Interrupts
	 */
	pcc2_reg_write(sc, PCC2REG_GENERAL_CONTROL,
	    pcc2_reg_read(sc, PCC2REG_GENERAL_CONTROL) | PCCTWO_GEN_CTRL_MIEN);

	/* What are we? */
	cid = pcc2_reg_read(sc, PCC2REG_CHIP_ID);

	/*
	 * Announce ourselves to the world in general
	 */
#if defined(MVME167) || defined(MVME177)
	if (cid == PCCTWO_CHIP_ID_PCC2) {
		printf(": Peripheral Channel Controller (PCCchip2), Rev %d\n",
		    pcc2_reg_read(sc, PCC2REG_CHIP_REVISION));
		pd = pcctwo_devices;
		sc->sc_vec2icsr = pcctwo_vec2icsr_1x7;
	} else
#endif
#if defined(MVME162) || defined(MVME172)
	if (cid == PCCTWO_CHIP_ID_MCCHIP) {
		printf(": Memory Controller ASIC (MCchip), Rev %d\n",
		    pcc2_reg_read(sc, PCC2REG_CHIP_REVISION));
		pd = mcchip_devices;
		sc->sc_vec2icsr = pcctwo_vec2icsr_1x2;

		evcnt_attach_dynamic(&sc->sc_evcnt, EVCNT_TYPE_INTR,
		    isrlink_evcnt(7), "nmi", "abort sw");
		pcctwointr_establish(MCCHIPV_ABORT, pcctwoabortintr, 7, NULL,
		    &sc->sc_evcnt);
	} else
#endif
	{
		/* This is one of those "Can't Happen" things ... */
		panic("pcctwoattach: unsupported ASIC!");
	}

	/*
	 * Attach configured children.
	 */
	npa._pa_base = ma->ma_offset;
	while (pd->pcc_name != NULL) {
		/*
		 * Note that IPL is filled in by match function.
		 */
		npa.pa_name = pd->pcc_name;
		npa.pa_ipl = -1;
		npa.pa_dmat = ma->ma_dmat;
		npa.pa_bust = ma->ma_bust;
		npa.pa_offset = pd->pcc_offset + ma->ma_offset;
		pd++;

		/* Attach the device if configured. */
		(void) config_found(self, &npa, pcctwoprint);
	}
}

int
pcctwoprint(aux, cp)
	void *aux;
	const char *cp;
{
	struct pcctwo_attach_args *pa;

	pa = aux;

	if (cp)
		printf("%s at %s", pa->pa_name, cp);

	printf(" offset 0x%lx", pa->pa_offset - pa->_pa_base);
	if (pa->pa_ipl != -1)
		printf(" ipl %d", pa->pa_ipl);

	return (UNCONF);
}

/*
 * pcctwointr_establish: Establish PCCChip2 Interrupt
 */
void
pcctwointr_establish(vec, hand, lvl, arg, evcnt)
	int vec;
	int (*hand) __P((void *)), lvl;
	void *arg;
	struct evcnt *evcnt;
{
	int vec2icsr;

#ifdef DEBUG
	if (vec < 0 || vec >= PCCTWOV_MAX) {
		printf("pcctwo: illegal vector offset: 0x%x\n", vec);
		panic("pcctwointr_establish");
	}
	if (lvl < 1 || lvl > 7) {
		printf("pcctwo: illegal interrupt level: %d\n", lvl);
		panic("pcctwointr_establish");
	}
	if (sys_pcctwo->sc_vec2icsr[vec] == -1) {
		printf("pcctwo: unsupported vector: %d\n", vec);
		panic("pcctwointr_establish");
	}
#endif

	vec2icsr = sys_pcctwo->sc_vec2icsr[vec];
	pcc2_reg_write(sys_pcctwo, VEC2ICSR_REG(vec2icsr), 0);

	/* Hook the interrupt */
	isrlink_vectored(hand, arg, lvl, vec + PCCTWO_VECBASE, evcnt);

	/* Enable it in hardware */
	pcc2_reg_write(sys_pcctwo, VEC2ICSR_REG(vec2icsr),
	    VEC2ICSR_INIT(vec2icsr) | lvl);
}

void
pcctwointr_disestablish(vec)
	int vec;
{

#ifdef DEBUG
	if (vec < 0 || vec >= PCCTWOV_MAX) {
		printf("pcctwo: illegal vector offset: 0x%x\n", vec);
		panic("pcctwointr_disestablish");
	}
	if (sys_pcctwo->sc_vec2icsr[vec] == -1) {
		printf("pcctwo: unsupported vector: %d\n", vec);
		panic("pcctwointr_establish");
	}
#endif

	/* Disable it in hardware */
	pcc2_reg_write(sys_pcctwo, sys_pcctwo->sc_vec2icsr[vec], 0);

	isrunlink_vectored(vec + PCCTWO_VECBASE);
}

#if defined(MVME162) || defined(MVME172)
static int
pcctwoabortintr(void *frame)
{

	pcc2_reg_write(sys_pcctwo, MCCHIPREG_ABORT_ICSR, PCCTWO_ICR_ICLR |
	    pcc2_reg_read(sys_pcctwo, MCCHIPREG_ABORT_ICSR));

	return (nmihand(frame));
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
	_softintr_chipset_assert = pcctwosoftintrassert;
}

static int
pcctwosoftintr(void *arg)
{
	struct pcctwo_softc *sc = arg;

	pcc2_reg_write32(sc, MCCHIPREG_TIMER4_CNTR, 0);
	pcc2_reg_write(sc, MCCHIPREG_TIMER4_CTRL, 0);
	pcc2_reg_write(sc, MCCHIPREG_TIMER4_ICSR,
	    PCCTWO_ICR_ICLR | PCCTWO_ICR_IEN | 1);

	softintr_dispatch();

	return (1);
}

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
