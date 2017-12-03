/* -*-C++-*-	$NetBSD: arm_console.cpp,v 1.7.24.1 2017/12/03 11:36:14 jdolecek Exp $	*/

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

#include <arm/arm_console.h>
#include <arm/arm_pxa2x0_console.h>
#include <arm/arm_sa1100_console.h>

ARMConsole *ARMConsole::_instance = NULL;

ARMConsole *
ARMConsole::Instance(MemoryManager *&mem, ArchitectureOps arch)
{

	if (!_instance) {
		switch (arch) {
		    default:
			_instance = NULL;
			break;

		    case ARCHITECTURE_ARM_SA1100:
			_instance = new SA1100Console(mem);
			break;

		    case ARCHITECTURE_ARM_PXA250:
		    case ARCHITECTURE_ARM_PXA270:
			_instance = new PXA2x0Console(mem);
			break;
		}
	}

	return _instance;
}

void
ARMConsole::Destroy(void)
{

	if (_instance) {
		delete _instance;
		_instance = NULL;
	}
}

void
ARMConsole::print(const TCHAR *fmt, ...)
{
	SETUP_WIDECHAR_BUFFER();

	if (!setupMultibyteBuffer())
		return;

	for (int i = 0; i < CONSOLE_BUFSIZE && _bufm[i] != '\0'; i++) {
		char s = _bufm[i];
		if (s == '\n')
			__putc('\r');
		__putc(s);
	}
}
