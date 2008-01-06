/*	$NetBSD: obio.c,v 1.6 2008/01/06 01:37:57 matt Exp $ */

/*
 * Copyright (c) 2002, 2003, 2005  Genetec corp.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corp.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corp. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORP. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORP.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/reboot.h>

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/mainbus/mainbus.h>
#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <evbarm/g42xxeb/g42xxeb_reg.h>
#include <evbarm/g42xxeb/g42xxeb_var.h>

#include "locators.h"

/* prototypes */
static int	obio_match(struct device *, struct cfdata *, void *);
static void	obio_attach(struct device *, struct device *, void *);
static int 	obio_search(struct device *, struct cfdata *,
			    const int *, void *);
static int	obio_print(void *, const char *);

/* attach structures */
CFATTACH_DECL(obio, sizeof(struct obio_softc), obio_match, obio_attach,
    NULL, NULL);

static int
obio_spurious(void *arg)
{
	int irqno = (int)arg;

	printf("Spurious interrupt %d on On-board peripheral", irqno);
	return 1;
}


/*
 * interrupt handler for GPIO0 (on-board peripherals)
 *
 * On G4250ebx, 10 interrupts are ORed through on-board logic,
 * and routed to GPIO0 of PXA250 processor.
 */
static int	
obio_intr(void *arg)
{
	int irqno, pending;
	struct obio_softc *sc = (struct obio_softc *)arg;
	int n=0;

#define get_pending(sc) \
	(bus_space_read_2( sc->sc_iot, sc->sc_obioreg_ioh, G42XXEB_INTSTS1) \
	& ~(sc->sc_intr_pending|sc->sc_intr_mask))

#ifdef DEBUG
	printf("obio_intr: pend=%x, mask=%x, pend=%x, mask=%x\n",
	    bus_space_read_2(sc->sc_iot, sc->sc_obioreg_ioh, G42XXEB_INTSTS1),
	    bus_space_read_2(sc->sc_iot, sc->sc_obioreg_ioh, G42XXEB_INTMASK),
	    sc->sc_intr_pending,
	    sc->sc_intr_mask);
#endif

	for (pending = get_pending(sc);
	     (irqno = find_first_bit(pending)) >= 0;
	     pending = get_pending(sc)) {

		/* reset pending bit */
		bus_space_write_2(sc->sc_iot, sc->sc_obioreg_ioh,
		    G42XXEB_INTSTS1, ~(1<<irqno));

#if 0
		if (sc->sc_handler[irqno].level > saved_spl_level) {
			int spl_save = _splraise(sc->sc_handler[irqno].level);
			(* sc->sc_handler[irqno].func)(
				sc->sc_handler[irqno].arg);
			splx(spl_save);
		}
		else 
#endif
		{
			int psw = disable_interrupts(I32_bit); /* XXX */

			/* mask this interrupt until software
			   interrupt is handled. */
			sc->sc_intr_pending |= (1U<<irqno);
			obio_update_intrmask(sc);

			restore_interrupts(psw);
			++n;
		}
#ifdef DIAGNOSTIC
		if (n > 1000)
			panic("obio_intr: stayed too long");
#endif
	}

	if (n > 0) {
		/* handle it later */
		softint_schedule(sc->sc_si);
	}

	/* GPIO interrupt is edge triggered.  make a pulse
	   to let Cotulla notice when other interrupts are
	   still pending */
	bus_space_write_2(sc->sc_iot, sc->sc_obioreg_ioh, 
	    G42XXEB_INTMASK, 0xffff);
	obio_update_intrmask(sc);

	return 1;
}

static void
obio_softint(void *arg)
{
	struct obio_softc *sc = (struct obio_softc *)arg;
	int irqno;
	int spl_save = current_spl_level;
	int psw;

	psw = disable_interrupts(I32_bit);
	while ((irqno = find_first_bit(sc->sc_intr_pending)) >= 0) {
		sc->sc_intr_pending &= ~(1U<<irqno);

		restore_interrupts(psw);

		_splraise(sc->sc_handler[irqno].level);
		(* sc->sc_handler[irqno].func)(
			sc->sc_handler[irqno].arg);
		splx(spl_save);
		
		psw = disable_interrupts(I32_bit);
	}

	/* assert(sc->sc_intr_pending==0) */

	bus_space_write_2(sc->sc_iot, sc->sc_obioreg_ioh, 
	    G42XXEB_INTMASK, 0xffff);
	obio_update_intrmask(sc);

	restore_interrupts(psw);
}

