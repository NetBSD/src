/*	$NetBSD: rtl81x9.c,v 1.18.2.6 2001/01/05 17:35:47 bouyer Exp $	*/

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

#include <uvm/uvm_extern.h>

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

#if defined(DEBUG)
#define STATIC
#else
#define STATIC static
#endif

STATIC void rtk_reset		__P((struct rtk_softc *));
STATIC void rtk_rxeof		__P((struct rtk_softc *));
STATIC void rtk_txeof		__P((struct rtk_softc *));
STATIC void rtk_start		__P((struct ifnet *));
STATIC int rtk_ioctl		__P((struct ifnet *, u_long, caddr_t));
STATIC int rtk_init		__P((struct ifnet *));
STATIC void rtk_stop		__P((struct ifnet *, int));

STATIC void rtk_watchdog	__P((struct ifnet *));
STATIC void rtk_shutdown	__P((void *));
STATIC int rtk_ifmedia_upd	__P((struct ifnet *));
STATIC void rtk_ifmedia_sts	__P((struct ifnet *, struct ifmediareq *));

STATIC u_int16_t rtk_read_eeprom __P((struct rtk_softc *, int, int));
STATIC void rtk_eeprom_putbyte	__P((struct rtk_softc *, int, int));
STATIC void rtk_mii_sync	__P((struct rtk_softc *));
STATIC void rtk_mii_send	__P((struct rtk_softc *, u_int32_t, int));
STATIC int rtk_mii_readreg	__P((struct rtk_softc *, struct rtk_mii_frame *));
STATIC int rtk_mii_writereg	__P((struct rtk_softc *, struct rtk_mii_frame *));

STATIC int rtk_phy_readreg	__P((struct device *, int, int));
STATIC void rtk_phy_writereg	__P((struct device *, int, int, int));
STATIC void rtk_phy_statchg	__P((struct device *));
STATIC void rtk_tick		__P((void *));

STATIC int rtk_enable		__P((struct rtk_softc *));
STATIC void rtk_disable		__P((struct rtk_softc *));
STATIC void rtk_power		__P((int, void *));

STATIC void rtk_setmulti	__P((struct rtk_softc *));
STATIC int rtk_list_tx_init	__P((struct rtk_softc *));

#define EE_SET(x)					\
	CSR_WRITE_1(sc, RTK_EECMD,			\
		CSR_READ_1(sc, RTK_EECMD) | (x))

#define EE_CLR(x)					\
	CSR_WRITE_1(sc, RTK_EECMD,			\
		CSR_READ_1(sc, RTK_EECMD) & ~(x))

/*
 * Send a read command and address to the EEPROM, check for ACK.
 */
STATIC void rtk_eeprom_putbyte(sc, addr, addr_len)
	struct rtk_softc	*sc;
	int			addr, addr_len;
{
	int			d, i;

	d = (RTK_EECMD_READ << addr_len) | addr;

	/*
	 * Feed in each bit and stobe the clock.
	 */
	for (i = RTK_EECMD_LEN + addr_len; i > 0; i--) {
		if (d & (1 << (i - 1))) {
			EE_SET(RTK_EE_DATAIN);
		} else {
			EE_CLR(RTK_EE_DATAIN);
		}
		DELAY(4);
		EE_SET(RTK_EE_CLK);
		DELAY(4);
		EE_CLR(RTK_EE_CLK);
		DELAY(4);
	}
}

/*
 * Read a word of data stored in the EEPROM at address 'addr.'
 */
u_int16_t rtk_read_eeprom(sc, addr, addr_len)
	struct rtk_softc	*sc;
	int			addr, addr_len;
{
	u_int16_t		word = 0;
	int			i;

	/* Enter EEPROM access mode. */
	CSR_WRITE_1(sc, RTK_EECMD, RTK_EEMODE_PROGRAM|RTK_EE_SEL);

	/*
	 * Send address of word we want to read.
	 */
	rtk_eeprom_putbyte(sc, addr, addr_len);

	CSR_WRITE_1(sc, RTK_EECMD, RTK_EEMODE_PROGRAM|RTK_EE_SEL);

	/*
	 * Start reading bits from EEPROM.
	 */
	for (i = 16; i > 0; i--) {
		EE_SET(RTK_EE_CLK);
		DELAY(4);
		if (CSR_READ_1(sc, RTK_EECMD) & RTK_EE_DATAOUT)
			word |= 1 << (i - 1);
		EE_CLR(RTK_EE_CLK);
		DELAY(4);
	}

	/* Turn off EEPROM access mode. */
	CSR_WRITE_1(sc, RTK_EECMD, RTK_EEMODE_OFF);

	return (word);
}

/*
 * MII access routines are provided for the 8129, which
 * doesn't have a built-in PHY. For the 8139, we fake things
 * up by diverting rtk_phy_readreg()/rtk_phy_writereg() to the
 * direct access PHY registers.
 */
#define MII_SET(x)					\
	CSR_WRITE_1(sc, RTK_MII,			\
		CSR_READ_1(sc, RTK_MII) | (x))

#define MII_CLR(x)					\
	CSR_WRITE_1(sc, RTK_MII,			\
		CSR_READ_1(sc, RTK_MII) & ~(x))

