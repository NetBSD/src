/*	$NetBSD: pcc.c,v 1.11.8.1 1999/01/30 21:58:42 scw Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Copyright (c) 1995 Charles D. Cranor
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
 *      This product includes software developed by Charles D. Cranor.
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

/*
 * peripheral channel controller
 */

#include "pcc.h"

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

#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>

/*
 * Autoconfiguration stuff.
 */

struct pccsoftc {
	struct device	sc_dev;
	struct pcc	*sc_pcc;
};

void	pccattach __P((struct device *, struct device *, void *));
int 	pccmatch __P((struct device *, struct cfdata *, void *));
int 	pccprint __P((void *, const char *));

struct cfattach pcc_ca = {
	sizeof(struct pccsoftc), pccmatch, pccattach
};

extern struct cfdriver pcc_cd;

int	pccintr __P((void *));

/*
 * globals
 */

struct pcc *sys_pcc = NULL;

/*
 * Devices that live on the PCC, attached in this order.
 */
struct pcc_device pcc_devices[] = {
	{ "clock",	PCC_CLOCK_OFF,	1 },
	{ "zsc",	PCC_ZS0_OFF,	1 },
	{ "zsc",	PCC_ZS1_OFF,	1 },
	{ "le",		PCC_LE_OFF,	2 },
	{ "wdsc",	PCC_WDSC_OFF,	1 },
	{ "lpt",	PCC_LPT_OFF,	1 },
	{ "vmechip",	PCC_VME_OFF,	2 },
	{ NULL,		0,		0 },
};

int
pccmatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	char *ma_name = args;

	/* Only attach one PCC. */
	if (sys_pcc)
		return (0);

	return (strcmp(ma_name, pcc_cd.cd_name) == 0);
}

void
pccattach(parent, self, args)
	struct device *parent, *self;
	void *args;
{
	struct pccsoftc *pccsc;
	struct pcc_attach_args npa;
	caddr_t kva;
	int i;

	if (sys_pcc)
		panic("pccattach: pcc already attached!");

	sys_pcc = (struct pcc *)PCC_VADDR(PCC_REG_OFF);

	/*
	 * link into softc and set up interrupt vector base,
	 * and initialize the chip.
	 */
	pccsc = (struct pccsoftc *) self;
	pccsc->sc_pcc = sys_pcc;
	pccsc->sc_pcc->int_vectr = PCC_VECBASE;

	printf(": Peripheral Channel Controller, "
	    "rev %d, vecbase 0x%x\n", pccsc->sc_pcc->pcc_rev,
	    pccsc->sc_pcc->int_vectr);

	/* Hook up interrupt handler for abort button. */
	pccintr_establish(PCCV_ABORT, pccintr, 7, NULL);

	/*
	 * Now that the interrupt handler has been established,
	 * enable the ABORT switch interrupt.
	 */
	pccsc->sc_pcc->abrt_int = PCC_ABORT_IEN | PCC_ABORT_ACK;

	/* Make sure the global interrupt line is hot. */
	pccsc->sc_pcc->gen_cr |= PCC_GENCR_IEN;

	/*
	 * Attach configured children.
	 */
	for (i = 0; pcc_devices[i].pcc_name != NULL; ++i) {
		/*
		 * Note that IPL is filled in by match function.
		 */
		npa.pa_name = pcc_devices[i].pcc_name;
		npa.pa_offset = pcc_devices[i].pcc_offset;
		npa.pa_ipl = -1;

		/* Check for hardware. (XXX is this really necessary?) */
		kva = PCC_VADDR(npa.pa_offset);
		if (badaddr(kva, pcc_devices[i].pcc_bytes)) {
			/*
			 * Hardware not present.
			 */
			continue;
		}

		/* Attach the device if configured. */
		(void)config_found(self, &npa, pccprint);
	}
}

int
pccprint(aux, cp)
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
 * pccintr_establish: establish pcc interrupt
 */
void
pccintr_establish(pccvec, hand, lvl, arg)
	int pccvec;
	int (*hand) __P((void *)), lvl;
	void *arg;
{

	if ((pccvec < 0) || (pccvec >= PCC_NVEC)) {
		printf("pcc: illegal vector offset: 0x%x\n", pccvec);
		panic("pccintr_establish");
	}

	if ((lvl < 1) || (lvl > 7)) {
		printf("pcc: illegal interrupt level: %d\n", lvl);
		panic("pccintr_establish");
	}

	isrlink_vectored(hand, arg, lvl, pccvec + PCC_VECBASE);
}

void
pccintr_disestablish(pccvec)
	int pccvec;
{

	if ((pccvec < 0) || (pccvec >= PCC_NVEC)) {
		printf("pcc: illegal vector offset: 0x%x\n", pccvec);
		panic("pccintr_establish");
	}

	isrunlink_vectored(pccvec + PCC_VECBASE);
}

/*
 * Handle NMI from abort switch.
 */
int
pccintr(frame)
	void *frame;
{

	/* XXX wait until button pops out */
	sys_pcc->abrt_int = PCC_ABORT_IEN | PCC_ABORT_ACK;
	nmihand((struct frame *)frame);
	return (1);
}
