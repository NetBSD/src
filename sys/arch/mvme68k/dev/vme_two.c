/*	$NetBSD: vme_two.c,v 1.13 2001/05/31 18:46:08 scw Exp $ */

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
 * VME support specific to the VMEchip2 found on the MVME-1[67][27] boards.
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/cpu.h>
#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <mvme68k/mvme68k/isr.h>

#include <mvme68k/dev/mainbus.h>
#include <mvme68k/dev/mvmebus.h>
#include <mvme68k/dev/vme_tworeg.h>
#include <mvme68k/dev/vme_twovar.h>

int vmetwo_match __P((struct device *, struct cfdata *, void *));
void vmetwo_attach __P((struct device *, struct device *, void *));

struct cfattach vmetwo_ca = {
	sizeof(struct vmetwo_softc), vmetwo_match, vmetwo_attach
};

extern struct cfdriver vmetwo_cd;


/*
 * Array of interrupt handlers registered with us for the non-VMEbus
 * vectored interrupts. Eg. ABORT Switch, SYSFAIL etc.
 *
 * We can't just install a caller's handler directly, since these
 * interrupts have to be manually cleared, so we have a trampoline
 * which does the clearing automatically.
 */
static struct vme_two_handler {
	int (*isr_hand) __P((void *));
	void *isr_arg;
} vme_two_handlers[(VME2_VECTOR_LOCAL_MAX - VME2_VECTOR_LOCAL_MIN) + 1];

#define VMETWO_HANDLERS_SZ	(sizeof(vme_two_handlers) /	\
				 sizeof(struct vme_two_handler))

static struct vmetwo_softc *vmetwo_sc;

void vmetwo_master_range __P((struct vmetwo_softc *, int,
			      struct mvmebus_range *));
void vmetwo_slave_range __P((struct vmetwo_softc *, int, vme_am_t,
			      struct mvmebus_range *));
int vmetwo_local_isr_trampoline __P((void *));
void vmetwo_intr_establish __P((void *, int, int, int, int,
				int (*)(void *), void *, struct evcnt *));
void vmetwo_intr_disestablish __P((void *, int, int, int, struct evcnt *));


/* ARGSUSED */
int
vmetwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct mainbus_attach_args *ma;

	ma = aux;

	if (strcmp(ma->ma_name, vmetwo_cd.cd_name))
		return (0);

	/* Only one VMEchip2, please. */
	if (vmetwo_sc)
		return (0);

	if (machineid != MVME_167 && machineid != MVME_177 &&
	    machineid != MVME_162 && machineid != MVME_172)
		return (0);

	return (1);
}

/* ARGSUSED */
void
vmetwo_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mainbus_attach_args *ma;
	struct vmetwo_softc *sc;
	u_int32_t reg;
	int i;

	sc = vmetwo_sc = (struct vmetwo_softc *) self;
	ma = aux;

	/*
	 * Map the local control registers
	 */
	bus_space_map(ma->ma_bust, ma->ma_offset + VME2REG_LCSR_OFFSET,
	    VME2LCSR_SIZE, 0, &sc->sc_lcrh);

#ifdef notyet
	/*
	 * Map the global control registers
	 */
	bus_space_map(ma->ma_bust, ma->ma_offset + VME2REG_GCSR_OFFSET,
	    VME2GCSR_SIZE, 0, &sc->sc_gcrh);
