/* $NetBSD: skg.c,v 1.4.12.1 2017/12/03 11:36:40 jdolecek Exp $ */

/*-
 * Copyright (c) 2010 Frank Wille.
 * All rights reserved.
 *
 * Written by Frank Wille for The NetBSD Project.
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
	uint32_t xd0, xd1, xd2, xd3, xd4;
	uint32_t rsrvd[3];
};
#define CTL_LS			0x20000000
#define CTL_FS			0x40000000
#define CTL_OWN			0x80000000
#define CTL_DEFOPC		0x00550000
#define FRAMEMASK		0x0000ffff
#define RXSTAT_RXOK		0x00000100

#define SK_CSR			0x0004
#define  CSR_SW_RESET		0x0001
#define  CSR_SW_UNRESET		0x0002
#define  CSR_MASTER_RESET	0x0004
#define  CSR_MASTER_UNRESET	0x0008
#define SK_IMR			0x000c
#define SK_BMU_RX_CSR0		0x0060
#define SK_BMU_TXS_CSR0		0x0068
#define SK_MAC0			0x0100
#define SK_MAC1			0x0108
#define SK_GPIO			0x015c
#define SK_RAMCTL		0x01a0
#define SK_TXAR1_COUNTERCTL	0x0210
#define  TXARCTL_ON		0x02
#define  TXARCTL_FSYNC_ON       0x80
#define SK_RXQ1_CURADDR_LO	0x0420
#define SK_RXQ1_CURADDR_HI	0x0424
#define SK_RXQ1_BMU_CSR		0x0434
#define  RXBMU_CLR_IRQ_EOF		0x00000002
#define  RXBMU_RX_START			0x00000010
#define  RXBMU_RX_STOP			0x00000020
#define  RXBMU_POLL_ON			0x00000080
#define  RXBMU_TRANSFER_SM_UNRESET	0x00000200
#define  RXBMU_DESCWR_SM_UNRESET	0x00000800
#define  RXBMU_DESCRD_SM_UNRESET	0x00002000
#define  RXBMU_SUPERVISOR_SM_UNRESET	0x00008000
#define  RXBMU_PFI_SM_UNRESET		0x00020000
#define  RXBMU_FIFO_UNRESET		0x00080000
#define  RXBMU_DESC_UNRESET		0x00200000
#define SK_TXQS1_CURADDR_LO	0x0620
#define SK_TXQS1_CURADDR_HI	0x0624
#define SK_TXQS1_BMU_CSR	0x0634
#define  TXBMU_CLR_IRQ_EOF		0x00000002
#define  TXBMU_TX_START			0x00000010
#define  TXBMU_TX_STOP			0x00000020
#define  TXBMU_POLL_ON			0x00000080
#define  TXBMU_TRANSFER_SM_UNRESET	0x00000200
#define  TXBMU_DESCWR_SM_UNRESET	0x00000800
#define  TXBMU_DESCRD_SM_UNRESET	0x00002000
#define  TXBMU_SUPERVISOR_SM_UNRESET	0x00008000
#define  TXBMU_PFI_SM_UNRESET		0x00020000
#define  TXBMU_FIFO_UNRESET		0x00080000
#define  TXBMU_DESC_UNRESET		0x00200000
#define SK_RXRB1_START		0x0800
#define SK_RXRB1_END		0x0804
#define SK_RXRB1_WR_PTR		0x0808
#define SK_RXRB1_RD_PTR		0x080c
#define SK_RXRB1_CTLTST		0x0828
#define  RBCTL_UNRESET		0x02
#define  RBCTL_ON		0x08
#define  RBCTL_STORENFWD_ON	0x20
#define SK_TXRBS1_START		0x0a00
#define SK_TXRBS1_END		0x0a04
#define SK_TXRBS1_WR_PTR	0x0a08
#define SK_TXRBS1_RD_PTR	0x0a0c
#define SK_TXRBS1_CTLTST	0x0a28
#define SK_RXMF1_CTRL_TEST	0x0c48
#define  RFCTL_OPERATION_ON	0x00000008
#define  RFCTL_RESET_CLEAR	0x00000002
#define SK_TXMF1_CTRL_TEST	0x0D48
#define  TFCTL_OPERATION_ON	0x00000008
#define  TFCTL_RESET_CLEAR	0x00000002
#define SK_GMAC_CTRL		0x0f00
#define  GMAC_LOOP_OFF		0x00000010
#define  GMAC_PAUSE_ON		0x00000008
#define  GMAC_RESET_CLEAR	0x00000002
#define  GMAC_RESET_SET		0x00000001
#define SK_GPHY_CTRL		0x0f04
#define  GPHY_INT_POL_HI	0x08000000
#define  GPHY_DIS_FC		0x02000000
#define  GPHY_DIS_SLEEP		0x01000000
#define  GPHY_ENA_XC		0x00040000
#define  GPHY_ENA_PAUSE		0x00002000
#define  GPHY_RESET_CLEAR	0x00000002
#define  GPHY_RESET_SET		0x00000001
#define  GPHY_ANEG_ALL		0x0009c000
#define  GPHY_COPPER		0x00f00000
#define SK_LINK_CTRL		0x0f10
#define  LINK_RESET_CLEAR	0x0002
#define  LINK_RESET_SET		0x0001

#define YUKON_GPCR		0x2804
#define  GPCR_TXEN		0x1000
#define  GPCR_RXEN		0x0800
#define YUKON_SA1		0x281c
#define YUKON_SA2		0x2828
#define YUKON_SMICR		0x2880
#define  SMICR_PHYAD(x)		(((x) & 0x1f) << 11)
#define  SMICR_REGAD(x)		(((x) & 0x1f) << 6)
#define  SMICR_OP_READ		0x0020
#define  SMICR_OP_WRITE		0x0000
#define  SMICR_READ_VALID	0x0010
#define  SMICR_BUSY		0x0008
#define YUKON_SMIDR		0x2884

#define MII_PSSR		0x11	/* MAKPHY status register */
#define  PSSR_DUPLEX		0x2000	/* FDX */
#define  PSSR_RESOLVED		0x0800	/* speed and duplex resolved */
#define  PSSR_LINK		0x0400  /* link indication */
#define  PSSR_SPEED(x)		(((x) >> 14) & 0x3)
#define  SPEED10		0
#define  SPEED100		1
#define  SPEED1000 		2

