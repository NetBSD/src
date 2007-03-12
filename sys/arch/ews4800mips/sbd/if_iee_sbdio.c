/*	$NetBSD: if_iee_sbdio.c,v 1.1.28.1 2007/03/12 05:47:42 rmind Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_iee_sbdio.c,v 1.1.28.1 2007/03/12 05:47:42 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <sys/socket.h>
#include <sys/mbuf.h>

#include <mips/cache.h>
#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/sbdvar.h>	/* for ether_addr() */
#include <machine/sbdiovar.h>

#include <dev/ic/i82596reg.h>
#include <dev/ic/i82596var.h>

int iee_sbdio_match(struct device *, struct cfdata *, void *);
void iee_sbdio_attach(struct device *, struct device *, void *);
int iee_sbdio_cmd(struct iee_softc *, uint32_t);
int iee_sbdio_reset(struct iee_softc *);

static void iee_sbdio_channel_attention(void *);
static void iee_sbdio_chip_reset(void *);
static void iee_sbdio_set_scp(void *, uint32_t);

struct iee_sbdio_softc {
	struct iee_softc sc_iee;
	/* CPU <-> i82589 interface */
	volatile uint32_t *sc_port;
};

CFATTACH_DECL(iee_sbdio, sizeof(struct iee_sbdio_softc),
    iee_sbdio_match, iee_sbdio_attach, 0, 0);

int
iee_sbdio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct sbdio_attach_args *sa = aux;

	return strcmp(sa->sa_name, "iee") ? 0 : 1;
}

void
iee_sbdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbdio_attach_args *sa = aux;
	struct iee_sbdio_softc *sc_ssc = (void *)self;
	struct iee_softc *sc = &sc_ssc->sc_iee;
	uint8_t eaddr[ETHER_ADDR_LEN];
	int media[2];
	int rsegs;

	printf(" at %p irq %d ", (void *)sa->sa_addr1, sa->sa_irq);

	sc_ssc->sc_port = (volatile uint32_t *)
	    MIPS_PHYS_TO_KSEG1(sa->sa_addr1);
	sc->sc_type = I82596_CA;
	sc->sc_flags = IEE_NEED_SWAP;


	/* bus_dma round the dma size to page size. */
	sc->sc_cl_align = 1;

	sc->sc_dmat = sa->sa_dmat;

	if (bus_dmamem_alloc(sc->sc_dmat, IEE_SHMEM_MAX, PAGE_SIZE, 0,
		&sc->sc_dma_segs, 1, &rsegs, BUS_DMA_NOWAIT) != 0) {
		aprint_normal(": iee_sbdio_attach: can't allocate %d bytes of "
		    "DMA memory\n", (int)IEE_SHMEM_MAX);
		return;
	}

	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_dma_segs, rsegs, IEE_SHMEM_MAX,
		(void **)sc->sc_shmem_addr, BUS_DMA_NOWAIT) != 0) {
		aprint_normal(": iee_sbdio_attach: can't map DMA memory\n");
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, rsegs);
		return;
	}

	if (bus_dmamap_create(sc->sc_dmat, IEE_SHMEM_MAX, rsegs,
		IEE_SHMEM_MAX, 0, BUS_DMA_NOWAIT, &sc->sc_shmem_map) != 0) {
		aprint_normal(": iee_sbdio_attach: can't create DMA map\n");
		bus_dmamem_unmap(sc->sc_dmat, sc->sc_shmem_addr, IEE_SHMEM_MAX);
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, rsegs);
		return;
	}

	if (bus_dmamap_load(sc->sc_dmat, sc->sc_shmem_map, sc->sc_shmem_addr,
		IEE_SHMEM_MAX, NULL, BUS_DMA_NOWAIT) != 0) {
		aprint_normal(": iee_sbdio_attach: can't load DMA map\n");
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_shmem_map);
		bus_dmamem_unmap(sc->sc_dmat, sc->sc_shmem_addr, IEE_SHMEM_MAX);
		bus_dmamem_free(sc->sc_dmat, &sc->sc_dma_segs, rsegs);
		return;
	}
	memset(sc->sc_shmem_addr, 0, IEE_SHMEM_MAX);

	mips_dcache_wbinv_range((vaddr_t)sc->sc_shmem_addr, IEE_SHMEM_MAX);
	sc->sc_shmem_addr = (void *)((vaddr_t)sc->sc_shmem_addr |
	    MIPS_KSEG1_START);

	/* Setup SYSBUS byte. TR2 specific? -uch */
	SC_SCP->scp_sysbus = IEE_SYSBUS_BE | IEE_SYSBUS_INT |
	    IEE_SYSBUS_LIEAR | IEE_SYSBUS_STD;

	sc->sc_iee_reset = iee_sbdio_reset;
	sc->sc_iee_cmd = iee_sbdio_cmd;
	sc->sc_mediachange = NULL;
	sc->sc_mediastatus = NULL;

	media[0] = IFM_ETHER | IFM_MANUAL;
	media[1] = IFM_ETHER | IFM_10_5;

	intr_establish(sa->sa_irq, iee_intr, self);
	(*platform.ether_addr)(eaddr);
	iee_attach(sc, eaddr, media, 2, IFM_ETHER | IFM_10_5);
}

