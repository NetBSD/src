/* $NetBSD: seeq8005.c,v 1.12 2001/03/24 23:31:06 bjh21 Exp $ */

/*
 * Copyright (c) 2000 Ben Harris
 * Copyright (c) 1995-1998 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
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
 * seeq8005.c - SEEQ 8005 device driver
 */
/*
 * This driver currently supports the following chip:
 * SEEQ 8005 Advanced Ethernet Data Link Controller
 */
/*
 * More information on the 8004 and 8005 AEDLC controllers can be found in
 * the SEEQ Technology Inc 1992 Data Comm Devices data book.
 *
 * This data book may no longer be available as these are rather old chips
 * (1991 - 1993)
 */
/*
 * This driver is based on the arm32 ea(4) driver, hence the names of many
 * of the functions.
 */
/*
 * Bugs/possible improvements:
 *	- Does not currently support DMA
 *	- Does not transmit multiple packets in one go
 *	- Does not support 8-bit busses
 */

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/types.h>
#include <sys/param.h>

__RCSID("$NetBSD: seeq8005.c,v 1.12 2001/03/24 23:31:06 bjh21 Exp $");

#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include "bpfilter.h"
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/ic/seeq8005reg.h>
#include <dev/ic/seeq8005var.h>

/*#define SEEQ_DEBUG*/

/* for debugging convenience */
#ifdef SEEQ_DEBUG
#define SEEQ_DEBUG_MISC		1
#define SEEQ_DEBUG_TX		2
#define SEEQ_DEBUG_RX		4
#define SEEQ_DEBUG_PKT		8
#define SEEQ_DEBUG_TXINT	16
#define SEEQ_DEBUG_RXINT	32
int seeq_debug = 0;
#define DPRINTF(f, x) { if (seeq_debug & (f)) printf x; }
#else
#define DPRINTF(f, x)
#endif
#define dprintf(x) DPRINTF(SEEQ_DEBUG_MISC, x)

#define	SEEQ_TX_BUFFER_SIZE		0x800		/* (> MAX_ETHER_LEN) */

/*
 * prototypes
 */

static int ea_init(struct ifnet *);
static int ea_ioctl(struct ifnet *, u_long, caddr_t);
static void ea_start(struct ifnet *);
static void ea_watchdog(struct ifnet *);
static void ea_chipreset(struct seeq8005_softc *);
static void ea_ramtest(struct seeq8005_softc *);
static int ea_stoptx(struct seeq8005_softc *);
static int ea_stoprx(struct seeq8005_softc *);
static void ea_stop(struct ifnet *, int);
static void ea_await_fifo_empty(struct seeq8005_softc *);
static void ea_await_fifo_full(struct seeq8005_softc *);
static void ea_writebuf(struct seeq8005_softc *, u_char *, int, size_t);
static void ea_readbuf(struct seeq8005_softc *, u_char *, int, size_t);
static void ea_select_buffer(struct seeq8005_softc *, int);
static void ea_set_address(struct seeq8005_softc *, int, const u_int8_t *);
static void ea_read(struct seeq8005_softc *, int, int);
static struct mbuf *ea_get(struct seeq8005_softc *, int, int, struct ifnet *);
static void ea_getpackets(struct seeq8005_softc *);
static void eatxpacket(struct seeq8005_softc *);
static int ea_writembuf(struct seeq8005_softc *, struct mbuf *, int);
static void ea_mc_reset(struct seeq8005_softc *);
static void ea_mc_reset_8004(struct seeq8005_softc *);
static void ea_mc_reset_8005(struct seeq8005_softc *);
static int ea_mediachange(struct ifnet *);
static void ea_mediastatus(struct ifnet *, struct ifmediareq *);


/*
 * Attach chip.
 */

