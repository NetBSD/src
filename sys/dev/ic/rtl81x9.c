/*	$NetBSD: rtl81x9.c,v 1.9 2000/05/15 01:55:53 thorpej Exp $	*/

/*
 * Copyright (c) 1997, 1998
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	FreeBSD Id: if_rl.c,v 1.17 1999/06/19 20:17:37 wpaul Exp
 */

/*
 * RealTek 8129/8139 PCI NIC driver
 *
 * Supports several extremely cheap PCI 10/100 adapters based on
 * the RealTek chipset. Datasheets can be obtained from
 * www.realtek.com.tw.
 *
 * Written by Bill Paul <wpaul@ctr.columbia.edu>
 * Electrical Engineering Department
 * Columbia University, New York City
 */

/*
 * The RealTek 8139 PCI NIC redefines the meaning of 'low end.' This is
 * probably the worst PCI ethernet controller ever made, with the possible
 * exception of the FEAST chip made by SMC. The 8139 supports bus-master
 * DMA, but it has a terrible interface that nullifies any performance
 * gains that bus-master DMA usually offers.
 *
 * For transmission, the chip offers a series of four TX descriptor
 * registers. Each transmit frame must be in a contiguous buffer, aligned
 * on a longword (32-bit) boundary. This means we almost always have to
 * do mbuf copies in order to transmit a frame, except in the unlikely
 * case where a) the packet fits into a single mbuf, and b) the packet
 * is 32-bit aligned within the mbuf's data area. The presence of only
 * four descriptor registers means that we can never have more than four
 * packets queued for transmission at any one time.
 *
 * Reception is not much better. The driver has to allocate a single large
 * buffer area (up to 64K in size) into which the chip will DMA received
 * frames. Because we don't know where within this region received packets
 * will begin or end, we have no choice but to copy data from the buffer
 * area into mbufs in order to pass the packets up to the higher protocol
 * levels.
 *
 * It's impossible given this rotten design to really achieve decent
 * performance at 100Mbps, unless you happen to have a 400Mhz PII or
 * some equally overmuscled CPU to drive it.
 *
 * On the bright side, the 8139 does have a built-in PHY, although
 * rather than using an MDIO serial interface like most other NICs, the
 * PHY registers are directly accessible through the 8139's register
 * space. The 8139 supports autonegotiation, as well as a 64-bit multicast
 * filter.
 *
 * The 8129 chip is an older version of the 8139 that uses an external PHY
 * chip. The 8129 has a serial MDIO interface for accessing the MII where
 * the 8139 lets you directly access the on-board PHY registers. We need
 * to select which interface to use depending on the chip type.
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_arp.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif
#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>
#include <machine/endian.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/ic/rtl81x9reg.h>
#include <dev/ic/rtl81x9var.h>

#if defined DEBUG
#define STATIC
#else
#define STATIC static
#endif

STATIC void rtk_reset		__P((struct rtk_softc *));
STATIC void rtk_rxeof		__P((struct rtk_softc *));
STATIC void rtk_txeof		__P((struct rtk_softc *));
STATIC void rtk_start		__P((struct ifnet *));
STATIC int rtk_ioctl		__P((struct ifnet *, u_long, caddr_t));
STATIC void rtk_init		__P((void *));
STATIC void rtk_stop		__P((struct rtk_softc *));
STATIC void rtk_watchdog		__P((struct ifnet *));
STATIC void rtk_shutdown		__P((void *));
STATIC int rtk_ifmedia_upd	__P((struct ifnet *));
STATIC void rtk_ifmedia_sts	__P((struct ifnet *, struct ifmediareq *));

STATIC u_int16_t rtk_read_eeprom __P((struct rtk_softc *, int, int));
STATIC void rtk_eeprom_putbyte	__P((struct rtk_softc *, int, int));
STATIC void rtk_mii_sync		__P((struct rtk_softc *));
STATIC void rtk_mii_send		__P((struct rtk_softc *, u_int32_t, int));
STATIC int rtk_mii_readreg	__P((struct rtk_softc *, struct rtk_mii_frame *));
STATIC int rtk_mii_writereg	__P((struct rtk_softc *, struct rtk_mii_frame *));

STATIC int rtk_phy_readreg	__P((struct device *, int, int));
STATIC void rtk_phy_writereg	__P((struct device *, int, int, int));
STATIC void rtk_phy_statchg	__P((struct device *));
STATIC void rtk_tick __P((void *));

STATIC void rtk_setmulti		__P((struct rtk_softc *));
STATIC int rtk_list_tx_init	__P((struct rtk_softc *));

STATIC int rtk_ether_ioctl __P((struct ifnet *, u_long, caddr_t));


#define EE_SET(x)					\
	CSR_WRITE_1(sc, RL_EECMD,			\
		CSR_READ_1(sc, RL_EECMD) | (x))

#define EE_CLR(x)					\
	CSR_WRITE_1(sc, RL_EECMD,			\
		CSR_READ_1(sc, RL_EECMD) & ~(x))

/*
 * Send a read command and address to the EEPROM, check for ACK.
 */
STATIC void rtk_eeprom_putbyte(sc, addr, addr_len)
	struct rtk_softc		*sc;
	int			addr, addr_len;
{
	int			d, i;

	d = (RL_EECMD_READ << addr_len) | addr;

	/*
	 * Feed in each bit and stobe the clock.
	 */
	for (i = RL_EECMD_LEN + addr_len - 1; i >= 0; i--) {
		if (d & (1 << i)) {
			EE_SET(RL_EE_DATAIN);
		} else {
			EE_CLR(RL_EE_DATAIN);
		}
		DELAY(100);
		EE_SET(RL_EE_CLK);
		DELAY(150);
		EE_CLR(RL_EE_CLK);
		DELAY(100);
	}
}

