/*
 * Copyright (c) 2002, 2003, 2005  Genetec corp.  All rights reserved.
 *
 * PCMCIA/CF support for TWINTAIL (G4255EB)
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <uvm/uvm.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pcmcia/pcmciareg.h>
#include <dev/pcmcia/pcmciavar.h>
#include <dev/pcmcia/pcmciachip.h>

#include <arch/arm/xscale/pxa2x0var.h>
#include <arch/arm/xscale/pxa2x0reg.h>
#include <arch/arm/sa11x0/sa11xx_pcicvar.h>
#include <arch/evbarm/g42xxeb/g42xxeb_reg.h>
#include <arch/evbarm/g42xxeb/g42xxeb_var.h>
#include <arch/evbarm/g42xxeb/gb225reg.h>
#include <arch/evbarm/g42xxeb/gb225var.h>


//#define DONT_USE_CARD_DETECT_INTR

#define PCMCIA_INT	G42XXEB_INT_EXT1
#define CF_INT     	G42XXEB_INT_EXT0

#ifdef DEBUG
#define DPRINTF(arg)	printf arg
#else
#define DPRINTF(arg)
#endif

struct opcic_softc;

struct opcic_socket {
	struct sapcic_socket ss;	/* inherit socket for sa11x0 pcic */

#if 0
	int voltage;			/* card power voltage selected by
					   upper layer */
#endif
};

struct opcic_softc {
	struct sapcic_softc sc_pc;	/* inherit SA11xx pcic */

	struct opcic_socket sc_socket[2];
	int sc_cards;
	bus_space_handle_t sc_memctl_ioh;
};

static int  	opcic_match(struct device *, struct cfdata *, void *);
static void  	opcic_attach(struct device *, struct device *, void *);
static int 	opcic_print(void *, const char *);

static	int	opcic_read(struct sapcic_socket *, int);
static	void	opcic_write(struct sapcic_socket *, int, int);
static	void	opcic_set_power(struct sapcic_socket *, int);
static	void	opcic_clear_intr(int);
static	void	*opcic_intr_establish(struct sapcic_socket *, int,
				       int (*)(void *), void *);
static	void	opcic_intr_disestablish(struct sapcic_socket *, void *);
#ifndef DONT_USE_CARD_DETECT_INTR
static	int	opcic_card_detect(void *, int);
#endif

CFATTACH_DECL(opcic, sizeof(struct opcic_softc),
    opcic_match, opcic_attach, NULL, NULL);

static struct sapcic_tag opcic_tag = {
	opcic_read,
	opcic_write,
	opcic_set_power,
	opcic_clear_intr,
	opcic_intr_establish,
	opcic_intr_disestablish,
};


#define HAVE_CARD(r)	(((r)&CARDDET_DET)==0)

static inline uint8_t
opcic_read_card_status(struct opcic_socket *so)
{
	struct opcic_softc *sc = (struct opcic_softc *)(so->ss.sc);
	struct opio_softc *osc = 
	    (struct opio_softc *)(sc->sc_pc.sc_dev.dv_parent);

	return bus_space_read_1(osc->sc_iot, osc->sc_ioh,
	    GB225_CFDET + 2 * so->ss.socket);

}

static int
opcic_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return	1;
}

