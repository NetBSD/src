/* -*-C++-*-	$NetBSD: sh_arch.h,v 1.5 2002/02/04 17:38:27 uch Exp $	*/

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

#ifndef _HPCBOOT_SH_ARCH_H_
#define _HPCBOOT_SH_ARCH_H_

#include <hpcboot.h>
#include <console.h>
#include <memory.h>
#include <arch.h>
#include <sh3/sh3.h>
#include <sh3/hd64461.h>
#include <sh3/hd64465.h>

template <class T>
inline T
REG_READ(paddr_t r)
{
	return r & SH_P1_START ? *(T *)r :  *(T *)(r | SH_P2_START);
}

class SHArchitecture : public Architecture {
protected:
	typedef void(*boot_func_t)(struct BootArgs *, struct PageTag *);
  
	u_int8_t reg_read_1(paddr_t reg) {
		return REG_READ <u_int8_t>(reg);
	}
	u_int16_t reg_read_2(paddr_t reg) {
		return REG_READ <u_int16_t>(reg);
	}
	u_int32_t reg_read_4(paddr_t reg) {
		return REG_READ <u_int32_t>(reg);
	}
	void hd64461_dump(platid_t &);
	void bsc_dump(void);
	void pfc_dump(void);
	void tmu_dump(void);
	void tmu_channel_dump(int, paddr_t, paddr_t, paddr_t);
	void icu_dump(void);
	void icu_priority(void);
	void icu_control(void);
	void scif_dump(int);

public:
	struct intr_priority {
		const char *name;
		paddr_t reg;
		int shift;
	};
	static struct intr_priority ipr_table[];

private:
	int _kmode;
	boot_func_t _boot_func;
	void print_stack_pointer(void);

protected:

	// should be created as actual product insntnce.
	SHArchitecture(Console *&cons, MemoryManager *&mem, boot_func_t bootfunc)
		: _boot_func(bootfunc), Architecture(cons, mem) {
		// NO-OP
	}
	virtual ~SHArchitecture(void) { /* NO-OP */ }

public:
	BOOL init(void);
	BOOL setupLoader(void);
	paddr_t setupBootInfo(Loader &);

	void systemInfo(void);
	void jump(kaddr_t info, kaddr_t pvce);

	virtual void cache_flush(void) { /* NO-OP */ }
};

// 
// SH product. setup cache flush routine and 2nd-bootloader.
//

//
// SH3 series.
///
#define SH_(x)								\
class SH##x : public SHArchitecture {					\
public:									\
	SH##x(Console *&cons, MemoryManager *&mem, boot_func_t bootfunc)\
		: SHArchitecture(cons, mem, bootfunc) {			\
		DPRINTF((TEXT("CPU: SH") TEXT(#x) TEXT("\n")));		\
	}								\
	~SH##x(void) { /* NO-OP */ }					\
									\
	void cache_flush(void) {					\
		SH##x##_CACHE_FLUSH();					\
	}								\
									\
	static void boot_func(struct BootArgs *, struct PageTag *);	\
}

// 
// 2nd-bootloader.  make sure that PIC and its size is lower than page size.
// and can't call subroutine.
//
#define SH_BOOT_FUNC_(x)						\
void									\
SH##x##::boot_func(struct BootArgs *bi, struct PageTag *p)		\
{									\
/* Disable interrupt. block exception.(TLB exception don't occur) */	\
	int tmp;							\
	__asm("stc	sr, r5\n"					\
	      "or	r4, r5\n"					\
	      "ldc	r5, sr\n", 0x500000f0, tmp);			\
	/* Now I run on P1, TLB flush. and disable. */			\
									\
	VOLATILE_REF(MMUCR) = MMUCR_TF;					\
	do {								\
		u_int32_t *dst =(u_int32_t *)p->dst;			\
		u_int32_t *src =(u_int32_t *)p->src;			\
		u_int32_t sz = p->sz / sizeof (int);			\
		if (p->src == ~0)					\
			while (sz--)					\
				*dst++ = 0;				\
		else	    						\
			while (sz--)					\
				*dst++ = *src++;			\
	} while ((p =(struct PageTag *)p->next) != ~0);			\
									\
	SH##x##_CACHE_FLUSH();						\
									\
	/* jump to kernel entry. */					\
	__asm("jmp	@r7\n"						\
	      "nop\n", bi->argc, bi->argv,				\
		 bi->bootinfo, bi->kernel_entry);			\
}

SH_(7709);
SH_(7709A);

//
// SH4 series.
///
class SH7750 : public SHArchitecture {
public:
	SH7750(Console *&cons, MemoryManager *&mem, boot_func_t bootfunc)
		: SHArchitecture(cons, mem, bootfunc) {
		DPRINTF((TEXT("CPU: SH7750\n")));
	}
	~SH7750(void) { /* NO-OP */ }

	void cache_flush(void) {
		//
		// To invalidate I-cache, program must run on P2. I can't 
		// do it myself, use WinCE API. (WCE2.10 or later)
		//
		CacheSync(CACHE_D_WBINV);
		CacheSync(CACHE_I_INV);
	}

	static void boot_func(struct BootArgs *, struct PageTag *);
};

#endif // _HPCBOOT_SH_ARCH_H_