void
seeq8005_attach(struct seeq8005_softc *sc, const u_int8_t *myaddr, int *media,
    int nmedia, int defmedia)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int id;

	KASSERT(myaddr != NULL);
	printf(" address %s", ether_sprintf(myaddr));

	/* Stop the board. */

	ea_chipreset(sc);

	/* Get the product ID */
	
	ea_select_buffer(sc, SEEQ_BUFCODE_PRODUCTID);
	id = bus_space_read_2(sc->sc_iot, sc->sc_ioh, SEEQ_BUFWIN);

	switch (id & SEEQ_PRODUCTID_MASK) {
	case SEEQ_PRODUCTID_8004:
		sc->sc_variant = SEEQ_8004;
		break;
	default:	/* XXX */
		sc->sc_variant = SEEQ_8005;
		break;
	}

	switch (sc->sc_variant) {
	case SEEQ_8004:
		printf(", SEEQ80C04 rev %x\n",
		    id & SEEQ_PRODUCTID_REV_MASK);
		break;
	case SEEQ_8005:
		if (id != 0xff)
			printf(", SEEQ8005 rev %x\n", id);
		else
			printf(", SEEQ8005\n");
		break;
	default:
		printf(", Unknown ethernet controller\n");
		return;
	}

	/* Both the 8004 and 8005 are designed for 64K Buffer memory */
	sc->sc_buffersize = SEEQ_MAX_BUFFER_SIZE;

	/*
	 * Set up tx and rx buffers.
	 *
	 * We use approximately a quarter of the packet memory for TX
	 * buffers and the rest for RX buffers
	 */
	/* sc->sc_tx_bufs = sc->sc_buffersize / SEEQ_TX_BUFFER_SIZE / 4; */
	sc->sc_tx_bufs = 1;
	sc->sc_tx_bufsize = sc->sc_tx_bufs * SEEQ_TX_BUFFER_SIZE;
	sc->sc_rx_bufsize = sc->sc_buffersize - sc->sc_tx_bufsize;
	sc->sc_enabled = 0;

	/* Test the RAM */
	ea_ramtest(sc);

	printf("%s: %dKB packet memory, txbuf=%dKB (%d buffers), rxbuf=%dKB",
	    sc->sc_dev.dv_xname, sc->sc_buffersize >> 10,
	    sc->sc_tx_bufsize >> 10, sc->sc_tx_bufs, sc->sc_rx_bufsize >> 10); 

	/* Initialise ifnet structure. */

	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = ea_start;
	ifp->if_ioctl = ea_ioctl;
	ifp->if_init = ea_init;
	ifp->if_stop = ea_stop;
	ifp->if_watchdog = ea_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_NOTRAILERS;
	if (sc->sc_variant == SEEQ_8004)
		ifp->if_flags |= IFF_SIMPLEX;
	IFQ_SET_READY(&ifp->if_snd);

	/* Initialize media goo. */
	ifmedia_init(&sc->sc_media, 0, ea_mediachange, ea_mediastatus);
	if (media != NULL) {
		int i;

		for (i = 0; i < nmedia; i++)
			ifmedia_add(&sc->sc_media, media[i], 0, NULL);
		ifmedia_set(&sc->sc_media, defmedia);
	} else {
		ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_MANUAL);
	}

	/* Now we can attach the interface. */

	if_attach(ifp);
	ether_ifattach(ifp, myaddr);

	printf("\n");
}

/*
 * Media change callback.
 */
static int
ea_mediachange(struct ifnet *ifp)
{
	struct seeq8005_softc *sc = ifp->if_softc;

	if (sc->sc_mediachange)
		return ((*sc->sc_mediachange)(sc));
	return (EINVAL);
}

/*
 * Media status callback.
 */
static void
ea_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct seeq8005_softc *sc = ifp->if_softc;

	if (sc->sc_enabled == 0) {
		ifmr->ifm_active = IFM_ETHER | IFM_NONE;
		ifmr->ifm_status = 0;
		return;
	}

	if (sc->sc_mediastatus)
		(*sc->sc_mediastatus)(sc, ifmr);
}

/*
 * Test the RAM on the ethernet card.
 */

void
ea_ramtest(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int loop;
	u_int sum = 0;

/*	dprintf(("ea_ramtest()\n"));*/

	/*
	 * Test the buffer memory on the board.
	 * Write simple pattens to it and read them back.
	 */

	/* Set up the whole buffer RAM for writing */

	ea_select_buffer(sc, SEEQ_BUFCODE_TX_EAP);
	bus_space_write_2(iot, ioh, SEEQ_BUFWIN, (SEEQ_MAX_BUFFER_SIZE >> 8) - 1);
	bus_space_write_2(iot, ioh, SEEQ_TX_PTR, 0x0000);
	bus_space_write_2(iot, ioh, SEEQ_RX_PTR, SEEQ_MAX_BUFFER_SIZE - 2);

#define SEEQ_RAMTEST_LOOP(value)						\
do {									\
	/* Set the write start address and write a pattern */		\
	ea_writebuf(sc, NULL, 0x0000, 0);				\
	for (loop = 0; loop < SEEQ_MAX_BUFFER_SIZE; loop += 2)		\
		bus_space_write_2(iot, ioh, SEEQ_BUFWIN, (value));	\
									\
	/* Set the read start address and verify the pattern */		\
	ea_readbuf(sc, NULL, 0x0000, 0);				\
	for (loop = 0; loop < SEEQ_MAX_BUFFER_SIZE; loop += 2)		\
		if (bus_space_read_2(iot, ioh, SEEQ_BUFWIN) != (value)) \
			++sum;						\
	if (sum != 0)							\
		dprintf(("sum=%d\n", sum));				\
} while (/*CONSTCOND*/0)

	SEEQ_RAMTEST_LOOP(loop);
	SEEQ_RAMTEST_LOOP(loop ^ (SEEQ_MAX_BUFFER_SIZE - 1));
	SEEQ_RAMTEST_LOOP(0xaa55);
	SEEQ_RAMTEST_LOOP(0x55aa);

	/* Report */

	if (sum > 0)
		printf("%s: buffer RAM failed self test, %d faults\n",
		       sc->sc_dev.dv_xname, sum);
}


/*
 * Stop the tx interface.
 *
 * Returns 0 if the tx was already stopped or 1 if it was active
 */

