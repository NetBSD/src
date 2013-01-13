/*	$NetBSD: lance.c,v 1.6 2013/01/13 14:24:24 tsutsui Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

/* LANCE driver for EWS4800/360 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/ic/am7990reg.h>
#include <dev/ic/lancereg.h>

#include "local.h"

/* Register Address Pointer */
#define	LANCE_RAP	((volatile uint16_t *)0xbe400006)
/* Register Data Port */
#define	LANCE_RDP	((volatile uint16_t *)0xbe400000)

#define	RX_DESC_NUM	8
#define	TX_DESC_NUM	8
#define	TX_BUFSIZE	0x1000
#define	RX_BUFSIZE	0x1000
struct {
	struct leinit leinit;
	struct lermd lermd[RX_DESC_NUM];
	struct letmd letmd[TX_DESC_NUM];
	uint8_t eaddr[6];
	uint8_t txdata[TX_BUFSIZE] __attribute__((__aligned__(0x1000)));
	uint8_t rxdata[RX_BUFSIZE] __attribute__((__aligned__(0x1000)));
} lance_mem __attribute__((__aligned__(64)));

bool lance_init(void);
void lance_eaddr(uint8_t *);
bool lance_get(void *, size_t);
bool lance_put(void *, size_t);

void lance_setup(void);
bool lance_set_initblock(struct leinit *);
bool lance_do_initialize(void);

bool lance_test(void);
bool lance_internal_loopback_test(bool);
void lance_internal_loopback_setup(bool);
void lance_internal_loopback_testdata(void);
bool lance_internal_loopback_data_check(bool);
bool __poll_interrupt(void);
bool __poll_lance_c0(uint16_t);

bool
lance_init(void)
{

	lance_setup();

	if (!lance_set_initblock(&lance_mem.leinit))
		return false;

	if (!lance_do_initialize())
		return false;

	*LANCE_RDP = LE_C0_STRT;

	return true;
}

void
lance_eaddr(uint8_t *p)
{
	int i;

	for (i = 0; i < 6; i++)
		p[i] = lance_mem.eaddr[i];
}

bool
lance_get(void *data, size_t len)
{
	static int current;
	struct lermd *rmd;
	int i, j, k, n;
	int start, end;
	uint8_t *q, *p = data, *p_end = p + len;

	while ((*LANCE_RDP & (LE_C0_RINT | LE_C0_INTR)) == 0)
		;
	*LANCE_RDP = LE_C0_RINT;

	start = end = -1;
	n = 0;
	for (i = 0; i < 8; i++) {
		rmd = &lance_mem.lermd[(current + i) & 0x7];
		if (rmd->rmd1_bits & LE_R1_STP)
			start = i;
		if (rmd->rmd1_bits & LE_R1_ENP) {
			end = i;
			n = rmd->rmd3;		/* total amount of packet */
			break;
		}
	}
#ifdef DEBUG
	printf("%s: %d [%d,%d] %d\n", __func__, len, start, end, n);
#endif
	if (start < 0 || end < 0)
		return false;

	for (i = start; i <= end; i++) {
		rmd = &lance_mem.lermd[(current + i) & 0x7];
		q = (uint8_t *)((rmd->rmd1_hadr << 16) | rmd->rmd0 |
		    0xa0000000);
		j = i == end ? n : -rmd->rmd2;
		for (k = 0; k < j; k++)
			if (p < p_end)
				*p++ = *q++;
		n -= j;
		rmd->rmd1_bits = LE_R1_OWN;	/* return to LANCE */
	}
	current = (current + i) & 0x7;

	return true;
}

