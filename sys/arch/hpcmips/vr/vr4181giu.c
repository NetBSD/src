/* $NetBSD: vr4181giu.c,v 1.2 2003/07/15 02:29:35 lukem Exp $ */

/*-
 * Copyright (c) 1999-2001
 *         Shin Takemura and PocketBSD Project. All rights reserved.
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
 *	This product includes software developed by the PocketBSD project
 *	and its contributors.
 * 4. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: vr4181giu.c,v 1.2 2003/07/15 02:29:35 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <hpcmips/vr/vripif.h>
#include <hpcmips/vr/vr4181giureg.h>

#define MAX_GIU4181INTR	16

struct vr4181giu_intr_entry {
	int	ih_port;
	int	(*ih_fun)(void *);
	void	*ih_arg;
	TAILQ_ENTRY(vr4181giu_intr_entry) ih_link;
};

struct vr4181giu_softc {
	struct device			sc_dev;
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_ioh;
	vrip_chipset_tag_t		sc_vc;
	void 				*sc_ih;
	u_int32_t			sc_intr_mode[MAX_GIU4181INTR];
	TAILQ_HEAD(, vr4181giu_intr_entry)
					sc_intr_head[MAX_GIU4181INTR];
	struct hpcio_chip		sc_iochip;
	struct hpcio_attach_args	sc_haa;
};

static int vr4181giu_match(struct device *, struct cfdata *, void *);
static void vr4181giu_attach(struct device *, struct device *, void *);

static void vr4181giu_callback(struct device *self);
static int vr4181giu_print(void *aux, const char *pnp);
static int vr4181giu_port_read(hpcio_chip_t hc, int port);
static void vr4181giu_port_write(hpcio_chip_t hc, int port, int onoff);
static void vr4181giu_update(hpcio_chip_t hc);
static void vr4181giu_dump(hpcio_chip_t hc);
static hpcio_chip_t vr4181giu_getchip(void* scx, int chipid);
static void *vr4181giu_intr_establish(hpcio_chip_t, int, int,
				      int (*)(void *),void *);
static void vr4181giu_intr_disestablish(hpcio_chip_t hc, void *arg);
static void vr4181giu_intr_clear(hpcio_chip_t hc, void *arg);
static void vr4181giu_register_iochip(hpcio_chip_t hc, hpcio_chip_t iochip);
static int vr4181giu_intr(void *arg);



static struct hpcio_chip vr4181giu_iochip = {
	.hc_portread =		vr4181giu_port_read,
	.hc_portwrite =		vr4181giu_port_write,
	.hc_intr_establish =	vr4181giu_intr_establish,
	.hc_intr_disestablish =	vr4181giu_intr_disestablish,
	.hc_intr_clear =	vr4181giu_intr_clear,
	.hc_register_iochip =	vr4181giu_register_iochip,
	.hc_update =		vr4181giu_update,
	.hc_dump =		vr4181giu_dump,
};

CFATTACH_DECL(vr4181giu, sizeof(struct vr4181giu_softc),
	      vr4181giu_match, vr4181giu_attach, NULL, NULL);

static int
vr4181giu_match(struct device *parent, struct cfdata *match, void *aux)
{
	return (2); /* 1st attach group of vrip */
}