static int
ea_stoptx(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	int status;

	DPRINTF(SEEQ_DEBUG_TX, ("seeq_stoptx()\n"));

	sc->sc_enabled = 0;

	status = bus_space_read_2(iot, ioh, SEEQ_STATUS);
	if (!(status & SEEQ_STATUS_TX_ON))
		return 0;

	/* Stop any tx and wait for confirmation */
	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
			  sc->sc_command | SEEQ_CMD_TX_OFF);

	timeout = 20000;
	do {
		status = bus_space_read_2(iot, ioh, SEEQ_STATUS);
		delay(1);
	} while ((status & SEEQ_STATUS_TX_ON) && --timeout > 0);
 	if (timeout == 0)
		log(LOG_ERR, "%s: timeout waiting for tx termination\n",
		    sc->sc_dev.dv_xname);

	/* Clear any pending tx interrupt */
	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
		   sc->sc_command | SEEQ_CMD_TX_INTACK);
	return 1;
}


/*
 * Stop the rx interface.
 *
 * Returns 0 if the tx was already stopped or 1 if it was active
 */

static int
ea_stoprx(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	int status;

	DPRINTF(SEEQ_DEBUG_RX, ("seeq_stoprx()\n"));

	status = bus_space_read_2(iot, ioh, SEEQ_STATUS);
	if (!(status & SEEQ_STATUS_RX_ON))
		return 0;

	/* Stop any rx and wait for confirmation */

	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
			  sc->sc_command | SEEQ_CMD_RX_OFF);

	timeout = 20000;
	do {
		status = bus_space_read_2(iot, ioh, SEEQ_STATUS);
	} while ((status & SEEQ_STATUS_RX_ON) && --timeout > 0);
	if (timeout == 0)
		log(LOG_ERR, "%s: timeout waiting for rx termination\n",
		    sc->sc_dev.dv_xname);

	/* Clear any pending rx interrupt */

	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
		   sc->sc_command | SEEQ_CMD_RX_INTACK);
	return 1;
}


/*
 * Stop interface.
 * Stop all IO and shut the interface down
 */

static void
ea_stop(struct ifnet *ifp, int disable)
{
	struct seeq8005_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	
	DPRINTF(SEEQ_DEBUG_MISC, ("seeq_stop()\n"));

	/* Stop all IO */
	ea_stoptx(sc);
	ea_stoprx(sc);

	/* Disable rx and tx interrupts */
	sc->sc_command &= (SEEQ_CMD_RX_INTEN | SEEQ_CMD_TX_INTEN);

	/* Clear any pending interrupts */
	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
			  sc->sc_command | SEEQ_CMD_RX_INTACK |
			  SEEQ_CMD_TX_INTACK | SEEQ_CMD_DMA_INTACK |
			  SEEQ_CMD_BW_INTACK);

	if (sc->sc_variant == SEEQ_8004) {
		/* Put the chip to sleep */
		ea_select_buffer(sc, SEEQ_BUFCODE_CONFIG3);
		bus_space_write_2(iot, ioh, SEEQ_BUFWIN,
		    sc->sc_config3 | SEEQ_CFG3_SLEEP);
	}

	/* Cancel any watchdog timer */
       	sc->sc_ethercom.ec_if.if_timer = 0;
}


/*
 * Reset the chip
 * Following this the software registers are reset
 */

static void
ea_chipreset(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	DPRINTF(SEEQ_DEBUG_MISC, ("seeq_chipreset()\n"));

	/* Reset the controller. Min of 4us delay here */

	bus_space_write_2(iot, ioh, SEEQ_CONFIG2, SEEQ_CFG2_RESET);
	delay(4);

	sc->sc_command = 0;
	sc->sc_config1 = 0;
	sc->sc_config2 = 0;
	sc->sc_config3 = 0;
}


/*
 * If the DMA FIFO's in write mode, wait for it to empty.  Needed when
 * switching the FIFO from write to read.  We also use it when changing
 * the address for writes.
 */
static void
ea_await_fifo_empty(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;
	
	timeout = 20000;
	if ((bus_space_read_2(iot, ioh, SEEQ_STATUS) &
	     SEEQ_STATUS_FIFO_DIR) != 0)
		return; /* FIFO is reading anyway. */
	while ((bus_space_read_2(iot, ioh, SEEQ_STATUS) &
		SEEQ_STATUS_FIFO_EMPTY) == 0 &&
	       --timeout > 0)
		continue;
}

/*
 * Wait for the DMA FIFO to fill before reading from it.
 */
static void
ea_await_fifo_full(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int timeout;

	timeout = 20000;
	while ((bus_space_read_2(iot, ioh, SEEQ_STATUS) &
		SEEQ_STATUS_FIFO_FULL) == 0 &&
	       --timeout > 0)
		continue;
}

/*
 * write to the buffer memory on the interface
 *
 * The buffer address is set to ADDR.
 * If len != 0 then data is copied from the address starting at buf
 * to the interface buffer.
 * BUF must be usable as a u_int16_t *.
 * If LEN is odd, it must be safe to overwrite one extra byte.
 */

static void
ea_writebuf(struct seeq8005_softc *sc, u_char *buf, int addr, size_t len)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	dprintf(("writebuf: st=%04x\n",
		 bus_space_read_2(iot, ioh, SEEQ_STATUS)));

#ifdef DIAGNOSTIC
	if (__predict_false(!ALIGNED_POINTER(buf, u_int16_t)))
		panic("%s: unaligned writebuf", sc->sc_dev.dv_xname);
