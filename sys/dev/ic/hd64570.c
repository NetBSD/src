/*	$NetBSD: hd64570.c,v 1.6 1999/03/19 22:43:11 erh Exp $	*/

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

/*
 * TODO:
 *
 *	o  teach the receive logic about errors, and about long frames that
 *         span more than one input buffer.  (Right now, receive/transmit is
 *	   limited to one descriptor's buffer space, which is MTU + 4 bytes.
 *	   This is currently 1504, which is large enough to hold the HDLC
 *	   header and the packet itself.  Packets which are too long are
 *	   silently dropped on transmit and silently dropped on receive.
 *	o  write code to handle the msci interrupts, needed only for CD
 *	   and CTS changes.
 *	o  consider switching back to a "queue tx with DMA active" model which
 *	   should help sustain outgoing traffic
 *	o  through clever use of bus_dma*() functions, it should be possible
 *	   to map the mbuf's data area directly into a descriptor transmit
 *	   buffer, removing the need to allocate extra memory.  If, however,
 *	   we run out of descriptors for this, we will need to then allocate
 *	   one large mbuf, copy the fragmented chain into it, and put it onto
 *	   a single descriptor.
 *	o  use bus_dmamap_sync() with the right offset and lengths, rather
 *	   than cheating and always sync'ing the whole region.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ic/hd64570reg.h>
#include <dev/ic/hd64570var.h>

#define SCA_DEBUG_RX		0x0001
#define SCA_DEBUG_TX		0x0002
#define SCA_DEBUG_CISCO		0x0004
#define SCA_DEBUG_DMA		0x0008
#define SCA_DEBUG_RXPKT		0x0010
#define SCA_DEBUG_TXPKT		0x0020
#define SCA_DEBUG_INTR		0x0040

#if 0
#define SCA_DEBUG_LEVEL	( SCA_DEBUG_TX )
#else
#define SCA_DEBUG_LEVEL 0
#endif

u_int32_t sca_debug = SCA_DEBUG_LEVEL;

#if SCA_DEBUG_LEVEL > 0
#define SCA_DPRINTF(l, x) do { \
	if ((l) & sca_debug) \
		printf x;\
	} while (0)
#else
#define SCA_DPRINTF(l, x)
#endif

#define SCA_MTU		1500	/* hard coded */

/*
 * buffers per tx and rx channels, per port, and the size of each.
 * Don't use these constants directly, as they are really only hints.
 * Use the calculated values stored in struct sca_softc instead.
 *
 * Each must be at least 2, receive would be better at around 20 or so.
 *
 * XXX Due to a damned near impossible to track down bug, transmit buffers
 * MUST be 2, no more, no less.
 */
#ifndef SCA_NtxBUFS
#define SCA_NtxBUFS	2
#endif
#ifndef SCA_NrxBUFS
#define SCA_NrxBUFS	20
#endif
#ifndef SCA_BSIZE
#define SCA_BSIZE	(SCA_MTU + 4)	/* room for HDLC as well */
#endif

#if 0
#define SCA_USE_FASTQ		/* use a split queue, one for fast traffic */
#endif

static inline void sca_write_1(struct sca_softc *, u_int, u_int8_t);
static inline void sca_write_2(struct sca_softc *, u_int, u_int16_t);
static inline u_int8_t sca_read_1(struct sca_softc *, u_int);
static inline u_int16_t sca_read_2(struct sca_softc *, u_int);

static inline void msci_write_1(sca_port_t *, u_int, u_int8_t);
static inline u_int8_t msci_read_1(sca_port_t *, u_int);

static inline void dmac_write_1(sca_port_t *, u_int, u_int8_t);
static inline void dmac_write_2(sca_port_t *, u_int, u_int16_t);
static inline u_int8_t dmac_read_1(sca_port_t *, u_int);
static inline u_int16_t dmac_read_2(sca_port_t *, u_int);

static	int sca_alloc_dma(struct sca_softc *);
static	void sca_setup_dma_memory(struct sca_softc *);
static	void sca_msci_init(struct sca_softc *, sca_port_t *);
static	void sca_dmac_init(struct sca_softc *, sca_port_t *);
static void sca_dmac_rxinit(sca_port_t *);

static	int sca_dmac_intr(sca_port_t *, u_int8_t);
static	int sca_msci_intr(struct sca_softc *, u_int8_t);

static	void sca_get_packets(sca_port_t *);
static	void sca_frame_process(sca_port_t *, sca_desc_t *, u_int8_t *);
static	int sca_frame_avail(sca_port_t *, int *);
static	void sca_frame_skip(sca_port_t *, int);

static	void sca_port_starttx(sca_port_t *);

static	void sca_port_up(sca_port_t *);
static	void sca_port_down(sca_port_t *);

static	int sca_output __P((struct ifnet *, struct mbuf *, struct sockaddr *,
			    struct rtentry *));
static	int sca_ioctl __P((struct ifnet *, u_long, caddr_t));
static	void sca_start __P((struct ifnet *));
static	void sca_watchdog __P((struct ifnet *));

static struct mbuf *sca_mbuf_alloc(caddr_t, u_int);

#if SCA_DEBUG_LEVEL > 0
static	void sca_frame_print(sca_port_t *, sca_desc_t *, u_int8_t *);
#endif

static inline void
sca_write_1(struct sca_softc *sc, u_int reg, u_int8_t val)
{
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, SCADDR(reg), val);
}

static inline void
sca_write_2(struct sca_softc *sc, u_int reg, u_int16_t val)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SCADDR(reg), val);
}

static inline u_int8_t
sca_read_1(struct sca_softc *sc, u_int reg)
{
	return bus_space_read_1(sc->sc_iot, sc->sc_ioh, SCADDR(reg));
}

static inline u_int16_t
sca_read_2(struct sca_softc *sc, u_int reg)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, SCADDR(reg));
}

static inline void
msci_write_1(sca_port_t *scp, u_int reg, u_int8_t val)
{
	sca_write_1(scp->sca, scp->msci_off + reg, val);
}

static inline u_int8_t
msci_read_1(sca_port_t *scp, u_int reg)
{
	return sca_read_1(scp->sca, scp->msci_off + reg);
}

static inline void
dmac_write_1(sca_port_t *scp, u_int reg, u_int8_t val)
{
	sca_write_1(scp->sca, scp->dmac_off + reg, val);
}

static inline void
dmac_write_2(sca_port_t *scp, u_int reg, u_int16_t val)
{
	sca_write_2(scp->sca, scp->dmac_off + reg, val);
}

static inline u_int8_t
dmac_read_1(sca_port_t *scp, u_int reg)
{
	return sca_read_1(scp->sca, scp->dmac_off + reg);
}

