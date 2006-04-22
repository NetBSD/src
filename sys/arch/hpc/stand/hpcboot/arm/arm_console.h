/* -*-C++-*-	$NetBSD: arm_console.h,v 1.5.6.1 2006/04/22 11:37:28 simonb Exp $	*/

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

#ifndef _HPCBOOT_ARM_CONSOLE_H_
#define	_HPCBOOT_ARM_CONSOLE_H_

#include <hpcboot.h>
#include <console.h>
#include <memory.h>

class ARMConsole : public SerialConsole {
private:
	typedef SerialConsole super;

	static ARMConsole *_instance;

	MemoryManager *&_mem;
	vaddr_t _uart_base;
	vaddr_t _uart_busy, _uart_transmit;

private:
	ARMConsole(MemoryManager *& mem) : _mem(mem) { /* NO-OP */ }

	virtual ~ARMConsole() {
		_mem->unmapPhysicalPage(_uart_base);
	}

	void __tx_busy(void) {
		uint8_t reg;
		do
			reg = VOLATILE_REF8(_uart_busy);
		while ((reg & 0x1) == 0x1 ||(reg & 0x4) == 0);
	}

	void __putc(const char s) {
		__tx_busy(); // wait until previous transmit done.
		VOLATILE_REF8(_uart_transmit) =
		    static_cast <uint8_t>(0xff & s);
	}

public:
	static ARMConsole *Instance(MemoryManager *&);
	static void Destroy(void);
	virtual BOOL init(void);
	virtual void print(const TCHAR *fmt, ...);
};
#endif //_HPCBOOT_ARM_CONSOLE_H_