#endif
	if (__predict_false(addr >= SEEQ_MAX_BUFFER_SIZE))
		panic("%s: writebuf out of range", sc->sc_dev.dv_xname);

	/* Assume that copying too much is safe. */
	if (len % 2 != 0)
		len++;

	if (addr != -1) {
		ea_await_fifo_empty(sc);

		ea_select_buffer(sc, SEEQ_BUFCODE_LOCAL_MEM);
		bus_space_write_2(iot, ioh, SEEQ_COMMAND,
		    sc->sc_command | SEEQ_CMD_FIFO_WRITE);
		bus_space_write_2(iot, ioh, SEEQ_DMA_ADDR, addr);
	}

	if (len > 0)
		bus_space_write_multi_2(iot, ioh, SEEQ_BUFWIN,
					(u_int16_t *)buf, len / 2);
	/* Leave FIFO to empty in the background */
}


/*
 * read from the buffer memory on the interface
 *
 * The buffer address is set to ADDR.
 * If len != 0 then data is copied from the interface buffer to the
 * address starting at buf.
 * BUF must be usable as a u_int16_t *.
 * If LEN is odd, it must be safe to overwrite one extra byte.
 */

static void
ea_readbuf(struct seeq8005_softc *sc, u_char *buf, int addr, size_t len)
{

	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	dprintf(("readbuf: st=%04x addr=%04x len=%d\n",
		 bus_space_read_2(iot, ioh, SEEQ_STATUS), addr, len));

#ifdef DIAGNOSTIC
	if (!ALIGNED_POINTER(buf, u_int16_t))
		panic("%s: unaligned readbuf", sc->sc_dev.dv_xname);
#endif
	if (addr >= SEEQ_MAX_BUFFER_SIZE)
		panic("%s: writebuf out of range", sc->sc_dev.dv_xname);

	/* Assume that copying too much is safe. */
	if (len % 2 != 0)
		len++;

	if (addr != -1) {
		ea_await_fifo_empty(sc);

		ea_select_buffer(sc, SEEQ_BUFCODE_LOCAL_MEM);
		bus_space_write_2(iot, ioh, SEEQ_DMA_ADDR, addr);
		bus_space_write_2(iot, ioh, SEEQ_COMMAND,
		    sc->sc_command | SEEQ_CMD_FIFO_READ);

		ea_await_fifo_full(sc);
	}

	if (len > 0)
		bus_space_read_multi_2(iot, ioh, SEEQ_BUFWIN,
				       (u_int16_t *)buf, len / 2);
}

static void
ea_select_buffer(struct seeq8005_softc *sc, int bufcode)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEEQ_CONFIG1,
			  sc->sc_config1 | bufcode);
}

/* Must be called at splnet */
static void
ea_set_address(struct seeq8005_softc *sc, int which, u_int8_t const *ea)
{
	int i;

	ea_select_buffer(sc, SEEQ_BUFCODE_STATION_ADDR0 + which);
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEEQ_BUFWIN,
				  ea[i]);
}

/*
 * Initialize interface.
 *
 * This should leave the interface in a state for packet reception and
 * transmission.
 */

