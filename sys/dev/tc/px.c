/*	$NetBSD: px.c,v 1.2 1998/01/12 09:51:33 thorpej Exp $	*/

/*
 * Copyright (c) 1997 Jonathan Stone
 * All rights reserved.
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
 *      This product includes software developed by Jonathan Stone for
 *      the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: px.c,v 1.2 1998/01/12 09:51:33 thorpej Exp $");

/*
 * px.c: placebo driver for the DEC TURBOchannel 2-d and 3-d
 * accelerated framebuffers with PixelStamp blitter asics and i860
 * accelerators.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <dev/tc/tcvar.h>
#include <dev/tc/sticvar.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/bus.h>

/*
 * hardware offsets  within PX board's TC slot.
 */
#define PX_STIC_POLL_OFFSET	0x000000
#define PX_STAMP_OFFSET		0x0c0000	/* pixelstamp space on STIC */
#define PX_STIC_OFFSET		0x180000	/* STIC registers */
#define PX_VDAC_OFFSET		0x200000
#define PX_ROM_OFFSET		0x300000


struct px_softc {
	struct 	device sc_dv;		/* device information */
	tc_addr_t px_slotbase;		/* kva of slot base. */
  	struct stic_softc px_stic;	/* address of pixelstamp and stic. */
};
	  
/* 
 * Local prototypes.
 */
int	px_match __P((struct device *, struct cfdata *, void *));
void	px_attach __P((struct device *, struct device *, void *));
int	px_intr __P((void *xxx_sc));
void	px_vblank_ctl __P((struct px_softc *sc, int onoff));
void	px_blank __P((struct  px_softc *sc));
void	px_unblank __P((struct  px_softc *sc));

struct cfattach px_ca = {
	sizeof (struct px_softc), px_match, px_attach,
};

/*
 * Match a PMAG-C pixelstamp board.
 */
int
px_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct tc_attach_args *ta = aux;
	void *pxaddr;

	if (strncmp("PMAG-CA ", ta->ta_modname, TC_ROM_LLEN))
		return (0);

	pxaddr = (void*)ta->ta_addr;
#if 0
	if (tc_badaddr(pxaddr + 0))
		return (0);
#endif

	return (1);
}


/*
 * Attach a PMAG-C pixelstamp graphics board.
 */
void
px_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct px_softc *sc  = (struct px_softc*)self;
	struct tc_attach_args *ta = aux;

	sc->px_slotbase = TC_PHYS_TO_UNCACHED(ta->ta_addr);
	sc->px_stic.stic_pktbuf=  
	     (void*)(sc->px_slotbase + PX_STIC_POLL_OFFSET);
	sc->px_stic.stic_addr = (void*)(sc->px_slotbase + PX_STIC_OFFSET);
	sc->px_stic.stamp_addr = (void*)(sc->px_slotbase + PX_STAMP_OFFSET);
	sc->px_stic.vdac_addr = (void*)(sc->px_slotbase + PX_VDAC_OFFSET);


	/* Turn off vertical-blank interrupts, unless we're debugging. */
#if !defined(DIAGNOSTIC) && !defined(DEBUG)
	px_vblank_ctl(sc, 0);
#endif

	tc_intr_establish(parent, ta->ta_cookie, TC_IPL_NONE, px_intr, sc);

	/* driver does nothing yet, except silently dismisses interrupts. */
	printf(": no raster-console or X11 support.\n");

}
	
/*
 * pixelstamp board interrupt hanlder.
 * XXX examine pixelstamp blitter-chip packet area, and 
 * send new packets.
 *
 * For now, we ignore interrupts from the blitter chip or i860,
 * and just handle vertical-retrace interrupt.
 */
int
px_intr(xxx_sc)
    void *xxx_sc;
{
	struct px_softc *sc = (struct px_softc *)xxx_sc;

	volatile struct stic_regs * stic =
	   STICADDR(sc->px_stic.stic_addr);

	register int intr_status = stic->ipdvint;

	/* Clear packet-done intr so we don't interrupt again. */
	/* Packet interrupt? */
	if (intr_status & STIC_INT_P) {

		/*
		 * Clear *only* packet done interrupt
		 */
		stic->ipdvint = (stic->ipdvint | STIC_INT_P_WE) &
		     ~(STIC_INT_E_WE | STIC_INT_V_WE | STIC_INT_P);
		tc_wmb();

	}
	/* Vertical-retrace interrupt ? */
	else if (intr_status & STIC_INT_V) {

		stic->ipdvint = (stic->ipdvint | STIC_INT_V_WE) &
			~(STIC_INT_E_WE | STIC_INT_P_WE | STIC_INT_V);
		tc_wmb();

#ifdef notyet
		/* Poll for LK-201 LED status, update LEDs */
		lk201_led(unit);
#endif

	/* Error, stray interrupt ?*/
	} else if (intr_status & STIC_INT_E) {
#if defined(DIAGNOSTIC) || 1
		 /* XXX not for me */
		printf("px_intr: stray intr INT_E, %x %x %x %x %x",
		       intr_status,
		       stic->sticsr, stic->buscsr,
		       stic->busadr, stic->busdat);
		DELAY(1000000);
		/*panic("px_intr: no intr condition\n");*/
#endif
	} else {
#if defined(DIAGNOSTIC) || 1
		DELAY(1000000);
		 /* XXX not for me */
		printf("px_intr:, no intr? %x %x %x %x %x", 
		       intr_status,
		       stic->sticsr, stic->buscsr,
		       stic->busadr, stic->busdat);
		 DELAY(100000);
		/*panic("px_intr: no intr condition\n");*/
#endif
	}

	return(0); /* XXX forme */
}


/*
 * Turn vertical retrace interrupt on or off
 */
void
px_vblank_ctl(sc, switch_on)
	struct px_softc	*sc;
	int	switch_on;

{
	register volatile struct stic_regs *stic = 
	    STICADDR(sc->px_stic.stic_addr);

	stic->ipdvint = (switch_on) ?
		STIC_INT_V_WE | STIC_INT_V_EN :
		STIC_INT_V_WE;

	tc_wmb();
}

void
px_blank(sc)
	struct  px_softc *sc;
{

}


void
px_unblank(sc)
	struct  px_softc *sc;
{
}
