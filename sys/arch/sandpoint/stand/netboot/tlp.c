/* $NetBSD: tlp.c,v 1.14.4.3 2009/07/18 14:52:55 yamt Exp $ */

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
#define CSR_READ(l, r)		in32rb((l)->csr+(r))
#define CSR_WRITE(l, r, v)	out32rb((l)->csr+(r), (v))
#define VTOPHYS(va)		(uint32_t)(va)
#define DEVTOV(pa)		(uint32_t)(pa)
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

#define PAR_CSR0	0x00		/* bus mode */
#define  PAR_DEFAULTS	0x00001000	/* PDF sez it should be ... */
#define  PAR_SWR	01
#define TDR_CSR1	0x08		/* T0_OWN poll demand */
#define RDR_CSR2	0x10		/* R0_OWN poll demand */
#define RDB_CSR3	0x18		/* Rx descriptor base */
#define TDB_CSR4	0x20		/* Tx descriptor base */
#define SR_CSR5		0x28		/* interrupt stauts */
#define NAR_CSR6	0x30		/* operation mode */
#define  NAR_NOSQE	(1U<<19)	/* _not_ use SQE signal */
#define  NAR_TEN	(1U<<13)	/* instruct start/stop Tx */
#define  NAR_REN	(1U<< 1)	/* instruct start/stop Rx */
#define IER_CSR7	0x38		/* interrupt enable mask */
#define SPR_CSR9	0x48		/* SEEPROM and MII management */
#define  MII_MDI	(1U<<19)	/* 0/1 presense after read op */
#define  MII_MIDIR	(1U<<18)	/* 1 for PHY->HOST */
#define  MII_MDO	(1U<<17)	/* 0/1 for write op */
#define  MII_MDC	(1U<<16)	/* MDIO clock */
#define  SROM_RD	(1U<<14)	/* read operation */
#define  SROM_WR	(1U<<13)	/* write openration */
#define  SROM_SR	(1U<<11)	/* SEEPROM select */
#define PAR0_CSR25	0xa4		/* MAC 3:0 */
#define PAR1_CSR26	0xa8		/* MAC 5:4 */
#define AN_OMODE	0xfc		/* operation mode */

#define FRAMESIZE	1536

struct local {
	struct desc txd[2];
	struct desc rxd[2];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, omr, tx, rx;
	unsigned phy, bmsr, anlpar;
};

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
	case PCI_DEVICE(0x1317, 0x0985): /* ADMTek/Infineon 983B/BX */
		return 1;
	}
	return 0;
}

void *
tlp_init(unsigned tag, void *data)
{
	struct local *l;
	struct desc *txd, *rxd;
	unsigned i, val, fdx;
	uint8_t *en;
	
	l = ALLOC(struct local, 2 * sizeof(struct desc)); /* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x14)); /* use mem space */

	CSR_WRITE(l, PAR_CSR0, PAR_SWR);
	i = 100;
	do {
		DELAY(10);
	} while (i-- > 0 && (CSR_READ(l, PAR_CSR0) & PAR_SWR) != 0);
	CSR_WRITE(l, PAR_CSR0, PAR_DEFAULTS);

	l->omr = NAR_NOSQE;
	CSR_WRITE(l, NAR_CSR6, l->omr);
	CSR_WRITE(l, SR_CSR5, ~0);
	CSR_WRITE(l, IER_CSR7, 0);

	en = data;
	val = CSR_READ(l, PAR0_CSR25);
	en[0] = val & 0xff;
	en[1] = (val >> 8) & 0xff;
	en[2] = (val >> 16) & 0xff;
	en[3] = (val >> 24) & 0xff;
	val = CSR_READ(l, PAR1_CSR26);
	en[4] = val & 0xff;
	en[5] = (val >> 8) & 0xff;
#if 1
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		en[0], en[1], en[2], en[3], en[4], en[5]);
#endif

	mii_initphy(l);
	mii_dealan(l, 5);

	val = CSR_READ(l, AN_OMODE);
	if (val & (1U << 29)) {
		printf("%s", (val & (1U << 31)) ? "100Mbps" : "10Mbps");
		fdx = !!(val & (1U << 30));
		if (fdx)
			printf("-FDX");
		printf("\n");
	}

	txd = &l->txd[0];
	txd[1].xd1 = htole32(T1_TER);
	rxd = &l->rxd[0];
	rxd[0].xd0 = htole32(R0_OWN);
	rxd[0].xd1 = htole32(FRAMESIZE);
	rxd[0].xd2 = htole32(VTOPHYS(l->rxstore[0]));
	rxd[1].xd0 = htole32(R0_OWN);
	rxd[1].xd1 = htole32(R1_RER | FRAMESIZE);
	rxd[1].xd2 = htole32(VTOPHYS(l->rxstore[1]));
	l->tx = l->rx = 0;

	/* make sure the entire descriptors transfered to memory */
	wbinv(l, sizeof(struct local));

	CSR_WRITE(l, TDB_CSR4, VTOPHYS(txd));
	CSR_WRITE(l, RDB_CSR3, VTOPHYS(rxd));

	/* start Tx/Rx */
	CSR_WRITE(l, NAR_CSR6, l->omr | NAR_TEN | NAR_REN);

	return l;
}

