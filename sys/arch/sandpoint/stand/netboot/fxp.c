/* $NetBSD: fxp.c,v 1.1.2.1 2007/06/02 08:44:37 nisimura Exp $ */

/*
 * most of the following code was imported from dev/ic/i82557.c; the
 * original copyright notice is as below.
 */

/*-
 * Copyright (c) 1997, 1998, 1999, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1995, David Greenman
 * Copyright (c) 2001 Jonathan Lemon <jlemon@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	Id: if_fxp.c,v 1.113 2001/05/17 23:50:24 jlemon
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

#include <dev/ic/i82557reg.h>

#include "globals.h"

#define FRAMESIZE 1536

/*
 * 82559ER 8086.1209/1229
 *
 * - reverse endian access for 16bit/32bit register.
 * - no vtophys() translation, vaddr_t == paddr_t. 
 * - PIPT writeback cache aware.
 */
#define CSR_WRITE_1(l, r, v)	*(volatile uint8_t *)((l)->iobase+(r)) = (v)
#define CSR_READ_1(l, r)	*(volatile uint8_t *)((l)->iobase+(r))
#define CSR_WRITE_2(l, r, v)	out16rb((l)->iobase+(r), (v))
#define CSR_READ_2(l, r)	in16rb((l)->iobase+(r))
#define CSR_WRITE_4(l, r, v) 	out32rb((l)->iobase+(r), (v))
#define CSR_READ_4(l, r)	in32rb((l)->iobase+(r))
#define VTOPHYS(va) 		(uint32_t)(va)
#define wb(adr, siz)		_wb(VTOPHYS(adr), (uint32_t)(siz))
#define wbinv(adr, siz)		_wbinv(VTOPHYS(adr), (uint32_t)(siz))
#define inv(adr, siz)		_inv(VTOPHYS(adr), (uint32_t)(siz))
#define DELAY(n)		delay(n)

struct txdesc {
	volatile uint16_t cb_status;
	volatile uint16_t cb_command; 
	volatile uint32_t link_addr;
	volatile uint32_t tbd_array_addr;
	volatile uint16_t byte_count;
	volatile uint8_t tx_threshold;
	volatile uint8_t tbd_number;
	volatile uint32_t tx_buf_addr0;
	volatile uint32_t tx_buf_size0;
	volatile uint32_t tx_buf_addr1;
	volatile uint32_t tx_buf_size1;
}; /* mimic extended TxCB layout */

struct rxdesc {
	volatile uint16_t rfa_status;
	volatile uint16_t rfa_control;
	volatile uint32_t link_addr; 
	volatile uint32_t rbd_addr;
	volatile uint16_t actual_size; 
	volatile uint16_t size;
}; /* 16B rfa */

struct local {
	struct txdesc TxD;
	struct rxdesc *RxD;
	uint8_t store[sizeof(struct rxdesc) + FRAMESIZE];
	unsigned iobase;
	unsigned eeprom_addr;
};

void *fxp_init(void *);
int fxp_send(void *, char *, unsigned);
int fxp_recv(void *, char *, unsigned, unsigned);
static void autosize_eeprom(struct local *);
static int read_eeprom(struct local *, int);
static void fxp_scb_wait(struct local *);
static int fxp_mdi_read(struct local *, int, int);

/*
 * Template for default configuration parameters.
 * See struct fxp_cb_config for the bit definitions.
 */
static uint8_t fxp_cb_config_template[] = {
	0x0, 0x0,		/* cb_status */
	0x80, 0x2,		/* cb_command */
	0xff, 0xff, 0xff, 0xff, /* link_addr */
	0x16,	/*  0 */
	0x8,	/*  1 */
	0x0,	/*  2 */
	0x0,	/*  3 */
	0x0,	/*  4 */
	0x80,	/*  5 */
	0xb2,	/*  6 */
	0x3,	/*  7 */
	0x1,	/*  8 */
	0x0,	/*  9 */
	0x26,	/* 10 */
	0x0,	/* 11 */
	0x60,	/* 12 */
	0x0,	/* 13 */
	0xf2,	/* 14 */
	0x48,	/* 15 */
	0x0,	/* 16 */
	0x40,	/* 17 */
	0xf3,	/* 18 */
	0x0,	/* 19 */
	0x3f,	/* 20 */
	0x5	/* 21 */
};

