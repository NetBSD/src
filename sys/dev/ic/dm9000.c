/*	$NetBSD: dm9000.c,v 1.2 2010/09/10 08:58:36 ahoka Exp $	*/

/*
 * Copyright (c) 2009 Paul Fleischer
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* based on sys/dev/ic/cs89x0.c */
/*
 * Copyright (c) 2004 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <net/bpf.h>
#include <net/bpfdesc.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/ic/dm9000var.h>
#include <dev/ic/dm9000reg.h>

#if 1
#undef DM9000_DEBUG
#undef  DM9000_TX_DEBUG
#undef DM9000_TX_DATA_DEBUG
#undef DM9000_RX_DEBUG
#undef  DM9000_RX_DATA_DEBUG
#else
#define DM9000_DEBUG
#define  DM9000_TX_DEBUG
#define DM9000_TX_DATA_DEBUG
#define DM9000_RX_DEBUG
#define  DM9000_RX_DATA_DEBUG
#endif

#ifdef DM9000_DEBUG
#define DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_TX_DEBUG
#define TX_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define TX_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_RX_DEBUG
#define RX_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define RX_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_RX_DATA_DEBUG
#define RX_DATA_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define RX_DATA_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif

#ifdef DM9000_TX_DATA_DEBUG
#define TX_DATA_DPRINTF(s) do {printf s; } while (/*CONSTCOND*/0)
#else
#define TX_DATA_DPRINTF(s) do {} while (/*CONSTCOND*/0)
#endif


uint16_t dme_phy_read(struct dme_softc *sc, int reg);
void dme_phy_write(struct dme_softc *sc, int reg, uint16_t value);

/*** Methods registered in struct ifnet ***/
void	dme_start_output(struct ifnet *ifp);
int	dme_init(struct ifnet *ifp);
int	dme_ioctl(struct ifnet *ifp, u_long cmd, void *data);
void	dme_stop(struct ifnet *ifp, int disable);

int	dme_mediachange(struct ifnet *ifp);
void	dme_mediastatus(struct ifnet *ufp, struct ifmediareq *ifmr);

/*** Internal methods ***/

/* Prepare data to be transmitted (i.e. dequeue and load it into the DM9000) */
void    dme_prepare(struct dme_softc *sc, struct ifnet *ifp);

/* Transmit prepared data */
void    dme_transmit(struct dme_softc *sc);

/* Receive data */
void    dme_receive(struct dme_softc *sc, struct ifnet *ifp);

/* Software Initialize/Reset of the DM9000 */
void    dme_reset(struct dme_softc *sc);

uint16_t
dme_phy_read(struct dme_softc *sc, int reg)
{
	uint16_t val;
	/* Select Register to read*/
	dme_write(sc, DM9000_EPAR, DM9000_EPAR_INT_PHY +
	    (reg & DM9000_EPAR_EROA_MASK));
	/* Select read operation (DM9000_EPCR_ERPRR) from the PHY */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_ERPRR + DM9000_EPCR_EPOS_PHY);

	/* Wait until access to PHY has completed */
	while (dme_read(sc, DM9000_EPCR) & DM9000_EPCR_ERRE);

	/* XXX: The delay is probably not necessary as we just busy-waited */
	delay(200);

	/* Reset ERPRR-bit */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_EPOS_PHY);

	val = dme_read(sc, DM9000_EPDRL);
	val += dme_read(sc, DM9000_EPDRH) << 8;

	return val;
}

void
dme_phy_write(struct dme_softc *sc, int reg, uint16_t value)
{
	/* Select Register to write*/
	dme_write(sc, DM9000_EPAR, DM9000_EPAR_INT_PHY +
	    (reg & DM9000_EPAR_EROA_MASK));

	/* Write data to the two data registers */
	dme_write(sc, DM9000_EPDRL, value & 0xFF);
	dme_write(sc, DM9000_EPDRH, (value >> 8) & 0xFF);

	/* Select write operation (DM9000_EPCR_ERPRW) from the PHY */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_ERPRW + DM9000_EPCR_EPOS_PHY);

	/* Wait until access to PHY has completed */
	while(dme_read(sc, DM9000_EPCR) & DM9000_EPCR_ERRE);


	/* XXX: The delay is probably not necessary as we just busy-waited */
	delay(200);

	/* Reset ERPRR-bit */
	dme_write(sc, DM9000_EPCR, DM9000_EPCR_EPOS_PHY);
}

int
dme_attach(struct dme_softc *sc, uint8_t *enaddr)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t b[2];

	dme_read_c(sc, DM9000_VID0, b, 2);
