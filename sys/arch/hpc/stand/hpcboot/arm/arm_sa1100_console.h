/* -*-C++-*-	$NetBSD: arm_sa1100_console.h,v 1.1 2008/03/08 02:26:03 rafal Exp $	*/

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Rafal K. Boni.
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

#ifndef _HPCBOOT_ARM_SA1100_CONSOLE_H_
#define	_HPCBOOT_ARM_SA1100_CONSOLE_H_

#include <hpcboot.h>
#include <memory.h>
#include <arm/arm_console.h>

class SA1100Console : public ARMConsole {
	friend class ARMConsole;

private:
	MemoryManager *&_mem;
	vaddr_t _uart_base;

private:
	SA1100Console(MemoryManager *& mem) : _mem(mem), _uart_base(~0) { }
	virtual ~SA1100Console() {
		if (_uart_base != ~0)
			_mem->unmapPhysicalPage(_uart_base);
	}

	void __tx_busy(void) {
		uint8_t reg;
		do
			reg = VOLATILE_REF8(_uart_base + 0x20);
		while ((reg & 0x1) == 0x1 ||(reg & 0x4) == 0);
	}

	virtual void __putc(const char s) {
		__tx_busy(); // wait until previous transmit done.
		VOLATILE_REF8(_uart_base + 0x14) =
		    static_cast <uint8_t>(0xff & s);
	}

public:
	virtual BOOL init(void) {
		if (!SerialConsole::init())
			return FALSE;

		_uart_base =
		    _mem->mapPhysicalPage(0x80050000, 0x100, PAGE_READWRITE);

		if (_uart_base == ~0)
			return FALSE;

		return TRUE;
	}
};
#endif //_HPCBOOT_ARM_SA1100_CONSOLE_H_
