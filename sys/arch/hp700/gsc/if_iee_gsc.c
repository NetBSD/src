/* $NetBSD: if_iee_gsc.c,v 1.5 2007/05/18 08:49:36 skrll Exp $ */

/*
 * Copyright (c) 2003 Jochen Kunz.
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
 * 3. The name of Jochen Kunz may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JOCHEN KUNZ
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL JOCHEN KUNZ
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * hp700 GSC bus MD frontend for the iee(4) Intel i82596 Ethernet driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_iee_gsc.c,v 1.5 2007/05/18 08:49:36 skrll Exp $");

/* autoconfig and device stuff */
#include <sys/param.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <machine/iomod.h>
#include <machine/autoconf.h>
#include <hp700/dev/cpudevs.h>
#include <hp700/gsc/gscbusvar.h>
#include "locators.h"
#include "ioconf.h"

/* bus_space / bus_dma etc. */
#include <machine/bus.h>
#include <machine/intr.h>

/* general system data and functions */
#include <sys/systm.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <sys/types.h>

/* tsleep / sleep / wakeup */
#include <sys/proc.h>
/* hz for above */
#include <sys/kernel.h>

/* network stuff */
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <sys/socket.h>
#include <sys/mbuf.h>

#include "bpfilter.h"
#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#include <dev/ic/i82596reg.h>
#include <dev/ic/i82596var.h>

#define IEE_GSC_IO_SZ	12
#define IEE_GSC_RESET	0
#define IEE_GSC_PORT	4
#define IEE_GSC_CHANATT	8
#define IEE_ISCP_BUSSY 0x1

/* autoconfig stuff */
static int iee_gsc_match(struct device *, struct cfdata *, void *);
static void iee_gsc_attach(struct device *, struct device *, void *);
static int iee_gsc_detach(struct device*, int);

struct iee_gsc_softc {
	struct iee_softc iee_sc;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	void *sc_ih;
};

CFATTACH_DECL(
	iee_gsc,
	sizeof(struct iee_gsc_softc),
	iee_gsc_match,
	iee_gsc_attach,
	iee_gsc_detach,
    	NULL
);

int iee_gsc_cmd(struct iee_softc *, u_int32_t);
int iee_gsc_reset(struct iee_softc *);

int
iee_gsc_cmd(struct iee_softc *sc, u_int32_t cmd)
{
	struct iee_gsc_softc *sc_gsc = (struct iee_gsc_softc *) sc;
	int n;

	SC_SCB->scb_cmd = cmd;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF, IEE_SCB_SZ,
	    BUS_DMASYNC_PREWRITE);
	/* Issue a Channel Attention to force the chip to read the cmd. */
	bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_CHANATT, 0);
	/* Wait for the cmd to finish */
	for (n = 0 ; n < 100000; n++) {
		DELAY(1);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF, 
		    IEE_SCB_SZ, BUS_DMASYNC_PREREAD);
		if (SC_SCB->scb_cmd == 0)
			break;
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF, IEE_SCB_SZ,
	    BUS_DMASYNC_PREREAD);
	if (n < 100000)
		return(0);
	printf("%s: iee_gsc_cmd: timeout n=%d\n", sc->sc_dev.dv_xname, n);
	return(-1);
}

int
iee_gsc_reset(struct iee_softc *sc)
{
	struct iee_gsc_softc *sc_gsc = (struct iee_gsc_softc *) sc;
	int n;
	u_int32_t cmd;

	/* Make sure the bussy byte is set and the cache is flushed. */
	SC_ISCP->iscp_bussy = IEE_ISCP_BUSSY;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCP_OFF, IEE_SCP_SZ 
	    + IEE_ISCP_SZ + IEE_SCB_SZ, BUS_DMASYNC_PREWRITE);
	/* Setup the PORT Command with pointer to SCP. */
	cmd = IEE_PORT_SCP | IEE_PHYS_SHMEM(IEE_SCP_OFF);
	/* Write a word to IEE_GSC_RESET to initiate a Hardware reset. */
	bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_RESET, 0);
	DELAY(1000);
	/* Write it to the chip, it wants this in two 16 bit parts. */
	if (sc->sc_type == I82596_CA) {
		bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_PORT,
		    (cmd & 0xffff));
		DELAY(1000);
		bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_PORT,
		    (cmd >> 16));
	} else {
		bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_PORT,
		    (cmd >> 16));
		DELAY(1000);
		bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_PORT,
		    (cmd & 0xffff));
	}
	DELAY(1000);
	/* Issue a Channel Attention to read SCP */
	bus_space_write_4(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_CHANATT, 0);
	/* Wait for the chip to initialize and read SCP and ISCP. */
	for (n = 0 ; n < 1000; n++) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_ISCP_OFF, 
		    IEE_ISCP_SZ, BUS_DMASYNC_PREREAD);
		if (SC_ISCP->iscp_bussy != IEE_ISCP_BUSSY)
			break;
		DELAY(100);
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_ISCP_OFF, 
	    IEE_ISCP_SZ, BUS_DMASYNC_PREREAD);
	if (n < 1000) {
		/* ACK interrupts we may have caused */
		(sc->sc_iee_cmd)(sc, IEE_SCB_ACK);
		return(0);
	}
	printf("%s: iee_gsc_reset timeout bussy=0x%x\n", sc->sc_dev.dv_xname, 
	    SC_ISCP->iscp_bussy);
	return(-1);
}