static struct fxp_cb_config store_cbc;
static struct fxp_cb_ias store_cbi;

void *
fxp_init(void *cookie)
{
	struct local *sc;
	uint8_t *en = cookie;
	struct fxp_cb_config *cbp = &store_cbc;
	struct fxp_cb_ias *cb_ias = &store_cbi;
	struct rxdesc *rfa;
	unsigned tag, v, i;

	if (pcifinddev(0x8086, 0x1209, &tag) != 0
	    && pcifinddev(0x8086, 0x1229, &tag) != 0) {
		printf("fxp NIC not found\n");
		return NULL;
	}

	sc = alloc(sizeof(struct local));
	memset(sc, 0, sizeof(struct local));
	sc->iobase = pcicfgread(tag, 0x10);

	CSR_WRITE_4(sc, FXP_CSR_PORT, FXP_PORT_SELECTIVE_RESET);
	DELAY(100);	

	autosize_eeprom(sc);
	v = read_eeprom(sc, 0); en[0] = v; en[1] = v >> 8;
	v = read_eeprom(sc, 1); en[2] = v; en[3] = v >> 8;
	v = read_eeprom(sc, 2); en[4] = v; en[5] = v >> 8;

	printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x, ",
		en[0], en[1], en[2], en[3], en[4], en[5]);

	printf("PHY %d (%04x.%04x)\n", fxp_mdi_read(sc, 1, 18),
	   fxp_mdi_read(sc, 1, 2), fxp_mdi_read(sc, 1, 3));

	/* 
	 * Initialize base of CBL and RFA memory. Loading with zero
	 * sets it up for regular linear addressing.
	 */
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, 0);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_BASE);

	fxp_scb_wait(sc);
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_RU_BASE);

	/*
	 * This memcpy is kind of disgusting, but there are a bunch of must be
	 * zero and must be one bits in this structure and this is the easiest
	 * way to initialize them all to proper values.
	 */
	memcpy(cbp, fxp_cb_config_template, sizeof(fxp_cb_config_template));

#define prm 0
#define phy_10Mbps_only 0
#define all_mcasts 0
	cbp->cb_status =	0;
	cbp->cb_command =	htole16(FXP_CB_COMMAND_CONFIG |
				    FXP_CB_COMMAND_EL);
	cbp->link_addr =	-1;	/* (no) next command */
	cbp->byte_count =	22;	/* (22) bytes to config */
	cbp->rx_fifo_limit =	8;	/* rx fifo threshold (32 bytes) */
	cbp->tx_fifo_limit =	0;	/* tx fifo threshold (0 bytes) */
	cbp->adaptive_ifs =	0;	/* (no) adaptive interframe spacing */
	cbp->rx_dma_bytecount = 0;	/* (no) rx DMA max */
	cbp->tx_dma_bytecount = 0;	/* (no) tx DMA max */
	cbp->dma_mbce =		0;	/* (disable) dma max counters */
	cbp->late_scb =		0;	/* (don't) defer SCB update */
	cbp->tno_int_or_tco_en = 0;	/* (disable) tx not okay interrupt */
	cbp->ci_int =		0;	/* interrupt on CU not active */
	cbp->save_bf =		prm;	/* save bad frames */
	cbp->disc_short_rx =	!prm;	/* discard short packets */
	cbp->underrun_retry =	1;	/* retry mode (1) on DMA underrun */
	cbp->mediatype =	!phy_10Mbps_only; /* interface mode */
	cbp->nsai =		1;     /* (don't) disable source addr insert */
	cbp->preamble_length =	2;	/* (7 byte) preamble */
	cbp->loopback =		0;	/* (don't) loopback */
	cbp->linear_priority =	0;	/* (normal CSMA/CD operation) */
	cbp->linear_pri_mode =	0;	/* (wait after xmit only) */
	cbp->interfrm_spacing = 6;	/* (96 bits of) interframe spacing */
	cbp->promiscuous =	prm;	/* promiscuous mode */
	cbp->bcast_disable =	0;	/* (don't) disable broadcasts */
	cbp->crscdt =		0;	/* (CRS only) */
	cbp->stripping =	!prm;	/* truncate rx packet to byte count */
	cbp->padding =		1;	/* (do) pad short tx packets */
	cbp->rcv_crc_xfer =	0;	/* (don't) xfer CRC to host */
	cbp->force_fdx =	0;	/* (don't) force full duplex */
	cbp->fdx_pin_en =	1;	/* (enable) FDX# pin */
	cbp->multi_ia =		0;	/* (don't) accept multiple IAs */
	cbp->mc_all =		all_mcasts;/* accept all multicasts */
