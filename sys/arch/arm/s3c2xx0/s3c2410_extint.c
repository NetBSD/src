/* $NetBSD: s3c2410_extint.c,v 1.7.44.1 2008/01/01 15:39:39 chris Exp $ */

/*
 * Copyright (c) 2003  Genetec corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Genetec corporation may not be used to endorse
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

/*
 * device driver to handle cascaded external interrupts of S3C2410.
 *
 * EXTINT [8..23] are cascaded to IRQ #5
 * EXTINT [4..7] are cascaded to IRQ #4
 * EXTINT [0..3] are not cascaded and connected directly as IRQ #[0..3].
 * EXTINT [0..3] are handled by main interrupt handler in s3c2410_intr.c.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: s3c2410_extint.c,v 1.7.44.1 2008/01/01 15:39:39 chris Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <arm/cpufunc.h>

#include <arm/s3c2xx0/s3c2410reg.h>
#include <arm/s3c2xx0/s3c2410var.h>

#include "locators.h"
#include "opt_s3c2410.h"	/* for S3C2410_EXTINT_MAX */

#ifndef	S3C2410_EXTINT_MAX
#define S3C2410_EXTINT_MAX  23
#endif

#define	EXTINT_CASCADE_MIN	4

#if S3C2410_EXTINT_MAX < EXTINT_CASCADE_MIN
#error "Don't enable ssextio if you don't use extint[4..23]."
#endif

#define	N_EXTINT	(S3C2410_EXTINT_MAX - EXTINT_CASCADE_MIN +1)

struct ssextio_softc {
	struct device	sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	uint32_t   sc_pending;
	uint32_t   sc_mask;

	struct extint_handler {
		int (* func)(void *);
		void *arg;
		void *sh;		/* softintr handler */
		int level;		/* IPL */
	} sc_handler[N_EXTINT];

};

static struct	ssextio_softc *ssextio_softc = NULL;

/* cookies */
#define	EXTINT_4_7	1
#define	EXTINT_8_23	2

/* prototypes */
static int	ssextio_match(struct device *, struct cfdata *, void *);
static void	ssextio_attach(struct device *, struct device *, void *);
static int 	ssextio_search(struct device *, struct cfdata *,
			       const int *, void *);
static int	ssextio_print(void *, const char *);

static int	ssextio_cascaded_intr(void *);
static void	ssextio_softintr(void *);

static inline void
update_hw_mask(void)
{
	bus_space_write_4(ssextio_softc->sc_iot, ssextio_softc->sc_ioh,
	    GPIO_EINTMASK, ssextio_softc->sc_mask | ssextio_softc->sc_pending);
}


/* attach structures */
CFATTACH_DECL(ssextio, sizeof(struct ssextio_softc), ssextio_match, ssextio_attach,
    NULL, NULL);

static int
ssextio_print(void *aux, const char *name)
{
	struct s3c2xx0_attach_args *sa = (struct s3c2xx0_attach_args*)aux;

	if (sa->sa_addr != SSEXTIOCF_ADDR_DEFAULT)
                aprint_normal(" addr 0x%lx", sa->sa_addr);
        if (sa->sa_intr > 0)
                aprint_normal(" intr %d", sa->sa_intr);
        return (UNCONF);
}

int
ssextio_match(struct device *parent, struct cfdata *match, void *aux)
{
#if S3C2410_EXTINT_MAX < 4
	/* better not configure this driver */
	return 0;
#else
	if (ssextio_softc != NULL)
		return 0;

	return 1;
#endif
}

void
ssextio_attach(struct device *parent, struct device *self, void *aux)
{
	struct ssextio_softc *sc = (struct ssextio_softc*)self;
	struct s3c24x0_softc *cpuc = (struct s3c24x0_softc *)parent;

	aprint_normal("\n");

	ssextio_softc = sc;

	sc->sc_iot = cpuc->sc_sx.sc_iot;
	sc->sc_ioh = cpuc->sc_sx.sc_gpio_ioh;

	sc->sc_pending = 0;
	sc->sc_mask = ~0;

	s3c24x0_intr_establish(S3C2410_INT_4_7, IPL_HIGH, IST_NONE,
	    ssextio_cascaded_intr, (void *)EXTINT_4_7);
#if S3C2410_EXTINT_MAX >= 8
	s3c24x0_intr_establish(S3C2410_INT_8_23, IPL_HIGH, IST_NONE,
	    ssextio_cascaded_intr, (void *)EXTINT_8_23);
#endif
	/*
	 *  Attach each devices
	 */
	config_search_ia(ssextio_search, self, "ssextio", NULL);
}