#endif

	/* Initialise stuff for the mvme68k common VMEbus front-end */
	sc->sc_mvmebus.sc_bust = ma->ma_bust;
	sc->sc_mvmebus.sc_dmat = ma->ma_dmat;
	sc->sc_mvmebus.sc_chip = sc;
	sc->sc_mvmebus.sc_nmasters = VME2_NMASTERS;
	sc->sc_mvmebus.sc_masters = &sc->sc_master[0];
	sc->sc_mvmebus.sc_nslaves = VME2_NSLAVES;
	sc->sc_mvmebus.sc_slaves = &sc->sc_slave[0];
	sc->sc_mvmebus.sc_intr_establish = vmetwo_intr_establish;
	sc->sc_mvmebus.sc_intr_disestablish = vmetwo_intr_disestablish;

	/* Clear out the ISR handler array */
	for (i = 0; i < VMETWO_HANDLERS_SZ; i++)
		vme_two_handlers[i].isr_hand = NULL;

	/*
	 * Initialize the chip.
	 * Firstly, disable all VMEChip2 Interrupts
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_MISC_STATUS) & ~VME2_MISC_STATUS_MIEN;
	vme2_lcsr_write(sc, VME2LCSR_MISC_STATUS, reg);
	vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE, 0);
	vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_CLEAR,
	    VME2_LOCAL_INTERRUPT_CLEAR_ALL);

	/* Zap all the IRQ level registers */
	for (i = 0; i < VME2_NUM_IL_REGS; i++)
		vme2_lcsr_write(sc, VME2LCSR_INTERRUPT_LEVEL_BASE + (i * 4), 0);

	/* Disable the tick timers */
	reg = vme2_lcsr_read(sc, VME2LCSR_TIMER_CONTROL);
	reg &= ~VME2_TIMER_CONTROL_EN(0);
	reg &= ~VME2_TIMER_CONTROL_EN(1);
	vme2_lcsr_write(sc, VME2LCSR_TIMER_CONTROL, reg);

	/* Set the VMEChip2's vector base register to the required value */
	reg = vme2_lcsr_read(sc, VME2LCSR_VECTOR_BASE);
	reg &= ~VME2_VECTOR_BASE_MASK;
	reg |= VME2_VECTOR_BASE_REG_VALUE;
	vme2_lcsr_write(sc, VME2LCSR_VECTOR_BASE, reg);

	/* Set the Master Interrupt Enable bit now */
	reg = vme2_lcsr_read(sc, VME2LCSR_MISC_STATUS) | VME2_MISC_STATUS_MIEN;
	vme2_lcsr_write(sc, VME2LCSR_MISC_STATUS, reg);

	reg = vme2_lcsr_read(sc, VME2LCSR_BOARD_CONTROL);
	printf(": Type 2 VMEchip, scon jumper %s\n",
	    (reg & VME2_BOARD_CONTROL_SCON) ? "enabled" : "disabled");

	/*
	 * Figure out what bits of the VMEbus we can access.
	 * First record the `fixed' maps (if they're enabled)
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_IO_CONTROL);
	if (reg & VME2_IO_CONTROL_I1EN) {
		/* This range is fixed to A16, DATA */
		sc->sc_master[0].vr_am = VME_AM_A16 | MVMEBUS_AM_CAP_DATA;

		/* However, SUPER/USER is selectable... */
		if (reg & VME2_IO_CONTROL_I1SU)
			sc->sc_master[0].vr_am |= MVMEBUS_AM_CAP_SUPER;
		else
			sc->sc_master[0].vr_am |= MVMEBUS_AM_CAP_USER;

		/* As is the datasize */
		sc->sc_master[0].vr_datasize = VME_D32 | VME_D16 | VME_D8;
		if (reg & VME2_IO_CONTROL_I1D16)
			sc->sc_master[0].vr_datasize &= ~VME_D32;

		sc->sc_master[0].vr_locstart = VME2_IO0_LOCAL_START;
		sc->sc_master[0].vr_mask = VME2_IO0_MASK;
		sc->sc_master[0].vr_vmestart = VME2_IO0_VME_START;
		sc->sc_master[0].vr_vmeend = VME2_IO0_VME_END;
	} else
		sc->sc_master[0].vr_am = MVMEBUS_AM_DISABLED;

	if (reg & VME2_IO_CONTROL_I2EN) {
		/* These two ranges are fixed to A24D16 and A32D16 */
		sc->sc_master[1].vr_am = VME_AM_A24;
		sc->sc_master[1].vr_datasize = VME_D16 | VME_D8;
		sc->sc_master[2].vr_am = VME_AM_A32;
		sc->sc_master[2].vr_datasize = VME_D16 | VME_D8;

		/* However, SUPER/USER is selectable */
		if (reg & VME2_IO_CONTROL_I2SU) {
			sc->sc_master[1].vr_am |= MVMEBUS_AM_CAP_SUPER;
			sc->sc_master[2].vr_am |= MVMEBUS_AM_CAP_SUPER;
		} else {
			sc->sc_master[1].vr_am |= MVMEBUS_AM_CAP_USER;
			sc->sc_master[2].vr_am |= MVMEBUS_AM_CAP_USER;
		}

		/* As is PROGRAM/DATA */
		if (reg & VME2_IO_CONTROL_I2PD) {
			sc->sc_master[1].vr_am |= MVMEBUS_AM_CAP_PROG;
			sc->sc_master[2].vr_am |= MVMEBUS_AM_CAP_PROG;
		} else {
			sc->sc_master[1].vr_am |= MVMEBUS_AM_CAP_DATA;
			sc->sc_master[2].vr_am |= MVMEBUS_AM_CAP_DATA;
		}

		sc->sc_master[1].vr_locstart = VME2_IO1_LOCAL_START;
		sc->sc_master[1].vr_mask = VME2_IO1_MASK;
		sc->sc_master[1].vr_vmestart = VME2_IO1_VME_START;
		sc->sc_master[1].vr_vmeend = VME2_IO1_VME_END;

		sc->sc_master[2].vr_locstart = VME2_IO2_LOCAL_START;
		sc->sc_master[2].vr_mask = VME2_IO2_MASK;
		sc->sc_master[2].vr_vmestart = VME2_IO2_VME_START;
		sc->sc_master[2].vr_vmeend = VME2_IO2_VME_END;
	} else {
		sc->sc_master[1].vr_am = MVMEBUS_AM_DISABLED;
		sc->sc_master[2].vr_am = MVMEBUS_AM_DISABLED;
	}

	/*
	 * Now read the progammable maps
	 */
	for (i = 0; i < VME2_MASTER_WINDOWS; i++)
		vmetwo_master_range(sc, i,
		    &(sc->sc_master[i + VME2_MASTER_PROG_START]));

	/* XXX: No A16 slave yet :XXX */
	sc->sc_slave[VME2_SLAVE_A16].vr_am = MVMEBUS_AM_DISABLED;

	for (i = 0; i < VME2_SLAVE_WINDOWS; i++) {
		vmetwo_slave_range(sc, i, VME_AM_A32,
		    &sc->sc_slave[i + VME2_SLAVE_PROG_START]);
		vmetwo_slave_range(sc, i, VME_AM_A24,
		    &sc->sc_slave[i + VME2_SLAVE_PROG_START + 2]);
	}

	if (machineid != MVME_162 && machineid != MVME_172) {
		/*
		 * Let the NMI handler deal with level 7 ABORT switch
		 * interrupts
		 */
		vmetwo_intr_establish(sc, 7, 7, VME2_VEC_ABORT, 1,
		    nmihand, NULL, NULL);
	}

	mvmebus_attach(&sc->sc_mvmebus);
}