/*
 * Sync the PHYs by setting data bit and strobing the clock 32 times.
 */
STATIC void rtk_mii_sync(sc)
	struct rtk_softc	*sc;
{
	int			i;

	MII_SET(RTK_MII_DIR|RTK_MII_DATAOUT);

	for (i = 0; i < 32; i++) {
		MII_SET(RTK_MII_CLK);
		DELAY(1);
		MII_CLR(RTK_MII_CLK);
		DELAY(1);
	}
}

/*
 * Clock a series of bits through the MII.
 */
STATIC void rtk_mii_send(sc, bits, cnt)
	struct rtk_softc	*sc;
	u_int32_t		bits;
	int			cnt;
{
	int			i;

	MII_CLR(RTK_MII_CLK);

	for (i = cnt; i > 0; i--) {
                if (bits & (1 << (i - 1))) {
			MII_SET(RTK_MII_DATAOUT);
                } else {
			MII_CLR(RTK_MII_DATAOUT);
                }
		DELAY(1);
		MII_CLR(RTK_MII_CLK);
		DELAY(1);
		MII_SET(RTK_MII_CLK);
	}
}

/*
 * Read an PHY register through the MII.
 */
STATIC int rtk_mii_readreg(sc, frame)
	struct rtk_softc	*sc;
	struct rtk_mii_frame	*frame;
{
	int			i, ack, s;

	s = splnet();

	/*
	 * Set up frame for RX.
	 */
	frame->mii_stdelim = RTK_MII_STARTDELIM;
	frame->mii_opcode = RTK_MII_READOP;
	frame->mii_turnaround = 0;
	frame->mii_data = 0;

	CSR_WRITE_2(sc, RTK_MII, 0);

	/*
 	 * Turn on data xmit.
	 */
	MII_SET(RTK_MII_DIR);

	rtk_mii_sync(sc);

	/*
	 * Send command/address info.
	 */
	rtk_mii_send(sc, frame->mii_stdelim, 2);
	rtk_mii_send(sc, frame->mii_opcode, 2);
	rtk_mii_send(sc, frame->mii_phyaddr, 5);
	rtk_mii_send(sc, frame->mii_regaddr, 5);

	/* Idle bit */
	MII_CLR((RTK_MII_CLK|RTK_MII_DATAOUT));
	DELAY(1);
	MII_SET(RTK_MII_CLK);
	DELAY(1);

	/* Turn off xmit. */
	MII_CLR(RTK_MII_DIR);

	/* Check for ack */
	MII_CLR(RTK_MII_CLK);
	DELAY(1);
	MII_SET(RTK_MII_CLK);
	DELAY(1);
	ack = CSR_READ_2(sc, RTK_MII) & RTK_MII_DATAIN;

	/*
	 * Now try reading data bits. If the ack failed, we still
	 * need to clock through 16 cycles to keep the PHY(s) in sync.
	 */
	if (ack) {
		for (i = 0; i < 16; i++) {
			MII_CLR(RTK_MII_CLK);
			DELAY(1);
			MII_SET(RTK_MII_CLK);
			DELAY(1);
		}
		goto fail;
	}

	for (i = 16; i > 0; i--) {
		MII_CLR(RTK_MII_CLK);
		DELAY(1);
		if (!ack) {
			if (CSR_READ_2(sc, RTK_MII) & RTK_MII_DATAIN)
				frame->mii_data |= 1 << (i - 1);
			DELAY(1);
		}
		MII_SET(RTK_MII_CLK);
		DELAY(1);
	}

 fail:
	MII_CLR(RTK_MII_CLK);
	DELAY(1);
	MII_SET(RTK_MII_CLK);
	DELAY(1);

	splx(s);

	if (ack)
		return (1);
	return (0);
}

/*
 * Write to a PHY register through the MII.
 */
STATIC int rtk_mii_writereg(sc, frame)
	struct rtk_softc	*sc;
	struct rtk_mii_frame	*frame;
{
	int			s;

	s = splnet();
	/*
	 * Set up frame for TX.
	 */
	frame->mii_stdelim = RTK_MII_STARTDELIM;
	frame->mii_opcode = RTK_MII_WRITEOP;
	frame->mii_turnaround = RTK_MII_TURNAROUND;
	
	/*
 	 * Turn on data output.
	 */
	MII_SET(RTK_MII_DIR);

	rtk_mii_sync(sc);

	rtk_mii_send(sc, frame->mii_stdelim, 2);
	rtk_mii_send(sc, frame->mii_opcode, 2);
	rtk_mii_send(sc, frame->mii_phyaddr, 5);
	rtk_mii_send(sc, frame->mii_regaddr, 5);
	rtk_mii_send(sc, frame->mii_turnaround, 2);
	rtk_mii_send(sc, frame->mii_data, 16);

	/* Idle bit. */
	MII_SET(RTK_MII_CLK);
	DELAY(1);
	MII_CLR(RTK_MII_CLK);
	DELAY(1);

	/*
	 * Turn off xmit.
	 */
	MII_CLR(RTK_MII_DIR);

	splx(s);

	return (0);
}