#define FRAMESIZE	1536

struct local {
	struct desc txd[2];
	struct desc rxd[2];
	uint8_t rxstore[2][FRAMESIZE];
	unsigned csr, rx, tx, phy;
	uint16_t pssr, anlpar;
};

static int mii_read(struct local *, int, int);
static void mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);
static void mii_dealan(struct local *, unsigned);

int
skg_match(unsigned tag, void *data)
{
	unsigned v;

	v = pcicfgread(tag, PCI_ID_REG);
	switch (v) {
	case PCI_DEVICE(0x1148, 0x4320):
	case PCI_DEVICE(0x11ab, 0x4320):
		return 1;
	}
	return 0;
}

void *
skg_init(unsigned tag, void *data)
{
	struct local *l;
	struct desc *txd, *rxd;
	uint8_t *en;
	unsigned i;
	uint16_t reg;

	l = ALLOC(struct local, 32);	/* desc alignment */
	memset(l, 0, sizeof(struct local));
	l->csr = DEVTOV(pcicfgread(tag, 0x10)); /* use mem space */

	/* make sure the descriptor bytes are not reversed */
	i = pcicfgread(tag, 0x44);
	pcicfgwrite(tag, 0x44, i & ~4);

	/* reset the chip */
	CSR_WRITE_2(l, SK_CSR, CSR_SW_RESET);
	CSR_WRITE_2(l, SK_CSR, CSR_MASTER_RESET);
	CSR_WRITE_2(l, SK_LINK_CTRL, LINK_RESET_SET);
	DELAY(1000);
	CSR_WRITE_2(l, SK_CSR, CSR_SW_UNRESET);
	DELAY(2);
	CSR_WRITE_2(l, SK_CSR, CSR_MASTER_UNRESET);
	CSR_WRITE_2(l, SK_LINK_CTRL, LINK_RESET_CLEAR);
	CSR_WRITE_4(l, SK_RAMCTL, 2);	/* enable RAM interface */

	mii_initphy(l);

	/* read ethernet address */
	en = data;
	if (brdtype == BRD_SYNOLOGY)
		read_mac_from_flash(en);
	else
		for (i = 0; i < 6; i++)
			en[i] = CSR_READ_1(l, SK_MAC0 + i);
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
	    en[0], en[1], en[2], en[3], en[4], en[5]);
	DPRINTF(("PHY %d (%04x.%04x)\n", l->phy,
	    mii_read(l, l->phy, 2), mii_read(l, l->phy, 3)));

	/* set station address */
	for (i = 0; i < 3; i++)
		CSR_WRITE_2(l, YUKON_SA1 + i * 4,
		    (en[i * 2] << 8) | en[i * 2 + 1]);

	/* configure RX and TX MAC FIFO */
	CSR_WRITE_1(l, SK_RXMF1_CTRL_TEST, RFCTL_RESET_CLEAR);
	CSR_WRITE_4(l, SK_RXMF1_CTRL_TEST, RFCTL_OPERATION_ON);
	CSR_WRITE_1(l, SK_TXMF1_CTRL_TEST, TFCTL_RESET_CLEAR);
	CSR_WRITE_4(l, SK_TXMF1_CTRL_TEST, TFCTL_OPERATION_ON);

	mii_dealan(l, 5);

	switch (PSSR_SPEED(l->pssr)) {
	case SPEED1000:
		printf("1000Mbps");
		break;
	case SPEED100:
		printf("100Mbps");
		break;
	case SPEED10:
		printf("10Mbps");
		break;
	}
	if (l->pssr & PSSR_DUPLEX)
		printf("-FDX");
	printf("\n");

	/* configure RAM buffers, assuming 64k RAM */
	CSR_WRITE_4(l, SK_RXRB1_CTLTST, RBCTL_UNRESET);
	CSR_WRITE_4(l, SK_RXRB1_START, 0);
	CSR_WRITE_4(l, SK_RXRB1_WR_PTR, 0);
	CSR_WRITE_4(l, SK_RXRB1_RD_PTR, 0);
	CSR_WRITE_4(l, SK_RXRB1_END, 0xfff);
	CSR_WRITE_4(l, SK_RXRB1_CTLTST, RBCTL_ON);
	CSR_WRITE_4(l, SK_TXRBS1_CTLTST, RBCTL_UNRESET);
	CSR_WRITE_4(l, SK_TXRBS1_CTLTST, RBCTL_STORENFWD_ON);
	CSR_WRITE_4(l, SK_TXRBS1_START, 0x1000);
	CSR_WRITE_4(l, SK_TXRBS1_WR_PTR, 0x1000);
	CSR_WRITE_4(l, SK_TXRBS1_RD_PTR, 0x1000);
	CSR_WRITE_4(l, SK_TXRBS1_END, 0x1fff);
	CSR_WRITE_4(l, SK_TXRBS1_CTLTST, RBCTL_ON);

	/* setup descriptors and BMU */
	CSR_WRITE_1(l, SK_TXAR1_COUNTERCTL, TXARCTL_ON|TXARCTL_FSYNC_ON);

	txd = &l->txd[0];
	txd[0].xd1 = htole32(VTOPHYS(&txd[1]));
	txd[1].xd1 = htole32(VTOPHYS(&txd[0]));
	rxd = &l->rxd[0];
	rxd[0].xd0 = htole32(FRAMESIZE|CTL_DEFOPC|CTL_LS|CTL_FS|CTL_OWN);
	rxd[0].xd1 = htole32(VTOPHYS(&rxd[1]));
	rxd[0].xd2 = htole32(VTOPHYS(l->rxstore[0]));
	rxd[1].xd0 = htole32(FRAMESIZE|CTL_DEFOPC|CTL_LS|CTL_FS|CTL_OWN);
	rxd[1].xd1 = htole32(VTOPHYS(&rxd[0]));
	rxd[1].xd2 = htole32(VTOPHYS(l->rxstore[1]));
	wbinv(l, sizeof(struct local));

	CSR_WRITE_4(l, SK_RXQ1_BMU_CSR,
	    RXBMU_TRANSFER_SM_UNRESET|RXBMU_DESCWR_SM_UNRESET|
	    RXBMU_DESCRD_SM_UNRESET|RXBMU_SUPERVISOR_SM_UNRESET|
	    RXBMU_PFI_SM_UNRESET|RXBMU_FIFO_UNRESET|
	    RXBMU_DESC_UNRESET);
	CSR_WRITE_4(l, SK_RXQ1_CURADDR_LO, VTOPHYS(rxd));
	CSR_WRITE_4(l, SK_RXQ1_CURADDR_HI, 0);

	CSR_WRITE_4(l, SK_TXQS1_BMU_CSR,
	    TXBMU_TRANSFER_SM_UNRESET|TXBMU_DESCWR_SM_UNRESET|
	    TXBMU_DESCRD_SM_UNRESET|TXBMU_SUPERVISOR_SM_UNRESET|
	    TXBMU_PFI_SM_UNRESET|TXBMU_FIFO_UNRESET|
	    TXBMU_DESC_UNRESET|TXBMU_POLL_ON);
	CSR_WRITE_4(l, SK_TXQS1_CURADDR_LO, VTOPHYS(txd));
	CSR_WRITE_4(l, SK_TXQS1_CURADDR_HI, 0);

	CSR_WRITE_4(l, SK_IMR, 0);
	CSR_WRITE_4(l, SK_RXQ1_BMU_CSR, RXBMU_RX_START);
	reg = CSR_READ_2(l, YUKON_GPCR);
	reg |= GPCR_TXEN | GPCR_RXEN;
	CSR_WRITE_2(l, YUKON_GPCR, reg);

	return l;
}

