/* $NetBSD: sip.c,v 1.2 2007/10/26 01:16:27 nisimura Exp $ */

#include <sys/param.h>
#include <sys/socket.h>
 
#include <netinet/in.h>
#include <netinet/in_systm.h>
 
#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include <dev/pci/if_sipreg.h>

#include "globals.h"

/*
 * - reverse endian access every CSR.
 * - no VTOPHYS() translation, vaddr_t == paddr_t. 
 * - PIPT writeback cache aware.
 */
#define CSR_READ(l, r)		in32rb((l)->csr+(r))
#define CSR_WRITE(l, r, v) 	out32rb((l)->csr+(r), (v))
#define VTOPHYS(va) 		(uint32_t)(va)
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)

void *sip_init(void *);
int sip_send(void *, char *, unsigned);
int sip_recv(void *, char *, unsigned, unsigned);

struct desc {
	uint32_t xd0, xd1, xd2;
};

#define FRAMESIZE	1536

struct local {
	struct desc TxD;
	struct desc RxD[2];
	uint8_t store[2][FRAMESIZE];
	unsigned csr, rx;
	unsigned phy, bmsr, anlpar;
};

static int read_eeprom(struct local *, int);
static unsigned sip_mii_read(struct local *, int, int);
static void sip_mii_write(struct local *, int, int, int);
static void mii_initphy(struct local *);

/* Table and macro to bit-reverse an octet. */
static const uint8_t bbr4[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
#define bbr(v)	((bbr4[(v)&0xf] << 4) | bbr4[((v)>>4) & 0xf])

void *
sip_init(void *cookie)
{
	unsigned tag, val, i, txcfg, rxcfg;
	struct local *l;
	struct desc *TxD, *RxD;
	uint16_t eedata[4], *ee;
	uint8_t *en;

	if (pcifinddev(0x100b, 0x0020, &tag) != 0) {
		printf("sip NIC not found\n");
		return NULL;
	}

	l = alloc(sizeof(struct local));
	memset(l, 0, sizeof(struct local));
	l->csr = pcicfgread(tag, 0x14); /* use mem space */

	CSR_WRITE(l, SIP_IER, 0);
	CSR_WRITE(l, SIP_IMR, 0);
	CSR_WRITE(l, SIP_RFCR, 0);
	CSR_WRITE(l, SIP_CR, CR_RST);
	do {
		val = CSR_READ(l, SIP_CR);
	} while (val & CR_RST); /* S1C */

	mii_initphy(l);

	ee = eedata; en = cookie;
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
#if 1
	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
		en[0], en[1], en[2], en[3], en[4], en[5]);
#endif

	TxD = &l->TxD;
	TxD->xd0 = htole32(VTOPHYS(TxD));
	RxD = l->RxD;
	RxD[0].xd0 = htole32(VTOPHYS(&RxD[1]));
	RxD[0].xd1 = htole32(CMDSTS_OWN | FRAMESIZE);
	RxD[0].xd2 = htole32(VTOPHYS(l->store[0]));
	RxD[1].xd0 = htole32(VTOPHYS(&RxD[0]));
	RxD[1].xd1 = htole32(CMDSTS_OWN | FRAMESIZE);
	RxD[1].xd2 = htole32(VTOPHYS(l->store[1]));
	l->rx = 0;

	wbinv(l, sizeof(struct local));
	CSR_WRITE(l, SIP_TXDP, VTOPHYS(TxD));
	CSR_WRITE(l, SIP_RXDP, VTOPHYS(RxD));

	txcfg = TXCFG_ATP | TXCFG_MXDMA_256;
	rxcfg = RXCFG_MXDMA_256;
	txcfg |= TXCFG_CSI | TXCFG_HBI;
	rxcfg |= RXCFG_ATX;
	CSR_WRITE(l, SIP_TXCFG, txcfg);
	CSR_WRITE(l, SIP_RXCFG, rxcfg);
	CSR_WRITE(l, SIP_CR, CR_RXE | CR_TXE);

	return l;
}

int
sip_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	struct desc *TxD;
	unsigned loop;

	if (len > 1520)
		printf("sip_send: len > 1520 (%u)\n", len);

	wbinv(buf, len);
	TxD = &l->TxD;
	TxD->xd2 = htole32(VTOPHYS(buf));
	TxD->xd1 = htole32(CMDSTS_OWN | (len & 0x7ff));
	wbinv(TxD, sizeof(*TxD));
	loop = 100;
	do {
		if ((le32toh(TxD->xd1) & CMDSTS_OWN) == 0)
			goto done;
		DELAY(10);
		inv(TxD, sizeof(*TxD));
	} while (--loop > 0);
	printf("xmit failed\n");
	return -1;
  done:
	return len;
}

int
sip_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	struct desc *RxD;
	time_t bound;
	unsigned rxstat;
	uint8_t *ptr;
	int len;

	bound = 1000 * timo;
