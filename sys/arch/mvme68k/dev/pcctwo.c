/*	$NetBSD: pcctwo.c,v 1.3 2000/03/18 22:33:03 scw Exp $ */

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
 * PCCchip2 Driver
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>

#include <mvme68k/mvme68k/isr.h>

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
 * Global Pointer to the PCCChip2's soft state
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
 * Devices that live on the PCCchip2, attached in this order.
 */
struct pcctwo_device pcctwo_devices[] = {
	{"clock", PCCTWO_RTC_OFF},
	{"clmpcc", PCCTWO_SCC_OFF},
	{"ie", PCCTWO_IE_OFF},
	{"ncrsc", PCCTWO_NCRSC_OFF},
	{"lpt", PCCTWO_LPT_OFF},
	{"nvram", PCCTWO_NVRAM_OFF},
	{NULL, 0}
};

/* ARGSUSED */
int
pcctwomatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct mainbus_attach_args *ma;

	ma = args;

	/*
	 * Note: We don't need to check we're running on a 'machineid'
	 * which contains a PCCChip2, since "mainbus_attach" already
	 * deals with it.
	 */

	/* Only attach one PCCchip2. */
	if (sys_pcctwo)
		return (0);

	return (strcmp(ma->ma_name, pcctwo_cd.cd_name) == 0);
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
	int i;

	ma = args;
	sc = sys_pcctwo = (struct pcctwo_softc *) self;

	/* Get a handle to the PCCChip2's registers */
	sc->sc_bust = ma->ma_bust;
	bus_space_map(sc->sc_bust, PCCTWO_REG_OFF + ma->ma_offset,
	    PCC2REG_SIZE, 0, &sc->sc_bush);

	/*
	 * Announce ourselves to the world in general
	 */
	printf(": Peripheral Channel Controller (PCCchip2), Rev %d\n",
	    pcc2_reg_read(sc, PCC2REG_CHIP_REVISION));

	/*
	 * Fix up the vector base for PCCChip2 Interrupts
	 */
	pcc2_reg_write(sc, PCC2REG_VECTOR_BASE, PCCTWO_VECBASE);

	/*
	 * Enable PCCChip2 Interrupts
	 */
	pcc2_reg_write(sc, PCC2REG_GENERAL_CONTROL,
	    pcc2_reg_read(sc, PCC2REG_GENERAL_CONTROL) | PCCTWO_GEN_CTRL_MIEN);

	/*
	 * Attach configured children.
	 */
	for (i = 0; pcctwo_devices[i].pcc_name != NULL; ++i) {
		/*
		 * Note that IPL is filled in by match function.
		 */
		npa.pa_name = pcctwo_devices[i].pcc_name;
		npa.pa_ipl = -1;
		npa.pa_dmat = ma->ma_dmat;
		npa.pa_bust = ma->ma_bust;
		npa.pa_offset = pcctwo_devices[i].pcc_offset + ma->ma_offset;

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

	printf(" offset 0x%lx", pa->pa_offset);
	if (pa->pa_ipl != -1)
		printf(" ipl %d", pa->pa_ipl);

	return (UNCONF);
}

/*
 * pcctwointr_establish: Establish PCCChip2 Interrupt
 */
void
pcctwointr_establish(vec, hand, lvl, arg)
	int vec;
	int (*hand) __P((void *)), lvl;
	void *arg;
{

#ifdef DEBUG
	if (vec < 0 || vec >= PCCTWOV_MAX) {
		printf("pcctwo: illegal vector offset: 0x%x\n", vec);
		panic("pcctwointr_establish");
	}
	if (lvl < 1 || lvl > 7) {
		printf("pcctwo: illegal interrupt level: %d\n", lvl);
		panic("pcctwointr_establish");
	}
#endif

	isrlink_vectored(hand, arg, lvl, vec + PCCTWO_VECBASE);
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
#endif

	isrunlink_vectored(vec + PCCTWO_VECBASE);
}