STATIC int rtk_phy_readreg(self, phy, reg)
	struct device		*self;
	int			phy, reg;
{
	struct rtk_softc	*sc = (void *)self;
	struct rtk_mii_frame	frame;
	int			rval = 0;
	int			rtk8139_reg = 0;

	if (sc->rtk_type == RTK_8139) {
		if (phy != 7)
			return (0);

		switch(reg) {
		case MII_BMCR:
			rtk8139_reg = RTK_BMCR;
			break;
		case MII_BMSR:
			rtk8139_reg = RTK_BMSR;
			break;
		case MII_ANAR:
			rtk8139_reg = RTK_ANAR;
			break;
		case MII_ANER:
			rtk8139_reg = RTK_ANER;
			break;
		case MII_ANLPAR:
			rtk8139_reg = RTK_LPAR;
			break;
		default:
#if 0
			printf("%s: bad phy register\n", sc->sc_dev.dv_xname);
#endif
			return (0);
		}
		rval = CSR_READ_2(sc, rtk8139_reg);
		return (rval);
	}

	bzero((char *)&frame, sizeof(frame));

	frame.mii_phyaddr = phy;
	frame.mii_regaddr = reg;
	rtk_mii_readreg(sc, &frame);

	return (frame.mii_data);
}

STATIC void rtk_phy_writereg(self, phy, reg, data)
	struct device		*self;
	int			phy, reg;
	int			data;
{
	struct rtk_softc	*sc = (void *)self;
	struct rtk_mii_frame	frame;
	int			rtk8139_reg = 0;

	if (sc->rtk_type == RTK_8139) {
		if (phy != 7)
			return;

		switch(reg) {
		case MII_BMCR:
			rtk8139_reg = RTK_BMCR;
			break;
		case MII_BMSR:
			rtk8139_reg = RTK_BMSR;
			break;
		case MII_ANAR:
			rtk8139_reg = RTK_ANAR;
			break;
		case MII_ANER:
			rtk8139_reg = RTK_ANER;
			break;
		case MII_ANLPAR:
			rtk8139_reg = RTK_LPAR;
			break;
		default:
#if 0
			printf("%s: bad phy register\n", sc->sc_dev.dv_xname);
#endif
			return;
		}
		CSR_WRITE_2(sc, rtk8139_reg, data);
		return;
	}

	bzero((char *)&frame, sizeof(frame));

	frame.mii_phyaddr = phy;
	frame.mii_regaddr = reg;
	frame.mii_data = data;

	rtk_mii_writereg(sc, &frame);
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
	struct rtk_softc	*sc;
{
	struct ifnet		*ifp;
	int			h = 0;
	u_int32_t		hashes[2] = { 0, 0 };
	u_int32_t		rxfilt;
	int			mcnt = 0;
	struct ether_multi *enm;
	struct ether_multistep step;

	ifp = &sc->ethercom.ec_if;

	rxfilt = CSR_READ_4(sc, RTK_RXCFG);

	if (ifp->if_flags & IFF_ALLMULTI || ifp->if_flags & IFF_PROMISC) {
		rxfilt |= RTK_RXCFG_RX_MULTI;
		CSR_WRITE_4(sc, RTK_RXCFG, rxfilt);
		CSR_WRITE_4(sc, RTK_MAR0, 0xFFFFFFFF);
		CSR_WRITE_4(sc, RTK_MAR4, 0xFFFFFFFF);
		return;
	}

	/* first, zot all the existing hash bits */
	CSR_WRITE_4(sc, RTK_MAR0, 0);
	CSR_WRITE_4(sc, RTK_MAR4, 0);

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
		rxfilt |= RTK_RXCFG_RX_MULTI;
	else
		rxfilt &= ~RTK_RXCFG_RX_MULTI;

	CSR_WRITE_4(sc, RTK_RXCFG, rxfilt);
	CSR_WRITE_4(sc, RTK_MAR0, hashes[0]);
	CSR_WRITE_4(sc, RTK_MAR4, hashes[1]);
}

void rtk_reset(sc)
	struct rtk_softc	*sc;
{
	int			i;

	CSR_WRITE_1(sc, RTK_COMMAND, RTK_CMD_RESET);

	for (i = 0; i < RTK_TIMEOUT; i++) {
		DELAY(10);
		if ((CSR_READ_1(sc, RTK_COMMAND) & RTK_CMD_RESET) == 0)
			break;
	}
	if (i == RTK_TIMEOUT)
		printf("%s: reset never completed!\n", sc->sc_dev.dv_xname);
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
	int error;
	int i, addr_len;

	callout_init(&sc->rtk_tick_ch);

	/*
	 * Check EEPROM type 9346 or 9356.
	 */
	if (rtk_read_eeprom(sc, RTK_EE_ID, RTK_EEADDR_LEN1) == 0x8129)
		addr_len = RTK_EEADDR_LEN1;
	else
		addr_len = RTK_EEADDR_LEN0;

	/*
	 * Get station address.
	 */
	val = rtk_read_eeprom(sc, RTK_EE_EADDR0, addr_len);
	eaddr[0] = val & 0xff;
	eaddr[1] = val >> 8;
	val = rtk_read_eeprom(sc, RTK_EE_EADDR1, addr_len);
	eaddr[2] = val & 0xff;
	eaddr[3] = val >> 8;
	val = rtk_read_eeprom(sc, RTK_EE_EADDR2, addr_len);
	eaddr[4] = val & 0xff;
	eaddr[5] = val >> 8;

	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    RTK_RXBUFLEN + 16, PAGE_SIZE, 0, &sc->sc_dmaseg, 1, &sc->sc_dmanseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't allocate recv buffer, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dmaseg, sc->sc_dmanseg,
	    RTK_RXBUFLEN + 16, (caddr_t *)&sc->rtk_cdata.rtk_rx_buf,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: can't map recv buffer, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    RTK_RXBUFLEN + 16, 1, RTK_RXBUFLEN + 16, 0, BUS_DMA_NOWAIT,
	    &sc->recv_dmamap)) != 0) {
		printf("%s: can't create recv buffer DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->recv_dmamap,
	    sc->rtk_cdata.rtk_rx_buf, RTK_RXBUFLEN + 16,
	    NULL, BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't load recv buffer DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	for (i = 0; i < RTK_TX_LIST_CNT; i++)
		if ((error = bus_dmamap_create(sc->sc_dmat,
		    MCLBYTES, 1, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->snd_dmamap[i])) != 0) {
			printf("%s: can't create snd buffer DMA map,"
			    " error = %d\n", sc->sc_dev.dv_xname, error);
			goto fail_4;
		}
	/*
	 * From this point forward, the attachment cannot fail. A failure
	 * before this releases all resources thar may have been
	 * allocated.
	 */
	sc->sc_flags |= RTK_ATTACHED;

	/* Reset the adapter. */
	rtk_reset(sc);

	printf("%s: Ethernet address %s\n",
	    sc->sc_dev.dv_xname, ether_sprintf(eaddr));

	ifp = &sc->ethercom.ec_if;
	ifp->if_softc = sc;
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = rtk_ioctl;
	ifp->if_start = rtk_start;
	ifp->if_watchdog = rtk_watchdog;
	ifp->if_init = rtk_init;
	ifp->if_stop = rtk_stop;
	IFQ_SET_READY(&ifp->if_snd);

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
		ifmedia_add(&sc->mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->mii.mii_media, IFM_ETHER|IFM_NONE);
	} else {
		ifmedia_set(&sc->mii.mii_media, IFM_ETHER|IFM_AUTO);
	}

	/*
	 * Call MI attach routines.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, eaddr);

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(rtk_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unbale to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	/*
	 * Add a suspend hook to make sure we come back up after a
	 * resume.
	 */
	sc->sc_powerhook = powerhook_establish(rtk_power, sc);
	if (sc->sc_powerhook == NULL)
		printf("%s: WARNING: unable to establish power hook\n",
		    sc->sc_dev.dv_xname);

	return;
 fail_4:
	for (i = 0; i < RTK_TX_LIST_CNT; i++)
		if (sc->snd_dmamap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->snd_dmamap[i]);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->recv_dmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->rtk_cdata.rtk_rx_buf,
	    RTK_RXBUFLEN + 16);
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dmaseg, sc->sc_dmanseg);
 fail_0:
	return;
}