static int
ea_init(struct ifnet *ifp)
{
	struct seeq8005_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s;

	dprintf(("ea_init()\n"));

	s = splnet();

	/* First, reset the board. */

	ea_chipreset(sc);

	/* Set up defaults for the registers */

	sc->sc_command = 0;
	sc->sc_config1 = 0;
#if BYTE_ORDER == BIG_ENDIAN
	sc->sc_config2 = SEEQ_CFG2_BYTESWAP;
#else
	sc->sc_config2 = 0;
#endif
	sc->sc_config3 = 0;

	bus_space_write_2(iot, ioh, SEEQ_COMMAND, sc->sc_command);
	bus_space_write_2(iot, ioh, SEEQ_CONFIG1, sc->sc_config1);
	bus_space_write_2(iot, ioh, SEEQ_CONFIG2, sc->sc_config2);
	if (sc->sc_variant == SEEQ_8004) {
		ea_select_buffer(sc, SEEQ_BUFCODE_CONFIG3);
		bus_space_write_2(iot, ioh, SEEQ_BUFWIN, sc->sc_config3);
	}

	/* Write the station address - the receiver must be off */
	ea_set_address(sc, 0, LLADDR(ifp->if_sadl));

	/* Split board memory into Rx and Tx. */
	ea_select_buffer(sc, SEEQ_BUFCODE_TX_EAP);
	bus_space_write_2(iot, ioh, SEEQ_BUFWIN, (sc->sc_tx_bufsize>> 8) - 1);

	if (sc->sc_variant == SEEQ_8004)
		sc->sc_config2 |= SEEQ_CFG2_RX_TX_DISABLE;

	/* Configure rx. */
	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_config1 = SEEQ_CFG1_PROMISCUOUS;
	else if (ifp->if_flags & IFF_ALLMULTI)
		sc->sc_config1 = SEEQ_CFG1_MULTICAST;
	else
		sc->sc_config1 = SEEQ_CFG1_BROADCAST;
	sc->sc_config1 |= SEEQ_CFG1_STATION_ADDR0;
	bus_space_write_2(iot, ioh, SEEQ_CONFIG1, sc->sc_config1);

	/* Setup the Rx pointers */
	sc->sc_rx_ptr = sc->sc_tx_bufsize;

	bus_space_write_2(iot, ioh, SEEQ_RX_PTR, sc->sc_rx_ptr);
	bus_space_write_2(iot, ioh, SEEQ_RX_END, sc->sc_rx_ptr >> 8);


	/* Place a NULL header at the beginning of the receive area */
	ea_writebuf(sc, NULL, sc->sc_rx_ptr, 0);
		
	bus_space_write_2(iot, ioh, SEEQ_BUFWIN, 0x0000);
	bus_space_write_2(iot, ioh, SEEQ_BUFWIN, 0x0000);


	/* Configure TX. */
	dprintf(("Configuring tx...\n"));

	bus_space_write_2(iot, ioh, SEEQ_TX_PTR, 0x0000);

	sc->sc_config2 |= SEEQ_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, SEEQ_CONFIG2, sc->sc_config2);

	/* Reset tx buffer pointers */
	sc->sc_tx_cur = 0;
	sc->sc_tx_used = 0;
	sc->sc_tx_next = 0;

	/* Place a NULL header at the beginning of the transmit area */
	ea_writebuf(sc, NULL, 0x0000, 0);
		
	bus_space_write_2(iot, ioh, SEEQ_BUFWIN, 0x0000);
	bus_space_write_2(iot, ioh, SEEQ_BUFWIN, 0x0000);

	sc->sc_command |= SEEQ_CMD_TX_INTEN;
	bus_space_write_2(iot, ioh, SEEQ_COMMAND, sc->sc_command);

	/* Turn on Rx */
	sc->sc_command |= SEEQ_CMD_RX_INTEN;
	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
			  sc->sc_command | SEEQ_CMD_RX_ON);

	/* TX_ON gets set by ea_txpacket when there's something to transmit. */


	/* Set flags appropriately. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	sc->sc_enabled = 1;

	/* And start output. */
	ea_start(ifp);

	splx(s);
	return 0;
}

/*
 * Start output on interface. Get datagrams from the queue and output them,
 * giving the receiver a chance between datagrams. Call only from splnet or
 * interrupt level!
 */

static void
ea_start(struct ifnet *ifp)
{
	struct seeq8005_softc *sc = ifp->if_softc;
	int s;

	s = splnet();
#ifdef SEEQ_TX_DEBUG
	dprintf(("ea_start()...\n"));
#endif

	/* Don't do anything if output is active. */

	if (ifp->if_flags & IFF_OACTIVE)
		return;

	/* Mark interface as output active */
	
	ifp->if_flags |= IFF_OACTIVE;

	/* tx packets */

	eatxpacket(sc);
	splx(s);
}


/*
 * Transfer a packet to the interface buffer and start transmission
 *
 * Called at splnet()
 */
 
void
eatxpacket(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct mbuf *m0;
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;

	/* Dequeue the next packet. */
	IFQ_DEQUEUE(&ifp->if_snd, m0);

	/* If there's nothing to send, return. */
	if (!m0) {
		ifp->if_flags &= ~IFF_OACTIVE;
		sc->sc_config2 |= SEEQ_CFG2_OUTPUT;
		bus_space_write_2(iot, ioh, SEEQ_CONFIG2, sc->sc_config2);
#ifdef SEEQ_TX_DEBUG
		dprintf(("tx finished\n"));
#endif
		return;
	}

#if NBPFILTER > 0
	/* Give the packet to the bpf, if any. */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

#ifdef SEEQ_TX_DEBUG
	dprintf(("Tx new packet\n"));
#endif

	sc->sc_config2 &= ~SEEQ_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, SEEQ_CONFIG2, sc->sc_config2);

	ea_writembuf(sc, m0, 0x0000);
	m_freem(m0);

	bus_space_write_2(iot, ioh, SEEQ_TX_PTR, 0x0000);

/*	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, SEEQ_STATUS)));*/

#ifdef SEEQ_PACKET_DEBUG
	ea_dump_buffer(sc, 0);
#endif


	/* Now transmit the datagram. */
/*	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, SEEQ_STATUS)));*/
	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
			  sc->sc_command | SEEQ_CMD_TX_ON);
#ifdef SEEQ_TX_DEBUG
	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, SEEQ_STATUS)));
	dprintf(("tx: queued\n"));
#endif
}

/*
 * Copy a packet from an mbuf to the transmit buffer on the card.
 *
 * Puts a valid Tx header at the start of the packet, and a null header at
 * the end.
 */
