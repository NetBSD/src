/*	$NetBSD: arm_boot.cpp,v 1.2 2001/03/11 11:47:24 toshii Exp $	*/

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

#include <arch.h>
#include <memory.h>

#include <arm/arm_arch.h>
#include <arm/arm_boot.h>
#include <arm/arm_console.h>

ARMBoot::ARMBoot(void)
{
}

ARMBoot::~ARMBoot(void)
{
	if (_mem)
		delete _mem;
	if (_arch)
		delete _arch;

	ARMConsole::Destroy();
}

BOOL
ARMBoot::setup(struct HpcMenuInterface::HpcMenuPreferences &pref)
{
	platid_t platid;
	platid.dw.dw0 = pref.platid_hi;
	platid.dw.dw1 = pref.platid_lo;

	if (platid_match(&platid, &platid_mask_CPU_ARM_STRONGARM_SA1100))
		args.architecture = ARCHITECTURE_ARM_SA1100;
	else if (platid_match(&platid, &platid_mask_CPU_ARM_STRONGARM_SA1110))
		args.architecture = ARCHITECTURE_ARM_SA1100;
	else
		return FALSE;

	args.memory = MEMORY_MANAGER_LOCKPAGES;

	return Boot::setup(pref);
}

BOOL
ARMBoot::create(void)
{
	BOOL(*lock_pages)(LPVOID, DWORD, PDWORD, int);
	BOOL(*unlock_pages)(LPVOID, DWORD);

	// Architercure dependent ops.
	_arch = new ARMArchitecture(_cons, _mem);
	_arch->setDebug() = args.architectureDebug;

	lock_pages = _arch->_load_LockPages();
	unlock_pages = _arch->_load_UnlockPages();
	if (lock_pages == 0 || unlock_pages == 0) {

		DPRINTF((TEXT("couldn't find LockPages/UnlockPages.\n")));
		return FALSE;
	}

	// Memory manager.
	switch(args.memory)
	{
	default:
	case MEMORY_MANAGER_VIRTUALCOPY:
		// FALLTHROUGH
	case MEMORY_MANAGER_SOFTMMU:
		// FALLTHROUGH
	case MEMORY_MANAGER_HARDMMU:
		DPRINTF((TEXT("unsupported memory address detection method.\n")));
		return FALSE;
	case MEMORY_MANAGER_LOCKPAGES:
		_mem = new MemoryManager_LockPages(lock_pages, unlock_pages,
						   _cons, 4096);
		break;
	}
	_mem->setDebug() = args.memorymanagerDebug;

	// Console
	if (args.console == CONSOLE_SERIAL) {
		_cons = ARMConsole::Instance(_mem);
		if (!_cons->init()) {
			_cons = Console::Instance();
			DPRINTF((TEXT("use LCD console instead.\n")));
		}
	}
  
	// File Manager, Loader
	return Boot::create();
}
