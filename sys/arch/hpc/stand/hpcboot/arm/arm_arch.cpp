/*	$NetBSD: arm_arch.cpp,v 1.8 2008/04/28 20:23:20 martin Exp $	*/

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

ARMArchitecture::ARMArchitecture(Console *&cons, MemoryManager *&mem)
	: Architecture(cons, mem)
{
	DPRINTF((TEXT("ARM architecture.\n")));
}

ARMArchitecture::~ARMArchitecture(void)
{
}

void
ARMArchitecture::systemInfo(void)
{
	Architecture::systemInfo();

	_kmode = SetKMode(1);
	DI();
	if ((GetCPSR() & 0x1f) != 0x1f)
		DPRINTF((TEXT("can't change to System mode\n")));

	DPRINTF((TEXT("Reg0 :%08x\n"), GetCop15Reg0()));
	DPRINTF((TEXT("Reg1 :%08x\n"), GetCop15Reg1()));
	DPRINTF((TEXT("Reg2 :%08x\n"), GetCop15Reg2()));
	DPRINTF((TEXT("Reg3 :%08x\n"), GetCop15Reg3()));
	DPRINTF((TEXT("Reg5 :%08x\n"), GetCop15Reg5()));
	DPRINTF((TEXT("Reg6 :%08x\n"), GetCop15Reg6()));
	DPRINTF((TEXT("Reg13:%08x\n"), GetCop15Reg13()));
	DPRINTF((TEXT("Reg14:%08x\n"), GetCop15Reg14()));
	DPRINTF((TEXT("CPSR :%08x\n"), GetCPSR()));
	EI();
	SetKMode(_kmode);
}