/*
 * Read a word of data stored in the EEPROM at address 'addr.'
 */
u_int16_t rtk_read_eeprom(sc, addr, addr_len)
	struct rtk_softc		*sc;
	int			addr, addr_len;
{
	u_int16_t		word = 0;
	int			i;

	/* Enter EEPROM access mode. */
	CSR_WRITE_1(sc, RL_EECMD, RL_EEMODE_PROGRAM|RL_EE_SEL);

	/*
	 * Send address of word we want to read.
	 */
	rtk_eeprom_putbyte(sc, addr, addr_len);

	CSR_WRITE_1(sc, RL_EECMD, RL_EEMODE_PROGRAM|RL_EE_SEL);

	/*
	 * Start reading bits from EEPROM.
	 */
	for (i = 15; i >= 0; i--) {
		EE_SET(RL_EE_CLK);
		DELAY(100);
		if (CSR_READ_1(sc, RL_EECMD) & RL_EE_DATAOUT)
			word |= (1 << i);
		EE_CLR(RL_EE_CLK);
		DELAY(100);
	}

	/* Turn off EEPROM access mode. */
	CSR_WRITE_1(sc, RL_EECMD, RL_EEMODE_OFF);

	return (word);
}

/*
 * MII access routines are provided for the 8129, which
 * doesn't have a built-in PHY. For the 8139, we fake things
 * up by diverting rtk_phy_readreg()/rtk_phy_writereg() to the
 * direct access PHY registers.
 */
#define MII_SET(x)					\
	CSR_WRITE_1(sc, RL_MII,				\
		CSR_READ_1(sc, RL_MII) | (x))

#define MII_CLR(x)					\
	CSR_WRITE_1(sc, RL_MII,				\
		CSR_READ_1(sc, RL_MII) & ~(x))

/*
 * Sync the PHYs by setting data bit and strobing the clock 32 times.
 */
STATIC void rtk_mii_sync(sc)
	struct rtk_softc		*sc;
{
	int			i;

	MII_SET(RL_MII_DIR|RL_MII_DATAOUT);

	for (i = 0; i < 32; i++) {
		MII_SET(RL_MII_CLK);
		DELAY(1);
		MII_CLR(RL_MII_CLK);
		DELAY(1);
	}

	return;
}

/*
 * Clock a series of bits through the MII.
 */
STATIC void rtk_mii_send(sc, bits, cnt)
	struct rtk_softc		*sc;
	u_int32_t		bits;
	int			cnt;
{
	int			i;

	MII_CLR(RL_MII_CLK);

	for (i = (0x1 << (cnt - 1)); i; i >>= 1) {
                if (bits & i) {
			MII_SET(RL_MII_DATAOUT);
                } else {
			MII_CLR(RL_MII_DATAOUT);
                }
		DELAY(1);
		MII_CLR(RL_MII_CLK);
		DELAY(1);
		MII_SET(RL_MII_CLK);
	}
}

/*
 * Read an PHY register through the MII.
 */
STATIC int rtk_mii_readreg(sc, frame)
	struct rtk_softc		*sc;
	struct rtk_mii_frame	*frame;
	
{
	int			i, ack, s;

	s = splnet();

	/*
	 * Set up frame for RX.
	 */
	frame->mii_stdelim = RL_MII_STARTDELIM;
	frame->mii_opcode = RL_MII_READOP;
	frame->mii_turnaround = 0;
	frame->mii_data = 0;
	
	CSR_WRITE_2(sc, RL_MII, 0);

	/*
 	 * Turn on data xmit.
	 */
	MII_SET(RL_MII_DIR);

	rtk_mii_sync(sc);

	/*
	 * Send command/address info.
	 */
	rtk_mii_send(sc, frame->mii_stdelim, 2);
	rtk_mii_send(sc, frame->mii_opcode, 2);
	rtk_mii_send(sc, frame->mii_phyaddr, 5);
	rtk_mii_send(sc, frame->mii_regaddr, 5);

	/* Idle bit */
	MII_CLR((RL_MII_CLK|RL_MII_DATAOUT));
	DELAY(1);
	MII_SET(RL_MII_CLK);
	DELAY(1);

	/* Turn off xmit. */
	MII_CLR(RL_MII_DIR);

	/* Check for ack */
	MII_CLR(RL_MII_CLK);
	DELAY(1);
	MII_SET(RL_MII_CLK);
	DELAY(1);
	ack = CSR_READ_2(sc, RL_MII) & RL_MII_DATAIN;

	/*
	 * Now try reading data bits. If the ack failed, we still
	 * need to clock through 16 cycles to keep the PHY(s) in sync.
	 */
	if (ack) {
		for(i = 0; i < 16; i++) {
			MII_CLR(RL_MII_CLK);
			DELAY(1);
			MII_SET(RL_MII_CLK);
			DELAY(1);
		}
		goto fail;
	}

	for (i = 0x8000; i; i >>= 1) {
		MII_CLR(RL_MII_CLK);
		DELAY(1);
		if (!ack) {
			if (CSR_READ_2(sc, RL_MII) & RL_MII_DATAIN)
				frame->mii_data |= i;
			DELAY(1);
		}
		MII_SET(RL_MII_CLK);
		DELAY(1);
	}

fail:

	MII_CLR(RL_MII_CLK);
	DELAY(1);
	MII_SET(RL_MII_CLK);
	DELAY(1);

	splx(s);

