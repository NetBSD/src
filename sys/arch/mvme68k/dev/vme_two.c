/*	$NetBSD: vme_two.c,v 1.1.16.1 2000/03/11 20:51:50 scw Exp $ */

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

#include <machine/bus.h>

#include <dev/vme/vmereg.h>
#include <dev/vme/vmevar.h>

#include <mvme68k/mvme68k/isr.h>
#include <mvme68k/dev/vme_tworeg.h>
#include <mvme68k/dev/vme_twovar.h>

int	vmetwo_match  __P((struct device *, struct cfdata *, void *));
void	vmetwo_attach __P((struct device *, struct device *, void *));

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
	void	(*isr_hand) __P((void *));
	void	*isr_arg;
} vme_two_handlers[(VME2_VECTOR_MAX - VME2_VECTOR_MIN) + 1];
#define VMETWO_HANDLERS_SZ	(sizeof(vme_two_handlers) /	\
				 sizeof(struct vme_two_handler))

static int vmetwo_attached = 0;
static int vmetwo_isr_trampoline __P((void *));


int
vmetwo_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	char *ma_name = (char *)aux;

	if ( strcmp(ma_name, vmetwo_cd.cd_name) )
		return 0;

	/* Only one VMEchip2, please. */
	if ( vmetwo_attached )
		return 0;

	if ( machineid != MVME_167 && machineid != MVME_177 )
		return 0;

	return 1;
}

void
vmetwo_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct maibus_attach_args *ma;
	struct vmebus_attach_args vaa;
	struct vmetwo_softc *sc;
	int i;

	ma = (struct mainbus_attach_args *) aux;
	sc = (struct vmetwo_softc *) self;

	sc->sc_dmat = pa->pa_dmat;
	sc->sc_bust = pa->pa_bust;
	sc->sc_vmet = MVME68K_VME_BUS_SPACE;

	bus_space_map(pa->pa_bust, pa->pa_offset + VME2REG_LCSR_OFFSET,
	    VME2REG_LCSR_SIZE, 0, &sc->sc_lcrh);

	bus_space_map(pa->pa_bust, pa->pa_offset + VME2REG_GCSR_OFFSET,
	    VME2REG_GCSR_SIZE, 0, &sc->sc_gcrh);

	/* Clear out the ISR handler array */
	for (i = 0; i < VMETWO_HANDLERS_SZ; i++) {
		vme_two_handlers[i].isr_hand = NULL;
		vme_two_handlers[i].isr_arg = NULL;
	}

	/*
	 * Initialize the chip.
	 * Firstly, disable all VMEChip2 Interrupts
	 */
	vme->vme_io_ctrl1 &= ~VME2_IO_CTRL1_MIEN;
	vme->vme_lbint_enable = 0;

	/* Zap all the IRQ level registers */
	for (i = 0; i < VME2_NUM_IL_REGS; i++)
		vme->vme_irq_levels[i] = 0;

	/* Disable the VMEbus Slave Windows for now */
	vme->vme_slave_ctrl = 0;

#if 0
	/* Disable the VMEbus Master Windows */
	vme->vme_local_2_vme_enab = 0;
	for (i = 0; i < VME2_MASTER_WINDOWS; i++)
		vme->vme_master_win[i] = 0;

	/* Disable VMEbus I/O access */
	vme->vme_local_2_vme_ctrl &= ~VME2_LOC2VME_MASK;
#endif

	/* Disable the tick timers */
	vme->vme_tt1_ctrl &= ~VME2_TICK1_TMR_MASK;
	vme->vme_tt2_ctrl &= ~VME2_TICK2_TMR_MASK;

	/* Set the VMEChip2's vector base register to the required value */
	vme->vme_vector_base &= ~VME2_VECTOR_BASE_MASK;
	vme->vme_vector_base |=  VME2_VECTOR_BASE_REG;

	/* Set the Master Interrupt Enable bit now */
	vme->vme_io_ctrl1 |= VME2_IO_CTRL1_MIEN;

	/* Let the NMI handler deal with level 7 ABORT switch interrupts */
	vmetwo_chip_intr_establish(sc, 7, VME2_VEC_ABORT,
	    (void *)nmihand, NULL);

	printf(": Type 2 VMEchip, scon jumper %s\n",
	    (vme->vme_board_ctrl & VME2_BRD_CTRL_SCON)? "enabled" : "disabled");

	sc->sc_vct.cookie = self;
	sc->sc_vct.vct_probe = vmetwo_probe;
	sc->sc_vct.vct_map = vmetwo_map;
	sc->sc_vct.vct_unmap = vmetwo_unmap;
	sc->sc_vct.vct_int_map = vmetwo_intmap;
	sc->sc_vct.vct_int_establish = vmetwo_intr_establish;
	sc->sc_vct.vct_int_disestablish = vmetwo_intr_disestablish;
	sc->sc_vct.vct_dmamap_create = NULL;
	sc->sc_vct.vct_dmamap_destroy = NULL;
	sc->sc_vct.vct_dmamap_alloc = NULL;
	sc->sc_vct.vct_dmamap_free = NULL;

	vaa.va_vct = &(sc->sc_vct);
	vaa.va_bdt = NULL;
	vaa.va_slaveconfig = NULL;

	/* Attach the MI VMEbus glue. */
	config_found(self, &vaa, 0);
}