#undef prm
#undef phy_10Mbps_only
#undef all_mcasts

	wbinv(cbp, sizeof(*cbp));
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, VTOPHYS(cbp));
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);
	i = 1000;
	while (!(le16toh(cbp->cb_status) & FXP_CB_STATUS_C) && --i > 0) {
		DELAY(1);
		inv(&cbp->cb_status, sizeof(cbp->cb_status));
	}
	if (i == 0)
		printf("cbp config timeout\n");

	/*
	 * Initialize the station address.
	 */
	cb_ias->cb_status = 0;
	cb_ias->cb_command = htole16(FXP_CB_COMMAND_IAS | FXP_CB_COMMAND_EL);
	cb_ias->link_addr = -1;
	memcpy(cb_ias->macaddr, en, 6);

	/*
	 * Start the IAS (Individual Address Setup) command/DMA.
	 */
	wbinv(cb_ias, sizeof(*cb_ias));
	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, VTOPHYS(cb_ias));
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);

	i = 1000;
	while (!(le16toh(cb_ias->cb_status) & FXP_CB_STATUS_C) && --i > 0) {
		DELAY(1);
		inv(&cb_ias->cb_status, sizeof(cb_ias->cb_status));
	}
	if (i == 0)
		printf("ias config timeout\n");

	rfa = (struct rxdesc *)sc->store;
	rfa->rfa_status = 0;
	rfa->rfa_control = htole16(FXP_RFA_CONTROL_S);
	rfa->link_addr = htole32(VTOPHYS(rfa));
	rfa->rbd_addr = -1;
	rfa->actual_size = 0;
	rfa->size = htole16(sizeof(sc->store) - sizeof(struct rxdesc));
	wbinv(rfa, sizeof(sc->store));
	sc->RxD = rfa;

	fxp_scb_wait(sc);
	CSR_WRITE_4(sc, FXP_CSR_SCB_GENERAL, VTOPHYS(sc->RxD));
	CSR_WRITE_1(sc, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_RU_START);

	return sc;
}

int
fxp_send(void *dev, char *buf, unsigned len)
{
	struct local *l = dev;
	struct txdesc *TxD;
	int loop;

	if (len > 1520)
		printf("fxp_send: len > 1520 (%u)\n", len);

	TxD = &l->TxD;
	TxD->cb_status = 0;
	TxD->cb_command =
	    htole16(FXP_CB_COMMAND_XMIT|FXP_CB_COMMAND_SF|FXP_CB_COMMAND_EL);
	TxD->link_addr = -1;
	TxD->tbd_array_addr = htole32(VTOPHYS(&TxD->tx_buf_addr0));
	TxD->tx_buf_addr0 = htole32(VTOPHYS(buf));
	TxD->tx_buf_size0 = htole32(len);
	TxD->byte_count = htole16(0x8000);
	TxD->tx_threshold = 0x20;
	TxD->tbd_number = 1;
	wbinv(buf, len);
	wbinv(TxD, sizeof(*TxD));

	fxp_scb_wait(l);
	CSR_WRITE_4(l, FXP_CSR_SCB_GENERAL, VTOPHYS(TxD));
	CSR_WRITE_1(l, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);
	
	loop = 10000;
	while (!(le16toh(TxD->cb_status) & FXP_CB_STATUS_C) && --loop > 0) {
		DELAY(1);
		inv(TxD, sizeof(struct txdesc));
	}
	if (loop == 0)
		printf("send timeout\n");

	return len;
}