void
vmetwo_master_range(sc, range, vr)
	struct vmetwo_softc *sc;
	int range;
	struct mvmebus_range *vr;
{
	u_int32_t start, end, attr;
	u_int32_t reg;

	/*
	 * First, check if the range is actually enabled...
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_MASTER_ENABLE);
	if ((reg & VME2_MASTER_ENABLE(range)) == 0) {
		vr->vr_am = MVMEBUS_AM_DISABLED;
		return;
	}

	/*
	 * Fetch and record the range's attributes
	 */
	attr = vme2_lcsr_read(sc, VME2LCSR_MASTER_ATTR);
	attr >>= VME2_MASTER_ATTR_AM_SHIFT(range);

	/*
	 * Fix up the datasizes available through this range
	 */
	vr->vr_datasize = VME_D32 | VME_D16 | VME_D8;
	if (attr & VME2_MASTER_ATTR_D16)
		vr->vr_datasize &= ~VME_D32;
	attr &= VME2_MASTER_ATTR_AM_MASK;

	vr->vr_am = (attr & VME_AM_ADRSIZEMASK) | MVMEBUS_AM2CAP(attr);
	switch (vr->vr_am & VME_AM_ADRSIZEMASK) {
	case VME_AM_A32:
	default:
		vr->vr_mask = 0xffffffffu;
		break;

	case VME_AM_A24:
		vr->vr_mask = 0x00ffffffu;
		break;

	case VME_AM_A16:
		vr->vr_mask = 0x0000ffffu;
		break;
	}