bool
lance_put(void *data, size_t len)
{
	static int current;
	struct letmd *tmd;
	uint16_t r;
	uint8_t *p, *q = data;
	int i, j, n, start;

	start = current;
	tmd = &lance_mem.letmd[current];
	tmd->tmd1_bits = LE_T1_STP;
	for (i = 0; i < 8; i++) {
		current = (current + 1) & 0x7;
		n = min(len, 512);
		p = (uint8_t *)((tmd->tmd1_hadr << 16) | tmd->tmd0 |
		    0xa0000000);
		for (j = 0; j < n; j++)
			*p++ = *q++;
		len -= n;
#if 1
		tmd->tmd2 = -max(n, 64) | 0xf000;
#else
		tmd->tmd2 = -n | 0xf000;
#endif
		tmd->tmd3 = 0;
		if (len == 0) {
			tmd->tmd1_bits |= LE_T1_ENP;
			break;
		}
		tmd = &lance_mem.letmd[current];
	}

	n = i + 1;

	for (i = 0; i < n; i++) {
		tmd = &lance_mem.letmd[start + i];
		*LANCE_RDP = LE_C0_INEA;
		tmd->tmd1_bits |= LE_T1_OWN;
		j = 0;
		do {
			*LANCE_RAP;
			r = *LANCE_RDP;
			if (r & LE_C0_ERR) {
				printf("Error. CSR0=%x\n", r);
				return false;
			}
			if (j++ > 0xa0000) {
				printf("Timeout CSR0=%x\n", r);
				return false;
			}
		} while ((r & (LE_C0_TINT | LE_C0_INTR)) == 0);

		*LANCE_RDP = LE_C0_TINT;
	}

	for (i = 0; i < n; i++) {
		uint8_t *bits = &lance_mem.letmd[i].tmd1_bits;
		if (*bits & LE_T1_OWN || *bits & LE_T1_ERR) {
			printf("desc%d not transmitted. cause=%x\n", i, *bits);
			return false;
		}
		*bits = 0;
	}

	return true;
}

bool
lance_set_initblock(struct leinit *leinit)
{
	uint16_t test_data[] = { 0xffff, 0xaaaa, 0x5555, 0x0000 };
	uint16_t t;
	uint32_t addr = (uint32_t)leinit;
	int i;

	/* Control and status register */
	for (i = 3; i >= 0; i--) {
		*LANCE_RAP = i;
		if ((*LANCE_RAP & 3) != i)
			goto reg_rw_error;
	}
	*LANCE_RDP = LE_C0_STOP;	/* disable all external activity */
	if (*LANCE_RDP != LE_C0_STOP)
		goto reg_rw_error;

	/* Low address of init block */
	for (i = 0; i < 4; i++) {
		t = test_data[i] & 0xfffe;
		*LANCE_RAP = LE_CSR1;
		*LANCE_RDP = t;
		if (*LANCE_RDP != t)
			goto reg_rw_error;
	}
	*LANCE_RDP = addr & 0xfffe;
#if DEBUG
	printf("initblock low addr=%x\n", *LANCE_RDP);
#endif

	/* High address of init block */
	for (i = 0; i < 4; i++) {
		t = test_data[i] & 0x00ff;
		*LANCE_RAP = LE_CSR2;
		*LANCE_RDP = t;
		if (*LANCE_RDP != t)
			goto reg_rw_error;
	}
	*LANCE_RDP = (addr >> 16) & 0x00ff;
#ifdef DEBUG
	printf("initblock high addr=%x\n", *LANCE_RDP);
#endif

	/* Bus master and control */
	*LANCE_RAP = LE_CSR3;
	*LANCE_RDP = 7;
	if (*LANCE_RDP != 7)
		goto reg_rw_error;

	*LANCE_RAP = LE_CSR3;
	*LANCE_RDP = 0;
	if (*LANCE_RDP != 0)
		goto reg_rw_error;

	*LANCE_RDP = LE_C3_BSWP | LE_C3_BCON;

	return true;

 reg_rw_error:
	printf("LANCE register r/w error.\n");
	return false;
}

bool
lance_do_initialize(void)
{

	/* Initialze LANCE */
	*LANCE_RAP = LE_CSR0;
	*LANCE_RDP = LE_C0_INEA | LE_C0_INIT;

	/* Wait interrupt */
	if (!__poll_interrupt())
		return false;
	*LANCE_RDP = *LANCE_RDP;

	return true;
}

