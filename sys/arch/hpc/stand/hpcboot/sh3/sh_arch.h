/* -*-C++-*-	$NetBSD: sh_arch.h,v 1.6 2002/02/11 17:08:56 uch Exp $	*/

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

#include <arch.h>
#include <memory.h>	// loadBank
#include <console.h>	// DPRINTF

#include <sh3/dev/sh_dev.h>

// CPU specific macro
#include <sh3/cpu/sh3.h>
#include <sh3/cpu/sh4.h>

class SHArchitecture : public Architecture {
protected:
	typedef void(*boot_func_t)(struct BootArgs *, struct PageTag *);
	SHdev *_dev;

private:
	typedef Architecture super;
	boot_func_t _boot_func;

protected:
	// should be created as actual product insntnce. not public.
	SHArchitecture(Console *&cons, MemoryManager *&mem, boot_func_t bootfunc)
		: _boot_func(bootfunc), Architecture(cons, mem) {
		// NO-OP
	}
	virtual ~SHArchitecture(void) { /* NO-OP */ }
	virtual void cache_flush(void) = 0;

public:
	virtual BOOL init(void);
	virtual BOOL setupLoader(void);
	virtual void systemInfo(void);
	virtual void jump(kaddr_t info, kaddr_t pvce);

	// returns host machines CPU type. 3 for SH3. 4 for SH4
	static int cpu_type(void);
};

// 
// SH product. setup cache flush routine and 2nd-bootloader.
//

//
// SH3 series.
///
#define SH_(x)								\
class SH ## x : public SHArchitecture {					\
private:								\
	typedef SHArchitecture super;					\
public:									\
	SH ## x(Console *&cons, MemoryManager *&mem, boot_func_t bootfunc)\
		: SHArchitecture(cons, mem, bootfunc) {			\
		DPRINTF((TEXT("CPU: SH") TEXT(#x) TEXT("\n")));		\
		_dev = new SH3dev;					\
	}								\
	~SH ## x(void) {						\
		delete _dev;						\
	}								\
									\
	virtual BOOL init(void) {					\
		int sz;							\
									\
		if (!super::init())					\
			return FALSE;					\
		/* SH7709, SH7709A split AREA3 to two area. */		\
		sz = SH_AREA_SIZE / 2;					\
		_mem->loadBank(SH_AREA3_START, sz);			\
		_mem->loadBank(SH_AREA3_START + sz , sz);		\
		return TRUE;						\
	}								\
									\
	virtual void cache_flush(void) {				\
		SH ## x ## _CACHE_FLUSH();				\
	}								\
									\
	static void boot_func(struct BootArgs *, struct PageTag *);	\
}

SH_(7709);
SH_(7709A);

//
// SH4 series.
///
class SH7750 : public SHArchitecture {
private:
	typedef SHArchitecture super;

public:
	SH7750(Console *&cons, MemoryManager *&mem, boot_func_t bootfunc)
		: SHArchitecture(cons, mem, bootfunc) {
		DPRINTF((TEXT("CPU: SH7750\n")));
		_dev = new SH4dev;
	}
	~SH7750(void) {
		delete _dev;
	}

	virtual BOOL init(void) {

		if (!super::init())
			return FALSE;
		_mem->loadBank(SH_AREA3_START, SH_AREA_SIZE);

		return TRUE;
	}

	virtual void cache_flush(void) {
		//
		// To invalidate I-cache, program must run on P2. I can't 
		// do it myself, use WinCE API. (WCE2.10 or later)
		//
		CacheSync(CACHE_D_WBINV);
		CacheSync(CACHE_I_INV);
	}

	virtual BOOL setupLoader(void) {
		//
		// 2nd boot loader access cache address array. run on P2.
		// 
		if (super::setupLoader()) {
			(u_int32_t)_loader_addr |= 0x20000000;
			DPRINTF
			    ((TEXT("loader address moved to P2-area 0x%08x\n"),
				(unsigned)_loader_addr));
			return TRUE;
		}

		return FALSE;
	}

	static void boot_func(struct BootArgs *, struct PageTag *);
};

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
	/* Now I run on P1(P2 for SH4), TLB flush. and disable. */	\
									\
	SH ## x ## _MMU_DISABLE();					\
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
	SH ## x ## _CACHE_FLUSH();					\
									\
	/* jump to kernel entry. */					\
	__asm("jmp	@r7\n"						\
	      "nop\n", bi->argc, bi->argv,				\
		 bi->bootinfo, bi->kernel_entry);			\
}

//   suspend/resume external Interrupt. 
//  (don't block) use under privilege mode.
//
__BEGIN_DECLS
u_int32_t suspendIntr(void);
void resumeIntr(u_int32_t);
__END_DECLS

#endif // _HPCBOOT_SH_ARCH_H_
