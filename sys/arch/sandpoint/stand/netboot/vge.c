/* $NetBSD: vge.c,v 1.8.4.3 2008/01/09 01:48:39 matt Exp $ */

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
 * - no vtophys() translation, vaddr_t == paddr_t.
 * - PIPT writeback cache aware.
 */
#define CSR_WRITE_1(l, r, v)	*(volatile uint8_t *)((l)->csr+(r)) = (v)
#define CSR_READ_1(l, r)	*(volatile uint8_t *)((l)->csr+(r))
#define CSR_WRITE_2(l, r, v)	out16rb((l)->csr+(r), (v))
#define CSR_READ_2(l, r)	in16rb((l)->csr+(r))
#define CSR_WRITE_4(l, r, v)	out32rb((l)->csr+(r), (v))
#define CSR_READ_4(l, r)	in32rb((l)->csr+(r))
#define VTOPHYS(va)		(uint32_t)(va)
#define DEVTOV(pa)		(uint32_t)(pa)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)	(T *)((unsigned)alloc(sizeof(T) + (A)) &~ ((A) - 1))

void *vge_init(unsigned, void *);
int vge_send(void *, char *, unsigned);
int vge_recv(void *, char *, unsigned, unsigned);

#define R0_OWN		(1U << 31)	/* 1: empty for HW to load anew */
#define R0_FLMASK	0x3fff0000	/* frame length */
#define R0_RXOK		(1U << 15)
#define R0_MAR		(1U << 13)	/* multicast frame */
#define R0_BAR		(1U << 12)	/* broadcast frame */
#define R0_PHY		(1U << 11)	/* unicast frame */	
#define R0_VTAG		(1U << 10)	/* VTAG indicator */	
#define R0_STP		(1U << 9)	/* first frame segment */	
#define R0_EDP		(1U << 8)	/* last frame segment */
#define R0_DETAG	(1U << 7)	/* VTAG has removed */
#define R0_SNTAG	(1U << 6)	/* tagged SNAP frame */
#define R0_SYME		(1U << 5)	/* symbol error */
#define R0_LENE		(1U << 4)	/* frame length error */	
#define R0_CSUME	(1U << 3)	/* TCP/IP bad csum */
#define R0_FAE		(1U << 2)	/* frame alignment error */
#define R0_CRCE		(1U << 1)	/* CRC error */ 
#define R0_VIDM		(1U << 0)	/* VTAG filter miss */		
#define R1_IPOK		(1U << 22)	/* IP csum was fine */
#define R1_TUPOK	(1U << 21)	/* TCP/UDP csum was fine */	
#define R1_FRAG		(1U << 20)	/* fragmented IP */
#define R1_CKSMZO	(1U << 19)	/* UDP csum field was zero */
#define R1_IPKT		(1U << 18)	/* frame was IPv4 */
#define R1_TPKT		(1U << 17)	/* frame was TCPv4 */
#define R1_UPKT		(1U << 16)	/* frame was UDPv4 */		
#define R3_IC		(1U << 31)	/* post Rx interrupt */
#define R_FLMASK	0x00003ffd	/* Rx segment buffer length */	

#define T0_OWN		(1U << 31)	/* 1: loaded for HW to send */	
#define T0_TERR		(1U << 15)	/* Tx error summary */		
#define T0_UDF		(1U << 12)	/* found link down when Tx */	
#define T0_SHDN		(1U << 10)	/* transfer was shutdowned */	
#define T0_CRS		(1U << 9)	/* found carrier sense lost */
#define T0_CDH		(1U << 8)	/* heartbeat check failure */
#define T0_ABT		(1U << 7)	/* excessive collision Tx abort */
#define T0_OWT		(1U << 6)	/* jumbo Tx frame was aborted */
#define T0_OWC		(1U << 5)	/* found out of window collision */
#define T0_COLS		(1U << 4)	/* collision detected */
#define T0_NCRMASK	0xf		/* number of collision retries */
#define T1_EOF		(1U << 25)	/* TCP large last segment */
#define T1_SOF		(1U << 24)	/* TCP large first segment */
#define T1_TIC		(1U << 13)	/* post Tx done interrupt */
#define T1_PIC		(1U << 22)	/* post priority interrupt */
#define T1_VTAG		(1U << 21)	/* insert VLAG tag */
#define T1_IPCK		(1U << 20)	/* generate IPv4 csum */
#define T1_UDPCK	(1U << 19)	/* generate UDPv4 csum */
#define T1_TCPCK	(1U << 18)	/* generate TCPv4 csum */
#define T1_JUMBO	(1U << 17)	/* jumbo frame */
#define T1_CRC		(1U << 16)	/* _disable_ CRC generation */
#define T1_PRIO		0x0000e000	/* VLAN priority value */
#define T1_CFI		(1U << 12)	/* VLAN CFI */
#define T1_VID		0x00000fff	/* VLAN ID 11:0 */
#define T_FLMASK	0x00003fff	/* Tx frame/segment length */
#define TF0_Q		(1U << 31)	/* "Q" bit of tf[0].hi */

