/* $NetBSD: seeq8005.c,v 1.6.2.4 2001/01/05 17:35:47 bouyer Exp $ */

/*
 * Copyright (c) 2000 Ben Harris
 * Copyright (c) 1995 Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
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
 * This driver is based on the arm32 ea(4) driver, hence the names of many
 * of the functions.
 */
/*
 * Bugs/possible improvements:
 *	- Does not currently support DMA
 *	- Does not currently support multicasts
 *	- Does not transmit multiple packets in one go
 *	- Does not support big-endian hosts
 *	- Does not support 8-bit busses
 */

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/types.h>
#include <sys/param.h>

__RCSID("$NetBSD: seeq8005.c,v 1.6.2.4 2001/01/05 17:35:47 bouyer Exp $");

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

#ifndef EA_TIMEOUT
#define EA_TIMEOUT	60
#endif

#define EA_TX_BUFFER_SIZE	0x4000
#define EA_RX_BUFFER_SIZE	0xC000

/*#define EA_TX_DEBUG*/
/*#define EA_RX_DEBUG*/
/*#define EA_DEBUG*/
/*#define EA_PACKET_DEBUG*/

/* for debugging convenience */
#ifdef EA_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

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
static void ea_writebuf(struct seeq8005_softc *, u_char *, u_int, size_t);
static void ea_readbuf(struct seeq8005_softc *, u_char *, u_int, size_t);
static void ea_select_buffer(struct seeq8005_softc *, int);
static void ea_set_address(struct seeq8005_softc *, int, const u_int8_t *);
static void earead(struct seeq8005_softc *, int, int);
static struct mbuf *eaget(struct seeq8005_softc *, int, int, struct ifnet *);
static void eagetpackets(struct seeq8005_softc *);
static void eatxpacket(struct seeq8005_softc *);
static void ea_mc_reset(struct seeq8005_softc *);


#ifdef EA_PACKET_DEBUG
void ea_dump_buffer(struct seeq8005_softc *, int);
#endif


#ifdef EA_PACKET_DEBUG
/*
 * Dump the interface buffer
 */

void
ea_dump_buffer(struct seeq8005_softc *sc, u_int offset)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int addr;
	int loop, ctrl, ptr;
	size_t size;
	
	addr = offset;

	do {
		bus_space_write_2(iot, ioh, EA_8005_COMMAND,
				 sc->sc_command | EA_CMD_FIFO_READ);
		bus_space_write_2(iot, ioh, EA_8005_CONFIG1,
				  sc->sc_config1 | EA_BUFCODE_LOCAL_MEM);
		bus_space_write_2(iot, ioh, EA_8005_DMA_ADDR, addr);

		ptr = bus_space_read_2(iot, ioh, EA_8005_BUFWIN);
		ctrl = bus_space_read_2(iot, ioh, EA_8005_BUFWIN);
		ptr = ((ptr & 0xff) << 8) | ((ptr >> 8) & 0xff);

		if (ptr == 0) break;
		size = ptr - addr;

		printf("addr=%04x size=%04x ", addr, size);
		printf("cmd=%02x st=%02x\n", ctrl & 0xff, ctrl >> 8);

		for (loop = 0; loop < size - 4; loop += 2)
			printf("%04x ",
			       bus_space_read_2(iot, ioh, EA_8005_BUFWIN));
		printf("\n");
		addr = ptr;
	} while (size != 0);
}
#endif

/*
 * Attach chip.
 */

void
seeq8005_attach(struct seeq8005_softc *sc, const u_int8_t *myaddr)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int id;

	printf(" address %s", ether_sprintf(myaddr));

	/* Stop the board. */

	ea_chipreset(sc);

	/* Get the product ID */
	
	ea_select_buffer(sc, EA_BUFCODE_PRODUCTID);
	id = bus_space_read_2(sc->sc_iot, sc->sc_ioh, EA_8005_BUFWIN);

	if ((id & 0xf0) == 0xa0) {
		sc->sc_flags |= SEEQ8005_80C04;
		printf(", SEEQ 80C04 rev %02x", id);
	} else
		printf(", SEEQ 8005");

	/* Initialise ifnet structure. */

	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = ea_start;
	ifp->if_ioctl = ea_ioctl;
	ifp->if_init = ea_init;
	ifp->if_stop = ea_stop;
	ifp->if_watchdog = ea_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_NOTRAILERS;
	IFQ_SET_READY(&ifp->if_snd);

	/* Now we can attach the interface. */

	if_attach(ifp);
	ether_ifattach(ifp, myaddr);

	/* Test the RAM */
	ea_ramtest(sc);

	printf("\n");
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

	ea_select_buffer(sc, EA_BUFCODE_TX_EAP);
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, (EA_BUFFER_SIZE >> 8) - 1);
	bus_space_write_2(iot, ioh, EA_8005_TX_PTR, 0x0000);
	bus_space_write_2(iot, ioh, EA_8005_RX_PTR, EA_BUFFER_SIZE - 2);