#if BYTE_ORDER == BIG_ENDIAN
	sc->sc_vendor_id = (b[0] << 8) | b[1];
#else
	sc->sc_vendor_id = b[0] | (b[1] << 8);
#endif
	dme_read_c(sc, DM9000_PID0, b, 2);
#if BYTE_ORDER == BIG_ENDIAN
	sc->sc_product_id = (b[0] << 8) | b[1];
#else
	sc->sc_product_id = b[0] | (b[1] << 8);
#endif
	/* TODO: Check the vendor ID as well */
	if (sc->sc_product_id != 0x9000) {
		panic("dme_attach: product id mismatch (0x%hx != 0x9000)",
		    sc->sc_product_id);
	}

#if 0
	{
		/* Force 10Mbps to test dme_phy_write */
		uint16_t bmcr;
		bmcr = dme_phy_read(sc, DM9000_PHY_BMCR);
		bmcr &= ~DM9000_PHY_BMCR_AUTO_NEG_EN;
		bmcr &= ~DM9000_PHY_BMCR_SPEED_SELECT; /* select 100Mbps */
		dme_phy_write(sc, DM9000_PHY_BMCR, bmcr);
	}
#endif
	/* Initialize ifnet structure. */
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = dme_start_output;
	ifp->if_init = dme_init;
	ifp->if_ioctl = dme_ioctl;
	ifp->if_stop = dme_stop;
	ifp->if_watchdog = NULL;	/* no watchdog at this stage */
	ifp->if_flags = IFF_SIMPLEX | IFF_NOTRAILERS |
		IFF_BROADCAST; /* No multicast support for now */
	IFQ_SET_READY(&ifp->if_snd);

	/* Initialize ifmedia structures. */
	ifmedia_init(&sc->sc_media, 0, dme_mediachange, dme_mediastatus);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_10_T|IFM_100_TX, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_10_T|IFM_100_TX);

	if (enaddr != NULL)
		memcpy(sc->sc_enaddr, enaddr, sizeof(sc->sc_enaddr));

	/* Configure DM9000 with the MAC address */
	dme_write_c(sc, DM9000_PAB0, sc->sc_enaddr, 6);

#ifdef DM9000_DEBUG
	{
		uint8_t macAddr[6];
		dme_read_c(sc, DM9000_PAB0, macAddr, 6);
		printf("DM9000 configured with MAC address: ");
		for (int i = 0; i < 6; i++) {
			printf("%02X:", macAddr[i]);
		}
		printf("\n");
	}
#endif

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

#ifdef DM9000_DEBUG
	{
		uint8_t network_state;
		network_state = dme_read(sc, DM9000_NSR);
		printf("DM9000 Link status: ");
		if (network_state & DM9000_NSR_LINKST) {
			if (network_state & DM9000_NSR_SPEED)
				printf("10Mbps");
			else
				printf("100Mbps");
		} else {
			printf("Down");
		}
		printf("\n");
	}
#endif

	sc->io_mode = (dme_read(sc, DM9000_ISR) &
	    DM9000_IOMODE_MASK) >> DM9000_IOMODE_SHIFT;
	if (sc->io_mode != DM9000_MODE_16BIT )
		panic("DM9000: Only 16-bit mode is supported!\n");
#ifdef DM9000_DEBUG
	printf("DM9000 Operation Mode: ");
	switch( sc->io_mode) {
	case DM9000_MODE_16BIT:
		printf("16-bit mode");
		break;
	case DM9000_MODE_32BIT:
		printf("32-bit mode");
		break;
	case DM9000_MODE_8BIT:
		printf("8-bit mode");
		break;
	case 3:
		printf("Invalid mode");
		break;
	}
	printf("\n");
#endif

	return 0;
}