	if (ack)
		return(1);
	return(0);
}

/*
 * Write to a PHY register through the MII.
 */
STATIC int rtk_mii_writereg(sc, frame)
	struct rtk_softc		*sc;
	struct rtk_mii_frame	*frame;
	
{
	int			s;

	s = splnet();
	/*
	 * Set up frame for TX.
	 */

	frame->mii_stdelim = RL_MII_STARTDELIM;
	frame->mii_opcode = RL_MII_WRITEOP;
	frame->mii_turnaround = RL_MII_TURNAROUND;
	
	/*
 	 * Turn on data output.
	 */
	MII_SET(RL_MII_DIR);

	rtk_mii_sync(sc);

	rtk_mii_send(sc, frame->mii_stdelim, 2);
	rtk_mii_send(sc, frame->mii_opcode, 2);
	rtk_mii_send(sc, frame->mii_phyaddr, 5);
	rtk_mii_send(sc, frame->mii_regaddr, 5);
	rtk_mii_send(sc, frame->mii_turnaround, 2);
	rtk_mii_send(sc, frame->mii_data, 16);

	/* Idle bit. */
	MII_SET(RL_MII_CLK);
	DELAY(1);
	MII_CLR(RL_MII_CLK);
	DELAY(1);

	/*
	 * Turn off xmit.
	 */
	MII_CLR(RL_MII_DIR);

	splx(s);

	return(0);
}

STATIC int rtk_phy_readreg(self, phy, reg)
	struct device		*self;
	int			phy, reg;
{
	struct rtk_softc		*sc = (void *)self;
	struct rtk_mii_frame	frame;
	u_int16_t		rval = 0;
	u_int16_t		rl8139_reg = 0;

	if (sc->rtk_type == RL_8139) {
		if (phy != 7)
			return (0);

		switch(reg) {
		case MII_BMCR:
			rl8139_reg = RL_BMCR;
			break;
		case MII_BMSR:
			rl8139_reg = RL_BMSR;
			break;
		case MII_ANAR:
			rl8139_reg = RL_ANAR;
			break;
		case MII_ANLPAR:
			rl8139_reg = RL_LPAR;
			break;
		default:
#if 0
			printf("%s: bad phy register\n", sc->sc_dev.dv_xname);
#endif
			return(0);
		}
		rval = CSR_READ_2(sc, rl8139_reg);
		return(rval);
	}

	bzero((char *)&frame, sizeof(frame));

	frame.mii_phyaddr = phy;
	frame.mii_regaddr = reg;
	rtk_mii_readreg(sc, &frame);

	return(frame.mii_data);
}

STATIC void rtk_phy_writereg(self, phy, reg, data)
	struct device		*self;
	int			phy, reg;
	int			data;
{
	struct rtk_softc		*sc = (void *)self;
	struct rtk_mii_frame	frame;
	u_int16_t		rl8139_reg = 0;

	if (sc->rtk_type == RL_8139) {
		if (phy != 7)
			return;

		switch(reg) {
		case MII_BMCR:
			rl8139_reg = RL_BMCR;
			break;
		case MII_BMSR:
			rl8139_reg = RL_BMSR;
			break;
		case MII_ANAR:
			rl8139_reg = RL_ANAR;
			break;
		case MII_ANLPAR:
			rl8139_reg = RL_LPAR;
			break;
		default:
#if 0
			printf("%s: bad phy register\n", sc->sc_dev.dv_xname);
#endif
			return;
		}
		CSR_WRITE_2(sc, rl8139_reg, data);
		return;
	}

	bzero((char *)&frame, sizeof(frame));

	frame.mii_phyaddr = phy;
	frame.mii_regaddr = reg;
	frame.mii_data = data;

	rtk_mii_writereg(sc, &frame);

	return;
}

STATIC void
rtk_phy_statchg(v)
	struct device *v;
{

	/* Nothing to do. */
}

#define	rtk_calchash(addr) \
	(ether_crc32_be((addr), ETHER_ADDR_LEN) >> 26)

/*
 * Program the 64-bit multicast hash filter.
 */
STATIC void rtk_setmulti(sc)
	struct rtk_softc		*sc;
{
	struct ifnet		*ifp;
	int			h = 0;
	u_int32_t		hashes[2] = { 0, 0 };
	u_int32_t		rxfilt;
	int			mcnt = 0;
	struct ether_multi *enm;
	struct ether_multistep step;

	ifp = &sc->ethercom.ec_if;

	rxfilt = CSR_READ_4(sc, RL_RXCFG);

	if (ifp->if_flags & IFF_ALLMULTI || ifp->if_flags & IFF_PROMISC) {
		rxfilt |= RL_RXCFG_RX_MULTI;
		CSR_WRITE_4(sc, RL_RXCFG, rxfilt);
		CSR_WRITE_4(sc, RL_MAR0, 0xFFFFFFFF);
		CSR_WRITE_4(sc, RL_MAR4, 0xFFFFFFFF);
		return;
	}

	/* first, zot all the existing hash bits */
	CSR_WRITE_4(sc, RL_MAR0, 0);
	CSR_WRITE_4(sc, RL_MAR4, 0);

	/* now program new ones */
	ETHER_FIRST_MULTI(step, &sc->ethercom, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
		    ETHER_ADDR_LEN) != 0)
			continue;

		h = rtk_calchash(enm->enm_addrlo);
		if (h < 32)
			hashes[0] |= (1 << h);
		else
			hashes[1] |= (1 << (h - 32));
		mcnt++;
		ETHER_NEXT_MULTI(step, enm);
	}

	if (mcnt)
		rxfilt |= RL_RXCFG_RX_MULTI;
	else
		rxfilt &= ~RL_RXCFG_RX_MULTI;

	CSR_WRITE_4(sc, RL_RXCFG, rxfilt);
	CSR_WRITE_4(sc, RL_MAR0, hashes[0]);
	CSR_WRITE_4(sc, RL_MAR4, hashes[1]);

	return;
}