static void
opcic_attach(struct device *parent, struct device *self, void *aux)
{
	int i;
	struct pcmciabus_attach_args paa;
	struct opcic_softc *sc = (struct opcic_softc *)self;
	struct opio_softc *psc = (struct opio_softc *)parent;
	struct obio_softc *bsc = (struct obio_softc *)parent->dv_parent;
	bus_space_handle_t memctl_ioh = bsc->sc_memctl_ioh;
	bus_space_tag_t iot =  psc->sc_iot;

	printf("\n");

	/* tell PXA2X0 that we have two sockets */
#if 0
	bus_space_write_4(iot, memctl_ioh, MEMCTL_MECR, MECR_NOS);
#else
	bus_space_write_4(iot, memctl_ioh, MEMCTL_MECR, MECR_CIT|MECR_NOS);
#endif
	sc->sc_pc.sc_iot = psc->sc_iot;
	sc->sc_memctl_ioh = memctl_ioh;

	sc->sc_cards = 0;

	for(i = 0; i < 2; i++) {
		sc->sc_socket[i].ss.sc = &sc->sc_pc;
		sc->sc_socket[i].ss.socket = i;
		sc->sc_socket[i].ss.pcictag_cookie = NULL;
		sc->sc_socket[i].ss.pcictag = &opcic_tag;
		sc->sc_socket[i].ss.event_thread = NULL;
		sc->sc_socket[i].ss.event = 0;
		sc->sc_socket[i].ss.laststatus = CARDDET_NOCARD;
		sc->sc_socket[i].ss.shutdown = 0;

		sc->sc_socket[i].ss.power_capability = 
		    (SAPCIC_POWER_5V|SAPCIC_POWER_3V);

		bus_space_write_4(iot, memctl_ioh, MEMCTL_MCIO(i), 
		    MC_TIMING_VAL(1,1,1));
#if 0
		bus_space_write_4(iot, memctl_ioh, MEMCTL_MCATT(i), 
		    MC_TIMING_VAL(31,31,31));
#endif

		paa.paa_busname = "pcmcia";
		paa.pct = (pcmcia_chipset_tag_t)&sa11x0_pcmcia_functions;
		paa.pch = (pcmcia_chipset_handle_t)&sc->sc_socket[i].ss;
		paa.iobase = 0;
		paa.iosize = 0x4000000;

		sc->sc_socket[i].ss.pcmcia =
		    (struct device *)config_found_ia(&sc->sc_pc.sc_dev,
		    "pcmciabus", &paa, opcic_print);

#ifndef DONT_USE_CARD_DETECT_INTR
		/* interrupt for card insertion/removal */
		opio_intr_establish(psc,
		    i==0 ? OPIO_INTR_CF_INSERT : OPIO_INTR_PCMCIA_INSERT,
		    IPL_BIO, opcic_card_detect, &sc->sc_socket[i]);
#else
		bus_space_write_4(iot, ioh, MEMCTL_MECR, MECR_NOS | MECR_CIT);

#endif

		/* schedule kthread creation */
		kthread_create(sapcic_kthread_create, &sc->sc_socket[i].ss);
	}

}

static int
opcic_print(void *aux, const char *name)
{
	return (UNCONF);
}

#ifndef DONT_USE_CARD_DETECT_INTR
static int
opcic_card_detect(void *arg, int val)
{
	struct opcic_socket *socket = arg;
	struct opcic_softc *sc = (struct opcic_softc *)socket->ss.sc;
	bus_space_tag_t iot = sc->sc_pc.sc_iot;
	bus_space_handle_t memctl_ioh = sc->sc_memctl_ioh;
	int sock_no = socket->ss.socket;
	int last, s;

	s = splbio();
	last = sc->sc_cards;
	if (HAVE_CARD(val)) {
		sc->sc_cards |= 1<<sock_no;
		/* if it is the first card, turn on expansion memory
		 * control. */
		if (last == 0)
			bus_space_write_4(iot, memctl_ioh, MEMCTL_MECR, 
			    MECR_NOS | MECR_CIT);
	}
	else {
		sc->sc_cards &= ~(1<<sock_no);
		/* if we loast all cards, turn off expansion memory
		 * control. */
#if 0
		if (sc->sc_cards == 0)
			bus_space_write_4(iot, memctl_ioh, 
			    MEMCTL_MECR, MECR_NOS);
#endif
	}
	splx(s);

	DPRINTF(("%s: card %d %s\n", sc->sc_pc.sc_dev.dv_xname, sock_no,
	    HAVE_CARD(val) ? "inserted" : "removed"));

	sapcic_intr(arg);

	return 1;
}
#endif /* DONT_USE_CARD_DETECT_INTR */