static inline u_int16_t
dmac_read_2(sca_port_t *scp, u_int reg)
{
	return sca_read_2(scp->sca, scp->dmac_off + reg);
}

int
sca_init(struct sca_softc *sc, u_int nports)
{
	/*
	 * Do a little sanity check:  check number of ports.
	 */
	if (nports < 1 || nports > 2)
		return 1;

	/*
	 * remember the details
	 */
	sc->sc_numports = nports;

	/*
	 * allocate the memory and chop it into bits.
	 */
	if (sca_alloc_dma(sc) != 0)
		return 1;
	sca_setup_dma_memory(sc);

	/*
	 * disable DMA and MSCI interrupts
	 */
	sca_write_1(sc, SCA_DMER, 0);
	sca_write_1(sc, SCA_IER0, 0);
	sca_write_1(sc, SCA_IER1, 0);
	sca_write_1(sc, SCA_IER2, 0);

	/*
	 * configure interrupt system
	 */
	sca_write_1(sc, SCA_ITCR, 0);	/* use ivr, no int ack */
	sca_write_1(sc, SCA_IVR, 0x40);
	sca_write_1(sc, SCA_IMVR, 0x40);

	/*
	 * set wait control register to zero wait states
	 */
	sca_write_1(sc, SCA_PABR0, 0);
	sca_write_1(sc, SCA_PABR1, 0);
	sca_write_1(sc, SCA_WCRL, 0);
	sca_write_1(sc, SCA_WCRM, 0);
	sca_write_1(sc, SCA_WCRH, 0);

	/*
	 * disable DMA and reset status
	 */
	sca_write_1(sc, SCA_PCR, SCA_PCR_PR2);

	/*
	 * disable transmit DMA for all channels
	 */
	sca_write_1(sc, SCA_DSR0 + SCA_DMAC_OFF_0, 0);
	sca_write_1(sc, SCA_DCR0 + SCA_DMAC_OFF_0, SCA_DCR_ABRT);
	sca_write_1(sc, SCA_DSR1 + SCA_DMAC_OFF_0, 0);
	sca_write_1(sc, SCA_DCR1 + SCA_DMAC_OFF_0, SCA_DCR_ABRT);
	sca_write_1(sc, SCA_DSR0 + SCA_DMAC_OFF_1, 0);
	sca_write_1(sc, SCA_DCR0 + SCA_DMAC_OFF_1, SCA_DCR_ABRT);
	sca_write_1(sc, SCA_DSR1 + SCA_DMAC_OFF_1, 0);
	sca_write_1(sc, SCA_DCR1 + SCA_DMAC_OFF_1, SCA_DCR_ABRT);

	/*
	 * enable DMA based on channel enable flags for each channel
	 */
	sca_write_1(sc, SCA_DMER, SCA_DMER_EN);

	/*
	 * Should check to see if the chip is responding, but for now
	 * assume it is.
	 */
	return 0;
}

/*
 * initialize the port and attach it to the networking layer
 */
void
sca_port_attach(struct sca_softc *sc, u_int port)
{
	sca_port_t *scp = &sc->sc_ports[port];
	struct ifnet *ifp;
	static u_int ntwo_unit = 0;

	scp->sca = sc;  /* point back to the parent */

	scp->sp_port = port;

	if (port == 0) {
		scp->msci_off = SCA_MSCI_OFF_0;
		scp->dmac_off = SCA_DMAC_OFF_0;
		if(sc->parent != NULL)
			ntwo_unit=sc->parent->dv_unit * 2 + 0;
		else
			ntwo_unit = 0;	/* XXX */
	} else {
		scp->msci_off = SCA_MSCI_OFF_1;
		scp->dmac_off = SCA_DMAC_OFF_1;
		if(sc->parent != NULL)
			ntwo_unit=sc->parent->dv_unit * 2 + 1;
		else
			ntwo_unit = 1;	/* XXX */
	}

	sca_msci_init(sc, scp);
	sca_dmac_init(sc, scp);

	/*
	 * attach to the network layer
	 */
	ifp = &scp->sp_if;
	sprintf(ifp->if_xname, "ntwo%d", ntwo_unit);
	ifp->if_softc = scp;
	ifp->if_mtu = SCA_MTU;
	ifp->if_flags = IFF_POINTOPOINT | IFF_MULTICAST;
	ifp->if_type = IFT_OTHER;  /* Should be HDLC, but... */
	ifp->if_hdrlen = HDLC_HDRLEN;
	ifp->if_ioctl = sca_ioctl;
	ifp->if_output = sca_output;
	ifp->if_watchdog = sca_watchdog;
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;
	scp->linkq.ifq_maxlen = 5; /* if we exceed this we are hosed already */
#ifdef SCA_USE_FASTQ
	scp->fastq.ifq_maxlen = IFQ_MAXLEN;
#endif
	if_attach(ifp);

#if NBPFILTER > 0
	bpfattach(&scp->sp_bpf, ifp, DLT_HDLC, HDLC_HDRLEN);
#endif

	if (sc->parent == NULL)
		printf("%s: port %d\n", ifp->if_xname, port);
	else
		printf("%s at %s port %d\n",
		       ifp->if_xname, sc->parent->dv_xname, port);

	/*
	 * reset the last seen times on the cisco keepalive protocol
	 */
	scp->cka_lasttx = time.tv_usec;
	scp->cka_lastrx = 0;
}

/*
 * initialize the port's MSCI
 */
static void
sca_msci_init(struct sca_softc *sc, sca_port_t *scp)
{
	msci_write_1(scp, SCA_CMD0, SCA_CMD_RESET);
	msci_write_1(scp, SCA_MD00,
		     (  SCA_MD0_CRC_1
		      | SCA_MD0_CRC_CCITT
		      | SCA_MD0_CRC_ENABLE
		      | SCA_MD0_MODE_HDLC));
	msci_write_1(scp, SCA_MD10, SCA_MD1_NOADDRCHK);
	msci_write_1(scp, SCA_MD20,
		     (SCA_MD2_DUPLEX | SCA_MD2_NRZ));

	/*
	 * reset the port (and lower RTS)
	 */
	msci_write_1(scp, SCA_CMD0, SCA_CMD_RXRESET);
	msci_write_1(scp, SCA_CTL0,
		     (SCA_CTL_IDLPAT | SCA_CTL_UDRNC | SCA_CTL_RTS));
	msci_write_1(scp, SCA_CMD0, SCA_CMD_TXRESET);

	/*
	 * select the RX clock as the TX clock, and set for external
	 * clock source.
	 */
	msci_write_1(scp, SCA_RXS0, 0);
	msci_write_1(scp, SCA_TXS0, 0);

	/*
	 * XXX don't pay attention to CTS or CD changes right now.  I can't
	 * simulate one, and the transmitter will try to transmit even if
	 * CD isn't there anyway, so nothing bad SHOULD happen.
	 */
	msci_write_1(scp, SCA_IE00, 0);
	msci_write_1(scp, SCA_IE10, 0); /* 0x0c == CD and CTS changes only */
	msci_write_1(scp, SCA_IE20, 0);
	msci_write_1(scp, SCA_FIE0, 0);

	msci_write_1(scp, SCA_SA00, 0);
	msci_write_1(scp, SCA_SA10, 0);

	msci_write_1(scp, SCA_IDL0, 0x7e);

	msci_write_1(scp, SCA_RRC0, 0x0e);
	msci_write_1(scp, SCA_TRC00, 0x10);
	msci_write_1(scp, SCA_TRC10, 0x1f);
}

