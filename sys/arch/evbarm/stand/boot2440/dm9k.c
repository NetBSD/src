/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
#include <sys/types.h>

#include <lib/libsa/stand.h>

#include <dev/ic/dm9000reg.h>

/* Register defines which are not yet in dm9000reg */
#define  DM9000_PHY_ANAR_CSMACD		0x01
#define  DM9000_PHY_ANAR_10_HDX		(1<<5)
#define  DM9000_PHY_ANAR_10_FDX		(1<<6)
#define  DM9000_PHY_ANAR_TX_HDX		(1<<7)
#define  DM9000_PHY_ANAR_TX_FDX		(1<<8)
#define  DM9000_PHY_ANAR_T4		(1<<9)
#define  DM9000_PHY_ANAR_FCS		(1<<10)
#define  DM9000_PHY_ANAR_RF		(1<<13)
#define  DM9000_PHY_ANAR_ACK		(1<<14)
#define  DM9000_PHY_ANAR_NP		(1<<15)

#define  DM9000_NCR_EXT_PHY     (1 << 7)

#define  DM9000_EPAR_PHY(n)     ((n<<6) & 0xC0)

/* IOMODE macros, which are no longer present in the real driver.
   Keep them here until we rework this driver.
 */
#define  DM9000_IOMODE(n)       (n<<DM9000_IOMODE_SHIFT) & DM9000_IOMODE_MASK
#define  DM9000_IOMODE_8BIT     DM9000_IOMODE(2)
#define  DM9000_IOMODE_16BIT    DM9000_IOMODE(0)
#define  DM9000_IOMODE_32BIT    DM9000_IOMODE(1)

#include "dm9k.h"

extern time_t getsecs(void);
extern struct btinfo_net	bi_net;

struct dm9k_sc {
	unsigned int csr;
	unsigned int tx;
	int phy;
};

static struct dm9k_sc dm9k_sc;
static uint8_t dm9k_mac[6] = {DM9000MAC};

/* Inline memory access methods */
static inline int
CSR_READ_1(struct dm9k_sc *sc, int reg)
{
	*(volatile uint8_t*)(sc->csr) = reg;
	return *(volatile uint8_t*)(sc->csr+4);
}

static inline int
CSR_READ_2(struct dm9k_sc *sc, int reg)
{
	*(volatile uint8_t *)(sc->csr) = reg;
	return *(volatile uint16_t *)(sc->csr + 4);
}

static inline void
CSR_WRITE_1(struct dm9k_sc *sc, int reg, int data)
{
	*(volatile uint8_t *)(sc->csr) = reg;
	*(volatile uint8_t *)(sc->csr + 4) = data;
}

static inline void
CSR_WRITE_2(struct dm9k_sc *sc, int reg, int data)
{
	*(volatile uint8_t *)(sc->csr) = reg;
	*(volatile uint16_t *)(sc->csr + 4) = data;
}

static u_int16_t   mii_read(struct dm9k_sc *sc, int phy, int reg);
static void	   mii_write(struct dm9k_sc *sc, int phy, int reg,
			     u_int16_t value);

int
dm9k_match(unsigned int tag, void *macaddr)
{
	struct dm9k_sc sc;
	uint8_t *en = macaddr;
	unsigned int val;

	sc.csr = 0x20000000;

	val = (CSR_READ_1(&sc, DM9000_PID0)) |
	  (CSR_READ_1(&sc, DM9000_PID1)<<8);
	val |=((CSR_READ_1(&sc, DM9000_VID0)) |
	       (CSR_READ_1(&sc, DM9000_VID1)<<8)) << 16;

	if( val != 0x0a469000 ) {
		printf("DM9000 Chip not found\n");
		return 0;
	}

	val = CSR_READ_1(&sc, DM9000_ISR) & DM9000_IOMODE_MASK;
	switch (val) {
	case DM9000_IOMODE_8BIT:
		printf("Unsupported I/O Mode: 8 Bit\n");
		return 0;
		break;
	case DM9000_IOMODE_16BIT:
		break;
	case DM9000_IOMODE_32BIT:
		printf("Unsupported I/O Mode: 32 Bit\n");
		return 0;
		break;
	}

	if( en != NULL &&
	    en[0] != 0x00 && en[1] != 0x00 &&
	    en[2] != 0x00 && en[3] != 0x00 &&
	    en[4] != 0x00 && en[5] != 0x00 ) {
		/* Set dm9k mac-address if input is not zero */
		memcpy(dm9k_mac, en, sizeof(dm9k_mac));
	} else {
		if( en != NULL ) {
			/* Return dm9k mac-address */
			memcpy(en, dm9k_mac, sizeof(dm9k_mac));
		}
	}

	return 1;
}

