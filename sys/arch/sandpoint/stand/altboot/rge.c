/* $NetBSD: rge.c,v 1.6.12.1 2013/02/25 00:28:55 tls Exp $ */

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
 * - reverse endian access every CSR.
 * - no vtophys() translation, vaddr_t == paddr_t. 
 * - PIPT writeback cache aware.
 */
#define CSR_WRITE_1(l, r, v)	out8((l)->csr+(r), (v))
#define CSR_READ_1(l, r)	in8((l)->csr+(r))
#define CSR_WRITE_2(l, r, v)	out16rb((l)->csr+(r), (v))
#define CSR_READ_2(l, r)	in16rb((l)->csr+(r))
#define CSR_WRITE_4(l, r, v)	out32rb((l)->csr+(r), (v))
#define CSR_READ_4(l, r)	in32rb((l)->csr+(r))
#define VTOPHYS(va)		(uint32_t)(va)
#define DEVTOV(pa)		(uint32_t)(pa)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)		(T *)allocaligned(sizeof(T),(A))

struct desc {
	uint32_t xd0, xd1, xd2, xd3;
};
#define T0_OWN		0x80000000	/* loaded for HW to send */
#define T0_EOR		0x40000000	/* end of ring */
#define T0_FS		0x20000000	/* first descriptor */
#define T0_LS		0x10000000	/* last descriptor */
#define T0_FRMASK	0x0000ffff

#define R0_OWN		0x80000000	/* empty for HW to load anew */
#define R0_EOR		0x40000000	/* end mark to form a ring */
#define R0_BUFLEN	0x00003ff8	/* max frag. size to receive */
#define R0_FS		0x20000000	/* start of frame */
#define R0_LS		0x10000000	/* end of frame */
#define R0_RES		0x00200000	/* Rx error summary */
#define R0_RUNT		0x00100000	/* runt frame received */
#define R0_CRC		0x00080000	/* CRC error found */
#define R0_FRMASK	0x00003fff	/* 13:0 frame length */

#define RGE_IDR0	0x00		/* MAC address [0] */
#define RGE_IDR1	0x01		/* MAC address [1] */
#define RGE_IDR2	0x02		/* MAC address [2] */
#define RGE_IDR3	0x03		/* MAC address [3] */
#define RGE_IDR4	0x04		/* MAC address [4] */
#define RGE_IDR5	0x05		/* MAC address [5] */
#define RGE_TNPDS	0x20		/* Tx descriptor base paddr */
#define RGE_THPDS	0x28		/* high pro. Tx des. base paddr */
#define RGE_CR		0x37		/* command */
#define	 CR_RESET	(1U << 4)	/* reset S1C */
#define	 CR_RXEN	(1U << 3)	/* Rx enable */
#define	 CR_TXEN	(1U << 2)	/* Tx enable */
#define RGE_TPPOLL	0x38		/* activate desc polling */
#define RGE_IMR		0x3c		/* interrupt mask */
#define RGE_ISR		0x3e		/* interrupt status */
#define RGE_TCR		0x40		/* Tx control */
#define	 TCR_MAXDMA	0x0700		/* 10:8 Tx DMA burst size */
#define RGE_RCR		0x44		/* Rx control */
#define	 RCR_RXTFH	0xe000		/* 15:13 Rx FIFO threshold */
#define	 RCR_MAXDMA	0x0700		/* 10:8 Rx DMA burst size */
#define	 RCR_AE		(1U << 5)	/* accept error frame */
#define	 RCR_RE		(1U << 4)	/* accept runt frame */
#define	 RCR_AB		(1U << 3)	/* accept broadcast frame */
#define	 RCR_AM		(1U << 2)	/* accept multicast frame */
#define	 RCR_APM	(1U << 1)	/* accept unicast frame */
#define	 RCR_AAP	(1U << 0)	/* promiscuous */
#define RGE_EECMD	0x50		/* EEPROM command register */
#define  EECMD_LOCK	0x00
#define  EECMD_UNLOCK	0xc0
#define RGE_PHYAR	0x60		/* PHY access */
#define RGE_PHYSR	0x6c		/* PHY status */
#define RGE_RMS		0xda		/* Rx maximum frame size */
#define RGE_RDSAR	0xe4		/* Rx descriptor base paddr */
#define RGE_ETTHR	0xec		/* Tx threshold */

#define FRAMESIZE	1536

struct local {
	struct desc txd[2]; /* 256B align */
	  uint8_t _hole0[256 - 2 * sizeof(struct desc)];
	struct desc rxd[2]; /* 256B align */
	  uint8_t _hole1[256 - 2 * sizeof(struct desc)];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, tx, rx;
	unsigned phy, bmsr, anlpar;
	unsigned tcr, rcr;
};

static int mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
static void mii_dealan(struct local *, unsigned);

int
rge_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x10ec, 0x8167):
	case PCI_DEVICE(0x10ec, 0x8169):
		return 1;
	}
	return 0;
}

