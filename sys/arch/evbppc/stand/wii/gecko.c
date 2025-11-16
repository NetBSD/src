/* $NetBSD: gecko.c,v 1.1 2025/11/16 20:11:47 jmcneill Exp $ */

/*-
 * Copyright (c) 2025 Jared McNeill <jmcneill@invisible.ca>
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

#include <lib/libsa/stand.h>

#include <machine/pio.h>

#include "gecko.h"
#include "timer.h"

#define EXI_BASE		0x0d806800
#define	EXI_CSR(n)		(EXI_BASE + 0x00 + (n) * 0x14)
#define  EXI_CSR_EXTINT		__BIT(11)
#define  EXI_CSR_EXTINTMASK	__BIT(10)
#define	 EXI_CSR_CS		__BITS(9,7)
#define	 EXI_CSR_CLK		__BITS(6,4)
#define	EXI_CR(n)		(EXI_BASE + 0x0c + (n) * 0x14)
#define	 EXI_CR_TLEN		__BITS(5,4)
#define  EXI_CR_RW		__BITS(3,2)
#define  EXI_CR_RW_READ		__SHIFTIN(0, EXI_CR_RW)
#define  EXI_CR_RW_WRITE	__SHIFTIN(1, EXI_CR_RW)
#define  EXI_CR_RW_READWRITE	__SHIFTIN(2, EXI_CR_RW)
#define	 EXI_CR_DMA		__BIT(1)
#define  EXI_CR_TSTART		__BIT(0)
#define	EXI_DATA(n)		(EXI_BASE + 0x10 + (n) * 0x14)

#define SI_BASE			0x0d806400
#define SI_EXILK		(SI_BASE + 0x03c)

#define	EXI_FREQ_32MHZ		5

#define GECKO_CSR					\
	(__SHIFTIN(1, EXI_CSR_CS) |			\
	 __SHIFTIN(EXI_FREQ_32MHZ, EXI_CSR_CLK))
#define GECKO_CR(len, rdwr)				\
	(__SHIFTIN((len) - 1, EXI_CR_TLEN) | 		\
	 (rdwr) | EXI_CR_TSTART)

#define GECKO_CMD_ID		0x9000
#define GECKO_CMD_RECV_BYTE	0xa000
#define GECKO_CMD_SEND_BYTE	0xb000

static int gecko_chan = -1;
static int gecko_keycurrent;
static int gecko_keypending;

static void
gecko_wait(void)
{
	int retry;

	for (retry = 0; retry < 1000; retry++) {
		if ((in32(EXI_CR(gecko_chan)) & EXI_CR_TSTART) == 0) {
			return;
		}
	}
}

static uint16_t
gecko_command(uint16_t command)
{
	uint16_t value;

	out32(EXI_CSR(gecko_chan), GECKO_CSR);
	out32(EXI_DATA(gecko_chan), (uint32_t)command << 16);
	out32(EXI_CR(gecko_chan), GECKO_CR(2, EXI_CR_RW_READWRITE));
	gecko_wait();
	value = in32(EXI_DATA(gecko_chan)) >> 16;
	out32(EXI_CSR(gecko_chan), 0);

	return value;
}

static uint32_t
gecko_id(void)
{
	uint32_t value;

	out32(EXI_CSR(gecko_chan), GECKO_CSR);
	out32(EXI_DATA(gecko_chan), 0);
	out32(EXI_CR(gecko_chan), GECKO_CR(2, EXI_CR_RW_READWRITE));
	gecko_wait();
	out32(EXI_CR(gecko_chan), GECKO_CR(4, EXI_CR_RW_READWRITE));
	gecko_wait();
	value = in32(EXI_DATA(gecko_chan));
	out32(EXI_CSR(gecko_chan), 0);

	return value;
}

int
gecko_getchar(void)
{
	int key;

	if (gecko_chan == -1) {
		return -1;
	}

	if (gecko_keypending) {
		key = gecko_keycurrent;
		gecko_keypending = 0;
	} else {
		uint16_t value;

		do {
			value = gecko_command(GECKO_CMD_RECV_BYTE);
		} while ((value & 0x0800) == 0);
		key = value & 0xff;
	}

	return key;
}

void
gecko_putchar(int c)
{
	if (gecko_chan == -1) {
		return;
	}

	c &= 0xff;
	gecko_command(GECKO_CMD_SEND_BYTE | (c << 4));
}

int
gecko_ischar(void)
{
	if (gecko_chan != -1 && !gecko_keypending) {
		uint16_t value;

		value = gecko_command(GECKO_CMD_RECV_BYTE);
		if ((value & 0x0800) != 0) {
			gecko_keycurrent = value & 0xff;
			gecko_keypending = 1;
		}
	}

	return gecko_keypending;
}

static int
gecko_detect(void)
{
	int nsamples = 0;

	/*
	 * MINI may be still using EXI when we get here. Wait for it to go
	 * idle before probing for Gecko.
	 */
	while (nsamples++ < 1000) {
		if ((in32(EXI_CR(gecko_chan)) & EXI_CR_TSTART) != 0) {
			printf("TSTART set, resetting samples\n");
			nsamples = 0;
		}
		timer_udelay(1000);
	}

	out32(EXI_CSR(gecko_chan), 0);
	out32(EXI_CSR(gecko_chan), EXI_CSR_EXTINT | EXI_CSR_EXTINTMASK);

	return gecko_id() == 0 &&
	       gecko_command(GECKO_CMD_ID) == 0x0470;
}

int
gecko_probe(void)
{
	out32(SI_EXILK, 0);

	gecko_chan = 0;
	if (!gecko_detect()) {
		gecko_chan = 1;
		if (!gecko_detect()) {
			gecko_chan = -1;
		}
	}

	return gecko_chan;
}
