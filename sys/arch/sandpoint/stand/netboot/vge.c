/* $NetBSD: vge.c,v 1.6.2.5 2007/12/03 16:14:12 joerg Exp $ */

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
#define DEVTOV(pa) 		(uint32_t)(pa)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)	(T *)((unsigned)alloc(sizeof(T) + (A)) &~ ((A) - 1))

void *vge_init(unsigned, void *);
int vge_send(void *, char *, unsigned);
int vge_recv(void *, char *, unsigned, unsigned);

#define R0_OWN		(1U << 31)	/* 1: empty for HW to load anew */
#define R0_FLMASK	0x3fff0000	/* frame length */
/* RX status upon Rx completed */
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

#define VGE_PAR0	0x00		/* SA [0] */
#define VGE_PAR1	0x01		/* SA [1] */
#define VGE_PAR2	0x02		/* SA [2] */
#define VGE_PAR3	0x03		/* SA [3] */
#define VGE_PAR4	0x04		/* SA [4] */
#define VGE_PAR5	0x05		/* SA [5] */
#define VGE_CAM0	0x10
#define VGE_RCR		0x06		/* Rx control */
#define  RCR_AP		(1U << 6)	/* accept unicast frame */
#define  RCR_AL		(1U << 5)	/* accept large frame */
#define  RCR_PROM	(1U << 4)	/* accept any frame */
#define  RCR_AB		(1U << 3)	/* accept broadcast frame */
#define  RCR_AM		(1U << 2)	/* use multicast filter */  
#define VGE_CTL0	0x08		/* control #0 */
#define  CTL0_TXON	(1U << 4)
#define  CTL0_RXON	(1U << 3)
#define  CTL0_STOP	(1U << 2)
#define  CTL0_START	(1U << 1)
#define VGE_CTL1	0x09		/* control #1 */
#define  CTL1_RESET	(1U << 7)
#define VGE_CTL2	0x0a		/* control #2 */
#define  CTL2_3XFLC	(1U << 7)	/* 802.3x PAUSE flow control */
#define  CTL2_TPAUSE	(1U << 6)	/* handle PAUSE on transmit side */
#define  CTL2_RPAUSE	(1U << 5)	/* handle PAUSE on receive side */
#define  CTL2_HDXFLC	(1U << 4)	/* HDX jabber flow control */
#define VGE_CTL3	0x0b		/* control #3 */
#define  CTL3_GINTMASK	(1U << 1)
#define VGE_TCR		0x07		/* Tx control */
#define VGE_DESCHI	0x18		/* RDES/TDES base high 63:32 */
#define VGE_DATAHI	0x1c		/* frame data base high 63:32 */
#define VGE_ISR		0x24		/* ISR0123 */
#define VGE_IEN		0x28		/* IEN0123 */
#define VGE_TDCSR	0x30
#define VGE_RDCSR	0x32
#define VGE_RDB		0x38		/* RDES base lo 31:0 */
#define VGE_RDINDX	0x3c
#define VGE_TDB0	0x40		/* #0 TDES base lo 31:0 */
#define VGE_RDCSIZE	0x50
#define VGE_TDCSIZE	0x52
#define VGE_TD0IDX	0x54
#define VGE_RBRDU	0x5e
#define VGE_MIICFG	0x6c		/* PHY number 4:0 */
#define VGE_PHYSR0	0x6e		/* PHY status */
#define VGE_MIICR	0x70		/* MII control */
#define  MIICR_RCMD	(1U << 6)	/* MII read operation */
#define  MIICR_WCMD	(1U << 5)	/* MII write operation */
#define VGE_MIIADR	0x71		/* MII indirect */
#define VGE_MIIDATA	0x72		/* MII read/write */
#define VGE_CAMADR	0x68
#define  CAMADR_EN	(1U << 7)	/* enable to manipulate */
#define VGE_CAMCTL	0x69
#define  CAMCTL_RD	(1U << 3)	/* CAM read op, W1S */
#define  CAMCTL_WR	(1U << 2)	/* CAM write op, W1S */
#define VGE_CHIPGCR	0x9f		/* chip global control */

#define FRAMESIZE	1536

struct local {
	struct tdesc txd;
	struct rdesc rxd[2];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, rx;
	unsigned phy, bmsr, anlpar;
	unsigned rcr, ctl0;
};

