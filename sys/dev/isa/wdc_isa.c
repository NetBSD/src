/*	$NetBSD: wdc_isa.c,v 1.2 1998/01/17 00:40:45 cgd Exp $ */

/*
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * DMA and multi-sector PIO handling are derived from code contributed by
 * Onno van der Linden.
 *
 * ISA attachment created by Christopher G. Demetriou.
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
 *	This product includes software developed by Charles M. Hannum.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/buf.h>
#include <sys/uio.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/disk.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/isadmavar.h>
#include <dev/ic/wdcvar.h>

#define	WDC_ISA_REG_NPORTS	8
#define	WDC_ISA_AUXREG_OFFSET	0x206
#define	WDC_ISA_AUXREG_NPORTS	1

/*
 * XXX This code currently doesn't even try to allow 32-bit data port use.
 */

struct wdc_isa_softc {
	struct wdc_softc sc_wdcdev;
	struct wdc_attachment_data sc_ad;
	void	*sc_ih;
	int	sc_drq;
};

#ifdef __BROKEN_INDIRECT_CONFIG
int	wdc_isa_probe	__P((struct device *, void *, void *));
#else
int	wdc_isa_probe	__P((struct device *, struct cfdata *, void *));
#endif
void	wdc_isa_attach	__P((struct device *, struct device *, void *));

struct cfattach wdc_isa_ca = {
	sizeof(struct wdc_isa_softc), wdc_isa_probe, wdc_isa_attach
};

static void	wdc_isa_dma_setup __P((void *));
static void	wdc_isa_dma_start __P((void *, void *, size_t, int));
static void	wdc_isa_dma_finish __P((void *));

int
wdc_isa_probe(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
#if 0 /* XXX memset */
	struct wdc_attachment_data ad = { 0 };
#else /* XXX memset */
	struct wdc_attachment_data ad;
#endif /* XXX memset */
	struct isa_attach_args *ia = aux;
	int result = 0;

#if 0 /* XXX memset */
#else /* XXX memset */
	bzero(&ad, sizeof ad);
#endif /* XXX memset */
	ad.iot = ia->ia_iot;
	if (bus_space_map(ad.iot, ia->ia_iobase, WDC_ISA_REG_NPORTS, 0,
	    &ad.ioh))
		goto out;

	ad.auxiot = ia->ia_iot;
	if (bus_space_map(ad.auxiot, ia->ia_iobase + WDC_ISA_AUXREG_OFFSET,
	    WDC_ISA_AUXREG_NPORTS, 0, &ad.auxioh))
		goto outunmap;

	result = wdcprobe(&ad);
	if (result) {
		ia->ia_iosize = WDC_ISA_REG_NPORTS;
		ia->ia_msize = 0;
	}

	bus_space_unmap(ad.auxiot, ad.auxioh, WDC_ISA_AUXREG_NPORTS);
outunmap:
	bus_space_unmap(ad.iot, ad.ioh, WDC_ISA_REG_NPORTS);
out:
	return (result);
}

void
wdc_isa_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct wdc_isa_softc *sc = (struct wdc_isa_softc *)self;
	struct isa_attach_args *ia = aux;

	bzero(&sc->sc_ad, sizeof sc->sc_ad);
	sc->sc_ad.iot = ia->ia_iot;
	sc->sc_ad.auxiot = ia->ia_iot;
	if (bus_space_map(sc->sc_ad.iot, ia->ia_iobase, WDC_ISA_REG_NPORTS, 0,
	      &sc->sc_ad.ioh) ||
	    bus_space_map(sc->sc_ad.auxiot,
	      ia->ia_iobase + WDC_ISA_AUXREG_OFFSET, WDC_ISA_AUXREG_NPORTS,
	      0, &sc->sc_ad.auxioh)) {
		printf(": couldn't map registers\n");
		panic("wdc_isa_attach: couldn't map registers\n");
	}

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_BIO, wdcintr, sc);

	if (ia->ia_drq != DRQUNK) {
		sc->sc_drq = ia->ia_drq;

		sc->sc_ad.cap |= WDC_CAPABILITY_DMA;
		sc->sc_ad.dma_setup = &wdc_isa_dma_setup;
		sc->sc_ad.dma_start = &wdc_isa_dma_start;
		sc->sc_ad.dma_finish = &wdc_isa_dma_finish;
	}	

	wdcattach(&sc->sc_wdcdev, &sc->sc_ad);
}

static void
wdc_isa_dma_setup(scv)
	void *scv;
{
	struct wdc_isa_softc *sc = scv;

	if (isa_dmamap_create(sc->sc_wdcdev.sc_dev.dv_parent, sc->sc_drq,
	    MAXPHYS, BUS_DMA_NOWAIT|BUS_DMA_ALLOCNOW)) {
		printf("%s: can't create map for drq %d\n",
		    sc->sc_wdcdev.sc_dev.dv_xname, sc->sc_drq);
		sc->sc_ad.cap &= ~WDC_CAPABILITY_DMA;
	}
}

static void
wdc_isa_dma_start(scv, buf, size, read)
	void *scv, *buf;
	size_t size;
	int read;
{
	struct wdc_isa_softc *sc = scv;

	isa_dmastart(sc->sc_wdcdev.sc_dev.dv_parent, sc->sc_drq, buf,
	    size, NULL, read ? DMAMODE_READ : DMAMODE_WRITE,
	    BUS_DMA_NOWAIT);
}

static void
wdc_isa_dma_finish(scv)
	void *scv;
{
	struct wdc_isa_softc *sc = scv;

	isa_dmadone(sc->sc_wdcdev.sc_dev.dv_parent, sc->sc_drq);
}
