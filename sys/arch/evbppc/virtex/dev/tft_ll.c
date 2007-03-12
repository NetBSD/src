/* 	$NetBSD: tft_ll.c,v 1.1.8.1 2007/03/12 05:47:40 rmind Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tft_ll.c,v 1.1.8.1 2007/03/12 05:47:40 rmind Exp $");

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

/* XXX needed? */
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wscons/wsconsio.h>
#include <dev/rasops/rasops.h>
#include <dev/wscons/wsdisplay_vconsvar.h>

#include <evbppc/virtex/dev/xcvbusvar.h>
#include <evbppc/virtex/dev/cdmacreg.h>
#include <evbppc/virtex/dev/tftreg.h>
#include <evbppc/virtex/dev/tftvar.h>


struct ll_tft_control {
	struct cdmac_descr      cd_dsc;
	u_char                  cd_img[];
} __packed;

struct ll_tft_softc {
	struct tft_softc 	lsc_sc;

	bus_space_tag_t 	lsc_dma_iot;
	bus_space_handle_t 	lsc_dma_ioh;

	bus_dma_tag_t 		lsc_dmat;
	bus_dmamap_t 		lsc_dmap;

	struct ll_tft_control 	*lsc_cd;
	bus_dma_segment_t 	lsc_seg;
};

static void 	ll_tft_attach(struct device *, struct device *, void *);
static paddr_t 	ll_tft_mmap(void *, void *, off_t, int);
static void 	ll_tft_shutdown(void *);

CFATTACH_DECL(ll_tft, sizeof(struct ll_tft_softc),
    xcvbus_child_match, ll_tft_attach, NULL, NULL);


static struct wsdisplay_accessops ll_tft_accessops = {
	.mmap 			= ll_tft_mmap,
};


