/*	$NetBSD: sh_arch.cpp,v 1.8 2002/02/04 17:38:27 uch Exp $	*/

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
#include <sh3/sh_arch.h>
#include <sh3/hd64461.h>
#include <sh3/hd64465.h>
#include "scifreg.h"

static void __tmu_channel_info(int, paddr_t, paddr_t, paddr_t);

struct SHArchitecture::intr_priority SHArchitecture::ipr_table[] = {
	{ "TMU0",	ICU_IPRA_REG16, 12 },
	{ "TMU1",	ICU_IPRA_REG16,  8 },
	{ "TMU2",	ICU_IPRA_REG16,  4 },
	{ "RTC",	ICU_IPRA_REG16,  0 },
	{ "WDT",	ICU_IPRB_REG16, 12 },
	{ "REF",	ICU_IPRB_REG16,  8 },
	{ "SCI",	ICU_IPRB_REG16,  4 },
	{ "reserve",	ICU_IPRB_REG16,  0 },
	{ "IRQ3",	ICU_IPRC_REG16, 12 },
	{ "IRQ2",	ICU_IPRC_REG16,  8 },
	{ "IRQ1",	ICU_IPRC_REG16,  4 },
	{ "IRQ0",	ICU_IPRC_REG16,  0 },
	{ "PINT0-7",	ICU_IPRD_REG16, 12 },
	{ "PINT8-15",	ICU_IPRD_REG16,  8 },
	{ "IRQ5",	ICU_IPRD_REG16,  4 },
	{ "IRQ4",	ICU_IPRD_REG16,  0 },
	{ "DMAC",	ICU_IPRE_REG16, 12 },
	{ "IrDA",	ICU_IPRE_REG16,  8 },
	{ "SCIF",	ICU_IPRE_REG16,  4 },
	{ "ADC",	ICU_IPRE_REG16,  0 },
	{ 0, 0, 0} /* terminator */
};

BOOL
SHArchitecture::init(void)
{

	if (!_mem->init()) {
		DPRINTF((TEXT("can't initialize memory manager.\n")));
		return FALSE;
	}
	// set D-RAM information
	DPRINTF((TEXT("Memory Bank:\n")));
	_mem->loadBank(DRAM_BANK0_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK1_START, DRAM_BANK_SIZE);

	return TRUE;
}

BOOL
SHArchitecture::setupLoader()
{
	vaddr_t v;

	if (!_mem->getPage(v , _loader_addr)) {
		DPRINTF((TEXT("can't get page for 2nd loader.\n")));
		return FALSE;
	}
	DPRINTF((TEXT("2nd bootloader vaddr=0x%08x paddr=0x%08x\n"),
	    (unsigned)v,(unsigned)_loader_addr));

	memcpy(LPVOID(v), LPVOID(_boot_func), _mem->getPageSize());

	return TRUE;
}

void
SHArchitecture::jump(paddr_t info, paddr_t pvec)
{
	kaddr_t sp;
	vaddr_t v;
	paddr_t p;

	// stack for bootloader
	_mem->getPage(v, p);
	sp = ptokv(p + _mem->getPageSize() / 2);

	info = ptokv(info);
	pvec = ptokv(pvec);
	_loader_addr = ptokv(_loader_addr);
	DPRINTF((TEXT("boot arg: 0x%08x stack: 0x%08x\nBooting kernel...\n"),
	    info, sp));

	// Change to privilege-mode.
	SetKMode(1);

	// Cache flush(for 2nd bootloader)
	//
	// SH4 uses WinCE CacheSync(). this routine may causes TLB
	// exception. so calls before suspendIntr().
	//
	cache_flush();

	// Disable external interrupt.
	suspendIntr();

	// jump to 2nd loader.(run P1) at this time I still use MMU.
	__asm(
	    "mov	r6, r15\n"
	    "jmp	@r7\n"
	    "nop	\n", info, pvec, sp, _loader_addr);
	// NOTREACHED
}

// disable external interrupt and save its priority.
u_int32_t
suspendIntr(void)
{
	u_int32_t sr;

	__asm(
	    "stc	sr, r0\n"
	    "mov.l	r0, @r4\n"
	    "or	r5, r0\n"
	    "ldc	r0, sr\n", &sr, 0x000000f0);
	return sr & 0x000000f0;
}

// resume external interrupt priority.
void
resumeIntr(u_int32_t s)
{

	__asm("stc	sr, r0\n"
	    "and	r5, r0\n"
	    "or	r4, r0\n"
	    "ldc	r0, sr\n", s, 0xffffff0f);
}

