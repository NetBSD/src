/* -*-C++-*-	$NetBSD: sh3_dev.cpp,v 1.4.6.1 2006/04/22 11:37:28 simonb Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sh3/cpu/sh3.h>
#include <sh3/dev/sh.h>
#include <sh3/dev/sh_dev.h>
#include <sh3/dev/hd64461.h>

static void __tmu_channel_info(int, paddr_t, paddr_t, paddr_t);

struct SH3dev::intr_priority SH3dev::_ipr_table[] = {
	{ "TMU0",	SH3_IPRA, 12 },
	{ "TMU1",	SH3_IPRA,  8 },
	{ "TMU2",	SH3_IPRA,  4 },
	{ "RTC",	SH3_IPRA,  0 },
	{ "WDT",	SH3_IPRB, 12 },
	{ "REF",	SH3_IPRB,  8 },
	{ "SCI",	SH3_IPRB,  4 },
	{ "reserve",	SH3_IPRB,  0 },
	{ "IRQ3",	SH3_IPRC, 12 },
	{ "IRQ2",	SH3_IPRC,  8 },
	{ "IRQ1",	SH3_IPRC,  4 },
	{ "IRQ0",	SH3_IPRC,  0 },
	{ "PINT0-7",	SH3_IPRD, 12 },
	{ "PINT8-15",	SH3_IPRD,  8 },
	{ "IRQ5",	SH3_IPRD,  4 },
	{ "IRQ4",	SH3_IPRD,  0 },
	{ "DMAC",	SH3_IPRE, 12 },
	{ "IrDA",	SH3_IPRE,  8 },
	{ "SCIF",	SH3_IPRE,  4 },
	{ "ADC",	SH3_IPRE,  0 },
	{ 0, 0, 0} /* terminator */
};

void
SH3dev::dump(uint8_t bit)
{
	int kmode;

	super::dump(bit);

	kmode = SetKMode(1);

	if (bit & DUMP_DEV) {
		// INTC
		icu_dump();

		// BSC
		bsc_dump();

		// TMU
		tmu_dump();

		// PFC , I/O port
		pfc_dump();
	}

	if (bit & DUMP_COMPANION) {
		// HD64461
		platid_t platform;
		platform.dw.dw0 = _menu->_pref.platid_hi;
		platform.dw.dw1 = _menu->_pref.platid_lo;
		hd64461_dump(platform);
	}

	SetKMode(kmode);
}

void
SH3dev::icu_dump()
{

	super::icu_dump_priority(_ipr_table);
	icu_control();
	DPRINTF((TEXT("ICR0   0x%08x\n"), _reg_read_2(SH3_ICR0)));
	DPRINTF((TEXT("ICR1   0x%08x\n"), _reg_read_2(SH3_ICR1)));
	DPRINTF((TEXT("ICR2   0x%08x\n"), _reg_read_2(SH3_ICR2)));
	DPRINTF((TEXT("PINTER 0x%08x\n"), _reg_read_2(SH3_PINTER)));
	DPRINTF((TEXT("IPRA   0x%08x\n"), _reg_read_2(SH3_IPRA)));
	DPRINTF((TEXT("IPRB   0x%08x\n"), _reg_read_2(SH3_IPRB)));
	DPRINTF((TEXT("IPRC   0x%08x\n"), _reg_read_2(SH3_IPRC)));
	DPRINTF((TEXT("IPRD   0x%08x\n"), _reg_read_2(SH3_IPRD)));
	DPRINTF((TEXT("IPRE   0x%08x\n"), _reg_read_2(SH3_IPRE)));
	DPRINTF((TEXT("IRR0   0x%08x\n"), _reg_read_1(SH3_IRR0)));
	DPRINTF((TEXT("IRR1   0x%08x\n"), _reg_read_1(SH3_IRR1)));
	DPRINTF((TEXT("IRR2   0x%08x\n"), _reg_read_1(SH3_IRR2)));
}