int
vmetwo_map(vsc, vmeaddr, len, am, datasize, swap, tag, handle, resc)
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
	struct vmetwo_mapresc_t *r
	vme_addr_t end;
	paddr_t paddr;

	sc = (struct vmetwo_softc *) vsc;
	end = (vmeaddr + len) - 1;
	paddr = 0;

	if ( paddr == 0 ) {
		printf("vme_pcc: can't map %s atype %d addr 0x%lx len 0x%x\n", 
		    vmel_cd.cd_name, atype, start, len);
		return ENOMEM;
	}

	if ( (r = malloc(sizeof(*r), M_DEVBUF, M_NOWAIT)) == NULL )
		return ENOMEM;

	r->size = len;
	if ( (r->vaddr = iomap(paddr, len)) == NULL ) {
		free(r, M_DEVBUF);
		return ENOMEM;
	}

	*resc = (vme_mapresc_t *) r;
	*tag = sc->sc_vmet;
	*handle = (bus_dma_handle_t) r->vaddr;

	return 0;
}

void
vme_pcc_unmap(vsc, resc)
	void *vsc;
	vme_mapresc_t *resc;
{
	struct vmetwo_mapresc_t *r

	sc = (struct vmetwo_softc *) vsc;
	r = (struct vmetwo_mapresc_t *) resc;

	iounmap(r->vaddr, r->size);
	free(r, M_DEVBUF);
}

int
vme_pcc_probe(vsc, vmeaddr, len, am, datasize, callback, arg)
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

	rv = vmetwo_map(vsc, vmeaddr, len, am, datasize, 0, 0,
	    &tag, &handle, &resc);
	if ( rv )
		return rv;

	if ( callback )
		rv = (*callback)(arg, tag, handle);
	else {
		/* Note: datasize is fixed by hardware */
		rv = badaddr((caddr_t) handle, (int) len) ? EIO : 0;
	}

	vmetwo_unmap(vsc, resc);

	return rv;
}

int
vme_pcc_intmap(vsc, level, vector, handlep)
	void *vsc;
	int level, vector;
	vme_intr_handle_t *handlep;
{
	/* This is rather gross */
	*handlep = (void *)(int)((level << 8) | vector);
	return 0;
}





/*
 * Install a handler for the vectored interrupts generated by
 * the VMEchip2 for things like the ABORT switch, VMEbus ACFAIL
 * and SYSFAIL, Memory Parity Error and so on.
 *
 * Note: These are distinct from real VMEbus interrupts where the
 *       vector comes in over the bus in that they have to be
 *       manually cleared.
 */
static void
vmetwo_chip_intr_establish(sc, lvl, vec, hand, arg)
	struct vmetwo_softc *sc;
	int lvl;
	int vec;
	void (*hand) __P((void *));
	void *arg;
{
	u_int8_t ilval;
	int ilreg;
	int ilshift;

	if ( vec < VME2_VECTOR_MIN || vec > VME2_VECTOR_MAX ) {
		printf("vmetwo: Illegal vector offset: 0x%x\n", vec);
		panic("vmetwo_intr_establish");
	}

	if ( lvl < 1 || lvl > 7 ) {
		printf("vmetwo: Illegal interrupt level: %d\n", lvl);
		panic("vmetwo_intr_establish");
	}

#ifdef DEBUG
	if ( (sc->sc_regs->vme_lbint_enable & VME2_LBINT_ENABLE(vec)) != 0 ) {
		printf("vmetwo: Interrupt vector 0x%x already enabled\n", vec);
		panic("vmetwo_intr_establish");
	}
#endif

	ilreg = VME2_ILREG_FROM_VEC(vec);
	ilshift = VME2_ILSHFT_FROM_VEC(vec);

	ilval = (sc->sc_regs->vme_irq_levels[ilreg] >> ilshift) & VME2_ILMASK;

#ifdef DEBUG
	if ( ilval != 0 && ilval != lvl ) {
		printf("vmetwo: Vector 0x%x: Level register non-zero (%d)\n",
			vec, ilval);
		panic("vmetwo_intr_establish");
	}

	if ( vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand != NULL ) {
		printf("vmetwo: Vector 0x%x already claimed\n", vec);
		panic("vmetwo_intr_establish");
	}
#endif

	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand = hand;
	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_arg = arg;

	/* Hook the interrupt */
	isrlink_vectored(vmetwo_isr_trampoline, (void *)vec,
			 lvl, vec + VME2_VECTOR_BASE);

	/* Program the specified interrupt to signal at 'lvl' */
	ilval = sc->sc_regs->vme_irq_levels[ilreg];
	ilval &= ~(VME2_ILMASK << ilshift);
	sc->sc_regs->vme_irq_levels[ilreg] = ilval | (lvl << ilshift);

	/* Clear it */
	sc->sc_regs->vme_lbint_clear |= VME2_LBINT_CLEAR(vec);

	/* Enable it. */
	sc->sc_regs->vme_lbint_enable |= VME2_LBINT_ENABLE(vec);
}