struct tdesc {
	uint32_t t0, t1;
	struct {
		uint32_t lo;
		uint32_t hi;
	} tf[7];
};

struct rdesc {
	uint32_t r0, r1, r2, r3;
};

#define VR_PAR0		0x00		/* SA [0] */
#define VR_PAR1		0x01		/* SA [1] */
#define VR_PAR2		0x02		/* SA [2] */
#define VR_PAR3		0x03		/* SA [3] */
#define VR_PAR4		0x04		/* SA [4] */
#define VR_PAR5		0x05		/* SA [5] */
#define VR_CAM0		0x10		/* 0..7 */
#define VR_RCR		0x06		/* Rx control */
#define  RCR_AP		(1U << 6)	/* accept unicast frame */
#define  RCR_AL		(1U << 5)	/* accept long VTAG frame */
#define  RCR_PROM	(1U << 4)	/* accept any frame */
#define  RCR_AB		(1U << 3)	/* accept broadcast frame */
#define  RCR_AM		(1U << 2)	/* use multicast filter */
#define VR_TCR		0x07		/* Tx control */
#define VR_CTL0		0x08		/* control #0 */
#define  CTL0_TXON	(1U << 3)	/* enable Tx DMA */
#define  CTL0_RXON	(1U << 2)	/* enable Rx DMA */
#define  CTL0_STOP	(1U << 1)	/* activate stop processing */
#define  CTL0_START	(1U << 0)	/* start and activate */
#define VR_CTL1		0x09		/* control #1 */
#define  CTL1_RESET	(1U << 7)
#define  CTL1_DPOLL	(1U << 3)	/* _disable_ TDES/RDES polling */
#define VR_CTL2		0x0a		/* control #2 */
#define  CTL2_3XFLC	(1U << 7)	/* 802.3x PAUSE flow control */
#define  CTL2_TPAUSE	(1U << 6)	/* handle PAUSE on transmit side */
#define  CTL2_RPAUSE	(1U << 5)	/* handle PAUSE on receive side */
#define  CTL2_HDXFLC	(1U << 4)	/* HDX jabber flow control */
#define VR_CTL3		0x0b		/* control #3 */
#define  CTL3_GIEN	(1U << 1)	/* global interrupt enable */
#define VR_DESCHI	0x18		/* RDES/TDES base high 63:32 */
#define VR_DATAHI	0x1c		/* frame data base high 63:48 */
#define VR_ISR		0x24		/* ISR0123 */
#define VR_IEN		0x28		/* IEN0123 */
#define VR_TDCSR	0x30
#define VR_RDCSR	0x32
#define VR_RDB		0x38		/* RDES base lo 31:0 */
#define VR_TDB0		0x40		/* #0 TDES base lo 31:0 */
#define VR_RDCSIZE	0x50		/* 0..255 */
#define VR_TDCSIZE	0x52		/* 0..4095 */
#define VR_RBRDU	0x5e		/* 0..255 */
#define VR_CAMADR	0x68
#define  CAM_EN		(1U << 7)	/* enable to manipulate */
#define  SADR_CAM	(0U << 6)	/* station address table */
#define  VTAG_CAM	(1U << 6)	/* VLAN tag table */
#define VR_CAMCTL	0x69
#define  CAMCTL_MULT	(00U << 6)	/* multicast address hash */
#define  CAMCTL_VBIT	(01U << 6)	/* valid bitmask */
#define  CAMCTL_ADDR	(02U << 6)	/* address data */
#define  CAMCTL_RD	(1U << 3)	/* CAM read op, auto cleared */
#define  CAMCTL_WR	(1U << 2)	/* CAM write op, auto cleared */
#define VR_MIICFG	0x6c		/* PHY number 4:0 */
#define VR_MIISR	0x6d		/* MII status */
#define  MIISR_MIDLE	(1U << 7)	/* not in auto polling */
#define VR_PHYSR0	0x6e		/* PHY status 0 */
#define VR_MIICR	0x70		/* MII control */
#define  MIICR_MAUTO	(1U << 7)	/* activate autopoll mode */
#define  MIICR_RCMD	(1U << 6)	/* MII read operation */
#define  MIICR_WCMD	(1U << 5)	/* MII write operation */
#define VR_MIIADR	0x71		/* MII indirect */
#define VR_MIIDATA	0x72		/* MII read/write */

