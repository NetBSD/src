/* $NetBSD: tlp.c,v 1.19.6.1 2009/01/19 13:16:39 skrll Exp $ */

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

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include "globals.h"

/*
 * - reverse endian access for CSR register.
 * - no vtophys() translation, vaddr_t == paddr_t. 
 * - PIPT writeback cache aware.
 */
#define CSR_WRITE(l, r, v) 	out32rb((l)->csr+(r), (v))
#define CSR_READ(l, r)		in32rb((l)->csr+(r))
#define VTOPHYS(va) 		(uint32_t)(va)
#define DEVTOV(pa) 		(uint32_t)(pa)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)	(T *)((unsigned)alloc(sizeof(T) + (A)) &~ ((A) - 1))

struct desc {
	uint32_t xd0, xd1, xd2, xd3;
};
#define T0_OWN		(1U<<31)	/* desc is ready to tx */
#define T0_ES		(1U<<15)	/* Tx error summary */
#define T1_LS		(1U<<30)	/* last segment */
#define T1_FS		(1U<<29)	/* first segment */
#define T1_SET		(1U<<27)	/* "setup packet" */
#define T1_TER		(1U<<25)	/* end of ring mark */
#define T1_TCH		(1U<<24)	/* TDES3 points the next desc */
#define T1_TBS_MASK	0x7ff		/* segment size 10:0 */
#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_FS		(1U<<30)	/* first desc of frame */
#define R0_LS		(1U<<8)		/* last desc of frame */
#define R0_ES		(1U<<15)	/* Rx error summary */
#define R1_RER		(1U<<25)	/* end of ring mark */
#define R1_RCH		(1U<<24)	/* RDES3 points the next desc */
#define R0_FLMASK	0x3fff0000	/* frame length 29:16 */
#define R1_RBS_MASK	0x7ff		/* segment size 10:0 */

#define TLP_BMR		0x00		/* 0: bus mode */
#define  BMR_RST	01
#define  BMR_CAL8	0x00004000	/* 32B cache alignment */
#define  BMR_CAL16	0x00008000	/* 64B */
#define  BMR_CAL32	0x0000c000	/* 128B */
#define  BMR_CAL	0x0000c000
#define TLP_TPD		0x08		/* 1: instruct Tx to start */
#define TLP_RPD		0x10		/* 2: instruct Rx to start */
#define TLP_RRBA	0x18		/* 3: Rx descriptor base */
#define TLP_TRBA	0x20		/* 4: Tx descriptor base */
#define TLP_STS		0x28		/* 5: status */
#define  STS_TS		0x00700000	/* Tx status */
#define  STS_RS		0x000e0000	/* Rx status */
#define TLP_OMR		0x30		/* 6: operation mode */
#define  OMR_SDP	(1U<<25)	/* always ON */
#define  OMR_PS		(1U<<18)	/* port select */
#define  OMR_PM		(1U<< 6)	/* promiscuous */
#define  OMR_TEN	(1U<<13)	/* instruct start/stop Tx */
#define  OMR_REN	(1U<< 1)	/* instruct start/stop Rx */
#define  OMR_FD		(1U<< 9)	/* FDX */
#define TLP_IEN		0x38		/* 7: interrupt enable mask */
#define TLP_APROM	0x48		/* 9: SEEPROM and MII management */
#define  SROM_RD	(1U <<14)	/* read operation */
#define  SROM_WR	(1U <<13)	/* write openration */
#define  SROM_SR	(1U <<11)	/* SEEPROM select */
#define TLP_CSR12	0x60		/* 12: SIA status */

#define FRAMESIZE	1536

struct local {
	struct desc txd;
	struct desc rxd[2];
	uint8_t txstore[192];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, omr, rx;
	unsigned sromsft;
	unsigned phy, bmsr, anlpar;
};

static void size_srom(struct local *);
static int read_srom(struct local *, int);
static unsigned mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
static void mii_dealan(struct local *, unsigned);

int
tlp_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x1011, 0x0009):
		return 1;
	}
	return 0;
}

