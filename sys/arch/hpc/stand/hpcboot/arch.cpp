/* -*-C++-*-	$NetBSD: arch.cpp,v 1.3.8.3 2002/04/01 07:40:13 nathanw Exp $	 */

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

#include <menu/window.h>
#include <menu/rootwindow.h>	// MessageBox

#include <console.h>
#include <memory.h>
#include <load.h>
#include <arch.h>
#include <framebuffer.h>

Architecture::Architecture(Console *&cons, MemoryManager *&mem)
	:_cons(cons), _mem(mem)
{

	_loader_addr = 0;
	_debug = FALSE;
	_dll = 0;
}

Architecture::~Architecture(void)
{

	if (_dll)
		FreeLibrary(_dll);
}

BOOL
Architecture::allocateMemory(size_t sz)
{
	//binary image.
	sz = _mem->estimateTaggedPageSize(sz);
	//pvec + BootArgs + 2 nd bootloader + bootloader stack.
	sz += _mem->getPageSize() * 4;
	sz = _mem->roundRegion(sz);
	return _mem->reservePage(sz);
}

paddr_t
Architecture::setupBootInfo(Loader &loader)
{
	HpcMenuInterface &menu = HpcMenuInterface::Instance();
	vaddr_t v;
	paddr_t p;

	_mem->getPage(v, p);
	_boot_arg = reinterpret_cast <struct BootArgs *>(v);

	_boot_arg->argc = menu.setup_kernel_args(v + sizeof(struct BootArgs),
	    p + sizeof(struct BootArgs),
	    _mem->getTaggedPageSize() - sizeof(struct BootArgs));
	_boot_arg->argv = ptokv(p + sizeof(struct BootArgs));
	menu.setup_bootinfo(_boot_arg->bi);
	_boot_arg->bi.bi_cnuse = _cons->getBootConsole();

	_boot_arg->bootinfo = ptokv(p + offsetof(struct BootArgs, bi));
	_boot_arg->kernel_entry = loader.jumpAddr();

	struct bootinfo &bi = _boot_arg->bi;
	DPRINTF((TEXT("framebuffer: %dx%d type=%d linebytes=%d addr=0x%08x\n"),
	    bi.fb_width, bi.fb_height, bi.fb_type, bi.fb_line_bytes,
	    bi.fb_addr));
	DPRINTF((TEXT("console = %d\n"), bi.bi_cnuse));

	return p;
}

void *
Architecture::_load_func(const TCHAR * name)
{

	if (_dll == NULL)
		_dll = LoadLibrary(TEXT("coredll.dll"));

	if (_dll == NULL) {
		MessageBox(HpcMenuInterface::Instance()._root->_window,
		    TEXT("Can't load Coredll.dll"), TEXT("WARNING"), 0);

		return NULL;
	}
	
	return reinterpret_cast <void *>(GetProcAddress(_dll, name));
}

void
Architecture::systemInfo(void)
{
	u_int32_t val = 0;
	SYSTEM_INFO si;
	HDC hdc;
	BOOL (*getVersionEx)(LPOSVERSIONINFO);
	
	//
	// WCE200 ... GetVersionEx
	// WCE210 or later ... GetVersionExA or GetVersionExW
	// see winbase.h
	//
	getVersionEx = reinterpret_cast <BOOL(*)(LPOSVERSIONINFO)>
	    (_load_func(TEXT("GetVersionEx")));
	
	if (getVersionEx) {
		getVersionEx(&WinCEVersion);
		DPRINTF((TEXT("GetVersionEx\n")));
	} else {
		GetVersionEx(&WinCEVersion);
		DPRINTF((TEXT("GetVersionExW\n")));
	}

	DPRINTF((TEXT("Windows CE %d.%d\n"), WinCEVersion.dwMajorVersion,
	    WinCEVersion.dwMinorVersion));

	GetSystemInfo(&si);
	DPRINTF((TEXT("GetSystemInfo:\n")));
	DPRINTF((TEXT("wProcessorArchitecture      0x%x\n"),
	    si.wProcessorArchitecture)); 
	DPRINTF((TEXT("dwPageSize                  0x%x\n"),
	    si.dwPageSize)); 
	DPRINTF((TEXT("dwAllocationGranularity     0x%08x\n"),
	    si.dwAllocationGranularity)); 
	DPRINTF((TEXT("dwProcessorType             0x%x\n"),
	    si.dwProcessorType)); 
	DPRINTF((TEXT("wProcessorLevel             0x%x\n"),
	    si.wProcessorLevel)); 
	DPRINTF((TEXT("wProcessorRevision          0x%x\n"),
	    si.wProcessorRevision)); 

	// inquire default setting.
	FrameBufferInfo fb(0, 0);
	DPRINTF((TEXT("Display: %dx%d %dbpp\n"), fb.width(), fb.height(),
	    fb.bpp()));

	ReleaseDC(0, hdc);
}

BOOL(*Architecture::_load_LockPages(void))(LPVOID, DWORD, PDWORD, int)
{

	return reinterpret_cast <BOOL(*)(LPVOID, DWORD, PDWORD, int)>
	    (_load_func(TEXT("LockPages")));
}

BOOL(*Architecture::_load_UnlockPages(void))(LPVOID, DWORD)
{

	return reinterpret_cast <BOOL(*)(LPVOID, DWORD)>
	    (_load_func(TEXT("UnlockPages")));
}