int
skg_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	volatile struct desc *txd;
	unsigned loop;

	wbinv(buf, len);
	txd = &l->txd[l->tx];
	txd->xd2 = htole32(VTOPHYS(buf));
	txd->xd0 = htole32((len & FRAMEMASK)|CTL_DEFOPC|CTL_FS|CTL_LS|CTL_OWN);
	wbinv(txd, sizeof(struct desc));
	CSR_WRITE_4(l, SK_BMU_TXS_CSR0, TXBMU_TX_START);
	loop = 100;
	do {
		if ((le32toh(txd->xd0) & CTL_OWN) == 0)
			goto done;
		DELAY(10);
		inv(txd, sizeof(struct desc));
	} while (--loop > 0);
	printf("xmit failed\n");
	return -1;
  done:
	l->tx ^= 1;
	return len;
}

int
skg_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	volatile struct desc *rxd;
	unsigned bound, ctl, rxstat, len;
	uint8_t *ptr;

	bound = 1000 * timo;
#if 0
printf("recving with %u sec. timeout\n", timo);
#endif
  again:
	rxd = &l->rxd[l->rx];
	do {
		inv(rxd, sizeof(struct desc));
		ctl = le32toh(rxd->xd0);
		if ((ctl & CTL_OWN) == 0)
			goto gotone;
		DELAY(1000);	/* 1 milli second */
	} while (--bound > 0);
	errno = 0;
	return -1;
  gotone:
  	rxstat = le32toh(rxd->xd4);
	if ((rxstat & RXSTAT_RXOK) == 0) {
		rxd->xd0 = htole32(FRAMESIZE|CTL_DEFOPC|CTL_LS|CTL_FS|CTL_OWN);
		wbinv(rxd, sizeof(struct desc));
		l->rx ^= 1;
		goto again;
	}
	len = ctl & FRAMEMASK;
	if (len > maxlen)
		len = maxlen;
	ptr = l->rxstore[l->rx];
	inv(ptr, len);
	memcpy(buf, ptr, len);
	rxd->xd0 = htole32(FRAMESIZE|CTL_DEFOPC|CTL_LS|CTL_FS|CTL_OWN);
	wbinv(rxd, sizeof(struct desc));
	l->rx ^= 1;
	return len;
}