	/*
	 * XXX
	 * It would be nice if users of the MI VMEbus code could pass down
	 * whether they can tolerate Write-Posting to their device(s).
	 * XXX
	 */

	/*
	 * Fetch the local-bus start and end addresses for the range
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_MASTER_ADDRESS(range));
	start = (reg & VME2_MAST_ADDRESS_START_MASK);
	start <<= VME2_MAST_ADDRESS_START_SHIFT;
	end = (reg & VME2_MAST_ADDRESS_END_MASK);
	end <<= VME2_MAST_ADDRESS_END_SHIFT;

	/*
	 * Local->VMEbus map '4' has optional translation bits, so
	 * the VMEbus start and end addresses may need to be adjusted.
	 */
	if (range == 3 && (reg = vme2_lcsr_read(sc, VME2LCSR_MAST4_TRANS))!=0) {
		uint32_t addr, sel, len = end - start;

		vr->vr_locstart = start;

		reg = vme2_lcsr_read(sc, VME2LCSR_MAST4_TRANS);
		reg &= VME2_MAST4_TRANS_SELECT_MASK;
		sel = reg << VME2_MAST4_TRANS_SELECT_SHIFT;

		reg = vme2_lcsr_read(sc, VME2LCSR_MAST4_TRANS);
		reg &= VME2_MAST4_TRANS_ADDRESS_MASK;
		addr = reg << VME2_MAST4_TRANS_ADDRESS_SHIFT;

		start = (addr & sel) | (start & (~sel));
		end = start + len;
		vr->vr_mask &= len - 1;
	} else
		vr->vr_locstart = 0;

	/* XXX Deal with overlap of onboard RAM address space */
	/* XXX Then again, 167-Bug warns about this at setup time ... */

	/*
	 * Fixup the addresses this range corresponds to
	 */
	vr->vr_vmestart = start;
	vr->vr_vmeend = end - 1;
}

void
vmetwo_slave_range(sc, range, am, vr)
	struct vmetwo_softc *sc;
	int range;
	vme_am_t am;
	struct mvmebus_range *vr;
{
	u_int32_t reg;