#define EA_RAMTEST_LOOP(value)						\
do {									\
	/* Set the write start address and write a pattern */		\
	ea_writebuf(sc, NULL, 0x0000, 0);				\
	for (loop = 0; loop < EA_BUFFER_SIZE; loop += 2)		\
		bus_space_write_2(iot, ioh, EA_8005_BUFWIN, (value));	\
									\
	/* Set the read start address and verify the pattern */		\
	ea_readbuf(sc, NULL, 0x0000, 0);				\
	for (loop = 0; loop < EA_BUFFER_SIZE; loop += 2)		\
		if (bus_space_read_2(iot, ioh, EA_8005_BUFWIN) != (value)) \
			++sum;						\
	if (sum != 0)							\
		dprintf(("sum=%d\n", sum));				\
} while (/*CONSTCOND*/0)

	EA_RAMTEST_LOOP(loop);
	EA_RAMTEST_LOOP(loop ^ (EA_BUFFER_SIZE - 1));
	EA_RAMTEST_LOOP(0xaa55);
	EA_RAMTEST_LOOP(0x55aa);

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

	dprintf(("ea_stoptx()\n"));

	status = bus_space_read_2(iot, ioh, EA_8005_STATUS);
	if (!(status & EA_STATUS_TX_ON))
		return 0;

	/* Stop any tx and wait for confirmation */
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_TX_OFF);

	timeout = 20000;
	do {
		status = bus_space_read_2(iot, ioh, EA_8005_STATUS);
	} while ((status & EA_STATUS_TX_ON) && --timeout > 0);
	if (timeout == 0)
		dprintf(("ea_stoptx: timeout waiting for tx termination\n"));

	/* Clear any pending tx interrupt */
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
		   sc->sc_command | EA_CMD_TX_INTACK);
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

	dprintf(("ea_stoprx()\n"));

	status = bus_space_read_2(iot, ioh, EA_8005_STATUS);
	if (!(status & EA_STATUS_RX_ON))
		return 0;

	/* Stop any rx and wait for confirmation */

	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_RX_OFF);

	timeout = 20000;
	do {
		status = bus_space_read_2(iot, ioh, EA_8005_STATUS);
	} while ((status & EA_STATUS_RX_ON) && --timeout > 0);
	if (timeout == 0)
		dprintf(("ea_stoprx: timeout waiting for rx termination\n"));

	/* Clear any pending rx interrupt */

	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
		   sc->sc_command | EA_CMD_RX_INTACK);
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
	
	dprintf(("ea_stop()\n"));

	/* Stop all IO */
	ea_stoptx(sc);
	ea_stoprx(sc);

	/* Disable rx and tx interrupts */
	sc->sc_command &= (EA_CMD_RX_INTEN | EA_CMD_TX_INTEN);

	/* Clear any pending interrupts */
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_RX_INTACK |
			  EA_CMD_TX_INTACK | EA_CMD_DMA_INTACK |
			  EA_CMD_BW_INTACK);
	dprintf(("st=%08x", bus_space_read_2(iot, ioh, EA_8005_STATUS)));

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

	dprintf(("ea_chipreset()\n"));

	/* Reset the controller. Min of 4us delay here */

	bus_space_write_2(iot, ioh, EA_8005_CONFIG2, EA_CFG2_RESET);
	delay(4);

	sc->sc_command = 0;
	sc->sc_config1 = 0;
	sc->sc_config2 = 0;
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
	if ((bus_space_read_2(iot, ioh, EA_8005_STATUS) &
	     EA_STATUS_FIFO_DIR) != 0)
		return; /* FIFO is reading anyway. */
	while ((bus_space_read_2(iot, ioh, EA_8005_STATUS) &
		EA_STATUS_FIFO_EMPTY) == 0 &&
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
	while ((bus_space_read_2(iot, ioh, EA_8005_STATUS) &
		EA_STATUS_FIFO_FULL) == 0 &&
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
ea_writebuf(struct seeq8005_softc *sc, u_char *buf, u_int addr, size_t len)
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	dprintf(("writebuf: st=%04x\n",
		 bus_space_read_2(iot, ioh, EA_8005_STATUS)));

#ifdef DIAGNOSTIC
	if (__predict_false(!ALIGNED_POINTER(buf, u_int16_t)))
		panic("%s: unaligned writebuf", sc->sc_dev.dv_xname);
#endif
	if (__predict_false(addr >= EA_BUFFER_SIZE))
		panic("%s: writebuf out of range", sc->sc_dev.dv_xname);

	/* Assume that copying too much is safe. */
	if (len % 2 != 0)
		len++;

	ea_await_fifo_empty(sc);

	ea_select_buffer(sc, EA_BUFCODE_LOCAL_MEM);
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_FIFO_WRITE);
       	bus_space_write_2(iot, ioh, EA_8005_DMA_ADDR, addr);

	if (len > 0)
		bus_space_write_multi_2(iot, ioh, EA_8005_BUFWIN,
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
ea_readbuf(struct seeq8005_softc *sc, u_char *buf, u_int addr, size_t len)
{

	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	dprintf(("readbuf: st=%04x addr=%04x len=%d\n",
		 bus_space_read_2(iot, ioh, EA_8005_STATUS), addr, len));

#ifdef DIAGNOSTIC
	if (!ALIGNED_POINTER(buf, u_int16_t))
		panic("%s: unaligned readbuf", sc->sc_dev.dv_xname);
#endif
	if (addr >= EA_BUFFER_SIZE)
		panic("%s: writebuf out of range", sc->sc_dev.dv_xname);

	/* Assume that copying too much is safe. */
	if (len % 2 != 0)
		len++;

	ea_await_fifo_empty(sc);

	ea_select_buffer(sc, EA_BUFCODE_LOCAL_MEM);
	bus_space_write_2(iot, ioh, EA_8005_DMA_ADDR, addr);
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_FIFO_READ);

	ea_await_fifo_full(sc);

	if (len > 0)
		bus_space_read_multi_2(iot, ioh, EA_8005_BUFWIN,
				       (u_int16_t *)buf, len / 2);
}

static void
ea_select_buffer(struct seeq8005_softc *sc, int bufcode)
{

	bus_space_write_2(sc->sc_iot, sc->sc_ioh, EA_8005_CONFIG1,
			  sc->sc_config1 | bufcode);
}

/* Must be called at splnet */
static void
ea_set_address(struct seeq8005_softc *sc, int which, u_int8_t const *ea)
{
	int i;

	ea_select_buffer(sc, EA_BUFCODE_STATION_ADDR0 + which);
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		bus_space_write_2(sc->sc_iot, sc->sc_ioh, EA_8005_BUFWIN,
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

	sc->sc_command = 0x00;
	sc->sc_config1 = 0x00; /* XXX DMA settings? */
#if BYTE_ORDER == BIG_ENDIAN
	sc->sc_config2 = EA_CFG2_BYTESWAP
#else
	sc->sc_config2 = 0;
#endif

	bus_space_write_2(iot, ioh, EA_8005_COMMAND, sc->sc_command);
	bus_space_write_2(iot, ioh, EA_8005_CONFIG1, sc->sc_config1);
	bus_space_write_2(iot, ioh, EA_8005_CONFIG2, sc->sc_config2);

	/* Split board memory into Rx and Tx. */
	ea_select_buffer(sc, EA_BUFCODE_TX_EAP);
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN,
			  (EA_TX_BUFFER_SIZE >> 8) - 1);

	/* Write the station address - the receiver must be off */
	ea_set_address(sc, 0, LLADDR(ifp->if_sadl));

	/* Configure rx. */
	dprintf(("Configuring rx...\n"));
	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_config1 = EA_CFG1_PROMISCUOUS;
	else
		sc->sc_config1 = EA_CFG1_BROADCAST;
	sc->sc_config1 |= EA_CFG1_STATION_ADDR0;
	bus_space_write_2(iot, ioh, EA_8005_CONFIG1, sc->sc_config1);

	/* Setup the Rx pointers */
	sc->sc_rx_ptr = EA_TX_BUFFER_SIZE;

	bus_space_write_2(iot, ioh, EA_8005_RX_PTR, sc->sc_rx_ptr);
	bus_space_write_2(iot, ioh, EA_8005_RX_END, sc->sc_rx_ptr >> 8);


	/* Place a NULL header at the beginning of the receive area */
	ea_writebuf(sc, NULL, sc->sc_rx_ptr, 0);
		
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, 0x0000);
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, 0x0000);


	/* Turn on Rx */
	sc->sc_command |= EA_CMD_RX_INTEN;
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_RX_ON);


	/* Configure TX. */
	dprintf(("Configuring tx...\n"));

	bus_space_write_2(iot, ioh, EA_8005_TX_PTR, 0x0000);

	sc->sc_config2 |= EA_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, EA_8005_CONFIG2, sc->sc_config2);


	/* Place a NULL header at the beginning of the transmit area */
	ea_writebuf(sc, NULL, 0x0000, 0);
		
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, 0x0000);
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, 0x0000);

	sc->sc_command |= EA_CMD_TX_INTEN;
	bus_space_write_2(iot, ioh, EA_8005_COMMAND, sc->sc_command);

	/* TX_ON gets set by ea_txpacket when there's something to transmit. */


	/* Set flags appropriately. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	dprintf(("init: st=%04x\n",
		 bus_space_read_2(iot, ioh, EA_8005_STATUS)));


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
#ifdef EA_TX_DEBUG
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
	struct mbuf *m, *m0;
	struct ifnet *ifp;
	int len, nextpacket;
	u_int8_t hdr[4];

	ifp = &sc->sc_ethercom.ec_if;

	/* Dequeue the next packet. */
	IFQ_DEQUEUE(&ifp->if_snd, m0);

	/* If there's nothing to send, return. */
	if (!m0) {
		ifp->if_flags &= ~IFF_OACTIVE;
		sc->sc_config2 |= EA_CFG2_OUTPUT;
		bus_space_write_2(iot, ioh, EA_8005_CONFIG2, sc->sc_config2);
#ifdef EA_TX_DEBUG
		dprintf(("tx finished\n"));
#endif
		return;
	}

