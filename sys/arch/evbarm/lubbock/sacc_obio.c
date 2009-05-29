/*	$NetBSD: sacc_obio.c,v 1.10 2009/05/29 14:15:44 rjs Exp $ */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by IWAMOTO Toshihiro.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * for SA-1111 companion chip on Intel DBPXA250 evaluation board.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sacc_obio.c,v 1.10 2009/05/29 14:15:44 rjs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <arm/sa11x0/sa1111_reg.h>
#include <arm/sa11x0/sa1111_var.h>
#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_gpio.h>

#include <evbarm/lubbock/lubbock_reg.h>
#include <evbarm/lubbock/lubbock_var.h>


static	void	sacc_obio_attach(device_t, device_t, void *);
static	int  sacc_obio_intr(void *arg);

CFATTACH_DECL_NEW(sacc_obio, sizeof(struct sacc_softc), sacc_probe, 
    sacc_obio_attach, NULL, NULL);

#if 0
#define DPRINTF(arg)	aprint_normal arg
#else
#define DPRINTF(arg)
#endif

uint16_t cs2_memctl_init = 0x7ff0;

static void
sacc_obio_attach(device_t parent, device_t self, void *aux)
{
	int i;
	u_int32_t skid, tmp;
	struct sacc_softc *sc = device_private(self);
	struct obio_softc *psc = device_private(parent);
	struct obio_attach_args *sa = aux;
	bus_space_tag_t iot = sa->oba_iot;
	bus_space_handle_t memctl_ioh;

	aprint_normal("\n");

	/* Set alternative function for GPIO pings 48..57 on PXA2X0 */
	for (i=48; i <= 55; ++i)
		pxa2x0_gpio_set_function(i, GPIO_ALT_FN_2_OUT);
	pxa2x0_gpio_set_function(56, GPIO_ALT_FN_1_IN);
	pxa2x0_gpio_set_function(57, GPIO_ALT_FN_1_IN);

	/* XXX */
	if (bus_space_map(iot, PXA2X0_MEMCTL_BASE, PXA2X0_MEMCTL_SIZE, 0,
			  &memctl_ioh))
		goto fail;

	tmp = bus_space_read_4(iot, memctl_ioh, MEMCTL_MSC2 );
	bus_space_write_4(iot, memctl_ioh, MEMCTL_MSC2, 
	    (tmp & 0xffff0000) | cs2_memctl_init );

	bus_space_unmap(iot, memctl_ioh, PXA2X0_MEMCTL_SIZE);

	sc->sc_dev = self;
	sc->sc_piot = sc->sc_iot = iot;
	sc->sc_gpioh = 0;	/* not used */

	if (bus_space_map(iot, sa->oba_addr, 0x2000/*size*/, 0, &sc->sc_ioh))
		goto fail;

	skid = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCSBI_SKID);

	aprint_normal_dev(self, "SA1111 rev %d.%d\n",
			  (skid & 0xf0) >> 4, skid & 0xf);

	tmp = bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCSBI_SKCR);
	tmp = (tmp & ~SKCR_VCOOFF) | SKCR_PLLBYPASS;
	bus_space_write_4( sc->sc_iot, sc->sc_ioh, SACCSBI_SKCR, tmp );

	delay(100);			/* XXX */

	tmp |= SKCR_RCLKEN;
	bus_space_write_4( sc->sc_iot, sc->sc_ioh, SACCSBI_SKCR, tmp );

#if 1
	if( tmp != bus_space_read_4( sc->sc_iot, sc->sc_ioh, SACCSBI_SKCR ) )
		printf( "!!! FAIL SKCR\n" );
#endif

	/* PCMCIA socket0 power control */
	bus_space_write_4( sc->sc_iot, sc->sc_ioh, SACCGPIOA_DVR, 0 );
	bus_space_write_4( sc->sc_iot, sc->sc_ioh, SACCGPIOA_DDR, 0 );

	for(i = 0; i < SACCIC_LEN; i++)
		sc->sc_intrhand[i] = NULL;

	/* initialize SA1111 interrupt controller */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN0, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTEN1, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTTSTSEL, 0);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  SACCIC_INTSTATCLR0, 0xffffffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh,
			  SACCIC_INTSTATCLR1, 0xffffffff);

	/* connect to On-board peripheral interrupt */
	obio_intr_establish(psc, sa->oba_intr,
			    IPL_HIGH, sacc_obio_intr, sc );
	/*
	 *  Attach each devices
	 */
	config_search_ia(sa1111_search, self, "sacc", NULL);

	return;

 fail:
	aprint_normal_dev(self, "unable to map registers\n");
}

static int
sacc_obio_intr(void *arg)
{
	int i;
	struct sacc_intrvec intstat;
	struct sacc_softc *sc = arg;
	struct sacc_intrhand *ih;

	intstat.lo =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR0);
	intstat.hi =
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, SACCIC_INTSTATCLR1);
	DPRINTF(("sacc_obio_intr_dispatch: %x %x\n", intstat.lo, intstat.hi));

	while ((i = find_first_bit(intstat.lo)) >= 0) {

		/*
		 * Clear intr status before calling intr handlers.
		 * This cause stray interrupts, but clearing
		 * after calling intr handlers cause intr lossage.
		 */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  SACCIC_INTSTATCLR0, 1U<<i );

		for(ih = sc->sc_intrhand[i]; ih; ih = ih->ih_next)
			softint_schedule(ih->ih_soft);

		intstat.lo &= ~(1U<<i);
	}

	while ((i = find_first_bit(intstat.hi)) >= 0) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
				  SACCIC_INTSTATCLR1, 1U<<i);

		for(ih = sc->sc_intrhand[i + 32]; ih; ih = ih->ih_next)
			softint_schedule(ih->ih_soft);

		intstat.hi &= ~(1U<<i);
	}

	return 1;
}