int dme_intr(void *arg)
{
	struct dme_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint8_t status;

	/* Disable interrupts */
	dme_write(sc, DM9000_IMR, DM9000_IMR_PAR );

	status = dme_read(sc, DM9000_ISR);
	dme_write(sc, DM9000_ISR, status);

	if (status & DM9000_ISR_PRS) {
		if (ifp->if_flags & IFF_RUNNING )
			dme_receive(sc, ifp);
	}
	if (status & DM9000_ISR_PTS) {
		uint8_t nsr;
		uint8_t tx_status = 0x01; /* Initialize to an error value */

		/* A packet has been transmitted */
		sc->txbusy = 0;

		nsr = dme_read(sc, DM9000_NSR);

		if (nsr & DM9000_NSR_TX1END) {
			tx_status = dme_read(sc, DM9000_TSR1);
			TX_DPRINTF(("dme_intr: Sent using channel 0\n"));
		} else if (nsr & DM9000_NSR_TX2END) {
			tx_status = dme_read(sc, DM9000_TSR2);
			TX_DPRINTF(("dme_intr: Sent using channel 1\n"));
		}

		if (tx_status == 0x0) {
			/* Frame successfully sent */
			ifp->if_opackets++;
		} else {
			ifp->if_oerrors++;
		}

		/* If we have nothing ready to transmit, prepare something */
		if (!sc->txready) {
			dme_prepare(sc, ifp);
		}

		if (sc->txready)
			dme_transmit(sc);

		/* Prepare the next frame */
		dme_prepare(sc, ifp);

	}
#ifdef notyet
	if (status & DM9000_ISR_LNKCHNG) {
	}
#endif

	/* Enable interrupts again */
	dme_write(sc, DM9000_IMR, DM9000_IMR_PAR | DM9000_IMR_PRM |
		 DM9000_IMR_PTM);

	return 1;
}

void
dme_start_output(struct ifnet *ifp)
{
	struct dme_softc *sc;

	sc = ifp->if_softc;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING) {
		printf("No output\n");
		return;
	}

	if (sc->txbusy && sc->txready) {
		panic("DM9000: Internal error, trying to send without"
		    " any empty queue\n");
	}

	dme_prepare(sc, ifp);

	if (sc->txbusy == 0) {
		/* We are ready to transmit right away */
		dme_transmit(sc);
		dme_prepare(sc, ifp); /* Prepare next one */
	} else {
		/* We need to wait until the current packet has
		 * been transmitted.
		 */
		ifp->if_flags |= IFF_OACTIVE;
	}
}

void
dme_prepare(struct dme_softc *sc, struct ifnet *ifp)
{
	struct mbuf *buf;
	struct mbuf *bufChain;
	uint16_t length;
	uint8_t *write_ptr;

	TX_DPRINTF(("dme_prepare: Entering\n"));

	if (sc->txready)
		panic("dme_prepare: Someone called us with txready set\n");

	IFQ_DEQUEUE(&ifp->if_snd, bufChain);
	if (bufChain == NULL) {
		TX_DPRINTF(("dme_prepare: Nothing to transmit\n"));
		ifp->if_flags &= ~IFF_OACTIVE; /* Clear OACTIVE bit */
		return; /* Nothing to transmit */
	}

	/* Element has now been removed from the queue, so we better send it */

	if (ifp->if_bpf)
		bpf_mtap(ifp, bufChain);


	length = 0;

	/* XXX: This support 16-bit I/O mode only. */
	/* XXX: This code must be factored out, such that architecture
	   dependant versions can be supplied */

	int left_over_count = 0; /* Number of bytes from previous mbuf, which
				    need to be written with the next.*/
	uint16_t left_over_buf = 0;

	/* Setup the DM9000 to accept the writes, and then write each buf in
	   the chain. */

	TX_DATA_DPRINTF(("dme_prepare: Writing data: "));
	bus_space_write_1(sc->sc_iot, sc->sc_ioh, sc->dme_io, DM9000_MWCMD);
	for (buf = bufChain; buf != NULL; buf = buf->m_next) {
		int to_write = buf->m_len;

		length += to_write;

		write_ptr = buf->m_data;
		while (to_write > 0 ||
		       (buf->m_next == NULL && left_over_count > 0)
		       ) {
			if (left_over_count > 0) {
				uint8_t b = 0;
				DPRINTF(("dme_prepare: "
					"Writing left over byte\n"));

				if (to_write > 0) {
					b = *write_ptr;
					to_write--;
					write_ptr++;

					DPRINTF(("Took single byte\n"));
				} else {
					DPRINTF(("Leftover in last run\n"));
					length++;
				}

				/* Does shift direction depend on endianess? */
				left_over_buf = left_over_buf | (b << 8);

				bus_space_write_2(sc->sc_iot, sc->sc_ioh,
						  sc->dme_data, left_over_buf);
				TX_DATA_DPRINTF(("%02X ", left_over_buf));
				left_over_count = 0;
			} else if ((long)write_ptr % 2 != 0) {
				/* Misaligned data */
				DPRINTF(("dme_prepare: "
					"Detected misaligned data\n"));
				left_over_buf = *write_ptr;
				left_over_count = 1;
				write_ptr++;
				to_write--;
			} else {
				int i;
				uint16_t *dptr = (uint16_t*)write_ptr;

				/* A block of aligned data. */
				for(i = 0; i < to_write/2; i++) {
					/* buf will be half-word aligned
					 * all the time
					 */
					bus_space_write_2(sc->sc_iot,
					    sc->sc_ioh, sc->dme_data, *dptr);
					TX_DATA_DPRINTF(("%02X %02X ",
					    *dptr & 0xFF, (*dptr>>8) & 0xFF));
					dptr++;
				}

				write_ptr += i*2;
				if (to_write % 2 != 0) {
					DPRINTF(("dme_prepare: "
						"to_write %% 2: %d\n",
						to_write % 2));
					left_over_count = 1;
					/* XXX: Does this depend on
					 * the endianess?
					 */
					left_over_buf = *write_ptr;
					
					write_ptr++;
					to_write--;
					DPRINTF(("dme_prepare: "
						"to_write (after): %d\n",
						to_write));
					DPRINTF(("dme_prepare: i*2: %d\n",
						i*2));
				}
				to_write -= i*2;
			}
		} /* while(...) */
	} /* for(...) */

	TX_DATA_DPRINTF(("\n"));

	if (length % 2 == 1) {
		panic("dme_prepare: length is not a word-length");
	}

	sc->txready_length = length;
	sc->txready = 1;

	TX_DPRINTF(("dme_prepare: txbusy: %d\ndme_prepare: "
		"txready: %d, txready_length: %d\n",
		sc->txbusy, sc->txready, sc->txready_length));

	m_freem(bufChain);

	TX_DPRINTF(("dme_prepare: Leaving\n"));
}