#if NBPFILTER > 0
	/* Give the packet to the bpf, if any. */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

#ifdef EA_TX_DEBUG
	dprintf(("Tx new packet\n"));
#endif

	sc->sc_config2 &= ~EA_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, EA_8005_CONFIG2, sc->sc_config2);

	/*
	 * Copy the frame to the start of the transmit area on the card,
	 * leaving four bytes for the transmit header.
	 */
	len = 0;
	for (m = m0; m; m = m->m_next) {
		if (m->m_len == 0)
			continue;
		ea_writebuf(sc, mtod(m, caddr_t), 4 + len, m->m_len);
		len += m->m_len;
	}
	m_freem(m0);


	/* If packet size is odd round up to the next 16 bit boundry */
	if (len % 2)
		++len;

	len = max(len, ETHER_MIN_LEN);
	
	if (len > (ETHER_MAX_LEN - ETHER_CRC_LEN))
		log(LOG_WARNING, "%s: oversize packet = %d bytes\n",
		    sc->sc_dev.dv_xname, len);

#if 0 /*def EA_TX_DEBUG*/
	dprintf(("ea: xfr pkt length=%d...\n", len));

	dprintf(("%s-->", ether_sprintf(sc->sc_pktbuf+6)));
	dprintf(("%s\n", ether_sprintf(sc->sc_pktbuf)));