int
fxp_recv(void *dev, char *buf, unsigned maxlen, unsigned timo)
{
	struct local *l = dev;
	struct rxdesc *rfa;
	time_t bound;
	unsigned ruscus, len;

	fxp_scb_wait(l);
	CSR_WRITE_1(l, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_RU_RESUME);

	bound = 1000 * timo;
	do {
		ruscus = CSR_READ_1(l, FXP_CSR_SCB_RUSCUS);
		if (((ruscus >> 2) & 0x0f) != FXP_SCB_RUS_READY
		    && (((ruscus >> 2) & 0x0f) == FXP_SCB_RUS_SUSPENDED))
			goto gotone;
		DELAY(1000);	/* 1 milli second */
	} while (bound-- > 0);
	errno = 0;
	return -1;
  gotone:
	rfa = l->RxD;
	inv(rfa, sizeof(l->store)); /* whole including received frame */
	if ((le16toh(rfa->rfa_status) & FXP_RFA_STATUS_C) == 0)
		return 0;
	len = le16toh(rfa->actual_size) & 0x7ff;
	if (len > maxlen)
		len = maxlen;
	memcpy(buf, &l->store[sizeof(struct rxdesc)], len);

	rfa->rfa_status = 0;
	rfa->rfa_control = htole16(FXP_RFA_CONTROL_S);
	rfa->actual_size = 0;
	wbinv(rfa, sizeof(struct rxdesc));
#if 0
	fxp_scb_wait(l);
	CSR_WRITE_1(l, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_RU_RESUME);
#endif
	return len;
}

static void
eeprom_shiftin(struct local *sc, int data, int len)
{
	uint16_t reg;
	int x;

	for (x = 1 << (len - 1); x != 0; x >>= 1) {
		DELAY(40);
		if (data & x)
			reg = FXP_EEPROM_EECS | FXP_EEPROM_EEDI;
		else
			reg = FXP_EEPROM_EECS;
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
		    reg | FXP_EEPROM_EESK);
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
	}
	DELAY(40);
}

void
autosize_eeprom(struct local *sc)
{
	int x;

	CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
	DELAY(40);

	/* Shift in read opcode. */
	eeprom_shiftin(sc, FXP_EEPROM_OPC_READ, 3);

	/*
	 * Shift in address, wait for the dummy zero following a correct
	 * address shift.
	 */
	for (x = 1; x <= 8; x++) {
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
		    FXP_EEPROM_EECS | FXP_EEPROM_EESK);
		DELAY(40);
		if ((CSR_READ_2(sc, FXP_CSR_EEPROMCONTROL) &
		    FXP_EEPROM_EEDO) == 0)
			break;
		DELAY(40);
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);
		DELAY(40);
	}
	CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
	DELAY(40);
	if (x != 6 && x != 8)
		printf("fxp: strange EEPROM address size (%d)\n", x);
	else
		sc->eeprom_addr = x;
}

static int
read_eeprom(struct local *sc, int offset)
{
	uint16_t reg;
	int x, val;

	CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, FXP_EEPROM_EECS);

	/* Shift in read opcode. */
	eeprom_shiftin(sc, FXP_EEPROM_OPC_READ, 3);

	/* Shift in address. */
	eeprom_shiftin(sc, offset, sc->eeprom_addr);

	reg = FXP_EEPROM_EECS;
	val = 0;
	/*
	 * Shift out data.
	 */
	for (x = 16; x > 0; x--) {
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL,
		    reg | FXP_EEPROM_EESK);
		DELAY(1);
		if (CSR_READ_2(sc, FXP_CSR_EEPROMCONTROL) &
		    FXP_EEPROM_EEDO)
			val |= (1 << (x - 1));
		CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, reg);
		DELAY(1);
	}
	CSR_WRITE_2(sc, FXP_CSR_EEPROMCONTROL, 0);
	DELAY(1);

	return val;
}

static void
fxp_scb_wait(struct local *sc)
{
	int loop = 5000;

	while (CSR_READ_1(sc, FXP_CSR_SCB_COMMAND) && --loop > 0)
		DELAY(2);
	if (loop == 0)
		printf("SCB timeout\n");
}

static int
fxp_mdi_read(struct local *sc, int phy, int reg)
{
	int count = 10000;
	int value;

	CSR_WRITE_4(sc, FXP_CSR_MDICONTROL,
	    (FXP_MDI_READ << 26) | (reg << 16) | (phy << 21));

	while (((value = CSR_READ_4(sc, FXP_CSR_MDICONTROL)) &
	    0x10000000) == 0 && count--)
		DELAY(10);

	if (count <= 0)
		printf("fxp_mdi_read: timed out\n");

	return (value & 0xffff);
}