void
SH3dev::icu_control()
{
	const char *sense_select[] = {
		"falling edge",
		"raising edge",
		"low level",
		"reserved",
	};
	uint16_t r;

	// PINT0-15
	DPRINTF((TEXT("PINT enable(on |)  :")));
	bitdisp(_reg_read_2(SH3_PINTER));
	DPRINTF((TEXT("PINT detect(high |):")));
	bitdisp(_reg_read_2(SH3_ICR2));
	// NMI
	r = _reg_read_2(SH3_ICR0);
	DPRINTF((TEXT("NMI(%S %S-edge),"),
	    r & SH3_ICR0_NMIL ? "High" : "Low",
	    r & SH3_ICR0_NMIE ? "raising" : "falling"));
	r = _reg_read_2(SH3_ICR1);
	DPRINTF((TEXT(" %S maskable,"), r & SH3_ICR1_MAI ? "" : "never"));
	DPRINTF((TEXT("  SR.BL %S\n"),
	    r & SH3_ICR1_BLMSK ? "ignored" : "maskable"));
	// IRQ0-5
	DPRINTF((TEXT("IRQ[3:0]pin : %S mode\n"),
	    r & SH3_ICR1_IRQLVL ? "IRL 15level" : "IRQ[0:3]"));
	if (r & SH3_ICR1_IRQLVL) {
		DPRINTF((TEXT("IRLS[0:3] %S\n"),
		    r & SH3_ICR1_IRLSEN ? "enabled" : "disabled"));
	}
	// sense select
	for (int i = 5; i >= 0; i--) {
		DPRINTF((TEXT("IRQ[%d] %S\n"), i,
		    sense_select [
			    (r >>(i * 2)) & SH3_SENSE_SELECT_MASK]));
	}
}