static int
mii_read(struct local *l, int phy, int reg)
{
	unsigned loop, v;

	CSR_WRITE_2(l, YUKON_SMICR, SMICR_PHYAD(phy) | SMICR_REGAD(reg) |
	    SMICR_OP_READ);
	loop = 1000;
	do {
		DELAY(1);
		v = CSR_READ_2(l, YUKON_SMICR);
	} while ((v & SMICR_READ_VALID) == 0 && --loop);
	if (loop == 0) {
		printf("mii_read timeout!\n");
		return 0;
	}
	return CSR_READ_2(l, YUKON_SMIDR);
}

static void
mii_write(struct local *l, int phy, int reg, int data)
{
	unsigned loop, v;

	CSR_WRITE_2(l, YUKON_SMIDR, data);
	CSR_WRITE_2(l, YUKON_SMICR, SMICR_PHYAD(phy) | SMICR_REGAD(reg) |
	    SMICR_OP_WRITE);
	loop = 1000;
	do {
		DELAY(1);
		v = CSR_READ_2(l, YUKON_SMICR);
	} while ((v & SMICR_BUSY) != 0 && --loop);
	if (loop == 0)
		printf("mii_write timeout!\n");
}

#define MII_BMCR	0x00	/* Basic mode control register (rw) */
#define  BMCR_AUTOEN	0x1000	/* autonegotiation enable */
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