static int
ssextio_search(struct device *parent, struct cfdata *cf,
	       const int *ldesc, void *aux)
{
	struct ssextio_softc *sc = (struct ssextio_softc *)parent;
	struct s3c24x0_softc *cpuc =(struct s3c24x0_softc *) device_parent(&sc->sc_dev);
	struct s3c2xx0_attach_args sa;

	sa.sa_sc = sc;
        sa.sa_iot = sc->sc_iot;
        sa.sa_addr = cf->cf_loc[SSEXTIOCF_ADDR];
        sa.sa_size = cf->cf_loc[SSEXTIOCF_SIZE];
        sa.sa_intr = cf->cf_loc[SSEXTIOCF_INTR];
	sa.sa_dmat = cpuc->sc_sx.sc_dmat;

        if (config_match(parent, cf, &sa))
                config_attach(parent, cf, &sa, ssextio_print);

        return 0;
}

void *
s3c2410_extint_establish(int extint, int ipl, int type,
    int (*func)(void *), void *arg)
{
	int save;
	int idx;
	int soft_level;

	if (extint < 0 || N_EXTINT <= extint)
		panic("Bad interrupt no for extio");

	if (extint < EXTINT_CASCADE_MIN) {
		/*
		 * EXINT[0..3] are not cascaded. they are handled by
		 * the main interrupt controller.
		 */
		return s3c24x0_intr_establish(extint, ipl, type, func, arg);
	}

#ifdef __GENERIC_SOFT_INTERRUPTS_ALL_LEVELS
	soft_level = ipl;
#else
	if (ipl >= IPL_SOFTSERIAL)
		soft_level = IPL_SOFTSERIAL;
	else if (ipl >= IPL_SOFTNET)
		soft_level = IPL_SOFTNET;
	else
		soft_level = IPL_SOFT;
#endif

	idx = extint - EXTINT_CASCADE_MIN;

	save = disable_interrupts(I32_bit);

	ssextio_softc->sc_handler[idx].func = func;
	ssextio_softc->sc_handler[idx].arg = arg;
	ssextio_softc->sc_handler[idx].level = ipl;

	ssextio_softc->sc_handler[idx].sh = softintr_establish(soft_level,
	    ssextio_softintr, &ssextio_softc->sc_handler[idx]);

	s3c2410_setup_extint(extint, type);

	bus_space_write_4(ssextio_softc->sc_iot, ssextio_softc->sc_ioh,
			  GPIO_EINTPEND, 1U << extint);
	ssextio_softc->sc_mask &= ~(1U << extint);
	update_hw_mask();

	restore_interrupts(save);
	return &ssextio_softc->sc_handler[idx];
}


static int
ssextio_cascaded_intr(void *cookie)
{
	uint32_t pending_mask, pending;
	int int_min;
	bus_space_tag_t iot = ssextio_softc->sc_iot;
	bus_space_handle_t ioh = ssextio_softc->sc_ioh;
	int save, i;

	switch((int)cookie) {
	case EXTINT_4_7:
		pending_mask = 0x000000f0;
		int_min = 4;
		break;

	case EXTINT_8_23:
		pending_mask = 0x00ffff00;
		int_min = 8;
		break;

	default:
		panic("Bad cookie for %s", __func__);
	}


	save = disable_interrupts(I32_bit);;
	pending = pending_mask & bus_space_read_4(iot, ioh, GPIO_EINTPEND);
	pending &= ~ssextio_softc->sc_mask;
	ssextio_softc->sc_pending |= pending;
	/* disable the extint until the handler is called. */
	update_hw_mask();
	restore_interrupts(save);

	for (i=int_min; pending; ++i) {
		if (pending & (1<<i)) {
			assert(ssextio_softc->sc_handler[i-EXTINT_CASCADE_MIN].sh != NULL);

			softintr_schedule(
			    ssextio_softc->sc_handler[i-EXTINT_CASCADE_MIN].sh);
			pending &= ~ (1<<i);
		}
	}

	return 1;
}

static void
ssextio_softintr(void *cookie)
{
	struct extint_handler *h = cookie;
	int extint = EXTINT_CASCADE_MIN + h - ssextio_softc->sc_handler;
	int s, save;

	save = disable_interrupts(I32_bit);
	/* clear hardware pending bits */
	bus_space_write_4(ssextio_softc->sc_iot, ssextio_softc->sc_ioh,
	    GPIO_EINTPEND, 1<<extint);
	ssextio_softc->sc_pending &= ~(1<<extint);
	update_hw_mask();
	restore_interrupts(save);

	s = _splraise(h->level);
	h->func(h->arg);
	splx(s);
}