/*
 * Take the memory for the port and construct two circular linked lists of
 * descriptors (one tx, one rx) and set the pointers in these descriptors
 * to point to the buffer space for this port.
 */
static void
sca_dmac_init(struct sca_softc *sc, sca_port_t *scp)
{
	sca_desc_t *desc;
	u_int32_t desc_p;
	u_int32_t buf_p;
	int i;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam,
			0, sc->sc_allocsize, BUS_DMASYNC_PREWRITE);

	desc = scp->txdesc;
	desc_p = scp->txdesc_p;
	buf_p = scp->txbuf_p;
	scp->txcur = 0;
	scp->txinuse = 0;

	for (i = 0 ; i < SCA_NtxBUFS ; i++) {
		/*
		 * desc_p points to the physcial address of the NEXT desc
		 */
		desc_p += sizeof(sca_desc_t);

		desc->cp = desc_p & 0x0000ffff;
		desc->bp = buf_p & 0x0000ffff;
		desc->bpb = (buf_p & 0x00ff0000) >> 16;
		desc->len = SCA_BSIZE;
		desc->stat = 0;

		desc++;  /* point to the next descriptor */
		buf_p += SCA_BSIZE;
	}

	/*
	 * "heal" the circular list by making the last entry point to the
	 * first.
	 */
	desc--;
	desc->cp = scp->txdesc_p & 0x0000ffff;

	/*
	 * Now, initialize the transmit DMA logic
	 *
	 * CPB == chain pointer base address
	 */
	dmac_write_1(scp, SCA_DSR1, 0);
	dmac_write_1(scp, SCA_DCR1, SCA_DCR_ABRT);
	dmac_write_1(scp, SCA_DMR1, SCA_DMR_TMOD | SCA_DMR_NF);
	dmac_write_1(scp, SCA_DIR1,
		     (SCA_DIR_EOT | SCA_DIR_BOF | SCA_DIR_COF));
	dmac_write_1(scp, SCA_CPB1,
		     (u_int8_t)((scp->txdesc_p & 0x00ff0000) >> 16));

	/*
	 * now, do the same thing for receive descriptors
	 */
	desc = scp->rxdesc;
	desc_p = scp->rxdesc_p;
	buf_p = scp->rxbuf_p;
	scp->rxstart = 0;
	scp->rxend = SCA_NrxBUFS - 1;

	for (i = 0 ; i < SCA_NrxBUFS ; i++) {
		/*
		 * desc_p points to the physcial address of the NEXT desc
		 */
		desc_p += sizeof(sca_desc_t);

		desc->cp = desc_p & 0x0000ffff;
		desc->bp = buf_p & 0x0000ffff;
		desc->bpb = (buf_p & 0x00ff0000) >> 16;
		desc->len = SCA_BSIZE;
		desc->stat = 0x00;

		desc++;  /* point to the next descriptor */
		buf_p += SCA_BSIZE;
	}

	/*
	 * "heal" the circular list by making the last entry point to the
	 * first.
	 */
	desc--;
	desc->cp = scp->rxdesc_p & 0x0000ffff;

	sca_dmac_rxinit(scp);

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam,
			0, sc->sc_allocsize, BUS_DMASYNC_POSTWRITE);
}

/*
 * reset and reinitialize the receive DMA logic
 */
static void
sca_dmac_rxinit(sca_port_t *scp)
{
	/*
	 * ... and the receive DMA logic ...
	 */
	dmac_write_1(scp, SCA_DSR0, 0);  /* disable DMA */
	dmac_write_1(scp, SCA_DCR0, SCA_DCR_ABRT);

	dmac_write_1(scp, SCA_DMR0, SCA_DMR_TMOD | SCA_DMR_NF);
	dmac_write_2(scp, SCA_BFLL0, SCA_BSIZE);

	/*
	 * CPB == chain pointer base
	 * CDA == current descriptor address
	 * EDA == error descriptor address (overwrite position)
	 */
	dmac_write_1(scp, SCA_CPB0,
		     (u_int8_t)((scp->rxdesc_p & 0x00ff0000) >> 16));
	dmac_write_2(scp, SCA_CDAL0,
		     (u_int16_t)(scp->rxdesc_p & 0xffff));
	dmac_write_2(scp, SCA_EDAL0,
		     (u_int16_t)(scp->rxdesc_p
				 + sizeof(sca_desc_t) * SCA_NrxBUFS));

	/*
	 * enable receiver DMA
	 */
	dmac_write_1(scp, SCA_DIR0, 
		     (SCA_DIR_EOT | SCA_DIR_EOM | SCA_DIR_BOF | SCA_DIR_COF));
	dmac_write_1(scp, SCA_DSR0, SCA_DSR_DE);
}