static int vge_mii_read(struct local *, int, int);
static void vge_mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);

void *
vge_init(unsigned tag, void *data)
{
	unsigned val, i, loop, chipgcr;
	struct local *l;
	struct tdesc *txd;
	struct rdesc *rxd;
	uint8_t *en;

	val = pcicfgread(tag, PCI_ID_REG);
	if (PCI_VENDOR(val) != 0x1106 && PCI_PRODUCT(val) != 0x3119)
		return NULL;

	l = ALLOC(struct local, 256);   /* tdesc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x14)); /* use mem space */

	val = CTL1_RESET;
	CSR_WRITE_1(l, VGE_CTL1, val);
	do {
		val = CSR_READ_1(l, VGE_CTL1);
	} while (val & CTL1_RESET);

	l->phy = CSR_READ_1(l, VGE_MIICFG) & 0x1f;
	mii_initphy(l);

	en = data;
	en[0] = CSR_READ_1(l, VGE_PAR0);
	en[1] = CSR_READ_1(l, VGE_PAR1);
	en[2] = CSR_READ_1(l, VGE_PAR2);
	en[3] = CSR_READ_1(l, VGE_PAR3);
	en[4] = CSR_READ_1(l, VGE_PAR4);
	en[5] = CSR_READ_1(l, VGE_PAR5);
#if 1
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		en[0], en[1], en[2], en[3], en[4], en[5]);
#endif

	txd = &l->txd;
	rxd = &l->rxd[0];
	rxd[0].r0 = htole32(R0_OWN);
	rxd[0].r1 = 0;
	rxd[0].r2 = htole32(VTOPHYS(l->rxstore[0]));
	rxd[0].r3 = htole32(FRAMESIZE << 16);
	rxd[1].r0 = htole32(R0_OWN);
	rxd[1].r1 = 0;
	rxd[1].r2 = htole32(VTOPHYS(l->rxstore[1]));
	rxd[1].r3 = htole32(FRAMESIZE << 16);
	wbinv(txd, sizeof(struct tdesc));
	wbinv(rxd, 2 * sizeof(struct rdesc));
	l->rx = 0;

	/* set own station address into CAM index #0 */	 
	CSR_WRITE_1(l, VGE_CAMCTL, 02 << 6);
	CSR_WRITE_1(l, VGE_CAMADR, CAMADR_EN | 0);
	for (i = 0; i < 6; i++)
		CSR_WRITE_1(l, VGE_CAM0 + i, en[i]);
	CSR_WRITE_1(l, VGE_CAMCTL, 02 << 6 | CAMCTL_WR); 
	loop = 20;
	while (--loop > 0 && (i = CSR_READ_1(l, VGE_CAMCTL)) & CAMCTL_WR)
		DELAY(1);
	CSR_WRITE_1(l, VGE_CAMCTL, 01 << 6);
	CSR_WRITE_1(l, VGE_CAMADR, CAMADR_EN | 0);
	CSR_WRITE_1(l, VGE_CAM0, 01); /* position 0 of 63:0 */
	CSR_WRITE_1(l, VGE_CAMCTL, 01 << 6 | CAMCTL_WR);
	CSR_WRITE_1(l, VGE_CAMADR, 0);

	/* prepare descriptor lists */
	CSR_WRITE_2(l, VGE_DATAHI, 0);
	CSR_WRITE_4(l, VGE_DESCHI, 0);
	CSR_WRITE_4(l, VGE_RDB, VTOPHYS(rxd));
	CSR_WRITE_2(l, VGE_RDCSIZE, 1);
	CSR_WRITE_2(l, VGE_RBRDU, 1);
	CSR_WRITE_4(l, VGE_TDB0, VTOPHYS(txd));
	CSR_WRITE_2(l, VGE_TDCSIZE, 0);
	CSR_WRITE_2(l, VGE_RDCSR, 01);
	CSR_WRITE_2(l, VGE_RDCSR, 04);
	CSR_WRITE_2(l, VGE_TDCSR, 01);

	/* determine speed and duplexity */
	val = vge_mii_read(l, l->phy, 31);
	chipgcr = 1U << 4;
	switch ((val >> 3) & 03) {
	case 02: /* 1000 */
		chipgcr |= 1U << 7;
		break;
	case 01: /* 100 */
	case 00: /* 10 */
		if(val & (1U << 5)) /* FDX */
			chipgcr |= 1U << 6;
		break;
	}
	CSR_WRITE_1(l, VGE_CHIPGCR, chipgcr);

	/* enable transmitter and receiver */
	l->rcr = RCR_AP;
	l->ctl0 |= (CTL0_TXON | CTL0_RXON | CTL0_START);
	CSR_WRITE_1(l, VGE_RCR, l->rcr);
	CSR_WRITE_1(l, VGE_TCR, 0);
	CSR_WRITE_4(l, VGE_ISR, ~0);
	CSR_WRITE_4(l, VGE_IEN, 0);
	CSR_WRITE_1(l, VGE_CTL0, CTL0_START);
	CSR_WRITE_1(l, VGE_CTL0, l->ctl0);

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
	txd = &l->txd;
	txd->tf[0].lo = htole32(VTOPHYS(buf));
	txd->tf[0].hi = htole32(len << 16);
	txd->t1 = htole32(T1_SOF | T1_EOF | (2 << 28));
	txd->t0 = htole32(T0_OWN | len << 16);
	txd->tf[0].hi |= htole32(TF0_Q);
	wbinv(txd, sizeof(struct tdesc));
	CSR_WRITE_2(l, VGE_TDCSR, 04);
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
	rxd->r0 = htole32(R0_OWN);
	rxd->r1 = 0;
	wbinv(rxd, sizeof(struct rdesc));
	l->rx ^= 1;
	return len;
}