static void
ll_tft_attach(struct device *parent, struct device *self, void *aux)
{
	struct xcvbus_attach_args *vaa = aux;
	struct ll_dmac 		*tx = vaa->vaa_tx_dmac;
	struct ll_tft_softc 	*lsc = (struct ll_tft_softc *)self;
	struct tft_softc 	*sc = &lsc->lsc_sc;
	int 			nseg, error;

	KASSERT(tx);

	lsc->lsc_dma_iot 	= tx->dmac_iot;
	lsc->lsc_dmat 		= vaa->vaa_dmat;
	sc->sc_iot 		= vaa->vaa_iot;

	printf(": LL_TFT\n");

	if ((error = bus_space_map(sc->sc_iot, vaa->vaa_addr, TFT_SIZE,
	    0, &sc->sc_ioh)) != 0) {
	    	printf("%s: could not map device registers\n",
		    device_xname(self));
	    	goto fail_0;
	}
	if ((error = bus_space_map(lsc->lsc_dma_iot, tx->dmac_ctrl_addr,
	    CDMAC_CTRL_SIZE, 0, &lsc->lsc_dma_ioh)) != 0) {
		printf("%s: could not map dmac registers\n",
		    device_xname(self));
		goto fail_1;
	}

	/* Fill in resolution, depth, size. */
	tft_mode(&sc->sc_dev);

	/* Allocate and map framebuffer control data. */
	if ((error = bus_dmamem_alloc(lsc->lsc_dmat,
	    sizeof(struct ll_tft_control) + sc->sc_size, 8, 0,
	    &lsc->lsc_seg, 1, &nseg, 0)) != 0) {
	    	printf("%s: could not allocate framebuffer\n",
		    device_xname(self));
		goto fail_2;
	}
	if ((error = bus_dmamem_map(lsc->lsc_dmat, &lsc->lsc_seg, nseg,
	    sizeof(struct ll_tft_control) + sc->sc_size,
	    (void **)&lsc->lsc_cd, BUS_DMA_COHERENT)) != 0) {
	    	printf("%s: could not map framebuffer\n",
		    device_xname(self));
		goto fail_3;
	}
	if ((error = bus_dmamap_create(lsc->lsc_dmat,
	    sizeof(struct ll_tft_control) + sc->sc_size, 1,
	    sizeof(struct ll_tft_control) + sc->sc_size, 0, 0,
	    &lsc->lsc_dmap)) != 0) {
	    	printf("%s: could not create framebuffer DMA map\n",
		    device_xname(self));
		goto fail_4;
	}
	if ((error = bus_dmamap_load(lsc->lsc_dmat, lsc->lsc_dmap, lsc->lsc_cd,
	    sizeof(struct ll_tft_control) + sc->sc_size, NULL, 0)) != 0) {
	    	printf("%s: could not load framebuffer DMA map\n",
		    device_xname(self));
		goto fail_5;
	}

	/* Clear screen, setup descriptor. */
	memset(lsc->lsc_cd, 0x00, sizeof(struct ll_tft_control));
	sc->sc_image = lsc->lsc_cd->cd_img;

	lsc->lsc_cd->cd_dsc.desc_next = lsc->lsc_dmap->dm_segs[0].ds_addr;
	lsc->lsc_cd->cd_dsc.desc_addr = lsc->lsc_dmap->dm_segs[0].ds_addr +
	    offsetof(struct ll_tft_control, cd_img);
	lsc->lsc_cd->cd_dsc.desc_size = sc->sc_size;
	lsc->lsc_cd->cd_dsc.desc_stat = CDMAC_STAT_SOP;

	bus_dmamap_sync(lsc->lsc_dmat, lsc->lsc_dmap, 0,
	    sizeof(struct ll_tft_control) + sc->sc_size,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	sc->sc_sdhook = shutdownhook_establish(ll_tft_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    device_xname(self));

	tft_attach(self, &ll_tft_accessops);

	printf("%s: video memory pa 0x%08x\n", device_xname(self),
	    (uint32_t)lsc->lsc_cd->cd_dsc.desc_addr);

	/* Timing sensitive... */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_RESET);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TFT_CTRL, CTRL_ENABLE);
	bus_space_write_4(lsc->lsc_dma_iot, lsc->lsc_dma_ioh, CDMAC_CURDESC,
	    lsc->lsc_dmap->dm_segs[0].ds_addr);

	return ;

 fail_5:
 	bus_dmamap_destroy(lsc->lsc_dmat, lsc->lsc_dmap);
 fail_4:
 	bus_dmamem_unmap(lsc->lsc_dmat, (void *)lsc->lsc_cd,
 	    sizeof(struct ll_tft_control) + sc->sc_size);
 fail_3:
 	bus_dmamem_free(lsc->lsc_dmat, &lsc->lsc_seg, nseg);
 fail_2:
	bus_space_unmap(lsc->lsc_dma_iot, lsc->lsc_dma_ioh, CDMAC_CTRL_SIZE);
 fail_1:
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, TFT_SIZE);
 fail_0:
	printf("%s: error %d\n", device_xname(self), error);
}

static paddr_t
ll_tft_mmap(void *arg, void *scr, off_t offs, int prot)
{
	struct ll_tft_softc 	*lsc = arg;
	paddr_t 		pa;

	if (offs < lsc->lsc_sc.sc_size) {
		pa = bus_dmamem_mmap(lsc->lsc_dmat, &lsc->lsc_seg, 1,
		    offs + offsetof(struct ll_tft_control, cd_img),
		    prot, BUS_DMA_WAITOK | BUS_DMA_COHERENT);

		return (pa);
	}

	return (-1);
} 

static void
ll_tft_shutdown(void *arg)
{
	struct ll_tft_softc 	*lsc = arg;

	bus_space_write_4(lsc->lsc_dma_iot, lsc->lsc_dma_ioh, 0,
	    CDMAC_STAT_RESET);

	tft_shutdown(&lsc->lsc_sc);
}
