/* -*-C++-*-	$NetBSD: mips_vr41.cpp,v 1.2.130.1 2008/05/16 02:22:26 yamt Exp $	*/

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

#include <console.h>
#include <memory.h>
#include <mips/mips_vr41.h>

VR41XX::VR41XX(Console *&cons, MemoryManager *&mem)
	: MIPSArchitecture(cons, mem)
{
	_boot_func = VR41XX::boot_func;
}

VR41XX::~VR41XX()
{
	/* NO-OP */
}

BOOT_FUNC_(VR41XX)

	BOOL
VR41XX::init()
{
	MIPSArchitecture::init();

	// set D-RAM information
	_mem->loadBank(0x80000000,	// VR-specific VirtualCopy trick.
	    0x04000000);	// 64MByte

	return TRUE;
}

void
VR41XX::systemInfo()
{
	MIPSArchitecture::systemInfo();
	DPRINTF((TEXT("VR41\n")));
}

void
VR41XX::cacheFlush()
{
	MIPS_VR41XX_CACHE_FLUSH();
}
