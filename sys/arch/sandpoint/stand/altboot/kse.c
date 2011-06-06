/* $NetBSD: kse.c,v 1.1.8.2 2011/06/06 09:06:35 jruoho Exp $ */

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
#define CSR_READ_4(l, r)	in32rb((l)->csr+(r))
#define CSR_WRITE_4(l, r, v) 	out32rb((l)->csr+(r), (v))
#define CSR_READ_2(l, r)	in16rb((l)->csr+(r))
#define CSR_WRITE_2(l, r, v) 	out16rb((l)->csr+(r), (v))
#define VTOPHYS(va) 		(uint32_t)(va)
#define DEVTOV(pa) 		(uint32_t)(pa)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)
#define ALLOC(T,A)		(T *)allocaligned(sizeof(T),(A))

struct desc {
	uint32_t xd0, xd1, xd2, xd3;
};
#define T0_OWN		(1U<<31)	/* desc is ready to Tx */
#define T1_IC		(1U<<31)	/* post interrupt on complete */
#define T1_FS		(1U<<30)	/* first segment of frame */
#define T1_LS		(1U<<29)	/* last segment of frame */
#define T1_TER		(1U<<25)	/* end of ring */
#define T1_SPN		0x00300000	/* 21:20 switch port 1/2 */
#define T1_TBS_MASK	0x7ff		/* segment size 10:0 */

#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_FS		(1U<<30)	/* first segment of frame */
#define R0_LS		(1U<<29)	/* last segment of frame */
#define R0_IPE		(1U<<28)	/* IP checksum error */
#define R0_TCPE		(1U<<27)	/* TCP checksum error */
#define R0_UDPE		(1U<<26)	/* UDP checksum error */
#define R0_ES		(1U<<25)	/* error summary */
#define R0_MF		(1U<<24)	/* multicast frame */
#define R0_SPN		0x00300000	/* 21:20 switch port 1/2 */
#define R0_RE		(1U<<19)	/* MII reported error */
#define R0_TL		(1U<<18)	/* frame too long, beyond 1518 */
#define R0_RF		(1U<<17)	/* damaged runt frame */
#define R0_CE		(1U<<16)	/* CRC error */
#define R0_FT		(1U<<15)	/* frame type */
#define R0_FL_MASK	0x7ff		/* frame length 10:0 */
#define R1_RER		(1U<<25)	/* end of ring */
#define R1_RBS_MASK	0x7fc		/* segment size 10:0 */

#define MDTXC	0x000	/* DMA transmit control */
#define MDRXC	0x004	/* DMA receive control */
#define MDTSC	0x008	/* DMA transmit start */
#define MDRSC	0x00c	/* DMA receive start */
#define TDLB	0x010	/* transmit descriptor list base */
#define RDLB	0x014	/* receive descriptor list base */
#define MARL	0x200	/* MAC address low */
#define MARM	0x202	/* MAC address middle */
#define MARH	0x204	/* MAC address high */
#define CIDR	0x400	/* chip ID and enable */
#define P1CR4	0x512	/* port 1 control 4 */
#define P1SR	0x514	/* port 1 status */

#define FRAMESIZE	1536

struct local {
	struct desc txd[2];
	struct desc rxd[2];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, tx, rx;
};

static void mii_dealan(struct local *, unsigned);

int
kse_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x16c6, 0x8841):
	case PCI_DEVICE(0x16c6, 0x8842):
		return 1;
	}
	return 0;
}

void *
kse_init(unsigned tag, void *data)
{
	struct local *l;
	struct desc *txd, *rxd;
	unsigned i, val, fdx;
	uint8_t *en;

	l = ALLOC(struct local, 32); /* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x10));

	en = data;
	i = CSR_READ_2(l, MARL);
	en[0] = i;
	en[1] = i >> 8;
	i = CSR_READ_2(l, MARM);
	en[2] = i;
	en[3] = i >> 8;
	i = CSR_READ_2(l, MARH);
	en[4] = i;
	en[5] = i >> 8;

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    en[0], en[1], en[2], en[3], en[4], en[5]);

	CSR_WRITE_2(l, CIDR, 1);

	mii_dealan(l, 5);

	val = pcicfgread(tag, PCI_ID_REG);
	if (PCI_PRODUCT(val) == 0x8841) {
		val = CSR_READ_2(l, P1SR);
		fdx = !!(val & (1U << 9));
		printf("%s", (val & (1U << 8)) ? "100Mbps" : "10Mbps");
		if (fdx)
			printf("-FDX");
		printf("\n");
	}

	txd = &l->txd[0];
	rxd = &l->rxd[0];
	rxd[0].xd0 = htole32(R0_OWN);
	rxd[0].xd1 = htole32(FRAMESIZE);
	rxd[0].xd2 = htole32(VTOPHYS(l->rxstore[0]));
	rxd[0].xd3 = htole32(VTOPHYS(&rxd[1]));
	rxd[1].xd0 = htole32(R0_OWN);
	rxd[1].xd1 = htole32(R1_RER | FRAMESIZE);
	rxd[1].xd2 = htole32(VTOPHYS(l->rxstore[1]));
	rxd[1].xd3 = htole32(VTOPHYS(&rxd[0]));
	l->tx = l->rx = 0;

	CSR_WRITE_4(l, TDLB, VTOPHYS(txd));
	CSR_WRITE_4(l, RDLB, VTOPHYS(rxd));
	CSR_WRITE_4(l, MDTXC, 07); /* stretch short, add CRC, Tx enable */
	CSR_WRITE_4(l, MDRXC, 01); /* Rx enable */
	CSR_WRITE_4(l, MDRSC, 01); /* start receiving */

	return l;
}

int
kse_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	volatile struct desc *txd;
	unsigned txstat, loop;

	wbinv(buf, len);
	txd = &l->txd[l->tx];
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd1 = htole32(T1_FS | T1_LS | (len & T1_TBS_MASK));
	txd->xd0 = htole32(T0_OWN);
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE_4(l, MDTSC, 01); /* start transmission */
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
kse_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
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
		CSR_WRITE_4(l, MDRSC, 01); /* restart receiving */
		goto again;
	}
	/* good frame */
	len = (rxstat & R0_FL_MASK) - 4 /* HASFCS */;
        if (len > maxlen)
                len = maxlen;
	ptr = l->rxstore[l->rx];
        inv(ptr, len);
        memcpy(buf, ptr, len);
	rxd->xd0 = htole32(R0_OWN);
	wbinv(rxd, sizeof(struct desc));
	l->rx ^= 1;
	CSR_WRITE_4(l, MDRSC, 01); /* necessary? */
	return len;
}

void
mii_dealan(struct local *l, unsigned timo)
{
	unsigned val, bound;

	val = (1U << 13) | (1U << 7) | 0x1f /* advertise all caps */;
	CSR_WRITE_2(l, P1CR4, val);
	bound = getsecs() + timo;
	do {
		val = CSR_READ_2(l, P1SR);
		if (val & (1U << 5)) /* link is found up */
			break;
		DELAY(10 * 1000);
	} while (getsecs() < bound);
	return;
}
