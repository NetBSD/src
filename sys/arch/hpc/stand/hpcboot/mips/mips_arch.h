/* -*-C++-*-	$NetBSD: mips_arch.h,v 1.2 2001/04/24 19:28:00 uch Exp $	*/

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

#ifndef _HPCBOOT_MIPS_ARCH_H_
#define _HPCBOOT_MIPS_ARCH_H_

#include <hpcboot.h>
#include <arch.h>

class Console;

class MIPSArchitecture : public Architecture {
protected:
	typedef void(*boot_func_t)(struct BootArgs *, struct PageTag *);

	int _kmode;
	boot_func_t _boot_func;

public:
	MIPSArchitecture(Console *&, MemoryManager *&);
	virtual ~MIPSArchitecture(void);

	virtual BOOL init(void);
	BOOL setupLoader(void);
	virtual void systemInfo(void);
	virtual void cacheFlush(void) = 0;
	void jump(paddr_t info, paddr_t pvce);
};

#define DI()								\
  __asm(".set noreorder;"						\
	 "nop;"								\
	 "mtc0	zero, $12;"						\
	 "nop;nop;nop;" /* CP0 hazard for R4000 */			\
	 ".set reorder")

#define GET_SR(x)							\
  __asm(".set noreorder;"						\
	 "mfc0	t0, $12;"						\
	 "sw	t0,(%0);"						\
	 ".set reorder", &(x));

#define SET_SR(x)							\
  __asm(".set noreorder;"						\
	 "lw	t0,(%0);"						\
	 "nop;"								\
	 "mtc0	t0, $12;"						\
	 "nop;nop;nop;" /* CP0 hazard for R4000 */			\
	 ".set reorder", &(x));

/* 
 * 2nd-bootloader.  make sure that PIC and its size is lower than page size.
 * and can't call subroutine.
 * naked funciton can't use stack. if you want to use, remove its declare.
 * interrupts are disabled. but if access kuseg,(should not occur)
 * it causes TLB exception and then Windows CE enable interrupts again.
 */
#define BOOT_FUNC_(x)							\
__declspec(naked) void							\
x##::boot_func(struct BootArgs *bi, struct PageTag *p)			\
{									\
  /* disable interrupt */						\
  DI();									\
  /* set kernel image */						\
  __asm(".set noreorder;"						\
	 "move	t6, a1;"	/* p */					\
	 "li	t1, 0xffffffff;"					\
"page_start:"								\
	 "beq	t6, t1, page_end;"					\
	 "move	t7, t6;"						\
	 "lw	t6, 0(t7);"	/* p = next */				\
	 "lw	t0, 4(t7);"	/* src */				\
	 "lw	t4, 8(t7);"	/* dst */				\
	 "lw	t2, 12(t7);"	/* sz */				\
	 "beq	t0, t1, page_clear;"					\
	 "addu	t5, t4, t2;"	/* dst + sz */				\
"page_copy:"								\
	 "lw	t3, 0(t0);"	/* bcopy */				\
	 "sw	t3, 0(t4);"						\
	 "addiu	t4, t4, 4;"						\
	 "bltu	t4, t5, page_copy;"					\
	 "addiu	t0, t0, 4;"						\
	 "b	page_start;"						\
	 "nop;"								\
"page_clear:"								\
	 "sw	zero, 0(t4);"	/* bzero */				\
	 "addiu	t4, t4, 4;"						\
	 "bltu	t4, t5, page_clear;"					\
	 "nop;"								\
	 "b	page_start;"						\
	 "nop;"								\
"page_end:"								\
	 "nop;"								\
	 ".set reorder");						\
									\
  /* Cache flush for kernel */						\
  MIPS_##x##_CACHE_FLUSH();						\
									\
  /* jump to kernel entry */						\
  __asm(".set noreorder;"						\
	 "move	t0, a0;"						\
	 "lw	t1, 0(t0);"						\
	 "lw	a0, 4(t0);"						\
	 "lw	a1, 8(t0);"						\
	 "lw	a2, 12(t0);"						\
	 "jr	t1;"							\
	 "nop;"								\
	 ".set reorder");						\
}

#endif // _HPCBOOT_MIPS_ARCH_H_
