/*	$NetBSD: zs.c,v 1.2.6.1 2008/05/18 12:31:45 yamt Exp $	*/

/*-
 * Copyright (c) 2008 Izumi Tsutsui. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef CONS_ZS
/*
 * optional Z85C30 serial support for Qube 2700
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/ic/z8530reg.h>

#include <machine/cpu.h>

#include "boot.h"
#include "zs.h"

#define ZSCLOCK		11059200	/* 19200 * 576 */

#define ZS_DELAY()	delay(2)

static uint8_t zs_read(void *, uint8_t);
static void zs_write(void *, uint8_t, uint8_t);
static void zs_write_reg(void *, uint8_t, uint8_t);
static void zs_reset(void *);

static uint8_t
zs_read(void *dev, uint8_t reg)
{
	volatile uint8_t *zs = dev;
	uint8_t val;

	val = *(volatile uint8_t *)(zs + reg);
	ZS_DELAY();

	return val;
}

static void
zs_write(void *dev, uint8_t reg, uint8_t val)
{
	volatile uint8_t *zs = dev;

        *(volatile uint8_t *)(zs + reg) = val;
	ZS_DELAY();
}

static void
zs_write_reg(void *dev, uint8_t reg, uint8_t val)
{

	zs_write(dev, ZS_CSR, reg);
	zs_write(dev, ZS_CSR, val);
}

static void
zs_reset(void *dev)
{

	/* clear errors */
	zs_write_reg(dev,  9, 0);
	/* hardware reset */
	zs_write_reg(dev,  9, ZSWR9_HARD_RESET);
	delay(1000);

	/* disable all inerttupts */
	zs_write_reg(dev,  1, 0);

	/* set TX/RX misc parameters and modes */
	zs_write_reg(dev,  4, ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP);
	zs_write_reg(dev, 10, ZSWR10_NRZ);
	zs_write_reg(dev,  3, ZSWR3_RX_8);
	zs_write_reg(dev,  5, ZSWR5_TX_8 | ZSWR5_DTR | ZSWR5_RTS);

	/* sync registers unused */
	zs_write_reg(dev,  6, 0);
	zs_write_reg(dev,  7, 0);

	/* set baud rate generator mode */
	zs_write_reg(dev, 14, ZSWR14_BAUD_FROM_PCLK);
	/* set clock mode */
	zs_write_reg(dev, 11, ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD);
	/* set baud rate constant */
	zs_write_reg(dev, 12, BPS_TO_TCONST(ZSCLOCK / 16, ZSSPEED));
	zs_write_reg(dev, 13, 0);

	/* enable baud rate generator */
	zs_write_reg(dev, 14, ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA);
	/* disable all external interrupts */
	zs_write_reg(dev, 15, 0);

	/* reset external status twice (see src/sys/dev/ic/z8530sc.c) */
	zs_write(dev, ZS_CSR, ZSWR0_RESET_STATUS);
	zs_write(dev, ZS_CSR, ZSWR0_RESET_STATUS);

	/* enable TX and RX */
	zs_write_reg(dev,  3, ZSWR3_RX_8 | ZSWR3_RX_ENABLE);
	zs_write_reg(dev,  5,
	    ZSWR5_TX_8 | ZSWR5_DTR | ZSWR5_RTS | ZSWR5_TX_ENABLE);
}

void *
zs_init(int addr, int speed)
{
	void *zs;

	zs = (void *)MIPS_PHYS_TO_KSEG1(ZS_BASE + addr);
	zs_reset(zs);

	return zs;
}

void
zs_putc(void *dev, int c)
{
	uint8_t csr;

	do {
		csr = zs_read(dev, ZS_CSR);
	} while ((csr & ZSRR0_TX_READY) == 0);

	zs_write(dev, ZS_DATA, c);
}

int
zs_getc(void *dev)
{
	uint8_t csr, data;

	do {
		csr = zs_read(dev, ZS_CSR);
	} while ((csr & ZSRR0_RX_READY) == 0);

	data = zs_read(dev, ZS_DATA);
	return data;
}

int
zs_scan(void *dev)
{
	uint8_t csr, data;

	csr = zs_read(dev, ZS_CSR);
	if ((csr & ZSRR0_RX_READY) == 0)
		return -1;

	data = zs_read(dev, ZS_DATA);
	return data;
}
#endif /* CONS_ZS */