void rtk_reset(sc)
	struct rtk_softc		*sc;
{
	int			i;

	CSR_WRITE_1(sc, RL_COMMAND, RL_CMD_RESET);

	for (i = 0; i < RL_TIMEOUT; i++) {
		DELAY(10);
		if (!(CSR_READ_1(sc, RL_COMMAND) & RL_CMD_RESET))
			break;
	}
	if (i == RL_TIMEOUT)
		printf("%s: reset never completed!\n", sc->sc_dev.dv_xname);

        return;
}

/*
 * Attach the interface. Allocate softc structures, do ifmedia
 * setup and ethernet/BPF attach.
 */
void
rtk_attach(sc)
	struct rtk_softc *sc;
{

	struct ifnet *ifp;
	u_int16_t val;
	u_int8_t eaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t dmaseg;
	int error,dmanseg;
	int i,addr_len;

	callout_init(&sc->rtk_tick_ch);

	/*
	 * Check EEPROM type 9346 or 9356.
	 */
	if (rtk_read_eeprom(sc, RL_EE_ID, RL_EEADDR_LEN1) == 0x8129)
		addr_len = RL_EEADDR_LEN1;
	else
		addr_len = RL_EEADDR_LEN0;

	/*
	 * Get station address.
	 */
	val = rtk_read_eeprom(sc, RL_EE_EADDR0, addr_len);
	eaddr[0] = val & 0xff;
	eaddr[1] = val >> 8;
	val = rtk_read_eeprom(sc, RL_EE_EADDR1, addr_len);
	eaddr[2] = val & 0xff;
	eaddr[3] = val >> 8;
	val = rtk_read_eeprom(sc, RL_EE_EADDR2, addr_len);
	eaddr[4] = val & 0xff;
	eaddr[5] = val >> 8;

	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    RL_RXBUFLEN + 32, NBPG, 0, &dmaseg, 1, &dmanseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't allocate recv buffer, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &dmaseg, dmanseg,
	    RL_RXBUFLEN + 32, (caddr_t *)&sc->rtk_cdata.rtk_rx_buf,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: can't map recv buffer, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail;
	}

	/* Leave a few bytes before the start of the RX ring buffer. */
	sc->rtk_cdata.rtk_rx_buf_ptr = sc->rtk_cdata.rtk_rx_buf;
	sc->rtk_cdata.rtk_rx_buf += sizeof(u_int64_t);

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    RL_RXBUFLEN + 32 - sizeof(u_int64_t), 1,
	    RL_RXBUFLEN + 32 - sizeof(u_int64_t), 0, BUS_DMA_NOWAIT,
	    &sc->recv_dmamap)) != 0) {
		printf("%s: can't create recv buffer DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->recv_dmamap,
	    sc->rtk_cdata.rtk_rx_buf, RL_RXBUFLEN + 32 - sizeof(u_int64_t), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't load recv buffer DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail;
	}

	for (i = 0; i < RL_TX_LIST_CNT; i++)
		if ((error = bus_dmamap_create(sc->sc_dmat,
		    MCLBYTES, 1, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->snd_dmamap[i])) != 0) {
			printf("%s: can't create snd buffer DMA map,"
			    " error = %d\n", sc->sc_dev.dv_xname, error);
		    goto fail;
		}

	/* Reset the adapter. */
	rtk_reset(sc);

	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	       ether_sprintf(eaddr));

	ifp = &sc->ethercom.ec_if;
	ifp->if_softc = sc;
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = rtk_ioctl;
#if 0
	ifp->if_output = ether_output;
#endif
	ifp->if_start = rtk_start;
	ifp->if_watchdog = rtk_watchdog;
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;

	/*
	 * Do ifmedia setup.
	 */
	sc->mii.mii_ifp = ifp;
	sc->mii.mii_readreg = rtk_phy_readreg;
	sc->mii.mii_writereg = rtk_phy_writereg;
	sc->mii.mii_statchg = rtk_phy_statchg;
	ifmedia_init(&sc->mii.mii_media, 0, rtk_ifmedia_upd, rtk_ifmedia_sts);
	mii_attach(&sc->sc_dev, &sc->mii, 0xffffffff, 
				MII_PHY_ANY, MII_OFFSET_ANY, 0);

	/* Choose a default media. */
	if (LIST_FIRST(&sc->mii.mii_phys) == NULL) {
		ifmedia_add(&sc->mii.mii_media, IFM_ETHER|IFM_NONE,
			    0, NULL);
		ifmedia_set(&sc->mii.mii_media, IFM_ETHER|IFM_NONE);
	} else {
		ifmedia_set(&sc->mii.mii_media, IFM_ETHER|IFM_AUTO);
	}

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, eaddr);

#if NBPFILTER > 0
	bpfattach(&sc->ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
		  sizeof(struct ether_header));
#endif
	shutdownhook_establish(rtk_shutdown, sc);

fail:
	return;
}

/*
 * Initialize the transmit descriptors.
 */
STATIC int rtk_list_tx_init(sc)
	struct rtk_softc		*sc;
{
	struct rtk_chain_data	*cd;
	int			i;