/*
 * Initialize the transmit descriptors.
 */
STATIC int rtk_list_tx_init(sc)
	struct rtk_softc	*sc;
{
	struct rtk_chain_data	*cd;
	int			i;

	cd = &sc->rtk_cdata;
	for (i = 0; i < RTK_TX_LIST_CNT; i++) {
		cd->rtk_tx_chain[i] = NULL;
		CSR_WRITE_4(sc,
		    RTK_TXADDR0 + (i * sizeof(u_int32_t)), 0x0000000);
	}

	sc->rtk_cdata.cur_tx = 0;
	sc->rtk_cdata.last_tx = 0;

	return (0);
}

/*
 * rtk_activate:
 *     Handle device activation/deactivation requests.
 */
int
rtk_activate(self, act)
	struct device *self;
	enum devact act;
{
	struct rtk_softc *sc = (void *) self;
	int s, error = 0;

	s = splnet();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;
	case DVACT_DEACTIVATE:
		mii_activate(&sc->mii, act, MII_PHY_ANY, MII_OFFSET_ANY);
		if_deactivate(&sc->ethercom.ec_if);
		break;
	}
	splx(s);

	return (error);
}

/*
 * rtk_detach:
 *     Detach a rtk interface.
 */
int 
rtk_detach(sc)
	struct rtk_softc *sc;
{
	struct ifnet *ifp = &sc->ethercom.ec_if;
	int i;

	/*
	 * Succeed now if thereisn't any work to do.
	 */
	if ((sc->sc_flags & RTK_ATTACHED) == 0)
		return (0);

	/* Unhook our tick handler. */
	callout_stop(&sc->rtk_tick_ch);

	/* Detach all PHYs. */
	mii_detach(&sc->mii, MII_PHY_ANY, MII_OFFSET_ANY);

	/* Delete all remaining media. */
	ifmedia_delete_instance(&sc->mii.mii_media, IFM_INST_ANY);

	ether_ifdetach(ifp);
	if_detach(ifp);