	/*
	 * First, check if the range is actually enabled.
	 * Note that bit 1 of `range' is used to indicte if we're
	 * looking for an A24 range (set) or an A32 range (clear).
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_SLAVE_CTRL);

	if (am == VME_AM_A32 && (reg & VME2_SLAVE_AMSEL_A32(range))) {
		vr->vr_am = VME_AM_A32;
		vr->vr_mask = 0xffffffffu;
	} else
	if (am == VME_AM_A24 && (reg & VME2_SLAVE_AMSEL_A24(range))) {
		vr->vr_am = VME_AM_A24;
		vr->vr_mask = 0x00ffffffu;
	} else {
		/* The range is not enabled */
		vr->vr_am = MVMEBUS_AM_DISABLED;
		return;
	}

	if ((reg & VME2_SLAVE_AMSEL_DAT(range)) != 0)
		vr->vr_am |= MVMEBUS_AM_CAP_DATA;

	if ((reg & VME2_SLAVE_AMSEL_PGM(range)) != 0)
		vr->vr_am |= MVMEBUS_AM_CAP_PROG;

	if ((reg & VME2_SLAVE_AMSEL_USR(range)) != 0)
		vr->vr_am |= MVMEBUS_AM_CAP_USER;

	if ((reg & VME2_SLAVE_AMSEL_SUP(range)) != 0)
		vr->vr_am |= MVMEBUS_AM_CAP_SUPER;

	if ((reg & VME2_SLAVE_AMSEL_BLK(range)) != 0)
		vr->vr_am |= MVMEBUS_AM_CAP_BLK;

	if ((reg & VME2_SLAVE_AMSEL_BLKD64(range)) != 0)
		vr->vr_am |= MVMEBUS_AM_CAP_BLKD64;

	vr->vr_datasize = VME_D32 | VME_D16 | VME_D8;

	/*
	 * Record the VMEbus start and end addresses of the slave image
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_SLAVE_ADDRESS(range));
	vr->vr_vmestart = reg & VME2_SLAVE_ADDRESS_START_MASK;
	vr->vr_vmestart <<= VME2_SLAVE_ADDRESS_START_SHIFT;
	vr->vr_vmestart &= vr->vr_mask;
	vr->vr_vmeend = reg & VME2_SLAVE_ADDRESS_END_MASK;
	vr->vr_vmeend <<= VME2_SLAVE_ADDRESS_END_SHIFT;
	vr->vr_vmeend &= vr->vr_mask;
	vr->vr_vmeend |= 0xffffu;

	/*
	 * Now figure out the local-bus address
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_SLAVE_CTRL);
	if ((reg & VME2_SLAVE_CTRL_ADDER(range)) != 0) {
		reg = vme2_lcsr_read(sc, VME2LCSR_SLAVE_TRANS(range));
		reg &= VME2_SLAVE_TRANS_ADDRESS_MASK;
		reg <<= VME2_SLAVE_TRANS_ADDRESS_SHIFT;
		vr->vr_locstart = vr->vr_vmestart + reg;
	} else {
		u_int32_t sel, addr;

		reg = vme2_lcsr_read(sc, VME2LCSR_SLAVE_TRANS(range));
		sel = reg & VME2_SLAVE_TRANS_SELECT_MASK;
		sel <<= VME2_SLAVE_TRANS_SELECT_SHIFT;
		addr = reg & VME2_SLAVE_TRANS_ADDRESS_MASK;
		addr <<= VME2_SLAVE_TRANS_ADDRESS_SHIFT;

		vr->vr_locstart = addr & sel;
		vr->vr_locstart |= vr->vr_vmestart & (~sel);
	}
}

int
vmetwo_local_isr_trampoline(arg)
	void *arg;
{
	struct vme_two_handler *isr;
	int s, vec;

	vec = (int) arg;	/* 0x08 <= vec <= 0x1f */

	/* No interrupts while we fiddle with the registers, please */
	s = splhigh();

	/* Clear the interrupt source */
	vme2_lcsr_write(vmetwo_sc, VME2LCSR_LOCAL_INTERRUPT_CLEAR,
	    VME2_LOCAL_INTERRUPT(vec));

	splx(s);

	isr = &vme_two_handlers[vec - VME2_VECTOR_LOCAL_OFFSET];
	if (isr->isr_hand)
		(void) (*isr->isr_hand) (isr->isr_arg);
	else
		printf("vmetwo: Spurious local interrupt, vector 0x%x\n", vec);

	return (1);
}

void
vmetwo_intr_establish(csc, prior, lvl, vec, first, hand, arg, evcnt)
	void *csc;
	int prior, lvl, vec, first;
	int (*hand)(void *);
	void *arg;
	struct evcnt *evcnt;
{
	struct vmetwo_softc *sc = csc;
	u_int32_t reg;
	int bitoff;
	int iloffset, ilshift;
	int s;

	s = splhigh();

	/*
	 * Sort out interrupts generated locally by the VMEChip2 from
	 * those generated by VMEbus devices...
	 */
	if (vec >= VME2_VECTOR_LOCAL_MIN && vec <= VME2_VECTOR_LOCAL_MAX) {
		/*
		 * Local interrupts need to be bounced through some
		 * trampoline code which acknowledges/clears them.
		 */
		vme_two_handlers[vec - VME2_VECTOR_LOCAL_MIN].isr_hand = hand;
		vme_two_handlers[vec - VME2_VECTOR_LOCAL_MIN].isr_arg = arg;
		hand = vmetwo_local_isr_trampoline;
		arg = (void *) (vec - VME2_VECTOR_BASE);

		/*
		 * Interrupt enable/clear bit offset is 0x08 - 0x1f
		 */
		bitoff = vec - VME2_VECTOR_BASE;
		first = 1;	/* Force the interrupt to be enabled */
	} else {
		/*
		 * Interrupts originating from the VMEbus are
		 * controlled by an offset of 0x00 - 0x07
		 */
		bitoff = lvl - 1;
	}

	/* Hook the interrupt */
	isrlink_vectored(hand, arg, prior, vec, evcnt);