static int
vge_mii_read(struct local *sc, int phy, int reg)
{
	int v;

	CSR_WRITE_1(sc, VGE_MIICFG, phy);
	CSR_WRITE_1(sc, VGE_MIIADR, reg);
	CSR_WRITE_1(sc, VGE_MIICR, MIICR_RCMD);
	do {
		v = CSR_READ_1(sc, VGE_MIICR);
	} while (v & MIICR_RCMD);
	return CSR_READ_2(sc, VGE_MIIDATA);
}

static void
vge_mii_write(struct local *sc, int phy, int reg, int data)
{
	int v;

	CSR_WRITE_2(sc, VGE_MIIDATA, data);
	CSR_WRITE_1(sc, VGE_MIICFG, phy);
	CSR_WRITE_1(sc, VGE_MIIADR, reg);
	CSR_WRITE_1(sc, VGE_MIICR, MIICR_WCMD);
	do {
		v = CSR_READ_1(sc, VGE_MIICR);
	} while (v & MIICR_WCMD);
}

#define MII_BMCR	0x00	/* Basic mode control register (rw) */
#define	 BMCR_RESET	0x8000	/* reset */
#define	 BMCR_AUTOEN	0x1000	/* autonegotiation enable */
#define	 BMCR_ISO	0x0400	/* isolate */
#define	 BMCR_STARTNEG	0x0200	/* restart autonegotiation */
#define MII_BMSR	0x01	/* Basic mode status register (ro) */

/* XXX GMII XXX */

static void
mii_initphy(struct local *l)
{
	int phy, ctl, sts, bound;

	l->bmsr = vge_mii_read(l, l->phy, MII_BMSR);
	return; /* XXX */

	for (phy = 0; phy < 32; phy++) {
		ctl = vge_mii_read(l, phy, MII_BMCR);
		sts = vge_mii_read(l, phy, MII_BMSR);
		if (ctl != 0xffff && sts != 0xffff)
			goto found;
	}
	printf("MII: no PHY found\n");
	return;
  found:
	ctl = vge_mii_read(l, phy, MII_BMCR);
	vge_mii_write(l, phy, MII_BMCR, ctl | BMCR_RESET);
	bound = 100;
	do {
		DELAY(10);
		ctl = vge_mii_read(l, phy, MII_BMCR);
		if (ctl == 0xffff) {
			printf("MII: PHY %d has died after reset\n", phy);
			return;
		}
	} while (bound-- > 0 && (ctl & BMCR_RESET));
	if (bound == 0) {
		printf("PHY %d reset failed\n", phy);
	}
	ctl &= ~BMCR_ISO;
	vge_mii_write(l, phy, MII_BMCR, ctl);
	sts = vge_mii_read(l, phy, MII_BMSR) |
	    vge_mii_read(l, phy, MII_BMSR); /* read twice */
	l->phy = phy;
	l->bmsr = sts;
}
