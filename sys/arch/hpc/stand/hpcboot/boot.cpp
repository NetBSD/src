/*	$NetBSD: boot.cpp,v 1.2 2001/04/24 19:27:58 uch Exp $	*/

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

#include <hpcdefs.h>
#include <hpcboot.h>
#include <boot.h>

#include <load.h>
#include <load_elf.h>
#include <load_coff.h>
#undef DPRINTF // trash coff_machdep.h's DPRINTF

#include <console.h>

#include <file.h>
#include <file_fat.h>
#include <file_http.h>
#include <file_ufs.h>

#ifdef ARM
#include <arm/arm_boot.h>
#endif
#ifdef MIPS
#include <mips/mips_boot.h>
#endif
#ifdef SHx
#include <sh3/sh_boot.h>
#endif

Boot *Boot::_instance = 0;

Boot &
Boot::Instance()
{
	if (_instance)
		return  *_instance;

	// register bootloader to menu system.
	// (will be invoked by boot button)
	struct HpcMenuInterface::boot_hook_args bha;
	bha.func	= hpcboot;
	bha.arg		= 0;
	HPC_MENU.register_boot_hook(bha);

#ifdef ARM
	_instance = new ARMBoot();
#endif
#ifdef MIPS
	_instance = new MIPSBoot();
#endif
#ifdef SHx
	_instance = new SHBoot();
#endif
  
	memset(&_instance->args, 0, sizeof(struct BootSetupArgs));
	_instance->args.consoleEnable = TRUE;

	return *_instance;
}

void
Boot::Destroy()
{
	if (_instance)
		delete _instance;
}

BOOL
Boot::setup()
{
	struct HpcMenuInterface::HpcMenuPreferences &pref = HPC_PREFERENCE;

	args.console = pref.boot_serial ? CONSOLE_SERIAL : CONSOLE_LCD;

	// file path.
	TCHAR *dir = pref.dir_user_path;
	if (wcsstr(dir, TEXT("http://")))
		args.file = FILE_HTTP;
	else
		args.file = wcschr(dir, TEXT('/')) ? FILE_UFS : FILE_FAT;
	wcscpy(args.fileRoot, dir);

	// file name.
	wcscpy(args.fileName, pref.kernel_user_file);
	args.loadmfs = (pref.rootfs == 2);
	if (args.loadmfs)
		wcscpy(args.mfsName, pref.rootfs_file);

	// debug options.
	args.loaderDebug	= FALSE;
	args.architectureDebug	= FALSE;
	args.memorymanagerDebug	= FALSE;
	args.fileDebug		= FALSE;
  
	return TRUE;
}

Boot::Boot()
{
	_arch	= 0;
	_mem	= 0;
	_file	= 0;
	_loader	= 0;
	// set default console
	_cons = Console::Instance();
}

Boot::~Boot()
{
	if (_file)
		delete _file;
	if (_loader)
		delete _loader;
}

BOOL
Boot::create()
{
	// File manager.
	_file = new FileManager(_cons, args.file);
	_file->setDebug() = args.fileDebug;

	return TRUE;
}

BOOL
Boot::attachLoader()
{
	switch (Loader::objectFormat(*_file)) {
	case LOADER_ELF:
		_loader = new ElfLoader(_cons, _mem);
		break;
	case LOADER_COFF:
		_loader = new CoffLoader(_cons, _mem);
		break;
	default:
		DPRINTF((TEXT("unknown file format.\n")));
		return FALSE;
	}

	return TRUE;
}
