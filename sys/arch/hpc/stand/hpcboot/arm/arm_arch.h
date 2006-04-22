/* -*-C++-*-	$NetBSD: arm_arch.h,v 1.3.6.1 2006/04/22 11:37:28 simonb Exp $	*/

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

#ifndef _HPCBOOT_ARM_ARCH_H_
#define	_HPCBOOT_ARM_ARCH_H_

#include <hpcboot.h>
#include <arch.h>

class Console;

class ARMArchitecture : public Architecture {
private:
	int _kmode;
	// test routine for SA-1100 peripherals.
	void testFramebuffer(void);
	void testUART(void);

public:
	ARMArchitecture(Console *&, MemoryManager *&);
	virtual ~ARMArchitecture(void);

	BOOL init(void);
	BOOL setupLoader(void);
	void systemInfo(void);
	void jump(paddr_t info, paddr_t pvce);
};

__BEGIN_DECLS
// Coprocessor 15
uint32_t GetCop15Reg0(void);
uint32_t GetCop15Reg1(void);	void SetCop15Reg1(uint32_t);
uint32_t GetCop15Reg2(void);	void SetCop15Reg2(uint32_t);
uint32_t GetCop15Reg3(void);	void SetCop15Reg3(uint32_t);
uint32_t GetCop15Reg5(void);
uint32_t GetCop15Reg6(void);
uint32_t GetCop15Reg13(void);	void SetCop15Reg13(uint32_t);
uint32_t GetCop15Reg14(void);

// Interrupt
void EI(void);
void DI(void);

// Write-Back I/D-separate Cache
void InvalidateICache(void);
void WritebackDCache(void);
void InvalidateDCache(void);
void WritebackInvalidateDCache(void);

// MMU TLB access
void FlushIDTLB(void);
void FlushITLB(void);
void FlushDTLB(void);
void FlushDTLBS(vaddr_t);

uint32_t GetCPSR(void);
void SetCPSR(uint32_t);
void SetSVCMode(void);
void SetSystemMode(void);

// 2nd bootloader
void boot_func(kaddr_t, kaddr_t, kaddr_t, kaddr_t);
extern char boot_func_end[];
#define	BOOT_FUNC_START		reinterpret_cast <vaddr_t>(boot_func)
#define	BOOT_FUNC_END		reinterpret_cast <vaddr_t>(boot_func_end)

/* jump to 2nd loader */
void FlatJump(kaddr_t, kaddr_t, kaddr_t, kaddr_t);
__END_DECLS

#endif // _HPCBOOT_ARM_ARCH_H_