	cd = &sc->rtk_cdata;
	for (i = 0; i < RL_TX_LIST_CNT; i++) {
		cd->rtk_tx_chain[i] = NULL;
		CSR_WRITE_4(sc,
		    RL_TXADDR0 + (i * sizeof(u_int32_t)), 0x0000000);
	}

	sc->rtk_cdata.cur_tx = 0;
	sc->rtk_cdata.last_tx = 0;

	return(0);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 *
 * You know there's something wrong with a PCI bus-master chip design
 * when you have to use m_devget().
 *
 * The receive operation is badly documented in the datasheet, so I'll
 * attempt to document it here. The driver provides a buffer area and
 * places its base address in the RX buffer start address register.
 * The chip then begins copying frames into the RX buffer. Each frame
 * is preceeded by a 32-bit RX status word which specifies the length
 * of the frame and certain other status bits. Each frame (starting with
 * the status word) is also 32-bit aligned. The frame length is in the
 * first 16 bits of the status word; the lower 15 bits correspond with
 * the 'rx status register' mentioned in the datasheet.
 *
 * Note: to make the Alpha happy, the frame payload needs to be aligned
 * on a 32-bit boundary. To achieve this, we cheat a bit by copying from
 * the ring buffer starting at an address two bytes before the actual
 * data location. We can then shave off the first two bytes using m_adj().
 * The reason we do this is because m_devget() doesn't let us specify an
 * offset into the mbuf storage space, so we have to artificially create
 * one. The ring is allocated in such a way that there are a few unused
 * bytes of space preceecing it so that it will be safe for us to do the
 * 2-byte backstep even if reading from the ring at offset 0.
 */
STATIC void rtk_rxeof(sc)
	struct rtk_softc		*sc;
{
        struct ether_header	*eh;
        struct mbuf		*m;
        struct ifnet		*ifp;
	int			total_len = 0;
	u_int32_t		rxstat;
	caddr_t			rxbufpos;
	int			wrap = 0;
	u_int16_t		cur_rx;
	u_int16_t		limit;
	u_int16_t		rx_bytes = 0, max_bytes;

	ifp = &sc->ethercom.ec_if;

	cur_rx = (CSR_READ_2(sc, RL_CURRXADDR) + 16) % RL_RXBUFLEN;

	/* Do not try to read past this point. */
	limit = CSR_READ_2(sc, RL_CURRXBUF) % RL_RXBUFLEN;

	if (limit < cur_rx)
		max_bytes = (RL_RXBUFLEN - cur_rx) + limit;
	else
		max_bytes = limit - cur_rx;

	while((CSR_READ_1(sc, RL_COMMAND) & RL_CMD_EMPTY_RXBUF) == 0) {
		rxbufpos = sc->rtk_cdata.rtk_rx_buf + cur_rx;
		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap, cur_rx,
		    sizeof(u_int32_t *), BUS_DMASYNC_POSTREAD);
		rxstat = le32toh(*(u_int32_t *)rxbufpos);
		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap, cur_rx,
		    sizeof(u_int32_t *), BUS_DMASYNC_PREREAD);

		/*
		 * Here's a totally undocumented fact for you. When the
		 * RealTek chip is in the process of copying a packet into
		 * RAM for you, the length will be 0xfff0. If you spot a
		 * packet header with this value, you need to stop. The
		 * datasheet makes absolutely no mention of this and
		 * RealTek should be shot for this.
		 */
		if ((u_int16_t)(rxstat >> 16) == RL_RXSTAT_UNFINISHED)
			break;
	
		if (!(rxstat & RL_RXSTAT_RXOK)) {
			ifp->if_ierrors++;

			/*
			 * submitted by:[netbsd-pcmcia:00484] 
			 *	Takahiro Kambe <taca@sky.yamashina.kyoto.jp>
			 * obtain from:
			 *     FreeBSD if_rl.c rev 1.24->1.25
			 *
			 */
#if 0
			if (rxstat & (RL_RXSTAT_BADSYM|RL_RXSTAT_RUNT|
					RL_RXSTAT_GIANT|RL_RXSTAT_CRCERR|
					RL_RXSTAT_ALIGNERR)) {
			if (rxstat & (RL_RXSTAT_BADSYM|RL_RXSTAT_RUNT|
					RL_RXSTAT_GIANT|RL_RXSTAT_CRCERR|
					RL_RXSTAT_ALIGNERR)) {
				CSR_WRITE_2(sc, RL_COMMAND, RL_CMD_TX_ENB);
				CSR_WRITE_2(sc, RL_COMMAND, RL_CMD_TX_ENB|
							RL_CMD_RX_ENB);
				CSR_WRITE_4(sc, RL_RXCFG, RL_RXCFG_CONFIG);
				CSR_WRITE_4(sc, RL_RXADDR,
					    sc->recv_dmamap->dm_segs[0].ds_addr);
				CSR_WRITE_2(sc, RL_CURRXADDR, cur_rx - 16);
				cur_rx = 0;
			}
			break;
#else
			rtk_init(sc);
			return;
#endif
		}

		/* No errors; receive the packet. */	
		total_len = rxstat >> 16;
		rx_bytes += total_len + 4;

		/*
		 * XXX The RealTek chip includes the CRC with every
		 * received frame, and there's no way to turn this
		 * behavior off (at least, I can't find anything in
	 	 * the manual that explains how to do it) so we have
		 * to trim off the CRC manually.
		 */
		total_len -= ETHER_CRC_LEN;

		/*
		 * Avoid trying to read more bytes than we know
		 * the chip has prepared for us.
		 */
		if (rx_bytes > max_bytes)
			break;

		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap,
		    cur_rx + sizeof(u_int32_t), total_len,
		    BUS_DMASYNC_POSTREAD);