void
lance_setup(void)
{
	struct leinit *init = &lance_mem.leinit;
	struct lermd *lermd = lance_mem.lermd;
	struct letmd *letmd = lance_mem.letmd;
	uint32_t addr;
	uint8_t *eaddr;
	int i;

	memset(&lance_mem, 0, sizeof lance_mem);
	/* Ethernet address from NVSRAM */
	eaddr = lance_mem.eaddr;
	for (i = 0; i < 6; i++)
		eaddr[i] = *(uint8_t *)(0xbe491008 + i * 4);

	/* Init block */
	init->init_mode = 0;
	init->init_padr[0] = (eaddr[1] << 8) | eaddr[0];
	init->init_padr[1] = (eaddr[3] << 8) | eaddr[2];
	init->init_padr[2] = (eaddr[5] << 8) | eaddr[4];
	/* Logical address filter */
	for (i = 0; i < 4; i++)
		init->init_ladrf[i] = 0x0000;

	/* Location of Rx descriptor ring */
	addr = (uint32_t)lermd;
	init->init_rdra = addr & 0xffff;
	init->init_rlen = ((ffs(RX_DESC_NUM) - 1) << 13) |
	    ((addr >> 16) & 0xff);

	/* Location of Tx descriptor ring */
	addr = (uint32_t)letmd;
	init->init_tdra = addr & 0xffff;
	init->init_tlen = ((ffs(RX_DESC_NUM) - 1) << 13) |
	    ((addr >> 16) & 0xff);

	/* Rx descriptor */
	addr = (uint32_t)lance_mem.rxdata;
	for (i = 0; i < RX_DESC_NUM; i++, lermd++) {
		lermd->rmd0 = (addr & 0xffff) +  i * 512; /* data block size */
		lermd->rmd1_hadr = (addr >> 16) & 0xff;
		lermd->rmd1_bits = LE_R1_OWN;
		lermd->rmd2 = -512;
		lermd->rmd3 = 0;
	}

	/* Tx descriptor */
	addr = (uint32_t)lance_mem.txdata;
	for (i = 0; i < TX_DESC_NUM; i++, letmd++) {
		letmd->tmd0 = (addr & 0xffff) + i * 512; /* data block size */
		letmd->tmd1_hadr = (addr >> 16) & 0xff;
		letmd->tmd1_bits = 0;
		letmd->tmd2 = 0;
		letmd->tmd3 = 0;
	}
}

/*
 * Internal loopback test.
 */
bool
lance_test(void)
{

	/* Internal loop back test. (no CRC) */
	if (!lance_internal_loopback_test(false))
		return false;

	/* Internal loop back test. (with CRC) */
	if (!lance_internal_loopback_test(true))
		return false;

	return true;
}

bool
lance_internal_loopback_test(bool crc)
{

	lance_internal_loopback_setup(crc);

	if (!lance_set_initblock(&lance_mem.leinit))
		return false;

	if (!lance_do_initialize())
		return false;

	/* Transmit Start */
	*LANCE_RAP = LE_CSR0;	/* Control and status register */
	*LANCE_RDP = LE_C0_INEA | LE_C0_STRT;

	/* Check trasmited data. */
	return lance_internal_loopback_data_check(crc);
}

void
lance_internal_loopback_setup(bool crc)
{
	struct leinit *init = &lance_mem.leinit;
	struct lermd *lermd = lance_mem.lermd;
	struct letmd *letmd = lance_mem.letmd;
	uint32_t addr;
	int i;

	memset(&lance_mem, 0, sizeof lance_mem);

	/* Init block */
	init->init_mode = LE_C15_INTL | LE_C15_LOOP;
	if (!crc)
		init->init_mode |= LE_C15_DXMTFCS;

	init->init_padr[0] = 0x0000;
	init->init_padr[1] = 0x8400;
	init->init_padr[2] = 0x0000;
	for (i = 0; i < 4; i++)
		init->init_ladrf[i] = 0x0000;

	addr = (uint32_t)lermd;
	init->init_rdra = addr & 0xffff;
	init->init_rlen = (ffs(RX_DESC_NUM) << 13) | ((addr >> 16) & 0xff);
	addr = (uint32_t)letmd;
	init->init_tdra = addr & 0xffff;
	init->init_tlen = (ffs(RX_DESC_NUM) << 13) | ((addr >> 16) & 0xff);

	/* Rx descriptor */
	addr = (uint32_t)lance_mem.rxdata;
	for (i = 0; i < RX_DESC_NUM; i++, lermd++) {
		lermd->rmd0 = (addr & 0xffff) +  i * 64; /* data block size */
		lermd->rmd1_hadr = (addr >> 16) & 0xff;
		lermd->rmd1_bits = LE_R1_OWN;
		lermd->rmd2 = -64;
		lermd->rmd3 = 0;
	}

	/* Tx descriptor */
	addr = (uint32_t)lance_mem.txdata;
	for (i = 0; i < TX_DESC_NUM; i++, letmd++) {
		letmd->tmd0 = (addr & 0xffff) + i * 64;	/* data block size */
		letmd->tmd1_hadr = (addr >> 16) & 0xff;
		letmd->tmd1_bits = LE_T1_STP | LE_T1_ENP;
		if (crc)
			letmd->tmd2 = -28;
		else
			letmd->tmd2 = -32;
		letmd->tmd3 = 0;
	}

	lance_internal_loopback_testdata();
}

