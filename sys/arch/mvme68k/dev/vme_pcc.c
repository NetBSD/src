/*	$NetBSD: vme_pcc.c,v 1.6.16.2 2000/12/08 09:28:32 bouyer Exp $	*/

/*-
 * Copyright (c) 1996-2000 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
 * VME support specific to the Type 1 VMEchip found on the
 * MVME-147.
 *
 * For a manual on the MVME-147, call: 408.991.8634.  (Yes, this
 * is the Sunnyvale sales office.)
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/kcore.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <mvme68k/mvme68k/isr.h>

#include <mvme68k/dev/pccreg.h>
#include <mvme68k/dev/pccvar.h>
#include <mvme68k/dev/mvmebus.h>
#include <mvme68k/dev/vme_pccreg.h>
#include <mvme68k/dev/vme_pccvar.h>


int vme_pcc_match(struct device *, struct cfdata *, void *);
void vme_pcc_attach(struct device *, struct device *, void *);

struct cfattach vmepcc_ca = {
	sizeof(struct vme_pcc_softc), vme_pcc_match, vme_pcc_attach
};

extern struct cfdriver vmepcc_cd;

extern phys_ram_seg_t mem_clusters[];
static int vme_pcc_attached;

void vme_pcc_intr_establish(void *, int, int, int, int,
    int (*)(void *), void *);
void vme_pcc_intr_disestablish(void *, int, int, int);


static struct mvmebus_range vme_pcc_masters[] = {
	{VME_AM_A24 |
	    MVMEBUS_AM_CAP_DATA  | MVMEBUS_AM_CAP_PROG |
	    MVMEBUS_AM_CAP_SUPER | MVMEBUS_AM_CAP_USER,
		VME_D32 | VME_D16 | VME_D8,
		VME1_A24D32_LOC_START,
		VME1_A24_MASK,
		VME1_A24D32_START,
		VME1_A24D32_END},

	{VME_AM_A32 |
	    MVMEBUS_AM_CAP_DATA  | MVMEBUS_AM_CAP_PROG |
	    MVMEBUS_AM_CAP_SUPER | MVMEBUS_AM_CAP_USER,
		VME_D32 | VME_D16 | VME_D8,
		VME1_A32D32_LOC_START,
		VME1_A32_MASK,
		VME1_A32D32_START,
		VME1_A32D32_END},

	{VME_AM_A24 |
	    MVMEBUS_AM_CAP_DATA  | MVMEBUS_AM_CAP_PROG |
	    MVMEBUS_AM_CAP_SUPER | MVMEBUS_AM_CAP_USER,
		VME_D16 | VME_D8,
		VME1_A24D16_LOC_START,
		VME1_A24_MASK,
		VME1_A24D16_START,
		VME1_A24D16_END},

	{VME_AM_A32 |
	    MVMEBUS_AM_CAP_DATA  | MVMEBUS_AM_CAP_PROG |
	    MVMEBUS_AM_CAP_SUPER | MVMEBUS_AM_CAP_USER,
		VME_D16 | VME_D8,
		VME1_A32D16_LOC_START,
		VME1_A32_MASK,
		VME1_A32D16_START,
		VME1_A32D16_END},

	{VME_AM_A16 |
	    MVMEBUS_AM_CAP_DATA  |
	    MVMEBUS_AM_CAP_SUPER | MVMEBUS_AM_CAP_USER,
		VME_D16 | VME_D8,
		VME1_A16D16_LOC_START,
		VME1_A16_MASK,
		VME1_A16D16_START,
		VME1_A16D16_END}
};
#define VME1_NMASTERS	(sizeof(vme_pcc_masters)/sizeof(struct mvmebus_range))


/* ARGSUSED */
int
vme_pcc_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pcc_attach_args *pa;

	pa = aux;

	/* Only one VME chip, please. */
	if (vme_pcc_attached)
		return (0);

	if (strcmp(pa->pa_name, vmepcc_cd.cd_name))
		return (0);

	return (1);
}

