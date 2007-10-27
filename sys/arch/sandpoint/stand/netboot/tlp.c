/* $NetBSD: tlp.c,v 1.7 2007/10/27 06:34:20 nisimura Exp $ */

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "globals.h"

/*
 * - reverse endian access for CSR register.
 * - no vtophys() translation, vaddr_t == paddr_t. 
 * - PIPT writeback cache aware.
 */
#define CSR_WRITE(l, r, v) 	out32rb((l)->csr+(r), (v))
#define CSR_READ(l, r)		in32rb((l)->csr+(r))
#define VTOPHYS(va) 		(uint32_t)(va)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)	(T *)((unsigned)alloc(sizeof(T) + (A)) &~ ((A) - 1))

void *tlp_init(void *);
int tlp_send(void *, char *, unsigned);
int tlp_recv(void *, char *, unsigned, unsigned);

#define T0_OWN		(1U<<31)	/* desc is ready to tx */
#define T0_ES		(1U<<15)	/* Tx error summary */
#define T1_LS		(1U<<30)	/* last segment */
#define T1_FS		(1U<<29)	/* first segment */
#define T1_SET		(1U<<27)	/* "setup packet" */
#define T1_TER		(1U<<25)	/* end of ring mark */
#define T1_TBS_MASK	0x7ff		/* segment size 10:0 */
#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_FS		(1U<<30)	/* first desc of frame */
#define R0_LS		(1U<<8)		/* last desc of frame */
#define R0_ES		(1U<<15)	/* Rx error summary */
#define R1_RER		(1U<<25)	/* end of ring mark */
#define R0_FL_MASK	0x3fff0000	/* frame length 29:16 */
#define R1_RBS_MASK	0x7ff		/* segment size 10:0 */

struct desc {
	uint32_t xd0, xd1, xd2, xd3;
};

#define TLP_BMR		0x000		/* 0: bus mode */
#define  BMR_RST	01
#define TLP_TPD		0x008		/* 1: instruct Tx to start */
#define TLP_RPD		0x010		/* 2: instruct Rx to start */
#define TLP_RRBA	0x018		/* 3: Rx descriptor base */
#define TLP_TRBA	0x020		/* 4: Tx descriptor base */
#define TLP_STS		0x028		/* 5: status */
#define  STS_TS		0x00700000	/* Tx status */
#define  STS_RS		0x000e0000	/* Rx status */
#define TLP_OMR		0x030		/* 6: operation mode */
#define  OMR_SDP	(1U<<25)	/* always ON */
#define  OMR_PS		(1U<<18)	/* port select */
#define  OMR_PM		(1U<< 6)	/* promicuous */
#define  OMR_TEN	(1U<<13)	/* instruct start/stop Tx */
#define  OMR_REN	(1U<< 1)	/* instruct start/stop Rx */
#define  OMR_FD		(1U<< 9)	/* FDX */
#define TLP_IEN		0x38		/* 7: interrupt enable mask */
#define TLP_APROM	0x048		/* 9: SEEPROM and MII management */
#define  SROM_RD	(1U <<14)	/* read operation */
#define  SROM_WR	(1U <<13)	/* write openration */
#define  SROM_SR	(1U <<11)	/* SEEPROM select */
#define TLP_CSR12	0x60		/* SIA status */

#define FRAMESIZE	1536

struct local {
	struct desc TxD;
	struct desc RxD[2];
	uint8_t txstore[FRAMESIZE];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, omr, rx;
	unsigned sromsft;
	unsigned phy, bmsr, anlpar;
};

static void size_srom(struct local *);
static int read_srom(struct local *, int);
#if 0
static unsigned tlp_mii_read(struct local *, int, int);
static void tlp_mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
#endif