static int
opcic_read(struct sapcic_socket *__so, int which)
{
	struct opcic_socket *so = (struct opcic_socket *)__so;
	uint8_t reg;

	reg = opcic_read_card_status(so);

	switch (which) {
	case SAPCIC_STATUS_CARD:
		return HAVE_CARD(reg) ? 
		    SAPCIC_CARD_VALID : SAPCIC_CARD_INVALID;

	case SAPCIC_STATUS_VS1:
		return (reg & CARDDET_NVS1) == 0;

	case SAPCIC_STATUS_VS2:
		return (reg & CARDDET_NVS2) == 0;

	case SAPCIC_STATUS_READY:
		return 1;

	default:
		panic("%s: bogus register", __FUNCTION__);
	}
}

static void
opcic_write(struct sapcic_socket *__so, int which, int arg)
{
	struct opcic_socket *so = (struct opcic_socket *)__so;
	struct opcic_softc *sc = (struct opcic_softc *)so->ss.sc;
	struct opio_softc *psc = 
	    (struct opio_softc *)sc->sc_pc.sc_dev.dv_parent;
	struct obio_softc *bsc = 
	    (struct obio_softc *)psc->sc_dev.dv_parent;

	switch (which) {
	case SAPCIC_CONTROL_RESET:
		obio_peripheral_reset(bsc, so->ss.socket, arg);
		delay(500*1000);
		break;

	case SAPCIC_CONTROL_LINEENABLE:
		break;

	case SAPCIC_CONTROL_WAITENABLE:
		break;

	case SAPCIC_CONTROL_POWERSELECT:
#if 0
		so->voltage = arg;
#endif
		break;

	default:
		panic("%s: bogus register", __FUNCTION__);
	}
}

static void
opcic_set_power(struct sapcic_socket *__so, int arg)
{
	struct opcic_socket *so = (struct opcic_socket *)__so;
	struct opcic_softc *sc = (struct opcic_softc *)so->ss.sc;
	struct opio_softc *psc = 
	    (struct opio_softc *)sc->sc_pc.sc_dev.dv_parent;
 	int shift, save;
	volatile uint8_t *p;

	if( arg < 0 || SAPCIC_POWER_5V < arg )
		panic("sacpcic_set_power: bogus arg\n");
			
	DPRINTF(("card %d: DET=%x\n",
	    so->ss.socket,
	    bus_space_read_1(psc->sc_iot, psc->sc_ioh,
		GB225_CFDET + 2*so->ss.socket)));

	p = (volatile uint8_t *)bus_space_vaddr(psc->sc_iot, psc->sc_ioh)
	    + GB225_CARDPOW;

	shift = 4 * !so->ss.socket;

	save = disable_interrupts(I32_bit);
	*p = (*p & ~(0x0f << shift)) | ((CARDPOW_VPPVCC | (arg<<2)) << shift);
	restore_interrupts(save);

	DPRINTF(("card %d power: %x\n", so->ss.socket, *p));
}

static void
opcic_clear_intr(int arg)
{
}

static void *
opcic_intr_establish(struct sapcic_socket *so, int level, 
    int (* ih_fun)(void *), void *ih_arg)
{
	struct opcic_softc *sc = (struct opcic_softc *)so->sc;
	struct opio_softc *psc = 
	    (struct opio_softc *)sc->sc_pc.sc_dev.dv_parent;
	struct obio_softc *bsc = 
	    (struct obio_softc *)psc->sc_dev.dv_parent;
	int irq;

	DPRINTF(("opcic_intr_establish %d\n", so->socket));

	irq = so->socket ? PCMCIA_INT : CF_INT;
	return obio_intr_establish(bsc, irq, level, IST_EDGE_FALLING,
	    ih_fun, ih_arg);
}

static void
opcic_intr_disestablish(struct sapcic_socket *so, void *ih)
{
	struct opcic_softc *sc = (struct opcic_softc *)so->sc;
	struct opio_softc *psc = 
	    (struct opio_softc *)sc->sc_pc.sc_dev.dv_parent;
	struct obio_softc *bsc = 
	    (struct obio_softc *)psc->sc_dev.dv_parent;
	int (* func)(void *) = ((struct obio_handler *)ih)->func;

	int irq = so->socket ? PCMCIA_INT : CF_INT;

	obio_intr_disestablish(bsc, irq, func);
}
