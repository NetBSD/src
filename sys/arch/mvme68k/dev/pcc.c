/*	$NetBSD: pcc.c,v 1.12.8.2 2000/12/08 09:28:31 bouyer Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe and Steve C. Woodford.
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kcore.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <mvme68k/mvme68k/isr.h>

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>

/*
 * Autoconfiguration stuff for the PCC chip on mvme147
 */

void pccattach __P((struct device *, struct device *, void *));
int pccmatch __P((struct device *, struct cfdata *, void *));
int pccprint __P((void *, const char *));

struct cfattach pcc_ca = {
	sizeof(struct pcc_softc), pccmatch, pccattach
};

extern struct cfdriver pcc_cd;
static int pccintr __P((void *));

/*
 * Structure used to describe a device for autoconfiguration purposes.
 */
struct pcc_device {
	char *pcc_name;		/* name of device (e.g. "clock") */
	bus_addr_t pcc_offset;	/* offset from PCC base */
};

/*
 * Devices that live on the PCC, attached in this order.
 */
static struct pcc_device pcc_devices[] = {
	{"clock", PCC_RTC_OFF},
	{"nvram", PCC_NVRAM_OFF},
	{"zsc", PCC_ZS0_OFF},
	{"zsc", PCC_ZS1_OFF},
	{"le", PCC_LE_OFF},
	{"wdsc", PCC_WDSC_OFF},
	{"lpt", PCC_LPT_OFF},
	{"vmepcc", PCC_VME_OFF},
	{NULL, 0},
};

static int pcc_vec2intctrl[] = {
	PCCREG_ACFAIL_INTR_CTRL,/* PCCV_ACFAIL */
	PCCREG_BUSERR_INTR_CTRL,/* PCCV_BERR */
	PCCREG_ABORT_INTR_CTRL,	/* PCCV_ABORT */
	PCCREG_SERIAL_INTR_CTRL,/* PCCV_ZS */
	PCCREG_LANCE_INTR_CTRL,	/* PCCV_LE */
	PCCREG_SCSI_INTR_CTRL,	/* PCCV_SCSI */
	PCCREG_DMA_INTR_CTRL,	/* PCCV_DMA */
	PCCREG_PRNT_INTR_CTRL,	/* PCCV_PRINTER */
	PCCREG_TMR1_INTR_CTRL,	/* PCCV_TIMER1 */
	PCCREG_TMR2_INTR_CTRL,	/* PCCV_TIMER2 */
	PCCREG_SOFT1_INTR_CTRL,	/* PCCV_SOFT1 */
	PCCREG_SOFT2_INTR_CTRL	/* PCCV_SOFT2 */
};

extern phys_ram_seg_t mem_clusters[];
struct pcc_softc *sys_pcc;

/* The base address of the MVME147 from the VMEbus */
bus_addr_t pcc_slave_base_addr;


/* ARGSUSED */
int
pccmatch(parent, cf, args)
	struct device *parent;
	struct cfdata *cf;
	void *args;
{
	struct mainbus_attach_args *ma;

	ma = args;

	/* Only attach one PCC. */
	if (sys_pcc)
		return (0);

	return (strcmp(ma->ma_name, pcc_cd.cd_name) == 0);
}

/* ARGSUSED */
void
pccattach(parent, self, args)
	struct device *parent;
	struct device *self;
	void *args;
{
	struct mainbus_attach_args *ma;
	struct pcc_attach_args npa;
	struct pcc_softc *sc;
	u_int8_t reg;
	int i;

	ma = args;
	sc = sys_pcc = (struct pcc_softc *) self;

	/* Get a handle to the PCC's registers. */
	sc->sc_bust = ma->ma_bust;
	bus_space_map(sc->sc_bust, PCC_REG_OFF + ma->ma_offset,
	    PCCREG_SIZE, 0, &sc->sc_bush);

	/* Tell the chip the base interrupt vector */
	pcc_reg_write(sc, PCCREG_VECTOR_BASE, PCC_VECBASE);

	printf(": Peripheral Channel Controller, "
	    "rev %d, vecbase 0x%x\n", pcc_reg_read(sc, PCCREG_REVISION),
	    pcc_reg_read(sc, PCCREG_VECTOR_BASE));

	/* Hook up interrupt handler for abort button, and enable it */
	pccintr_establish(PCCV_ABORT, pccintr, 7, NULL);
	pcc_reg_write(sc, PCCREG_ABORT_INTR_CTRL,
	    PCC_ABORT_IEN | PCC_ABORT_ACK);

	/* Make sure the global interrupt line is hot. */
	reg = pcc_reg_read(sc, PCCREG_GENERAL_CONTROL) | PCC_GENCR_IEN;
	pcc_reg_write(sc, PCCREG_GENERAL_CONTROL, reg);

	/*
	 * Calculate the board's VMEbus slave base address, for the
	 * benefit of the VMEchip driver.
	 * (Weird that this register is in the PCC ...)
	 */
	reg = pcc_reg_read(sc, PCCREG_SLAVE_BASE_ADDR) & PCC_SLAVE_BASE_MASK;
	pcc_slave_base_addr = (bus_addr_t)reg * mem_clusters[0].size;

	/*
	 * Attach configured children.
	 */
	npa._pa_base = ma->ma_offset;
	for (i = 0; pcc_devices[i].pcc_name != NULL; ++i) {
		/*
		 * Note that IPL is filled in by match function.
		 */
		npa.pa_name = pcc_devices[i].pcc_name;
		npa.pa_ipl = -1;
		npa.pa_dmat = ma->ma_dmat;
		npa.pa_bust = ma->ma_bust;
		npa.pa_offset = pcc_devices[i].pcc_offset + ma->ma_offset;

		/* Attach the device if configured. */
		(void) config_found(self, &npa, pccprint);
	}
}

int
pccprint(aux, cp)
	void *aux;
	const char *cp;
{
	struct pcc_attach_args *pa;

	pa = aux;

	if (cp)
		printf("%s at %s", pa->pa_name, cp);

	printf(" offset 0x%lx", pa->pa_offset - pa->_pa_base);
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

#ifdef DEBUG
	if (pccvec < 0 || pccvec >= PCC_NVEC) {
		printf("pcc: illegal vector offset: 0x%x\n", pccvec);
		panic("pccintr_establish");
	}
	if (lvl < 1 || lvl > 7) {
		printf("pcc: illegal interrupt level: %d\n", lvl);
		panic("pccintr_establish");
	}
#endif

	isrlink_vectored(hand, arg, lvl, pccvec + PCC_VECBASE);
}

void
pccintr_disestablish(pccvec)
	int pccvec;
{

#ifdef DEBUG
	if (pccvec < 0 || pccvec >= PCC_NVEC) {
		printf("pcc: illegal vector offset: 0x%x\n", pccvec);
		panic("pccintr_disestablish");
	}
#endif

	/* Disable the interrupt */
	pcc_reg_write(sys_pcc, pcc_vec2intctrl[pccvec], PCC_ICLEAR);
	isrunlink_vectored(pccvec + PCC_VECBASE);
}

/*
 * Handle NMI from abort switch.
 */
static int
pccintr(frame)
	void *frame;
{

	/* XXX wait until button pops out */
	pcc_reg_write(sys_pcc, PCCREG_ABORT_INTR_CTRL,
	    PCC_ABORT_IEN | PCC_ABORT_ACK);

	return (nmihand(frame));
}