void*
dm9k_init(unsigned int tag, void *macaddr)
{
	uint8_t var;
	int start;
	struct dm9k_sc *sc = &dm9k_sc;

	sc->csr = 0x20000000;
	sc->phy = 1; /* Internal PHY */
	sc->tx = 0;

	CSR_WRITE_1(sc, DM9000_PAB0, dm9k_mac[0]);
	CSR_WRITE_1(sc, DM9000_PAB1, dm9k_mac[1]);
	CSR_WRITE_1(sc, DM9000_PAB2, dm9k_mac[2]);
	CSR_WRITE_1(sc, DM9000_PAB3, dm9k_mac[3]);
	CSR_WRITE_1(sc, DM9000_PAB4, dm9k_mac[4]);
	CSR_WRITE_1(sc, DM9000_PAB5, dm9k_mac[5]);

	printf("Davicom DM9000 NIC configured with MAC address: "
	       "%x:%x:%x:%x:%x:%x\n",
	       dm9k_mac[0], dm9k_mac[1], dm9k_mac[2],
	       dm9k_mac[3], dm9k_mac[4], dm9k_mac[5]);

	memcpy(macaddr, dm9k_mac, sizeof(dm9k_mac));

	/* Recommended initialization procedure as described in the
	 * DM9000 ISA Programming Guide section 2: */

	/* (1) PHY Reset (Internal) */
	mii_write(sc, sc->phy, DM9000_PHY_BMCR, DM9000_PHY_BMCR_RESET);

	/* (2) PHY Power down */
	var = CSR_READ_1(sc, DM9000_GPR);
	CSR_WRITE_1(sc, DM9000_GPR, var | DM9000_GPR_PHY_PWROFF);

	/* (3) Disable all interrupts & RX / TX */
	CSR_WRITE_1(sc, DM9000_IMR, 0x0);
	CSR_WRITE_1(sc, DM9000_TCR, 0x0);
	CSR_WRITE_1(sc, DM9000_RCR, 0x0);

	/* (4) Software Reset*/
	CSR_WRITE_1(sc, DM9000_NCR, DM9000_NCR_RST |
		    DM9000_NCR_LBK_MAC_INTERNAL);
	while( CSR_READ_1(sc, DM9000_NCR) & DM9000_NCR_RST );

	/* (5a) PHY Configuration */
	/* Setup PHY auto-negotiation capabilities */
	mii_write(sc, sc->phy, DM9000_PHY_ANAR,
		  DM9000_PHY_ANAR_10_HDX | DM9000_PHY_ANAR_10_FDX |
		  DM9000_PHY_ANAR_TX_HDX | DM9000_PHY_ANAR_TX_FDX);

	/* Ask PHY to start auto-negotiation */
	mii_write(sc, sc->phy, DM9000_PHY_BMCR, DM9000_PHY_BMCR_AUTO_NEG_EN |
		  DM9000_PHY_BMCR_RESTART_AN);

	/* (5b) PHY Enable */
	var = CSR_READ_1(sc, DM9000_GPR);
	CSR_WRITE_1(sc, DM9000_GPR, var & ~DM9000_GPR_PHY_PWROFF );
	var = CSR_READ_1(sc, DM9000_GPCR);
	CSR_WRITE_1(sc, DM9000_GPCR, var | DM9000_GPCR_GPIO0_OUT );

	/* (6) Software Reset */
	CSR_WRITE_1(sc, DM9000_NCR, DM9000_NCR_RST |
		    DM9000_NCR_LBK_MAC_INTERNAL);
	while( CSR_READ_1(sc, DM9000_NCR) & DM9000_NCR_RST );

	/* (7) Setup Registers: Remainder of this function */

	/* Select internal PHY, no wakeup event, no collosion mode, normal
	   loopback mode, and full duplex mode (for external PHY only) */
	var = DM9000_NCR_LBK_NORMAL;
	if (sc->phy != 0x01) {
		var = DM9000_NCR_EXT_PHY;
	}
	CSR_WRITE_1(sc, DM9000_NCR, var);

	/* Will clear TX1END, TX2END, and WAKEST fields by reading DM9000_NSR
	 */
	CSR_READ_1(sc, DM9000_NSR);

	/* Enable wraparound of read/write pointer. */
	CSR_WRITE_1(sc, DM9000_IMR, DM9000_IMR_PAR);

	/* Enable RX without watchdog */
	CSR_WRITE_1(sc, DM9000_RCR, DM9000_RCR_RXEN | DM9000_RCR_WTDIS);

	start = getsecs();
	/* Wait for auto-negotiation to complete.
	   Currently there is no timeout on this due to the fact that we
	   have no way to signal an error -- and therefore the caller
	   cannot retry.
	 */
	do {
		if (getsecs() - start > 10) {
			printf("No active link, check cable connection.\n");
			start = getsecs();
		}
		/* We have no delay() function at this point, just busy-wait.*/
	} while( !(mii_read(sc, sc->phy, DM9000_PHY_BMSR) &
		   DM9000_PHY_BMSR_AUTO_NEG_COM));

	return sc;
}