		rxbufpos = sc->rtk_cdata.rtk_rx_buf +
			((cur_rx + sizeof(u_int32_t)) % RL_RXBUFLEN);

		if (rxbufpos == (sc->rtk_cdata.rtk_rx_buf + RL_RXBUFLEN))
			rxbufpos = sc->rtk_cdata.rtk_rx_buf;

		wrap = (sc->rtk_cdata.rtk_rx_buf + RL_RXBUFLEN) - rxbufpos;

		if (total_len > wrap) {
			m = m_devget(rxbufpos - RL_ETHER_ALIGN,
			   wrap + RL_ETHER_ALIGN, 0, ifp, NULL);
			if (m == NULL) {
				ifp->if_ierrors++;
				printf("%s: out of mbufs, tried to "
				    "copy %d bytes\n", sc->sc_dev.dv_xname,
				    wrap);
			}
			else {
				m_adj(m, RL_ETHER_ALIGN);
				m_copyback(m, wrap, total_len - wrap,
					sc->rtk_cdata.rtk_rx_buf);
			}
			cur_rx = (total_len - wrap + ETHER_CRC_LEN);
		} else {
			m = m_devget(rxbufpos - RL_ETHER_ALIGN,
			    total_len + RL_ETHER_ALIGN, 0, ifp, NULL);
			if (m == NULL) {
				ifp->if_ierrors++;
				printf("%s: out of mbufs, tried to "
				    "copy %d bytes\n", sc->sc_dev.dv_xname,
				    total_len);
			} else
				m_adj(m, RL_ETHER_ALIGN);
			cur_rx += total_len + 4 + ETHER_CRC_LEN;
		}

		/*
		 * Round up to 32-bit boundary.
		 */
		cur_rx = (cur_rx + 3) & ~3;
		CSR_WRITE_2(sc, RL_CURRXADDR, cur_rx - 16);

		if (m == NULL)
			continue;

		eh = mtod(m, struct ether_header *);
		ifp->if_ipackets++;

#if NBPFILTER > 0
		/*
		 * Handle BPF listeners. Let the BPF user see the packet, but
		 * don't pass it up to the ether_input() layer unless it's
		 * a broadcast packet, multicast packet, matches our ethernet
		 * address or the interface is in promiscuous mode.
		 */
		if (ifp->if_bpf) {
			bpf_mtap(ifp->if_bpf, m);
			if ((ifp->if_flags & IFF_PROMISC) != 0 &&
				ETHER_IS_MULTICAST(eh->ether_dhost) == 0 &&
				memcmp(eh->ether_dhost, LLADDR(ifp->if_sadl),
						ETHER_ADDR_LEN) != 0) {
				m_freem(m);
				continue;
			}
		}
#endif
		/* pass it on. */
		(*ifp->if_input)(ifp, m);

		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap,
		    cur_rx + sizeof(u_int32_t),
		    total_len, BUS_DMASYNC_PREREAD);
	}

	return;
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */
STATIC void rtk_txeof(sc)
	struct rtk_softc		*sc;
{
	struct ifnet		*ifp;
	u_int32_t		txstat;

	ifp = &sc->ethercom.ec_if;

	/* Clear the timeout timer. */
	ifp->if_timer = 0;

	/*
	 * Go through our tx list and free mbufs for those
	 * frames that have been uploaded.
	 */
	do {
		txstat = CSR_READ_4(sc, RL_LAST_TXSTAT(sc));
		if (!(txstat & (RL_TXSTAT_TX_OK|
		    RL_TXSTAT_TX_UNDERRUN|RL_TXSTAT_TXABRT)))
			break;

		bus_dmamap_sync(sc->sc_dmat,
		    sc->snd_dmamap[sc->rtk_cdata.last_tx], 0,
		    sc->snd_dmamap[sc->rtk_cdata.last_tx]->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat,
		    sc->snd_dmamap[sc->rtk_cdata.last_tx]);
		m_freem(RL_LAST_TXMBUF(sc));
		RL_LAST_TXMBUF(sc) = NULL;

		ifp->if_collisions += (txstat & RL_TXSTAT_COLLCNT) >> 24;

		if (txstat & RL_TXSTAT_TX_OK)
			ifp->if_opackets++;
		else {
			ifp->if_oerrors++;
			if ((txstat & RL_TXSTAT_TXABRT) ||
			    (txstat & RL_TXSTAT_OUTOFWIN))
				CSR_WRITE_4(sc, RL_TXCFG, RL_TXCFG_CONFIG);
		}
		RL_INC(sc->rtk_cdata.last_tx);
		ifp->if_flags &= ~IFF_OACTIVE;
	} while (sc->rtk_cdata.last_tx != sc->rtk_cdata.cur_tx);

	return;
}

int rtk_intr(arg)
	void			*arg;
{
	struct rtk_softc		*sc;
	struct ifnet		*ifp;
	u_int16_t		status;
	int handled = 0;

	sc = arg;
	ifp = &sc->ethercom.ec_if;

	/* Disable interrupts. */
	CSR_WRITE_2(sc, RL_IMR, 0x0000);

	for (;;) {

		status = CSR_READ_2(sc, RL_ISR);
		if (status)
			CSR_WRITE_2(sc, RL_ISR, status);

		handled = 1;

		if ((status & RL_INTRS) == 0)
			break;

		if (status & RL_ISR_RX_OK)
			rtk_rxeof(sc);

		if (status & RL_ISR_RX_ERR)
			rtk_rxeof(sc);

		if ((status & RL_ISR_TX_OK) || (status & RL_ISR_TX_ERR))
			rtk_txeof(sc);

		if (status & RL_ISR_SYSTEM_ERR) {
			rtk_reset(sc);
			rtk_init(sc);
		}

	}

	/* Re-enable interrupts. */
	CSR_WRITE_2(sc, RL_IMR, RL_INTRS);

	if (ifp->if_snd.ifq_head != NULL) {
		rtk_start(ifp);
	}

	return (handled);
}

