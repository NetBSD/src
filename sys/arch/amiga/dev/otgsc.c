/*	$NetBSD: otgsc.c,v 1.30 2004/03/28 18:59:39 mhitch Exp $ */

/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)csa12gdma.c
 */

/*
 * Copyright (c) 1994 Michael L. Hitch
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
 *
 *	@(#)csa12gdma.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: otgsc.c,v 1.30 2004/03/28 18:59:39 mhitch Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <dev/scsipi/scsi_all.h>
#include <dev/scsipi/scsipi_all.h>
#include <dev/scsipi/scsiconf.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/scireg.h>
#include <amiga/dev/scivar.h>
#include <amiga/dev/zbusvar.h>

void otgscattach(struct device *, struct device *, void *);
int otgscmatch(struct device *, struct cfdata *, void *);

int otgsc_dma_xfer_in(struct sci_softc *dev, int len,
    register u_char *buf, int phase);
int otgsc_dma_xfer_out(struct sci_softc *dev, int len,
    register u_char *buf, int phase);
int otgsc_intr(void *);


#ifdef DEBUG
extern int sci_debug;
#define QPRINTF(a) if (sci_debug > 1) printf a
#else
#define QPRINTF(a)
#endif

extern int sci_data_wait;

CFATTACH_DECL(otgsc, sizeof(struct sci_softc),
    otgscmatch, otgscattach, NULL, NULL);

/*
 * if we are my Hacker's SCSI board we are here.
 */
int
otgscmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	struct zbus_args *zap;

	zap = auxp;

	/*
	 * Check manufacturer and product id.
	 */
	if (zap->manid == 1058 && zap->prodid == 21)
		return(1);
	else
		return(0);
}

void
otgscattach(struct device *pdp, struct device *dp, void *auxp)
{
	volatile u_char *rp;
	struct sci_softc *sc = (struct sci_softc *)dp;
	struct zbus_args *zap;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	printf("\n");

	zap = auxp;

	sc = (struct sci_softc *)dp;
	rp = (u_char *)zap->va + 0x2000;
	sc->sci_data = rp;
	sc->sci_odata = rp;
	sc->sci_icmd = rp + 0x10;
	sc->sci_mode = rp + 0x20;
	sc->sci_tcmd = rp + 0x30;
	sc->sci_bus_csr = rp + 0x40;
	sc->sci_sel_enb = rp + 0x40;
	sc->sci_csr = rp + 0x50;
	sc->sci_dma_send = rp + 0x50;
	sc->sci_idata = rp + 0x60;
	sc->sci_trecv = rp + 0x60;
	sc->sci_iack = rp + 0x70;
	sc->sci_irecv = rp + 0x70;

	sc->dma_xfer_in = otgsc_dma_xfer_in;
	sc->dma_xfer_out = otgsc_dma_xfer_out;

	sc->sc_isr.isr_intr = otgsc_intr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr(&sc->sc_isr);

	scireset(sc);

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = &sc->sc_dev;
	adapt->adapt_nchannels = 1;
	adapt->adapt_openings = 7;
	adapt->adapt_max_periph = 1;
	adapt->adapt_request = sci_scsipi_request;
	adapt->adapt_minphys = sci_minphys;

	/*
	 * Fill in the scsipi_channel.
	 */
	memset(chan, 0, sizeof(*chan));
	chan->chan_adapter = adapt;
	chan->chan_bustype = &scsi_bustype;
	chan->chan_channel = 0;
	chan->chan_ntargets = 8;
	chan->chan_nluns = 8;
	chan->chan_id = 7;

	/*
	 * attach all scsi units on us
	 */
	config_found(dp, chan, scsiprint);
}

int
otgsc_dma_xfer_in(struct sci_softc *dev, int len, register u_char *buf,
                  int phase)
{
	int wait = sci_data_wait;
	volatile register u_char *sci_dma = dev->sci_data + 0x100;
	volatile register u_char *sci_csr = dev->sci_csr;
#ifdef DEBUG
	u_char *obp = buf;
#endif

	QPRINTF(("otgsc_dma_in %d, csr=%02x\n", len, *dev->sci_bus_csr));

	*dev->sci_tcmd = phase;
	*dev->sci_mode |= SCI_MODE_DMA;
	*dev->sci_icmd = 0;
	*dev->sci_irecv = 0;
	while (len > 0) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("otgsc_dma_in fail: l%d i%x w%d\n",
					len, *dev->sci_bus_csr, wait);
#endif
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}

		*buf++ = *sci_dma;
		len--;
	}

	QPRINTF(("otgsc_dma_in {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	  obp[6], obp[7], obp[8], obp[9]));

	*dev->sci_mode &= ~SCI_MODE_DMA;
	return 0;
}

int
otgsc_dma_xfer_out(struct sci_softc *dev, int len, register u_char *buf,
                   int phase)
{
	int wait = sci_data_wait;
	volatile register u_char *sci_dma = dev->sci_data + 0x100;
	volatile register u_char *sci_csr = dev->sci_csr;

	QPRINTF(("otgsc_dma_out %d, csr=%02x\n", len, *dev->sci_bus_csr));

	QPRINTF(("otgsc_dma_out {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	 len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
	 buf[6], buf[7], buf[8], buf[9]));

	*dev->sci_tcmd = phase;
	*dev->sci_mode |= SCI_MODE_DMA;
	*dev->sci_icmd = SCI_ICMD_DATA;
	*dev->sci_dma_send = 0;
	while (len > 0) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("otgsc_dma_out fail: l%d i%x w%d\n",
					len, *dev->sci_bus_csr, wait);
#endif
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}

		*sci_dma = *buf++;
		len--;
	}

	wait = sci_data_wait;
	while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) ==
	  SCI_CSR_PHASE_MATCH && --wait);


	*dev->sci_mode &= ~SCI_MODE_DMA;
	return 0;
}

int
otgsc_intr(void *arg)
{
	struct sci_softc *dev = arg;
	u_char stat;

	if ((*dev->sci_csr & SCI_CSR_INT) == 0)
		return (1);
	stat = *dev->sci_iack;
	*dev->sci_mode = 0;
	return (1);
}