int
tlp_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	volatile struct desc *txd;
	unsigned txstat, loop;

	wbinv(buf, len);
	txd = &l->txd[l->tx];
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd1 &= htole32(T1_TER);
	txd->xd1 |= htole32(T1_FS | T1_LS | (len & T1_TBS_MASK));
	txd->xd0 = htole32(T0_OWN);
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE(l, TDR_CSR1, 01);
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
	l->tx ^= 1;
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
#if 0
printf("recving with %u sec. timeout\n", timo);
#endif
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
	return len;
}

/*
 * bare MII access with bitbang'ing
 */
#define R110	6		/* SEEPROM/MDIO read op */
#define W101	5		/* SEEPROM/MDIO write op */
#define CS	(1U << 0)	/* hold chip select */
#define CLK	(1U << 1)	/* clk bit */
#define D1	(1U << 2)	/* bit existence */
#define VV	(1U << 3)	/* taken 0/1 from SEEPROM */

static unsigned
mii_read(struct local *l, int phy, int reg)
{
	unsigned data, rv, v, i;

	data = (R110 << 10) | (phy << 5) | reg;
	CSR_WRITE(l, SPR_CSR9, MII_MDO);
	for (i = 0; i < 32; i++) {
		CSR_WRITE(l, SPR_CSR9, MII_MDO | MII_MDC);
		DELAY(1);
		CSR_WRITE(l, SPR_CSR9, MII_MDO);
		DELAY(1);
	}
	CSR_WRITE(l, SPR_CSR9, 0);
	v = 0; /* 4OP + 5ADDR + 5REG */
	for (i = (1 << 13); i != 0; i >>= 1) {
		if (data & i)
			v |= MII_MDO;
		else
			v &= ~MII_MDO;
		CSR_WRITE(l, SPR_CSR9, v);
		DELAY(1);
		CSR_WRITE(l, SPR_CSR9, v | MII_MDC);
		DELAY(1);
		CSR_WRITE(l, SPR_CSR9, v);
		DELAY(1);
	}
	rv = 0; /* 2TA + 16MDI */
	for (i = 0; i < 18; i++) {
		CSR_WRITE(l, SPR_CSR9, MII_MIDIR);
		DELAY(1);
		rv = (rv << 1) | !!(CSR_READ(l, SPR_CSR9) & MII_MDI);
		CSR_WRITE(l, SPR_CSR9, MII_MIDIR | MII_MDC);
		DELAY(1);
	}
	CSR_WRITE(l, SPR_CSR9, 0);
	return rv & 0xffff;
}

static void
mii_write(struct local *l, int phy, int reg, int val)
{
	unsigned data, v, i;

	data = (W101 << 28) | (phy << 23) | (reg << 18) | (02 << 16);
	data |= val & 0xffff;
	CSR_WRITE(l, SPR_CSR9, MII_MDO);
	for (i = 0; i < 32; i++) {
		CSR_WRITE(l, SPR_CSR9, MII_MDO | MII_MDC);
		DELAY(1);
		CSR_WRITE(l, SPR_CSR9, MII_MDO);
		DELAY(1);
	}
	CSR_WRITE(l, SPR_CSR9, 0);
	v = 0; /* 4OP + 5ADDR + 5REG + 2TA + 16DATA */
	for (i = (1 << 31); i != 0; i >>= 1) {
		if (data & i)
			v |= MII_MDO;
		else
			v &= ~MII_MDO;
		CSR_WRITE(l, SPR_CSR9, v);
		DELAY(1);
		CSR_WRITE(l, SPR_CSR9, v | MII_MDC);
		DELAY(1);
		CSR_WRITE(l, SPR_CSR9, v);
		DELAY(1);
	}
	CSR_WRITE(l, SPR_CSR9, 0);
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
