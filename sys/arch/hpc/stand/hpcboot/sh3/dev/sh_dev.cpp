/* -*-C++-*-	$NetBSD: sh_dev.cpp,v 1.1.2.2 2002/02/28 04:09:48 nathanw Exp $	*/

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

#include <sh3/sh_mmu.h>
#include <sh3/dev/sh_dev.h>

#include <sh3/dev/sh.h>

SHdev::SHdev()
{

	_menu = &HpcMenuInterface::Instance();	
	_cons = Console::Instance();
}

void
SHdev::dump(u_int8_t bit)
{
	u_int32_t reg = 0;
	int kmode;

	DPRINTF((TEXT("DEBUG BIT: ")));
	bitdisp(bit);

	if (bit & DUMP_CPU) {
		// Cache
		MemoryManager_SHMMU::CacheDump();
		// MMU
		MemoryManager_SHMMU::MMUDump();
		// Status register
		kmode = SetKMode(1);
		__asm(
			"stc	sr, r0\n"
			"mov.l	r0, @r4", &reg);
		SetKMode(kmode);
		DPRINTF((TEXT("SR: ")));
		bitdisp(reg);
	}

	if (bit & DUMP_DEV) {
		kmode = SetKMode(1);
		print_stack_pointer();
		// SCIF
		scif_dump(HPC_PREFERENCE.serial_speed);
		SetKMode(kmode);
	}
}

void
SHdev::print_stack_pointer(void)
{
	int sp;

	__asm("mov.l	r15, @r4", &sp);
	DPRINTF((TEXT("SP 0x%08x\n"), sp));
}

//
// SH3/SH4 common functions.
//
// SCIF
void
SHdev::scif_dump(int bps)
{
	u_int16_t r16;
	u_int32_t r;
	int n;

	print_stack_pointer();
	DPRINTF((TEXT("<<<SCIF>>>\n")));
	/* mode */
	r = _scif_reg_read(SH3_SCSMR2);
	n = 1 << ((r & SCSMR2_CKS) << 1);
	DPRINTF((TEXT("mode: %dbit %S-parity %d stop bit clock PCLOCK/%d\n"),
	    r & SCSMR2_CHR ? 7 : 8,
	    r & SCSMR2_PE  ? r & SCSMR2_OE ? "odd" : "even" : "non",
	    r & SCSMR2_STOP ? 2 : 1,
	    n));
	/* bit rate */
	r = _scif_reg_read(SH3_SCBRR2);
	DPRINTF((TEXT("SCBRR=%d(%dbps) estimated PCLOCK %dHz\n"), r, bps,
	    32 * bps *(r + 1) * n));

	/* control */
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, SCSCR2_##m, #m)
	DPRINTF((TEXT("SCSCR2: ")));
	r = _scif_reg_read(SH3_SCSCR2);
	DBG_BIT_PRINT(r, TIE);
	DBG_BIT_PRINT(r, RIE);
	DBG_BIT_PRINT(r, TE);
	DBG_BIT_PRINT(r, RE);
	DPRINTF((TEXT("CKE=%d\n"), r & SCSCR2_CKE));
#undef	DBG_BIT_PRINT

	/* status */
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, SCSSR2_##m, #m)
	r16 = _reg_read_2(SH3_SCSSR2);
	DPRINTF((TEXT("SCSSR2: ")));
	DBG_BIT_PRINT(r16, ER);
	DBG_BIT_PRINT(r16, TEND);
	DBG_BIT_PRINT(r16, TDFE);
	DBG_BIT_PRINT(r16, BRK);
	DBG_BIT_PRINT(r16, FER);
	DBG_BIT_PRINT(r16, PER);
	DBG_BIT_PRINT(r16, RDF);
	DBG_BIT_PRINT(r16, DR);
#undef	DBG_BIT_PRINT

	/* FIFO control */
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, SCFCR2_##m, #m)
	r = _scif_reg_read(SH3_SCFCR2);
	DPRINTF((TEXT("SCFCR2: ")));
	DBG_BIT_PRINT(r, RTRG1);
	DBG_BIT_PRINT(r, RTRG0);
	DBG_BIT_PRINT(r, TTRG1);
	DBG_BIT_PRINT(r, TTRG0);
	DBG_BIT_PRINT(r, MCE);
	DBG_BIT_PRINT(r, TFRST);
	DBG_BIT_PRINT(r, RFRST);
	DBG_BIT_PRINT(r, LOOP);
	DPRINTF((TEXT("\n")));
#undef	DBG_BIT_PRINT
}

// INTC
void
SHdev::icu_dump_priority(struct intr_priority *tab)
{
	
	DPRINTF((TEXT("<<<INTC>>>\n")));

	DPRINTF((TEXT("----interrupt priority----\n")));
	for (; tab->name; tab++) {
		DPRINTF((TEXT("%-10S %d\n"), tab->name,
		    (_reg_read_2(tab->reg) >> tab->shift) & SH_IPR_MASK));
	}
	DPRINTF((TEXT("--------------------------\n")));
}