void
vmetwo_chip_intr_disestablish(sc, vec)
	struct vmetwo_softc *sc;
	int vec;
{
	u_int8_t ilval;
	int ilreg;
	int ilshift;

	if ( vec < VME2_VECTOR_MIN || vec > VME2_VECTOR_MAX ) {
		printf("vmetwo: Illegal vector offset: 0x%x\n", vec);
		panic("vmetwo_intr_disestablish");
	}

#ifdef DEBUG
	if ( (sc->sc_regs->vme_lbint_enable & VME2_LBINT_ENABLE(vec)) == 0 ) {
		printf("vmetwo: Interrupt vector 0x%x already disabled\n", vec);
		panic("vmetwo_intr_disestablish");
	}
#endif

	ilreg = VME2_ILREG_FROM_VEC(vec);
	ilshift = VME2_ILSHFT_FROM_VEC(vec);
	ilval = sc->sc_regs->vme_irq_levels[ilreg];

#ifdef DEBUG
	if ( ((ilval >> ilshift) & VME2_ILMASK) == 0 ) {
		printf("vmetwo: Vector 0x%x: Level register is zero (%d)\n",
			vec, ilval);
		panic("vmetwo_intr_establish");
	}

	if ( vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand == NULL ) {
		printf("vmetwo: Vector 0x%x already unclaimed\n", vec);
		panic("vmetwo_intr_disestablish");
	}
#endif

	/* Disable it. */
	sc->sc_regs->vme_lbint_enable &= ~VME2_LBINT_ENABLE(vec);

	/* Set the interrupt's level to zero */
	ilval &= ~(VME2_ILMASK << ilshift);
	sc->sc_regs->vme_irq_levels[ilreg] = ilval;

	/* Clear it */
	sc->sc_regs->vme_lbint_clear |= VME2_LBINT_CLEAR(vec);

	/* Un-hook it */
	isrunlink_vectored(vec + VME2_VECTOR_BASE);

	/* Finally, clear the entry in our handler array */
	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_hand = NULL;
	vme_two_handlers[vec - VME2_VECTOR_MIN].isr_arg = NULL;
}

int
vmetwo_translate_addr(start, len, bustype, atype, addrp)
	u_long start;
	size_t len;
	int bustype, atype;
	u_long *addrp;		/* result */
{
	/*
	 * Not yet implemented.
	 *
	 * However, it might be useful to require locators on a 'vmetwo'
	 * device to specify how we should set up the master/slave
	 * windows. We then attach up to four 'vmechip's with locators
	 * specifying the VMEbus window each 'vmechip' refers to. The
	 * traditional 'vmes' and 'vmel' then attach under those.
	 *
	 * Hmm, maybe 'vmechip' should be renamed to 'vmemaster'...
	 * Then we could have 'vmeslave' too ;-)
	 *
	 * Alternatively we just fix things up so the board's view of
	 * the VMEbus is pretty much the same as the MVME-147. 8-)
	 */
	return (1);
}

void
vmetwo_intrline_enable(ipl)
	int ipl;
{
#ifdef DEBUG
	if ( ipl < 1 || ipl > 7 ) {
		printf("vmetwo: Invalid VMEbus IRQ level: %d\n", ipl);
		panic("vmetwo_intrline_enable");
	}
#endif

	sys_vme_two->vme_lbint_enable |= VME2_LBINT_ENABLE(ipl - 1);
}

void
vmetwo_intrline_disable(ipl)
	int ipl;
{
#ifdef DEBUG
	if ( ipl < 1 || ipl > 7 ) {
		printf("vmetwo: Invalid VMEbus IRQ level: %d\n", ipl);
		panic("vmetwo_intrline_disable");
	}
#endif

	sys_vme_two->vme_lbint_enable &= ~VME2_LBINT_ENABLE(ipl - 1);
}

static int
vmetwo_isr_trampoline(arg)
	void *arg;
{
	struct vme_two_handler *isr;
	int vec = (int)arg;
	int s;

#ifdef DEBUG
	if ( vec < VME2_VECTOR_MIN || vec > VME2_VECTOR_MAX ) {
		printf("vmetwo: ISR trampoline: Bad vector 0x%x\n", vec);
		panic("vmetwo_isr_trampoline");
	}
#endif

	/* No interrupts while we fiddle with the registers, please */
	s = splhigh();

	/* Clear the interrupt source */
	sys_vme_two->vme_lbint_clear |= VME2_LBINT_CLEAR(vec);

	splx(s);

	isr = & vme_two_handlers[vec - VME2_VECTOR_MIN];
	if ( isr->isr_hand )
		(*isr->isr_hand)(isr->isr_arg);
	else
		printf("vmetwo: Spurious interrupt, vector 0x%x\n", vec);

	return 1;
}