void
lance_internal_loopback_testdata(void)
{
	uint16_t test_data[] = {
		0x55aa, 0xff00, 0x0102, 0x0304, 0x0506, 0x0708, 0x0910,
		0x40db, 0xdfcf, /* CRC */
		0x23dc, 0x23dc, 0x1918, 0x1716, 0x1514, 0x1312, 0x1110,
		0x7081, 0x90cb, /* CRC */
		0x6699, 0xaa55, 0x0515, 0x2535, 0x4555, 0x6575,	0x8595,
		0x55f6, 0xa448, /* CRC */
		0x4e4e, 0x5a5a, 0x6969, 0x7878, 0x0f0f,	0x1e1e, 0x2d2d,
		0xa548, 0x7404, /* CRC */
	};
	uint16_t test_header[] = {
		0x0000, 0x0084, 0x0000,	/* dst */
		0x0000, 0x0084, 0x0000,	/* src */
		0x000e
	};
	uint16_t *p = (uint16_t *)lance_mem.txdata;
	int i, j, k;

	for (i = 0; i < 2; i++) {			/* 64byte * 8 */
		uint16_t *r = test_data;
		for (j = 0; j < 4; j++) {		/* 64byte * 4 */
			uint16_t *q = test_header;
			for (k = 0; k < 7; k++)		/* 14byte */
				*p++ = *q++;
			for (k = 0; k < 9; k++)		/* 18byte */
				*p++ = *r++;
			p += 16;			/* 32byte skip */
		}
	}
}

bool
lance_internal_loopback_data_check(bool crc_check)
{
	uint32_t *p = (uint32_t *)lance_mem.txdata;
	uint32_t *q = (uint32_t *)lance_mem.rxdata;
	int i, j;

	/* Read all data block */
	for (i = 0; i < 8; i++) {
		printf("block %d ", i);
		lance_mem.letmd[i].tmd1_bits |= LE_T1_OWN;/* buffer is filled */
		/* wait interrupt */
		if (!__poll_interrupt())
			goto timeout_error;
		/* wait LANCE status */
		if (!__poll_lance_c0(LE_C0_RINT | LE_C0_TINT | LE_C0_INTR |
		    LE_C0_INEA | LE_C0_RXON | LE_C0_TXON | LE_C0_STRT |
		    LE_C0_INIT))
			goto timeout_error;

		/* check Tx descriptor */
		if (lance_mem.letmd[i].tmd1_bits & LE_T1_ERR) {
			printf("tx desc error.\n");
			goto tx_rx_error;
		}

		/* check Rx descriptor */
		if (lance_mem.lermd[i].rmd1_bits & LE_R1_ERR) {
			printf("rx desc error.\n");
			goto tx_rx_error;
		}

		/* Compare transmitted data */
		for (j = 0; j < 7; j++)	/* first 28byte */
			if (*p++ != *q++) {
				printf("data error.\n");
				goto tx_rx_error;
			}

		/* check CRC */
		if (crc_check) {
			printf("CRC=%x ", *p);
			if (*p != *q) {	/* CRC */
				goto crc_error;
			}
		}
		printf("ok.\n");

		p += 9;	/* 36byte skip */
		q += 9;
	}
	return true;
 timeout_error:
	printf("LANCE timeout.\n");
	return false;
 tx_rx_error:
	printf("LANCE Tx/Rx data error.\n");
	return false;
 crc_error:
	printf("LANCE CRC error.\n");
	return false;
}

bool
__poll_interrupt(void)
{
	int j;

	for (j = 0; j < 0x10000; j++) {
		*LANCE_RAP;
		if (*(volatile uint32_t *)0xbe40a008 & 1)
			break;
	}
	if (j == 0x10000) {
		printf ("interrupt timeout.\n");
		return false;
	}

	return true;
}

bool
__poll_lance_c0(uint16_t r)
{
	int j;

	for (j = 0; j < 0x60000; j++)
		if (*LANCE_RDP == r)
			break;
	if (j == 0x60000) {
		printf("lance CSR0 %x != %x\n", *LANCE_RDP, r);
		return false;
	}

	*LANCE_RDP = (LE_C0_RINT | LE_C0_TINT| LE_C0_INEA) & r;

	return true;
}
