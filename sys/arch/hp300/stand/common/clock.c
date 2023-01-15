/*	$NetBSD: clock.c,v 1.14 2023/01/15 06:19:46 tsutsui Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */

#include <sys/param.h>

#include <net/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <hp300/stand/common/hilreg.h>

#include <hp300/dev/frodoreg.h>		/* for APCI offsets */
#include <hp300/dev/intioreg.h>		/* for frodo offsets */
#include <dev/ic/mc146818reg.h>
#include <dev/clock_subr.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <hp300/stand/common/samachdep.h>

#define FEBRUARY        2

#define BBC_SET_REG     0xe0
#define BBC_WRITE_REG   0xc2
#define BBC_READ_REG    0xc3
#define NUM_BBC_REGS    12

#define range_test(n, l, h)	if ((n) < (l) || (n) > (h)) return false
#define bbc_to_decimal(a,b)	(bbc_registers[a] * 10 + bbc_registers[b])

static uint8_t bbc_registers[13];
static struct hil_dev *bbcaddr = BBCADDR;

static volatile uint8_t *mcclock =
    (volatile uint8_t *)(INTIOBASE + FRODO_BASE + FRODO_CALENDAR);

static bool clock_to_gmt(satime_t *);

static void read_bbc(void);
static uint8_t read_bbc_reg(int);
static uint8_t mc_read(u_int);

satime_t
getsecs(void)
{
	static bool clockinited = false;
	satime_t timbuf = 0;

	if (!clock_to_gmt(&timbuf) && !clockinited)
		printf("WARNING: bad date in battery clock\n");
	clockinited = true;

	/* Battery clock does not store usec's, so forget about it. */
	return timbuf;
}

static bool
clock_to_gmt(satime_t *timbuf)
{
	int i;
	satime_t tmp;
	int year, month, day, hour, min, sec;

	if (machineid == HP_425 && mmuid == MMUID_425_E) {
		/* 425e uses mcclock on the frodo utility chip */
		while ((mc_read(MC_REGA) & MC_REGA_UIP) != 0)
			continue;
		sec   = mc_read(MC_SEC);
		min   = mc_read(MC_MIN);
		hour  = mc_read(MC_HOUR);
		day   = mc_read(MC_DOM);
		month = mc_read(MC_MONTH);
		year  = mc_read(MC_YEAR) + 1900;
	} else {
		/* Use the traditional HIL bbc for all other models */
		read_bbc();

		sec = bbc_to_decimal(1, 0);
		min = bbc_to_decimal(3, 2);

		/*
		 * Hours are different for some reason. Makes no sense really.
		 */
		hour  = ((bbc_registers[5] & 0x03) * 10) + bbc_registers[4];
		day   = bbc_to_decimal(8, 7);
		month = bbc_to_decimal(10, 9);
		year  = bbc_to_decimal(12, 11) + 1900;
	}

	if (year < POSIX_BASE_YEAR)
		year += 100;

#ifdef CLOCK_DEBUG
	printf("clock todr: %u/%u/%u %u:%u:%u\n",
	    year, month, day, hour, min, sec);
#endif

	range_test(hour, 0, 23);
	range_test(day, 1, 31);
	range_test(month, 1, 12);

	tmp = 0;

	for (i = POSIX_BASE_YEAR; i < year; i++)
		tmp += days_per_year(i);
	if (is_leap_year(year) && month > FEBRUARY)
		tmp++;

	for (i = 1; i < month; i++)
	  	tmp += days_in_month(i);

	tmp += (day - 1);
	tmp = ((tmp * 24 + hour) * 60 + min) * 60 + sec;

	*timbuf = tmp;
	return true;
}

static void
read_bbc(void)
{
  	int i;
	bool read_okay;

	read_okay = false;
	while (!read_okay) {
		read_okay = true;
		for (i = 0; i <= NUM_BBC_REGS; i++)
			bbc_registers[i] = read_bbc_reg(i);
		for (i = 0; i <= NUM_BBC_REGS; i++)
			if (bbc_registers[i] != read_bbc_reg(i))
				read_okay = false;
	}
}

static uint8_t
read_bbc_reg(int reg)
{
	uint8_t data = reg;

	if (bbcaddr != NULL) {
#if 0
		send_hil_cmd(bbcaddr, BBC_SET_REG, &data, 1, NULL);
		send_hil_cmd(bbcaddr, BBC_READ_REG, NULL, 0, &data);
#else
		HILWAIT(bbcaddr);
		bbcaddr->hil_cmd = BBC_SET_REG;
		HILWAIT(bbcaddr);
		bbcaddr->hil_data = data;
		HILWAIT(bbcaddr);
		bbcaddr->hil_cmd = BBC_READ_REG;
		HILDATAWAIT(bbcaddr);
		data = bbcaddr->hil_data;
#endif
	}
	return data;
}

uint8_t
mc_read(u_int reg)
{
	uint8_t datum;

	mcclock[0] = (uint8_t)reg;
	datum = mcclock[1 << 2];	/* frodo chip has 4 byte stride */

	return datum;
}
