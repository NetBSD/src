/* $NetBSD: wm.c,v 1.1.8.3 2008/01/21 09:39:10 yamt Exp $ */

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
 
#include <netinet/in.h>
#include <netinet/in_systm.h>
 
#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include <dev/pci/if_wmreg.h>

#include "globals.h"

/*
 * - reverse endian access every CSR.
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

void *wm_init(unsigned, void *);
int wm_send(void *, char *, unsigned);
int wm_recv(void *, char *, unsigned, unsigned);

struct rdesc {
	uint32_t lo;	/* 31:0 */
	uint32_t hi;	/* 63:32 */
	uint32_t r2;	/* 31:16 checksum, 15:0 Rx frame length */
	uint32_t r3;	/* 31:16 special, 15:8 errors, 7:0 status */
};
struct tdesc {
	uint32_t lo;	/* 31:0 */
	uint32_t hi;	/* 63:32 */
	uint32_t t2;	/* 31:16 command, 15:0 Tx frame length */
	uint32_t t3;	/* 31:16 VTAG, 15:8 opt, 7:0 Tx status */
};

#define R2_FLMASK	0xffff		/* 15:0 */
/* R3 status */
#define	R3_DD		(1U << 0)	/* 1: Rx frame loaded and available */
#define R3_EOP		(1U << 1)	/* end of packet */
#define R3_IXSM		(1U << 2)	/* ignore checksum indication */
#define R3_VP		(1U << 3)	/* VLAN packet */
#define R3_TCPCS	(1U << 5)	/* TCP csum performed */
#define R3_IPCS		(1U << 6)	/* IP csum performed */
#define R3_PIF		(1U << 7)	/* passed in-exact filter */
/* R3 error status */
#define R3_CE		(1U << 8)	/* CRC error */
#define R3_SE		(1U << 9)	/* symbol error */
#define R3_SEQ		(1U << 10)	/* sequence error */
#define R3_CXE		(1U << 12)	/* carrier extention error */
#define R3_TCPE		(1U << 13)	/* TCP csum error found */
#define R3_IPE		(1U << 14)	/* IP csum error found */
#define R3_RXE		(1U << 15)	/* Rx data error */

/* T2 command */
#define T2_FLMASK	0xffff		/* 15:0 */
#define T2_DTYP_C	(1U << 20)	/* data descriptor */
#define T2_EOP		(1U << 24)	/* end of packet */
#define T2_IFCS		(1U << 25)	/* insert FCS */
#define T2_RS		(1U << 27)	/* report status */
#define T2_RPS		(1U << 28)	/* report packet sent */
#define T2_DEXT		(1U << 29)	/* descriptor extention */
#define T2_VLE		(1U << 30)	/* VLAN enable */
#define T2_IDE		(1U << 31)	/* interrupt delay enable */
/* T3 status */
#define T3_DD		(1U << 0)	/* 1: Tx has done and vacant */
/* T3 option */
#define T3_IXSM		(1U << 16)	/* generate IP csum */
#define T3_TXSM		(1U << 17)	/* generate TCP/UDP csum */

#define FRAMESIZE	1536

struct local {
	struct tdesc txd;
	struct rdesc rxd[2];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, rx;
	unsigned ctl, tctl, rctl;
	unsigned phy, bmsr, anlpar;
	int sromsft;
};

static int read_srom(struct local *, int);
static unsigned mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
static void mii_dealan(struct local *, unsigned);