static int
sca_alloc_dma(struct sca_softc *sc)
{
	u_int	allocsize;
	int	err;
	int	rsegs;
	u_int	bpp;

	SCA_DPRINTF(SCA_DEBUG_DMA,
		    ("sizeof sca_desc_t: %d bytes\n", sizeof (sca_desc_t)));

	bpp = sc->sc_numports * (SCA_NtxBUFS + SCA_NrxBUFS);

	allocsize = bpp * (SCA_BSIZE + sizeof (sca_desc_t));

	/*
	 * sanity checks:
	 *
	 * Check the total size of the data buffers, and so on.  The total
	 * DMAable space needs to fit within a single 16M region, and the
	 * descriptors need to fit within a 64K region.
	 */
	if (allocsize > 16 * 1024 * 1024)
		return 1;
	if (bpp * sizeof (sca_desc_t) > 64 * 1024)
		return 1;

	sc->sc_allocsize = allocsize;

	/*
	 * Allocate one huge chunk of memory.
	 */
	if (bus_dmamem_alloc(sc->sc_dmat,
			     allocsize,
			     SCA_DMA_ALIGNMENT,
			     SCA_DMA_BOUNDRY,
			     &sc->sc_seg, 1, &rsegs, BUS_DMA_NOWAIT) != 0) {
		printf("Could not allocate DMA memory\n");
		return 1;
	}
	SCA_DPRINTF(SCA_DEBUG_DMA,
		    ("DMA memory allocated:  %d bytes\n", allocsize));

	if (bus_dmamem_map(sc->sc_dmat, &sc->sc_seg, 1, allocsize,
			   &sc->sc_dma_addr, BUS_DMA_NOWAIT) != 0) {
		printf("Could not map DMA memory into kernel space\n");
		return 1;
	}
	SCA_DPRINTF(SCA_DEBUG_DMA, ("DMA memory mapped\n"));

	if (bus_dmamap_create(sc->sc_dmat, allocsize, 2,
			      allocsize, SCA_DMA_BOUNDRY,
			      BUS_DMA_NOWAIT, &sc->sc_dmam) != 0) {
		printf("Could not create DMA map\n");
		return 1;
	}
	SCA_DPRINTF(SCA_DEBUG_DMA, ("DMA map created\n"));

	err = bus_dmamap_load(sc->sc_dmat, sc->sc_dmam, sc->sc_dma_addr,
			      allocsize, NULL, BUS_DMA_NOWAIT);
	if (err != 0) {
		printf("Could not load DMA segment:  %d\n", err);
		return 1;
	}
	SCA_DPRINTF(SCA_DEBUG_DMA, ("DMA map loaded\n"));

	return 0;
}

/*
 * Take the memory allocated with sca_alloc_dma() and divide it among the
 * two ports.
 */
static void
sca_setup_dma_memory(struct sca_softc *sc)
{
	sca_port_t *scp0, *scp1;
	u_int8_t  *vaddr0;
	u_int32_t paddr0;
	u_long addroff;

	/*
	 * remember the physical address to 24 bits only, since the upper
	 * 8 bits is programed into the device at a different layer.
	 */
	paddr0 = (sc->sc_dmam->dm_segs[0].ds_addr & 0x00ffffff);
	vaddr0 = sc->sc_dma_addr;

	/*
	 * if we have only one port it gets the full range.  If we have
	 * two we need to do a little magic to divide things up.
	 *
	 * The descriptors will all end up in the front of the area, while
	 * the remainder of the buffer is used for transmit and receive
	 * data.
	 *
	 * -------------------- start of memory
	 *    tx desc port 0
	 *    rx desc port 0
	 *    tx desc port 1
	 *    rx desc port 1
	 *    tx buffer port 0
	 *    rx buffer port 0
	 *    tx buffer port 1
	 *    rx buffer port 1
	 * -------------------- end of memory
	 */
	scp0 = &sc->sc_ports[0];
	scp1 = &sc->sc_ports[1];

	scp0->txdesc_p = paddr0;
	scp0->txdesc = (sca_desc_t *)vaddr0;
	addroff = sizeof(sca_desc_t) * SCA_NtxBUFS;

	/*
	 * point to the range following the tx descriptors, and
	 * set the rx descriptors there.
	 */
	scp0->rxdesc_p = paddr0 + addroff;
	scp0->rxdesc = (sca_desc_t *)(vaddr0 + addroff);
	addroff += sizeof(sca_desc_t) * SCA_NrxBUFS;

	if (sc->sc_numports == 2) {
		scp1->txdesc_p = paddr0 + addroff;
		scp1->txdesc = (sca_desc_t *)(vaddr0 + addroff);
		addroff += sizeof(sca_desc_t) * SCA_NtxBUFS;

		scp1->rxdesc_p = paddr0 + addroff;
		scp1->rxdesc = (sca_desc_t *)(vaddr0 + addroff);
		addroff += sizeof(sca_desc_t) * SCA_NrxBUFS;
	}

	/*
	 * point to the memory following the descriptors, and set the
	 * transmit buffer there.
	 */
	scp0->txbuf_p = paddr0 + addroff;
	scp0->txbuf = vaddr0 + addroff;
	addroff += SCA_BSIZE * SCA_NtxBUFS;

	/*
	 * lastly, skip over the transmit buffer and set up pointers into
	 * the receive buffer.
	 */
	scp0->rxbuf_p = paddr0 + addroff;
	scp0->rxbuf = vaddr0 + addroff;
	addroff += SCA_BSIZE * SCA_NrxBUFS;

	if (sc->sc_numports == 2) {
		scp1->txbuf_p = paddr0 + addroff;
		scp1->txbuf = vaddr0 + addroff;
		addroff += SCA_BSIZE * SCA_NtxBUFS;

		scp1->rxbuf_p = paddr0 + addroff;
		scp1->rxbuf = vaddr0 + addroff;
		addroff += SCA_BSIZE * SCA_NrxBUFS;
	}

	/*
	 * as a consistancy check, addroff should be equal to the allocation
	 * size.
	 */
	if (sc->sc_allocsize != addroff)
		printf("ERROR:  sc_allocsize != addroff: %lu != %lu\n",
		       sc->sc_allocsize, addroff);
}

/*
 * Queue the packet for our start routine to transmit
 */
static int
sca_output(ifp, m, dst, rt0)
     struct ifnet *ifp;
     struct mbuf *m;
     struct sockaddr *dst;
     struct rtentry *rt0;
{
	int error;
	int s;
	u_int16_t protocol;
	hdlc_header_t *hdlc;
	struct ifqueue *ifq;
#ifdef SCA_USE_FASTQ
	struct ip *ip;
	sca_port_t *scp = ifp->if_softc;
	int highpri;
#endif

	error = 0;
	ifp->if_lastchange = time;

	if ((ifp->if_flags & IFF_UP) != IFF_UP) {
		error = ENETDOWN;
		goto bad;
	}

#ifdef SCA_USE_FASTQ
	highpri = 0;
#endif

	/*
	 * determine address family, and priority for this packet
	 */
	switch (dst->sa_family) {
	case AF_INET:
		protocol = HDLC_PROTOCOL_IP;

#ifdef SCA_USE_FASTQ
		ip = mtod(m, struct ip *);
		if ((ip->ip_tos & IPTOS_LOWDELAY) == IPTOS_LOWDELAY)
			highpri = 1;
#endif
		break;

	default:
		printf("%s: address family %d unsupported\n",
		       ifp->if_xname, dst->sa_family);
		error = EAFNOSUPPORT;
		goto bad;
	}