int
iee_sbdio_cmd(struct iee_softc *sc, uint32_t cmd)
{
	int retry = 8;
	int n;

	SC_SCB->scb_cmd = cmd;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF, IEE_SCB_SZ,
	    BUS_DMASYNC_PREWRITE);

	iee_sbdio_channel_attention(sc);

	/* Wait for the cmd to finish */
	for (n = 0 ; n < retry; n++) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF,
		    IEE_SCB_SZ, BUS_DMASYNC_PREREAD);

		if (SC_SCB->scb_cmd == 0)
			break;
		delay(1);
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF, IEE_SCB_SZ,
	    BUS_DMASYNC_PREREAD);
	if (n < retry)
		return 0;

	printf("%s: command timeout. retry=%d\n", sc->sc_dev.dv_xname, retry);

	return -1;
}

int
iee_sbdio_reset(struct iee_softc *sc)
{
#define	IEE_ISCP_BUSSY 0x1
	int n, retry = 8;
	uint32_t cmd;

	/* Make sure the busy byte is set and the cache is flushed. */
	SC_ISCP->iscp_bussy = IEE_ISCP_BUSSY;
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCP_OFF, IEE_SCP_SZ
	    + IEE_ISCP_SZ + IEE_SCB_SZ, BUS_DMASYNC_PREWRITE);

	/* Setup the PORT Command with pointer to SCP. */
	cmd = IEE_PORT_SCP | IEE_PHYS_SHMEM(IEE_SCP_OFF);

	/* Initiate a Hardware reset. */
	printf("%s: reseting chip... ", sc->sc_dev.dv_xname);
	iee_sbdio_chip_reset(sc);

	/* Set SCP address to CU */
	iee_sbdio_set_scp(sc, cmd);

	/* Wait for the chip to initialize and read SCP and ISCP. */
	for (n = 0 ; n < retry; n++) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_ISCP_OFF,
		    IEE_ISCP_SZ, BUS_DMASYNC_PREREAD);
		if (SC_ISCP->iscp_bussy != IEE_ISCP_BUSSY) {
			break;
		}
		delay(100);
	}
	bus_dmamap_sync(sc->sc_dmat, sc->sc_shmem_map, IEE_SCB_OFF, IEE_SCB_SZ,
	    BUS_DMASYNC_PREREAD);

	if (n < retry) {
		/* ACK interrupts we may have caused */
		sc->sc_iee_cmd(sc, IEE_SCB_ACK);
		printf("done.\n");

		return 0;
	}
	printf("time out.\n");

	return -1;
}

static void
iee_sbdio_channel_attention(void *cookie)
{
	struct iee_sbdio_softc *sc = cookie;
	uint32_t dummy;

	/* Issue a Channel Attention */
	dummy = *sc->sc_port;
	dummy = *sc->sc_port;
}

static void
iee_sbdio_chip_reset(void *cookie)
{
	struct iee_sbdio_softc *sc = cookie;

	*sc->sc_port = 0;
	*sc->sc_port = 0;
	delay(10);
}

static void
iee_sbdio_set_scp(void *cookie, uint32_t cmd)
{
	struct iee_sbdio_softc *sc = cookie;
	cmd = (cmd << 16) | (cmd >> 16);

	*sc->sc_port = cmd;
	*sc->sc_port = cmd;

	/* Issue a Channel Attention to read SCP */
	iee_sbdio_channel_attention(cookie);
}