void *
tlp_init(void *cookie)
{
	unsigned tag, val;
	struct local *l;
	struct desc *TxD, *RxD;
	uint8_t *en;
	unsigned *p;

	if (pcifinddev(0x1011, 0x0009, &tag) != 0) {
		/* genuine DE500 */
		printf("tlp NIC not found\n");
		return NULL;
	}
	
	l = ALLOC(struct local, sizeof(struct desc));
	memset(l, 0, sizeof(struct local));
	l->csr = pcicfgread(tag, 0x14); /* use mem space */

	val = CSR_READ(l, TLP_BMR);
	CSR_WRITE(l, TLP_BMR, val | BMR_RST);
	DELAY(1000);
	CSR_WRITE(l, TLP_BMR, val);
	DELAY(1000);
	(void)CSR_READ(l, TLP_BMR);

	l->omr = OMR_PS | OMR_SDP;
	CSR_WRITE(l, TLP_OMR, l->omr);
	CSR_WRITE(l, TLP_STS, ~0);
	CSR_WRITE(l, TLP_IEN, 0);

	size_srom(l);
	en = cookie;
	val = read_srom(l, 20/2+0); en[0] = val; en[1] = val >> 8;
	val = read_srom(l, 20/2+1); en[2] = val; en[3] = val >> 8;
	val = read_srom(l, 20/2+2); en[4] = val; en[5] = val >> 8;
#if 1
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		en[0], en[1], en[2], en[3], en[4], en[5]);
#endif

	TxD = &l->TxD;
	RxD = &l->RxD[0];
	RxD[0].xd0 = htole32(R0_OWN);
	RxD[0].xd1 = htole32(FRAMESIZE);
	RxD[0].xd2 = htole32(VTOPHYS(l->rxstore[0]));
	RxD[0].xd3 = htole32(VTOPHYS(&RxD[1]));
	RxD[1].xd0 = htole32(R0_OWN);
	RxD[1].xd1 = htole32(R1_RER | FRAMESIZE);
	RxD[1].xd2 = htole32(VTOPHYS(l->rxstore[1]));
	RxD[1].xd3 = htole32(VTOPHYS(&RxD[0]));
	CSR_WRITE(l, TLP_TRBA, VTOPHYS(TxD));
	CSR_WRITE(l, TLP_RRBA, VTOPHYS(RxD));

	/* "setup packet" to have own station address */
	TxD = &l->TxD;
	TxD->xd3 = htole32(VTOPHYS(TxD));
	TxD->xd2 = htole32(VTOPHYS(l->txstore));
	TxD->xd1 = htole32(T1_SET | T1_TER);
	TxD->xd0 = htole32(T0_OWN);
	p = (unsigned *)l->txstore;
	memset(p, 0, FRAMESIZE);
	p[0] = en[1] << 8 | en[0];
	p[1] = en[3] << 8 | en[2];
	p[2] = en[5] << 8 | en[4];

	/* make sure the entire descriptors transfered to memory */
	wbinv(l, sizeof(struct local));

	l->rx = 0;
	l->omr |= OMR_FD | OMR_TEN | OMR_REN;

	/* start Tx/Rx */
	CSR_WRITE(l, TLP_OMR, l->omr);
	CSR_WRITE(l, TLP_TPD, 01);
	CSR_WRITE(l, TLP_RPD, 01);

	return l;
}

int
tlp_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	struct desc *TxD;
	unsigned loop;

	wbinv(buf, len);
	TxD = &l->TxD;
	TxD->xd3 = htole32(VTOPHYS(TxD));
	TxD->xd2 = htole32(VTOPHYS(buf));
	TxD->xd1 = htole32(T1_FS | T1_LS | T1_TER | (len & T1_TBS_MASK));
	TxD->xd0 = htole32(T0_OWN);
	wbinv(TxD, sizeof(struct desc));
	CSR_WRITE(l, TLP_TPD, 01);
	loop = 100;
	do {
		if ((le32toh(TxD->xd0) & T0_OWN) == 0)
			goto done;
		DELAY(10);
		inv(TxD, sizeof(struct desc));
	} while (--loop > 0);
	printf("xmit failed\n");
	return -1;
  done:
	return len;
}

int
tlp_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	struct desc *RxD;
	unsigned bound, rxstat, len;
	uint8_t *ptr;

	bound = 1000 * timo;
printf("recving with %u sec. timeout\n", timo);
  again:
	RxD = &l->RxD[l->rx];
	do {
		inv(RxD, sizeof(struct desc));
		rxstat = le32toh(RxD->xd0);
		if ((rxstat & R0_OWN) == 0)
			goto gotone;
		DELAY(1000); /* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
	if (rxstat & R0_ES) {
		RxD->xd0 = htole32(R0_OWN);
		wbinv(RxD, sizeof(struct desc));
		l->rx ^= 1;
		CSR_WRITE(l, TLP_RPD, 01);
		goto again;
	}
	/* good frame */
	len = ((rxstat & R0_FL_MASK) >> 16) - 4; /* HASFCS */
        if (len > maxlen)
                len = maxlen;
	ptr = l->rxstore[l->rx];
        inv(ptr, len);
        memcpy(buf, ptr, len);
	RxD->xd0 = htole32(R0_OWN);
	wbinv(RxD, sizeof(struct desc));
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
#define R110	6		/* SEEPROM read op */
#define CS  	(1U << 0)	/* hold chip select */
#define CLK	(1U << 1)	/* clk bit */
#define D1	(1U << 2)	/* bit existence */
#define D0	0		/* bit absence */
#define VV 	(1U << 3)	/* taken 0/1 from SEEPROM */

static int
read_srom(struct local *l, int off)
{
	unsigned idx, val, x1, x0, cnt, bit, ret;

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

static unsigned
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
	int phy, ctl, sts, bound;

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
mii_dealan(struct local *, unsigned timo)
{
	unsigned anar, bound;

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
