/* $NetBSD: sip.c,v 1.4.2.5 2008/01/21 09:39:10 yamt Exp $ */

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

void *sip_init(unsigned, void *);
int sip_send(void *, char *, unsigned);
int sip_recv(void *, char *, unsigned, unsigned);

#define XD1_OWN		(1U << 31)
#define XD1_OK		(1U << 27)

struct desc {
	uint32_t xd0, xd1, xd2;
	uint32_t hole;
};

#define SIP_CR		0x00
#define  CR_RST		(1U << 8)	/* software reset */
#define  CR_RXR		(1U << 5)	/* Rx abort and reset */
#define  CR_TXR		(1U << 4)	/* Tx abort and reset */
#define  CR_RXD		(1U << 3)	/* graceful Rx stop */
#define  CR_RXE		(1U << 2)	/* run and activate Rx */
#define  CR_TXD		(1U << 1)	/* graceful Tx stop */
#define  CR_TXE		(1U << 0)	/* run and activate Tx */
#define SIP_CFG		0x04
#define SIP_MEAR	0x08
#define  MEAR_EESEL	(1U << 3)	/* SEEP chipselect */
#define  MEAR_EECLK	(1U << 2)	/* clock */
#define  MEAR_EEDO	(1U << 1)	/* bit retrieve */
#define  MEAR_EEDI	(1U << 0)	/* bit feed */
#define SIP_IMR		0x14
#define SIP_IER		0x18
#define SIP_TXDP	0x20
#define SIP_TXCFG	0x24
#define  TXCFG_CSI	(1U << 31)
#define  TXCFG_HBI	(1U << 30)
#define  TXCFG_ATP	(1U << 28)
#define  TXCFG_DMA256	0x300000
#define SIP_RXDP	0x30
#define SIP_RXCFG	0x34
#define  RXCFG_ATX	(1U << 28)
#define  RXCFG_DMA256	0x300000
#define SIP_RFCR	0x48
#define  RFCR_RFEN	(1U << 31)	/* activate Rx filter */
#define  RFCR_APM	(1U << 27)	/* accept perfect match */
#define SIP_RFDR	0x4c
#define SIP_MIBC	0x5c
#define SIP_BMCR	0x80
#define SIP_PHYSTS	0xc0
#define SIP_PHYCR	0xe4

#define FRAMESIZE	1536

struct local {
	struct desc txd;
	struct desc rxd[2];
	uint8_t store[2][FRAMESIZE];
	unsigned csr, rx;
	unsigned phy, bmsr, anlpar;
	unsigned cr;
};

static int read_eeprom(struct local *, int);
static unsigned mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
static void mii_dealan(struct local *, unsigned);

/* Table and macro to bit-reverse an octet. */
static const uint8_t bbr4[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
#define bbr(v)	((bbr4[(v)&0xf] << 4) | bbr4[((v)>>4) & 0xf])

void *
sip_init(unsigned tag, void *data)
{
	unsigned val, i, fdx, txc, rxc;
	struct local *l;
	struct desc *txd, *rxd;
	uint16_t eedata[4], *ee;
	uint8_t *en;

	val = pcicfgread(tag, PCI_ID_REG);
	if (PCI_VENDOR(val) != 0x100b && PCI_PRODUCT(val) != 0x0020)
		return NULL;

	l = ALLOC(struct local, sizeof(struct desc));
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x14)); /* use mem space */

	CSR_WRITE(l, SIP_IER, 0);
	CSR_WRITE(l, SIP_IMR, 0);
	CSR_WRITE(l, SIP_RFCR, 0);
	CSR_WRITE(l, SIP_CR, CR_RST);
	do {
		val = CSR_READ(l, SIP_CR);
	} while (val & CR_RST); /* S1C */

	mii_initphy(l);

	ee = eedata; en = data;
	ee[0] = read_eeprom(l, 6);
	ee[1] = read_eeprom(l, 7);
	ee[2] = read_eeprom(l, 8);
	ee[3] = read_eeprom(l, 9);
	en[0] = ((*ee & 0x1) << 7);
	ee++;
	en[0] |= ((*ee & 0xFE00) >> 9);
	en[1] = ((*ee & 0x1FE) >> 1);
	en[2] = ((*ee & 0x1) << 7);
	ee++;
	en[2] |= ((*ee & 0xFE00) >> 9);
	en[3] = ((*ee & 0x1FE) >> 1);
	en[4] = ((*ee & 0x1) << 7);
	ee++;
	en[4] |= ((*ee & 0xFE00) >> 9);
	en[5] = ((*ee & 0x1FE) >> 1);
	for (i = 0; i < 6; i++)
		en[i] = bbr(en[i]);

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x, ",
	    en[0], en[1], en[2], en[3], en[4], en[5]);
	printf("PHY %d (%04x.%04x)\n", l->phy,
	    mii_read(l, l->phy, 2), mii_read(l, l->phy, 3));

	mii_dealan(l, 5);

	/* speed and duplexity are found in CFG */
	val = CSR_READ(l, SIP_CFG);
	fdx = !!(val & (1U << 29));
	printf("%s", (val & (1U << 30)) ? "100Mbps" : "10Mbps");
	if (fdx)
		printf("-FDX");
	printf("\n");

	txd = &l->txd;
	txd->xd0 = htole32(VTOPHYS(txd));
	rxd = l->rxd;
	rxd[0].xd0 = htole32(VTOPHYS(&rxd[1]));
	rxd[0].xd1 = htole32(XD1_OWN | FRAMESIZE);
	rxd[0].xd2 = htole32(VTOPHYS(l->store[0]));
	rxd[1].xd0 = htole32(VTOPHYS(&rxd[0]));
	rxd[1].xd1 = htole32(XD1_OWN | FRAMESIZE);
	rxd[1].xd2 = htole32(VTOPHYS(l->store[1]));
	wbinv(l, sizeof(struct local));
	l->rx = 0;

	CSR_WRITE(l, SIP_RFCR, 0);
	CSR_WRITE(l, SIP_RFDR, (en[1] << 8) | en[0]);
	CSR_WRITE(l, SIP_RFCR, 2);
	CSR_WRITE(l, SIP_RFDR, (en[3] << 8) | en[2]);
	CSR_WRITE(l, SIP_RFCR, 4);
	CSR_WRITE(l, SIP_RFDR, (en[5] << 8) | en[4]);
	CSR_WRITE(l, SIP_RFCR, RFCR_RFEN | RFCR_APM);

	txc = TXCFG_ATP | TXCFG_DMA256 | 0x1002;
	rxc = RXCFG_DMA256 | 0x20;
	if (fdx) {
		txc |= TXCFG_CSI | TXCFG_HBI;
		rxc |= RXCFG_ATX;
	}
	l->cr = CR_RXE;
	CSR_WRITE(l, SIP_TXDP, VTOPHYS(txd));
	CSR_WRITE(l, SIP_RXDP, VTOPHYS(rxd));
	CSR_WRITE(l, SIP_TXCFG, txc);
	CSR_WRITE(l, SIP_RXCFG, rxc);
	CSR_WRITE(l, SIP_CR, l->cr);

	return l;
}

