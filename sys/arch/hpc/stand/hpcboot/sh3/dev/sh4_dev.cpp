/* -*-C++-*-	$NetBSD: sh4_dev.cpp,v 1.1.2.2 2002/02/28 04:09:48 nathanw Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <hpcboot.h>
#include <hpcmenu.h>
#include <console.h>

#include <sh3/sh_arch.h> //suspend/resumeIntr

#include <sh3/cpu/sh4.h>
#include <sh3/dev/sh_dev.h>

#include <sh3/dev/sh.h>
#include <sh3/dev/hd64465.h>
#include <sh3/dev/mq100.h>

struct SH4dev::intr_priority SH4dev::_ipr_table[] = {
	// SH7750, SH7750S
	{ "TMU0",	SH4_IPRA, 12 },
	{ "TMU1",	SH4_IPRA,  8 },
	{ "TMU2",	SH4_IPRA,  4 },
	{ "RTC",	SH4_IPRA,  0 },
	{ "WDT",	SH4_IPRB, 12 },
	{ "REF",	SH4_IPRB,  8 },
	{ "SCI",	SH4_IPRB,  4 },
	{ "reserve",	SH4_IPRB,  0 },
	{ "GPIO",	SH4_IPRC, 12 },
	{ "DMAC",	SH4_IPRC,  8 },
	{ "SCIF",	SH4_IPRC,  4 },
	{ "H-UDI",	SH4_IPRC,  0 },
	// SH7750S
	{ "IRL0",	SH4_IPRD, 12 },
	{ "IRL1",	SH4_IPRD,  8 },
	{ "IRL2",	SH4_IPRD,  4 },
	{ "IRL3",	SH4_IPRD,  0 },
	{ 0, 0, 0} /* terminator */
};

void
SH4dev::dump(u_int8_t bit)
{
	int kmode;

	super::dump(bit);

	kmode = SetKMode(1);
	if (bit & DUMP_DEV) {
		// INTC
		icu_dump();
	}

	if (bit & DUMP_COMPANION) {
		// HD64465
		hd64465_dump();
	}

	if (bit & DUMP_VIDEO) {
		// MQ100
		mq100_dump();
	}

	SetKMode(kmode);

}

// INTC
void
SH4dev::icu_dump()
{
#define ON(x, c)	((x) & (c) ? check[1] : check[0])
#define _(n)		DPRINTF((TEXT("%S %S "), #n, ON(r, SH4_ICR_ ## n)))
	static const char *check[] = { "[_]", "[x]" };
	u_int16_t r;
	
	super::icu_dump_priority(_ipr_table);

	r = _reg_read_2(SH4_ICR);
	DPRINTF((TEXT("ICR: ")));
	_(NMIL);_(MAI);_(NMIB);_(NMIE);_(IRLM);
	DPRINTF((TEXT("0x%04x\n"), r));

#if 0 // monitoring SH4 interrupt request.
	// disable SH3 internal devices interrupt.
	suspendIntr();
	_reg_write_2(SH4_IPRA, 0);
	_reg_write_2(SH4_IPRB, 0);
	_reg_write_2(SH4_IPRC, 0);
//	_reg_write_2(SH4_IPRD, 0);  SH7709S only.
	resumeIntr(0);	// all interrupts enable.
	while (1) {
		DPRINTF((TEXT("%04x ", _reg_read_2(HD64465_NIRR))));
		bitdisp(_reg_read_4(SH4_INTEVT));
	}
	/* NOTREACHED */
#endif
#undef _
#undef ON
}

void
SH4dev::hd64465_dump()
{

	DPRINTF((TEXT("<<<HD64465>>>\n")));
	if (_reg_read_2(HD64465_SDIDR) != 0x8122) {
		DPRINTF((TEXT("not found.\n")));
		return;
	}

	DPRINTF((TEXT("SMSCR:  ")));	// standby
	bitdisp(_reg_read_2(HD64465_SMSCR));
	DPRINTF((TEXT("SPCCR:  ")));	// clock
	bitdisp(_reg_read_2(HD64465_SPCCR));

	DPRINTF((TEXT("\nNIRR:   ")));	// request
	bitdisp(_reg_read_2(HD64465_NIRR));
	DPRINTF((TEXT("NIMR:   ")));	// mask
	bitdisp(_reg_read_2(HD64465_NIMR));
	DPRINTF((TEXT("NITR:   ")));	// trigger
	bitdisp(_reg_read_2(HD64465_NITR));

#if 0 // monitoring HD64465 interrupt request.
	suspendIntr();
	while (1)
		bitdisp(_reg_read_2(HD64465_NIRR));
	/* NOTREACHED */
#endif	
}

void
SH4dev::mq100_dump()
{
	u_int32_t a, e;
	int i;

	// This is HPW650PA test. 640 * 480 linebytes 1280.
	DPRINTF((TEXT("<<<MQ100/HD64464>>>\n")));
	a = MQ100_FB_BASE + 0x4b000;
	e = a + 640 * 480 * sizeof(u_int16_t);
	while (a < e) {
		for (i = 0; i < 640; i++, a += sizeof(u_int16_t))
			_reg_write_2(a, ~_reg_read_2(a) & 0xffff);
	}
}