int
dme_init(struct ifnet *ifp)
{
	int s;
	struct dme_softc *sc = ifp->if_softc;

	dme_stop(ifp, 0);

	s = splnet();

	dme_reset(sc);

	sc->sc_ethercom.ec_if.if_flags |= IFF_RUNNING;
	sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;
	sc->sc_ethercom.ec_if.if_timer = 0;

	splx(s);

	return 0;
}

int
dme_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct dme_softc *sc = ifp->if_softc;
	struct ifreq *ifr = data;
	int s, error = 0;

	s = splnet();

	switch(cmd) {
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	splx(s);
	return error;
}

void
dme_stop(struct ifnet *ifp, int disable)
{
	struct dme_softc *sc = ifp->if_softc;

	/* Not quite sure what to do when called with disable == 0 */
	if (disable) {
		/* Disable RX */
		dme_write(sc, DM9000_RCR, 0x0);
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

int
dme_mediachange(struct ifnet *ifp)
{
	/* TODO: Make this function do something useful. */
	return 0;
}

void
dme_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	/* TODO: Make this function do something useful. */
	struct dme_softc *sc = ifp->if_softc;
	ifmr->ifm_active = sc->sc_media.ifm_cur->ifm_media;

	if (ifp->if_flags & IFF_UP) {
		ifmr->ifm_status = IFM_AVALID | IFM_ACTIVE;
	} else {
		ifmr->ifm_status = 0;
	}
}

void
dme_transmit(struct dme_softc *sc)
{
	uint8_t status;

	TX_DPRINTF(("dme_transmit: PRE: txready: %d, txbusy: %d\n",
		sc->txready, sc->txbusy));

	dme_write(sc, DM9000_TXPLL, sc->txready_length & 0xff);
	dme_write(sc, DM9000_TXPLH, (sc->txready_length >> 8) & 0xff );

	/* Request to send the packet */
	status = dme_read(sc, DM9000_ISR);

	dme_write(sc, DM9000_TCR, DM9000_TCR_TXREQ);

	sc->txready = 0;
	sc->txbusy = 1;
	sc->txready_length = 0;
}

void
dme_receive(struct dme_softc *sc, struct ifnet *ifp)
{
	uint8_t ready = 0x01;

	DPRINTF(("inside dme_receive\n"));

	while (ready == 0x01) {
		/* Packet received, retrieve it */

		/* Read without address increment to get the ready byte without moving past it. */
		bus_space_write_1(sc->sc_iot, sc->sc_ioh,
		    sc->dme_io, DM9000_MRCMDX);
		/* Dummy ready */
		ready = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
		ready = bus_space_read_1(sc->sc_iot, sc->sc_ioh, sc->dme_data);
		ready &= 0x03;	/* we only want bits 1:0 */
		if (ready == 0x01) {
			uint8_t rx_status;

			uint16_t data;
			uint16_t frame_length;
			uint16_t i;
			struct mbuf *m;
			uint16_t *buf;
			int pad;

			/* TODO: Add support for 8-bit and
			 *  32-bit transfer modes.
			 */

			/* Read with address increment. */
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    sc->dme_io, DM9000_MRCMD);
			data = bus_space_read_2(sc->sc_iot, sc->sc_ioh,
			    sc->dme_data);

			rx_status = data & 0xFF;
			frame_length = bus_space_read_2(sc->sc_iot,
			    sc->sc_ioh, sc->dme_data);

			RX_DPRINTF(("dme_receive: "
				"rx_statux: 0x%x, frame_length: %d\n",
				rx_status, frame_length));


			MGETHDR(m, M_DONTWAIT, MT_DATA);
			m->m_pkthdr.rcvif = ifp;
			/* Ensure that we always allocate an even number of
			 * bytes in order to avoid writing beyond the buffer
			 */
			m->m_pkthdr.len = frame_length + (frame_length % 2);
			pad = ALIGN(sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			/* All our frames have the CRC attached */
			m->m_flags |= M_HASFCS;
			if (m->m_pkthdr.len + pad > MHLEN )
				MCLGET(m, M_DONTWAIT);

			m->m_data += pad;
			m->m_len = frame_length + (frame_length % 2);
			buf = mtod(m, uint16_t*);

			RX_DPRINTF(("dme_receive: "));

			for(i=0; i< frame_length; i+=2 ) {
				data = bus_space_read_2(sc->sc_iot,
				    sc->sc_ioh, sc->dme_data);
				if ( (frame_length % 2 != 0) &&
				    (i == frame_length-1) ) {
					data = data & 0xff;
					RX_DPRINTF((" L "));
				}
				*buf = data;
				buf++;
				RX_DATA_DPRINTF(("%02X %02X ", data & 0xff,
					    (data>>8) & 0xff));
			}

			RX_DATA_DPRINTF(("\n"));
			RX_DPRINTF(("Read %d bytes\n", i));

			if (rx_status & (DM9000_RSR_CE | DM9000_RSR_PLE)) {
				/* Error while receiving the packet,
				 * discard it and keep track of counters
				 */
				ifp->if_ierrors++;
				RX_DPRINTF(("dme_receive: "
					"Error reciving packet\n"));
			} else if (rx_status & DM9000_RSR_LCS) {
				ifp->if_collisions++;
			} else {
				if (ifp->if_bpf)
					bpf_mtap(ifp, m);
				ifp->if_ipackets++;
				(*ifp->if_input)(ifp, m);
			}

		} else if (ready != 0x00) {
			/* Should this be logged somehow? */
			DPRINTF(("DM9000: Resetting chip\n"));
			dme_reset(sc);
		}
	}
}

void
dme_reset(struct dme_softc *sc)
{
	uint8_t var;

	/* Enable PHY */
	var = dme_read(sc, DM9000_GPCR);
	dme_write(sc, DM9000_GPCR, var | DM9000_GPCR_GPIO0_OUT);
	var = dme_read(sc, DM9000_GPR);
	dme_write(sc, DM9000_GPR, var & ~DM9000_GPR_PHY_PWROFF);

	/* Reset the DM9000 twice, as describe din section 5.2 of the
	 * Application Notes
	 */
	dme_write(sc, DM9000_NCR,
	    DM9000_NCR_RST | DM9000_NCR_LBK_MAC_INTERNAL);
	
	delay(20);
	dme_write(sc, DM9000_NCR, 0x0);
	dme_write(sc, DM9000_NCR,
	    DM9000_NCR_RST | DM9000_NCR_LBK_MAC_INTERNAL);
	
	delay(20);
	dme_write(sc, DM9000_NCR, 0x0);

	/* Select internal PHY, no wakeup event, no collosion mode,
	 * normal loopback mode, and no full duplex mode
	 */
	dme_write(sc, DM9000_NCR, DM9000_NCR_LBK_NORMAL );

	/* Will clear TX1END, TX2END, and WAKEST fields by reading DM9000_NSR*/
	dme_read(sc, DM9000_NSR);

	/* Enable wraparound of read/write pointer, packet received latch,
	 * and packet transmitted latch.
	 */
	dme_write(sc, DM9000_IMR,
	    DM9000_IMR_PAR | DM9000_IMR_PRM | DM9000_IMR_PTM);

	/* Enable RX without watchdog */
	dme_write(sc, DM9000_RCR, DM9000_RCR_RXEN | DM9000_RCR_WTDIS);

	sc->txbusy = 0;
	sc->txready = 0;
}
