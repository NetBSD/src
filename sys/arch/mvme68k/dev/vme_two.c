/*	$NetBSD: vme_two.c,v 1.3 2000/06/04 19:14:50 cgd Exp $ */

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
 * VME support specific to the VMEchip2 found on the MVME-1[67]7 boards.
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
			      struct vmetwo_range *));
int vmetwo_local_isr_trampoline __P((void *));
const struct evcnt *vmetwo_intr_evcnt __P((struct vmetwo_softc *, int));
void vmetwo_intr_establish __P((struct vmetwo_softc *, int, int,
	                      int (*) (void *), void *));
void vmetwo_intr_disestablish __P((struct vmetwo_softc *, int, int));

#ifdef DIAGNOSTIC
static const char *_vme2_mod_string __P((vme_addr_t, vme_size_t,
					 vme_am_t, vme_datasize_t));
#endif

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

	if (machineid != MVME_167 && machineid != MVME_177)
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
	struct vmebus_attach_args vaa;
	struct vmetwo_softc *sc;
	u_int32_t reg;
	int i;

	sc = vmetwo_sc = (struct vmetwo_softc *) self;
	ma = aux;

	sc->sc_dmat = ma->ma_dmat;
	sc->sc_bust = ma->ma_bust;
	sc->sc_vmet = MVME68K_VME_BUS_SPACE;

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

	/* Clear out the ISR handler array */
	for (i = 0; i < VMETWO_HANDLERS_SZ; i++)
		vme_two_handlers[i].isr_hand = NULL;

	/* Zap the IRQ reference counts */
	for (i = 0; i < 8; i++)
		sc->sc_irqref[i] = 0;

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

	/* Disable the VMEbus Slave Windows for now */
	vme2_lcsr_write(sc, VME2LCSR_SLAVE_CTRL, 0);

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
		sc->sc_ranges[0].vr_am = VME_AM_MBO | VME_AM_A16 | VME_AM_DATA;

		/* However, SUPER/USER is selectable... */
		if (reg & VME2_IO_CONTROL_I1SU)
			sc->sc_ranges[0].vr_am |= VME_AM_SUPER;
		else
			sc->sc_ranges[0].vr_am |= VME_AM_USER;

		/* As is the datasize */
		if (reg & VME2_IO_CONTROL_I1D16)
			sc->sc_ranges[0].vr_datasize = VME_D16;
		else
			sc->sc_ranges[0].vr_datasize = VME_D32 | VME_D16;

		sc->sc_ranges[0].vr_locstart = VME2_IO0_LOCAL_START;
		sc->sc_ranges[0].vr_mask = VME2_IO0_MASK;
		sc->sc_ranges[0].vr_vmestart = VME2_IO0_VME_START;
		sc->sc_ranges[0].vr_vmeend = VME2_IO0_VME_END;
	} else
		sc->sc_ranges[0].vr_am = VME2_AM_DISABLED;

	if (reg & VME2_IO_CONTROL_I2EN) {
		/* These two ranges are fixed to A24D16 and A32D16 */
		sc->sc_ranges[1].vr_am = VME_AM_MBO | VME_AM_A24;
		sc->sc_ranges[1].vr_datasize = VME_D16;
		sc->sc_ranges[2].vr_am = VME_AM_MBO | VME_AM_A32;
		sc->sc_ranges[2].vr_datasize = VME_D16;

		/* However, SUPER/USER is selectable */
		if (reg & VME2_IO_CONTROL_I2SU) {
			sc->sc_ranges[1].vr_am |= VME_AM_SUPER;
			sc->sc_ranges[2].vr_am |= VME_AM_SUPER;
		} else {
			sc->sc_ranges[1].vr_am |= VME_AM_USER;
			sc->sc_ranges[2].vr_am |= VME_AM_USER;
		}

		/* As is PROGRAM/DATA */
		if (reg & VME2_IO_CONTROL_I2PD) {
			sc->sc_ranges[1].vr_am |= VME_AM_PRG;
			sc->sc_ranges[2].vr_am |= VME_AM_PRG;
		} else {
			sc->sc_ranges[1].vr_am |= VME_AM_DATA;
			sc->sc_ranges[2].vr_am |= VME_AM_DATA;
		}

		sc->sc_ranges[1].vr_locstart = VME2_IO1_LOCAL_START;
		sc->sc_ranges[1].vr_mask = VME2_IO1_MASK;
		sc->sc_ranges[1].vr_vmestart = VME2_IO1_VME_START;
		sc->sc_ranges[1].vr_vmeend = VME2_IO1_VME_END;

		sc->sc_ranges[2].vr_locstart = VME2_IO2_LOCAL_START;
		sc->sc_ranges[2].vr_mask = VME2_IO2_MASK;
		sc->sc_ranges[2].vr_vmestart = VME2_IO2_VME_START;
		sc->sc_ranges[2].vr_vmeend = VME2_IO2_VME_END;
	} else {
		sc->sc_ranges[1].vr_am = VME2_AM_DISABLED;
		sc->sc_ranges[2].vr_am = VME2_AM_DISABLED;
	}

	/*
	 * Now read the progammable maps
	 */
	for (i = 0; i < VME2_MASTER_WINDOWS; i++)
		vmetwo_master_range(sc, i, &(sc->sc_ranges[i + 3]));

