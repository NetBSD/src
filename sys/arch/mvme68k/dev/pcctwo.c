/*	$NetBSD: pcctwo.c,v 1.2 1999/02/14 17:54:28 scw Exp $ */

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

#include "pcctwo.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/callout.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/fcntl.h>
#include <sys/device.h>
#include <machine/cpu.h>
#include <dev/cons.h>

#include <mvme68k/mvme68k/isr.h>

#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/pcctworeg.h>

/*
 * Autoconfiguration stuff.
 */

struct pcctwosoftc {
	struct device	sc_dev;
	struct pcctwo	*sc_pcc;
};

void	pcctwoattach __P((struct device *, struct device *, void *));
int 	pcctwomatch __P((struct device *, struct cfdata *, void *));
int 	pcctwoprint __P((void *, const char *));

struct cfattach pcctwo_ca = {
	sizeof(struct pcctwosoftc), pcctwomatch, pcctwoattach
};

extern struct cfdriver pcctwo_cd;

int	pcctwointr __P((void *));

/*
 * Global Pointer to the PCCChip2's Registers
 */
struct pcctwo *sys_pcctwo = NULL;

/*
 * Devices that live on the PCCchip2, attached in this order.
 */
struct pcc_device pcctwo_devices[] = {
	{ "clock",	PCCTWO_CLOCK_OFF,	1 },
	{ "clmpcc",	PCCTWO_SCC_OFF,		1 },
	{ "ie",		PCCTWO_IE_OFF,		1 },
	{ "ncrsc",	PCCTWO_NCRSC_OFF,	1 },
	{ "lpt",	PCCTWO_LPT_OFF,		1 },
	{ NULL,		0,			0 },
};

int
pcctwomatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	char *ma_name = (char *)args;

	/*
	 * Note: We don't need to check we're running on a 'machineid'
	 * which contains a PCCChip2, since "mainbus_attach" already
	 * deals with it.
	 */

	/* Only attach one PCCchip2. */
	if (sys_pcctwo)
		return (0);

	return (strcmp(ma_name, pcctwo_cd.cd_name) == 0);
}

void
pcctwoattach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct pcctwosoftc *pccsc;
	struct pcc_attach_args npa;
	caddr_t kva;
	int i;

	if (sys_pcctwo)
		panic("pcctwoattach: PCCchip2 already attached!");

	sys_pcctwo = (struct pcctwo *)PCCTWO_VADDR(PCCTWO_REG_OFF);

	/*
	 * Get Soft State Structure and record register base
	 */
	pccsc = (struct pcctwosoftc *) self;
	pccsc->sc_pcc = sys_pcctwo;

	/*
	 * Announce ourselves to the world in general
	 */
	printf(": Peripheral Channel Controller (PCCchip2), Rev %d\n",
		pccsc->sc_pcc->chip_rev);

	/*
	 * Fix up the vector base for PCCChip2 Interrupts
	 */
	pccsc->sc_pcc->vector_base = PCCTWO_VECBASE;

	/*
	 * Enable PCCChip2 Interrupts
	 */
	pccsc->sc_pcc->gen_ctrl |= PCCTWO_GEN_CTRL_MIEN;

	/*
	 * Attach configured children.
	 */
	for (i = 0; pcctwo_devices[i].pcc_name != NULL; ++i) {
		/*
		 * Note that IPL is filled in by match function.
		 */
		npa.pa_name = pcctwo_devices[i].pcc_name;
		npa.pa_offset = pcctwo_devices[i].pcc_offset;
		npa.pa_ipl = -1;

		/* Check for hardware. (XXX is this really necessary?) */
		kva = PCCTWO_VADDR(npa.pa_offset);
		if (badaddr(kva, pcctwo_devices[i].pcc_bytes)) {
			/*
			 * Hardware not present.
			 */
			continue;
		}

		/* Attach the device if configured. */
		(void)config_found(self, &npa, pcctwoprint);
	}
}

int
pcctwoprint(aux, cp)
	void *aux;
	const char *cp;
{
	struct pcc_attach_args *pa = aux;

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
	if ((vec < 0) || (vec >= PCCTWOV_MAX)) {
		printf("pcctwo: illegal vector offset: 0x%x\n", vec);
		panic("pcctwointr_establish");
	}

	if ((lvl < 1) || (lvl > 7)) {
		printf("pcctwo: illegal interrupt level: %d\n", lvl);
		panic("pcctwointr_establish");
	}

	isrlink_vectored(hand, arg, lvl, vec + PCCTWO_VECBASE);
}

void
pcctwointr_disestablish(vec)
	int vec;
{
	if ((vec < 0) || (vec >= PCCTWOV_MAX)) {
		printf("pcctwo: illegal vector offset: 0x%x\n", vec);
		panic("pcctwointr_establish");
	}

	isrunlink_vectored(vec + PCCTWO_VECBASE);
}
