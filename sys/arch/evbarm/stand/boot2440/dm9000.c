/* $NetBSD: dm9000.c,v 1.2.4.1 2017/12/03 11:36:07 jdolecek Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
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

/*
 * This DM9000 is wired as a 16bit device and manages Tx/Rx SRAM buffer
 * in 16bit quantity.  MRCMD/MWCMD access increments buffer pointer
 * by two regardless of r/w size.  Mixing 16/8bit access is not possible.
 * Byte read would end up with loosing every odd indexed datum.
 *
 * The DM9000 CMD pin is tied with SoC LADDR2 address line.  SA9-SA4
 * pins are hardwired to fixed decoding 0x300.  Thus address [26:3] in
 * CS4 range 0x2000'0000 are don't-care bits to manipulate the chip.
 * The DM9000 INDEX port is accessed at the address b'000 while the
 * DATA port at the address b'100.
 *
 * This code assumes Little endian CPU.
 */

#define NCR	0x00		/* control */
#define  NCR_FDX	(1<<3)	/* FDX link detection report */
#define  NCR_RST	(1<<0)	/* instruct reset, goes 0 at done */
#define NSR	0x01		/* status */
#define  NSR_SPEED	(1<<7)	/* 1->100M, 0->10M, when link is up */
#define  NSR_LINKST	(1<<6)	/* 1->linkup, 0->linkdown */
#define  NSR_TX2END	(1<<3)	/* Tx frame #2 completed */
#define  NSR_TX1END	(1<<2)	/* Tx frame #1 completed */
#define  NSR_RXOV	(1<<1)	/* Rx FIFO overflow detected */
#define TCR	0x02		/* Tx control */
#define  TCR_TXREQ	0x01	/* request to start transmit, goes 0 at done */
#define TCR2	0x2d		/* Tx control #2 */
#define  TCR2_ONEPM	0x10	/* send single Tx frame at a time */
#define RCR	0x05		/* Rx control */
#define  RCR_WTDIS	0x40	/* disable frame receipt watchdog timer */
#define  RCR_DIS_LONG	0x20	/* discard too-long Rx frame */
#define  RCR_DIS_CRC	0x10	/* discard CRC error Rx frame */
#define  RCR_ALL	0x08	/* accept MCAST frames */
#define  RCR_RUNT	0x04	/* accept runt Rx frame */
#define  RCR_PRMSC	0x02	/* promiscous */
#define  RCR_RXEN	0x01	/* enable frame reception */
#define RSR	0x06		/* RX status */
#define  RSR_MF		(1<<6)	/* bcast/mcast frame found */
#define FCR	0x0a		/* flow control */
#define  FCR_FLCE	0x01	/* enable Tx/Rx flow control */
#define EPCR	0x0b		/* EEPROM and PHY control */
#define  EP_EPOS	(1<<3)	/* 1 for PHY op, 0 for EEPROM op */
#define  EP_ERPRR	(1<<2)	/* instruct to start read op */
#define  EP_ERPRW	(1<<1)	/* instruct to start write op */
#define  EP_ERRE	(1<<0)	/* 1 while operation is in progress */
#define EPAR	0x0c		/* [7:6] for PHY#, [5:0] for addr */
#define EPDRL	0x0d		/* EEPROM/PHY data low byte */
#define EPDRH	0x0e		/* EEPROM/PHY data high byte */
#define PAR	0x10		/* station address */
#define MAR	0x16		/* multicast filter hash value */
#define GPR	0x1f		/* gpio control */
#define  GPR_PHYPWROFF	0x01	/* powerdown internal PHY */
#define VID0	0x28		/* vendor ID low byte */
#define VID1	0x29		/* vendor ID high byte */
#define PID0	0x2a		/* product ID low byte */
#define PID1	0x2b		/* product ID high byte */
#define CHIPR	0x2c		/* chip revision */
#define MRCMDX	0xf0		/* read data w/o pointer incr */
#define MRCMD	0xf2		/* read data with pointer auto incr */
#define MWCMD	0xf8		/* write data with pointer auto incr */
#define TXPLL	0xfc		/* Tx frame length low byte */
#define TXPLH	0xfd		/* Tx frame length high byte */
#define ISR	0xfe		/* interrupt status report */
#define  ISR_LNKCHG	0x20	/* link status change detected */
#define  ISR_UDRUN	0x10	/* transmit underrun detected */
#define  ISR_PTM	(1<<1)	/* frame Tx completed */
#define  ISR_PRS	(1<<0)	/* frame Rx completed */
#define IMR	0xff		/* interrupt mask */
#define  IMR_PAR	(1<<7)	/* use 3/13K SRAM partitioning with autowrap */
#define  IMR_PRM	(1<<0)	/* post interrupt when Rx completed */

