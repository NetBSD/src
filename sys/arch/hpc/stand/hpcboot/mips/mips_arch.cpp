/* -*-C++-*-	$NetBSD: mips_arch.cpp,v 1.2 2001/05/08 18:51:25 uch Exp $	*/

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
#undef DEBUG_KERNADDR_ACCESS
#undef DEBUG_CP0_ACCESS

#include <hpcboot.h>
#include <mips/mips_arch.h>
#include <console.h>
#include <memory.h>

MIPSArchitecture::MIPSArchitecture(Console *&cons, MemoryManager *&mem)
	: Architecture(cons, mem)
{
	/* NO-OP */
}

MIPSArchitecture::~MIPSArchitecture(void)
{
	/* NO-OP */
}

void
MIPSArchitecture::systemInfo()
{
	u_int32_t r0, r1;
	Architecture::systemInfo();
	r0 = r1 = 0;

#ifdef DEBUG_CP0_ACCESS
	/* CP0 access test */
	_kmode = SetKMode(1);

	DPRINTF((TEXT("status register test\n")));
	GET_SR(r0);
	DPRINTF((TEXT("current value: 0x%08x\n"), r0));
	SET_SR(r1);
	GET_SR(r1);
	DPRINTF((TEXT("write test:    0x%08x\n"), r1));
	SET_SR(r0);

	SetKMode(_kmode);
#endif // DEBUG_CP0_ACCESS
}

BOOL
MIPSArchitecture::init()
{
	if (!_mem->init()) {
		DPRINTF((TEXT("can't initialize memory manager.\n")));
		return FALSE;
	}

	return TRUE;
}

BOOL
MIPSArchitecture::setupLoader()
{
	vaddr_t v;

#ifdef DEBUG_KERNADDR_ACCESS // kernel address access test
#define TEST_MAGIC		0xac1dcafe
	paddr_t p;
	u_int32_t r0;

	_kmode = SetKMode(1);
	_mem->getPage(v, p);
	VOLATILE_REF(ptokv(p)) = TEST_MAGIC;
	cacheFlush();
	r0 = VOLATILE_REF(v);
	DPRINTF((TEXT("kernel address access test: %S\n"),
	    r0 == TEST_MAGIC ? "OK" : "NG"));
	SetKMode(_kmode);
#endif // DEBUG_KERNADDR_ACCESS

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
MIPSArchitecture::jump(paddr_t info, paddr_t pvec)
{
	kaddr_t sp;
	vaddr_t v;
	paddr_t p;

	// stack for bootloader(but mips loader don't use stack)
	_mem->getPage(v, p);
	sp = ptokv(p + _mem->getPageSize() - 0x10);

	info = ptokv(info);
	pvec = ptokv(pvec);
	_loader_addr = ptokv(_loader_addr);

	// switch kernel mode.
	SetKMode(1);
	if (SetKMode(1) != 1) {
		DPRINTF((TEXT("SetKMode(1) failed.\n")));
		return;
	}
	DPRINTF((TEXT("jump to 0x%08x(info=0x%08x, pvec=0x%08x\n"),
	    _loader_addr, info, pvec));

	// writeback whole D-cache and invalidate whole I-cache.
	// 2nd boot-loader access data via kseg0 which were writed via kuseg,
	cacheFlush();

	// jump to 2nd-loader(run kseg0)
	__asm(".set noreorder;"
	    "jr	a3;"
	    "move	sp, a2;"
	    ".set reorder", info, pvec, sp, _loader_addr);
	// NOTREACHED
}
