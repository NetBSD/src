/*	$NetBSD: tlp.c,v 1.9 2010/01/19 15:28:52 tsutsui Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include <mips/cpuregs.h>

#include <machine/cpu.h>

#include "boot.h"

/*
 * - little endian access for CSR register.
 * - assume KSEG0 on vtophys() translation. 
 * - PIPT writeback cache aware.
 */
#define CSR_WRITE(l, r, v)					 	\
do {									\
	*(volatile uint32_t *)((l)->csr + (r)) = (v);			\
} while (0)
#define CSR_READ(l, r)		(*(volatile uint32_t *)((l)->csr + (r)))
#define VTOPHYS(va) 		MIPS_KSEG0_TO_PHYS(va)
#define wb(adr, siz)		pdcache_wb((uint32_t)(adr), (u_int)(siz))
#define wbinv(adr, siz)		pdcache_wbinv((uint32_t)(adr), (u_int)(siz))
#define inv(adr, siz)		pdcache_inv((uint32_t)(adr), (u_int)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T, A)	(T *)((uint32_t)alloc(sizeof(T) + (A)) & ~((A) - 1))

#define T0_OWN		(1U<<31)	/* desc is ready to tx */
#define T0_ES		(1U<<15)	/* Tx error summary */
#define T1_LS		(1U<<30)	/* last segment */
#define T1_FS		(1U<<29)	/* first segment */
#define T1_SET		(1U<<27)	/* "setup packet" */
#define T1_TER		(1U<<25)	/* end of ring mark */
#define T1_TCH		(1U<<24)	/* Second address chained */
#define T1_TBS_MASK	0x7ff		/* segment size 10:0 */
#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_FS		(1U<<30)	/* first desc of frame */
#define R0_LS		(1U<<8)		/* last desc of frame */
#define R0_ES		(1U<<15)	/* Rx error summary */
#define R1_RCH		(1U<<24)	/* Second address chained */
#define R1_RER		(1U<<25)	/* end of ring mark */
#define R0_FL_MASK	0x3fff0000	/* frame length 29:16 */
#define R1_RBS_MASK	0x7ff		/* segment size 10:0 */

#define DESCSIZE	16
struct desc {
	volatile uint32_t xd0, xd1, xd2, xd3;
#if CACHELINESIZE > DESCSIZE
	uint8_t pad[CACHELINESIZE - DESCSIZE];
#endif
};

#define TLP_BMR		0x00		/* 0: bus mode */
#define  BMR_RST	(1U<< 0)	/* software reset */
#define  BMR_BAR	(1U<< 1)	/* bus arbitration */
#define  BMR_PBL8	(1U<<11)	/* burst length 8 longword */
#define  BMR_CAL8	(1U<<13)	/* cache alignment 8 longword */
#define TLP_TPD		0x08		/* 1: instruct Tx to start */
#define  TPD_POLL	(1U<< 0)	/* transmit poll demand */
#define TLP_RPD		0x10		/* 2: instruct Rx to start */
#define  RPD_POLL	(1U<< 0)	/* receive poll demand */
#define TLP_RRBA	0x18		/* 3: Rx descriptor base */
#define TLP_TRBA	0x20		/* 4: Tx descriptor base */
#define TLP_STS		0x28		/* 5: status */
#define  STS_TS		0x00700000	/* Tx status */
#define  STS_RS		0x000e0000	/* Rx status */
#define TLP_OMR		0x30		/* 6: operation mode */
#define  OMR_SDP	(1U<<25)	/* always ON */
#define  OMR_PS		(1U<<18)	/* port select */
#define  OMR_PM		(1U<< 6)	/* promicuous */
#define  OMR_TEN	(1U<<13)	/* instruct start/stop Tx */
#define  OMR_REN	(1U<< 1)	/* instruct start/stop Rx */
#define  OMR_FD		(1U<< 9)	/* FDX */
#define TLP_IEN		0x38		/* 7: interrupt enable mask */
#define TLP_APROM	0x48		/* 9: SEEPROM and MII management */
#define  SROM_RD	(1U <<14)	/* read operation */
#define  SROM_WR	(1U <<13)	/* write openration */
#define  SROM_SR	(1U <<11)	/* SEEPROM select */
#define TLP_CSR12	0x60		/* SIA status */

#define TLP_CSR13	0x68		/* SIA connectivity register */
#define  SIACONN_10BT	0x0000ef01	/* 10BASE-T for 21041 */

#define TLP_CSR14	0x70		/* SIA TX RX register */
#define  SIATXRX_10BT	0x0000ffff	/* 10BASE-T for 21041 pass 2 */

#define TLP_CSR15	0x78		/* SIA general register */
#define  SIAGEN_MD0	(1U<<16)
#define  SIAGEN_CWE	(1U<<28)
#define  SIAGEN_10BT	0x00000000	/* 10BASE-T for 21041 */