	/*
	 * Do we need to tell the VMEChip2 to let the interrupt through?
	 * (This is always true for locally-generated interrupts, but only
	 * needs doing once for each VMEbus interrupt level which is hooked)
	 */
	if (first) {
		if (evcnt)
			evcnt_attach_dynamic(evcnt, EVCNT_TYPE_INTR,
			    isrlink_evcnt(prior),
			    sc->sc_mvmebus.sc_dev.dv_xname,
			    mvmebus_irq_name[lvl]);

		iloffset = VME2_ILOFFSET_FROM_VECTOR(bitoff) +
		    VME2LCSR_INTERRUPT_LEVEL_BASE;
		ilshift = VME2_ILSHIFT_FROM_VECTOR(bitoff);

		/* Program the specified interrupt to signal at 'prior' */
		reg = vme2_lcsr_read(sc, iloffset);
		reg &= ~(VME2_INTERRUPT_LEVEL_MASK << ilshift);
		reg |= (prior << ilshift);
		vme2_lcsr_write(sc, iloffset, reg);

		/* Clear it */
		vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_CLEAR,
		    VME2_LOCAL_INTERRUPT(bitoff));

		/* Enable it. */
		reg = vme2_lcsr_read(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE);
		reg |= VME2_LOCAL_INTERRUPT(bitoff);
		vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE, reg);
	}
#ifdef DIAGNOSTIC
	else {
		/* Verify the interrupt priority is the same */
		iloffset = VME2_ILOFFSET_FROM_VECTOR(bitoff) +
		    VME2LCSR_INTERRUPT_LEVEL_BASE;
		ilshift = VME2_ILSHIFT_FROM_VECTOR(bitoff);

		reg = vme2_lcsr_read(sc, iloffset);
		reg &= (VME2_INTERRUPT_LEVEL_MASK << ilshift);

		if ((prior << ilshift) != reg)
			panic("vmetwo_intr_establish: priority mismatch!");
	}
#endif
	splx(s);
}

void
vmetwo_intr_disestablish(csc, lvl, vec, last, evcnt)
	void *csc;
	int lvl, vec, last;
	struct evcnt *evcnt;
{
	struct vmetwo_softc *sc = csc;
	u_int32_t reg;
	int iloffset, ilshift;
	int bitoff;
	int s;

	s = splhigh();

	/*
	 * Sort out interrupts generated locally by the VMEChip2 from
	 * those generated by VMEbus devices...
	 */
	if (vec >= VME2_VECTOR_LOCAL_MIN && vec <= VME2_VECTOR_LOCAL_MAX) {
		/*
		 * Interrupt enable/clear bit offset is 0x08 - 0x1f
		 */
		bitoff = vec - VME2_VECTOR_BASE;
		vme_two_handlers[vec - VME2_VECTOR_LOCAL_MIN].isr_hand = NULL;
		last = 1; /* Force the interrupt to be cleared */
	} else {
		/*
		 * Interrupts originating from the VMEbus are
		 * controlled by an offset of 0x00 - 0x07
		 */
		bitoff = lvl - 1;
	}

	/*
	 * Do we need to tell the VMEChip2 to block the interrupt?
	 * (This is always true for locally-generated interrupts, but only
	 * needs doing once when the last VMEbus handler for any given level
	 * has been unhooked.)
	 */
	if (last) {
		iloffset = VME2_ILOFFSET_FROM_VECTOR(bitoff) +
		    VME2LCSR_INTERRUPT_LEVEL_BASE;
		ilshift = VME2_ILSHIFT_FROM_VECTOR(bitoff);

		/* Disable it. */
		reg = vme2_lcsr_read(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE);
		reg &= ~VME2_LOCAL_INTERRUPT(bitoff);
		vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE, reg);

		/* Set the interrupt's level to zero */
		reg = vme2_lcsr_read(sc, iloffset);
		reg &= ~(VME2_INTERRUPT_LEVEL_MASK << ilshift);
		vme2_lcsr_write(sc, iloffset, reg);

		/* Clear it */
		vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_CLEAR,
		    VME2_LOCAL_INTERRUPT(vec));

		if (evcnt)
			evcnt_detach(evcnt);
	}
	/* Un-hook it */
	isrunlink_vectored(vec);

	splx(s);
}