#endif

/*	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, EA_8005_STATUS)));*/

	/* Follow it with a NULL packet header */
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, 0x0000);
	bus_space_write_2(iot, ioh, EA_8005_BUFWIN, 0x0000);


	/* Write the packet header */

	nextpacket = len + 4;
	hdr[0] = (nextpacket >> 8) & 0xff;
	hdr[1] = nextpacket & 0xff;
	hdr[2] = EA_PKTHDR_TX | EA_PKTHDR_DATA_FOLLOWS |
		EA_TXHDR_XMIT_SUCCESS_INT | EA_TXHDR_COLLISION_INT;
	hdr[3] = 0; /* Status byte -- will be update by hardware. */
	ea_writebuf(sc, hdr, 0x0000, 4);

	bus_space_write_2(iot, ioh, EA_8005_TX_PTR, 0x0000);

/*	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, EA_8005_STATUS)));*/

#ifdef EA_PACKET_DEBUG
	ea_dump_buffer(sc, 0);
#endif


	/* Now transmit the datagram. */
/*	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, EA_8005_STATUS)));*/
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_TX_ON);
#ifdef EA_TX_DEBUG
	dprintf(("st=%04x\n", bus_space_read_2(iot, ioh, EA_8005_STATUS)));
	dprintf(("tx: queued\n"));