void *
tlp_init(unsigned tag, void *data)
{
	unsigned val, i;
	struct local *l;
	struct desc *txd, *rxd;
	uint8_t *en;
	uint32_t *p;
	
	l = ALLOC(struct local, sizeof(struct desc)); /* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x14)); /* use mem space */

	val = CSR_READ(l, TLP_BMR);
	CSR_WRITE(l, TLP_BMR, val | BMR_RST);
	DELAY(1000);
	val &= ~BMR_CAL;
	switch (pcicfgread(tag, 0x0c) & 0xff) {
	case 32:
		val |= BMR_CAL32; break;
	case 16:
		val |= BMR_CAL16; break;
	case 8:
	default:
		val |= BMR_CAL8; break;
	}
	CSR_WRITE(l, TLP_BMR, val);
	DELAY(1000);
	(void)CSR_READ(l, TLP_BMR);

	l->omr = OMR_PS | OMR_SDP;
	CSR_WRITE(l, TLP_OMR, l->omr);
	CSR_WRITE(l, TLP_STS, ~0);
	CSR_WRITE(l, TLP_IEN, 0);

	size_srom(l);
	en = data;
	val = read_srom(l, 20/2+0); en[0] = val; en[1] = val >> 8;
	val = read_srom(l, 20/2+1); en[2] = val; en[3] = val >> 8;
	val = read_srom(l, 20/2+2); en[4] = val; en[5] = val >> 8;
#if 1
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		en[0], en[1], en[2], en[3], en[4], en[5]);
#endif

	mii_initphy(l);
	mii_dealan(l, 5);

	txd = &l->txd;
	rxd = &l->rxd[0];
	rxd[0].xd0 = htole32(R0_OWN);
	rxd[0].xd1 = htole32(R1_RCH | FRAMESIZE);
	rxd[0].xd2 = htole32(VTOPHYS(l->rxstore[0]));
	rxd[0].xd3 = htole32(VTOPHYS(&rxd[1]));
	rxd[1].xd0 = htole32(R0_OWN);
	rxd[1].xd1 = htole32(R1_RER | FRAMESIZE);
	rxd[1].xd2 = htole32(VTOPHYS(l->rxstore[1]));
	/* R1_RER neglects xd3 */
	l->rx = 0;

	/* "setup frame" to have own station address */
	txd = &l->txd;
	txd->xd3 = htole32(VTOPHYS(txd));
	txd->xd2 = htole32(VTOPHYS(l->txstore));
	txd->xd1 = htole32(T1_SET | sizeof(l->txstore));
	txd->xd0 = htole32(T0_OWN);
	p = (uint32_t *)l->txstore;
	p[0] = htole32(en[1] << 8 | en[0]);
	p[1] = htole32(en[3] << 8 | en[2]);
	p[2] = htole32(en[5] << 8 | en[4]);
	for (i = 1; i < 16; i++)
		memcpy(&p[3 * i], &p[0], 3 * sizeof(p[0]));

	/* make sure the entire descriptors transfered to memory */
	wbinv(l, sizeof(struct local));

	CSR_WRITE(l, TLP_TRBA, VTOPHYS(txd));
	CSR_WRITE(l, TLP_RRBA, VTOPHYS(rxd));

	/* start Tx/Rx */
	l->omr |= OMR_FD | OMR_TEN | OMR_REN;
	CSR_WRITE(l, TLP_OMR, l->omr);
	CSR_WRITE(l, TLP_TPD, 01);
	/* could wait for "setup frame" completion */
	CSR_WRITE(l, TLP_RPD, 01);

	return l;
}

int
tlp_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	volatile struct desc *txd;
	unsigned txstat, loop;

	/* send a single frame with no T1_TER|T1_TCH designation */
	wbinv(buf, len);
	txd = &l->txd;
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd1 = htole32(T1_FS | T1_LS | (len & T1_TBS_MASK));
	txd->xd0 = htole32(T0_OWN);
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE(l, TLP_TPD, 01);
	loop = 100;
	do {
		txstat = le32toh(txd->xd0);
		if ((txstat & T0_OWN) == 0)
			goto done;
		DELAY(10);
		inv(txd, sizeof(struct desc));
	} while (--loop != 0);
	printf("xmit failed\n");
	return -1;
  done:
	return len;
}

int
tlp_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	volatile struct desc *rxd;
	unsigned bound, rxstat, len;
	uint8_t *ptr;

	bound = 1000 * timo;
