/*	$NetBSD: iris_zs.c,v 1.1 2019/01/12 16:44:47 tsutsui Exp $	*/

/*
 * Copyright (c) 2018 Naruaki Etomi
 * All rights reserved.
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

/*-
 * Copyright (c) 1996, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gordon W. Ross and Wayne Knowles
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

/*
 * Silicon Graphics "IRIS" series MIPS processors machine bootloader.
 * Zilog Z8530 Dual UART driver.
 * Most of the following was adapted from /sys/arch/sgimips/dev/zs.c.
 *	NetBSD: zs.c,v 1.39 2015/02/18 16:47:58 macallan Exp
 */

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/ic/z8530reg.h>

#include <mips/cpuregs.h>
#include <machine/cpu.h>

#include "iris_machdep.h"
#include "iris_zs.h"

#define ZSCLOCK		3672000	 /* PCLK pin input clock rate */
#define ZS_DELAY()	DELAY(3)
#define ZS_DEFSPEED	9600

static void zs_write(void *, uint8_t);
static void zs_write_reg(void *, uint8_t, uint8_t);
static void zs_reset(void *);
static struct zschan *zs_get_chan_addr(int zs_unit, int channel);
int	zs_getc(void *);
void	zs_putc(void *, int);
int	zs_scan(void *);

static int cons_port;

static void
zs_write(void *dev, uint8_t val)
{
	struct zschan *zc = dev;

	zc->zc_csr = val;
	ZS_DELAY();
}

static void
zs_write_reg(void *dev, uint8_t reg, uint8_t val)
{

	zs_write(dev, reg);
	zs_write(dev, val);
}

static void
zs_reset(void *dev)
{

	/* clear errors */
	zs_write_reg(dev,  9, 0);
	/* hardware reset */
	zs_write_reg(dev,  9, ZSWR9_HARD_RESET);
	DELAY(1000);

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

	/* set clock mode */
	zs_write_reg(dev, 11,
	    ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD | ZSWR11_TRXC_OUT_ENA);

	/* set baud rate constant */
	zs_write_reg(dev, 12, BPS_TO_TCONST(ZSCLOCK / 16, ZS_DEFSPEED));
	zs_write_reg(dev, 13, 0);

	/* enable baud rate generator */
	zs_write_reg(dev, 14, ZSWR14_BAUD_ENA);

	/* disable all external interrupts */
	zs_write_reg(dev, 15, 0);

	/* reset external status twice (see src/sys/dev/ic/z8530sc.c) */
	zs_write(dev, ZSWR0_RESET_STATUS);
	zs_write(dev, ZSWR0_RESET_STATUS);

	/* enable TX and RX */
	zs_write_reg(dev,  3, ZSWR3_RX_8 | ZSWR3_RX_ENABLE);
	zs_write_reg(dev,  5,
	    ZSWR5_TX_8 | ZSWR5_DTR | ZSWR5_RTS | ZSWR5_TX_ENABLE);
}

static struct zschan *
zs_get_chan_addr(int zs_unit, int channel)
{
	struct zsdevice *addr;
	struct zschan *zc;

	addr = (struct zsdevice *)MIPS_PHYS_TO_KSEG1(ZS_ADDR);

	zc = &addr->zs_chan_b;

	return zc;
}

void *
zs_init(int addr, int speed)
{
	struct zschan *zs;
	cons_port = 0;

	zs = zs_get_chan_addr(1, cons_port);

	zs_reset(zs);

	return zs;
}

void
zscnputc(void *dev, int c)
{
	struct zschan *zs;

	zs = zs_get_chan_addr(1, cons_port);

	zs_putc(zs, c);
}

void
zs_putc(void *arg, int c)
{
	register volatile struct zschan *zc = arg;
	register int rr0;

	/* Wait for transmitter to become ready. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_TX_READY) == 0);

	zc->zc_data = c;
	ZS_DELAY();
}

int
zscngetc(void *dev)
{
	struct zschan *zs;

	zs = zs_get_chan_addr(1, cons_port);

	return zs_getc(zs);
}

int
zs_getc(void *arg)
{
	struct zschan *zc = arg;
	int c, rr0;

	/* Wait for a character to arrive. */
	do {
		rr0 = zc->zc_csr;
		ZS_DELAY();
	} while ((rr0 & ZSRR0_RX_READY) == 0);

	c = zc->zc_data;
	ZS_DELAY();

	return c;
}

int
zscnscanc(void *dev)
{
	struct zschan *zs;

	zs = zs_get_chan_addr(1, cons_port);

	return zs_scan(zs);
}

int
zs_scan(void *arg)
{
	struct zschan *zc = arg;
	int c, rr0;

	/* Wait for a character to arrive. */
	rr0 = zc->zc_csr;
	ZS_DELAY();

	if ((rr0 & ZSRR0_RX_READY) == 0) {
		return -1;
	}

	c = zc->zc_data;
	ZS_DELAY();

	return c;
}