static int
ea_writembuf(struct seeq8005_softc *sc, struct mbuf *m0, int bufstart)
{
	struct mbuf *m;
	int len, nextpacket;
	u_int8_t hdr[4];

	/*
	 * Copy the datagram to the packet buffer.
	 */
	ea_writebuf(sc, NULL, bufstart + 4, 0);

	len = 0;
	for (m = m0; m; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		ea_writebuf(sc, mtod(m, caddr_t), -1, m->m_len);
		len += m->m_len;
	}

	/* If packet size is odd round up to the next 16 bit boundry */
	if (len % 2)
		++len;

	len = max(len, ETHER_MIN_LEN);

	ea_writebuf(sc, NULL, bufstart + 4 + len, 0);
	/* Follow it with a NULL packet header */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEEQ_BUFWIN, 0x0000);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEEQ_BUFWIN, 0x0000);

	/* Ok we now have a packet len bytes long in our packet buffer */
	DPRINTF(SEEQ_DEBUG_TX, ("seeq_writembuf: length=%d\n", len));

	/* Write the packet header */
	nextpacket = len + 4;
	hdr[0] = (nextpacket >> 8) & 0xff;
	hdr[1] = nextpacket & 0xff;
	hdr[2] = SEEQ_PKTCMD_TX | SEEQ_PKTCMD_DATA_FOLLOWS |
		SEEQ_TXCMD_XMIT_SUCCESS_INT | SEEQ_TXCMD_COLLISION_INT;
	hdr[3] = 0; /* Status byte -- will be update by hardware. */
	ea_writebuf(sc, hdr, 0x0000, 4);

	return len;
}

/*
 * Ethernet controller interrupt.
 */

int
seeq8005intr(void *arg)
{
	struct seeq8005_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int status, handled;
	u_int8_t txhdr[4];
	u_int txstatus;

	handled = 0;
	dprintf(("eaintr: "));


	/* Get the controller status */
	status = bus_space_read_2(iot, ioh, SEEQ_STATUS);
        dprintf(("st=%04x ", status));	


	/* Tx interrupt ? */
	if (status & SEEQ_STATUS_TX_INT) {
		dprintf(("txint "));
		handled = 1;

		/* Acknowledge the interrupt */
		bus_space_write_2(iot, ioh, SEEQ_COMMAND,
				  sc->sc_command | SEEQ_CMD_TX_INTACK);

		ea_readbuf(sc, txhdr, 0x0000, 4);

#ifdef SEEQ_TX_DEBUG		
		dprintf(("txstatus=%02x %02x %02x %02x\n",
			 txhdr[0], txhdr[1], txhdr[2], txhdr[3]));
#endif
		txstatus = txhdr[3];

		/*
		 * If SEEQ_TXSTAT_COLLISION is set then we received at least
		 * one collision. On the 8004 we can find out exactly how many
		 * collisions occurred.
		 *
		 * The SEEQ_PKTSTAT_DONE will be set if the transmission has
		 * completed.
		 *
		 * If SEEQ_TXSTAT_COLLISION16 is set then 16 collisions
		 * occurred and the packet transmission was aborted.
		 * This situation is untested as present.
		 *
		 * The SEEQ_TXSTAT_BABBLE should never be set and is untested
		 * as we should never xmit oversized packets.
		 */
		if (txstatus & SEEQ_TXSTAT_COLLISION) {
			switch (sc->sc_variant) {
			case SEEQ_8004: {
				int colls;

				/*
				 * The 8004 contains a 4 bit collision count
				 * in the status register.
				 */

				/* This appears to be broken on 80C04.AE */
/*				ifp->if_collisions +=
				    (txstatus >> SEEQ_TXSTAT_COLLISIONS_SHIFT)
				    & SEEQ_TXSTAT_COLLISION_MASK;*/
				    
				/* Use the TX Collision register */
				ea_select_buffer(sc, SEEQ_BUFCODE_TX_COLLS);
				colls = bus_space_read_1(iot, ioh,
				    SEEQ_BUFWIN);
				ifp->if_collisions += colls;
				break;
			}
			case SEEQ_8005:
				/* We known there was at least 1 collision */
				ifp->if_collisions++;
				break;
			}
		} else if (txstatus & SEEQ_TXSTAT_COLLISION16) {
			printf("seeq_intr: col16 %x\n", txstatus);
			ifp->if_collisions += 16;
			ifp->if_oerrors++;
		} else if (txstatus & SEEQ_TXSTAT_BABBLE) {
			ifp->if_oerrors++;
		}

		/* Have we completed transmission on the packet ? */
		if (txstatus & SEEQ_PKTSTAT_DONE) {
			/* Clear watchdog timer. */
			ifp->if_timer = 0;
			ifp->if_flags &= ~IFF_OACTIVE;

			/* Update stats */
			ifp->if_opackets++;

			/* Tx next packet */

			eatxpacket(sc);
		}

	}


	/* Rx interrupt ? */
	if (status & SEEQ_STATUS_RX_INT) {
		dprintf(("rxint "));
		handled = 1;

		/* Acknowledge the interrupt */
		bus_space_write_2(iot, ioh, SEEQ_COMMAND,
				  sc->sc_command | SEEQ_CMD_RX_INTACK);

		/* Processes the received packets */
		ea_getpackets(sc);


#if 0
		/* Make sure the receiver is on */
		if ((status & SEEQ_STATUS_RX_ON) == 0) {
			bus_space_write_2(iot, ioh, SEEQ_COMMAND,
					  sc->sc_command | SEEQ_CMD_RX_ON);
			printf("rxintr: rx is off st=%04x\n",status);
		}
#endif
	}

#ifdef SEEQ_DEBUG
	status = bus_space_read_2(iot, ioh, SEEQ_STATUS);
        dprintf(("st=%04x\n", status));
#endif

	return handled;
}