void
vme_pcc_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct pcc_attach_args *pa;
	struct vme_pcc_softc *sc;
	vme_am_t am;
	u_int8_t reg;

	sc = (struct vme_pcc_softc *) self;
	pa = aux;

	/* Map the VMEchip's registers */
	bus_space_map(pa->pa_bust, pa->pa_offset, VME1REG_SIZE, 0,
	    &sc->sc_bush);

	/* Initialise stuff used by the mvme68k common VMEbus front-end */
	sc->sc_mvmebus.sc_bust = pa->pa_bust;
	sc->sc_mvmebus.sc_dmat = pa->pa_dmat;
	sc->sc_mvmebus.sc_chip = sc;
	sc->sc_mvmebus.sc_nmasters = VME1_NMASTERS;
	sc->sc_mvmebus.sc_masters = &vme_pcc_masters[0];
	sc->sc_mvmebus.sc_nslaves = VME1_NSLAVES;
	sc->sc_mvmebus.sc_slaves = &sc->sc_slave[0];
	sc->sc_mvmebus.sc_intr_establish = vme_pcc_intr_establish;
	sc->sc_mvmebus.sc_intr_disestablish = vme_pcc_intr_disestablish;

	/* Initialize the chip. */
	reg = vme1_reg_read(sc, VME1REG_SCON) & ~VME1_SCON_SYSFAIL;
	vme1_reg_write(sc, VME1REG_SCON, reg);

	printf(": Type 1 VMEchip, scon jumper %s\n",
	    (reg & VME1_SCON_SWITCH) ? "enabled" : "disabled");

	/*
	 * Adjust the start address of the first range in vme_pcc_masters[]
	 * according to how much onboard memory exists. Disable the first
	 * range if onboard memory >= 16Mb, and adjust the start of the
	 * second range (A32D32).
	 */
	vme_pcc_masters[0].vr_vmestart = (vme_addr_t) mem_clusters[0].size;
	if (mem_clusters[0].size >= 0x01000000) {
		vme_pcc_masters[0].vr_am = MVMEBUS_AM_DISABLED;
		vme_pcc_masters[1].vr_vmestart +=
		    (vme_addr_t) (mem_clusters[0].size - 0x01000000);
	}

	am = 0;
	reg = vme1_reg_read(sc, VME1REG_SLADDRMOD);
	if ((reg & VME1_SLMOD_DATA) != 0)
		am |= MVMEBUS_AM_CAP_DATA;
	if ((reg & VME1_SLMOD_PRGRM) != 0)
		am |= MVMEBUS_AM_CAP_PROG;
	if ((reg & VME1_SLMOD_SUPER) != 0)
		am |= MVMEBUS_AM_CAP_SUPER;
	if ((reg & VME1_SLMOD_USER) != 0)
		am |= MVMEBUS_AM_CAP_USER;
	if ((reg & VME1_SLMOD_BLOCK) != 0)
		am |= MVMEBUS_AM_CAP_BLK;

#ifdef notyet
	if ((reg & VME1_SLMOD_SHORT) != 0) {
		sc->sc_slave[VME1_SLAVE_A16].vr_am = am | VME_AM_A16;
		sc->sc_slave[VME1_SLAVE_A16].vr_mask = 0xffffu;
	} else
#endif
		sc->sc_slave[VME1_SLAVE_A16].vr_am = MVMEBUS_AM_DISABLED;

	if (pcc_slave_base_addr < 0x01000000u && (reg & VME1_SLMOD_STND) != 0) {
		sc->sc_slave[VME1_SLAVE_A24].vr_am = am | VME_AM_A24;
		sc->sc_slave[VME1_SLAVE_A24].vr_datasize = VME_D32 |
		    VME_D16 | VME_D8;
		sc->sc_slave[VME1_SLAVE_A24].vr_mask = 0xffffffu;
		sc->sc_slave[VME1_SLAVE_A24].vr_locstart = 0;
		sc->sc_slave[VME1_SLAVE_A24].vr_vmestart = pcc_slave_base_addr;
		sc->sc_slave[VME1_SLAVE_A24].vr_vmeend = (pcc_slave_base_addr +
		    mem_clusters[0].size - 1) & 0x00ffffffu;
	} else
		sc->sc_slave[VME1_SLAVE_A24].vr_am = MVMEBUS_AM_DISABLED;

	if ((reg & VME1_SLMOD_EXTED) != 0) {
		sc->sc_slave[VME1_SLAVE_A32].vr_am = am | VME_AM_A32;
		sc->sc_slave[VME1_SLAVE_A32].vr_datasize = VME_D32 |
		    VME_D16 | VME_D8;
		sc->sc_slave[VME1_SLAVE_A32].vr_mask = 0xffffffffu;
		sc->sc_slave[VME1_SLAVE_A32].vr_locstart = 0;
		sc->sc_slave[VME1_SLAVE_A32].vr_vmestart = pcc_slave_base_addr;
		sc->sc_slave[VME1_SLAVE_A32].vr_vmeend =
		    pcc_slave_base_addr + mem_clusters[0].size - 1;
	} else
		sc->sc_slave[VME1_SLAVE_A32].vr_am = MVMEBUS_AM_DISABLED;

	vme_pcc_attached = 1;

	mvmebus_attach(&sc->sc_mvmebus);
}

void
vme_pcc_intr_establish(csc, prior, level, vector, first, func, arg)
	void *csc;
	int prior, level, vector, first;
	int (*func)(void *);
	void *arg;
{
	struct vme_pcc_softc *sc = csc;

	if (prior != level)
		panic("vme_pcc_intr_establish: cpu priority != VMEbus irq level");

	isrlink_vectored(func, arg, prior, vector);

	if (first) {
		/*
		 * There had better not be another VMEbus master responding
		 * to this interrupt level...
		 */
		vme1_reg_write(sc, VME1REG_IRQEN,
		    vme1_reg_read(sc, VME1REG_IRQEN) | VME1_IRQ_VME(level));
	}
}

void
vme_pcc_intr_disestablish(csc, level, vector, last)
	void *csc;
	int level, vector, last;
{
	struct vme_pcc_softc *sc = csc;

	isrunlink_vectored(vector);

	if (last) {
		vme1_reg_write(sc, VME1REG_IRQEN,
		    vme1_reg_read(sc, VME1REG_IRQEN) & ~VME1_IRQ_VME(level));
	}
}