void *
wm_init(unsigned tag, void *data)
{
	unsigned val, fdx;
	struct local *l;
	struct tdesc *txd;
	struct rdesc *rxd;
	uint8_t *en;

	val = pcicfgread(tag, PCI_ID_REG);
	if (PCI_VENDOR(val) != 0x8086
		    && PCI_PRODUCT(val) != 0x107c)
		return NULL;

	l = ALLOC(struct local, 16);
	memset(l, 0, sizeof(struct local));
	l->csr = pcicfgread(tag, 0x10); /* use mem space */

	CSR_WRITE(l, WMREG_TCTL, 0);
	CSR_WRITE(l, WMREG_RCTL, 0);

	mii_initphy(l);

	l->sromsft = 6;
	en = data;
	val = read_srom(l, 0); en[0] = val; en[1] = (val >> 8);
	val = read_srom(l, 1); en[2] = val; en[3] = (val >> 8);
	val = read_srom(l, 2); en[4] = val; en[5] = (val >> 8);

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    en[0], en[1], en[2], en[3], en[4], en[5]);
	printf("PHY %d (%04x.%04x)\n", l->phy,
	    mii_read(l, l->phy, 2), mii_read(l, l->phy, 3));

	mii_dealan(l, 5);

	/* speed and duplexity are found at 82451 internal GPHY reg 17 */
	val = mii_read(l, l->phy, 0x11);
	fdx = !!(val & 0x0200);
	switch (val & 0xc000) {
	case 0x4000: printf("10Mbps"); break;
	case 0x8000: printf("100Mbps"); break;
	case 0xc000: printf("1000Mbps"); break;
	}
	if (fdx)
		printf("-FDX");
	printf("\n");

	txd = &l->txd;
	rxd = &l->rxd[0];
	rxd[0].lo = htole32(VTOPHYS(l->rxstore[0]));
	rxd[0].r2 = 0;
	rxd[0].r3 = 0;
	rxd[1].lo = htole32(VTOPHYS(l->rxstore[1]));
	rxd[1].r2 = 0;
	rxd[0].r3 = 0;
	l->rx = 0;

	CSR_WRITE(l, WMREG_TBDAH, 0);
	CSR_WRITE(l, WMREG_TBDAL, VTOPHYS(txd));
	CSR_WRITE(l, WMREG_TDLEN, sizeof(l->txd));
	CSR_WRITE(l, WMREG_TDH, 0);
	CSR_WRITE(l, WMREG_TDT, 0);
	CSR_WRITE(l, WMREG_TIDV, 64);
	CSR_WRITE(l, WMREG_TADV, 128);
	CSR_WRITE(l, WMREG_TXDCTL, TXDCTL_PTHRESH(0) |
	    TXDCTL_HTHRESH(0) | TXDCTL_WTHRESH(0));
	CSR_WRITE(l, WMREG_TQSA_LO, 0);
	CSR_WRITE(l, WMREG_TQSA_HI, 0);

	CSR_WRITE(l, WMREG_RDBAH, 0);
	CSR_WRITE(l, WMREG_RDBAL, VTOPHYS(rxd));
	CSR_WRITE(l, WMREG_RDLEN, sizeof(l->rxd));
	CSR_WRITE(l, WMREG_RDH, 0);
	CSR_WRITE(l, WMREG_RDT, 0);
	CSR_WRITE(l, WMREG_RDTR, 0 | RDTR_FPD);
	CSR_WRITE(l, WMREG_RADV, 128);
	CSR_WRITE(l, WMREG_RXDCTL, RXDCTL_PTHRESH(0) |
	    RXDCTL_HTHRESH(0) | RXDCTL_WTHRESH(1));

	CSR_WRITE(l, WMREG_VET, 0);
	CSR_WRITE(l, WMREG_IMC, ~0);
	CSR_WRITE(l, WMREG_IMS, 0);

	l->tctl = TCTL_EN | TCTL_PSP | TCTL_CT(15);
	l->rctl = RCTL_EN | RCTL_LBM_NONE | RCTL_RDMTS_1_2;
	CSR_WRITE(l, WMREG_TCTL, l->tctl);
	CSR_WRITE(l, WMREG_RCTL, l->rctl);

	return l;
}

int
wm_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	struct tdesc *txd;
	unsigned loop;

	wbinv(buf, len);
	txd = &l->txd;
	txd->lo = htole32(VTOPHYS(buf));
	txd->t2 = htole32(T2_EOP|T2_IFCS|T2_RS | (len & T2_FLMASK));
	txd->t3 = 0;
	wbinv(txd, sizeof(struct tdesc));
	CSR_WRITE(l, WMREG_TDT, 0);
	loop = 100;
	do {
		if ((le32toh(txd->t3) & T3_DD) != 0)
			goto done;
		DELAY(10);
		inv(txd, sizeof(struct tdesc));
	} while (--loop > 0);
	printf("xmit failed\n");
	return -1;
  done:
	return len;
}

int
wm_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	struct rdesc *rxd;
	unsigned bound, rxstat, len;
	uint8_t *ptr;

	bound = 1000 * timo;