	if (M_LEADINGSPACE(m) < HDLC_HDRLEN) {
		m = m_prepend(m, HDLC_HDRLEN, M_DONTWAIT);
		if (m == NULL) {
			error = ENOBUFS;
			goto bad;
		}
		m->m_len = 0;
	} else {
		m->m_data -= HDLC_HDRLEN;
	}

	hdlc = mtod(m, hdlc_header_t *);
	if ((m->m_flags & (M_BCAST | M_MCAST)) != 0)
		hdlc->addr = CISCO_MULTICAST;
	else
		hdlc->addr = CISCO_UNICAST;
	hdlc->control = 0;
	hdlc->protocol = htons(protocol);
	m->m_len += HDLC_HDRLEN;

	/*
	 * queue the packet.  If interactive, use the fast queue.
	 */
	s = splnet();
#ifdef SCA_USE_FASTQ
	ifq = (highpri == 1 ? &scp->fastq : &ifp->if_snd);
#else
	ifq = &ifp->if_snd;
#endif
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		ifp->if_oerrors++;
		ifp->if_collisions++;
		error = ENOBUFS;
		splx(s);
		goto bad;
	}
	ifp->if_obytes += m->m_pkthdr.len;
	IF_ENQUEUE(ifq, m);

	ifp->if_lastchange = time;

	if (m->m_flags & M_MCAST)
		ifp->if_omcasts++;

	sca_start(ifp);
	splx(s);

	return (error);

 bad:
	if (m)
		m_freem(m);
	return (error);
}

static int
sca_ioctl(ifp, cmd, addr)
     struct ifnet *ifp;
     u_long cmd;
     caddr_t addr;
{
	struct ifreq *ifr;
	struct ifaddr *ifa;
	int error;
	int s;

	s = splnet();

	ifr = (struct ifreq *)addr;
	ifa = (struct ifaddr *)addr;
	error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		if (ifa->ifa_addr->sa_family == AF_INET)
			sca_port_up(ifp->if_softc);
		else
			error = EAFNOSUPPORT;
		break;

	case SIOCSIFDSTADDR:
		if (ifa->ifa_addr->sa_family != AF_INET)
			error = EAFNOSUPPORT;
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (ifr == 0) {
			error = EAFNOSUPPORT;		/* XXX */
			break;
		}
		switch (ifr->ifr_addr.sa_family) {

#ifdef INET
		case AF_INET:
			break;
#endif

		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if (ifr->ifr_flags & IFF_UP)
			sca_port_up(ifp->if_softc);
		else
			sca_port_down(ifp->if_softc);

		break;

	default:
		error = EINVAL;
	}

	splx(s);
	return error;
}

/*
 * start packet transmission on the interface
 *
 * MUST BE CALLED AT splnet()
 */
static void
sca_start(ifp)
	struct ifnet *ifp;
{
	sca_port_t *scp = ifp->if_softc;
	struct sca_softc *sc = scp->sca;
	struct mbuf *m, *mb_head;
	sca_desc_t *desc;
	u_int8_t *buf;
	u_int32_t buf_p;
	int nexttx;
	int trigger_xmit;

	/*
	 * can't queue when we are full or transmitter is busy
	 */
	if ((scp->txinuse >= (SCA_NtxBUFS - 1))
	    || ((ifp->if_flags & IFF_OACTIVE) == IFF_OACTIVE))
		return;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam,
			0, sc->sc_allocsize,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	trigger_xmit = 0;

 txloop:
	IF_DEQUEUE(&scp->linkq, mb_head);
	if (mb_head == NULL)
#ifdef SCA_USE_FASTQ
		IF_DEQUEUE(&scp->fastq, mb_head);
	if (mb_head == NULL)
#endif
		IF_DEQUEUE(&ifp->if_snd, mb_head);
	if (mb_head == NULL)
		goto start_xmit;

	if (scp->txinuse != 0) {
		/* Kill EOT interrupts on the previous descriptor. */
		desc = &scp->txdesc[scp->txcur];
		desc->stat &= ~SCA_DESC_EOT;

		/* Figure out what the next free descriptor is. */
		if ((scp->txcur + 1) == SCA_NtxBUFS)
			nexttx = 0;
		else
			nexttx = scp->txcur + 1;
	} else
		nexttx = 0;

	desc = &scp->txdesc[nexttx];
	buf = scp->txbuf + SCA_BSIZE * nexttx;
	buf_p = scp->txbuf_p + SCA_BSIZE * nexttx;

	desc->bp = (u_int16_t)(buf_p & 0x0000ffff);
	desc->bpb = (u_int8_t)((buf_p & 0x00ff0000) >> 16);
	desc->stat = SCA_DESC_EOT | SCA_DESC_EOM;  /* end of frame and xfer */
	desc->len = 0;

	/*
	 * Run through the chain, copying data into the descriptor as we
	 * go.  If it won't fit in one transmission block, drop the packet.
	 * No, this isn't nice, but most of the time it _will_ fit.
	 */
	for (m = mb_head ; m != NULL ; m = m->m_next) {
		if (m->m_len != 0) {
			desc->len += m->m_len;
			if (desc->len > SCA_BSIZE) {
				m_freem(mb_head);
				goto txloop;
			}
			bcopy(mtod(m, u_int8_t *), buf, m->m_len);
			buf += m->m_len;
		}
	}

	ifp->if_opackets++;

#if NBPFILTER > 0
	/*
	 * Pass packet to bpf if there is a listener.
	 */
	if (scp->sp_bpf)
		bpf_mtap(scp->sp_bpf, mb_head);
#endif

	m_freem(mb_head);

	if (scp->txinuse != 0) {
		scp->txcur++;
		if (scp->txcur == SCA_NtxBUFS)
			scp->txcur = 0;
	}
	scp->txinuse++;
	trigger_xmit = 1;

	SCA_DPRINTF(SCA_DEBUG_TX,
		    ("TX: inuse %d index %d\n", scp->txinuse, scp->txcur));

	if (scp->txinuse < (SCA_NtxBUFS - 1))
		goto txloop;

 start_xmit:
	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmam,
			0, sc->sc_allocsize,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	if (trigger_xmit != 0)
		sca_port_starttx(scp);
}

static void
sca_watchdog(ifp)
	struct ifnet *ifp;
{
}

