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
 *	@(#)supradma.c
 *	$Id: supradma.c,v 1.3 1994/03/28 06:16:23 chopps Exp $
 */

/*
 * dummy Supra 5380 DMA driver
 */

#include "suprascsi.h"

#if NSUPRASCSI > 0

#include <sys/param.h>

#include <amiga/dev/device.h>
#include <amiga/dev/scivar.h>
#include <amiga/dev/scireg.h>

int supradma_pseudo = 0;	/* 0=none, 1=byte, 2=word */

#ifdef DEBUG
extern int sci_debug;
#define QUASEL
#endif
#define	HIST(h,w)

#ifdef QUASEL
#define QPRINTF(a) if (sci_debug > 1) printf a
#else
#define QPRINTF
#endif

extern int sci_data_wait;

static int dma_xfer_in __P((struct sci_softc *dev, int len,
    register u_char *buf, int phase));
static int dma_xfer_out __P((struct sci_softc *dev, int len,
    register u_char *buf, int phase));
static int dma_xfer_in2 __P((struct sci_softc *dev, int len,
    register u_short *buf, int phase));
static int dma_xfer_out2 __P((struct sci_softc *dev, int len,
    register u_short *buf, int phase));
static int supra_intr __P((struct sci_softc *dev));

void
supradmainit (dev)
	struct sci_softc *dev;
{
	if (supradma_pseudo == 2) {
		dev->dma_xfer_in = dma_xfer_in2;
		dev->dma_xfer_out = dma_xfer_out2;
	} else if (supradma_pseudo == 1) {
		dev->dma_xfer_in = dma_xfer_in;
		dev->dma_xfer_out = dma_xfer_out;
	}
	dev->dma_intr = supra_intr;
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
	u_char *obp = (u_char *) buf;
	volatile register u_char *sci_dma = dev->sci_idata;
	volatile register u_char *sci_csr = dev->sci_csr;

	QPRINTF(("supradma_in %d, csr=%02x\n", len, *dev->sci_bus_csr));

	*dev->sci_tcmd = phase;
	*dev->sci_icmd = 0;
	*dev->sci_mode = SCI_MODE_DMA;
	*dev->sci_irecv = 0;

	while (len >= 128) {
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug | 1)
					printf("supradma2_in fail: l%d i%x w%d\n",
					len, *dev->sci_bus_csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode = 0;
				return 0;
			}
		}

		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
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
				if (sci_debug | 1)
					printf("supradma1_in fail: l%d i%x w%d\n",
					len, *dev->sci_bus_csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode = 0;
				return 0;
			}
		}

		*buf++ = *sci_dma;
		len--;
	}

	QPRINTF(("supradma_in {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	  obp[6], obp[7], obp[8], obp[9]));

	HIST(ixin_wait, wait)
	*dev->sci_mode = 0;
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
	volatile register u_char *sci_dma = dev->sci_data;
	volatile register u_char *sci_csr = dev->sci_csr;

	QPRINTF(("supradma_out %d, csr=%02x\n", len, *dev->sci_bus_csr));

	QPRINTF(("supradma_out {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	 len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5],
	 buf[6], buf[7], buf[8], buf[9]));

	*dev->sci_tcmd = phase;
	*dev->sci_mode = SCI_MODE_DMA;
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
					printf("supradma_out fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode = 0;
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
	*dev->sci_mode = 0;
	*dev->sci_icmd = 0;
	return 0;
}


static int
dma_xfer_in2 (dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_short *buf;
	int phase;
{
	int wait = sci_data_wait;
	u_char csr;
	u_char *obp = (u_char *) buf;
	volatile register u_short *sci_dma = (u_short *)(dev->sci_idata + 0x10);
	volatile register u_char *sci_csr = dev->sci_csr + 0x10;

	QPRINTF(("supradma_in2 %d, csr=%02x\n", len, *dev->sci_bus_csr));

	*dev->sci_tcmd = phase;
	*dev->sci_mode = SCI_MODE_DMA;
	*dev->sci_icmd = 0;
	*(dev->sci_irecv + 16) = 0;
	while (len >= 128) {
#if 0
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug | 1)
					printf("supradma2_in2 fail: l%d i%x w%d\n",
					len, *dev->sci_bus_csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}
#else
		while (!(*sci_csr & SCI_CSR_DREQ))
			;
#endif

		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		*buf++ = *sci_dma; *buf++ = *sci_dma;
		len -= 128;
	}
	while (len > 0) {
#if 0
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug | 1)
					printf("supradma1_in2 fail: l%d i%x w%d\n",
					len, *dev->sci_bus_csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode &= ~SCI_MODE_DMA;
				return 0;
			}
		}
#else
		while (!(*sci_csr * SCI_CSR_DREQ))
			;
#endif

		*buf++ = *sci_dma;
		len -= 2;
	}

	QPRINTF(("supradma_in2 {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	  len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	  obp[6], obp[7], obp[8], obp[9]));

	HIST(ixin_wait, wait)
	*dev->sci_irecv = 0;
	*dev->sci_mode = 0;
	return 0;
}

static int
dma_xfer_out2 (dev, len, buf, phase)
	struct sci_softc *dev;
	int len;
	register u_short *buf;
	int phase;
{
	int wait = sci_data_wait;
	u_char csr;
	u_char *obp = (u_char *) buf;
	volatile register u_short *sci_dma = (ushort *)(dev->sci_data + 0x10);
	volatile register u_char *sci_bus_csr = dev->sci_bus_csr;

	QPRINTF(("supradma_out2 %d, csr=%02x\n", len, *dev->sci_bus_csr));

	QPRINTF(("supradma_out2 {%d} %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
  	 len, obp[0], obp[1], obp[2], obp[3], obp[4], obp[5],
	 obp[6], obp[7], obp[8], obp[9]));

	*dev->sci_tcmd = phase;
	*dev->sci_mode = SCI_MODE_DMA;
	*dev->sci_icmd = SCI_ICMD_DATA;
	*dev->sci_dma_send = 0;
	while (len > 0) {
#if 0
		wait = sci_data_wait;
		while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) !=
		  (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) {
			if (!(*sci_csr & SCI_CSR_PHASE_MATCH)
			  || !(*dev->sci_bus_csr & SCI_BUS_BSY)
			  || --wait < 0) {
#ifdef DEBUG
				if (sci_debug)
					printf("supradma_out2 fail: l%d i%x w%d\n",
					len, csr, wait);
#endif
				HIST(ixin_wait, wait)
				*dev->sci_mode = 0;
				return 0;
			}
		}
#else
		*dev->sci_mode = 0;
		*dev->sci_icmd &= ~SCI_ICMD_ACK;
		while (!(*sci_bus_csr & SCI_BUS_REQ))
			;
		*dev->sci_mode = SCI_MODE_DMA;
		*dev->sci_dma_send = 0;
#endif

		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		*sci_dma = *buf++; *sci_dma = *buf++;
		if (*(sci_bus_csr + 0x10) & SCI_BUS_REQ)
			;
		len -= 64;
	}

#if 0
	wait = sci_data_wait;
	while ((*sci_csr & (SCI_CSR_DREQ|SCI_CSR_PHASE_MATCH)) ==
	  SCI_CSR_PHASE_MATCH && --wait);
#endif


	HIST(ixin_wait, wait)
	*dev->sci_irecv = 0;
	*dev->sci_icmd &= ~SCI_ICMD_ACK;
	*dev->sci_mode = 0;
	*dev->sci_icmd = 0;
	return 0;
}

static int
supra_intr (dev)
	struct sci_softc *dev;
{
	if (*(dev->sci_csr + 0x10) & SCI_CSR_INT) {
		char dummy;
#if 0
printf ("supra_intr\n");
#endif
		dummy = *(dev->sci_iack + 0x10);
		return (1);
	}
	return (0);
}
#endif