int
sip_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	struct desc *txd;
	unsigned loop;

	wbinv(buf, len);
	txd = &l->txd;
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd1 = htole32(XD1_OWN | (len & 0xfff));
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE(l, SIP_CR, l->cr | CR_TXE);
	loop = 100;
	do {
		if ((le32toh(txd->xd1) & XD1_OWN) == 0)
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
sip_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	struct desc *rxd;
	unsigned bound, rxstat, len;
	uint8_t *ptr;

	bound = 1000 * timo;
printf("recving with %u sec. timeout\n", timo);
  again:
	rxd = &l->rxd[l->rx];
	do {
		inv(rxd, sizeof(struct desc));
		rxstat = le32toh(rxd->xd1);
		if ((rxstat & XD1_OWN) == 0)
			goto gotone;
		DELAY(1000);	/* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
	if ((rxstat & XD1_OK) == 0) {
		rxd->xd1 = htole32(XD1_OWN | FRAMESIZE);
		wbinv(rxd, sizeof(struct desc));
		l->rx ^= 1;
		goto again;
	}
	/* good frame */
	len = (rxstat & 0xfff) - 4 /* HASFCS */;
	if (len > maxlen)
		len = maxlen;
	ptr = l->store[l->rx];
	inv(ptr, len);
	memcpy(buf, ptr, len);
	rxd->xd1 = htole32(XD1_OWN | FRAMESIZE);
	wbinv(rxd, sizeof(struct desc));
	l->rx ^= 1;
	CSR_WRITE(l, SIP_CR, l->cr);
	return len;
}

static int
read_eeprom(struct local *l, int loc)
{
#define R110 06 /* SEEPROM READ op. */
	unsigned data, v, i;

	/* hold chip select */
	v = MEAR_EESEL;
	CSR_WRITE(l, SIP_MEAR, v);

	data = (R110 << 6) | (loc & 0x3f); /* 6 bit addressing */
	/* instruct R110 op. at loc in MSB first order */
	for (i = (1 << 8); i != 0; i >>= 1) {
		if (data & i)
			v |= MEAR_EEDI;
		else
			v &= ~MEAR_EEDI;
		CSR_WRITE(l, SIP_MEAR, v);
		CSR_WRITE(l, SIP_MEAR, v | MEAR_EECLK);
		DELAY(4);
		CSR_WRITE(l, SIP_MEAR, v);
		DELAY(4);
	}
	v = MEAR_EESEL;
	/* read 16bit quantity in MSB first order */
	data = 0;
	for (i = 0; i < 16; i++) {
		CSR_WRITE(l, SIP_MEAR, v | MEAR_EECLK);
		DELAY(4);
		data = (data << 1) | !!(CSR_READ(l, SIP_MEAR) & MEAR_EEDO);
		CSR_WRITE(l, SIP_MEAR, v);
		DELAY(4);
	}
	/* turn off chip select */
	CSR_WRITE(l, SIP_MEAR, 0);
	DELAY(4);
	return data;
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

unsigned
mii_read(struct local *l, int phy, int reg)
{
	unsigned val;

	do {
		val = CSR_READ(l, SIP_BMCR + (reg << 2));
	} while (reg == MII_BMSR && val == 0);
	return val & 0xffff;
}

void
mii_write(struct local *l, int phy, int reg, int val)
{

	CSR_WRITE(l, SIP_BMCR + (reg << 2), val);
}

void
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
	l->phy = phy; /* should be 0 */
	l->bmsr = sts;
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
