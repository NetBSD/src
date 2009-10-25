/* 	$NetBSD: tft_plb.c,v 1.3 2009/10/25 09:32:25 ahoka Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
__KERNEL_RCSID(0, "$NetBSD: tft_plb.c,v 1.3 2009/10/25 09:32:25 ahoka Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>

#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <evbppc/virtex/dev/xcvbusvar.h>
#include <evbppc/virtex/dev/tftreg.h>
#include <evbppc/virtex/dev/tftvar.h>


struct plb_tft_softc {
	struct tft_softc 	psc_sc;

	bus_dma_tag_t 		psc_dmat;
	bus_dmamap_t 		psc_dmap; 	/* for psc_sc.sc_image */
	bus_dma_segment_t 	psc_seg;

	void 			*sc_sdhook; 	/* stop DMA */
};

static void 	plb_tft_attach(struct device *, struct device *, void *);
static paddr_t 	plb_tft_mmap(void *, void *, off_t, int);

CFATTACH_DECL(plb_tft, sizeof(struct plb_tft_softc),
    xcvbus_child_match, plb_tft_attach, NULL, NULL);


/* Patched by tft_attach(), may be shared by all instances. */
static struct wsdisplay_accessops plb_tft_accessops = {
	.mmap 		= plb_tft_mmap,
};


/*
 * Generic device.
 */
static void
plb_tft_attach(struct device *parent, struct device *self, void *aux)
{
	struct xcvbus_attach_args *vaa = aux;
	struct plb_tft_softc 	*psc = device_private(self);
	struct tft_softc 	*sc = &psc->psc_sc;
	int 			nseg, error;

	psc->psc_dmat 	= vaa->vaa_dmat;
	sc->sc_iot 	= vaa->vaa_iot;

	printf(": PLB_TFT\n");

	if ((error = bus_space_map(sc->sc_iot, vaa->vaa_addr, TFT_SIZE,
	    0, &sc->sc_ioh)) != 0) {
	    	printf("%s: could not map device registers\n",
		    device_xname(self));
	    	goto fail_0;
	}

	/* Fill in resolution, depth, size. */
	tft_mode(self);

	/* Allocate and map framebuffer control data. */
	if ((error = bus_dmamem_alloc(psc->psc_dmat, sc->sc_size, ADDR_ALIGN,
	    0, &psc->psc_seg, 1, &nseg, 0)) != 0) {
	    	printf("%s: could not allocate framebuffer\n",
	    	    device_xname(self));
		goto fail_1;
	}
	if ((error = bus_dmamem_map(psc->psc_dmat, &psc->psc_seg, nseg,
	    sc->sc_size, &sc->sc_image, BUS_DMA_COHERENT)) != 0) {
	    	printf("%s: could not map framebuffer\n",
	    	    device_xname(self));
		goto fail_2;
	}
	if ((error = bus_dmamap_create(psc->psc_dmat, sc->sc_size, 1,
	    sc->sc_size, 0, 0, &psc->psc_dmap)) != 0) {
	    	printf("%s: could not create framebuffer DMA map\n",
	    	    device_xname(self));
		goto fail_3;
	}
	if ((error = bus_dmamap_load(psc->psc_dmat, psc->psc_dmap,
	    sc->sc_image, sc->sc_size, NULL, 0)) != 0) {
	    	printf("%s: could not load framebuffer DMA map\n",
	    	    device_xname(self));
		goto fail_4;
	}
	/* XXX hack, we linear map whole RAM and we have single segment */
	/* sc->sc_image = (void *)psc->psc_dmap->dm_segs[0].ds_addr; */
	/* XXX hack, use predefined base addr */
	sc->sc_image = (void *)(uintptr_t)0x3c00000;

	/* XXX forget the hack above, use "virtex-tft-framebuffer-base" prop */

	tft_attach(self, &plb_tft_accessops);

	printf("%s: video memory pa 0x%08x\n", device_xname(self),
	    (uint32_t)psc->psc_dmap->dm_segs[0].ds_addr);

#if 0
	/* XXX powerpc bus_dma doesn't support COHERENT. */
	bus_dmamap_sync(psc->psc_dmat, psc->psc_dmap, 0,
	    sc->sc_size, BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
#endif

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_RESET);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_ENABLE);

#if 0
	/* XXX how do we change framebuffer base? */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_RESET);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_ENABLE);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_ADDR,
	    ADDR_MAKE(psc->psc_dmap->dm_segs[0].ds_addr));
#endif

	return ;

 fail_4:
 	bus_dmamap_destroy(psc->psc_dmat, psc->psc_dmap);
 fail_3:
 	bus_dmamem_unmap(psc->psc_dmat, sc->sc_image, sc->sc_size);
 fail_2:
 	bus_dmamem_free(psc->psc_dmat, &psc->psc_seg, nseg);
 fail_1:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, TFT_SIZE);
 fail_0:
	printf("%s: error %d\n", device_xname(self), error);
}

static paddr_t
plb_tft_mmap(void *arg, void *scr, off_t offs, int prot)
{
	struct vcons_data 	*vc = arg;
	struct plb_tft_softc 	*psc = vc->cookie;
	paddr_t 		pa = -1;

#if 0 	/* XXX hack */
	if (offs < psc->psc_sc.sc_size)
		pa = bus_dmamem_mmap(psc->psc_dmat, &psc->psc_seg, 1,
		    offs, prot, BUS_DMA_WAITOK | BUS_DMA_COHERENT);
#endif
	if (offs < psc->psc_sc.sc_size)
		pa = (paddr_t)(intptr_t)psc->psc_sc.sc_image + offs;

	return (pa);
} 