/*
 * int obio_print(void *aux, const char *name)
 * print configuration info for children
 */

static int
obio_print(void *aux, const char *name)
{
	struct obio_attach_args *oba = (struct obio_attach_args*)aux;

	if (oba->oba_addr != OBIOCF_ADDR_DEFAULT)
                printf(" addr 0x%lx", oba->oba_addr);
        if (oba->oba_intr > 0)
                printf(" intr %d", oba->oba_intr);
        return (UNCONF);
}

int
obio_match(struct device *parent, struct cfdata *match, void *aux)
{
	return 1;
}

void
obio_attach(struct device *parent, struct device *self, void *aux)
{
	struct obio_softc *sc = (struct obio_softc*)self;
	struct sa11x0_attach_args *sa = (struct sa11x0_attach_args *)aux;
	bus_space_tag_t iot = sa->sa_iot;
	int i;
	uint16_t reg;

	/* tweak memory access timing for CS3. 
	   the value set by redboot is too slow */
	if (bus_space_map(iot, PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0, 
		&sc->sc_memctl_ioh))
		goto fail;
	bus_space_write_4(iot, sc->sc_memctl_ioh, MEMCTL_MSC1,
	    (0xffff & bus_space_read_4(iot, sc->sc_memctl_ioh, MEMCTL_MSC1))
	    | (0x6888 << 16));

	/* Map on-board FPGA registers */
	sc->sc_iot = iot;
	if (bus_space_map(iot, G42XXEB_PLDREG_BASE, G42XXEB_PLDREG_SIZE,
	    0, &(sc->sc_obioreg_ioh)))
		goto fail;
		
	/*
	 *  Mask all interrupts.
	 *  They are later unmasked at each device's attach routine.
	 */
	sc->sc_intr_mask = 0xffff;
	bus_space_write_2(iot, sc->sc_obioreg_ioh, G42XXEB_INTMASK,
	    sc->sc_intr_mask );

#if 0
	sc->sc_intr = 8;		/* GPIO0 */
#endif
	sc->sc_intr_pending = 0;

	for (i=0; i < G42XXEB_N_INTS; ++i) {
		sc->sc_handler[i].func = obio_spurious;
		sc->sc_handler[i].arg = (void *)i;
	}

	obio_peripheral_reset(sc, 1, 0);

	/*
	 * establish interrupt handler.
	 * level is very high to allow high priority sub-interrupts.
	 */
	sc->sc_ipl = IPL_AUDIO;
	sc->sc_ih = pxa2x0_gpio_intr_establish(0, IST_EDGE_FALLING, sc->sc_ipl,
	    obio_intr, sc);
	sc->sc_si = softint_establish(SOFTINT_NET, obio_softint, sc);

	reg = bus_space_read_2(iot, sc->sc_obioreg_ioh, G42XXEB_PLDVER);
	aprint_normal(": board %d version %x\n", reg>>8, reg & 0xff);

	/*
	 *  Attach each devices
	 */
	config_search_ia(obio_search, self, "obio", NULL);
	return;

 fail:
	printf( "%s: can't map FPGA registers\n", self->dv_xname );
}

int
obio_search(struct device *parent, struct cfdata *cf,
	    const int *ldesc, void *aux)
{
	struct obio_softc *sc = (struct obio_softc *)parent;
	struct obio_attach_args oba;

	oba.oba_sc = sc;
        oba.oba_iot = sc->sc_iot;
        oba.oba_addr = cf->cf_loc[OBIOCF_ADDR];
        oba.oba_intr = cf->cf_loc[OBIOCF_INTR];

        if (config_match(parent, cf, &oba) > 0)
                config_attach(parent, cf, &oba, obio_print);

        return 0;
}

