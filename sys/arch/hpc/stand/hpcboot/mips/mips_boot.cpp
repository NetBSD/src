/* -*-C++-*-	$NetBSD: mips_boot.cpp,v 1.2 2001/04/24 19:28:00 uch Exp $	*/

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

#include <hpcboot.h>
#include <console.h>

#include <arch.h>
#include <memory.h>

#include <mips/mips_arch.h>
#include <mips/mips_boot.h>
#include <mips/mips_vr41.h>
#include <mips/mips_tx39.h>
#include <mips/mips_console.h>

MIPSBoot::MIPSBoot()
{
}

MIPSBoot::~MIPSBoot()
{
	if (_mem)
		delete _mem;
	if (_arch)
		delete _arch;

	MIPSConsole::Destroy();
}

BOOL
MIPSBoot::setup()
{
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;

	platid_t platid;
	platid.dw.dw0 = pref.platid_hi;
	platid.dw.dw1 = pref.platid_lo;

	if (platid_match(&platid, &platid_mask_CPU_MIPS_VR)) {
		args.architecture = ARCHITECTURE_MIPS_VR41;
	} else if (platid_match(&platid, &platid_mask_CPU_MIPS_TX_3900)) {
		args.architecture = ARCHITECTURE_MIPS_TX3900;
	} else if (platid_match(&platid, &platid_mask_CPU_MIPS_TX_3920)) {
		args.architecture = ARCHITECTURE_MIPS_TX3920;
	} else {
		DPRINTF((TEXT("unknown MIPS variants.\n")));
		return FALSE;
	}

	return super::setup();
}

BOOL
MIPSBoot::create()
{
	size_t pagesz;
	int shift;
	BOOL(*lock_pages)(LPVOID, DWORD, PDWORD, int);
	BOOL(*unlock_pages)(LPVOID, DWORD);

	// Console
	if (args.console == CONSOLE_SERIAL) {
		_cons = MIPSConsole::Instance();
		if (!_cons->init()) {
			_cons = Console::Instance();
			DPRINTF((TEXT("use LCD console instead.\n")));
		}
	}

	// Architercure dependent ops.
	switch(args.architecture) {
	default:
		DPRINTF((TEXT("unsupported architecture.\n")));
		return FALSE;
	case ARCHITECTURE_MIPS_TX3900:
		// FALLTHROUGH
	case ARCHITECTURE_MIPS_TX3920:
		_arch = new TX39XX(_cons, _mem, args.architecture);
		pagesz = 4096;
		shift = 0;
		break;
	case ARCHITECTURE_MIPS_VR41:
		_arch = new VR41XX(_cons, _mem);
		pagesz = 1024;
		shift = 4; // VR41 specific shift. for LockPages()
		break;
	}
	_arch->setDebug() = args.architectureDebug;

	// Memory manager.
	// LockPages API exists in Windows CE 2.11 or later. check it.
	lock_pages = _arch->_load_LockPages();
	unlock_pages = _arch->_load_UnlockPages();
	if (lock_pages == 0 || unlock_pages == 0)
		args.memory = MEMORY_MANAGER_VIRTUALCOPY;
	else
		args.memory = MEMORY_MANAGER_LOCKPAGES;

	switch(args.memory) {
	default:
		break;
	case MEMORY_MANAGER_SOFTMMU:
		// FALLTHROUGH
	case MEMORY_MANAGER_HARDMMU:
		DPRINTF((TEXT("unsupported memory address detection method.\n")));
		return FALSE;
	case MEMORY_MANAGER_LOCKPAGES:
		_mem = new MemoryManager_LockPages(lock_pages, unlock_pages,
						   _cons, pagesz, shift);
		break;
	case MEMORY_MANAGER_VIRTUALCOPY:
		_mem = new MemoryManager_VirtualCopy(_cons, pagesz);
		break;
	}
	_mem->setDebug() = args.memorymanagerDebug;

  
	// File Manager, Loader
	return super::create();
}