/*
 * Main transmit routine.
 */

STATIC void rtk_start(ifp)
	struct ifnet		*ifp;
{
	struct rtk_softc		*sc;
	struct mbuf		*m_head = NULL, *m_new;
	int			error, idx, len;

	sc = ifp->if_softc;

	while(RL_CUR_TXMBUF(sc) == NULL) {
		IF_DEQUEUE(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;

		idx = sc->rtk_cdata.cur_tx;

		/*
		 * Load the DMA map.  If this fails, the packet didn't
		 * fit in one DMA segment, and we need to copy.  Note,
		 * the packet must also be aligned.
		 */
		if ((mtod(m_head, bus_addr_t) & 3) != 0 ||
		    bus_dmamap_load_mbuf(sc->sc_dmat, sc->snd_dmamap[idx],
			m_head, BUS_DMA_NOWAIT) != 0) {
			MGETHDR(m_new, M_DONTWAIT, MT_DATA);
			if (m_new == NULL) {
				printf("%s: unable to allocate Tx mbuf\n",
				    sc->sc_dev.dv_xname);
				IF_PREPEND(&ifp->if_snd, m_new);
				break;
			}
			if (m_head->m_pkthdr.len > MHLEN) {
				MCLGET(m_new, M_DONTWAIT);
				if ((m_new->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					    "cluster\n", sc->sc_dev.dv_xname);
					m_freem(m_new);
					IF_PREPEND(&ifp->if_snd, m_head);
					break;
				}
			}
			m_copydata(m_head, 0, m_head->m_pkthdr.len,
			    mtod(m_new, caddr_t));
			m_new->m_pkthdr.len = m_new->m_len =
			    m_head->m_pkthdr.len;
			m_freem(m_head);
			m_head = m_new;
			error = bus_dmamap_load_mbuf(sc->sc_dmat,
			    sc->snd_dmamap[idx], m_head, BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
				IF_PREPEND(&ifp->if_snd, m_head);
				break;
			}
		}

		RL_CUR_TXMBUF(sc) = m_head;

#if NBPFILTER > 0
		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, RL_CUR_TXMBUF(sc));
#endif
		/*
		 * Transmit the frame.
	 	 */
		bus_dmamap_sync(sc->sc_dmat,
		    sc->snd_dmamap[idx], 0, sc->snd_dmamap[idx]->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		len = sc->snd_dmamap[idx]->dm_segs[0].ds_len;
		if (len < (ETHER_MIN_LEN - ETHER_CRC_LEN))
			len = (ETHER_MIN_LEN - ETHER_CRC_LEN);

		CSR_WRITE_4(sc, RL_CUR_TXADDR(sc),
			    sc->snd_dmamap[idx]->dm_segs[0].ds_addr);
		CSR_WRITE_4(sc, RL_CUR_TXSTAT(sc), RL_TX_EARLYTHRESH | len);

		RL_INC(sc->rtk_cdata.cur_tx);
	}

	/*
	 * We broke out of the loop because all our TX slots are
	 * full. Mark the NIC as busy until it drains some of the
	 * packets from the queue.
	 */
	if (RL_CUR_TXMBUF(sc) != NULL)
		ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;

	return;
}

STATIC void rtk_init(xsc)
	void			*xsc;
{
	struct rtk_softc		*sc = xsc;
	struct ifnet		*ifp = &sc->ethercom.ec_if;
	int			s, i;
	u_int32_t		rxcfg;
	u_int16_t		phy_bmcr = 0;

	s = splnet();

	/*
	 * XXX Hack for the 8139: the built-in autoneg logic's state
	 * gets reset by rtk_init() when we don't want it to. Try
	 * to preserve it.
	 */
	if (sc->rtk_type == RL_8139)
		phy_bmcr = rtk_phy_readreg((struct device *)sc, 7, MII_BMCR);

	/*
	 * Cancel pending I/O and free all RX/TX buffers.
	 */
	rtk_stop(sc);

	/* Init our MAC address */
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		CSR_WRITE_1(sc, RL_IDR0 + i, LLADDR(ifp->if_sadl)[i]);
	}

	/* Init the RX buffer pointer register. */
	bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap, 0,
	    sc->recv_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
	CSR_WRITE_4(sc, RL_RXADDR, sc->recv_dmamap->dm_segs[0].ds_addr);

	/* Init TX descriptors. */
	rtk_list_tx_init(sc);

	/*
	 * Enable transmit and receive.
	 */
	CSR_WRITE_1(sc, RL_COMMAND, RL_CMD_TX_ENB|RL_CMD_RX_ENB);

	/*
	 * Set the initial TX and RX configuration.
	 */
	CSR_WRITE_4(sc, RL_TXCFG, RL_TXCFG_CONFIG);
	CSR_WRITE_4(sc, RL_RXCFG, RL_RXCFG_CONFIG);

	/* Set the individual bit to receive frames for this host only. */
	rxcfg = CSR_READ_4(sc, RL_RXCFG);
	rxcfg |= RL_RXCFG_RX_INDIV;