#define FRAMESIZE	1536
#define NRXDESC		4	/* HW demands multiple of 4 */

struct local {
	struct tdesc txd;
	struct rdesc rxd[NRXDESC];
	uint8_t rxstore[NRXDESC][FRAMESIZE];
	unsigned csr, rx;
	unsigned phy, bmsr, anlpar;
};

static void mii_autopoll(struct local *);
static void mii_stoppoll(struct local *);
static int mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_dealan(struct local *, unsigned);

void *
vge_init(unsigned tag, void *data)
{
	unsigned val, i, fdx, loop;
	struct local *l;
	struct tdesc *txd;
	struct rdesc *rxd;
	uint8_t *en;

	val = pcicfgread(tag, PCI_ID_REG);
	if (PCI_VENDOR(val) != 0x1106 && PCI_PRODUCT(val) != 0x3119)
		return NULL;

	l = ALLOC(struct local, 64);   /* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x14)); /* use mem space */

	val = CTL1_RESET;
	CSR_WRITE_1(l, VR_CTL1, val);
	do {
		val = CSR_READ_1(l, VR_CTL1);
	} while (val & CTL1_RESET);

	l->phy = CSR_READ_1(l, VR_MIICFG) & 0x1f;

	en = data;
	en[0] = CSR_READ_1(l, VR_PAR0);
	en[1] = CSR_READ_1(l, VR_PAR1);
	en[2] = CSR_READ_1(l, VR_PAR2);
	en[3] = CSR_READ_1(l, VR_PAR3);
	en[4] = CSR_READ_1(l, VR_PAR4);
	en[5] = CSR_READ_1(l, VR_PAR5);

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    en[0], en[1], en[2], en[3], en[4], en[5]);
	printf("PHY %d (%04x.%04x)\n", l->phy,
	    mii_read(l, l->phy, 2), mii_read(l, l->phy, 3));

	mii_dealan(l, 5);
	
	/* speed and duplexity can be seen in MII 28 */
	val = mii_read(l, l->phy, 28);
	fdx = (val >> 5) & 01;
	switch ((val >> 3) & 03) {
	case 0: printf("10baseT"); break;
	case 1: printf("100baseTX"); break;
	case 2: printf("1000baseT"); break;
	}
	if (fdx)
		printf("-FDX");
	printf("\n");

	txd = &l->txd;
	rxd = &l->rxd[0];
	for (i = 0; i < NRXDESC; i++) {
		rxd[i].r0 = htole32(R0_OWN);
		rxd[i].r1 = 0;
		rxd[i].r2 = htole32(VTOPHYS(l->rxstore[i]));
		rxd[i].r3 = htole32(FRAMESIZE << 16);
	}
	wbinv(l, sizeof(struct local));
	l->rx = 0;

	/* set own station address into entry #0 */	
	CSR_WRITE_1(l, VR_CAMCTL, CAMCTL_ADDR);
	CSR_WRITE_1(l, VR_CAMADR, CAM_EN | SADR_CAM | 0);
	for (i = 0; i < 6; i++)
		CSR_WRITE_1(l, VR_CAM0 + i, en[i]);
	CSR_WRITE_1(l, VR_CAMCTL, CAMCTL_ADDR | CAMCTL_WR);
	loop = 20;
	while (--loop > 0 && (i = CSR_READ_1(l, VR_CAMCTL)) & CAMCTL_WR)
		DELAY(1);
	/* mark entry #0 valid, position 0 of 63:0 */
	CSR_WRITE_1(l, VR_CAMCTL, CAMCTL_VBIT);
	CSR_WRITE_1(l, VR_CAM0, 01);
	for (i = 1; i < 8; i++)
		CSR_WRITE_1(l, VR_CAM0 + i, 00);
	CSR_WRITE_1(l, VR_CAMADR, 0);
	CSR_WRITE_1(l, VR_CAMCTL, 0);

	/* prepare descriptor lists */
	CSR_WRITE_4(l, VR_RDB, VTOPHYS(rxd));
	CSR_WRITE_2(l, VR_RDCSIZE, NRXDESC - 1);
	CSR_WRITE_2(l, VR_RBRDU, NRXDESC - 1);
	CSR_WRITE_4(l, VR_TDB0, VTOPHYS(txd));
	CSR_WRITE_2(l, VR_TDCSIZE, 0);

	/* enable transmitter and receiver */
	CSR_WRITE_1(l, VR_RDCSR, 01);
	CSR_WRITE_1(l, VR_RDCSR, 04);
	CSR_WRITE_2(l, VR_TDCSR, 01);
	CSR_WRITE_1(l, VR_RCR, RCR_AP);
	CSR_WRITE_1(l, VR_TCR, 0);
	CSR_WRITE_1(l, VR_CTL0 + 0x4, CTL0_STOP);
	CSR_WRITE_1(l, VR_CTL0, CTL0_TXON | CTL0_RXON | CTL0_START);
	CSR_WRITE_4(l, VR_ISR, ~0);
	CSR_WRITE_4(l, VR_IEN, 0);

	return l;
}

