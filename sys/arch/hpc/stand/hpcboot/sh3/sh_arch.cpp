/*	$NetBSD: sh_arch.cpp,v 1.1 2001/02/09 18:35:16 uch Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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

#include <sh3/sh_arch.h>
#include "scifreg.h"

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
	DPRINTF((TEXT("2nd bootloader copy done.\n")));

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
	DPRINTF((TEXT("BootArgs 0x%08x Stack 0x%08x\nBooting kernel...\n"),
		 info, sp));

	// Change to privilege-mode.
	SetKMode(1);

	// Disable external interrupt.
	suspendIntr();

	// Cache flush(for 2nd bootloader)
	cache_flush();

	// jump to 2nd loader.(run P1) at this time I still use MMU.
	__asm("mov	r6, r15\n"
	      "jmp	@r7\n"
	      "nop\n", info, pvec, sp, _loader_addr);
	// NOTREACHED
}

// disable external interrupt and save its priority.
u_int32_t
suspendIntr(void)
{
	u_int32_t sr;
	__asm("stc	sr, r0\n"
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

	Architecture::systemInfo();

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

	// SCIF
	scif_dump(19200);
	
	// ICU
	print_stack_pointer();
	icu_dump();

#if 0	// Frame Buffer (this test is destructive.)
	hd64461_framebuffer_test();
#endif

	SetKMode(_kmode);
}

void
SHArchitecture::icu_dump(void)
{
	print_stack_pointer();

	DPRINTF((TEXT("ICR0   0x%08x\n"), reg_read16(ICU_ICR0_REG16)));
	DPRINTF((TEXT("ICR1   0x%08x\n"), reg_read16(ICU_ICR1_REG16)));
	DPRINTF((TEXT("ICR2   0x%08x\n"), reg_read16(ICU_ICR2_REG16)));
	DPRINTF((TEXT("PINTER 0x%08x\n"), reg_read16(ICU_PINTER_REG16)));
	DPRINTF((TEXT("IPRA   0x%08x\n"), reg_read16(ICU_IPRA_REG16)));
	DPRINTF((TEXT("IPRB   0x%08x\n"), reg_read16(ICU_IPRB_REG16)));
	DPRINTF((TEXT("IPRC   0x%08x\n"), reg_read16(ICU_IPRC_REG16)));
	DPRINTF((TEXT("IPRD   0x%08x\n"), reg_read16(ICU_IPRD_REG16)));
	DPRINTF((TEXT("IPRE   0x%08x\n"), reg_read16(ICU_IPRE_REG16)));
	DPRINTF((TEXT("IRR0   0x%08x\n"), reg_read8(ICU_IRR0_REG8)));
	DPRINTF((TEXT("IRR1   0x%08x\n"), reg_read8(ICU_IRR1_REG8)));
	DPRINTF((TEXT("IRR2   0x%08x\n"), reg_read8(ICU_IRR2_REG8)));
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
			 (reg_read16(tab->reg) >> tab->shift) & ICU_IPR_MASK));
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
	bitdisp(reg_read16(ICU_PINTER_REG16));
	DPRINTF((TEXT("PINT detect(high |):")));
	bitdisp(reg_read16(ICU_ICR2_REG16));
	// NMI
	r = reg_read16(ICU_ICR0_REG16);
	DPRINTF((TEXT("NMI(%S %S-edge),"),
		 r & ICU_ICR0_NMIL ? "High" : "Low",
		 r & ICU_ICR0_NMIE ? "raising" : "falling"));
	r = reg_read16(ICU_ICR1_REG16);
	DPRINTF((TEXT(" %S maskable,"), r & ICU_ICR1_MAI ? "" : "never"));
	DPRINTF((TEXT("  SR.BL %S\n"),
		 r & ICU_ICR1_BLMSK ? "ignored" : "maskable"));
	// IRQ0-5  
	DPRINTF((TEXT("IRQ[3:0] : %S source\n"),
		 r & ICU_ICR1_IRQLVL ? "IRL 15level" :
		 "dependent IRQ[0:3](IRL disabled)"));
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

//
// Debug Functions.
//
void
SHArchitecture::scif_dump(int bps)
{
	u_int16_t r16;
	u_int8_t r8;
	int n;
	
	/* mode */
	r8 = SHREG_SCSMR2;
	n = 1 <<((r8 & SCSMR2_CKS) << 1);
	DPRINTF((TEXT("mode: %dbit %S-parity %d stop bit clock PCLOCK/%d\n"),
		 r8 & SCSMR2_CHR ? 7 : 8,
		 r8 & SCSMR2_PE	? r8 & SCSMR2_OE ? "odd" : "even" : "non",
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
SHArchitecture::hd64461_framebuffer_test()
{
	DPRINTF((TEXT("frame buffer test start\n")));
#if SH7709TEST
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
#else
	u_int8_t *fb = reinterpret_cast<u_int8_t *>(HD64461_FB_ADDR);

	for (int i = 0; i < 480 * 240 * 2 / 8; i++)
		*fb++ = 0xff;
#endif
	DPRINTF((TEXT("frame buffer test end\n")));
}
