/*	$NetBSD: if_ne_obio.c,v 1.2.6.1 2006/04/22 11:37:23 simonb Exp $ */

/*
 * Copyright (c) 2002, 2003  Genetec corp.  All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ne_obio.c,v 1.2.6.1 2006/04/22 11:37:23 simonb Exp $");

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

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>
#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <evbarm/g42xxeb/g42xxeb_var.h>
#include <arm/xscale/pxa2x0var.h>

struct ne_obio_softc {
	struct ne2000_softc nsc;

	void  *sc_ih;			/* interrupt handler */
	int  intr_no;
};

int	ne_obio_match( struct device *, struct cfdata *, void * );
void	ne_obio_attach( struct device *, struct device *, void * );


CFATTACH_DECL(ne_obio, sizeof (struct ne_obio_softc), ne_obio_match, ne_obio_attach,
    NULL, NULL);

//#define DEBUG_GPIO  2
//#define DEBUG_GPIO2  5

#if 1
#define	intr_handler	dp8390_intr
#else
#define	intr_handler	ne_obio_intr

static int
ne_obio_intr(void *arg)
{
	int rv=1;

#ifdef DEBUG_GPIO
	//printf( "ne_obio_intr arg=%p\n", arg );
	bus_space_write_4( pxa2x0_softc->saip.sc_iot, 
	    pxa2x0_softc->saip.sc_gpioh,
	    GPIO_GPSR0, 1U<<DEBUG_GPIO );
#endif
	
#if 0
	while( (rv = dp8390_intr(arg)) != 0)
		/*continue*/;
#else
	rv = dp8390_intr(arg);
#endif

#ifdef DEBUG_GPIO
	bus_space_write_4( pxa2x0_softc->saip.sc_iot, 
	    pxa2x0_softc->saip.sc_gpioh,
	    GPIO_GPCR0, 1U<<DEBUG_GPIO );
#endif
	return rv;
}
#endif

static int
ne_obio_enable(struct dp8390_softc *dsc)
{
	struct ne_obio_softc *sc = (struct ne_obio_softc *)dsc;
	struct obio_softc *psc;

	printf("%s: enabled\n", dsc->sc_dev.dv_xname);

	psc = (struct obio_softc *)device_parent(&dsc->sc_dev);

	/* Establish the interrupt handler. */
	sc->sc_ih = obio_intr_establish(psc, sc->intr_no, IPL_NET,
	    IST_LEVEL_LOW,
	    intr_handler,
	    sc);

	return 0;
}


int
ne_obio_match( struct device *parent, struct cfdata *match, void *aux )
{
	/* XXX: probe? */
	return 1;
}