int
vge_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	struct tdesc *txd;
	unsigned loop;
	
	wbinv(buf, len);
	len = (len & T_FLMASK);
	if (len < 60)
		len = 60; /* needs to stretch to ETHER_MIN_LEN - 4 */
	txd = &l->txd;
	txd->tf[0].lo = htole32(VTOPHYS(buf));
	txd->tf[0].hi = htole32(len << 16);
	txd->t1 = htole32(T1_SOF | T1_EOF | (2 << 28));
	txd->t0 = htole32(T0_OWN | len << 16);
	wbinv(txd, sizeof(struct tdesc));
	CSR_WRITE_2(l, VR_TDCSR, 04);
	loop = 100;
	do {
		if ((le32toh(txd->t0) & T0_OWN) == 0)
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
vge_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
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
		rxstat = le32toh(rxd->r0);
		if ((rxstat & R0_OWN) == 0)
			goto gotone;
		DELAY(1000);	/* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
	if ((rxstat & R0_RXOK) == 0) {
		rxd->r0 = htole32(R0_OWN);
		rxd->r1 = 0;
		wbinv(rxd, sizeof(struct rdesc));
		l->rx ^= 1;
		goto again;
	}
	len = ((rxstat & R0_FLMASK) >> 16) - 4 /* HASFCS */;
	if (len > maxlen)
		len = maxlen;
	ptr = l->rxstore[l->rx];
	inv(ptr, len);
	memcpy(buf, ptr, len);
	if ((l->rx & 03) == 3) {
		/* needs to set R0_OWN to 4 descriptors at a time */
		rxd[00].r0 = htole32(R0_OWN);
		rxd[00].r1 = 0;
		rxd[-1].r0 = htole32(R0_OWN);
		rxd[-1].r1 = 0;
		rxd[-2].r0 = htole32(R0_OWN);
		rxd[-2].r1 = 0;
		rxd[-3].r0 = htole32(R0_OWN);
		rxd[-3].r1 = 0;
		wbinv(rxd, NRXDESC * sizeof(struct rdesc));
	}
	l->rx = (l->rx + 1) & (NRXDESC - 1);
	return len;
}

static void
mii_autopoll(struct local *l)
{
	int v;

	CSR_WRITE_1(l, VR_MIICR, 0);
	CSR_WRITE_1(l, VR_MIIADR, 1U << 7);
	do {
		DELAY(1);
		v = CSR_READ_1(l, VR_MIISR);
	} while ((v & MIISR_MIDLE) == 0);
	CSR_WRITE_1(l, VR_MIICR, MIICR_MAUTO);
	do {
		DELAY(1);
		v = CSR_READ_1(l, VR_MIISR);
	} while ((v & MIISR_MIDLE) != 0);
}

static void
mii_stoppoll(struct local *l)
{	
	int v;
	
	CSR_WRITE_1(l, VR_MIICR, 0);
	do {
		DELAY(1);
		v = CSR_READ_1(l, VR_MIISR);
	} while ((v & MIISR_MIDLE) == 0);
}

static int
mii_read(struct local *l, int phy, int reg)
{
	int v;

	mii_stoppoll(l);
	CSR_WRITE_1(l, VR_MIICFG, phy);
	CSR_WRITE_1(l, VR_MIIADR, reg);
	CSR_WRITE_1(l, VR_MIICR, MIICR_RCMD);
	do {
		v = CSR_READ_1(l, VR_MIICR);
	} while (v & MIICR_RCMD);
	v = CSR_READ_2(l, VR_MIIDATA);
	mii_autopoll(l);
	return v;
}

static void
mii_write(struct local *l, int phy, int reg, int data)
{
	int v;

	mii_stoppoll(l);
	CSR_WRITE_2(l, VR_MIIDATA, data);
	CSR_WRITE_1(l, VR_MIICFG, phy);
	CSR_WRITE_1(l, VR_MIIADR, reg);
	CSR_WRITE_1(l, VR_MIICR, MIICR_WCMD);
	do {
		v = CSR_READ_1(l, VR_MIICR);
	} while (v & MIICR_WCMD);
	mii_autopoll(l);
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
