/*	$NetBSD: bcm2835_mbox.c,v 1.12 2017/12/10 21:38:26 skrll Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: bcm2835_mbox.c,v 1.12 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/timetc.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <arm/broadcom/bcm2835_mbox.h>
#include <arm/broadcom/bcm2835_mboxreg.h>
#include <arm/broadcom/bcm2835reg.h>

#include <dev/fdt/fdtvar.h>

struct bcm2835mbox_softc {
	device_t sc_dev;
	device_t sc_platdev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;
	void *sc_intrh;

	kmutex_t sc_lock;
	kmutex_t sc_intr_lock;
	kcondvar_t sc_chan[BCM2835_MBOX_NUMCHANNELS];
	uint32_t sc_mbox[BCM2835_MBOX_NUMCHANNELS];
};

static struct bcm2835mbox_softc *bcm2835mbox_sc;

static int bcmmbox_match(device_t, cfdata_t, void *);
static void bcmmbox_attach(device_t, device_t, void *);
static int bcmmbox_intr1(struct bcm2835mbox_softc *, int);
static int bcmmbox_intr(void *);

CFATTACH_DECL_NEW(bcmmbox, sizeof(struct bcm2835mbox_softc),
    bcmmbox_match, bcmmbox_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmmbox_match(device_t parent, cfdata_t match, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-mbox", NULL };
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bcmmbox_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2835mbox_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct bcmmbox_attach_args baa;
	const int phandle = faa->faa_phandle;
	int i;

	aprint_naive("\n");
	aprint_normal(": VC mailbox\n");

	sc->sc_dev = self;
	sc->sc_iot = faa->faa_bst;
	sc->sc_dmat = faa->faa_dmat;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_VM);
	for (i = 0; i < BCM2835_MBOX_NUMCHANNELS; ++i)
		cv_init(&sc->sc_chan[i], "bcmmbox");

	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get register address\n");
		return;
	}

	if (bus_space_map(faa->faa_bst, addr, size, 0, &sc->sc_ioh) != 0) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	char intrstr[128];
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	sc->sc_intrh = fdtbus_intr_establish(phandle, 0, IPL_VM, IST_LEVEL,
	    bcmmbox_intr, sc);
	if (sc->sc_intrh == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	/* enable mbox interrupt */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, BCM2835_MBOX_CFG,
	    BCM2835_MBOX_CFG_DATAIRQEN);

	if (bcm2835mbox_sc == NULL)
		bcm2835mbox_sc = sc;

	baa.baa_dmat = sc->sc_dmat;
	sc->sc_platdev = config_found_ia(self, "bcmmboxbus", &baa, NULL);
}

static int
bcmmbox_intr1(struct bcm2835mbox_softc *sc, int cv)
{
	uint32_t mbox, chan, data;
	int ret = 0;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	bus_space_barrier(sc->sc_iot, sc->sc_ioh, 0, BCM2835_MBOX_SIZE,
	    BUS_SPACE_BARRIER_READ);

	while ((bus_space_read_4(sc->sc_iot, sc->sc_ioh,
	    BCM2835_MBOX0_STATUS) & BCM2835_MBOX_STATUS_EMPTY) == 0) {

		mbox = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    BCM2835_MBOX0_READ);

		chan = BCM2835_MBOX_CHAN(mbox);
		data = BCM2835_MBOX_DATA(mbox);
		ret = 1;

		if (BCM2835_MBOX_CHAN(sc->sc_mbox[chan]) != 0) {
			aprint_error("bcmmbox_intr: chan %d overflow\n",chan);
			continue;
		}

		sc->sc_mbox[chan] = data | BCM2835_MBOX_CHANMASK;

		if (cv)
			cv_broadcast(&sc->sc_chan[chan]);
	}

	return ret;
}

static int
bcmmbox_intr(void *cookie)
{
	struct bcm2835mbox_softc *sc = cookie;
	int ret;

	mutex_enter(&sc->sc_intr_lock);

	ret = bcmmbox_intr1(sc, 1);

	mutex_exit(&sc->sc_intr_lock);

	return ret;
}

void
bcmmbox_read(uint8_t chan, uint32_t *data)
{
	struct bcm2835mbox_softc *sc = bcm2835mbox_sc;

	KASSERT(sc != NULL);

	mutex_enter(&sc->sc_lock);

	mutex_enter(&sc->sc_intr_lock);
	while (BCM2835_MBOX_CHAN(sc->sc_mbox[chan]) == 0) {
		if (cold)
			bcmmbox_intr1(sc, 0);
		else
			cv_wait(&sc->sc_chan[chan], &sc->sc_intr_lock);
	}
	*data = BCM2835_MBOX_DATA(sc->sc_mbox[chan]);
	sc->sc_mbox[chan] = 0;
	mutex_exit(&sc->sc_intr_lock);

	mutex_exit(&sc->sc_lock);
}

void
bcmmbox_write(uint8_t chan, uint32_t data)
{
	struct bcm2835mbox_softc *sc = bcm2835mbox_sc;

	KASSERT(sc != NULL);
	KASSERT(BCM2835_MBOX_CHAN(chan) == chan);
	KASSERT(BCM2835_MBOX_CHAN(data) == 0);

	mutex_enter(&sc->sc_lock);

	bcm2835_mbox_write(sc->sc_iot, sc->sc_ioh, chan, data);

	mutex_exit(&sc->sc_lock);
}

int
bcmmbox_request(uint8_t chan, void *buf, size_t buflen, uint32_t *pres)
{
	struct bcm2835mbox_softc *sc = bcm2835mbox_sc;
	void *dma_buf;
	bus_dmamap_t map;
	bus_dma_segment_t segs[1];
	int nsegs;
	int error;

	KASSERT(sc != NULL);

	error = bus_dmamem_alloc(sc->sc_dmat, buflen, 16, 0, segs, 1,
	    &nsegs, BUS_DMA_WAITOK);
	if (error)
		return error;
	error = bus_dmamem_map(sc->sc_dmat, segs, nsegs, buflen, &dma_buf,
	    BUS_DMA_WAITOK);
	if (error)
		goto map_failed;
	error = bus_dmamap_create(sc->sc_dmat, buflen, 1, buflen, 0,
	    BUS_DMA_WAITOK, &map);
	if (error)
		goto create_failed;
	error = bus_dmamap_load(sc->sc_dmat, map, dma_buf, buflen, NULL,
	    BUS_DMA_WAITOK);
	if (error)
		goto load_failed;

	memcpy(dma_buf, buf, buflen);

	bus_dmamap_sync(sc->sc_dmat, map, 0, buflen,
	     BUS_DMASYNC_PREWRITE|BUS_DMASYNC_PREREAD);
	bcmmbox_write(chan, map->dm_segs[0].ds_addr);
	bcmmbox_read(chan, pres);
	bus_dmamap_sync(sc->sc_dmat, map, 0, buflen,
	    BUS_DMASYNC_POSTWRITE|BUS_DMASYNC_POSTREAD);

	memcpy(buf, dma_buf, buflen);

	bus_dmamap_unload(sc->sc_dmat, map);
load_failed:
	bus_dmamap_destroy(sc->sc_dmat, map);
create_failed:
	bus_dmamem_unmap(sc->sc_dmat, dma_buf, buflen);
map_failed:
	bus_dmamem_free(sc->sc_dmat, segs, nsegs);

	return error;
}