#define TLP_SETUP_NADDR	16
#define TLP_SETUPLEN	192		/* 16 * 3 * sizeof(uint32_t) */

#define FRAMESIZE	1536
#define BUFSIZE		2048
#define NTXBUF		2
#define NEXT_TXBUF(x)	(((x) + 1) & (NTXBUF - 1))
#define NRXBUF		2
#define NEXT_RXBUF(x)	(((x) + 1) & (NRXBUF - 1))

struct local {
	struct desc txd[NTXBUF];
	struct desc rxd[NRXBUF];
	uint8_t txstore[TLP_SETUPLEN];
	uint8_t rxstore[NRXBUF][BUFSIZE];
	uint32_t csr, omr;
	u_int tx;
	u_int rx;
	u_int sromsft;
	u_int phy;
	uint32_t bmsr, anlpar;
};

#define COBALT_TLP0_BASE	0x10100000
#define SROM_MAC_OFFSET		0

static void size_srom(struct local *);
static u_int read_srom(struct local *, int);
#if 0
static u_int tlp_mii_read(struct local *, int, int);
static void tlp_mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
#endif

void *
tlp_init(void *cookie)
{
	uint32_t val, tag;
	struct local *l;
	struct desc *txd, *rxd;
	uint8_t *en, *p;
	int i;
	int is21041;

	if (cobalt_id == COBALT_ID_QUBE2700)
		is21041 = 1;
	else
		is21041 = 0;

	l = ALLOC(struct local, CACHELINESIZE);
	memset(l, 0, sizeof(struct local));

	DPRINTF(("tlp: l = %p, txd[0] = %p, txd[1] = %p\n",
	    l, &l->txd[0], &l->txd[1]));
	DPRINTF(("tlp: rxd[0] = %p, rxd[1] = %p\n",
	    &l->rxd[0], &l->rxd[1]));
	DPRINTF(("tlp: txstore = %p, rxstore[0] = %p, rxstore[1] = %p\n",
	    l->txstore, l->rxstore[0], l->rxstore[1]));

#if 1
	/* XXX assume tlp0 at pci0 dev 7 function 0 */
	tag = (0 << 16) | ( 7 << 11) | (0 << 8);
	/* memory map is not initialized by the firmware on cobalt */
	l->csr = MIPS_PHYS_TO_KSEG1(pcicfgread(tag, 0x10) & ~3U);
	DPRINTF(("%s: CSR = 0x%x\n", __func__, l->csr));
#else
	l->csr = MIPS_PHYS_TO_KSEG1(COBALT_TLP0_BASE);
#endif

	val = CSR_READ(l, TLP_BMR);
	CSR_WRITE(l, TLP_BMR, val | BMR_RST);
	DELAY(1000);
	CSR_WRITE(l, TLP_BMR, val);
	DELAY(1000);
	(void)CSR_READ(l, TLP_BMR);

	if (is21041) {
		/* reset SIA for 10BASE-T */
		CSR_WRITE(l, TLP_CSR13, 0);
		DELAY(1000);
		CSR_WRITE(l, TLP_CSR15, SIAGEN_10BT);
		CSR_WRITE(l, TLP_CSR14, SIATXRX_10BT);
		CSR_WRITE(l, TLP_CSR13, SIACONN_10BT);
	} else {
		/* reset PHY (cobalt quirk from if_tlp_pci.c) */
		CSR_WRITE(l, TLP_CSR15, SIAGEN_CWE | SIAGEN_MD0);
		DELAY(10);
		CSR_WRITE(l, TLP_CSR15, SIAGEN_CWE);
		DELAY(10);
	}

	l->omr = OMR_PS | OMR_SDP;
	CSR_WRITE(l, TLP_OMR, l->omr);
	CSR_WRITE(l, TLP_IEN, 0);
	CSR_WRITE(l, TLP_STS, ~0);

#if 0
	mii_initphy(l);
#endif
	size_srom(l);

	en = cookie;
	/* MAC address is stored at offset 0 in SROM on cobalt */
	val = read_srom(l, SROM_MAC_OFFSET / 2 + 0);
	en[0] = val;
	en[1] = val >> 8;
	val = read_srom(l, SROM_MAC_OFFSET / 2 + 1);
	en[2] = val;
	en[3] = val >> 8;
	val = read_srom(l, SROM_MAC_OFFSET / 2 + 2);
	en[4] = val;
	en[5] = val >> 8;

	DPRINTF(("tlp: MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    en[0], en[1], en[2], en[3], en[4], en[5]));

	rxd = &l->rxd[0];
	for (i = 0; i < NRXBUF; i++) {
		rxd[i].xd3 = htole32(VTOPHYS(&rxd[NEXT_RXBUF(i)]));
		rxd[i].xd2 = htole32(VTOPHYS(l->rxstore[i]));
		rxd[i].xd1 = htole32(R1_RCH|FRAMESIZE);
		rxd[i].xd0 = htole32(R0_OWN);
	}

	txd = &l->txd[0];
	for (i = 0; i < NTXBUF; i++) {
		txd[i].xd3 = htole32(VTOPHYS(&txd[NEXT_TXBUF(i)]));
		txd[i].xd0 = htole32(0);
	}

	/* prepare setup packet */
	p = l->txstore;
	memset(p, 0, TLP_SETUPLEN);
	/* put broadcast first */
	p[0] = p[1] = p[4] = p[5] = p[8] = p[9] = 0xff;
	for (i = 1; i < TLP_SETUP_NADDR; i++) {
		/* put own station address to the rest */
		p[i * 12 + 0] = en[0];
		p[i * 12 + 1] = en[1];
		p[i * 12 + 4] = en[2];
		p[i * 12 + 5] = en[3];
		p[i * 12 + 8] = en[4];
		p[i * 12 + 9] = en[5];
	}

	txd = &l->txd[0];
	txd->xd2 = htole32(VTOPHYS(l->txstore));
	txd->xd1 = htole32(T1_SET | T1_TCH | TLP_SETUPLEN);
	txd->xd0 = htole32(T0_OWN);

	/* make sure the entire descriptors transfered to memory */
	wbinv(l, sizeof(struct local));

	CSR_WRITE(l, TLP_RRBA, VTOPHYS(rxd));
	CSR_WRITE(l, TLP_TRBA, VTOPHYS(txd));

	l->tx = NEXT_TXBUF(0);
	l->rx = 0;
	l->omr |= OMR_TEN | OMR_REN;
	if (!is21041)
		l->omr |= OMR_FD;

	/* enable Tx/Rx */
	CSR_WRITE(l, TLP_OMR, l->omr);
	/* start TX and send setup packet */
	CSR_WRITE(l, TLP_TPD, TPD_POLL);
	DELAY(50000);
	/* start RX */
	CSR_WRITE(l, TLP_RPD, RPD_POLL);

	return l;
}

int
tlp_send(void *dev, char *buf, u_int len)
{
	struct local *l = dev;
	struct desc *txd;
	u_int loop;

	wb(buf, len);
	txd = &l->txd[l->tx];
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd1 = htole32(T1_FS | T1_LS | T1_TCH | (len & T1_TBS_MASK));
	txd->xd0 = htole32(T0_OWN);
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE(l, TLP_TPD, TPD_POLL);
	l->tx = NEXT_TXBUF(l->tx);
	loop = 100;
	do {
		if ((le32toh(txd->xd0) & T0_OWN) == 0)
			goto done;
		inv(txd, sizeof(struct desc));
		DELAY(10);
	} while (--loop > 0);
	printf("xmit failed\n");
	return -1;
  done:
	return len;
}

int
tlp_recv(void *dev, char *buf, u_int maxlen, u_int timo)
{
	struct local *l = dev;
	struct desc *rxd;
	u_int bound, len;
	uint32_t rxstat;
	uint8_t *ptr;

	bound = timo * 1000000;

  again:
	rxd = &l->rxd[l->rx];
	do {
		rxstat = le32toh(rxd->xd0);
		inv(rxd, sizeof(struct desc));
		if ((rxstat & R0_OWN) == 0)
			goto gotone;
		DELAY(1);
	} while (--bound > 0);
	errno = 0;
	CSR_WRITE(l, TLP_RPD, RPD_POLL);
	return -1;
  gotone:
	if (rxstat & R0_ES) {
		rxd->xd0 = htole32(R0_OWN);
		wbinv(rxd, sizeof(struct desc));
		l->rx = NEXT_RXBUF(l->rx);
		CSR_WRITE(l, TLP_RPD, RPD_POLL);
		goto again;
	}
	/* good frame */
	len = ((rxstat & R0_FL_MASK) >> 16) - 4; /* HASFCS */
        if (len > maxlen)
                len = maxlen;
	ptr = l->rxstore[l->rx];
	memcpy(buf, ptr, len);
	inv(ptr, FRAMESIZE);
	rxd->xd0 = htole32(R0_OWN);
	wbinv(rxd, sizeof(struct desc));
	l->rx = NEXT_RXBUF(l->rx);
	CSR_WRITE(l, TLP_OMR, l->omr); /* necessary? */
	return len;
}

static void
size_srom(struct local *l)
{
	/* determine 8/6 bit addressing SEEPROM */
	l->sromsft = 8;
	l->sromsft = (read_srom(l, 255) & 0x40000) ? 8 : 6;
}

/*
 * bare SEEPROM access with bitbang'ing
 */
#define R110	6		/* SEEPROM read op */
#define CS  	(1U << 0)	/* hold chip select */
#define CLK	(1U << 1)	/* clk bit */
#define D1	(1U << 2)	/* bit existence */
#define D0	0		/* bit absence */
#define VV 	(1U << 3)	/* taken 0/1 from SEEPROM */

static u_int
read_srom(struct local *l, int off)
{
	u_int idx, cnt, ret;
	uint32_t val, x1, x0, bit;

	idx = off & 0xff;		/* A7-A0 */
	idx |= R110 << l->sromsft;	/* 110 for READ */

	val = SROM_RD | SROM_SR;
	CSR_WRITE(l, TLP_APROM, val);
	val |= CS;			/* hold CS */
	CSR_WRITE(l, TLP_APROM, val);

	x1 = val | D1;			/* 1 */
	x0 = val | D0;			/* 0 */
	/* instruct R110 op. at off in MSB first order */
	for (cnt = (1 << (l->sromsft + 2)); cnt > 0; cnt >>= 1) {
		bit = (idx & cnt) ? x1 : x0;
		CSR_WRITE(l, TLP_APROM, bit);
		DELAY(10);
		CSR_WRITE(l, TLP_APROM, bit | CLK);
		DELAY(10);
	}
	/* read 16bit quantity in MSB first order */
	ret = 0;
	for (cnt = 16; cnt > 0; cnt--) {
		CSR_WRITE(l, TLP_APROM, val);
		DELAY(10);
		CSR_WRITE(l, TLP_APROM, val | CLK);
		DELAY(10);
		ret = (ret << 1) | !!(CSR_READ(l, TLP_APROM) & VV);
	}
	val &= ~CS; /* turn off chip select */
	CSR_WRITE(l, TLP_APROM, val);

	return ret;
}

#if 0

static u_int
tlp_mii_read(struct local *l, int phy, int reg)
{
	/* later ... */
	return 0;
}

static void
tlp_mii_write(struct local *l, int phy, int reg, int val)
{
	/* later ... */
}

#define MII_BMCR	0x00 	/* Basic mode control register (rw) */
#define  BMCR_RESET	0x8000	/* reset */
#define  BMCR_AUTOEN	0x1000	/* autonegotiation enable */
#define  BMCR_ISO	0x0400	/* isolate */
#define  BMCR_STARTNEG	0x0200	/* restart autonegotiation */
#define MII_BMSR	0x01	/* Basic mode status register (ro) */

static void
mii_initphy(struct local *l)
{
	int phy, bound;
	uint32_t ctl, sts;

	for (phy = 0; phy < 32; phy++) {
		ctl = tlp_mii_read(l, phy, MII_BMCR);
		sts = tlp_mii_read(l, phy, MII_BMSR);
		if (ctl != 0xffff && sts != 0xffff)
			goto found;
	}
	printf("MII: no PHY found\n");
	return;
  found:
	ctl = tlp_mii_read(l, phy, MII_BMCR);
	tlp_mii_write(l, phy, MII_BMCR, ctl | BMCR_RESET);
	bound = 100;
	do {
		DELAY(10);
		ctl = tlp_mii_read(l, phy, MII_BMCR);
		if (ctl == 0xffff) {
			printf("MII: PHY %d has died after reset\n", phy);
			return;
		}
	} while (bound-- > 0 && (ctl & BMCR_RESET));
	if (bound == 0) {
		printf("PHY %d reset failed\n", phy);
	}
	ctl &= ~BMCR_ISO;
	tlp_mii_write(l, phy, MII_BMCR, ctl);
	sts = tlp_mii_read(l, phy, MII_BMSR) |
	    tlp_mii_read(l, phy, MII_BMSR); /* read twice */
	l->phy = phy;
	l->bmsr = sts;
}

static void
mii_dealan(struct local *, u_int timo)
{
	uint32_t anar;
	u_int bound;

	anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10 | ANAR_CSMA;
	tlp_mii_write(l, l->phy, MII_ANAR, anar);
	tlp_mii_write(l, l->phy, MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);
	l->anlpar = 0;
	bound = getsecs() + timo;
	do {
		l->bmsr = tlp_mii_read(l, l->phy, MII_BMSR) |
		   tlp_mii_read(l, l->phy, MII_BMSR); /* read twice */
		if ((l->bmsr & BMSR_LINK) && (l->bmsr & BMSR_ACOMP)) {
			l->anlpar = tlp_mii_read(l, l->phy, MII_ANLPAR);
			break;
		}
		DELAY(10 * 1000);
	} while (getsecs() < bound);
	return;
}
#endif