void *
obio_intr_establish(struct obio_softc *sc, int irq, int ipl, 
    int type, int (*func)(void *), void *arg)
{
	int save;
	int regidx, sft;
	uint16_t reg;
	static const uint8_t ist_code[] = {
		0,
		G42XXEB_INT_EDGE_FALLING, /* pulse */
		G42XXEB_INT_EDGE_FALLING, /* IST_EDGE */
		G42XXEB_INT_LEVEL_LOW,	   /* IST_LEVEL */
		G42XXEB_INT_LEVEL_HIGH,   /* IST_LEVEL_HIGH */
		G42XXEB_INT_EDGE_RISING,  /* IST_EDGE_RISING */
		G42XXEB_INT_EDGE_BOTH,	   /* IST_EDGE_BOTH */
	};

	if (irq < 0 || G42XXEB_N_INTS <= irq)
		panic("Bad irq no. for obio (%d)", irq);

	if (type < 0 || IST_EDGE_BOTH < type)
		panic("Bad interrupt type for obio (%d)", type);

	regidx = G42XXEB_INTCNTL;
	sft = 3 * irq;
	if (irq >= 5) {
		regidx = G42XXEB_INTCNTH;
		sft -= 3*5;
	}

	save = disable_interrupts(I32_bit);

	sc->sc_handler[irq].func = func;
	sc->sc_handler[irq].arg = arg;
	sc->sc_handler[irq].level = ipl;

	/* set interrupt type */
	reg = bus_space_read_2(sc->sc_iot, sc->sc_obioreg_ioh, regidx);
	bus_space_write_2(sc->sc_iot, sc->sc_obioreg_ioh, regidx,
	    (reg & ~(7<<sft)) | (ist_code[type] << sft));

#ifdef DEBUG
	printf("INTCTL=%x,%x\n",
	    bus_space_read_2(sc->sc_iot, sc->sc_obioreg_ioh, G42XXEB_INTCNTL),
	    bus_space_read_2(sc->sc_iot, sc->sc_obioreg_ioh, G42XXEB_INTCNTH));
#endif

	sc->sc_intr_mask &= ~(1U << irq);
	obio_update_intrmask(sc);

	restore_interrupts(save);

#if 0
	if (ipl > sc->sc_ipl) {
		pxa2x0_update_intr_masks(sc->sc_intr, ipl);
		sc->sc_ipl = ipl;
	}
#endif

	return &sc->sc_handler[irq];
}

void
obio_intr_disestablish(struct obio_softc *sc, int irq, int (* func)(void *))
{
	int error = 0;
	int save;

	save = disable_interrupts(I32_bit);

	if (sc->sc_handler[irq].func != func)
		error = 1;
	else {
		sc->sc_handler[irq].func = obio_spurious;
		sc->sc_handler[irq].level = IPL_NONE;

		sc->sc_intr_pending &= ~(1U << irq);
		sc->sc_intr_mask |= (1U << irq);
		obio_update_intrmask(sc);
	}

	restore_interrupts(save);

	if (error)
		aprint_error("%s: bad intr_disestablish\n", 
		    sc->sc_dev.dv_xname);
}

void
obio_intr_mask(struct obio_softc *sc, struct obio_handler *ih)
{
	int irqno;
	int save;

	irqno = ih - sc->sc_handler;
#ifdef DIAGNOSTIC
	if (ih == NULL || ih->func==NULL || irqno < 0 ||
	    irqno >= G42XXEB_N_INTS)
		panic("Bad arg for obio_intr_mask");
#endif

	save = disable_interrupts(I32_bit);
	sc->sc_intr_mask |= 1U<<irqno;
	obio_update_intrmask(sc);
	restore_interrupts(save);
}

void
obio_intr_unmask(struct obio_softc *sc, struct obio_handler *ih)
{
	int irqno;
	int save;

	irqno = ih - sc->sc_handler;
#ifdef DIAGNOSTIC
	if (ih == NULL || ih->func==NULL || irqno < 0 ||
	    irqno >= G42XXEB_N_INTS)
		panic("Bad arg for obio_intr_unmask");
#endif

	save = disable_interrupts(I32_bit);
	sc->sc_intr_mask &= ~(1U<<irqno);
	obio_update_intrmask(sc);
	restore_interrupts(save);
}

void
obio_peripheral_reset(struct obio_softc *bsc, int no, int onoff)
{
	uint16_t reg;

	reg = bus_space_read_2(bsc->sc_iot, bsc->sc_obioreg_ioh,
	    G42XXEB_RST);
	bus_space_write_2(bsc->sc_iot, bsc->sc_obioreg_ioh, G42XXEB_RST,
	    onoff ?  (reg & ~RST_EXT(no)) : (reg | RST_EXT(no)));
}
	