#ifndef DM9000MAC
#define DM9000MAC 0x08,0x08,0x11,0x18,0x12,0x27
#endif

struct local {
	unsigned int csr;
	unsigned int phy, bmsr, anlpar;
	uint8_t en[6];
};

static struct local dm9000local;

int dm9k_match(unsigned int, void *);
void *dm9k_init(unsigned int, void *);
int dm9k_send(void *, char *, unsigned int);
int dm9k_recv(void *, char *, unsigned int, unsigned int);

static unsigned mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_dealan(struct local *, unsigned int);

extern void usleep(int);

static inline int
CSR_READ_1(struct local *l, int reg)
{
	*(volatile uint8_t *)(l->csr) = reg;
	return *(volatile uint8_t*)(l->csr + 4);
}

static inline int
CSR_READ_2(struct local *l, int reg)
{
	*(volatile uint8_t *)(l->csr) = reg;
	return *(volatile uint16_t *)(l->csr + 4);
}

static inline void
CSR_WRITE_1(struct local *l, int reg, int data)
{
	*(volatile uint8_t *)(l->csr) = reg;
	*(volatile uint8_t *)(l->csr + 4) = data;
}

static inline void
CSR_WRITE_2(struct local *l, int reg, int data)
{
	*(volatile uint8_t *)(l->csr) = reg;
	*(volatile uint16_t *)(l->csr + 4) = data;
}

int
dm9k_match(unsigned int tag, void *aux)
{
	struct local *l = &dm9000local;
	uint8_t *en = aux;
	uint8_t std[6] = { DM9000MAC };
	int val;

	l->csr = 0x20000000;
	val =  CSR_READ_1(l, PID0);
	val |= CSR_READ_1(l, PID1) << 8;
	val |= CSR_READ_1(l, VID0) << 16;
	val |= CSR_READ_1(l, VID1) << 24;
	if (val != 0x0a469000) {
		printf("DM9000 not found at 0x%x\n", l->csr);
		return 0;
	}
	if (en != NULL
	    && en[0] && en[1] && en[2] && en[3] && en[4] && en[5])
		memcpy(l->en, en, 6);
	else if (en != NULL) {
		memcpy(en, std, 6);
		memcpy(l->en, std, 6);
	}
	return 1;
}