int
dm9k_recv(void *priv, char *pkt, unsigned int len, unsigned int timo)
{
	struct dm9k_sc *sc = priv;
	uint8_t buf;
	uint8_t rx_status;
	uint16_t data;
	uint16_t frame_length;
	time_t start_time;
	int i;
	int trailing_byte;

	errno = 0;
	trailing_byte = 0;

	start_time = getsecs();

	CSR_READ_2(sc, DM9000_MRCMDX);

	/* Wait until data is available */
	do {
		buf = *(volatile uint8_t*)(sc->csr+4);
	} while ( (buf == 0x00) && (getsecs()-start_time <= timo*10));

	if (buf == 0x00) {
		/* Timeout */
		errno = ETIMEDOUT;
		return -1;
	}

	if (buf != 0x01) {
		panic("DM9000 needs to be reset, but this is not implemented yet!");
	}

	/* Get status */
	rx_status = CSR_READ_1(sc, DM9000_MRCMD);

	/* Frame has 4 CRC bytes at the end, that we do not deliver to the
	   upper layer */
	frame_length = *(volatile uint16_t*)(sc->csr+4);
	if (frame_length-4 > len) {
		errno = ENOBUFS;
		printf("Received frame is too big for given buffer, ignoring data...\n");
		printf("Expected maximum of %d bytes, but got %d bytes\n", len, frame_length-4);
	}
	len = frame_length - 4;

	if (len & 0x01) {
		trailing_byte = 1;
		len--;
	}

	/* DM9000 is runing in 16-bit mode, transfer two bytes at a time */
	for (i = 0; i<len; i+=2) {
		data = *(volatile uint16_t*)(sc->csr+4);
		if (errno == 0) {
			pkt[i] = data & 0xFF;
			pkt[i+1] = (data >> 8) & 0xFF;
		}
	}

	if (trailing_byte) {
		len++;
		data = *(volatile uint16_t*)(sc->csr+4);
		if (errno == 0) {
			pkt[i] = data & 0xFF;
		}
	}

	/* Read out the remaining part of the received frame, which
	 * under normal circumstances are the 4 CRC bytes. */
	for (i = 0; i<(frame_length-len); i+=2) {
		data = *(volatile uint16_t*)(sc->csr+4);
	}

	if (errno)
		return -1;

	if (rx_status & (DM9000_RSR_CE | DM9000_RSR_PLE)) {
		printf("Read error: %x\n", rx_status);
		errno = EIO;
		return -1;
	}

	return len;
}

