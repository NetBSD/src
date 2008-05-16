/*	$NetBSD: arm_sa1100.cpp,v 1.1.12.1 2008/05/16 02:22:24 yamt Exp $	*/

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

#include <arm/arm_arch.h>
#include <console.h>
#include <memory.h>
#include <arm/arm_sa1100.h>

/*
 * Strong-ARM SA-1100 (e.g. HP-820 JORNADA)
 */

#define	PAGE_SIZE		0x1000
#define	DRAM_BANK_NUM		4		/* total 512MByte */
#define	DRAM_BANK_SIZE		0x08000000	/* 128Mbyte */

#define	DRAM_BANK0_START	0xc0000000
#define	DRAM_BANK0_SIZE		DRAM_BANK_SIZE
#define	DRAM_BANK1_START	0xc8000000
#define	DRAM_BANK1_SIZE		DRAM_BANK_SIZE
#define	DRAM_BANK2_START	0xd0000000
#define	DRAM_BANK2_SIZE		DRAM_BANK_SIZE
#define	DRAM_BANK3_START	0xd8000000
#define	DRAM_BANK3_SIZE		DRAM_BANK_SIZE
#define	ZERO_BANK_START		0xe0000000
#define	ZERO_BANK_SIZE		DRAM_BANK_SIZE

__BEGIN_DECLS

// 2nd bootloader
void boot_func(kaddr_t, kaddr_t, kaddr_t, kaddr_t);
extern char boot_func_end[];
#define	BOOT_FUNC_START		reinterpret_cast <vaddr_t>(boot_func)
#define	BOOT_FUNC_END		reinterpret_cast <vaddr_t>(boot_func_end)

/* jump to 2nd loader */
void FlatJump(kaddr_t, kaddr_t, kaddr_t, kaddr_t);

__END_DECLS

SA1100Architecture::SA1100Architecture(Console *&cons, MemoryManager *&mem)
	: ARMArchitecture(cons, mem)
{
	DPRINTF((TEXT("SA-11x0 CPU.\n")));
}

SA1100Architecture::~SA1100Architecture(void)
{
}

BOOL
SA1100Architecture::init(void)
{
	if (!_mem->init()) {
		DPRINTF((TEXT("can't initialize memory manager.\n")));
		return FALSE;
	}
	// set D-RAM information
	_mem->loadBank(DRAM_BANK0_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK1_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK2_START, DRAM_BANK_SIZE);
	_mem->loadBank(DRAM_BANK3_START, DRAM_BANK_SIZE);

	return TRUE;
}

void
SA1100Architecture::testFramebuffer(void)
{
	// get frame buffer address from LCD controller register.
	paddr_t fbaddr_p = _mem->readPhysical4(0xb0100010); // 0xc0002e00
	// map frame buffer
	vaddr_t fbaddr_v = _mem->mapPhysicalPage(fbaddr_p, 0x50000,
	    PAGE_READWRITE);

	// test frame buffer
	int j, k;
	DI();
	for (j = 0; j < 480; j++)
		for (k = 0; k < 640; k++)
			VOLATILE_REF8(fbaddr_v + 0x200 + j * 640 + k)
			    = j * k & 0xff;
	for (j = 120; j < 360; j++)
		for (k = 120; k < 520; k++)
			VOLATILE_REF8(fbaddr_v + 0x200 + j * 640 + k) = 0x3;
	EI();
	_mem->unmapPhysicalPage(fbaddr_v);
}

void
SA1100Architecture::testUART(void)
{
#define	TBY			VOLATILE_REF(uart + 0x20)
#define	UTDR			VOLATILE_REF(uart + 0x14)
#define	TBY_BUSY		while (TBY & 0x1)
#define	UTDR_PUTCHAR(c)		(UTDR =(c))
#define	_(c)								\
__BEGIN_MACRO								\
	TBY_BUSY;							\
	UTDR_PUTCHAR(c);						\
__END_MACRO
	vaddr_t uart =
	    _mem->mapPhysicalPage(0x80050000, 0x100, PAGE_READWRITE);
	_('H');_('e');_('l');_('l');_('o');_(' ');
	_('W');_('o');_('r');_('l');_('d');_('\r');_('\n');
	_mem->unmapPhysicalPage(uart);
}

BOOL
SA1100Architecture::setupLoader(void)
{
	vaddr_t v;
	vsize_t sz = BOOT_FUNC_END - BOOT_FUNC_START;

	// check 2nd bootloader size.
	if (sz > _mem->getPageSize()) {
		DPRINTF((TEXT("2nd bootloader size(%dbyte) is larger than page size(%d).\n"),
		    sz, _mem->getPageSize()));
		return FALSE;
	}

	// get physical mapped page and copy loader to there.
	// don't writeback D-cache here. make sure to writeback before jump.
	if (!_mem->getPage(v , _loader_addr)) {
		DPRINTF((TEXT("can't get page for 2nd loader.\n")));
		return FALSE;
	}
	DPRINTF((TEXT("2nd bootloader vaddr=0x%08x paddr=0x%08x\n"),
	    (unsigned)v,(unsigned)_loader_addr));

	memcpy(reinterpret_cast <LPVOID>(v),
	    reinterpret_cast <LPVOID>(BOOT_FUNC_START), sz);
	DPRINTF((TEXT("2nd bootloader copy done.\n")));

	return TRUE;
}

void
SA1100Architecture::jump(paddr_t info, paddr_t pvec)
{
	kaddr_t sp;
	vaddr_t v;
	paddr_t p;

	// stack for bootloader
	_mem->getPage(v, p);
	sp = ptokv(p) + _mem->getPageSize();
	DPRINTF((TEXT("sp for bootloader = %08x + %08x = %08x\n"),
	    ptokv(p), _mem->getPageSize(), sp));

	// writeback whole D-cache
	WritebackDCache();

	SetKMode(1);
	FlatJump(info, pvec, sp, _loader_addr);
	// NOTREACHED
}