void
SHArchitecture::print_stack_pointer(void)
{
	int sp;

	__asm("mov.l	r15, @r4", &sp);
	DPRINTF((TEXT("SP 0x%08x\n"), sp));
}

void
SHArchitecture::systemInfo()
{
	u_int32_t reg;
	HpcMenuInterface &menu = HpcMenuInterface::Instance();

	Architecture::systemInfo();

	// check debug level.
	if (menu._cons_parameter == 0)
		return;

	_kmode = SetKMode(1);

	// Cache
	reg = VOLATILE_REF(CCR);
	DPRINTF((TEXT("Cache ")));
	if (reg & CCR_CE)
		DPRINTF((TEXT("Enabled. %s-mode, P0/U0/P3 Write-%s, P1 Write-%s\n"),
		    reg & CCR_RA ? TEXT("RAM") : TEXT("normal"),
		    reg & CCR_WT ? TEXT("Through") : TEXT("Back"),
		    reg & CCR_CB ? TEXT("Back") : TEXT("Through")));
	else
		DPRINTF((TEXT("Disabled.\n")));

	// MMU
	reg = VOLATILE_REF(MMUCR);
	DPRINTF((TEXT("MMU ")));
	if (reg & MMUCR_AT)
		DPRINTF((TEXT("Enabled. %s index-mode, %s virtual storage mode\n"),
		    reg & MMUCR_IX 
		    ? TEXT("ASID + VPN") : TEXT("VPN only"),
		    reg & MMUCR_SV ? TEXT("single") : TEXT("multiple")));
	else
		DPRINTF((TEXT("Disabled.\n")));

	// Status register
	reg = 0;
	__asm("stc	sr, r0\n"
	    "mov.l	r0, @r4", &reg);
	DPRINTF((TEXT("SR 0x%08x\n"), reg));

	// BSC
	bsc_dump();

	// ICU
	print_stack_pointer();
	icu_dump();

	// TMU
	tmu_dump();

	// PFC , I/O port
	pfc_dump();

	// SCIF
	scif_dump(HPC_PREFERENCE.serial_speed);

	// HD64461
	platid_t platform;
	platform.dw.dw0 = menu._pref.platid_hi;
	platform.dw.dw1 = menu._pref.platid_lo;
	hd64461_dump(platform);

	SetKMode(_kmode);
}

void
SHArchitecture::icu_dump(void)
{

	DPRINTF((TEXT("<<<Interrupt Controller>>>\n")));
	print_stack_pointer();

	DPRINTF((TEXT("ICR0   0x%08x\n"), reg_read_2(ICU_ICR0_REG16)));
	DPRINTF((TEXT("ICR1   0x%08x\n"), reg_read_2(ICU_ICR1_REG16)));
	DPRINTF((TEXT("ICR2   0x%08x\n"), reg_read_2(ICU_ICR2_REG16)));
	DPRINTF((TEXT("PINTER 0x%08x\n"), reg_read_2(ICU_PINTER_REG16)));
	DPRINTF((TEXT("IPRA   0x%08x\n"), reg_read_2(ICU_IPRA_REG16)));
	DPRINTF((TEXT("IPRB   0x%08x\n"), reg_read_2(ICU_IPRB_REG16)));
	DPRINTF((TEXT("IPRC   0x%08x\n"), reg_read_2(ICU_IPRC_REG16)));
	DPRINTF((TEXT("IPRD   0x%08x\n"), reg_read_2(ICU_IPRD_REG16)));
	DPRINTF((TEXT("IPRE   0x%08x\n"), reg_read_2(ICU_IPRE_REG16)));
	DPRINTF((TEXT("IRR0   0x%08x\n"), reg_read_1(ICU_IRR0_REG8)));
	DPRINTF((TEXT("IRR1   0x%08x\n"), reg_read_1(ICU_IRR1_REG8)));
	DPRINTF((TEXT("IRR2   0x%08x\n"), reg_read_1(ICU_IRR2_REG8)));
	icu_control();
	icu_priority();
}

void
SHArchitecture::icu_priority(void)
{
	struct intr_priority *tab;

	DPRINTF((TEXT("----interrupt priority----\n")));
	for (tab = ipr_table; tab->name; tab++) {
		DPRINTF((TEXT("%-10S %d\n"), tab->name,
		    (reg_read_2(tab->reg) >> tab->shift) & ICU_IPR_MASK));
	}
	DPRINTF((TEXT("--------------------------\n")));
}