static int
iee_gsc_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct gsc_attach_args *ga = aux;

	if (ga->ga_type.iodc_type == HPPA_TYPE_FIO
	    && ga->ga_type.iodc_sv_model == HPPA_FIO_GLAN)
		/* beat old ie(4) i82586 driver */
		return(10);
	return(0);
}

static void
iee_gsc_attach(struct device *parent, struct device *self, void *aux)
{
	struct iee_gsc_softc *sc_gsc = (struct iee_gsc_softc *) self;
	struct iee_softc *sc = (struct iee_softc *) self;
	struct gsc_attach_args *ga = aux;
	char cpu_spec;
	int media[2];
	int rsegs;

	if (ga->ga_type.iodc_sv_model == HPPA_FIO_LAN)
		sc->sc_type = I82596_DX;	/* ASP(2) based */
	else
		sc->sc_type = I82596_CA;	/* LASI based */
	sc->sc_flags = IEE_NEED_SWAP;
	/*
	 * Pre PA7100LC CPUs don't support uncacheable mappings. So make 
	 * descriptors align to cache lines. Needed to avoid race conditions 
	 * caused by flushing cache lines that overlap multiple descriptors. 
	 */
        cpu_spec = HPPA_PA_SPEC_LETTER(hppa_cpu_info->hppa_cpu_info_pa_spec);
	if (cpu_spec == '\0' || cpu_spec == 'a' || cpu_spec == 'b')
		sc->sc_cl_align = 32;
	else
		sc->sc_cl_align = 1;

	sc_gsc->sc_iot = ga->ga_iot;
	if (bus_space_map(sc_gsc->sc_iot, ga->ga_hpa, IEE_GSC_IO_SZ, 0, 
	    &sc_gsc->sc_ioh)) {
		aprint_normal(": iee_gsc_attach: can't map I/O space\n");
		return;
	}

	sc->sc_dmat = ga->ga_dmatag;
	if (bus_dmamem_alloc(sc->sc_dmat, IEE_SHMEM_MAX, PAGE_SIZE, 0,
	    &sc->sc_dma_segs, 1, &rsegs, BUS_DMA_NOWAIT) != 0) {
		aprint_normal(": iee_gsc_attach: can't allocate %d bytes of "
		    "DMA memory\n", (int)IEE_SHMEM_MAX);
		return;
	}
	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_dma_segs, rsegs, IEE_SHMEM_MAX, 
	    (void **)&sc->sc_shmem_addr, BUS_DMA_NOWAIT) != 0) {
		aprint_normal(": iee_gsc_attach: can't map DMA memory\n");
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, rsegs);
		return;
	}
	if (bus_dmamap_create(sc->sc_dmat, IEE_SHMEM_MAX, rsegs, 
	    IEE_SHMEM_MAX, 0, BUS_DMA_NOWAIT, &sc->sc_shmem_map) != 0) {
		aprint_normal(": iee_gsc_attach: can't create DMA map\n");
		bus_dmamem_unmap(sc->sc_dmat, sc->sc_shmem_addr, IEE_SHMEM_MAX);
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, rsegs);
		return;
	}
	if (bus_dmamap_load(sc->sc_dmat, sc->sc_shmem_map, sc->sc_shmem_addr,
	    IEE_SHMEM_MAX, NULL, BUS_DMA_NOWAIT) != 0) {
		aprint_normal(": iee_gsc_attach: can't load DMA map\n");
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_shmem_map);
		bus_dmamem_unmap(sc->sc_dmat, sc->sc_shmem_addr, IEE_SHMEM_MAX);
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, rsegs);
		return;
	}
	memset(sc->sc_shmem_addr, 0, IEE_SHMEM_MAX);

	/* Setup SYSBUS byte. */
	SC_SCP->scp_sysbus = IEE_SYSBUS_BE | IEE_SYSBUS_INT | 
	    IEE_SYSBUS_TRG | IEE_SYSBUS_LIEAR | IEE_SYSBUS_STD;

	sc_gsc->sc_ih = hp700_intr_establish(&sc->sc_dev, IPL_NET,
	    iee_intr, sc, ga->ga_int_reg, ga->ga_irq);

	sc->sc_iee_reset = iee_gsc_reset;
	sc->sc_iee_cmd = iee_gsc_cmd;
	sc->sc_mediachange = NULL;
	sc->sc_mediastatus = NULL;

	media[0] = IFM_ETHER | IFM_MANUAL;
	media[1] = IFM_ETHER | IFM_AUTO;
	iee_attach(sc, ga->ga_ether_address, media, 2, IFM_ETHER | IFM_AUTO);
}

int
iee_gsc_detach(struct device* self, int flags)
{
	struct iee_gsc_softc *sc_gsc = (struct iee_gsc_softc *) self;
	struct iee_softc *sc = (struct iee_softc *) self;

	iee_detach(sc, flags);
	bus_space_unmap(sc_gsc->sc_iot, sc_gsc->sc_ioh, IEE_GSC_IO_SZ);
	bus_dmamap_unload(sc->sc_dmat, sc->sc_shmem_map);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_shmem_map);
	bus_dmamem_unmap(sc->sc_dmat, sc->sc_shmem_addr, IEE_SHMEM_MAX);
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, 1);
	/* There is no hp700_intr_disestablish()! */
	return(0);
}