#endif
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
	int status, s, handled;
	u_int8_t txhdr[4];
	u_int txstatus;

	handled = 0;
	dprintf(("eaintr: "));


	/* Get the controller status */
	status = bus_space_read_2(iot, ioh, EA_8005_STATUS);
        dprintf(("st=%04x ", status));	


	/* Tx interrupt ? */
	if (status & EA_STATUS_TX_INT) {
		dprintf(("txint "));
		handled = 1;

		/* Acknowledge the interrupt */
		bus_space_write_2(iot, ioh, EA_8005_COMMAND,
				  sc->sc_command | EA_CMD_TX_INTACK);

		ea_readbuf(sc, txhdr, 0x0000, 4);

#ifdef EA_TX_DEBUG		
		dprintf(("txstatus=%02x %02x %02x %02x\n",
			 txhdr[0], txhdr[1], txhdr[2], txhdr[3]));
#endif
		txstatus = txhdr[3];

		/*
		 * Did it succeed ? Did we collide ?
		 *
		 * The exact proceedure here is not clear. We should get
		 * an interrupt on a sucessfull tx or on a collision.
		 * The done flag is set after successfull tx or 16 collisions
		 * We should thus get a interrupt for each of collision
		 * and the done bit should not be set. However it does appear
		 * to be set at the same time as the collision bit ...
		 *
		 * So we will count collisions and output errors and will
		 * assume that if the done bit is set the packet was
		 * transmitted. Stats may be wrong if 16 collisions occur on
		 * a packet as the done flag should be set but the packet
		 * may not have been transmitted. so the output count might
		 * not require incrementing if the 16 collisions flags is
		 * set. I don;t know abou this until it happens.
		 */

		if (txstatus & EA_TXHDR_COLLISION)
			ifp->if_collisions++;
		else if (txstatus & EA_TXHDR_ERROR_MASK)
			ifp->if_oerrors++;

#if 0
		if (txstatus & EA_TXHDR_ERROR_MASK)
			log(LOG_WARNING, "tx packet error =%02x\n", txstatus);
#endif

		if (txstatus & EA_PKTHDR_DONE) {
			ifp->if_opackets++;

			/* Tx next packet */

			s = splnet();
			eatxpacket(sc);
			splx(s);
		}
	}


	/* Rx interrupt ? */
	if (status & EA_STATUS_RX_INT) {
		dprintf(("rxint "));
		handled = 1;

		/* Acknowledge the interrupt */
		bus_space_write_2(iot, ioh, EA_8005_COMMAND,
				  sc->sc_command | EA_CMD_RX_INTACK);

		/* Install a watchdog timer needed atm to fixed rx lockups */
		ifp->if_timer = EA_TIMEOUT;

		/* Processes the received packets */
		eagetpackets(sc);


#if 0
		/* Make sure the receiver is on */
		if ((status & EA_STATUS_RX_ON) == 0) {
			bus_space_write_2(iot, ioh, EA_8005_COMMAND,
					  sc->sc_command | EA_CMD_RX_ON);
			printf("rxintr: rx is off st=%04x\n",status);
		}
#endif
	}

#ifdef EA_DEBUG
	status = bus_space_read_2(iot, ioh, EA_8005_STATUS);
        dprintf(("st=%04x\n", status));
#endif

	return handled;
}