void
ea_getpackets(struct seeq8005_softc *sc)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int addr;
	int len;
	int ctrl;
	int ptr;
	int pack;
	int status;
	u_int8_t rxhdr[4];
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;


	/* We start from the last rx pointer position */
	addr = sc->sc_rx_ptr;
	sc->sc_config2 &= ~SEEQ_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, SEEQ_CONFIG2, sc->sc_config2);

	do {
		/* Read rx header */
		ea_readbuf(sc, rxhdr, addr, 4);
		
		/* Split the packet header */
		ptr = (rxhdr[0] << 8) | rxhdr[1];
		ctrl = rxhdr[2];
		status = rxhdr[3];

#ifdef SEEQ_RX_DEBUG
		dprintf(("addr=%04x ptr=%04x ctrl=%02x status=%02x\n",
			 addr, ptr, ctrl, status));
#endif


		/* Zero packet ptr ? then must be null header so exit */
		if (ptr == 0) break;


		/* Get packet length */
       		len = (ptr - addr) - 4;

		if (len < 0)
			len += sc->sc_rx_bufsize;

#ifdef SEEQ_RX_DEBUG
		dprintf(("len=%04x\n", len));
#endif


		/* Has the packet rx completed ? if not then exit */
		if ((status & SEEQ_PKTSTAT_DONE) == 0)
			break;

		/*
		 * Did we have any errors? then note error and go to
		 * next packet
		 */
		if (__predict_false(status & SEEQ_RXSTAT_ERROR_MASK)) {
			++ifp->if_ierrors;
			log(LOG_WARNING,
			    "%s: rx packet error (%02x) - dropping packet\n",
			    sc->sc_dev.dv_xname, status & 0x0f);
			sc->sc_config2 |= SEEQ_CFG2_OUTPUT;
			bus_space_write_2(iot, ioh, SEEQ_CONFIG2,
					  sc->sc_config2);
			ea_init(ifp);
			return;
		}

		/*
		 * Is the packet too big ? - this will probably be trapped
		 * above as a receive error
		 */
		if (__predict_false(len > (ETHER_MAX_LEN - ETHER_CRC_LEN))) {
			++ifp->if_ierrors;
			log(LOG_WARNING, "%s: rx packet size error len=%d\n",
			    sc->sc_dev.dv_xname, len);
			sc->sc_config2 |= SEEQ_CFG2_OUTPUT;
			bus_space_write_2(iot, ioh, SEEQ_CONFIG2,
					  sc->sc_config2);
			ea_init(ifp);
			return;
		}

		ifp->if_ipackets++;
		/* Pass data up to upper levels. */
		ea_read(sc, addr + 4, len);

		addr = ptr;
		++pack;
	} while (len != 0);

	sc->sc_config2 |= SEEQ_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, SEEQ_CONFIG2, sc->sc_config2);

#ifdef SEEQ_RX_DEBUG
	dprintf(("new rx ptr=%04x\n", addr));
#endif


	/* Store new rx pointer */
	sc->sc_rx_ptr = addr;
	bus_space_write_2(iot, ioh, SEEQ_RX_END, sc->sc_rx_ptr >> 8);

	/* Make sure the receiver is on */
	bus_space_write_2(iot, ioh, SEEQ_COMMAND,
			  sc->sc_command | SEEQ_CMD_RX_ON);

}


/*
 * Pass a packet up to the higher levels.
 */

static void
ea_read(struct seeq8005_softc *sc, int addr, int len)
{
	struct mbuf *m;
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;

	/* Pull packet off interface. */
	m = ea_get(sc, addr, len, ifp);
	if (m == 0)
		return;

#ifdef SEEQ_RX_DEBUG
	dprintf(("%s-->", ether_sprintf(eh->ether_shost)));
	dprintf(("%s\n", ether_sprintf(eh->ether_dhost)));
#endif

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to bpf.
	 */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m);
#endif

	(*ifp->if_input)(ifp, m);
}

/*
 * Pull read data off a interface.  Len is length of data, with local net
 * header stripped.  We copy the data into mbufs.  When full cluster sized
 * units are present we copy into clusters.
 */