static void
mii_initphy(struct local *l)
{
	unsigned val;

	l->phy = 0;

	/* take PHY out of reset */
	val = CSR_READ_4(l, SK_GPIO);
	CSR_WRITE_4(l, SK_GPIO, (val | 0x2000000) & ~0x200);

	/* GMAC and GPHY reset */
	CSR_WRITE_4(l, SK_GPHY_CTRL, GPHY_RESET_SET);
	CSR_WRITE_4(l, SK_GMAC_CTRL, GMAC_RESET_SET);
	DELAY(1000);
	CSR_WRITE_4(l, SK_GMAC_CTRL, GMAC_RESET_CLEAR);
	CSR_WRITE_4(l, SK_GMAC_CTRL, GMAC_RESET_SET);
	DELAY(1000);

	val = GPHY_INT_POL_HI | GPHY_DIS_FC | GPHY_DIS_SLEEP | GPHY_ENA_XC |
	    GPHY_ANEG_ALL | GPHY_ENA_PAUSE | GPHY_COPPER;
	CSR_WRITE_4(l, SK_GPHY_CTRL, val | GPHY_RESET_SET);
	DELAY(1000);
	CSR_WRITE_4(l, SK_GPHY_CTRL, val | GPHY_RESET_CLEAR);
	CSR_WRITE_4(l, SK_GMAC_CTRL, GMAC_LOOP_OFF | GMAC_PAUSE_ON |
	    GMAC_RESET_CLEAR);
}

static void
mii_dealan(struct local *l, unsigned timo)
{
	unsigned bmsr, bound;

	mii_write(l, l->phy, MII_ANAR, ANAR_TX_FD | ANAR_TX | ANAR_10_FD |
	    ANAR_10 | ANAR_CSMA | ANAR_FC);
	mii_write(l, l->phy, MII_GTCR, GANA_1000TFDX | GANA_1000THDX);
	mii_write(l, l->phy, MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);
	l->anlpar = 0;
	bound = getsecs() + timo;
	do {
		bmsr = mii_read(l, l->phy, MII_BMSR) |
		   mii_read(l, l->phy, MII_BMSR); /* read twice */
		if ((bmsr & BMSR_LINK) && (bmsr & BMSR_ACOMP)) {
			l->pssr = mii_read(l, l->phy, MII_PSSR);
			l->anlpar = mii_read(l, l->phy, MII_ANLPAR);
			if ((l->pssr & PSSR_RESOLVED) == 0)
				continue;
			break;
		}
		DELAY(10 * 1000);
	} while (getsecs() < bound);
}