printf("recving with %u sec. timeout\n", timo);
  again:
	rxd = &l->rxd[l->rx];
	do {
		inv(rxd, sizeof(struct rdesc));
		rxstat = le32toh(rxd->r3);
		if ((rxstat & R3_DD) != 0)
			goto gotone;
		DELAY(1000);	/* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
	/* expect this has R3_EOP mark */
	if (rxstat & (R3_CE|R3_SE|R3_SEQ|R3_CXE|R3_RXE)) {
		rxd->r2 = 0;
		rxd->r3 = 0;
		wbinv(rxd, sizeof(struct rdesc));
		CSR_WRITE(l, WMREG_RDT, l->rx);
		l->rx ^= 1;
		goto again;
	}
	len = (rxstat & R2_FLMASK) - 4 /* HASFCS */; 
	if (len > maxlen)
		len = maxlen;
	ptr = l->rxstore[l->rx];
	inv(ptr, len);
	memcpy(buf, ptr, len);
	rxd->r2 = 0;
	rxd->r3 = 0;
	wbinv(rxd, sizeof(struct rdesc));
	CSR_WRITE(l, WMREG_RDT, l->rx);
	l->rx ^= 1;
	return len;
}

/*
 * bare SEEPROM access with bitbang'ing
 */
#define R110	6		/* SEEPROM read op */
#define CS  	(1U << 0)	/* hold chip select */
#define CLK	(1U << 1)	/* clk bit */
#define D1	(1U << 2)	/* bit existence */
#define VV 	(1U << 3)	/* taken 0/1 from SEEPROM */

static int
read_srom(struct local *l, int off)
{
	unsigned data, v, i;

	data = off & 0xff;		/* A5/A7-A0 */
	data |= R110 << l->sromsft;	/* 110 for READ */

	v = CSR_READ(l, WMREG_EECD) & ~(EECD_SK | EECD_DI);
	CSR_WRITE(l, WMREG_EECD, v);
	v |= EECD_CS;			/* hold CS */
	CSR_WRITE(l, WMREG_EECD, v);
	DELAY(2);

	/* instruct R110 op. at off in MSB first order */
	for (i = (1 << (l->sromsft + 2)); i != 0; i >>= 1) {
		if (data & i)
			v |= EECD_DI;
		else
			v &= ~EECD_DI;
		CSR_WRITE(l, WMREG_EECD, v);
		DELAY(2);
		CSR_WRITE(l, WMREG_EECD, v | EECD_SK);
		DELAY(2);
		CSR_WRITE(l, WMREG_EECD, v);
		DELAY(2);
	}
	v &= ~EECD_DI;

	/* read 16bit quantity in MSB first order */
	data = 0;
	for (i = 0; i < 16; i++) {
		CSR_WRITE(l, WMREG_EECD, v | EECD_SK);
		DELAY(2);
		data = (data << 1) | !!(CSR_READ(l, WMREG_EECD) & EECD_DO);
		CSR_WRITE(l, WMREG_EECD, v);
		DELAY(2);
	}
	/* turn off chip select */
	v = CSR_READ(l, WMREG_EECD) & ~EECD_CS;
	CSR_WRITE(l, WMREG_EECD, v);
	DELAY(2);

	return data;
}

#define MREG(v)		((v)<< 16)
#define MPHY(v)		((v)<< 21)

unsigned
mii_read(struct local *l, int phy, int reg)
{
	unsigned data;

	data = (2U << 26) | MPHY(phy) | MREG(reg);
	CSR_WRITE(l, WMREG_MDIC, data);
	do {
		data = CSR_READ(l, WMREG_MDIC);
	} while ((data & (1U << 28)) == 0);
	return data & 0xffff;
}

void
mii_write(struct local *l, int phy, int reg, int val)
{
	unsigned data;

	data = (1U << 26) | MPHY(phy) | MREG(reg) | (val & 0xffff);
	CSR_WRITE(l, WMREG_MDIC, data);
	do {
		data = CSR_READ(l, WMREG_MDIC);
	} while ((data & (1U << 28)) == 0);
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
#define MII_GTCR	0x09	/* 1000baseT control */
#define  GANA_1000TFDX	0x0200	/* advertise 1000baseT FDX */
#define  GANA_1000THDX	0x0100	/* advertise 1000baseT HDX */
#define MII_GTSR	0x0a	/* 1000baseT status */
#define  GLPA_1000TFDX	0x0800	/* link partner 1000baseT FDX capable */
#define  GLPA_1000THDX	0x0400	/* link partner 1000baseT HDX capable */
#define  GLPA_ASM_DIR	0x0200	/* link partner asym. pause dir. capable */

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

void
mii_dealan(struct local *l, unsigned timo)
{
	unsigned anar, gtcr, bound;

	anar = ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10 | ANAR_CSMA;
	anar |= ANAR_FC;
	gtcr = GANA_1000TFDX | GANA_1000THDX;
	mii_write(l, l->phy, MII_ANAR, anar);
	mii_write(l, l->phy, MII_GTCR, gtcr);
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