void *
rge_init(unsigned tag, void *data)
{
	unsigned val;
	struct local *l;
	struct desc *txd, *rxd;
	uint32_t reg;
	uint8_t *en;

	l = ALLOC(struct local, 256);	/* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x14)); /* use mem space */

	CSR_WRITE_1(l, RGE_CR, CR_RESET);
	do {
		val = CSR_READ_1(l, RGE_CR);
	} while (val & CR_RESET);

	mii_initphy(l);
	en = data;

	if (brdtype == BRD_QNAPTS) {
		/* read the MAC from flash and write it into the ID-Regs */
		read_mac_from_flash(en);

		CSR_WRITE_1(l, RGE_EECMD, EECMD_UNLOCK);
		reg = en[0] | (en[1] << 8) | (en[2] << 16) | (en[3] << 24);
		CSR_WRITE_4(l, RGE_IDR0, reg);
		reg = en[4] | (en[5] << 8);
		CSR_WRITE_4(l, RGE_IDR4, reg);
		CSR_WRITE_1(l, RGE_EECMD, EECMD_LOCK);
	} else {
		/* pretent the ID-Regs have the correct address */
		en[0] = CSR_READ_1(l, RGE_IDR0);
		en[1] = CSR_READ_1(l, RGE_IDR1);
		en[2] = CSR_READ_1(l, RGE_IDR2);
		en[3] = CSR_READ_1(l, RGE_IDR3);
		en[4] = CSR_READ_1(l, RGE_IDR4);
		en[5] = CSR_READ_1(l, RGE_IDR5);
	}

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    en[0], en[1], en[2], en[3], en[4], en[5]);
	DPRINTF(("PHY %d (%04x.%04x)\n", l->phy,
	    mii_read(l, l->phy, 2), mii_read(l, l->phy, 3)));

	mii_dealan(l, 5);

	/* speed and duplexity can be seen in PHYSR */
	val = CSR_READ_1(l, RGE_PHYSR);
	if (val & (1U << 4))
		printf("1000Mbps");
	if (val & (1U << 3))
		printf("100Mbps");
	if (val & (1U << 2))
		printf("10Mbps");
	if (val & (1U << 0))
		printf("-FDX");
	printf("\n");

	txd = &l->txd[0];
	txd[1].xd0 = htole32(T0_EOR);
	rxd = &l->rxd[0];
	rxd[0].xd0 = htole32(R0_OWN | FRAMESIZE);
	rxd[0].xd2 = htole32(VTOPHYS(l->rxstore[0]));
	rxd[1].xd0 = htole32(R0_OWN | R0_EOR | FRAMESIZE);
	rxd[1].xd2 = htole32(VTOPHYS(l->rxstore[1]));
	wbinv(l, sizeof(struct local));
	l->tx = l->rx = 0;

	l->tcr = (03 << 24) | (07 << 8);
	l->rcr = (07 << 13) | (07 << 8) | RCR_APM;
	CSR_WRITE_1(l, RGE_CR, CR_TXEN | CR_RXEN);
	CSR_WRITE_1(l, RGE_ETTHR, 0x3f);
	CSR_WRITE_2(l, RGE_RMS, FRAMESIZE);
	CSR_WRITE_4(l, RGE_TCR, l->tcr);
	CSR_WRITE_4(l, RGE_RCR, l->rcr);
	CSR_WRITE_4(l, RGE_TNPDS, VTOPHYS(txd));
	CSR_WRITE_4(l, RGE_RDSAR, VTOPHYS(rxd));
	CSR_WRITE_4(l, RGE_TNPDS + 4, 0); 
	CSR_WRITE_4(l, RGE_RDSAR + 4, 0); 
	CSR_WRITE_2(l, RGE_ISR, ~0);
	CSR_WRITE_2(l, RGE_IMR, 0);
	return l;
}

int
rge_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	volatile struct desc *txd;
	unsigned loop, ret;

	ret = len;
	if (len < 60) {
		memset(buf + len, 0, 60 - len);
		len = 60; /* RTL does not stretch <60 Tx frame */
	}
	wbinv(buf, len);
	txd = &l->txd[l->tx];
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd0 &= htole32(T0_EOR);
	txd->xd0 |= htole32(T0_OWN | T0_FS | T0_LS | (len & T0_FRMASK));
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE_1(l, RGE_TPPOLL, 0x40);
	loop = 100;
	do {
		if ((le32toh(txd->xd0) & T0_OWN) == 0)
			goto done;
		DELAY(10);
		inv(txd, sizeof(struct desc));
	} while (--loop > 0);
	printf("xmit failed\n");
	return -1;
  done:
	l->tx ^= 1;
	return ret;
}

int
rge_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
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
		DELAY(1000);	/* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
	if (rxstat & R0_RES) {
		rxd->xd0 &= htole32(R0_EOR);
		rxd->xd0 |= htole32(R0_OWN | FRAMESIZE);
		wbinv(rxd, sizeof(struct desc));
		l->rx ^= 1;
		goto again;
	}
	len = rxstat & R0_FRMASK;
	if (len > maxlen)
		len = maxlen;
	ptr = l->rxstore[l->rx];
	inv(ptr, len);
	memcpy(buf, ptr, len);
	rxd->xd0 &= htole32(R0_EOR);
	rxd->xd0 |= htole32(R0_OWN | FRAMESIZE);
	wbinv(rxd, sizeof(struct desc));
	l->rx ^= 1;
	return len;
}

static int
mii_read(struct local *l, int phy, int reg)
{
	unsigned v;

	v = reg << 16;
	CSR_WRITE_4(l, RGE_PHYAR, v);
	DELAY(1000);
	do {
		DELAY(100);
		v = CSR_READ_4(l, RGE_PHYAR);
	} while ((v & (1U << 31)) == 0); /* wait for 0 -> 1 */
	return v & 0xffff;
}

static void
mii_write(struct local *l, int phy, int reg, int data)
{
	unsigned v;

	v = (reg << 16) | (data & 0xffff) | (1U << 31);
	CSR_WRITE_4(l, RGE_PHYAR, v);
	DELAY(1000);
	do {
		DELAY(100);
		v = CSR_READ_4(l, RGE_PHYAR);
	} while (v & (1U << 31)); /* wait for 1 -> 0 */
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
	int bound, ctl, phy, sts;

	phy = 7;	/* internal rgephy, always at 7 */
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