void
ne_obio_attach( struct device *parent, struct device *self, void *aux )
{
	struct ne_obio_softc *sc = (struct ne_obio_softc *)self;
	struct obio_attach_args *oba = aux;
	bus_space_tag_t iot = oba->oba_iot;
	bus_space_handle_t  nioh, aioh;
	uint8_t my_ea[ETHER_ADDR_LEN];
	int i;

	printf("\n");

	/* Map i/o space. */
	if (bus_space_map(iot, oba->oba_addr, NE2000_NPORTS, 0, &nioh))
		return;

	if (bus_space_subregion(iot, nioh, NE2000_ASIC_OFFSET,
	    NE2000_ASIC_NPORTS, &aioh))
		goto out2;

	sc->nsc.sc_dp8390.sc_regt = iot;
	sc->nsc.sc_dp8390.sc_regh = nioh;

	sc->nsc.sc_asict = iot;
	sc->nsc.sc_asich = aioh;

	/*
	 * XXX:
	 * AX88796's reset register doesn't seem to work, and
	 * ne2000_detect() fails.  We hardcord NIC type here.
	 */
	sc->nsc.sc_type = NE2000_TYPE_NE2000; /* XXX _AX88796 ? */
	sc->nsc.sc_dp8390.sc_flags = DP8390_NO_REMOTE_DMA_COMPLETE;
	/* DP8390_NO_MULTI_BUFFERING; XXX */

	/* G4250EBX doesn't have EEPROM hooked to AX88796.  Read MAC
	 * address set by Redboot and don't let ne2000_atthch() try to
	 * read MAC from hardware.  (current ne2000 driver doesn't
	 * support AX88796's EEPROM interface)
	 */

	bus_space_write_1( iot, nioh, ED_P0_CR, ED_CR_RD2|ED_CR_PAGE_1 );

	for( i=0; i < ETHER_ADDR_LEN; ++i )
		my_ea[i] = bus_space_read_1( iot, nioh, ED_P1_PAR0+i );

	bus_space_write_1( iot, nioh, ED_P0_CR, ED_CR_RD2|ED_CR_PAGE_0 );

	/* disable all interrupts */
	bus_space_write_1(iot, nioh, ED_P0_IMR, 0);

#ifdef DEBUG_GPIO
	bus_space_write_4( pxa2x0_softc->saip.sc_iot, 
	    pxa2x0_softc->saip.sc_gpioh,
	    GPIO_GPDR0, (0x01<<DEBUG_GPIO) | (0x01<<DEBUG_GPIO2) |
	    bus_space_read_4(pxa2x0_softc->saip.sc_iot, pxa2x0_softc->saip.sc_gpioh,
		GPIO_GPDR0));

#endif
	/* delay intr_establish until this IF is enabled
	   to avoid spurious interrupts. */
	sc->nsc.sc_dp8390.sc_enabled = 0;
	sc->nsc.sc_dp8390.sc_enable = ne_obio_enable;
	sc->intr_no = oba->oba_intr;

	if( ne2000_attach( &sc->nsc, my_ea ) )
		goto out;

#ifndef ED_DCR_RDCR
#define ED_DCR_RDCR 0
#endif

	bus_space_write_1(iot, nioh, ED_P0_DCR, ED_DCR_RDCR|ED_DCR_WTS);

	return;
	
 out:
	bus_space_unmap( iot, aioh, NE2000_ASIC_NPORTS );
 out2:
	bus_space_unmap( iot, nioh, NE2000_NPORTS );
	return;
}

#if 0
void debug_obio_ne(struct dp8390_softc *);
void
debug_obio_ne(struct dp8390_softc *sc)
{
	struct obio_softc *osc =
	    (struct obio_softc *)device_parent(&sc->sc_dev);
	struct pxa2x0_softc *psc =
	    (struct pxa2x0_softc *)device_parent(&osc->sc_dev);

	printf( "ISR=%02x obio: pending=(%x,%x) mask=%x pending=%x mask=%x\n",
	    bus_space_read_1(sc->sc_regt, sc->sc_regh,
		ED_P0_ISR ),
	    bus_space_read_2(osc->sc_iot, osc->sc_obioreg_ioh, G4250EBX_INTSTS1),
	    bus_space_read_2(osc->sc_iot, osc->sc_obioreg_ioh, G4250EBX_INTSTS2),
	    bus_space_read_2(osc->sc_iot, osc->sc_obioreg_ioh, G4250EBX_INTMASK),
	    osc->sc_intr_pending,
	    osc->sc_intr_mask );

	printf( "intc: mask=%08x pending=%08x\n",
	    bus_space_read_4(psc->saip.sc_iot, psc->saip.sc_ioh, SAIPIC_MR ),
	    bus_space_read_4(psc->saip.sc_iot, psc->saip.sc_ioh, SAIPIC_IP ) );
}
#endif

#ifdef DEBUG_GPIO2
void dp8390_debug_overrun(void);
void dp8390_debug_overrun(void)
{
	static int toggle=0;

	//printf( "ne_obio_intr arg=%p\n", arg );
	bus_space_write_4( pxa2x0_softc->saip.sc_iot, 
	    pxa2x0_softc->saip.sc_gpioh,
	    toggle ? GPIO_GPCR0 : GPIO_GPSR0, 
			   1U<<DEBUG_GPIO2 );
	toggle ^= 1;
	
}
#endif
