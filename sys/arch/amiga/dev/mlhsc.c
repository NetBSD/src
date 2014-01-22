/*	$NetBSD: mlhsc.c,v 1.32 2014/01/22 00:25:16 christos Exp $ */

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
 *	@(#)dma.c
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
 *	@(#)dma.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mlhsc.c,v 1.32 2014/01/22 00:25:16 christos Exp $");

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

void mlhscattach(device_t, device_t, void *);
int mlhscmatch(device_t, cfdata_t, void *);

int mlhsc_dma_xfer_in(struct sci_softc *dev, int len,
    register u_char *buf, int phase);
int mlhsc_dma_xfer_out(struct sci_softc *dev, int len,
    register u_char *buf, int phase);

#ifdef DEBUG
extern int sci_debug;
#define QPRINTF(a) if (sci_debug > 1) printf a
#else
#define QPRINTF(a)
#endif

extern int sci_data_wait;

CFATTACH_DECL_NEW(mlhsc, sizeof(struct sci_softc),
    mlhscmatch, mlhscattach, NULL, NULL);

/*
 * if we are my Hacker's SCSI board we are here.
 */
int
mlhscmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = aux;

	/*
	 * Check manufacturer and product id.
	 */
	if (zap->manid == 2011 && zap->prodid == 1)
		return(1);
	else
		return(0);
}

void
mlhscattach(device_t parent, device_t self, void *aux)
{
	volatile u_char *rp;
	struct sci_softc *sc = device_private(self);
	struct zbus_args *zap;
	struct scsipi_adapter *adapt = &sc->sc_adapter;
	struct scsipi_channel *chan = &sc->sc_channel;

	sc->sc_dev = self;

	printf("\n");

	zap = aux;

	rp = zap->va;
	sc->sci_data = rp + 1;
	sc->sci_odata = rp + 1;
	sc->sci_icmd = rp + 3;
	sc->sci_mode = rp + 5;
	sc->sci_tcmd = rp + 7;
	sc->sci_bus_csr = rp + 9;
	sc->sci_sel_enb = rp + 9;
	sc->sci_csr = rp + 11;
	sc->sci_dma_send = rp + 11;
	sc->sci_idata = rp + 13;
	sc->sci_trecv = rp + 13;
	sc->sci_iack = rp + 15;
	sc->sci_irecv = rp + 15;

	sc->dma_xfer_in = mlhsc_dma_xfer_in;
	sc->dma_xfer_out = mlhsc_dma_xfer_out;

	scireset(sc);

	/*
	 * Fill in the scsipi_adapter.
	 */
	memset(adapt, 0, sizeof(*adapt));
	adapt->adapt_dev = self;
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
	config_found(self, chan, scsiprint);
}

int
mlhsc_dma_xfer_in(struct sci_softc *dev, int len, register u_char *buf,
                  int phase)
{
	int wait = sci_data_wait;
	u_char csr;
	volatile register u_char *sci_dma = dev->sci_data + 16;
	volatile register u_char *sci_csr = dev->sci_csr;
#ifdef DEBUG
	u_char *obp = buf;
#endif

	csr = *dev->sci_bus_csr;
	__USE(csr);

	QPRINTF(("mlhdma_in %d, csr=%02x\n", len, csr));

	*dev->sci_tcmd = phase;
	*dev->sci_mode |= SCI_MODE_DMA;
	*dev->sci_icmd = 0;
	*dev->sci_irecv = 0;
	while (len > 128) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("mlhdma_in fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}

#define R1	(*buf++ = *sci_dma)
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		R1; R1; R1; R1; R1; R1; R1; R1;
		len -= 128;
	}
	while (len > 0) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("mlhdma_in fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}

		*buf++ = *sci_dma;
		len--;
	}

	QPRINTF(("mlhdma_in {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	  obp[6], obp[7], obp[8], obp[9]));

	*dev->sci_mode &= ~SCI_MODE_DMA;
	return 0;
}

int
mlhsc_dma_xfer_out(struct sci_softc *dev, int len, register u_char *buf,
                   int phase)
{
	int wait = sci_data_wait;
	u_char csr;
	volatile register u_char *sci_dma = dev->sci_data + 16;
	volatile register u_char *sci_csr = dev->sci_csr;

	csr = *dev->sci_bus_csr;
	__USE(csr);

	QPRINTF(("mlhdma_xfer %d, csr=%02x\n", len, csr));

	QPRINTF(("mlhgdma_out {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	 len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
	 buf[6], buf[7], buf[8], buf[9]));

	*dev->sci_tcmd = phase;
	*dev->sci_mode |= SCI_MODE_DMA;
	*dev->sci_icmd = SCI_ICMD_DATA;
	*dev->sci_dma_send = 0;
	while (len > 64) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("mlhdma_out fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}

#define W1	(*sci_dma = *buf++)
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		W1; W1; W1; W1; W1; W1; W1; W1;
		len -= 64;
	}
	while (len > 0) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("mlhdma_out fail: l%d i%x w%d\n",
					len, csr, wait);
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