	/* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC) {
		rxcfg |= RL_RXCFG_RX_ALLPHYS;
		CSR_WRITE_4(sc, RL_RXCFG, rxcfg);
	} else {
		rxcfg &= ~RL_RXCFG_RX_ALLPHYS;
		CSR_WRITE_4(sc, RL_RXCFG, rxcfg);
	}

	/*
	 * Set capture broadcast bit to capture broadcast frames.
	 */
	if (ifp->if_flags & IFF_BROADCAST) {
		rxcfg |= RL_RXCFG_RX_BROAD;
		CSR_WRITE_4(sc, RL_RXCFG, rxcfg);
	} else {
		rxcfg &= ~RL_RXCFG_RX_BROAD;
		CSR_WRITE_4(sc, RL_RXCFG, rxcfg);
	}

	/*
	 * Program the multicast filter, if necessary.
	 */
	rtk_setmulti(sc);

	/*
	 * Enable interrupts.
	 */
	CSR_WRITE_2(sc, RL_IMR, RL_INTRS);

	/* Start RX/TX process. */
	CSR_WRITE_4(sc, RL_MISSEDPKT, 0);

	/* Enable receiver and transmitter. */
	CSR_WRITE_1(sc, RL_COMMAND, RL_CMD_TX_ENB|RL_CMD_RX_ENB);

	/* Restore state of BMCR */
	if (sc->rtk_type == RL_8139)
		rtk_phy_writereg((struct device *)sc, 7, MII_BMCR, phy_bmcr);

	CSR_WRITE_1(sc, RL_CFG1, RL_CFG1_DRVLOAD|RL_CFG1_FULLDUPLEX);

	/*
	 * Set current media.
	 */
	mii_mediachg(&sc->mii);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	(void)splx(s);

	callout_reset(&sc->rtk_tick_ch, hz, rtk_tick, sc);
}

/*
 * Set media options.
 */
STATIC int rtk_ifmedia_upd(ifp)
	struct ifnet		*ifp;
{
	struct rtk_softc		*sc;
	struct ifmedia		*ifm;

	sc = ifp->if_softc;
	ifm = &sc->mii.mii_media;

	if (IFM_TYPE(ifm->ifm_media) != IFM_ETHER)
		return(EINVAL);

	return (mii_mediachg(&sc->mii));
}

/*
 * Report current media status.
 */
STATIC void rtk_ifmedia_sts(ifp, ifmr)
	struct ifnet		*ifp;
	struct ifmediareq	*ifmr;
{
	struct rtk_softc		*sc;

	sc = ifp->if_softc;

	mii_pollstat(&sc->mii);
	ifmr->ifm_status = sc->mii.mii_media_status;
	ifmr->ifm_active = sc->mii.mii_media_active;
}

STATIC int
rtk_ether_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *) data;
	struct rtk_softc *sc = ifp->if_softc;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			rtk_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			 struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			 if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)
				    LLADDR(ifp->if_sadl);
			 else
				bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
				    ifp->if_addrlen);
			 /* Set new address. */
			 rtk_init(sc);
			 break;
		    }
#endif
		default:
			rtk_init(sc);
			break;
		}
		break;

	default:
		return (EINVAL);
	}

	return (0);
}

STATIC int rtk_ioctl(ifp, command, data)
	struct ifnet		*ifp;
	u_long			command;
	caddr_t			data;
{
	struct rtk_softc		*sc = ifp->if_softc;
	struct ifreq		*ifr = (struct ifreq *) data;
	int			s, error = 0;

	s = splnet();

	switch(command) {
	case SIOCSIFADDR:
	case SIOCGIFADDR:
	case SIOCSIFMTU:
		error = rtk_ether_ioctl(ifp, command, data);
		break;
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			rtk_init(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				rtk_stop(sc);
		}
		error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		rtk_setmulti(sc);
		error = 0;
		break;
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->mii.mii_media, command);
		break;
	default:
		error = EINVAL;
		break;
	}

	(void)splx(s);

	return(error);
}

STATIC void rtk_watchdog(ifp)
	struct ifnet		*ifp;
{
	struct rtk_softc		*sc;

	sc = ifp->if_softc;

	printf("%s: watchdog timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;
	rtk_txeof(sc);
	rtk_rxeof(sc);
	rtk_init(sc);

	return;
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
STATIC void rtk_stop(sc)
	struct rtk_softc		*sc;
{
	int			i;
	struct ifnet		*ifp;

	ifp = &sc->ethercom.ec_if;
	ifp->if_timer = 0;

	callout_stop(&sc->rtk_tick_ch);

	mii_down(&sc->mii);

	CSR_WRITE_1(sc, RL_COMMAND, 0x00);
	CSR_WRITE_2(sc, RL_IMR, 0x0000);

	/*
	 * Free the TX list buffers.
	 */
	for (i = 0; i < RL_TX_LIST_CNT; i++) {
		if (sc->rtk_cdata.rtk_tx_chain[i] != NULL) {
			m_freem(sc->rtk_cdata.rtk_tx_chain[i]);
			sc->rtk_cdata.rtk_tx_chain[i] = NULL;
			CSR_WRITE_4(sc, RL_TXADDR0 + i, 0x0000000);
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);

	return;
}

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
STATIC void rtk_shutdown(vsc)
	void			*vsc;
{
	struct rtk_softc		*sc = (struct rtk_softc *)vsc;

	rtk_stop(sc);

	return;
}

STATIC void
rtk_tick(arg)
	void *arg;
{
	struct rtk_softc *sc = arg;
	int s = splnet();

	mii_tick(&sc->mii);
	splx(s);

	callout_reset(&sc->rtk_tick_ch, hz, rtk_tick, sc);
}