printf("recving with %u sec. timeout\n", timo);
  again:
	rxd = &l->rxd[l->rx];
	do {
		inv(rxd, sizeof(struct desc));
		rxstat = le32toh(rxd->xd0);
		if ((rxstat & R0_OWN) == 0)
			goto gotone;
		DELAY(1000); /* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
	if (rxstat & R0_ES) {
		rxd->xd0 = htole32(R0_OWN);
		wbinv(rxd, sizeof(struct desc));
		l->rx ^= 1;
		CSR_WRITE(l, TLP_RPD, 01);
		goto again;
	}
	/* good frame */
	len = ((rxstat & R0_FLMASK) >> 16) - 4 /* HASFCS */; 
        if (len > maxlen)
                len = maxlen;
	ptr = l->rxstore[l->rx];
        inv(ptr, len);
        memcpy(buf, ptr, len);
	rxd->xd0 = htole32(R0_OWN);
	wbinv(rxd, sizeof(struct desc));
	l->rx ^= 1;
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
#define R110	6		/* SEEPROM/MDIO read op */
#define W101	5		/* SEEPROM/MDIO write op */
#define CS  	(1U << 0)	/* hold chip select */
#define CLK	(1U << 1)	/* clk bit */
#define D1	(1U << 2)	/* bit existence */
#define VV 	(1U << 3)	/* taken 0/1 from SEEPROM */

static int
read_srom(struct local *l, int off)
{
	unsigned data, v, i;

	data = off & 0xff;		/* A7-A0 */
	data |= R110 << l->sromsft;	/* 110 for READ */

	v = SROM_RD | SROM_SR;
	CSR_WRITE(l, TLP_APROM, v);
	v |= CS;			/* hold CS */
	CSR_WRITE(l, TLP_APROM, v);

	/* instruct R110 op. at off in MSB first order */
	for (i = (1 << (l->sromsft + 2)); i != 0; i >>= 1) {
		if (data & i)
			v |= D1;
		else
			v &= ~D1;
		CSR_WRITE(l, TLP_APROM, v);
		DELAY(10);
		CSR_WRITE(l, TLP_APROM, v | CLK);
		DELAY(10);
	}
	v &= ~D1;

	/* read 16bit quantity in MSB first order */
	data = 0;
	for (i = 0; i < 16; i++) {
		CSR_WRITE(l, TLP_APROM, v);
		DELAY(10);
		CSR_WRITE(l, TLP_APROM, v | CLK);
		DELAY(10);
		data = (data << 1) | !!(CSR_READ(l, TLP_APROM) & VV);
	}
	/* turn off chip select */
	CSR_WRITE(l, TLP_APROM, 0);

	return data;
}

/*
 * bare MII access with bitbang'ing
 */
#define MDI	(1U << 19)	/* taken 0/1 from MDIO */
#define MII	(1U << 18)	/* read operation */
#define MDO	(1U << 17)	/* bit existence */
#define MDC	(1U << 16)	/* clock bit */

static unsigned
mii_read(struct local *l, int phy, int reg)
{
	unsigned data, v, i;

	data = (R110 << 10) | (phy << 5) | reg;
	CSR_WRITE(l, TLP_APROM, MDO);
	for (i = 0; i < 32; i++) {
		CSR_WRITE(l, TLP_APROM, MDO | MDC);
		DELAY(1);
		CSR_WRITE(l, TLP_APROM, MDO);
		DELAY(1);
	}
	CSR_WRITE(l, TLP_APROM, 0);
	v = 0; /* 4OP + 5ADDR + 5REG */
	for (i = (1 << 13); i != 0; i >>= 1) {
		if (data & i)
			v |= MDO;
		else
			v &= ~MDO;
		CSR_WRITE(l, TLP_APROM, v);
		DELAY(1);
		CSR_WRITE(l, TLP_APROM, v | MDC);
		DELAY(1);
		CSR_WRITE(l, TLP_APROM, v);
		DELAY(1);
	}
	data = 0; /* 2TA + 16MDI */
	for (i = 0; i < 18; i++) {
		CSR_WRITE(l, TLP_APROM, MII);
		DELAY(1);
		data = (data << 1) | !!(CSR_READ(l, TLP_APROM) & MDI);
		CSR_WRITE(l, TLP_APROM, MII | MDC);
		DELAY(1);
	}
	CSR_WRITE(l, TLP_APROM, 0);
	return data & 0xffff;
}

static void
mii_write(struct local *l, int phy, int reg, int val)
{
	unsigned data, v, i;

	data = (W101 << 28) | (phy << 23) | (reg << 18) | (02 << 16);
	data |= val & 0xffff;
	CSR_WRITE(l, TLP_APROM, MDO);
	for (i = 0; i < 32; i++) {
		CSR_WRITE(l, TLP_APROM, MDO | MDC);
		DELAY(1);
		CSR_WRITE(l, TLP_APROM, MDO);
		DELAY(1);
	}
	CSR_WRITE(l, TLP_APROM, 0);
	v = 0; /* 4OP + 5ADDR + 5REG + 2TA + 16DATA */
	for (i = (1 << 31); i != 0; i >>= 1) {
		if (data & i)
			v |= MDO;
		else
			v &= ~MDO;
		CSR_WRITE(l, TLP_APROM, v);
		DELAY(1);
		CSR_WRITE(l, TLP_APROM, v | MDC);
		DELAY(1);
		CSR_WRITE(l, TLP_APROM, v);
		DELAY(1);
	}
	CSR_WRITE(l, TLP_APROM, 0);
}

#define MII_BMCR	0x00	/* Basic mode control register (rw) */
#define  BMCR_RESET	0x8000	/* reset */
#define  BMCR_AUTOEN	0x1000	/* autonegotiation enable */
#define  BMCR_ISO	0x0400	/* isolate */
#define  BMCR_STARTNEG	0x0200	/* restart autonegotiation */
#define MII_BMSR	0x01	/* Basic mode status register (ro) */
#define  BMSR_ACOMP	0x0020	/* Autonegotiation complete */
#define  BMSR_LINK	0x0004	/* Link status */
#define MII_ANAR	0x04	/* Autonegotiation advertisement (rw) */
#define  ANAR_FC	0x0400	/* local device supports PAUSE */
#define  ANAR_TX_FD	0x0100	/* local device supports 100bTx FD */
#define  ANAR_TX	0x0080	/* local device supports 100bTx */
#define  ANAR_10_FD	0x0040	/* local device supports 10bT FD */
#define  ANAR_10	0x0020	/* local device supports 10bT */
#define  ANAR_CSMA	0x0001	/* protocol selector CSMA/CD */
#define MII_ANLPAR	0x05	/* Autonegotiation lnk partner abilities (rw) */

static void
mii_initphy(struct local *l)
{
	int phy, ctl, sts, bound;

	for (phy = 0; phy < 32; phy++) {
		ctl = mii_read(l, phy, MII_BMCR);
		sts = mii_read(l, phy, MII_BMSR);
		if (ctl != 0xffff && sts != 0xffff)
			goto found;
	}
	printf("MII: no PHY found\n");
	return;
  found:
	ctl = mii_read(l, phy, MII_BMCR);
	mii_write(l, phy, MII_BMCR, ctl | BMCR_RESET);
	bound = 100;
	do {
		DELAY(10);
		ctl = mii_read(l, phy, MII_BMCR);
		if (ctl == 0xffff) {
			printf("MII: PHY %d has died after reset\n", phy);
			return;
		}
	} while (bound-- > 0 && (ctl & BMCR_RESET));
	if (bound == 0) {
		printf("PHY %d reset failed\n", phy);
	}
	ctl &= ~BMCR_ISO;
	mii_write(l, phy, MII_BMCR, ctl);
	sts = mii_read(l, phy, MII_BMSR) |
	    mii_read(l, phy, MII_BMSR); /* read twice */
	l->phy = phy;
	l->bmsr = sts;
}

static void
mii_dealan(struct local *l, unsigned timo)
{
	unsigned anar, bound;

	anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10 | ANAR_CSMA;
	mii_write(l, l->phy, MII_ANAR, anar);
	mii_write(l, l->phy, MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);
	l->anlpar = 0;
	bound = getsecs() + timo;
	do {
		l->bmsr = mii_read(l, l->phy, MII_BMSR) |
		   mii_read(l, l->phy, MII_BMSR); /* read twice */
		if ((l->bmsr & BMSR_LINK) && (l->bmsr & BMSR_ACOMP)) {
			l->anlpar = mii_read(l, l->phy, MII_ANLPAR);
			break;
		}
		DELAY(10 * 1000);
	} while (getsecs() < bound);
	return;
}