	for (i = 0; i < RTK_TX_LIST_CNT; i++)
		if (sc->snd_dmamap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->snd_dmamap[i]);
	bus_dmamap_destroy(sc->sc_dmat, sc->recv_dmamap);
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->rtk_cdata.rtk_rx_buf,
	    RTK_RXBUFLEN + 16);
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dmaseg, sc->sc_dmanseg);

	shutdownhook_disestablish(sc->sc_sdhook);
	powerhook_disestablish(sc->sc_powerhook);

	return (0);
}

/*
 * rtk_enable:
 *     Enable the RTL81X9 chip.
 */
int 
rtk_enable(sc)
	struct rtk_softc *sc;
{

	if (RTK_IS_ENABLED(sc) == 0 && sc->sc_enable != NULL) {
		if ((*sc->sc_enable)(sc) != 0) {
			printf("%s: device enable failed\n",
			    sc->sc_dev.dv_xname);
			return (EIO);
		}
		sc->sc_flags |= RTK_ENABLED;
	}
	return (0);
}

/*
 * rtk_disable:
 *     Disable the RTL81X9 chip.
 */
void 
rtk_disable(sc)
	struct rtk_softc *sc;
{

	if (RTK_IS_ENABLED(sc) && sc->sc_disable != NULL) {
		(*sc->sc_disable)(sc);
		sc->sc_flags &= ~RTK_ENABLED;
	}
}

/*
 * rtk_power:
 *     Power management (suspend/resume) hook.
 */
void 
rtk_power(why, arg)
	int why;
	void *arg;
{
	struct rtk_softc *sc = (void *) arg;
	struct ifnet *ifp = &sc->ethercom.ec_if;
	int s;

	s = splnet();
	switch (why) {
	case PWR_SUSPEND:
	case PWR_STANDBY:
		rtk_stop(ifp, 0);
		if (sc->sc_power != NULL)
			(*sc->sc_power)(sc, why);
		break;
	case PWR_RESUME:
		if (ifp->if_flags & IFF_UP) {
			if (sc->sc_power != NULL)
				(*sc->sc_power)(sc, why);
			rtk_init(ifp);
		}
		break;
	case PWR_SOFTSUSPEND:
	case PWR_SOFTSTANDBY:
	case PWR_SOFTRESUME:
		break;
	}
	splx(s);
}

/*
 * A frame has been uploaded: pass the resulting mbuf chain up to
 * the higher level protocols.
 *
 * You know there's something wrong with a PCI bus-master chip design.
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
 * on a 32-bit boundary. To achieve this, we copy the data to mbuf
 * shifted forward 2 bytes.
 */
STATIC void rtk_rxeof(sc)
	struct rtk_softc	*sc;
{
        struct mbuf		*m;
        struct ifnet		*ifp;
	caddr_t			rxbufpos, dst;
	int			total_len, wrap = 0;
	u_int32_t		rxstat;
	u_int16_t		cur_rx, new_rx;
	u_int16_t		limit;
	u_int16_t		rx_bytes = 0, max_bytes;

	ifp = &sc->ethercom.ec_if;

	cur_rx = (CSR_READ_2(sc, RTK_CURRXADDR) + 16) % RTK_RXBUFLEN;

	/* Do not try to read past this point. */
	limit = CSR_READ_2(sc, RTK_CURRXBUF) % RTK_RXBUFLEN;

	if (limit < cur_rx)
		max_bytes = (RTK_RXBUFLEN - cur_rx) + limit;
	else
		max_bytes = limit - cur_rx;