void
SHArchitecture::icu_control(void)
{
	const char *sense_select[] = {
		"falling edge",
		"raising edge",
		"low level",
		"reserved",
	};
	u_int16_t r;

	// PINT0-15
	DPRINTF((TEXT("PINT enable(on |)  :")));
	bitdisp(reg_read_2(ICU_PINTER_REG16));
	DPRINTF((TEXT("PINT detect(high |):")));
	bitdisp(reg_read_2(ICU_ICR2_REG16));
	// NMI
	r = reg_read_2(ICU_ICR0_REG16);
	DPRINTF((TEXT("NMI(%S %S-edge),"),
	    r & ICU_ICR0_NMIL ? "High" : "Low",
	    r & ICU_ICR0_NMIE ? "raising" : "falling"));
	r = reg_read_2(ICU_ICR1_REG16);
	DPRINTF((TEXT(" %S maskable,"), r & ICU_ICR1_MAI ? "" : "never"));
	DPRINTF((TEXT("  SR.BL %S\n"),
	    r & ICU_ICR1_BLMSK ? "ignored" : "maskable"));
	// IRQ0-5  
	DPRINTF((TEXT("IRQ[3:0]pin : %S mode\n"),
	    r & ICU_ICR1_IRQLVL ? "IRL 15level" : "IRQ[0:3]"));
	if (r & ICU_ICR1_IRQLVL) {
		DPRINTF((TEXT("IRLS[0:3] %S\n"),
		    r & ICU_ICR1_IRLSEN ? "enabled" : "disabled"));
	}
	// sense select
	for (int i = 5; i >= 0; i--) {
		DPRINTF((TEXT("IRQ[%d] %S\n"), i,
		    sense_select [
			    (r >>(i * 2)) & ICU_SENSE_SELECT_MASK]));
	}
}

SH_BOOT_FUNC_(7709);
SH_BOOT_FUNC_(7709A);
SH_BOOT_FUNC_(7750);