void *
dm9k_init(unsigned int tag, void *aux)
{
	struct local *l = &dm9000local;
	uint8_t *en = l->en;
	unsigned int val, fdx;

	val = CSR_READ_1(l, CHIPR);
	printf("DM9000 rev. %#x", val);
	val = CSR_READ_1(l, ISR);
	printf(", %d bit mode\n", (val & 1<<7) ? 8 : 16);

	CSR_WRITE_1(l, NCR, 0);	/* use internal PHY */
	l->phy = 1;

	/* force PHY poweroff */
	CSR_WRITE_1(l, GPR, GPR_PHYPWROFF);

	CSR_WRITE_1(l, IMR, 0);
	CSR_WRITE_1(l, TCR, 0);
	CSR_WRITE_1(l, RCR, 0);

	/* SW reset */
	CSR_WRITE_1(l, NCR, NCR_RST);
	do {
		usleep(1);
	} while (NCR_RST & CSR_READ_1(l, NCR));

	/* negate PHY poweroff condition */
	CSR_WRITE_1(l, GPR, 0);

	/* SW reset, again */
	CSR_WRITE_1(l, NCR, NCR_RST);
	do {
		usleep(1);
	} while (NCR_RST & CSR_READ_1(l, NCR));

	/* clear NSR bits */
	(void) CSR_READ_1(l, NSR);

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	       en[0], en[1], en[2], en[3], en[4], en[5]);
	CSR_WRITE_1(l, PAR + 0, en[0]);
	CSR_WRITE_1(l, PAR + 1, en[1]);
	CSR_WRITE_1(l, PAR + 2, en[2]);
	CSR_WRITE_1(l, PAR + 3, en[3]);
	CSR_WRITE_1(l, PAR + 4, en[4]);
	CSR_WRITE_1(l, PAR + 5, en[5]);

	/* make sure not to receive bcast/mcast frames */
	CSR_WRITE_1(l, MAR + 0, 0);
	CSR_WRITE_1(l, MAR + 1, 0);
	CSR_WRITE_1(l, MAR + 2, 0);
	CSR_WRITE_1(l, MAR + 3, 0);
	CSR_WRITE_1(l, MAR + 4, 0);
	CSR_WRITE_1(l, MAR + 5, 0);
	CSR_WRITE_1(l, MAR + 6, 0);
	CSR_WRITE_1(l, MAR + 7, 0);

	/* perform link auto-negotiation */
	printf("waiting for linkup ... ");
	mii_dealan(l, 5);

	val = CSR_READ_1(l, NSR);
	if ((val & NSR_LINKST) == 0) {
		printf("failed; cable problem?\n");
		return NULL;
	}

	/*
	 * speed and duplexity can be seen in MII 17.
	 *  bit15 100Mbps-FDX
	 *  bit14 100Mbps
	 *  bit13 10Mbps-FDX
	 *  bit12 10Mbps
	 * also available in NSR[SPEED] and NCR[FDX] respectively.
	 */
	val = mii_read(l, l->phy, 17);
	if (val & (03 << 14))
		printf("100Mbps");
	if (val & (03 << 12))
		printf("10Mbps");
	fdx = !!(val & (05 << 13));
	if (fdx) {
		printf("-FDX");
		val = CSR_READ_1(l, FCR);
		CSR_WRITE_1(l, FCR, val | FCR_FLCE);
	}
	printf("\n");

	CSR_WRITE_1(l, NSR, ~0);
	CSR_WRITE_1(l, ISR, ~0);

	/*
	 * - send one frame at a time.
	 * - disable Rx watchdog timer, discard too-long/CRC error frames.
	 * - 3/13K SRAM partitioning, r/w pointer autowrap.
	 */
	CSR_WRITE_1(l, TCR2, TCR2_ONEPM); 
	CSR_WRITE_1(l, RCR, RCR_RXEN | RCR_WTDIS | RCR_DIS_LONG | RCR_DIS_CRC);
	CSR_WRITE_1(l, IMR, IMR_PAR);

	memcpy(aux, l->en, 6);
	return l;
}

int
dm9k_send(void *dev, char *buf, unsigned int len)
{
	struct local *l = dev;
	unsigned int val, cnt, bound;

	if (len > 1520) {
		printf("dm9k_send: len > 1520 (%u)\n", len);
		len = 1520;
	}

	CSR_WRITE_1(l, ISR, ISR_PTM); /* clear ISR Tx complete bit */
	for (cnt = 0; cnt < len; cnt += 2) {
		val = (buf[1] << 8) | buf[0];
		CSR_WRITE_2(l, MWCMD, val);
		buf += 2;
	}
	CSR_WRITE_1(l, TXPLL, len);
	CSR_WRITE_1(l, TXPLH, len >> 8);
	CSR_WRITE_1(l, TCR, TCR_TXREQ);	/* request to transmit */

	bound = getsecs() + 1;
	do {
		val = CSR_READ_1(l, TCR);
		if ((val & TCR_TXREQ) == 0)
			goto done;
	} while (getsecs() < bound);
	printf("xmit failed\n");
	return -1;
  done:
	return len;
}