struct mbuf *
ea_get(struct seeq8005_softc *sc, int addr, int totlen, struct ifnet *ifp)
{
        struct mbuf *top, **mp, *m;
        int len;
        u_int cp, epkt;

        cp = addr;
        epkt = cp + totlen;

        MGETHDR(m, M_DONTWAIT, MT_DATA);
        if (m == 0)
                return 0;
        m->m_pkthdr.rcvif = ifp;
        m->m_pkthdr.len = totlen;
        m->m_len = MHLEN;
        top = 0;
        mp = &top;

        while (totlen > 0) {
                if (top) {
                        MGET(m, M_DONTWAIT, MT_DATA);
                        if (m == 0) {
                                m_freem(top);
                                return 0;
                        }
                        m->m_len = MLEN;
                }
                len = min(totlen, epkt - cp);
                if (len >= MINCLSIZE) {
                        MCLGET(m, M_DONTWAIT);
                        if (m->m_flags & M_EXT)
                                m->m_len = len = min(len, MCLBYTES);
                        else
                                len = m->m_len;
                } else {
                        /*
                         * Place initial small packet/header at end of mbuf.
                         */
                        if (len < m->m_len) {
                                if (top == 0 && len + max_linkhdr <= m->m_len)
                                        m->m_data += max_linkhdr;
                                m->m_len = len;
                        } else
                                len = m->m_len;
                }
		if (top == 0) {
			/* Make sure the payload is aligned */
			caddr_t newdata = (caddr_t)
			    ALIGN(m->m_data + sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			len -= newdata - m->m_data;
			m->m_len = len;
			m->m_data = newdata;
		}
                ea_readbuf(sc, mtod(m, u_char *),
		    cp < SEEQ_MAX_BUFFER_SIZE ? cp : cp - sc->sc_rx_bufsize,
		    len);
                cp += len;
                *mp = m;
                mp = &m->m_next;
                totlen -= len;
                if (cp == epkt)
                        cp = addr;
        }

        return top;
}

/*
 * Process an ioctl request.  Mostly boilerplate.
 */
static int
ea_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct seeq8005_softc *sc = ifp->if_softc;
	int s, error = 0;

	s = splnet();
	switch (cmd) {

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			ea_mc_reset(sc);
			error = 0;
		}
		break;
	}

	splx(s);
	return error;
}

/* Must be called at splnet() */

static void
ea_mc_reset(struct seeq8005_softc *sc)
{

	switch (sc->sc_variant) {
	case SEEQ_8004:
		ea_mc_reset_8004(sc);
		return;
	case SEEQ_8005:
		ea_mc_reset_8005(sc);
		return;
	}
}

static void
ea_mc_reset_8004(struct seeq8005_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	u_int8_t *cp, c;
	u_int32_t crc;
	int i, len;
	struct ether_multistep step;
	u_int8_t af[8];

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using bits 2 - 7 as an index
	 * into the 64 bit logical address filter.  The high order bits
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	if (ifp->if_flags & IFF_PROMISC) {
		ifp->if_flags |= IFF_ALLMULTI;
		for (i = 0; i < 8; i++)
			af[i] = 0xff;
		return;
	}
	for (i = 0; i < 8; i++)
		af[i] = 0;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
		    sizeof(enm->enm_addrlo)) != 0) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			for (i = 0; i < 8; i++)
				af[i] = 0xff;
			return;
		}
		cp = enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = sizeof(enm->enm_addrlo); --len >= 0;) {
			c = *cp++;
			for (i = 8; --i >= 0;) {
				if (((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01)) {
					crc <<= 1;
					crc ^= 0x04c11db6 | 1;
				} else
					crc <<= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc = (crc >> 2) & 0x3f;

		/* Turn on the corresponding bit in the filter. */
		af[crc >> 3] |= 1 << (crc & 0x7);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;

	ea_select_buffer(sc, SEEQ_BUFCODE_MULTICAST);
		for (i = 0; i < 8; ++i)
			bus_space_write_1(sc->sc_iot, sc->sc_ioh,
			    SEEQ_BUFWIN, af[i]);
}

static void
ea_mc_reset_8005(struct seeq8005_softc *sc)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	int naddr, maxaddrs;

	naddr = 0;
	maxaddrs = 5;
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm != NULL) {
		/* Have we got space? */
		if (naddr >= maxaddrs ||
		    bcmp(enm->enm_addrlo, enm->enm_addrhi, 6) != 0) {
			sc->sc_ethercom.ec_if.if_flags |= IFF_ALLMULTI;
			ea_ioctl(&sc->sc_ethercom.ec_if, SIOCSIFFLAGS, NULL);
			return;
		}
		ea_set_address(sc, 1 + naddr, enm->enm_addrlo);
		sc->sc_config1 |= SEEQ_CFG1_STATION_ADDR1 << naddr;
		naddr++;
		ETHER_NEXT_MULTI(step, enm);
	}
	for (; naddr < maxaddrs; naddr++)
		sc->sc_config1 &= ~(SEEQ_CFG1_STATION_ADDR1 << naddr);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, SEEQ_CONFIG1,
			  sc->sc_config1);
}

/*
 * Device timeout routine.
 *
 * Ok I am not sure exactly how the device timeout should work....
 * Currently what will happens is that that the device timeout is only
 * set when a packet it received. This indicates we are on an active
 * network and thus we should expect more packets. If non arrive in
 * in the timeout period then we reinitialise as we may have jammed.
 * We zero the timeout at this point so that we don't end up with
 * an endless stream of timeouts if the network goes down.
 */

static void
ea_watchdog(struct ifnet *ifp)
{
	struct seeq8005_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;
	dprintf(("ea_watchdog: "));
	dprintf(("st=%04x\n",
		 bus_space_read_2(sc->sc_iot, sc->sc_ioh, SEEQ_STATUS)));

	/* Kick the interface */

	ea_init(ifp);

/*	ifp->if_timer = SEEQ_TIMEOUT;*/
	ifp->if_timer = 0;
}

/* End of if_ea.c */