#ifdef DIAGNOSTIC
	for (i = 0; i < VME2_NRANGES; i++) {
		if (sc->sc_ranges[i].vr_am == VME2_AM_DISABLED) {
			printf("%s: Map#%d: disabled\n",
			    sc->sc_dev.dv_xname, i);
			continue;
		}
		printf("%s: Map#%d: 0x%08lx -> %s\n",
		    sc->sc_dev.dv_xname, i,
		    sc->sc_ranges[i].vr_locstart,
		    _vme2_mod_string(sc->sc_ranges[i].vr_vmestart,
			(sc->sc_ranges[i].vr_vmeend -
			    sc->sc_ranges[i].vr_vmestart) + 1,
			sc->sc_ranges[i].vr_am,
			sc->sc_ranges[i].vr_datasize));
	}
#endif

	sc->sc_vct.cookie = self;
	sc->sc_vct.vct_probe = _vmetwo_probe;
	sc->sc_vct.vct_map = _vmetwo_map;
	sc->sc_vct.vct_unmap = _vmetwo_unmap;
	sc->sc_vct.vct_int_map = _vmetwo_intmap;
	sc->sc_vct.vct_int_evcnt = _vmetwo_intr_evcnt;
	sc->sc_vct.vct_int_establish = _vmetwo_intr_establish;
	sc->sc_vct.vct_int_disestablish = _vmetwo_intr_disestablish;
	sc->sc_vct.vct_dmamap_create = _vmetwo_dmamap_create;
	sc->sc_vct.vct_dmamap_destroy = _vmetwo_dmamap_destroy;
	sc->sc_vct.vct_dmamem_alloc = _vmetwo_dmamem_alloc;
	sc->sc_vct.vct_dmamem_free = _vmetwo_dmamem_free;

	vaa.va_vct = &(sc->sc_vct);
	vaa.va_bdt = sc->sc_dmat;
	vaa.va_slaveconfig = NULL;

	/* Let the NMI handler deal with level 7 ABORT switch interrupts */
	vmetwo_intr_establish(sc, 7, VME2_VEC_ABORT, nmihand, NULL);

	/* Attach the MI VMEbus glue. */
	config_found(self, &vaa, 0);
}

void
vmetwo_master_range(sc, range, vr)
	struct vmetwo_softc *sc;
	int range;
	struct vmetwo_range *vr;
{
	u_int32_t start, end, attr;
	u_int32_t reg;

	/*
	 * First, check if the range is actually enabled...
	 */
	reg = vme2_lcsr_read(sc, VME2LCSR_MASTER_ENABLE);
	if ((reg & VME2_MASTER_ENABLE(range)) == 0) {
		vr->vr_am = VME2_AM_DISABLED;
		return;
	}