//
// Debug Functions.
//
void
SH3dev::bsc_dump()
{

	DPRINTF((TEXT("<<<Bus State Controller>>>\n")));
#define	DUMP_BSC_REG(x)							\
	DPRINTF((TEXT("%-8S"), #x));					\
	bitdisp(_reg_read_2(SH3_ ## x))
	DUMP_BSC_REG(BCR1);
	DUMP_BSC_REG(BCR2);
	DUMP_BSC_REG(WCR1);
	DUMP_BSC_REG(WCR2);
	DUMP_BSC_REG(MCR);
	DUMP_BSC_REG(DCR);
	DUMP_BSC_REG(PCR);
	DUMP_BSC_REG(RTCSR);
	DUMP_BSC_REG(RTCNT);
	DUMP_BSC_REG(RTCOR);
	DUMP_BSC_REG(RFCR);
	DUMP_BSC_REG(BCR3);
#undef DUMP_BSC_REG
}

void
SH3dev::pfc_dump()
{
	DPRINTF((TEXT("<<<Pin Function Controller>>>\n")));
	DPRINTF((TEXT("[control]\n")));
#define	DUMP_PFC_REG(x)							\
	DPRINTF((TEXT("P%SCR :"), #x));					\
	bitdisp(_reg_read_2(SH3_P##x##CR))
	DUMP_PFC_REG(A);
	DUMP_PFC_REG(B);
	DUMP_PFC_REG(C);
	DUMP_PFC_REG(D);
	DUMP_PFC_REG(E);
	DUMP_PFC_REG(F);
	DUMP_PFC_REG(G);
	DUMP_PFC_REG(H);
	DUMP_PFC_REG(J);
	DUMP_PFC_REG(K);
	DUMP_PFC_REG(L);
#undef DUMP_PFC_REG
	DPRINTF((TEXT("SCPCR :")));
	bitdisp(_reg_read_2(SH3_SCPCR));
	DPRINTF((TEXT("\n[data]\n")));
#define	DUMP_IOPORT_REG(x)						\
	DPRINTF((TEXT("P%SDR :"), #x));					\
	bitdisp(_reg_read_1(SH3_P##x##DR))
	DUMP_IOPORT_REG(A);
	DUMP_IOPORT_REG(B);
	DUMP_IOPORT_REG(C);
	DUMP_IOPORT_REG(D);
	DUMP_IOPORT_REG(E);
	DUMP_IOPORT_REG(F);
	DUMP_IOPORT_REG(G);
	DUMP_IOPORT_REG(H);
	DUMP_IOPORT_REG(J);
	DUMP_IOPORT_REG(K);
	DUMP_IOPORT_REG(L);
#undef DUMP_IOPORT_REG
	DPRINTF((TEXT("SCPDR :")));
	bitdisp(_reg_read_1(SH3_SCPDR));
}

void
SH3dev::tmu_dump()
{
	uint8_t r8;

	DPRINTF((TEXT("<<<TMU>>>\n")));
	/* Common */
	/* TOCR  timer output control register */
	r8 = _reg_read_1(SH3_TOCR);
	DPRINTF((TEXT("TCLK = %S\n"),
	    r8 & SH3_TOCR_TCOE ? "RTC output" : "input"));
	/* TSTR */
	r8 = _reg_read_1(SH3_TSTR);
	DPRINTF((TEXT("Timer start(#0:2) [%c][%c][%c]\n"),
	    r8 & SH3_TSTR_STR0 ? 'x' : '_',
	    r8 & SH3_TSTR_STR1 ? 'x' : '_',
	    r8 & SH3_TSTR_STR2 ? 'x' : '_'));

#define	CHANNEL_DUMP(a, x)						\
	tmu_channel_dump(x, SH##a##_TCOR##x,				\
			 SH##a##_TCNT##x,				\
			 SH##a##_TCR##x##)
	CHANNEL_DUMP(3, 0);
	CHANNEL_DUMP(3, 1);
	CHANNEL_DUMP(3, 2);
#undef	CHANNEL_DUMP
	DPRINTF((TEXT("\n")));
}

void
SH3dev::tmu_channel_dump(int unit, paddr_t tcor, paddr_t tcnt,
    paddr_t tcr)
{
	uint32_t r32;
	uint16_t r16;

	DPRINTF((TEXT("TMU#%d:"), unit));
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, SH3_TCR_##m, #m)
	/* TCR*/
	r16 = _reg_read_2(tcr);
	DBG_BIT_PRINT(r16, UNF);
	DBG_BIT_PRINT(r16, UNIE);
	DBG_BIT_PRINT(r16, CKEG1);
	DBG_BIT_PRINT(r16, CKEG0);
	DBG_BIT_PRINT(r16, TPSC2);
	DBG_BIT_PRINT(r16, TPSC1);
	DBG_BIT_PRINT(r16, TPSC0);
	/* channel 2 has input capture. */
	if (unit == 2) {
		DBG_BIT_PRINT(r16, ICPF);
		DBG_BIT_PRINT(r16, ICPE1);
		DBG_BIT_PRINT(r16, ICPE0);
	}
#undef DBG_BIT_PRINT
	/* TCNT0  timer counter */
	r32 = _reg_read_4(tcnt);
	DPRINTF((TEXT("\ncnt=0x%08x"), r32));
	/* TCOR0  timer constant register */
	r32 = _reg_read_4(tcor);
	DPRINTF((TEXT(" constant=0x%04x"), r32));

	if (unit == 2)
		DPRINTF((TEXT(" input capture=0x%08x\n"), SH3_TCPR2));
	else
		DPRINTF((TEXT("\n")));
}

void
SH3dev::hd64461_dump(platid_t &platform)
{
	uint16_t r16;
	uint8_t r8;

#define	MATCH(p)						\
	platid_match(&platform, &platid_mask_MACH_##p)

	DPRINTF((TEXT("<<<HD64461>>>\n")));
	if (!MATCH(HP_LX) &&
	    !MATCH(HP_JORNADA_6XX) &&
	    !MATCH(HITACHI_PERSONA_HPW230JC)) {
		DPRINTF((TEXT("don't exist.")));
		return;
	}

#if 0
	DPRINTF((TEXT("frame buffer test start\n")));
	uint8_t *fb = reinterpret_cast<uint8_t *>(HD64461_FBBASE);

	for (int i = 0; i < 320 * 240 * 2 / 8; i++)
		*fb++ = 0xff;
	DPRINTF((TEXT("frame buffer test end\n")));
#endif
	// System
	DPRINTF((TEXT("STBCR (System Control Register)\n")));
	r16 = _reg_read_2(HD64461_SYSSTBCR_REG16);
	bitdisp(r16);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_SYSSTBCR_##m, #m)
	DBG_BIT_PRINT(r16, CKIO_STBY);
	DBG_BIT_PRINT(r16, SAFECKE_IST);
	DBG_BIT_PRINT(r16, SLCKE_IST);
	DBG_BIT_PRINT(r16, SAFECKE_OST);
	DBG_BIT_PRINT(r16, SLCKE_OST);
	DBG_BIT_PRINT(r16, SMIAST);
	DBG_BIT_PRINT(r16, SLCDST);
	DBG_BIT_PRINT(r16, SPC0ST);
	DBG_BIT_PRINT(r16, SPC1ST);
	DBG_BIT_PRINT(r16, SAFEST);
	DBG_BIT_PRINT(r16, STM0ST);
	DBG_BIT_PRINT(r16, STM1ST);
	DBG_BIT_PRINT(r16, SIRST);
	DBG_BIT_PRINT(r16, SURTSD);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("SYSCR (System Configuration Register)\n")));
	r16 = _reg_read_2(HD64461_SYSSYSCR_REG16);
	bitdisp(r16);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_SYSSYSCR_##m, #m)
	DBG_BIT_PRINT(r16, SCPU_BUS_IGAT);
	DBG_BIT_PRINT(r16, SPTA_IR);
	DBG_BIT_PRINT(r16, SPTA_TM);
	DBG_BIT_PRINT(r16, SPTB_UR);
	DBG_BIT_PRINT(r16, WAIT_CTL_SEL);
	DBG_BIT_PRINT(r16, SMODE1);
	DBG_BIT_PRINT(r16, SMODE0);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("SCPUCR (CPU Data Bus Control Register)\n")));
	r16 = _reg_read_2(HD64461_SYSSCPUCR_REG16);
	bitdisp(r16);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_SYSSCPUCR_##m, #m)
	DBG_BIT_PRINT(r16, SPDSTOF);
	DBG_BIT_PRINT(r16, SPDSTIG);
	DBG_BIT_PRINT(r16, SPCSTOF);
	DBG_BIT_PRINT(r16, SPCSTIG);
	DBG_BIT_PRINT(r16, SPBSTOF);
	DBG_BIT_PRINT(r16, SPBSTIG);
	DBG_BIT_PRINT(r16, SPASTOF);
	DBG_BIT_PRINT(r16, SPASTIG);
	DBG_BIT_PRINT(r16, SLCDSTIG);
	DBG_BIT_PRINT(r16, SCPU_CS56_EP);
	DBG_BIT_PRINT(r16, SCPU_CMD_EP);
	DBG_BIT_PRINT(r16, SCPU_ADDR_EP);
	DBG_BIT_PRINT(r16, SCPDPU);
	DBG_BIT_PRINT(r16, SCPU_A2319_EP);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("\n")));

	// INTC
	DPRINTF((TEXT("NIRR (Interrupt Request Register)\n")));
	r16 = _reg_read_2(HD64461_INTCNIRR_REG16);
	bitdisp(r16);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_INTCNIRR_##m, #m)
	DBG_BIT_PRINT(r16, PCC0R);
	DBG_BIT_PRINT(r16, PCC1R);
	DBG_BIT_PRINT(r16, AFER);
	DBG_BIT_PRINT(r16, GPIOR);
	DBG_BIT_PRINT(r16, TMU0R);
	DBG_BIT_PRINT(r16, TMU1R);
	DBG_BIT_PRINT(r16, IRDAR);
	DBG_BIT_PRINT(r16, UARTR);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("NIMR (Interrupt Mask Register)\n")));
	r16 = _reg_read_2(HD64461_INTCNIMR_REG16);
	bitdisp(r16);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_INTCNIMR_##m, #m)
	DBG_BIT_PRINT(r16, PCC0M);
	DBG_BIT_PRINT(r16, PCC1M);
	DBG_BIT_PRINT(r16, AFEM);
	DBG_BIT_PRINT(r16, GPIOM);
	DBG_BIT_PRINT(r16, TMU0M);
	DBG_BIT_PRINT(r16, TMU1M);
	DBG_BIT_PRINT(r16, IRDAM);
	DBG_BIT_PRINT(r16, UARTM);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("\n")));

	// PCMCIA
	// PCC0
	DPRINTF((TEXT("[PCC0 memory and I/O card (SH3 Area 6)]\n")));
	DPRINTF((TEXT("PCC0 Interface Status Register\n")));
	r8 = _reg_read_1(HD64461_PCC0ISR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0ISR_##m, #m)
	DBG_BIT_PRINT(r8, P0READY);
	DBG_BIT_PRINT(r8, P0MWP);
	DBG_BIT_PRINT(r8, P0VS2);
	DBG_BIT_PRINT(r8, P0VS1);
	DBG_BIT_PRINT(r8, P0CD2);
	DBG_BIT_PRINT(r8, P0CD1);
	DBG_BIT_PRINT(r8, P0BVD2);
	DBG_BIT_PRINT(r8, P0BVD1);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC0 General Control Register\n")));
	r8 = _reg_read_1(HD64461_PCC0GCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0GCR_##m, #m)
	DBG_BIT_PRINT(r8, P0DRVE);
	DBG_BIT_PRINT(r8, P0PCCR);
	DBG_BIT_PRINT(r8, P0PCCT);
	DBG_BIT_PRINT(r8, P0VCC0);
	DBG_BIT_PRINT(r8, P0MMOD);
	DBG_BIT_PRINT(r8, P0PA25);
	DBG_BIT_PRINT(r8, P0PA24);
	DBG_BIT_PRINT(r8, P0REG);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC0 Card Status Change Register\n")));
	r8 = _reg_read_1(HD64461_PCC0CSCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0CSCR_##m, #m)
	DBG_BIT_PRINT(r8, P0SCDI);
	DBG_BIT_PRINT(r8, P0IREQ);
	DBG_BIT_PRINT(r8, P0SC);
	DBG_BIT_PRINT(r8, P0CDC);
	DBG_BIT_PRINT(r8, P0RC);
	DBG_BIT_PRINT(r8, P0BW);
	DBG_BIT_PRINT(r8, P0BD);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC0 Card Status Change Interrupt Enable Register\n")));
	r8 = _reg_read_1(HD64461_PCC0CSCIER_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0CSCIER_##m, #m)
	DBG_BIT_PRINT(r8, P0CRE);
	DBG_BIT_PRINT(r8, P0SCE);
	DBG_BIT_PRINT(r8, P0CDE);
	DBG_BIT_PRINT(r8, P0RE);
	DBG_BIT_PRINT(r8, P0BWE);
	DBG_BIT_PRINT(r8, P0BDE);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\ninterrupt type: ")));
	switch (r8 & HD64461_PCC0CSCIER_P0IREQE_MASK) {
	case HD64461_PCC0CSCIER_P0IREQE_NONE:
		DPRINTF((TEXT("none\n")));
		break;
	case HD64461_PCC0CSCIER_P0IREQE_LEVEL:
		DPRINTF((TEXT("level\n")));
		break;
	case HD64461_PCC0CSCIER_P0IREQE_FEDGE:
		DPRINTF((TEXT("falling edge\n")));
		break;
	case HD64461_PCC0CSCIER_P0IREQE_REDGE:
		DPRINTF((TEXT("rising edge\n")));
		break;
	}

	DPRINTF((TEXT("PCC0 Software Control Register\n")));
	r8 = _reg_read_1(HD64461_PCC0SCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0SCR_##m, #m)
	DBG_BIT_PRINT(r8, P0VCC1);
	DBG_BIT_PRINT(r8, P0SWP);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	// PCC1
	DPRINTF((TEXT("[PCC1 memory card only (SH3 Area 5)]\n")));
	DPRINTF((TEXT("PCC1 Interface Status Register\n")));
	r8 = _reg_read_1(HD64461_PCC1ISR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1ISR_##m, #m)
	DBG_BIT_PRINT(r8, P1READY);
	DBG_BIT_PRINT(r8, P1MWP);
	DBG_BIT_PRINT(r8, P1VS2);
	DBG_BIT_PRINT(r8, P1VS1);
	DBG_BIT_PRINT(r8, P1CD2);
	DBG_BIT_PRINT(r8, P1CD1);
	DBG_BIT_PRINT(r8, P1BVD2);
	DBG_BIT_PRINT(r8, P1BVD1);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 General Contorol Register\n")));
	r8 = _reg_read_1(HD64461_PCC1GCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1GCR_##m, #m)
	DBG_BIT_PRINT(r8, P1DRVE);
	DBG_BIT_PRINT(r8, P1PCCR);
	DBG_BIT_PRINT(r8, P1VCC0);
	DBG_BIT_PRINT(r8, P1MMOD);
	DBG_BIT_PRINT(r8, P1PA25);
	DBG_BIT_PRINT(r8, P1PA24);
	DBG_BIT_PRINT(r8, P1REG);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Card Status Change Register\n")));
	r8 = _reg_read_1(HD64461_PCC1CSCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1CSCR_##m, #m)
	DBG_BIT_PRINT(r8, P1SCDI);
	DBG_BIT_PRINT(r8, P1CDC);
	DBG_BIT_PRINT(r8, P1RC);
	DBG_BIT_PRINT(r8, P1BW);
	DBG_BIT_PRINT(r8, P1BD);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Card Status Change Interrupt Enable Register\n")));
	r8 = _reg_read_1(HD64461_PCC1CSCIER_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1CSCIER_##m, #m)
	DBG_BIT_PRINT(r8, P1CRE);
	DBG_BIT_PRINT(r8, P1CDE);
	DBG_BIT_PRINT(r8, P1RE);
	DBG_BIT_PRINT(r8, P1BWE);
	DBG_BIT_PRINT(r8, P1BDE);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Software Control Register\n")));
	r8 = _reg_read_1(HD64461_PCC1SCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1SCR_##m, #m)
	DBG_BIT_PRINT(r8, P1VCC1);
	DBG_BIT_PRINT(r8, P1SWP);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	// General Control
	DPRINTF((TEXT("[General Control]\n")));
	DPRINTF((TEXT("PCC0 Output pins Control Register\n")));
	r8 = _reg_read_1(HD64461_PCCP0OCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCCP0OCR_##m, #m)
	DBG_BIT_PRINT(r8, P0DEPLUP);
	DBG_BIT_PRINT(r8, P0AEPLUP);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Output pins Control Register\n")));
	r8 = _reg_read_1(HD64461_PCCP1OCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCCP1OCR_##m, #m)
	DBG_BIT_PRINT(r8, P1RST8MA);
	DBG_BIT_PRINT(r8, P1RST4MA);
	DBG_BIT_PRINT(r8, P1RAS8MA);
	DBG_BIT_PRINT(r8, P1RAS4MA);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PC Card General Control Register\n")));
	r8 = _reg_read_1(HD64461_PCCPGCR_REG8);
	bitdisp(r8);
#define	DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCCPGCR_##m, #m)
	DBG_BIT_PRINT(r8, PSSDIR);
	DBG_BIT_PRINT(r8, PSSRDWR);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	// GPIO
#define	GPIO_DUMP(x)							\
	bitdisp(_reg_read_2(HD64461_GPA##x##R_REG16));			\
	bitdisp(_reg_read_2(HD64461_GPB##x##R_REG16));			\
	bitdisp(_reg_read_2(HD64461_GPC##x##R_REG16));			\
	bitdisp(_reg_read_2(HD64461_GPD##x##R_REG16))

	DPRINTF((TEXT("GPIO Port Control Register\n")));
	GPIO_DUMP(C);
	DPRINTF((TEXT("GPIO Port Data Register\n")));
	GPIO_DUMP(D);
	DPRINTF((TEXT("GPIO Port Interrupt Control Register\n")));
	GPIO_DUMP(IC);
	DPRINTF((TEXT("GPIO Port Interrupt Status  Register\n")));
	GPIO_DUMP(IS);
}

#ifdef SH7709TEST
uint32_t sh7707_fb_dma_addr;
uint16_t val;
int s;

s = suspendIntr();
VOLATILE_REF16(SH7707_LCDAR) = SH7707_LCDAR_LCDDMR0;
val = VOLATILE_REF16(SH7707_LCDDMR);
sh7707_fb_dma_addr = val;
VOLATILE_REF16(SH7707_LCDAR) = SH7707_LCDAR_LCDDMR1;
val = VOLATILE_REF16(SH7707_LCDDMR);
sh7707_fb_dma_addr |= (val << 16);
resumeIntr(s);

DPRINTF((TEXT("SH7707 frame buffer DMA address: 0x%08x\n"),
    sh7707_fb_dma_addr));
#endif