static void
vr4181giu_attach(struct device *parent, struct device *self, void *aux)
{
	struct vr4181giu_softc	*sc = (struct vr4181giu_softc*) self;
	struct vrip_attach_args	*va = aux;
	int			i;

	sc->sc_iot = va->va_iot;
	sc->sc_vc = va->va_vc;

	if (bus_space_map(sc->sc_iot, va->va_addr, va->va_size,
			  0 /* no cache */, &sc->sc_ioh)) {
		printf(": can't map i/o space\n");
		return;
	}

	for (i = 0; i < MAX_GIU4181INTR; i++)
		TAILQ_INIT(&sc->sc_intr_head[i]);

	if (!(sc->sc_ih
	      = vrip_intr_establish(va->va_vc, va->va_unit, 0,
				    IPL_BIO, vr4181giu_intr, sc))) {
		printf("%s: can't establish interrupt\n", sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * fill hpcio_chip structure
	 */
	sc->sc_iochip = vr4181giu_iochip; /* structure copy */
	sc->sc_iochip.hc_chipid = VRIP_IOCHIP_VR4181GIU;
	sc->sc_iochip.hc_name = sc->sc_dev.dv_xname;
	sc->sc_iochip.hc_sc = sc;
	/* Register functions to upper interface */
	vrip_register_gpio(va->va_vc, &sc->sc_iochip);

	printf("\n");

	/* 
	 *  hpcio I/F
	 */
	sc->sc_haa.haa_busname = HPCIO_BUSNAME;
	sc->sc_haa.haa_sc = sc;
	sc->sc_haa.haa_getchip = vr4181giu_getchip;
	sc->sc_haa.haa_iot = sc->sc_iot;
	while (config_found(self, &sc->sc_haa, vr4181giu_print)) ;

	/*
	 * GIU-ISA bridge
	 */
#if 1 /* XXX Sometimes mounting root device failed. Why? XXX*/
	config_defer(self, vr4181giu_callback);
#else
	vr4181giu_callback(self);
#endif
}

static void
vr4181giu_callback(struct device *self)
{
	struct vr4181giu_softc		*sc = (void *) self;

	sc->sc_haa.haa_busname = "vrisab";
	config_found(self, &sc->sc_haa, vr4181giu_print);
}

static int
vr4181giu_print(void *aux, const char *pnp)
{
	if (pnp)
		return (QUIET);
	return (UNCONF);
}

static int
vr4181giu_port_read(hpcio_chip_t hc, int port)
{
	struct vr4181giu_softc	*sc = hc->hc_sc;
	u_int16_t		r;

	if (port < 0 || 32 <= port)
		panic("vr4181giu_port_read: invalid gpio port");

	if (port < 16) {
		r = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				     VR4181GIU_PIOD_L_REG_W)
			& 1 << port;
	} else {
		r = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				     VR4181GIU_PIOD_H_REG_W)
			& 1 << (port - 16);
	}
	return r ? 1 : 0;
}

static void
vr4181giu_port_write(hpcio_chip_t hc, int port, int onoff)
{
	struct vr4181giu_softc	*sc = hc->hc_sc;
	u_int16_t		r;
	
	if (port < 16) {
		r = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				     VR4181GIU_PIOD_L_REG_W);
		if (onoff) {
			r |= 1 << port;
		} else {
			r &= ~(1 << port);
		}
		bus_space_write_2(sc->sc_iot, sc->sc_ioh,
				  VR4181GIU_PIOD_L_REG_W, r);
	} else {
		r = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
				     VR4181GIU_PIOD_H_REG_W);
		if (onoff) {
			r |= 1 << (port - 16);
		} else {
			r &= ~(1 << (port - 16));
		}
		bus_space_write_2(sc->sc_iot, sc->sc_ioh,
				  VR4181GIU_PIOD_H_REG_W, r);
	}
}

/*
 * XXXXXXXXXXXXXXXXXXXXXXXX
 */
static void
vr4181giu_update(hpcio_chip_t hc)
{
}

static void
vr4181giu_dump(hpcio_chip_t hc)
{
}

static hpcio_chip_t
vr4181giu_getchip(void* scx, int chipid)
{
	struct vr4181giu_softc	*sc = scx;

	return (&sc->sc_iochip);
}