printf("recving with %u sec. timeout\n", timo);
  again:
	RxD = &l->RxD[l->rx];
	do {
		inv(RxD, sizeof(*RxD));
		rxstat = le32toh(RxD->xd1);
		if ((rxstat & CMDSTS_OWN) == 0)
			goto gotone;
		DELAY(1000);	/* 1 milli second */
	} while (bound-- > 0);
	errno = 0;
	return -1;
  gotone:
	if (rxstat & 0x07ff0000) {
		RxD->xd1 = htole32(CMDSTS_OWN | FRAMESIZE);
		wbinv(RxD, sizeof(*RxD));
		l->rx ^= 1;
		goto again;
	}
	/* good frame */
	len = (rxstat & 0x7ff) - 4 /* HASFCS */;
	if (len > maxlen)
		len = maxlen;
	ptr = l->store[l->rx];
	inv(ptr, len);
	memcpy(buf, ptr, len);
	RxD->xd1 = htole32(CMDSTS_OWN | FRAMESIZE);
	wbinv(RxD, sizeof(*RxD));
	l->rx ^= 1;
	return len;
}

static int
read_eeprom(l, loc)
	struct local *l;
	int loc;
{
	int reg, data, x;

	reg = EROMAR_EECS;
	CSR_WRITE(l, SIP_EROMAR, reg);

	/* Shift in the READ opcode. */
	for (x = 3; x > 0; x--) {
		if (SIP_EEPROM_OPC_READ & (1 << (x - 1)))
			reg |= EROMAR_EEDI;
		else
			reg &= ~EROMAR_EEDI;
		CSR_WRITE(l, SIP_EROMAR, reg);
		CSR_WRITE(l, SIP_EROMAR,
		    reg | EROMAR_EESK);
		DELAY(4);
		CSR_WRITE(l, SIP_EROMAR, reg);
		DELAY(4);
	}

	/* Shift in address. */
	for (x = 6; x > 0; x--) {
		if (loc & (1 << (x - 1)))
			reg |= EROMAR_EEDI;
		else
			reg &= ~EROMAR_EEDI;
		CSR_WRITE(l, SIP_EROMAR, reg);
		CSR_WRITE(l, SIP_EROMAR,
		    reg | EROMAR_EESK);
		DELAY(4);
		CSR_WRITE(l, SIP_EROMAR, reg);
		DELAY(4);
	}

	/* Shift out data. */
	reg = EROMAR_EECS;
	data = 0;
	for (x = 16; x > 0; x--) {
		CSR_WRITE(l, SIP_EROMAR,
		    reg | EROMAR_EESK);
		DELAY(4);
		if (CSR_READ(l, SIP_EROMAR) & EROMAR_EEDO)
			data |= (1 << (x - 1));
		CSR_WRITE(l, SIP_EROMAR, reg);
		DELAY(4);
	}

	/* Clear CHIP SELECT. */
	CSR_WRITE(l, SIP_EROMAR, 0);
	DELAY(4);
	return data;
}

#define	MII_BMCR	0x00 	/* Basic mode control register (rw) */
#define	 BMCR_RESET	0x8000	/* reset */
#define	 BMCR_AUTOEN	0x1000	/* autonegotiation enable */
#define	 BMCR_ISO	0x0400	/* isolate */
#define	 BMCR_STARTNEG	0x0200	/* restart autonegotiation */
#define	MII_BMSR	0x01	/* Basic mode status register (ro) */

unsigned
sip_mii_read(struct local *l, int phy, int reg)
{
	unsigned val;

	do {
		val = CSR_READ(l, 0x80 + (reg << 2));
	} while (reg == MII_BMSR && val == 0);
	return val & 0xffff;
}

void
sip_mii_write(struct local *l, int phy, int reg, int val)
{

	CSR_WRITE(l, 0x80 + (reg << 2), val);
}

void
mii_initphy(l)
	struct local *l;
{
	int phy, ctl, sts, bound;

	for (phy = 0; phy < 32; phy++) {
		ctl = sip_mii_read(l, phy, MII_BMCR);
		sts = sip_mii_read(l, phy, MII_BMSR);
		if (ctl != 0xffff && sts != 0xffff)
			goto found;
	}
	printf("MII: no PHY found\n");
	return;
  found:
	ctl = sip_mii_read(l, phy, MII_BMCR);
	sip_mii_write(l, phy, MII_BMCR, ctl | BMCR_RESET);
	bound = 100;
	do {
		DELAY(10);
		ctl = sip_mii_read(l, phy, MII_BMCR);
		if (ctl == 0xffff) {
			printf("MII: PHY %d has died after reset\n", phy);
			return;
		}
	} while (bound-- > 0 && (ctl & BMCR_RESET));
	if (bound == 0) {
		printf("PHY %d reset failed\n", phy);
	}
	ctl &= ~BMCR_ISO;
	sip_mii_write(l, phy, MII_BMCR, ctl);
	sts = sip_mii_read(l, phy, MII_BMSR) |
	    sip_mii_read(l, phy, MII_BMSR); /* read twice */
	l->phy = phy; /* should be 0 */
	l->bmsr = sts;
}

#if 0

void
mii_makean(l, timo)
	struct local *l;
	unsigned timo;
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
#endif
