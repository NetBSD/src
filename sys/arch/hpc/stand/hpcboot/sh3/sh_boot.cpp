/*	$NetBSD: sh_boot.cpp,v 1.6 2002/02/11 17:08:56 uch Exp $	*/

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

#include <arch.h>
#include <memory.h>

#include <sh3/sh_arch.h>
#include <sh3/sh_mmu.h>
#include <sh3/sh_console.h>
#include <sh3/sh_boot.h>

SHBoot::SHBoot()
{
}

SHBoot::~SHBoot()
{
	if (_mem)
		delete _mem;
	if (_arch)
		delete _arch;

	SHConsole::Destroy();
}

BOOL
SHBoot::setup()
{
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;

	platid_t platid;
	platid.dw.dw0 = pref.platid_hi;
	platid.dw.dw1 = pref.platid_lo;

	if (platid_match(&platid, &platid_mask_CPU_SH_3_7709)) {
		args.architecture = ARCHITECTURE_SH3_7709;
	} else if (platid_match(&platid, &platid_mask_CPU_SH_3_7709A)) {
		args.architecture = ARCHITECTURE_SH3_7709A;
	} else if (platid_match(&platid, &platid_mask_CPU_SH_4_7750)) {
		args.architecture = ARCHITECTURE_SH4_7750;
	} else {
		DPRINTF((TEXT("CPU not supported.")));
		return FALSE;
	}

	return super::setup();
}

BOOL
SHBoot::create()
{
	BOOL(*lock_pages)(LPVOID, DWORD, PDWORD, int);
	BOOL(*unlock_pages)(LPVOID, DWORD);
	size_t page_size;

	// Setup console. this setting is passed to kernel bootinfo.
	if (args.console == CONSOLE_SERIAL) {
		_cons = new SHConsole;
		if (!_cons->init()) {
			_cons = Console::Instance();
			DPRINTF((TEXT("use LCD console instead.\n")));
		}
	} else {
		_cons = Console::Instance();
		SHConsole::selectBootConsole(*_cons, SHConsole::VIDEO);
	}

	// Architecture dependent ops.
	switch(args.architecture) {
	default:
		DPRINTF((TEXT("unsupported architecture.\n")));
		return FALSE;
	case ARCHITECTURE_SH3_7709:
		_arch = new SH7709(_cons, _mem, SH7709::boot_func);      
		page_size = SH3_PAGE_SIZE;
		if (SHArchitecture::cpu_type() != 3)
			goto cpu_mismatch;
		break;
	case ARCHITECTURE_SH3_7709A:
		_arch = new SH7709A(_cons, _mem, SH7709A::boot_func);
		page_size = SH3_PAGE_SIZE;
		if (SHArchitecture::cpu_type() != 3)
			goto cpu_mismatch;
		break;
	case ARCHITECTURE_SH4_7750:
		_arch = new SH7750(_cons, _mem, SH7750::boot_func);
		page_size = SH4_PAGE_SIZE;
		if (SHArchitecture::cpu_type() != 4)
			goto cpu_mismatch;
		break;
	}
	_arch->setDebug() = args.architectureDebug;

	lock_pages = _arch->_load_LockPages();
	unlock_pages = _arch->_load_UnlockPages();
	if (lock_pages == 0 || unlock_pages == 0)
		args.memory = MEMORY_MANAGER_HARDMMU;
	else
		args.memory = MEMORY_MANAGER_LOCKPAGES;

	// Memory manager.
	switch(args.memory) {
	default:
	case MEMORY_MANAGER_VIRTUALCOPY:
		// VirtualCopy method causes Windows CE unstable.
		/* FALLTHROUGH */
	case MEMORY_MANAGER_SOFTMMU:
		DPRINTF((TEXT("unsupported address detection method.\n")));
		return FALSE;
	case MEMORY_MANAGER_HARDMMU:
		if (args.architecture == ARCHITECTURE_SH4_7750) {
			DPRINTF((TEXT("No SH4 MMU code.\n")));
			return FALSE;
		}

		_mem = new MemoryManager_SHMMU(_cons, page_size);
		break;
	case MEMORY_MANAGER_LOCKPAGES:
		_mem = new MemoryManager_LockPages(lock_pages, unlock_pages,
		    _cons, page_size);
		break;
	}
	_mem->setDebug() = args.memorymanagerDebug;

	// File Manager, Loader
	return super::create();

 cpu_mismatch:
	DPRINTF((TEXT("CPU mismatch. CPU is SH%d\n"),
	    SHArchitecture::cpu_type()));

	return FALSE;
}