int
dm9k_send(void *priv, char *pkt, unsigned int len)
{
	struct dm9k_sc *sc = priv;
	int i;
	int finish_bit;
	unsigned int cnt;
	unsigned int bound;
	uint16_t data;

	cnt = len;
	if (len & 0x01)
		cnt--;

	CSR_READ_1(sc, DM9000_MWCMD);

	for(i = 0; i < cnt; i+=2) {
		data = pkt[i] | (pkt[i+1] << 8);
		*(volatile uint16_t*)(sc->csr+4) = data;
	}

	if (len & 0x01) {
		/* Copy the remaining, last, byte  */
		data = pkt[len-1];
		*(volatile uint16_t*)(sc->csr+4) = data;
	}

	CSR_WRITE_1(sc, DM9000_TXPLL, len & 0xFF);
	CSR_WRITE_1(sc, DM9000_TXPLH, len >> 8);

	CSR_WRITE_1(sc, DM9000_TCR, DM9000_TCR_TXREQ);

	finish_bit = (sc->tx) ? DM9000_NSR_TX2END : DM9000_NSR_TX1END;
	bound = getsecs() + 1;
	do {
		if (CSR_READ_1(sc, DM9000_NSR) & finish_bit) {
			goto success;
		}
	} while(getsecs() < bound);
	printf("Transmit timeout\n");
	return -1;

 success:
	sc->tx ^= 1;

	return len;
}

u_int16_t
mii_read(struct dm9k_sc *sc, int phy, int reg)
{
	u_int16_t val;
	/* Select Register to read*/
	CSR_WRITE_1(sc,
		    DM9000_EPAR, DM9000_EPAR_PHY(phy) |
		    (reg & DM9000_EPAR_EROA_MASK));
	/* Select read operation (DM9000_EPCR_ERPRR) from the PHY */
	CSR_WRITE_1(sc, DM9000_EPCR, DM9000_EPCR_ERPRR | DM9000_EPCR_EPOS_PHY);

	/* Wait until access to PHY has completed */
	while(CSR_READ_1(sc, DM9000_EPCR) & DM9000_EPCR_ERRE);

	/* Reset ERPRR-bit */
	CSR_WRITE_1(sc, DM9000_EPCR, DM9000_EPCR_EPOS_PHY);

	val = CSR_READ_1(sc, DM9000_EPDRL);
	val |= CSR_READ_1(sc, DM9000_EPDRH) << 8;

	return val;
}

void
mii_write(struct dm9k_sc *sc, int phy, int reg, u_int16_t value)
{
	/* Select Register to write*/
	CSR_WRITE_1(sc, DM9000_EPAR,
		    DM9000_EPAR_PHY(phy) | (reg & DM9000_EPAR_EROA_MASK));

	/* Write data to the two data registers */
	CSR_WRITE_1(sc, DM9000_EPDRL, value & 0xFF);
	CSR_WRITE_1(sc, DM9000_EPDRH, (value >> 8) & 0xFF);

	/* Select write operation (DM9000_EPCR_ERPRW) from the PHY */
	CSR_WRITE_1(sc, DM9000_EPCR, DM9000_EPCR_ERPRW + DM9000_EPCR_EPOS_PHY);

	/* Wait until access to PHY has completed */
	while(CSR_READ_1(sc, DM9000_EPCR) & DM9000_EPCR_ERRE);

	/* Reset ERPRR-bit */
	CSR_WRITE_1(sc, DM9000_EPCR, DM9000_EPCR_EPOS_PHY);
}