int
dm9k_recv(void *dev, char *buf, unsigned int maxlen, unsigned int timo)
{
	struct local *l = dev;
	unsigned int bound, val, mark, stat, len, upto, cnt;
	char *ptr;

	bound = getsecs() + timo; /* second */
  again:
	do {
		/* wait for Rx completion */
		val = CSR_READ_1(l, ISR);
		if (val & ISR_PRS)
			goto gotone;
		/* usleep(10); this makes a stuck in mid transfer */
	} while (getsecs() < bound);
	printf("receive timeout (%d seconds wait)\n", timo);
	errno = 0;
	return -1;
  gotone:
	CSR_WRITE_1(l, ISR, ISR_PRS); /* clear ISR Rx complete bit */
	(void) CSR_READ_2(l, MRCMDX); /* dummy read */
	mark = CSR_READ_2(l, MRCMDX); /* mark in [7:0] */
	if ((mark & 03) != 01) {
		stat = CSR_READ_1(l, RSR);
		printf("dm9k_recv: mark %x, RSR %x\n", mark, stat);
		/* XXX got hosed, need full scale reinitialise XXX */
		goto again;
	}

	stat = CSR_READ_2(l, MRCMD); /* stat in [15:8] */
	len  = CSR_READ_2(l, MRCMD);

	/* should not happen, make sure to discard bcast/mcast frames */
	if (stat & (RSR_MF<<8)) {
		for (cnt = 0; cnt < len; cnt += 2)
			(void) CSR_READ_2(l, MRCMD);
		printf("bcast/mcast frame, len = %d\n", len);
		goto again;
	}

	upto = len - 4;	/* HASFCS */
	if (upto > maxlen)
		upto = maxlen;
	ptr = buf;
	for (cnt = 0; cnt < upto; cnt += 2) {
		val = CSR_READ_2(l, MRCMD);
		ptr[0] = val;
		ptr[1] = val >> 8;
		ptr += 2;
	}
	/* discard trailing bytes */
	for (; cnt < len; cnt += 2)
		(void) CSR_READ_2(l, MRCMD);

	return upto;
}

static unsigned int
mii_read(struct local *l, int phy, int reg)
{
	int v;

	CSR_WRITE_1(l, EPAR, phy << 6 | reg);
	CSR_WRITE_1(l, EPCR, EP_EPOS | EP_ERPRR);
	do {
		v = CSR_READ_1(l, EPCR);
	} while (v & EP_ERRE);
	CSR_WRITE_1(l, EPCR, 0);
	v = (CSR_READ_1(l, EPDRH) << 8) | CSR_READ_1(l, EPDRL);
	return v;
}

static void
mii_write(struct local *l, int phy, int reg, int data)
{
	int v;

	CSR_WRITE_1(l, EPAR, phy << 6 | reg);
	CSR_WRITE_1(l, EPDRL, data);
	CSR_WRITE_1(l, EPDRH, data >> 8);
	CSR_WRITE_1(l, EPCR, EP_EPOS | EP_ERPRW);
	do {
		v = CSR_READ_1(l, EPCR);
	} while (v & EP_ERRE);
	CSR_WRITE_1(l, EPCR, 0);
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
mii_dealan(struct local *l, unsigned int timo)
{
	unsigned int bound;

	mii_write(l, l->phy, MII_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_10_FD |
	    ANAR_10 | ANAR_CSMA | ANAR_FC);
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
		usleep(10 * 1000);
	} while (getsecs() < bound);
	return;
}