	while((CSR_READ_1(sc, RTK_COMMAND) & RTK_CMD_EMPTY_RXBUF) == 0) {
		rxbufpos = sc->rtk_cdata.rtk_rx_buf + cur_rx;
		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap, cur_rx,
		    RTK_RXSTAT_LEN, BUS_DMASYNC_POSTREAD);
		rxstat = le32toh(*(u_int32_t *)rxbufpos);
		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap, cur_rx,
		    RTK_RXSTAT_LEN, BUS_DMASYNC_PREREAD);

		/*
		 * Here's a totally undocumented fact for you. When the
		 * RealTek chip is in the process of copying a packet into
		 * RAM for you, the length will be 0xfff0. If you spot a
		 * packet header with this value, you need to stop. The
		 * datasheet makes absolutely no mention of this and
		 * RealTek should be shot for this.
		 */
		total_len = rxstat >> 16;
		if (total_len == RTK_RXSTAT_UNFINISHED)
			break;

		if ((rxstat & RTK_RXSTAT_RXOK) == 0) {
			ifp->if_ierrors++;

			/*
			 * submitted by:[netbsd-pcmcia:00484] 
			 *	Takahiro Kambe <taca@sky.yamashina.kyoto.jp>
			 * obtain from:
			 *     FreeBSD if_rl.c rev 1.24->1.25
			 *
			 */
#if 0
			if (rxstat & (RTK_RXSTAT_BADSYM|RTK_RXSTAT_RUNT|
			    RTK_RXSTAT_GIANT|RTK_RXSTAT_CRCERR|
			    RTK_RXSTAT_ALIGNERR)) {
				CSR_WRITE_2(sc, RTK_COMMAND, RTK_CMD_TX_ENB);
				CSR_WRITE_2(sc, RTK_COMMAND,
				    RTK_CMD_TX_ENB|RTK_CMD_RX_ENB);
				CSR_WRITE_4(sc, RTK_RXCFG, RTK_RXCFG_CONFIG);
				CSR_WRITE_4(sc, RTK_RXADDR,
				    sc->recv_dmamap->dm_segs[0].ds_addr);
				cur_rx = 0;
			}
			break;
#else
			rtk_init(ifp);
			return;
#endif
		}

		/* No errors; receive the packet. */	
		rx_bytes += total_len + RTK_RXSTAT_LEN;

		/*
		 * Avoid trying to read more bytes than we know
		 * the chip has prepared for us.
		 */
		if (rx_bytes > max_bytes)
			break;

		/*
		 * Skip the status word, wrapping around to the beginning
		 * of the Rx area, if necessary.
		 */
		cur_rx += RTK_RXSTAT_LEN;
		rxbufpos = sc->rtk_cdata.rtk_rx_buf + (cur_rx % RTK_RXBUFLEN);

		/*
		 * Compute the number of bytes at which the packet
		 * will wrap to the beginning of the ring buffer.
		 */
		wrap = RTK_RXBUFLEN - (cur_rx % RTK_RXBUFLEN);

		/*
		 * Compute where the next pending packet is.
		 */
		if (total_len > wrap)
			new_rx = total_len - wrap;
		else
			new_rx = cur_rx + total_len;
		/* Round up to 32-bit boundary. */
		new_rx = (new_rx + 3) & ~3;

		/*
		 * Now allocate an mbuf (and possibly a cluster) to hold
		 * the packet. Note we offset the packet 2 bytes so that
		 * data after the Ethernet header will be 4-byte aligned.
		 */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			printf("%s: unable to allocate Rx mbuf\n",
			    sc->sc_dev.dv_xname);
			ifp->if_ierrors++;
			goto next_packet;
		}
		if (total_len > (MHLEN - RTK_ETHER_ALIGN)) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				printf("%s: unable to allocate Rx cluster\n",
				    sc->sc_dev.dv_xname);
				ifp->if_ierrors++;
				m_freem(m);
				m = NULL;
				goto next_packet;
			}
		}
		m->m_data += RTK_ETHER_ALIGN;	/* for alignment */
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = total_len;
		dst = mtod(m, caddr_t);

		/*
		 * If the packet wraps, copy up to the wrapping point.
		 */
		if (total_len > wrap) {
			bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap,
			    cur_rx, wrap, BUS_DMASYNC_POSTREAD);
			memcpy(dst, rxbufpos, wrap);
			bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap,
			    cur_rx, wrap, BUS_DMASYNC_PREREAD);
			cur_rx = 0;
			rxbufpos = sc->rtk_cdata.rtk_rx_buf;
			total_len -= wrap;
			dst += wrap;
		}

		/*
		 * ...and now the rest.
		 */
		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap,
		    cur_rx, total_len, BUS_DMASYNC_POSTREAD);
		memcpy(dst, rxbufpos, total_len);
		bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap,
		    cur_rx, total_len, BUS_DMASYNC_PREREAD);

 next_packet:
		CSR_WRITE_2(sc, RTK_CURRXADDR, new_rx - 16);
		cur_rx = new_rx;

		if (m == NULL)
			continue;

		/*
		 * The RealTek chip includes the CRC with every
		 * incoming packet.
		 */
		m->m_flags |= M_HASFCS;

		ifp->if_ipackets++;

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		/* pass it on. */
		(*ifp->if_input)(ifp, m);
	}
}

/*
 * A frame was downloaded to the chip. It's safe for us to clean up
 * the list buffers.
 */
STATIC void rtk_txeof(sc)
	struct rtk_softc	*sc;
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
		txstat = CSR_READ_4(sc, RTK_LAST_TXSTAT(sc));
		if ((txstat & (RTK_TXSTAT_TX_OK|
		    RTK_TXSTAT_TX_UNDERRUN|RTK_TXSTAT_TXABRT)) == 0)
			break;

		bus_dmamap_sync(sc->sc_dmat,
		    sc->snd_dmamap[sc->rtk_cdata.last_tx], 0,
		    sc->snd_dmamap[sc->rtk_cdata.last_tx]->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat,
		    sc->snd_dmamap[sc->rtk_cdata.last_tx]);
		m_freem(RTK_LAST_TXMBUF(sc));
		RTK_LAST_TXMBUF(sc) = NULL;

		ifp->if_collisions += (txstat & RTK_TXSTAT_COLLCNT) >> 24;

		if (txstat & RTK_TXSTAT_TX_OK)
			ifp->if_opackets++;
		else {
			ifp->if_oerrors++;
			if (txstat & (RTK_TXSTAT_TXABRT|RTK_TXSTAT_OUTOFWIN))
				CSR_WRITE_4(sc, RTK_TXCFG, RTK_TXCFG_CONFIG);
		}
		RTK_INC(sc->rtk_cdata.last_tx);
		ifp->if_flags &= ~IFF_OACTIVE;
	} while (sc->rtk_cdata.last_tx != sc->rtk_cdata.cur_tx);
}