//
// Debug Functions.
//
void
SHArchitecture::bsc_dump()
{

	DPRINTF((TEXT("<<<Bus State Controller>>>\n")));
#define DUMP_BSC_REG(x)							\
	DPRINTF((TEXT("%-8S"), #x));					\
	bitdisp(reg_read_2(SH3_BSC_##x##_REG))
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
SHArchitecture::scif_dump(int bps)
{
	u_int16_t r16;
#ifdef SH4
	u_int16_t r8;
#else
	u_int8_t r8;
#endif
	int n;
	
	DPRINTF((TEXT("<<<SCIF>>>\n")));
	/* mode */
	r8 = SHREG_SCSMR2;
	n = 1 <<((r8 & SCSMR2_CKS) << 1);
	DPRINTF((TEXT("mode: %dbit %S-parity %d stop bit clock PCLOCK/%d\n"),
	    r8 & SCSMR2_CHR ? 7 : 8,
	    r8 & SCSMR2_PE  ? r8 & SCSMR2_OE ? "odd" : "even" : "non",
	    r8 & SCSMR2_STOP ? 2 : 1,
	    n));
	/* bit rate */
	r8 = SHREG_SCBRR2;
	DPRINTF((TEXT("SCBRR=%d(%dbps) estimated PCLOCK %dHz\n"), r8, bps,
	    32 * bps *(r8 + 1) * n));

	/* control */
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, SCSCR2_##m, #m)
	DPRINTF((TEXT("SCSCR2: ")));
	r8 = SHREG_SCSCR2;
	DBG_BIT_PRINT(r8, TIE);
	DBG_BIT_PRINT(r8, RIE);
	DBG_BIT_PRINT(r8, TE);
	DBG_BIT_PRINT(r8, RE);
	DPRINTF((TEXT("CKE=%d\n"), r8 & SCSCR2_CKE));
#undef	DBG_BIT_PRINT

	/* status */
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, SCSSR2_##m, #m)
	r16 = SHREG_SCSSR2;
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
	r8 = SHREG_SCFCR2;
	DPRINTF((TEXT("SCFCR2: ")));
	DBG_BIT_PRINT(r8, RTRG1);
	DBG_BIT_PRINT(r8, RTRG0);
	DBG_BIT_PRINT(r8, TTRG1);
	DBG_BIT_PRINT(r8, TTRG0);
	DBG_BIT_PRINT(r8, MCE);
	DBG_BIT_PRINT(r8, TFRST);
	DBG_BIT_PRINT(r8, RFRST);
	DBG_BIT_PRINT(r8, LOOP);
	DPRINTF((TEXT("\n")));
#undef	DBG_BIT_PRINT
}

void
SHArchitecture::pfc_dump()
{
	DPRINTF((TEXT("<<<Pin Function Controller>>>\n")));
	DPRINTF((TEXT("[control]\n")));
#define DUMP_PFC_REG(x)							\
	DPRINTF((TEXT("P%SCR :"), #x));					\
	bitdisp(reg_read_2(SH3_P##x##CR_REG16))
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
	bitdisp(reg_read_2(SH3_SCPCR_REG16));
	DPRINTF((TEXT("\n[data]\n")));
#define DUMP_IOPORT_REG(x)						\
	DPRINTF((TEXT("P%SDR :"), #x));					\
	bitdisp(reg_read_1(SH3_P##x##DR_REG8))
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
	bitdisp(reg_read_1(SH3_SCPDR_REG8));
}

void
SHArchitecture::tmu_dump()
{
	u_int8_t r8;
	
	DPRINTF((TEXT("<<<TMU>>>\n")));
	/* Common */
	/* TOCR  timer output control register */
	r8 = reg_read_1(SH3_TOCR_REG8);
	DPRINTF((TEXT("TCLK = %S\n"),
	    r8 & TOCR_TCOE ? "RTC output" : "input"));
	/* TSTR */
	r8 = reg_read_1(SH3_TSTR_REG8);
	DPRINTF((TEXT("Timer start(#0:2) [%c][%c][%c]\n"),
	    r8 & TSTR_STR0 ? 'x' : '_',
	    r8 & TSTR_STR1 ? 'x' : '_',
	    r8 & TSTR_STR2 ? 'x' : '_'));

#define CHANNEL_DUMP(a, x)						\
	tmu_channel_dump(x, SH##a##_TCOR##x##_REG,			\
			 SH##a##_TCNT##x##_REG,				\
			 SH##a##_TCR##x##_REG16)
	CHANNEL_DUMP(3, 0);
	CHANNEL_DUMP(3, 1);
	CHANNEL_DUMP(3, 2);
#undef	CHANNEL_DUMP
	DPRINTF((TEXT("\n")));
}

void
SHArchitecture::tmu_channel_dump(int unit, paddr_t tcor, paddr_t tcnt,
    paddr_t tcr)
{
	u_int32_t r32;
	u_int16_t r16;

	DPRINTF((TEXT("TMU#%d:"), unit));
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, TCR_##m, #m)
	/* TCR*/
	r16 = reg_read_2(tcr);
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
	r32 = reg_read_4(tcnt);
	DPRINTF((TEXT("\ncnt=0x%08x"), r32));
	/* TCOR0  timer constant register */
	r32 = reg_read_4(tcor);
	DPRINTF((TEXT(" constant=0x%04x"), r32));

	if (unit == 2)
		DPRINTF((TEXT(" input capture=0x%08x\n"), SH3_TCPR2_REG));
	else
		DPRINTF((TEXT("\n")));
}

void
SHArchitecture::hd64461_dump(platid_t &platform)
{
	u_int16_t r16;
	u_int8_t r8;

#define MATCH(p)						\
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
	u_int8_t *fb = reinterpret_cast<u_int8_t *>(HD64461_FBBASE);

	for (int i = 0; i < 320 * 240 * 2 / 8; i++)
		*fb++ = 0xff;
	DPRINTF((TEXT("frame buffer test end\n")));
#endif
	// System
	DPRINTF((TEXT("STBCR (System Control Register)\n")));
	r16 = reg_read_2(HD64461_SYSSTBCR_REG16);
	bitdisp(r16);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_SYSSTBCR_##m, #m)
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
	r16 = reg_read_2(HD64461_SYSSYSCR_REG16);
	bitdisp(r16);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_SYSSYSCR_##m, #m)
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
	r16 = reg_read_2(HD64461_SYSSCPUCR_REG16);
	bitdisp(r16);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_SYSSCPUCR_##m, #m)
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
	r16 = reg_read_2(HD64461_INTCNIRR_REG16);
	bitdisp(r16);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_INTCNIRR_##m, #m)
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
	r16 = reg_read_2(HD64461_INTCNIMR_REG16);
	bitdisp(r16);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_INTCNIMR_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC0ISR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0ISR_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC0GCR_REG8);	
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0GCR_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC0CSCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0CSCR_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC0CSCIER_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0CSCIER_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC0SCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC0SCR_##m, #m)
	DBG_BIT_PRINT(r8, P0VCC1);
	DBG_BIT_PRINT(r8, P0SWP);	
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	// PCC1
	DPRINTF((TEXT("[PCC1 memory card only (SH3 Area 5)]\n")));
	DPRINTF((TEXT("PCC1 Interface Status Register\n")));
	r8 = reg_read_1(HD64461_PCC1ISR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1ISR_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC1GCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1GCR_##m, #m)
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
	r8 = reg_read_1(HD64461_PCC1CSCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1CSCR_##m, #m)
	DBG_BIT_PRINT(r8, P1SCDI);
	DBG_BIT_PRINT(r8, P1CDC);
	DBG_BIT_PRINT(r8, P1RC);
	DBG_BIT_PRINT(r8, P1BW);
	DBG_BIT_PRINT(r8, P1BD);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Card Status Change Interrupt Enable Register\n")));
	r8 = reg_read_1(HD64461_PCC1CSCIER_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1CSCIER_##m, #m)
	DBG_BIT_PRINT(r8, P1CRE);
	DBG_BIT_PRINT(r8, P1CDE);
	DBG_BIT_PRINT(r8, P1RE);
	DBG_BIT_PRINT(r8, P1BWE);
	DBG_BIT_PRINT(r8, P1BDE);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Software Control Register\n")));
	r8 = reg_read_1(HD64461_PCC1SCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCC1SCR_##m, #m)
	DBG_BIT_PRINT(r8, P1VCC1);
	DBG_BIT_PRINT(r8, P1SWP);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	// General Control
	DPRINTF((TEXT("[General Control]\n")));
	DPRINTF((TEXT("PCC0 Output pins Control Register\n")));
	r8 = reg_read_1(HD64461_PCCP0OCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCCP0OCR_##m, #m)
	DBG_BIT_PRINT(r8, P0DEPLUP);
	DBG_BIT_PRINT(r8, P0AEPLUP);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PCC1 Output pins Control Register\n")));
	r8 = reg_read_1(HD64461_PCCP1OCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCCP1OCR_##m, #m)
	DBG_BIT_PRINT(r8, P1RST8MA);
	DBG_BIT_PRINT(r8, P1RST4MA);
	DBG_BIT_PRINT(r8, P1RAS8MA);
	DBG_BIT_PRINT(r8, P1RAS4MA);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	DPRINTF((TEXT("PC Card General Control Register\n")));
	r8 = reg_read_1(HD64461_PCCPGCR_REG8);
	bitdisp(r8);
#define DBG_BIT_PRINT(r, m)	_dbg_bit_print(r, HD64461_PCCPGCR_##m, #m)
	DBG_BIT_PRINT(r8, PSSDIR);
	DBG_BIT_PRINT(r8, PSSRDWR);
#undef DBG_BIT_PRINT
	DPRINTF((TEXT("\n")));

	// GPIO
#define GPIO_DUMP_REG8(x)						\
	bitdisp(reg_read_1(HD64461_GPA##x##R_REG16));			\
	bitdisp(reg_read_1(HD64461_GPB##x##R_REG16));			\
	bitdisp(reg_read_1(HD64461_GPC##x##R_REG16));			\
	bitdisp(reg_read_1(HD64461_GPD##x##R_REG16))
#define GPIO_DUMP_REG16(x)						\
	bitdisp(reg_read_2(HD64461_GPA##x##R_REG16));			\
	bitdisp(reg_read_2(HD64461_GPB##x##R_REG16));			\
	bitdisp(reg_read_2(HD64461_GPC##x##R_REG16));			\
	bitdisp(reg_read_2(HD64461_GPD##x##R_REG16))

	DPRINTF((TEXT("GPIO Port Control Register\n")));
	GPIO_DUMP_REG16(C);
	DPRINTF((TEXT("GPIO Port Data Register\n")));
	GPIO_DUMP_REG8(D);
	DPRINTF((TEXT("GPIO Port Interrupt Control Register\n")));
	GPIO_DUMP_REG8(IC);
	DPRINTF((TEXT("GPIO Port Interrupt Status  Register\n")));
	GPIO_DUMP_REG8(IS);
}

#ifdef SH7709TEST
u_int32_t sh7707_fb_dma_addr;
u_int16_t val;
int s;
	
s = suspendIntr();
VOLATILE_REF16(SH7707_LCDAR_REG16) = SH7707_LCDAR_LCDDMR0;
val = VOLATILE_REF16(SH7707_LCDDMR_REG16);
sh7707_fb_dma_addr = val;
VOLATILE_REF16(SH7707_LCDAR_REG16) = SH7707_LCDAR_LCDDMR1;	
val = VOLATILE_REF16(SH7707_LCDDMR_REG16);
sh7707_fb_dma_addr |= (val << 16);
resumeIntr(s);

DPRINTF((TEXT("SH7707 frame buffer dma address: 0x%08x\n"),
    sh7707_fb_dma_addr));
#endif