	/*
	 * Fetch and record the range's attributes
	 */
	attr = vme2_lcsr_read(sc, VME2LCSR_MASTER_ATTR);
	attr >>= VME2_MASTER_ATTR_AM_SHIFT(range);
	vr->vr_am = VME_AM_MBO | (attr & VME2_MASTER_ATTR_AM_MASK);

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
	 * Fix up the datasizes available through this range
	 */
	if (attr & VME2_MASTER_ATTR_D16)
		vr->vr_datasize = VME_D16 | VME_D8;
	else
		vr->vr_datasize = VME_D32 | VME_D16 | VME_D8;

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
	 *
	 * Note that if the translation register is zero, translation
	 * is not enabled. This code works either way.
	 */
	if (range == 3) {
		vr->vr_locstart = start;

		reg = vme2_lcsr_read(sc, VME2LCSR_MAST4_TRANS);
		reg &= VME2_MAST4_TRANS_SELECT_MASK;
		reg <<= VME2_MAST4_TRANS_SELECT_SHIFT;
		vr->vr_mask &= ~reg;
		start &= ~reg;
		end &= ~reg;

		reg = vme2_lcsr_read(sc, VME2LCSR_MAST4_TRANS);
		reg &= VME2_MAST4_TRANS_ADDRESS_MASK;
		reg <<= VME2_MAST4_TRANS_ADDRESS_SHIFT;
		start |= reg;
		end |= reg;
	} else
		vr->vr_locstart = 0;

	/*
	 * Fixup the addresses this range corresponds to
	 */
	vr->vr_vmestart = start;
	vr->vr_vmeend = end - 1;
}

/* ARGSUSED */
int
_vmetwo_map(vsc, vmeaddr, len, am, datasize, swap, tag, handle, resc)
	void *vsc;
	vme_addr_t vmeaddr;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasize;
	vme_swap_t swap;
	bus_space_tag_t *tag;
	bus_space_handle_t *handle;
	vme_mapresc_t *resc;
{
	struct vmetwo_softc *sc;
	struct vmetwo_mapresc_t *pm;
	struct vmetwo_range *vr;
	vme_addr_t end, mask;
	paddr_t paddr;
	int rv, i;

	sc = vsc;
	end = (vmeaddr + len) - 1;
	mask = 0;
	paddr = 0;

	/*
	 * Scan each enabled range to see if we can access the
	 * requested VMEbus addresses
	 */
	for (i = 0, vr = &sc->sc_ranges[0]; i < VME2_NRANGES; i++, vr++) {
		if (vr->vr_am == VME2_AM_DISABLED)
			continue;

		/*
		 * Check the range against the address modifier and datasize
		 */
		if (am != vr->vr_am || datasize > vr->vr_datasize)
			continue;

		/*
		 * Now check the range accesses the required VMEbus range
		 */
		if (vmeaddr >= vr->vr_vmestart && end <= vr->vr_vmeend) {
			/*
			 * We have a match. Compute the required local-bus
			 * address which maps to the VMEbus start address.
			 */
			paddr = vr->vr_locstart + (vmeaddr & vr->vr_mask);
			break;
		}
	}

	if (paddr == 0) {
#ifdef DIAGNOSTIC
		printf("%s: Unable to map %s\n", sc->sc_dev.dv_xname,
		    _vme2_mod_string(vmeaddr, len, am, datasize));
#endif
		return (ENOMEM);
	}
	if ((rv = bus_space_map(sc->sc_vmet, paddr, len, 0, handle)) != 0)
		return (rv);

	/* Allocate space for the resource tag */
	if ((pm = malloc(sizeof(*pm), M_DEVBUF, M_NOWAIT)) == NULL) {
		bus_space_unmap(sc->sc_vmet, *handle, len);
		return (ENOMEM);
	}

	/* Record the range's details */
	pm->pm_am = am;
	pm->pm_datasize = datasize;
	pm->pm_addr = vmeaddr;
	pm->pm_size = len;
	pm->pm_handle = *handle;
	pm->pm_range = i;

	*tag = sc->sc_vmet;
	*resc = (vme_mapresc_t *) pm;

	return (0);
}

void
_vmetwo_unmap(vsc, resc)
	void *vsc;
	vme_mapresc_t resc;
{
	struct vmetwo_softc *sc;
	struct vmetwo_mapresc_t *pm;

	sc = vsc;
	pm = (struct vmetwo_mapresc_t *) resc;

	bus_space_unmap(sc->sc_vmet, pm->pm_handle, pm->pm_size);

	free(pm, M_DEVBUF);
}