int rtk_intr(arg)
	void			*arg;
{
	struct rtk_softc	*sc;
	struct ifnet		*ifp;
	u_int16_t		status;
	int handled = 0;

	sc = arg;
	ifp = &sc->ethercom.ec_if;

	/* Disable interrupts. */
	CSR_WRITE_2(sc, RTK_IMR, 0x0000);

	for (;;) {

		status = CSR_READ_2(sc, RTK_ISR);
		if (status)
			CSR_WRITE_2(sc, RTK_ISR, status);

		handled = 1;

		if ((status & RTK_INTRS) == 0)
			break;

		if (status & RTK_ISR_RX_OK)
			rtk_rxeof(sc);

		if (status & RTK_ISR_RX_ERR)
			rtk_rxeof(sc);

		if (status & (RTK_ISR_TX_OK|RTK_ISR_TX_ERR))
			rtk_txeof(sc);

		if (status & RTK_ISR_SYSTEM_ERR) {
			rtk_reset(sc);
			rtk_init(ifp);
		}
	}

	/* Re-enable interrupts. */
	CSR_WRITE_2(sc, RTK_IMR, RTK_INTRS);

	if (IFQ_IS_EMPTY(&ifp->if_snd) == 0)
		rtk_start(ifp);

	return (handled);
}

/*
 * Main transmit routine.
 */

STATIC void rtk_start(ifp)
	struct ifnet		*ifp;
{
	struct rtk_softc	*sc;
	struct mbuf		*m_head = NULL, *m_new;
	int			error, idx, len;

	sc = ifp->if_softc;

	while(RTK_CUR_TXMBUF(sc) == NULL) {
		IFQ_POLL(&ifp->if_snd, m_head);
		if (m_head == NULL)
			break;
		m_new = NULL;

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
				break;
			}
			if (m_head->m_pkthdr.len > MHLEN) {
				MCLGET(m_new, M_DONTWAIT);
				if ((m_new->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					    "cluster\n", sc->sc_dev.dv_xname);
					m_freem(m_new);
					break;
				}
			}
			m_copydata(m_head, 0, m_head->m_pkthdr.len,
			    mtod(m_new, caddr_t));
			m_new->m_pkthdr.len = m_new->m_len =
			    m_head->m_pkthdr.len;
			error = bus_dmamap_load_mbuf(sc->sc_dmat,
			    sc->snd_dmamap[idx], m_new, BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
				break;
			}
		}
		IFQ_DEQUEUE(&ifp->if_snd, m_head);
		if (m_new != NULL) {
			m_freem(m_head);
			m_head = m_new;
		}

		RTK_CUR_TXMBUF(sc) = m_head;

#if NBPFILTER > 0
		/*
		 * If there's a BPF listener, bounce a copy of this frame
		 * to him.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, RTK_CUR_TXMBUF(sc));
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

		CSR_WRITE_4(sc, RTK_CUR_TXADDR(sc),
		    sc->snd_dmamap[idx]->dm_segs[0].ds_addr);
		CSR_WRITE_4(sc, RTK_CUR_TXSTAT(sc), RTK_TX_EARLYTHRESH | len);

		RTK_INC(sc->rtk_cdata.cur_tx);
	}

	/*
	 * We broke out of the loop because all our TX slots are
	 * full. Mark the NIC as busy until it drains some of the
	 * packets from the queue.
	 */
	if (RTK_CUR_TXMBUF(sc) != NULL)
		ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Set a timeout in case the chip goes out to lunch.
	 */
	ifp->if_timer = 5;
}

STATIC int rtk_init(ifp)
	struct ifnet *ifp;
{
	struct rtk_softc	*sc = ifp->if_softc;
	int			error = 0, i;
	u_int32_t		rxcfg;

	if ((error = rtk_enable(sc)) != 0)
		goto out;

	/*
	 * Cancel pending I/O.
	 */
	rtk_stop(ifp, 0);

