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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	$Id: csa12gdma.c,v 1.2 1994/03/25 16:32:27 chopps Exp $
 */

/*
 * dummy CSA 12 Gauge 5380 DMA driver
 */

#include "csa12gscsi.h"

#if NCSA12GSCSI > 0

#include <sys/param.h>

#include <amiga/dev/device.h>
#include <amiga/dev/scivar.h>
#include <amiga/dev/scireg.h>

#define QPRINTF
#ifdef DEBUG
extern int sci_debug;
#endif
#define	HIST(h,w)

extern int sci_data_wait;

static int dma_xfer_in __P((struct sci_softc *dev, int len,
    register u_char *buf, int phase));
static int dma_xfer_out __P((struct sci_softc *dev, int len,
    register u_char *buf, int phase));

void
csa12gdmainit (dev)
	struct sci_softc *dev;
{
	dev->dma_xfer_in = dma_xfer_in;
	dev->dma_xfer_out = dma_xfer_out;
}

static int
dma_xfer_in (dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_char *buf;
	int phase;
{
	int wait = sci_data_wait;
	u_char csr;
	u_char *obp = buf;
	volatile register u_char *sci_dma = dev->sci_data + 0x80;
	volatile register u_char *sci_csr = dev->sci_csr;
	volatile register u_char *sci_icmd = dev->sci_icmd;

	QPRINTF(("csa12gdma_in %d, csr=%02x\n", len, *dev->sci_bus_csr));

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
					printf("csa12g_in fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}

		*buf++ = *sci_dma;
		len--;
	}

	QPRINTF(("csa12gdma_in {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	  obp[6], obp[7], obp[8], obp[9]));

	/* this leaves with one csr to be read */
	HIST(ixin_wait, wait)
	*dev->sci_mode &= ~SCI_MODE_DMA;
	return 0;
}

static int
dma_xfer_out (dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_char *buf;
	int phase;
{
	int wait = sci_data_wait;
	u_char csr;
	u_char *obp = buf;
	volatile register u_char *sci_dma = dev->sci_data + 0x80;
	volatile register u_char *sci_csr = dev->sci_csr;
	volatile register u_char *sci_icmd = dev->sci_icmd;

	QPRINTF(("csa12gdma_out %d, csr=%02x\n", len, *dev->sci_bus_csr));

	QPRINTF(("csa12gdma_out {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
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
					printf("csa12gdma_out fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				HIST(ixin_wait, wait)
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


	HIST(ixin_wait, wait)
	*dev->sci_mode &= ~SCI_MODE_DMA;
	return 0;
}
#endif
