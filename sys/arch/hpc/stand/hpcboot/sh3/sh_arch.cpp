/*	$NetBSD: sh_arch.cpp,v 1.7.2.2 2002/03/16 15:57:52 jdolecek Exp $	*/

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

SH_BOOT_FUNC_(7709);
SH_BOOT_FUNC_(7709A);
SH_BOOT_FUNC_(7750);

static int _cpu_type;

int
SHArchitecture::cpu_type()
{
	if (_cpu_type == 0) {
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		_cpu_type = si.wProcessorLevel;
	}

	return _cpu_type;
}

BOOL
SHArchitecture::init()
{

	if (!_mem->init()) {
		DPRINTF((TEXT("can't initialize memory manager.\n")));
		return FALSE;
	}
	// D-RAM information
	DPRINTF((TEXT("Memory Bank:\n")));

	return TRUE;
}

void
SHArchitecture::systemInfo()
{

	// Windows CE common infomation.
	super::systemInfo();

	// CPU specific.
	_dev->dump(HPC_MENU._cons_parameter);
}

BOOL
SHArchitecture::setupLoader()
{
	vaddr_t v;

	if (!_mem->getPage(v , _loader_addr)) {
		DPRINTF((TEXT("can't get page for 2nd loader.\n")));
		return FALSE;
	}
	_loader_addr = ptokv(_loader_addr);

	DPRINTF((TEXT("2nd bootloader address U0: 0x%08x P1: 0x%08x\n"),
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
suspendIntr()
{
	u_int32_t sr;

	__asm(
	    "stc	sr, r0\n"
	    "mov.l	r0, @r4\n"
	    "or		r5, r0\n"
	    "ldc	r0, sr\n", &sr, 0x000000f0);
	return sr & 0x000000f0;
}

// resume external interrupt priority.
void
resumeIntr(u_int32_t s)
{

	__asm(
	    "stc	sr, r0\n"
	    "and	r5, r0\n"
	    "or		r4, r0\n"
	    "ldc	r0, sr\n", s, 0xffffff0f);
}