void
eagetpackets(struct seeq8005_softc *sc)
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
	sc->sc_config2 &= ~EA_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, EA_8005_CONFIG2, sc->sc_config2);

	do {
		/* Read rx header */
		ea_readbuf(sc, rxhdr, addr, 4);
		
		/* Split the packet header */
		ptr = (rxhdr[0] << 8) | rxhdr[1];
		ctrl = rxhdr[2];
		status = rxhdr[3];

#ifdef EA_RX_DEBUG
		dprintf(("addr=%04x ptr=%04x ctrl=%02x status=%02x\n",
			 addr, ptr, ctrl, status));
#endif


		/* Zero packet ptr ? then must be null header so exit */
		if (ptr == 0) break;


		/* Get packet length */
       		len = (ptr - addr) - 4;

		if (len < 0)
			len += EA_RX_BUFFER_SIZE;

#ifdef EA_RX_DEBUG
		dprintf(("len=%04x\n", len));
#endif


		/* Has the packet rx completed ? if not then exit */
		if ((status & EA_PKTHDR_DONE) == 0)
			break;

		/*
		 * Did we have any errors? then note error and go to
		 * next packet
		 */
		if (__predict_false(status & 0x0f)) {
			++ifp->if_ierrors;
			log(LOG_WARNING,
			    "%s: rx packet error (%02x) - dropping packet\n",
			    sc->sc_dev.dv_xname, status & 0x0f);
			sc->sc_config2 |= EA_CFG2_OUTPUT;
			bus_space_write_2(iot, ioh, EA_8005_CONFIG2,
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
			sc->sc_config2 |= EA_CFG2_OUTPUT;
			bus_space_write_2(iot, ioh, EA_8005_CONFIG2,
					  sc->sc_config2);
			ea_init(ifp);
			return;
		}

		ifp->if_ipackets++;
		/* Pass data up to upper levels. */
		earead(sc, addr + 4, len);

		addr = ptr;
		++pack;
	} while (len != 0);

	sc->sc_config2 |= EA_CFG2_OUTPUT;
	bus_space_write_2(iot, ioh, EA_8005_CONFIG2, sc->sc_config2);

#ifdef EA_RX_DEBUG
	dprintf(("new rx ptr=%04x\n", addr));
#endif


	/* Store new rx pointer */
	sc->sc_rx_ptr = addr;
	bus_space_write_2(iot, ioh, EA_8005_RX_END, sc->sc_rx_ptr >> 8);

	/* Make sure the receiver is on */
	bus_space_write_2(iot, ioh, EA_8005_COMMAND,
			  sc->sc_command | EA_CMD_RX_ON);

}


/*
 * Pass a packet up to the higher levels.
 */

static void
earead(struct seeq8005_softc *sc, int addr, int len)
{
	struct mbuf *m;
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;

	/* Pull packet off interface. */
	m = eaget(sc, addr, len, ifp);
	if (m == 0)
		return;

#ifdef EA_RX_DEBUG
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
eaget(struct seeq8005_softc *sc, int addr, int totlen, struct ifnet *ifp)
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
			   cp < EA_BUFFER_SIZE ? cp : cp - EA_RX_BUFFER_SIZE,
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
	struct ether_multi *enm;
	struct ether_multistep step;
	int naddr, maxaddrs;

	naddr = 0;
	maxaddrs = (sc->sc_flags & SEEQ8005_80C04) ? 5 : 0;
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm != NULL) {
		/* Have we got space? */
		if (naddr >= maxaddrs ||
		    bcmp(enm->enm_addrlo, enm->enm_addrhi, 6) != 0) {
			sc->sc_ethercom.ec_if.if_flags |= IFF_ALLMULTI;
			ea_ioctl(&sc->sc_ethercom.ec_if, SIOCSIFFLAGS, NULL);
			return;
		}
		ea_set_address(sc, naddr, enm->enm_addrlo);
		sc->sc_config1 |= EA_CFG1_STATION_ADDR0 << naddr;
		naddr++;
		ETHER_NEXT_MULTI(step, enm);
	}
	for (; naddr < maxaddrs; naddr++)
		sc->sc_config1 &= ~(EA_CFG1_STATION_ADDR0 << naddr);
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, EA_8005_CONFIG1,
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
		 bus_space_read_2(sc->sc_iot, sc->sc_ioh, EA_8005_STATUS)));

	/* Kick the interface */

	ea_init(ifp);

/*	ifp->if_timer = EA_TIMEOUT;*/
	ifp->if_timer = 0;
}

/* End of if_ea.c */
