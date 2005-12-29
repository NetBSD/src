/*	$NetBSD: cons_zs.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

/*-
 * Copyright (c) 2005 Izumi Tsutsui.
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <dev/ic/z8530reg.h>

#include <machine/sbd.h>

#include "console.h"


struct zs zs;

static void zs_init(void);
static int zs_cngetc(void);
static int zs_cnscan(void);
static void zs_cnputc(int, int, int);

#define	ZS_CONSDEFSPEED	9600

void
zs_set_addr(uint32_t csr, uint32_t data, int clock)
{

	zs.csr  = (volatile uint8_t *)csr;
	zs.data = (volatile uint8_t *)data;
	zs.clock = clock;

	cons.init = zs_init;
	cons.getc = zs_cngetc;
	cons.scan = zs_cnscan;
	cons.putc = zs_cnputc;
}

#define	ZS_WRITE_REG(zs, reg, val)	\
	do {				\
		*zs.csr = reg;		\
		*zs.csr = val;		\
	} while (/* CONSTCOND */ 0)

static void
zs_init(void)
{

	ZS_WRITE_REG(zs,  9, 0);
	ZS_WRITE_REG(zs,  9, ZSWR9_HARD_RESET);

	ZS_WRITE_REG(zs,  4, ZSWR4_CLK_X16 | ZSWR4_ONESB | ZSWR4_EVENP);
	ZS_WRITE_REG(zs, 10, 0x00);
	ZS_WRITE_REG(zs,  3, ZSWR3_RX_8);
	ZS_WRITE_REG(zs,  5, ZSWR5_TX_8 | ZSWR5_DTR | ZSWR5_RTS);

	ZS_WRITE_REG(zs,  6, 0x00);
	ZS_WRITE_REG(zs,  7, 0x00);

	ZS_WRITE_REG(zs, 14, ZSWR14_BAUD_FROM_PCLK);
	ZS_WRITE_REG(zs, 11, ZSWR11_RXCLK_BAUD | ZSWR11_TXCLK_BAUD);
	ZS_WRITE_REG(zs, 12, BPS_TO_TCONST(zs.clock / 16, ZS_CONSDEFSPEED));
	ZS_WRITE_REG(zs, 13, 0);

	ZS_WRITE_REG(zs, 14, ZSWR14_BAUD_FROM_PCLK | ZSWR14_BAUD_ENA);
	ZS_WRITE_REG(zs, 15, 0x00);

	*zs.csr = ZSWR0_RESET_STATUS;
	*zs.csr = ZSWR0_RESET_STATUS;

	ZS_WRITE_REG(zs,  3, ZSWR3_RX_8 | ZSWR3_RX_ENABLE);
	ZS_WRITE_REG(zs,  5,
	    ZSWR5_TX_8 | ZSWR5_DTR | ZSWR5_RTS | ZSWR5_TX_ENABLE);
}

static int
zs_cngetc(void)
{
	int csr, data;

	do {
		csr = *zs.csr;
	} while ((csr & ZSRR0_RX_READY) == 0);

	data = *zs.data;

	return data;
}

int
zs_cnscan(void)
{
	int csr, data;

	csr = *zs.csr;
	if ((csr & ZSRR0_RX_READY) == 0)
		return -1;

	data = *zs.data;

	return data;
}

static void
zs_cnputc(int x, int y, int c)
{
	int csr;

	do {
		csr = *zs.csr;
	} while ((csr & ZSRR0_TX_READY) == 0);

	*zs.data = c;
}