static void *
vr4181giu_intr_establish(
	hpcio_chip_t hc,
	int port, /* GPIO pin # */
	int mode, /* GIU trigger setting */
	int (*ih_fun)(void *),
	void *ih_arg)
{
	struct vr4181giu_softc		*sc = hc->hc_sc;
	struct vr4181giu_intr_entry	*ih;
	int				s;
	u_int32_t 			mask;
	u_int32_t 			raw_intr_type;
	int				regmod;
	int				reghl;
	int				bitoff;
	u_int16_t			r;

	/*
	 * trigger mode translation
	 *
	 * VR4181 only support for four type of interrupt trigger
	 * listed below:
	 *
	 * 1. high level
	 * 2. low level
	 * 3. rising edge
	 * 4. falling edge
	 *
	 * argument mode is a bitmap as following:
	 *
	 * 001 detection trigger       (1:edge/0:level  )
	 * 010 signal hold/through     (1:hold/0:through)
	 * 100 detection level         (1:high/0:low    )
	 *
	 * possible mode value is 000B to 111B.
	 *
	 * 000 HPCIO_INTR_LEVEL_LOW_THROUGH
	 * 001 HPCIO_INTR_EDGE_THROUGH
	 * 010 HPCIO_INTR_LEVEL_LOW_HOLD
	 * 011 HPCIO_INTR_EDGE_HOLD
	 * 100 HPCIO_INTR_LEVEL_HIGH_THROUGH
	 * 101 falling edge and through?
	 * 110 HPCIO_INTR_LEVEL_HIGH_HOLD
	 * 111 falling edge and hold?
	 */

	static u_int32_t intr_mode_trans[8] = {
		VR4181GIU_INTTYP_LOW_LEVEL,	/* 000 */
		VR4181GIU_INTTYP_RISING_EDGE,	/* 001 */
		VR4181GIU_INTTYP_LOW_LEVEL,	/* 010 */
		VR4181GIU_INTTYP_RISING_EDGE,	/* 011 */
		VR4181GIU_INTTYP_HIGH_LEVEL,	/* 100 */
		VR4181GIU_INTTYP_FALLING_EDGE,	/* 101 */
		VR4181GIU_INTTYP_HIGH_LEVEL,	/* 110 */
		VR4181GIU_INTTYP_FALLING_EDGE,	/* 111 */
	};

	raw_intr_type = intr_mode_trans[mode];
	if (raw_intr_type == VR4181GIU_INTTYP_INVALID)
		panic("vr4181giu_intr_establish: invalid interrupt mode.");
	
	if (port < 0 || MAX_GIU4181INTR <= port)
		panic("vr4181giu_intr_establish: invalid interrupt line.");
	if (!TAILQ_EMPTY(&sc->sc_intr_head[port])
	    && raw_intr_type != sc->sc_intr_mode[port])
		panic("vr4181giu_intr_establish: "
		      "cannot use one line with two modes at a time.");
	else
		sc->sc_intr_mode[port] = raw_intr_type;
	mask = (1 << port);

	s = splhigh();

	if ((ih = malloc(sizeof *ih, M_DEVBUF, M_NOWAIT)) == NULL)
		panic("vr4181giu_intr_establish: memory exhausted.");

	ih->ih_port = port;
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	TAILQ_INSERT_TAIL(&sc->sc_intr_head[port], ih, ih_link);

	/*
	 * setup GIU registers
	 */

	/* disable interrupt at first */
	r = bus_space_read_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTEN_REG_W);
	r &= ~mask;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTEN_REG_W, r);

	/* mode */
	regmod = port >> 3;
	bitoff = (port & 0x7) << 1;
	r = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
			     VR4181GIU_MODE0_REG_W + regmod);
	r &= ~(0x3 << bitoff);
	r |= (VR4181GIU_MODE_IN  | VR4181GIU_MODE_GPIO) << bitoff;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VR4181GIU_MODE0_REG_W + regmod, r);
	/* interrupt type */
	reghl = port < 8 ? 2 : 0;	/* high byte: 0x0, lowbyte: 0x2 */
	r = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
			     VR4181GIU_INTTYP_REG + reghl);
	r &= ~(0x3 << bitoff);
	r |= raw_intr_type << bitoff;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VR4181GIU_INTTYP_REG + reghl, r);
	
	/* clear status */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VR4181GIU_INTSTAT_REG_W, mask);

	/* unmask */
	r = bus_space_read_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTMASK_REG_W);
	r &= ~mask;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTMASK_REG_W, r);

	/* enable */
	r = bus_space_read_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTEN_REG_W);
	r |= mask;
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTEN_REG_W, r);

	splx(s);

	return ih;
}

static void
vr4181giu_intr_disestablish(hpcio_chip_t hc, void *arg)
{
}

static void
vr4181giu_intr_clear(hpcio_chip_t hc, void *arg)
{
	struct vr4181giu_softc		*sc = hc->hc_sc;
	struct vr4181giu_intr_entry	*ih = arg;

	bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			  VR4181GIU_INTSTAT_REG_W, 1 << ih->ih_port);
}

static void
vr4181giu_register_iochip(hpcio_chip_t hc, hpcio_chip_t iochip)
{
	struct vr4181giu_softc	*sc = hc->hc_sc;

	vrip_register_gpio(sc->sc_vc, iochip);
}

/*
 * interrupt handler
 */
static int
vr4181giu_intr(void *arg)
{
	struct vr4181giu_softc	*sc = arg;
	int			i;
	u_int16_t		r;

	r = bus_space_read_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTSTAT_REG_W);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, VR4181GIU_INTSTAT_REG_W, r);

	for (i = 0; i < MAX_GIU4181INTR; i++) {
		if (r & (1 << i)) {
			struct vr4181giu_intr_entry *ih;
			TAILQ_FOREACH(ih, &sc->sc_intr_head[i], ih_link) {
				ih->ih_fun(ih->ih_arg);
			}
		}
	}

	return 0;
}
