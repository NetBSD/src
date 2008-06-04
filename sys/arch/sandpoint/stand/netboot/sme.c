/* $NetBSD: sme.c,v 1.1.4.2 2008/06/04 02:04:52 yamt Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
 * - reverse endian access every CSR.
 * - no VTOPHYS() translation, vaddr_t == paddr_t. 
 * - PIPT writeback cache aware.
 */
#define CSR_READ(l, r)		in32rb((l)->csr+(r))
#define CSR_WRITE(l, r, v) 	out32rb((l)->csr+(r), (v))
#define VTOPHYS(va) 		(uint32_t)(va)
#define DEVTOV(pa) 		(uint32_t)(pa)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)	(T *)((unsigned)alloc(sizeof(T) + (A)) &~ ((A) - 1))

int sme_match(unsigned, void *);
void *sme_init(unsigned, void *);
int sme_send(void *, char *, unsigned);
int sme_recv(void *, char *, unsigned, unsigned);

struct desc {
	uint32_t xd0, xd1, xd2, xd3;
};
#define T0_OWN		(1U<<31)	/* */
#define T0_ES		(1U<<15)	/* error summary */
#define	T0_FL		0x7fff0000	/* frame length */
#define T1_LS		(1U<<30)	/* last descriptor of Tx frame */
#define T1_FS		(1U<<29)	/* first descriptor of Tx frame */
#define T1_TER		(1U<<25)	/* wrap mark to form a ring */
#define T1_TCH		(1U<<24)	/* TDES3 points the next desc */
#define T1_FL		0x00007ff	/* Tx frame/segment length */
#define R0_OWN		(1U<<31)	/* */
#define R0_FL		0x3fff0000	/* frame length */
#define R0_ES		(1U<<15)	/* error summary */
#define R1_RER		(1U<<25)	/* wrap mark to form a ring */
#define	R1_RCH		(1U<<24)	/* RDES3 points the next desc */
/* RDES1 will be never changed while operation */

#define	BUSMODE		0x00
#define	TXPOLLD		0x04		/* start transmission */
#define RXPOLLD		0x08		/* start receiving */
#define RXDBASE		0x0c		/* Rx descriptor list base */
#define	TXDBASE		0x10		/* Tx descriptor list base */
#define DMACCTL		0x18		/* DMAC control */
#define  DMACCTL_ST	(1U<<13)	/* start/stop Tx DMA */
#define  DMACCTL_SR	(1U<< 1)	/* start/stop Rx DMA */
#define MAC_CR		0x80		/* MAC control */
#define  MACCR_FDPX     (1U<<20)	/* full duplex operation */
#define  MACCR_TXEN     (1U<< 3)	/* enable xmit */
#define  MACCR_RXEN     (1U<< 2)	/* enable recv */
#define ADDRH		0x84		/* ea 5:4 */
#define ADDRL		0x88		/* ea 3:0 */
#define MIIADDR		0x94		/* MII control */
#define MIIDATA		0x98		/* MII data */

#define FRAMESIZE	1536

struct local {
	struct desc txd;
	struct desc rxd[2];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, rx;
	unsigned phy, bmsr, anlpar;
};

static int mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_dealan(struct local *, unsigned);

int
sme_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x1055, 0xe940):
		return 1;
	}
	return 0;
}

void *
sme_init(unsigned tag, void *data)
{
	struct local *l;
	struct desc *txd, *rxd;
	unsigned mac32, mac16, val, fdx;
	uint8_t *en;

	l = ALLOC(struct local, sizeof(struct desc)); /* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x1c)); /* BAR3 mem space, LE */
	l->phy = 1; /* 9420 internal PHY */

	en = data;
	mac32 = CSR_READ(l, ADDRL);
	mac16 = CSR_READ(l, ADDRH);
	en[0] = mac32;
	en[1] = mac32 >> 8;
	en[2] = mac32 >> 16;
	en[3] = mac32 >> 24;
	en[4] = mac16;
	en[5] = mac16 >> 8;
#if 1
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		en[0], en[1], en[2], en[3], en[4], en[5]);
	printf("PHY %d (%04x.%04x)\n", l->phy,
	    mii_read(l, l->phy, 2), mii_read(l, l->phy, 3));
#endif

	mii_dealan(l, 5);

	/* speed and duplexity can be seen in MII 31 */
	val = mii_read(l, l->phy, 31);
	fdx = !!(val & (1U << 4));
	printf("%s", (val & (1U << 3)) ? "100Mbps" : "10Mbps");
	if (fdx)
		printf("-FDX");
	printf("\n");

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

	wbinv(l, sizeof(struct local));

	CSR_WRITE(l, TXDBASE, VTOPHYS(txd));
	CSR_WRITE(l, RXDBASE, VTOPHYS(rxd));
	val = MACCR_TXEN | MACCR_RXEN;
	if (fdx)
		val |= MACCR_FDPX;
	CSR_WRITE(l, BUSMODE, 0);
	CSR_WRITE(l, DMACCTL, DMACCTL_ST | DMACCTL_SR);
	CSR_WRITE(l, MAC_CR, val); /* (FDX), Tx/Rx enable */
	CSR_WRITE(l, RXPOLLD, 01); /* start receiving */

	return l;
}

int
sme_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	volatile struct desc *txd;
	unsigned txstat, loop;

	/* send a single frame with no T1_TER|T1_TCH designation */
	wbinv(buf, len);
	txd = &l->txd;
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd1 = htole32(T1_FS | T1_LS | (len & T1_FL));
	txd->xd0 = htole32(T0_OWN | (len & T0_FL) << 16);
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE(l, TXPOLLD, 01); /* start transmission */
	loop = 100;
	do {
		txstat = le32toh(txd->xd0);
		if (txstat & T0_ES)
			break;
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
sme_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
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
		CSR_WRITE(l, RXPOLLD, 01); /* restart receiving */
		goto again;
	}
	/* good frame */
	len = (rxstat & R0_FL) >> 16 /* no FCS included */;
        if (len > maxlen)
                len = maxlen;
	ptr = l->rxstore[l->rx];
        inv(ptr, len);
        memcpy(buf, ptr, len);
	rxd->xd0 = htole32(R0_OWN);
	wbinv(rxd, sizeof(struct desc));
	l->rx ^= 1;
	CSR_WRITE(l, RXPOLLD, 01); /* necessary? */
	return len;
}

#define MII_BMCR	0x00 	/* Basic mode control register (rw) */
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

static int
mii_read(struct local *l, int phy, int reg)
{
	uint32_t ctl;

	do {
		ctl = CSR_READ(l, MIIADDR);
	} while (ctl & 01);
	ctl = (phy << 11) | (reg << 6) | (0 << 1); /* READ op */
	CSR_WRITE(l, MIIADDR, ctl);
	do {
		ctl = CSR_READ(l, MIIADDR);
	} while (ctl & 01);
	return CSR_READ(l, MIIDATA);
}

void
mii_write(struct local *l, int phy, int reg, int val)
{
	uint32_t ctl;

	do {
		ctl = CSR_READ(l, MIIADDR);
	} while (ctl & 01);
	ctl = (phy << 11) | (reg << 6) | (1 << 1); /* WRITE op */
	CSR_WRITE(l, MIIDATA, val);
}

void
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