int
sca_hardintr(struct sca_softc *sc)
{
	u_int8_t isr0, isr1, isr2;
	int	ret;

	ret = 0;  /* non-zero means we processed at least one interrupt */

	while (1) {
		/*
		 * read SCA interrupts
		 */
		isr0 = sca_read_1(sc, SCA_ISR0);
		isr1 = sca_read_1(sc, SCA_ISR1);
		isr2 = sca_read_1(sc, SCA_ISR2);

		if (isr0 == 0 && isr1 == 0 && isr2 == 0)
			break;

		SCA_DPRINTF(SCA_DEBUG_INTR,
			    ("isr0 = %02x, isr1 = %02x, isr2 = %02x\n",
			     isr0, isr1, isr2));

		/*
		 * check DMA interrupt
		 */
		if (isr1 & 0x0f)
			ret += sca_dmac_intr(&sc->sc_ports[0],
					     isr1 & 0x0f);
		if (isr1 & 0xf0)
			ret += sca_dmac_intr(&sc->sc_ports[1],
					     (isr1 & 0xf0) >> 4);

		if (isr0)
			ret += sca_msci_intr(sc, isr0);

#if 0 /* We don't GET timer interrupts, we have them disabled (msci IE20) */
		if (isr2)
			ret += sca_timer_intr(sc, isr2);
#endif
	}

	return (ret);
}

static int
sca_dmac_intr(sca_port_t *scp, u_int8_t isr)
{
	u_int8_t	 dsr;
	int		 ret;

	ret = 0;

	/*
	 * Check transmit channel
	 */
	if (isr & 0x0c) {
		SCA_DPRINTF(SCA_DEBUG_INTR,
			    ("TX INTERRUPT port %d\n", scp->sp_port));

		dsr = 1;
		while (dsr != 0) {
			ret++;
			/*
			 * reset interrupt
			 */
			dsr = dmac_read_1(scp, SCA_DSR1);
			dmac_write_1(scp, SCA_DSR1,
				     dsr | SCA_DSR_DEWD);

			/*
			 * filter out the bits we don't care about
			 */
			dsr &= ( SCA_DSR_COF | SCA_DSR_BOF | SCA_DSR_EOT);
			if (dsr == 0)
				break;

			/*
			 * check for counter overflow
			 */
			if (dsr & SCA_DSR_COF) {
				printf("%s: TXDMA counter overflow\n",
				       scp->sp_if.if_xname);
				
				scp->sp_if.if_flags &= ~IFF_OACTIVE;
				scp->txcur = 0;
				scp->txinuse = 0;
			}

			/*
			 * check for buffer overflow
			 */
			if (dsr & SCA_DSR_BOF) {
				printf("%s: TXDMA buffer overflow, cda 0x%04x, eda 0x%04x, cpb 0x%02x\n",
				       scp->sp_if.if_xname,
				       dmac_read_2(scp, SCA_CDAL1),
				       dmac_read_2(scp, SCA_EDAL1),
				       dmac_read_1(scp, SCA_CPB1));

				/*
				 * Yikes.  Arrange for a full
				 * transmitter restart.
				 */
				scp->sp_if.if_flags &= ~IFF_OACTIVE;
				scp->txcur = 0;
				scp->txinuse = 0;
			}

			/*
			 * check for end of transfer, which is not
			 * an error. It means that all data queued
			 * was transmitted, and we mark ourself as
			 * not in use and stop the watchdog timer.
			 */
			if (dsr & SCA_DSR_EOT) {
				SCA_DPRINTF(SCA_DEBUG_TX,
					    ("Transmit completed.\n"));

				scp->sp_if.if_flags &= ~IFF_OACTIVE;
				scp->txcur = 0;
				scp->txinuse = 0;

				/*
				 * check for more packets
				 */
				sca_start(&scp->sp_if);
			}
		}
	}
	/*
	 * receive channel check
	 */
	if (isr & 0x03) {
		SCA_DPRINTF(SCA_DEBUG_INTR,
			    ("RX INTERRUPT port %d\n", mch));

		dsr = 1;
		while (dsr != 0) {
			ret++;

			dsr = dmac_read_1(scp, SCA_DSR0);
			dmac_write_1(scp, SCA_DSR0, dsr | SCA_DSR_DEWD);

			/*
			 * filter out the bits we don't care about
			 */
			dsr &= (SCA_DSR_EOM | SCA_DSR_COF
				| SCA_DSR_BOF | SCA_DSR_EOT);
			if (dsr == 0)
				break;

			/*
			 * End of frame
			 */
			if (dsr & SCA_DSR_EOM) {
				SCA_DPRINTF(SCA_DEBUG_RX, ("Got a frame!\n"));

				sca_get_packets(scp);
			}

			/*
			 * check for counter overflow
			 */
			if (dsr & SCA_DSR_COF) {
				printf("%s: RXDMA counter overflow\n",
				       scp->sp_if.if_xname);

				sca_dmac_rxinit(scp);
			}

			/*
			 * check for end of transfer, which means we
			 * ran out of descriptors to receive into.
			 * This means the line is much faster than
			 * we can handle.
			 */
			if (dsr & (SCA_DSR_BOF | SCA_DSR_EOT)) {
				printf("%s: RXDMA buffer overflow\n",
				       scp->sp_if.if_xname);

				sca_dmac_rxinit(scp);
			}
		}
	}

	return ret;
}

static int
sca_msci_intr(struct sca_softc *sc, u_int8_t isr)
{
	printf("Got msci interrupt XXX\n");

	return 0;
}

