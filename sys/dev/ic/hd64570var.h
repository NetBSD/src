/*	$NetBSD: hd64570var.h,v 1.2 1998/10/28 16:26:01 kleink Exp $	*/

/*
 * Copyright (c) 1998 Vixie Enterprises
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Vixie Enterprises nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY VIXIE ENTERPRISES AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL VIXIE ENTERPRISES OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for Vixie Enterprises by Michael Graff
 * <explorer@flame.org>.  To learn more about Vixie Enterprises, see
 * ``http://www.vix.com''.
 */

#ifndef _DEV_IC_HD64570VAR_H_
#define _DEV_IC_HD64570VAR_H_

#include "bpfilter.h"

#define SCA_USE_FASTQ		/* use a split queue, one for fast traffic */

struct sca_softc;
typedef struct sca_port sca_port_t;
typedef struct sca_desc sca_desc_t;

/*
 * device DMA descriptor
 */
struct sca_desc {
	u_int16_t	cp;	/* chain pointer */
	u_int16_t	bp;	/* buffer pointer (low bits) */
	u_int8_t	bpb;	/* buffer pointer (high bits) */
	u_int8_t	unused0;
	u_int16_t	len;	/* total length */
	u_int8_t	stat;	/* status */
	u_int8_t	unused1;
};
#define SCA_DESC_EOT            0x01
#define SCA_DESC_CRC            0x04
#define SCA_DESC_OVRN           0x08
#define SCA_DESC_RESD           0x10
#define SCA_DESC_ABORT          0x20
#define SCA_DESC_SHRTFRM        0x40
#define SCA_DESC_EOM            0x80
#define SCA_DESC_ERRORS         0x7C

/*
 * softc structure for each port
 */
struct sca_port {
	u_int msci_off;		/* offset for msci address for this port */
	u_int dmac_off;		/* offset of dmac address for this port */

	u_int sp_port;

	/*
	 * CISCO keepalive stuff
	 */
	u_int32_t	cka_lasttx;
	u_int32_t	cka_lastrx;

	/*
	 * start of each important bit of information for transmit and
	 * receive buffers.
	 */
	u_int32_t txdesc_p;
	sca_desc_t *txdesc;
	u_int32_t txbuf_p;
	u_int8_t *txbuf;
	volatile u_int txcur;		/* last descriptor in chain */
	volatile u_int txinuse;		/* descriptors in use */
	volatile u_int txstart;		/* start descriptor */

	u_int32_t rxdesc_p;
	sca_desc_t *rxdesc;
	u_int32_t rxbuf_p;
	u_int8_t *rxbuf;
	u_int rxstart;			/* index of first descriptor */
	u_int rxend;			/* index of last descriptor */

	struct ifnet sp_if;		/* the network information */
	struct ifqueue linkq;		/* link-level packets are high prio */
#ifdef SCA_USE_FASTQ
	struct ifqueue fastq;		/* interactive packets */
#endif

#if NBPFILTER > 0
	caddr_t	sp_bpf;			/* hook for BPF */
#endif

	struct sca_softc *sca;		/* pointer to parent */
};

/*
 * softc structure for the chip itself
 */
struct sca_softc {
	struct device *parent;		/* our parent device, or NULL */
	int sc_numports;		/* number of ports present */

	/*
	 * a callback into the parent, since the SCA chip has no control
	 * over DTR, we have to make a callback into the parent, which
	 * might know about DTR.
	 *
	 * If the function pointer is NULL, no callback is specified.
	 */
	void (*dtr_callback) __P((void *aux, int port, int state));
	void *dtr_aux;

	sca_port_t sc_ports[2];
	bus_space_handle_t sc_ioh;
	bus_space_tag_t sc_iot;

	bus_dma_tag_t sc_dmat;		/* bus dma tag */
	bus_dmamap_t sc_dmam;		/* bus dma map */
	bus_dma_segment_t sc_seg;	/* bus dma segment allocated */
	caddr_t sc_dma_addr;		/* kva address of segment */
	u_long sc_allocsize;		/* size of region */
};


int	sca_init(struct sca_softc *, u_int);
void	sca_port_attach(struct sca_softc *, u_int);
int	sca_hardintr(struct sca_softc *);
void	sca_shutdown(struct sca_softc *);

#endif /* _DEV_IC_HD64570VAR_H_ */