	/* Init our MAC address */
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		CSR_WRITE_1(sc, RTK_IDR0 + i, LLADDR(ifp->if_sadl)[i]);
	}

	/* Init the RX buffer pointer register. */
	bus_dmamap_sync(sc->sc_dmat, sc->recv_dmamap, 0,
	    sc->recv_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
	CSR_WRITE_4(sc, RTK_RXADDR, sc->recv_dmamap->dm_segs[0].ds_addr);

	/* Init TX descriptors. */
	rtk_list_tx_init(sc);

	/*
	 * Enable transmit and receive.
	 */
	CSR_WRITE_1(sc, RTK_COMMAND, RTK_CMD_TX_ENB|RTK_CMD_RX_ENB);

	/*
	 * Set the initial TX and RX configuration.
	 */
	CSR_WRITE_4(sc, RTK_TXCFG, RTK_TXCFG_CONFIG);
	CSR_WRITE_4(sc, RTK_RXCFG, RTK_RXCFG_CONFIG);

	/* Set the individual bit to receive frames for this host only. */
	rxcfg = CSR_READ_4(sc, RTK_RXCFG);
	rxcfg |= RTK_RXCFG_RX_INDIV;

	/* If we want promiscuous mode, set the allframes bit. */
	if (ifp->if_flags & IFF_PROMISC) {
		rxcfg |= RTK_RXCFG_RX_ALLPHYS;
		CSR_WRITE_4(sc, RTK_RXCFG, rxcfg);
	} else {
		rxcfg &= ~RTK_RXCFG_RX_ALLPHYS;
		CSR_WRITE_4(sc, RTK_RXCFG, rxcfg);
	}

	/*
	 * Set capture broadcast bit to capture broadcast frames.
	 */
	if (ifp->if_flags & IFF_BROADCAST) {
		rxcfg |= RTK_RXCFG_RX_BROAD;
		CSR_WRITE_4(sc, RTK_RXCFG, rxcfg);
	} else {
		rxcfg &= ~RTK_RXCFG_RX_BROAD;
		CSR_WRITE_4(sc, RTK_RXCFG, rxcfg);
	}

	/*
	 * Program the multicast filter, if necessary.
	 */
	rtk_setmulti(sc);

	/*
	 * Enable interrupts.
	 */
	CSR_WRITE_2(sc, RTK_IMR, RTK_INTRS);

	/* Start RX/TX process. */
	CSR_WRITE_4(sc, RTK_MISSEDPKT, 0);

	/* Enable receiver and transmitter. */
	CSR_WRITE_1(sc, RTK_COMMAND, RTK_CMD_TX_ENB|RTK_CMD_RX_ENB);

	CSR_WRITE_1(sc, RTK_CFG1, RTK_CFG1_DRVLOAD|RTK_CFG1_FULLDUPLEX);

	/*
	 * Set current media.
	 */
	mii_mediachg(&sc->mii);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	callout_reset(&sc->rtk_tick_ch, hz, rtk_tick, sc);

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	}
	return (error);
}

/*
 * Set media options.
 */
STATIC int rtk_ifmedia_upd(ifp)
	struct ifnet		*ifp;
{
	struct rtk_softc	*sc;

	sc = ifp->if_softc;

	return (mii_mediachg(&sc->mii));
}

/*
 * Report current media status.
 */
STATIC void rtk_ifmedia_sts(ifp, ifmr)
	struct ifnet		*ifp;
	struct ifmediareq	*ifmr;
{
	struct rtk_softc	*sc;

	sc = ifp->if_softc;

	mii_pollstat(&sc->mii);
	ifmr->ifm_status = sc->mii.mii_media_status;
	ifmr->ifm_active = sc->mii.mii_media_active;
}

STATIC int rtk_ioctl(ifp, command, data)
	struct ifnet		*ifp;
	u_long			command;
	caddr_t			data;
{
	struct rtk_softc	*sc = ifp->if_softc;
	struct ifreq		*ifr = (struct ifreq *) data;
	int			s, error = 0;

	s = splnet();

	switch (command) {
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->mii.mii_media, command);
		break;

	default:
		error = ether_ioctl(ifp, command, data);
		if (error == ENETRESET) {
			if (RTK_IS_ENABLED(sc)) {
				/*
				 * Multicast list has changed.  Set the
				 * hardware filter accordingly.
				 */
				rtk_setmulti(sc);
			}
			error = 0;
		}
		break;
	}

	splx(s);

	return (error);
}

STATIC void rtk_watchdog(ifp)
	struct ifnet		*ifp;
{
	struct rtk_softc	*sc;

	sc = ifp->if_softc;

	printf("%s: watchdog timeout\n", sc->sc_dev.dv_xname);
	ifp->if_oerrors++;
	rtk_txeof(sc);
	rtk_rxeof(sc);
	rtk_init(ifp);
}

/*
 * Stop the adapter and free any mbufs allocated to the
 * RX and TX lists.
 */
STATIC void rtk_stop(ifp, disable)
	struct ifnet *ifp;
	int disable;
{
	struct rtk_softc *sc = ifp->if_softc;
	int i;

	callout_stop(&sc->rtk_tick_ch);

	mii_down(&sc->mii);

	CSR_WRITE_1(sc, RTK_COMMAND, 0x00);
	CSR_WRITE_2(sc, RTK_IMR, 0x0000);

	/*
	 * Free the TX list buffers.
	 */
	for (i = 0; i < RTK_TX_LIST_CNT; i++) {
		if (sc->rtk_cdata.rtk_tx_chain[i] != NULL) {
			bus_dmamap_unload(sc->sc_dmat, sc->snd_dmamap[i]);
			m_freem(sc->rtk_cdata.rtk_tx_chain[i]);
			sc->rtk_cdata.rtk_tx_chain[i] = NULL;
			CSR_WRITE_4(sc, RTK_TXADDR0 + i, 0x0000000);
		}
	}

	if (disable)
		rtk_disable(sc);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/*
 * Stop all chip I/O so that the kernel's probe routines don't
 * get confused by errant DMAs when rebooting.
 */
STATIC void rtk_shutdown(vsc)
	void			*vsc;
{
	struct rtk_softc	*sc = (struct rtk_softc *)vsc;

	rtk_stop(&sc->ethercom.ec_if, 0);
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