int
_vmetwo_probe(vsc, vmeaddr, len, am, datasize, callback, arg)
	void *vsc;
	vme_addr_t vmeaddr;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasize;
	int (*callback) __P((void *, bus_space_tag_t, bus_space_handle_t));
	void *arg;
{
	bus_space_tag_t tag;
	bus_space_handle_t handle;
	vme_mapresc_t resc;
	int rv;

	/* Get a temporary mapping to the VMEbus range */
	rv = _vmetwo_map(vsc, vmeaddr, len, am, datasize, 0,
	    &tag, &handle, &resc);
	if (rv)
		return (rv);

	if (callback)
		rv = (*callback) (arg, tag, handle);
	else {
		/*
		 * FIXME: Using badaddr() in this way may cause several
		 * accesses to each VMEbus address. Also, using 'handle' in
		 * this way is a bit presumptuous...
		 */
		rv = badaddr((caddr_t) handle, (int) len) ? EIO : 0;
	}

	_vmetwo_unmap(vsc, resc);

	return (rv);
}

/* ARGSUSED */
int
_vmetwo_intmap(vsc, level, vector, handlep)
	void *vsc;
	int level, vector;
	vme_intr_handle_t *handlep;
{

	if (level < 1 || level > 7 || vector < 0x80 || vector > 0xff)
		return (EINVAL);

	/* This is rather gross */
	*handlep = (void *) (int) ((level << 8) | vector);
	return (0);
}

const struct evcnt *
_vmetwo_intr_establish(vsc, handle, prior, func, arg)
	void *vsc;
	vme_intr_handle_t handle;
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
_vmetwo_intr_establish(vsc, handle, prior, func, arg)
	void *vsc;
	vme_intr_handle_t handle;
	int prior;
	int (*func) __P((void *));
	void *arg;
{
	struct vmetwo_softc *sc;
	int level, vector;

	sc = vsc;

	/* Extract the interrupt's level and vector */
	level = ((int) handle) >> 8;
	vector = ((int) handle) & 0xff;

	vmetwo_intr_establish(sc, level, vector, func, arg);

	return ((void *) handle);
}

void
_vmetwo_intr_disestablish(vsc, handle)
	void *vsc;
	vme_intr_handle_t handle;
{
	struct vmetwo_softc *sc;
	int level, vector;

	sc = vsc;

	/* Extract the interrupt's level and vector */
	level = ((int) handle) >> 8;
	vector = ((int) handle) & 0xff;

	vmetwo_intr_disestablish(sc, level, vector);
}

/* ARGSUSED */
int
_vmetwo_dmamap_create(vsc, len, am, datasize, swap, nsegs,
    segsz, bound, flags, mapp)
	void *vsc;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasize;
	vme_swap_t swap;
	int nsegs;
	vme_size_t segsz;
	vme_addr_t bound;
	int flags;
	bus_dmamap_t *mapp;
{
	return (EINVAL);
}

/* ARGSUSED */
void
_vmetwo_dmamap_destroy(vsc, map)
	void *vsc;
	bus_dmamap_t map;
{
}

/* ARGSUSED */
int
_vmetwo_dmamem_alloc(vsc, len, am, datasizes, swap,
    segs, nsegs, rsegs, flags)
	void *vsc;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t datasizes;
	vme_swap_t swap;
	bus_dma_segment_t *segs;
	int nsegs;
	int *rsegs;
	int flags;
{
	return (EINVAL);
}

/* ARGSUSED */
void
_vmetwo_dmamem_free(vsc, segs, nsegs)
	void *vsc;
	bus_dma_segment_t *segs;
	int nsegs;
{
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
vmetwo_intr_establish(sc, lvl, vec, hand, arg)
	struct vmetwo_softc *sc;
	int lvl, vec;
	int (*hand) __P((void *));
	void *arg;
{
	u_int32_t reg;
	int bitoff, doenable;
	int iloffset, ilshift;
	int s;

#ifdef DEBUG
	if (vec < 0 || vec > 0xff) {
		printf("%s: Illegal vector offset: 0x%x\n",
		    sc->sc_dev.dv_xname, vec);
		panic("vmetwo_intr_establish");
	}
	if (lvl < 1 || lvl > 7) {
		printf("%s: Illegal interrupt level: %d\n",
		    sc->sc_dev.dv_xname, lvl);
		panic("vmetwo_intr_establish");
	}
#endif

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
		doenable = 1;
	} else {
		/*
		 * Interrupts originating from the VMEbus are
		 * controlled by an offset of 0x00 - 0x07
		 */
		bitoff = lvl - 1;
		doenable = (sc->sc_irqref[lvl] == 0);
		sc->sc_irqref[lvl]++;
	}

	/* Hook the interrupt */
	isrlink_vectored(hand, arg, lvl, vec);

	/*
	 * Do we need to tell the VMEChip2 to let the interrupt through?
	 * (This is always true for locally-generated interrupts, but only
	 * needs doing once for each VMEbus interrupt level which is hooked)
	 */
	if (doenable) {
		iloffset = VME2_ILOFFSET_FROM_VECTOR(bitoff) +
		    VME2LCSR_INTERRUPT_LEVEL_BASE;
		ilshift = VME2_ILSHIFT_FROM_VECTOR(bitoff);

		/* Program the specified interrupt to signal at 'lvl' */
		reg = vme2_lcsr_read(sc, iloffset);
		reg &= ~(VME2_INTERRUPT_LEVEL_MASK << ilshift);
		reg |= (lvl << ilshift);
		vme2_lcsr_write(sc, iloffset, reg);

		/* Clear it */
		vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_CLEAR,
		    VME2_LOCAL_INTERRUPT(bitoff));

		/* Enable it. */
		reg = vme2_lcsr_read(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE);
		reg |= VME2_LOCAL_INTERRUPT(bitoff);
		vme2_lcsr_write(sc, VME2LCSR_LOCAL_INTERRUPT_ENABLE, reg);
	}
	splx(s);
}