static void
sca_get_packets(sca_port_t *scp)
{
	int		 descidx;
	sca_desc_t	*desc;
	u_int8_t	*buf;

	bus_dmamap_sync(scp->sca->sc_dmat, scp->sca->sc_dmam,
			0, scp->sca->sc_allocsize,
			BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Loop while there are packets to receive.  After each is processed,
	 * call sca_frame_skip() to update the DMA registers to the new
	 * state.
	 */
	while (sca_frame_avail(scp, &descidx)) {
		desc = &scp->rxdesc[descidx];
		buf = scp->rxbuf + SCA_BSIZE * descidx;

		sca_frame_process(scp, desc, buf);
#if SCA_DEBUG_LEVEL > 0
		if (sca_debug & SCA_DEBUG_RXPKT)
			sca_frame_print(scp, desc, buf);
#endif
		sca_frame_skip(scp, descidx);
	}

	bus_dmamap_sync(scp->sca->sc_dmat, scp->sca->sc_dmam,
			0, scp->sca->sc_allocsize,
			BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
}

/*
 * Starting with the first descriptor we wanted to read into, up to but
 * not including the current SCA read descriptor, look for a packet.
 */
static int
sca_frame_avail(sca_port_t *scp, int *descindx)
{
	u_int16_t	 cda;
	int		 cdaidx;
	u_int32_t	 desc_p;	/* physical address (lower 16 bits) */
	sca_desc_t	*desc;
	u_int8_t	 rxstat;

	/*
	 * Read the current descriptor from the SCA.
	 */
	cda = dmac_read_2(scp, SCA_CDAL0);

	/*
	 * calculate the index of the current descriptor
	 */
	desc_p = cda - (u_int16_t)(scp->rxdesc_p & 0x0000ffff);
	cdaidx = desc_p / sizeof(sca_desc_t);

	if (cdaidx >= SCA_NrxBUFS)
		return 0;

	for (;;) {
		/*
		 * if the SCA is reading into the first descriptor, we somehow
		 * got this interrupt incorrectly.  Just return that there are
		 * no packets ready.
		 */
		if (cdaidx == scp->rxstart)
			return 0;

		/*
		 * We might have a valid descriptor.  Set up a pointer
		 * to the kva address for it so we can more easily examine
		 * the contents.
		 */
		desc = &scp->rxdesc[scp->rxstart];

		rxstat = desc->stat;

		/*
		 * check for errors
		 */
		if (rxstat & SCA_DESC_ERRORS)
			goto nextpkt;

		/*
		 * full packet?  Good.
		 */
		if (rxstat & SCA_DESC_EOM) {
			*descindx = scp->rxstart;
			return 1;
		}

		/*
		 * increment the rxstart address, since this frame is
		 * somehow damaged.  Skip over it in later calls.
		 * XXX This breaks multidescriptor receives, so each
		 * frame HAS to fit within one descriptor's buffer
		 * space now...
		 */
	nextpkt:
		scp->rxstart++;
		if (scp->rxstart == SCA_NrxBUFS)
			scp->rxstart = 0;
	}

	return 0;
}

/*
 * Pass the packet up to the kernel if it is a packet we want to pay
 * attention to.
 *
 * MUST BE CALLED AT splnet()
 */
static void
sca_frame_process(sca_port_t *scp, sca_desc_t *desc, u_int8_t *p)
{
	hdlc_header_t	*hdlc;
	cisco_pkt_t	*cisco, *ncisco;
	u_int16_t	 len;
	struct mbuf	*m;
	u_int8_t	*nbuf;
	u_int32_t	 t = (time.tv_sec - boottime.tv_sec) * 1000;
	struct ifqueue *ifq;

	len = desc->len;

	/*
	 * skip packets that are too short
	 */
	if (len < sizeof(hdlc_header_t))
		return;

#if NBPFILTER > 0
	if (scp->sp_bpf)
		bpf_tap(scp->sp_bpf, p, len);
#endif

	/*
	 * read and then strip off the HDLC information
	 */
	hdlc = (hdlc_header_t *)p;

	scp->sp_if.if_ipackets++;
	scp->sp_if.if_lastchange = time;

	switch (ntohs(hdlc->protocol)) {
	case HDLC_PROTOCOL_IP:
		SCA_DPRINTF(SCA_DEBUG_RX, ("Received IP packet\n"));

		m = sca_mbuf_alloc(p, len);
		if (m == NULL) {
			scp->sp_if.if_iqdrops++;
			return;
		}
		m->m_pkthdr.rcvif = &scp->sp_if;

		if (IF_QFULL(&ipintrq)) {
			IF_DROP(&ipintrq);
			scp->sp_if.if_ierrors++;
			scp->sp_if.if_iqdrops++;
			m_freem(m);
		} else {
			/*
			 * strip off the HDLC header and hand off to IP stack
			 */
			m->m_pkthdr.len -= HDLC_HDRLEN;
			m->m_data += HDLC_HDRLEN;
			m->m_len -= HDLC_HDRLEN;
			IF_ENQUEUE(&ipintrq, m);
			schednetisr(NETISR_IP);
		}

		break;

	case CISCO_KEEPALIVE:
		SCA_DPRINTF(SCA_DEBUG_CISCO,
			    ("Received CISCO keepalive packet\n"));

		if (len < CISCO_PKT_LEN) {
			SCA_DPRINTF(SCA_DEBUG_CISCO,
				    ("short CISCO packet %d, wanted %d\n",
				     len, CISCO_PKT_LEN));
			return;
		}

		/*
		 * allocate an mbuf and copy the important bits of data
		 * into it.
		 */
		m = sca_mbuf_alloc(p, HDLC_HDRLEN + CISCO_PKT_LEN);
		if (m == NULL)
			return;

		nbuf = mtod(m, u_int8_t *);
		ncisco = (cisco_pkt_t *)(nbuf + HDLC_HDRLEN);
		m->m_pkthdr.rcvif = &scp->sp_if;

		cisco = (cisco_pkt_t *)(p + HDLC_HDRLEN);

		switch (ntohl(cisco->type)) {
		case CISCO_ADDR_REQ:
			printf("Got CISCO addr_req, ignoring\n");
			m_freem(m);
			break;

		case CISCO_ADDR_REPLY:
			printf("Got CISCO addr_reply, ignoring\n");
			m_freem(m);
			break;

		case CISCO_KEEPALIVE_REQ:
			SCA_DPRINTF(SCA_DEBUG_CISCO,
				    ("Received KA, mseq %d,"
				     " yseq %d, rel 0x%04x, t0"
				     " %04x, t1 %04x\n",
				     ntohl(cisco->par1), ntohl(cisco->par2),
				     ntohs(cisco->rel), ntohs(cisco->time0),
				     ntohs(cisco->time1)));

			scp->cka_lastrx = ntohl(cisco->par1);
			scp->cka_lasttx++;

			/*
			 * schedule the transmit right here.
			 */
			ncisco->par2 = cisco->par1;
			ncisco->par1 = htonl(scp->cka_lasttx);
			ncisco->time0 = htons((u_int16_t)(t >> 16));
			ncisco->time1 = htons((u_int16_t)(t & 0x0000ffff));

			ifq = &scp->linkq;
			if (IF_QFULL(ifq)) {
				IF_DROP(ifq);
				m_freem(m);
				return;
			}
			IF_ENQUEUE(ifq, m);

			sca_start(&scp->sp_if);

			break;

		default:
			m_freem(m);
			SCA_DPRINTF(SCA_DEBUG_CISCO,
				    ("Unknown CISCO keepalive protocol 0x%04x\n",
				     ntohl(cisco->type)));
			return;
		}

		break;

	default:
		SCA_DPRINTF(SCA_DEBUG_RX,
			    ("Unknown/unexpected ethertype 0x%04x\n",
			     ntohs(hdlc->protocol)));
	}
}

#if SCA_DEBUG_LEVEL > 0
/*
 * do a hex dump of the packet received into descriptor "desc" with
 * data buffer "p"
 */
static void
sca_frame_print(sca_port_t *scp, sca_desc_t *desc, u_int8_t *p)
{
	int i;
	int nothing_yet = 1;

	printf("descriptor va %p: cp 0x%x bpb 0x%0x bp 0x%0x stat 0x%0x len %d\n",
	       desc, desc->cp, desc->bpb, desc->bp, desc->stat, desc->len);

	for (i = 0 ; i < desc->len ; i++) {
		if (nothing_yet == 1 && *p == 0) {
			p++;
			continue;
		}
		nothing_yet = 0;
		if (i % 16 == 0)
			printf("\n");
		printf("%02x ", *p++);
	}

	if (i % 16 != 1)
		printf("\n");
}
#endif

/*
 * skip all frames before the descriptor index "indx" -- we do this by
 * moving the rxstart pointer to the index following this one, and
 * setting the end descriptor to this index.
 */
static void
sca_frame_skip(sca_port_t *scp, int indx)
{
	u_int32_t	desc_p;

	scp->rxstart++;
	if (scp->rxstart == SCA_NrxBUFS)
		scp->rxstart = 0;

	desc_p = scp->rxdesc_p * sizeof(sca_desc_t) * indx;
	dmac_write_2(scp, SCA_EDAL0,
		     (u_int16_t)(desc_p & 0x0000ffff));
}

/*
 * set a port to the "up" state
 */
static void
sca_port_up(sca_port_t *scp)
{
	struct sca_softc *sc = scp->sca;

	/*
	 * reset things
	 */
#if 0
	msci_write_1(scp, SCA_CMD0, SCA_CMD_TXRESET);
	msci_write_1(scp, SCA_CMD0, SCA_CMD_RXRESET);
#endif
	/*
	 * clear in-use flag
	 */
	scp->sp_if.if_flags &= ~IFF_OACTIVE;

	/*
	 * raise DTR
	 */
	sc->dtr_callback(sc->dtr_aux, scp->sp_port, 1);

	/*
	 * raise RTS
	 */
	msci_write_1(scp, SCA_CTL0,
		     msci_read_1(scp, SCA_CTL0) & ~SCA_CTL_RTS);

	/*
	 * enable interrupts
	 */
	if (scp->sp_port == 0) {
		sca_write_1(sc, SCA_IER0, sca_read_1(sc, SCA_IER0) | 0x0f);
		sca_write_1(sc, SCA_IER1, sca_read_1(sc, SCA_IER1) | 0x0f);
	} else {
		sca_write_1(sc, SCA_IER0, sca_read_1(sc, SCA_IER0) | 0xf0);
		sca_write_1(sc, SCA_IER1, sca_read_1(sc, SCA_IER1) | 0xf0);
	}

	/*
	 * enable transmit and receive
	 */
	msci_write_1(scp, SCA_CMD0, SCA_CMD_TXENABLE);
	msci_write_1(scp, SCA_CMD0, SCA_CMD_RXENABLE);

	/*
	 * reset internal state
	 */
	scp->txinuse = 0;
	scp->txcur = 0;
	scp->cka_lasttx = time.tv_usec;
	scp->cka_lastrx = 0;
}

/*
 * set a port to the "down" state
 */
static void
sca_port_down(sca_port_t *scp)
{
	struct sca_softc *sc = scp->sca;

	/*
	 * lower DTR
	 */
	sc->dtr_callback(sc->dtr_aux, scp->sp_port, 0);

	/*
	 * lower RTS
	 */
	msci_write_1(scp, SCA_CTL0,
		     msci_read_1(scp, SCA_CTL0) | SCA_CTL_RTS);

	/*
	 * disable interrupts
	 */
	if (scp->sp_port == 0) {
		sca_write_1(sc, SCA_IER0, sca_read_1(sc, SCA_IER0) & 0xf0);
		sca_write_1(sc, SCA_IER1, sca_read_1(sc, SCA_IER1) & 0xf0);
	} else {
		sca_write_1(sc, SCA_IER0, sca_read_1(sc, SCA_IER0) & 0x0f);
		sca_write_1(sc, SCA_IER1, sca_read_1(sc, SCA_IER1) & 0x0f);
	}

	/*
	 * disable transmit and receive
	 */
	msci_write_1(scp, SCA_CMD0, SCA_CMD_RXDISABLE);
	msci_write_1(scp, SCA_CMD0, SCA_CMD_TXDISABLE);

	/*
	 * no, we're not in use anymore
	 */
	scp->sp_if.if_flags &= ~IFF_OACTIVE;
}

/*
 * disable all DMA and interrupts for all ports at once.
 */
void
sca_shutdown(struct sca_softc *sca)
{
	/*
	 * disable DMA and interrupts
	 */
	sca_write_1(sca, SCA_DMER, 0);
	sca_write_1(sca, SCA_IER0, 0);
	sca_write_1(sca, SCA_IER1, 0);
}

/*
 * If there are packets to transmit, start the transmit DMA logic.
 */
static void
sca_port_starttx(sca_port_t *scp)
{
	struct sca_softc *sc;
	u_int32_t	startdesc_p, enddesc_p;
	int enddesc;

	sc = scp->sca;

	if (((scp->sp_if.if_flags & IFF_OACTIVE) == IFF_OACTIVE)
	    || scp->txinuse == 0)
		return;
	scp->sp_if.if_flags |= IFF_OACTIVE;

	/*
	 * We have something to do, since we have at least one packet
	 * waiting, and we are not already marked as active.
	 */
	enddesc = scp->txcur;
	enddesc++;
	if (enddesc == SCA_NtxBUFS)
		enddesc = 0;

	startdesc_p = scp->txdesc_p;
	enddesc_p = scp->txdesc_p + sizeof(sca_desc_t) * enddesc;

	dmac_write_2(scp, SCA_EDAL1, (u_int16_t)(enddesc_p & 0x0000ffff));
	dmac_write_2(scp, SCA_CDAL1,
		     (u_int16_t)(startdesc_p & 0x0000ffff));

	/*
	 * enable the DMA
	 */
	dmac_write_1(scp, SCA_DSR1, SCA_DSR_DE);
}

/*
 * allocate an mbuf at least long enough to hold "len" bytes.
 * If "p" is non-NULL, copy "len" bytes from it into the new mbuf,
 * otherwise let the caller handle copying the data in.
 */
static struct mbuf *
sca_mbuf_alloc(caddr_t p, u_int len)
{
	struct mbuf *m;

	/*
	 * allocate an mbuf and copy the important bits of data
	 * into it.  If the packet won't fit in the header,
	 * allocate a cluster for it and store it there.
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return NULL;
	if (len > MHLEN) {
		if (len > MCLBYTES) {
			m_freem(m);
			return NULL;
		}
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			return NULL;
		}
	}
	if (p != NULL)
		bcopy(p, mtod(m, caddr_t), len);
	m->m_len = len;
	m->m_pkthdr.len = len;

	return (m);
}