void
vmetwo_intr_disestablish(sc, lvl, vec)
	struct vmetwo_softc *sc;
	int lvl, vec;
{
	u_int32_t reg;
	int iloffset, ilshift;
	int bitoff, doclear;
	int s;

#ifdef DEBUG
	if (vec < 0 || vec > 0xff) {
		printf("%s: Illegal vector offset: 0x%x\n",
		    sc->sc_dev.dv_xname, vec);
		panic("vmetwo_intr_disestablish");
	}
	if (lvl < 1 || lvl > 7) {
		printf("%s: Illegal interrupt level: %d\n",
		    sc->sc_dev.dv_xname, lvl);
		panic("vmetwo_intr_disestablish");
	}
#endif

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
		doclear = 1;
		vme_two_handlers[vec - VME2_VECTOR_LOCAL_MIN].isr_hand = NULL;
	} else {
		/*
		 * Interrupts originating from the VMEbus are
		 * controlled by an offset of 0x00 - 0x07
		 */
		bitoff = lvl - 1;
#ifdef DEBUG
		if (sc->sc_irqref[lvl] == 0) {
			printf("%s: VMEirq#%d: Reference count already zero!\n",
			    sc->sc_dev.dv_xname, lvl);
			panic("vmetwo_intr_disestablish");
		}
#endif
		doclear = (sc->sc_irqref[lvl]-- == 0);
	}

	/*
	 * Do we need to tell the VMEChip2 to block the interrupt?
	 * (This is always true for locally-generated interrupts, but only
	 * needs doing once when the last VMEbus handler for any given level
	 * has been unhooked.)
	 */
	if (doclear) {
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
	}
	/* Un-hook it */
	isrunlink_vectored(vec);

	splx(s);
}

#ifdef DIAGNOSTIC
static const char *
_vme2_mod_string(addr, len, am, ds)
	vme_addr_t addr;
	vme_size_t len;
	vme_am_t am;
	vme_datasize_t ds;
{
	static const char *mode[] = {"BLT64)", "DATA)", "PROG)", "BLT32)"};
	static const char *dsiz[] = {"(", "(D8,", "(D16,", "(D16-D8,",
	"(D32,", "(D32,D8,", "(D32-D16,", "(D32-D8,"};
	static char mstring[40];
	char *fmt;

	switch (am & VME_AM_ADRSIZEMASK) {
	case VME_AM_A32:
		fmt = "A32:%08x-%08x ";
		break;

	case VME_AM_A24:
		fmt = "A24:%06x-%06x ";
		break;

	case VME_AM_A16:
		fmt = "A16:%04x-%04x ";
		break;

	case VME_AM_USERDEF:
		fmt = "USR:%08x-%08x ";
		break;
	}

	sprintf(mstring, fmt, addr, addr + len - 1);
	strcat(mstring, dsiz[ds & 0x7]);
	strcat(mstring, ((am & VME_AM_PRIVMASK) == VME_AM_USER) ?
	    "USER," : "SUPER,");
	strcat(mstring, mode[am & VME_AM_MODEMASK]);

	return (mstring);
}
#endif
