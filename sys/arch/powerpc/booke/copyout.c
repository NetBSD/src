/*	$NetBSD: copyout.c,v 1.3.26.1 2014/08/10 06:54:05 tls Exp $	*/

/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: copyout.c,v 1.3.26.1 2014/08/10 06:54:05 tls Exp $");

#include <sys/param.h>
#include <sys/lwp.h>

#include <powerpc/pcb.h>

#include <powerpc/booke/cpuvar.h>

static inline void
copyout_uint8(uint8_t *udaddr, uint8_t data, register_t ds_msr)
{
	register_t msr;
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"stb	%[data],0(%[udaddr])"		/* store user byte */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr)
	    : [ds_msr] "r" (ds_msr), [data] "r" (data), [udaddr] "b" (udaddr));
}

#if 0
static inline void
copyout_uint16(uint8_t *udaddr, uint8_t data, register_t ds_msr)
{
	register_t msr;
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"stb	%[data],0(%[udaddr])"		/* store user byte */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr)
	    : [ds_msr] "r" (ds_msr), [data] "r" (data), [udaddr] "b" (udaddr));
}
#endif

static inline void
copyout_uint32(uint32_t * const udaddr, uint32_t data, register_t ds_msr)
{
	register_t msr;
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"stw	%[data],0(%[udaddr])"		/* store user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr)
	    : [ds_msr] "r" (ds_msr), [data] "r" (data), [udaddr] "b" (udaddr));
}

#if 0
static inline void
copyout_le32(uint32_t * const udaddr, uint32_t data, register_t ds_msr)
{
	register_t msr;
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"stwbrx	%[data],0,%[udaddr]"		/* store user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr)
	    : [ds_msr] "r" (ds_msr), [data] "r" (data), [udaddr] "b" (udaddr));
}

static inline void
copyout_le32_with_mask(uint32_t * const udaddr, uint32_t data,
	uint32_t mask, register_t ds_msr)
{
	register_t msr;
	uint32_t tmp;
	KASSERT((data & ~mask) == 0);
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"lwbrx	%[tmp],0,%[udaddr]"		/* fetch user data */
	"\n\t"	"andc	%[tmp],%[tmp],%[mask]"		/* mask out new data */
	"\n\t"	"or	%[tmp],%[tmp],%[data]"		/* merge new data */
	"\n\t"	"stwbrx	%[tmp],0,%[udaddr]"		/* store user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr), [tmp] "=&r" (tmp)
	    : [ds_msr] "r" (ds_msr), [data] "r" (data),
	      [mask] "r" (mask), [udaddr] "b" (udaddr));
}
#endif

static inline void
copyout_16uint8s(const uint8_t *ksaddr8, uint8_t *udaddr8, register_t ds_msr)
{
	register_t msr;
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"stb	%[data0],0(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data1],1(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data2],2(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data3],3(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data4],4(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data5],5(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data6],6(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data7],7(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data8],8(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data9],9(%[udaddr8])"		/* store user data */
	"\n\t"	"stb	%[data10],10(%[udaddr8])"	/* store user data */
	"\n\t"	"stb	%[data11],11(%[udaddr8])"	/* store user data */
	"\n\t"	"stb	%[data12],12(%[udaddr8])"	/* store user data */
	"\n\t"	"stb	%[data13],13(%[udaddr8])"	/* store user data */
	"\n\t"	"stb	%[data14],14(%[udaddr8])"	/* store user data */
	"\n\t"	"stb	%[data15],15(%[udaddr8])"	/* store user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr)
	    : [ds_msr] "r" (ds_msr), [udaddr8] "b" (udaddr8),
	      [data0] "r" (ksaddr8[0]), [data1] "r" (ksaddr8[1]),
	      [data2] "r" (ksaddr8[2]), [data3] "r" (ksaddr8[3]),
	      [data4] "r" (ksaddr8[4]), [data5] "r" (ksaddr8[5]),
	      [data6] "r" (ksaddr8[6]), [data7] "r" (ksaddr8[7]),
	      [data8] "r" (ksaddr8[8]), [data9] "r" (ksaddr8[9]),
	      [data10] "r" (ksaddr8[10]), [data11] "r" (ksaddr8[11]),
	      [data12] "r" (ksaddr8[12]), [data13] "r" (ksaddr8[13]),
	      [data14] "r" (ksaddr8[14]), [data15] "r" (ksaddr8[15]));
}

static inline void
copyout_8uint32s(const uint32_t * const ksaddr32, uint32_t * const udaddr32,
	const register_t ds_msr, const size_t line_mask)
{
	register_t msr;
	register_t tmp;
	__asm volatile(
		"and.	%[tmp],%[line_mask],%[udaddr32]"
	"\n\t"	"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"bne	0,1f"
	"\n\t"	"dcba	0,%[udaddr32]"
	"\n"	"1:"
	"\n\t"	"stw	%[data0],0(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data1],4(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data2],8(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data3],12(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data4],16(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data5],20(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data6],24(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data7],28(%[udaddr32])"	/* store user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr), [tmp] "=&r" (tmp)
	    : [ds_msr] "r" (ds_msr), [udaddr32] "b" (udaddr32),
	      [line_mask] "r" (line_mask),
	      [data0] "r" (ksaddr32[0]), [data1] "r" (ksaddr32[1]),
	      [data2] "r" (ksaddr32[2]), [data3] "r" (ksaddr32[3]),
	      [data4] "r" (ksaddr32[4]), [data5] "r" (ksaddr32[5]),
	      [data6] "r" (ksaddr32[6]), [data7] "r" (ksaddr32[7])
	    : "cr0");
}

static inline void
copyout_16uint32s(const uint32_t * const ksaddr32, uint32_t * const udaddr32,
	const register_t ds_msr, const size_t line_mask)
{
	KASSERT(((uintptr_t)udaddr32 & line_mask) == 0);
	register_t msr;
	register_t tmp;
	__asm volatile(
		"and.	%[tmp],%[line_mask],%[udaddr32]"
	"\n\t"	"cmplwi	2,%[line_size],32"
	"\n\t"	"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"bne	0,1f"
	"\n\t"	"dcba	0,%[udaddr32]"
	"\n\t"	"bne	2,1f"
	"\n\t"	"dcba	%[line_size],%[udaddr32]"
	"\n"	"1:"
	"\n\t"	"stw	%[data0],0(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data1],4(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data2],8(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data3],12(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data4],16(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data5],20(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data6],24(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data7],28(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data8],32(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data9],36(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data10],40(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data11],44(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data12],48(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data13],52(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data14],56(%[udaddr32])"	/* store user data */
	"\n\t"	"stw	%[data15],60(%[udaddr32])"	/* store user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr), [tmp] "=&r" (tmp)
	    : [ds_msr] "r" (ds_msr), [udaddr32] "b" (udaddr32),
	      [line_size] "r" (line_mask + 1), [line_mask] "r" (line_mask),
	      [data0] "r" (ksaddr32[0]), [data1] "r" (ksaddr32[1]),
	      [data2] "r" (ksaddr32[2]), [data3] "r" (ksaddr32[3]),
	      [data4] "r" (ksaddr32[4]), [data5] "r" (ksaddr32[5]),
	      [data6] "r" (ksaddr32[6]), [data7] "r" (ksaddr32[7]),
	      [data8] "r" (ksaddr32[8]), [data9] "r" (ksaddr32[9]),
	      [data10] "r" (ksaddr32[10]), [data11] "r" (ksaddr32[11]),
	      [data12] "r" (ksaddr32[12]), [data13] "r" (ksaddr32[13]),
	      [data14] "r" (ksaddr32[14]), [data15] "r" (ksaddr32[15])
	    : "cr0", "cr2");
}

static inline void
copyout_uint8s(vaddr_t ksaddr, vaddr_t udaddr, size_t len, register_t ds_msr)
{
	const uint8_t *ksaddr8 = (void *)ksaddr;
	uint8_t *udaddr8 = (void *)udaddr;

	__builtin_prefetch(ksaddr8, 0, 1);

	for (; len >= 16; len -= 16, ksaddr8 += 16, udaddr8 += 16) {
		__builtin_prefetch(ksaddr8 + 16, 0, 1);
		copyout_16uint8s(ksaddr8, udaddr8, ds_msr);
	}

	while (len-- > 0) {
		copyout_uint8(udaddr8++, *ksaddr8++, ds_msr);
	}
}

static inline void
copyout_uint32s(vaddr_t ksaddr, vaddr_t udaddr, size_t len, register_t ds_msr)
{
	const size_t line_size = curcpu()->ci_ci.dcache_line_size;
	const size_t line_mask = line_size - 1;
	const size_t udalignment = udaddr & line_mask;
	KASSERT((ksaddr & 3) == 0);
	KASSERT((udaddr & 3) == 0);
	const uint32_t *ksaddr32 = (void *)ksaddr;
	uint32_t *udaddr32 = (void *)udaddr;
	len >>= 2;
	__builtin_prefetch(ksaddr32, 0, 1);
	if (udalignment != 0 && udalignment + 4*len > line_size) {
		size_t slen = (line_size - udalignment) >> 2;
		len -= slen;
		for (; slen >= 8; ksaddr32 += 8, udaddr32 += 8, slen -= 8) {
			copyout_8uint32s(ksaddr32, udaddr32, ds_msr, line_mask);
		}
		while (slen-- > 0) {
			copyout_uint32(udaddr32++, *ksaddr32++, ds_msr);
		}
		if (len == 0)
			return;
	}
	__builtin_prefetch(ksaddr32, 0, 1);
	while (len >= 16) {
		__builtin_prefetch(ksaddr32 + 8, 0, 1);
		__builtin_prefetch(ksaddr32 + 16, 0, 1);
		copyout_16uint32s(ksaddr32, udaddr32, ds_msr, line_mask);
		ksaddr32 += 16, udaddr32 += 16, len -= 16;
	}
	KASSERT(len <= 16);
	if (len >= 8) {
		__builtin_prefetch(ksaddr32 + 8, 0, 1);
		copyout_8uint32s(ksaddr32, udaddr32, ds_msr, line_mask);
		ksaddr32 += 8, udaddr32 += 8, len -= 8;
	}
	while (len-- > 0) {
		copyout_uint32(udaddr32++, *ksaddr32++, ds_msr);
	}
}

int
copyout(const void *vksaddr, void *vudaddr, size_t len)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	struct faultbuf env;
	vaddr_t udaddr = (vaddr_t) vudaddr;
	vaddr_t ksaddr = (vaddr_t) vksaddr;

	if (__predict_false(len == 0)) {
		return 0;
	}

	const register_t ds_msr = mfmsr() | PSL_DS;

	int rv = setfault(&env);
	if (rv != 0) {
		pcb->pcb_onfault = NULL;
		return rv;
	}

	if (__predict_false(len < 4)) {
		copyout_uint8s(ksaddr, udaddr, len, ds_msr);
		pcb->pcb_onfault = NULL;
		return 0;
	}

	const size_t alignment = (udaddr ^ ksaddr) & 3;
	if (__predict_true(alignment == 0)) {
		size_t slen;
		if (__predict_false(ksaddr & 3)) {
			slen = 4 - (ksaddr & 3);
			copyout_uint8s(ksaddr, udaddr, slen, ds_msr);
			udaddr += slen, ksaddr += slen, len -= slen;
		}
		slen = len & ~3;
		if (__predict_true(slen >= 4)) {
			copyout_uint32s(ksaddr, udaddr, slen, ds_msr);
			udaddr += slen, ksaddr += slen, len -= slen;
		}
	}

	if (len > 0) {
		copyout_uint8s(ksaddr, udaddr, len, ds_msr);
	}
	pcb->pcb_onfault = NULL;
	return 0;
}

int
copyoutstr(const void *ksaddr, void *udaddr, size_t len, size_t *lenp)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	struct faultbuf env;

	if (__predict_false(len == 0)) {
		if (lenp)
			*lenp = 0;
		return 0;
	}

	if (setfault(&env)) {
		pcb->pcb_onfault = NULL;
		if (lenp)
			*lenp = 0;
		return EFAULT;
	}

	const register_t ds_msr = mfmsr() | PSL_DS;
	const uint8_t *ksaddr8 = ksaddr;
	size_t copylen = 0;

#if 1
	uint8_t *udaddr8 = (void *)udaddr;

	while (copylen++ < len) {
		const uint8_t data = *ksaddr8++;
		copyout_uint8(udaddr8++, data, ds_msr);
		if (data == 0)
			break;
	}
#else
	uint32_t *udaddr32 = (void *)((uintptr_t)udaddr & ~3);

	size_t boff = (uintptr_t)udaddr & 3;
	bool done = false;
	size_t wlen = 0;
	size_t data = 0;

	/*
	 * If the destination buffer doesn't start on a 32-bit boundary
	 * try to partially fill in the first word.  If we succeed we can
	 * finish writing it while preserving the bytes on front.
	 */
	if (boff > 0) {
		KASSERT(len > 0);
		do {
			data = (data << 8) | *ksaddr8++;
			wlen++;
			done = ((uint8_t)data == 0 || len == wlen);
		} while (!done && boff + wlen < 4);
		KASSERT(wlen > 0);
		data <<= 8 * boff;
		if (!done || boff + wlen == 4) {
			uint32_t mask = 0xffffffff << (8 * boff);
			copyout_le32_with_mask(udaddr32++, data, mask, ds_msr);
			boff = 0;
			copylen = wlen;
			wlen = 0;
			data = 0;
		}
	}

	/*
	 * Now we get to the heart of the routine.  Build up complete words
	 * if possible.  When we have one, write it to the user's address
	 * space and go for the next.  If we ran out of space or we found the
	 * end of the string, stop building.  If we managed to build a complete
	 * word, just write it and be happy.  Otherwise we have to deal with
	 * the trailing bytes.
	 */
	KASSERT(done || boff == 0);
	KASSERT(done || copylen < len);
	while (!done) {
		KASSERT(wlen == 0);
		KASSERT(copylen < len);
		do {
			data = (data << 8) | *ksaddr8++;
			wlen++;
			done = ((uint8_t)data == 0 || copylen + wlen == len);
		} while (!done && wlen < 4);
		KASSERT(done || wlen == 4);
		if (__predict_true(wlen == 4)) {
			copyout_le32(udaddr32++, data, ds_msr);
			data = 0;
			copylen += wlen;
			wlen = 0;
			KASSERT(copylen < len || done);
		}
	}
	KASSERT(wlen < 3);
	if (wlen) {
		/*
		 * Remember even though we are running big-endian we are using
		 * byte reversed load/stores so we need to deal with things as
		 * little endian.
		 *
		 * wlen=1 boff=0:
		 * (~(~0 <<  8) <<  0) -> (~(0xffffff00) <<  0) -> 0x000000ff
		 * wlen=1 boff=1:
		 * (~(~0 <<  8) <<  8) -> (~(0xffffff00) <<  8) -> 0x0000ff00
		 * wlen=1 boff=2:
		 * (~(~0 <<  8) << 16) -> (~(0xffffff00) << 16) -> 0x00ff0000
		 * wlen=1 boff=3:
		 * (~(~0 <<  8) << 24) -> (~(0xffffff00) << 24) -> 0xff000000
		 * wlen=2 boff=0:
		 * (~(~0 << 16) <<  0) -> (~(0xffff0000) <<  0) -> 0x0000ffff
		 * wlen=2 boff=1:
		 * (~(~0 << 16) <<  8) -> (~(0xffff0000) <<  8) -> 0x00ffff00
		 * wlen=2 boff=2:
		 * (~(~0 << 16) << 16) -> (~(0xffff0000) << 16) -> 0xffff0000
		 * wlen=3 boff=0:
		 * (~(~0 << 24) <<  0) -> (~(0xff000000) <<  0) -> 0x00ffffff
		 * wlen=3 boff=1:
		 * (~(~0 << 24) <<  8) -> (~(0xff000000) <<  8) -> 0xffffff00
		 */
		KASSERT(boff + wlen <= 4);
		uint32_t mask = (~(~0 << (8 * wlen))) << (8 * boff);
		KASSERT(mask != 0xffffffff);
		copyout_le32_with_mask(udaddr32, data, mask, ds_msr);
		copylen += wlen;
	}
#endif

	pcb->pcb_onfault = NULL;
	if (lenp)
		*lenp = copylen;
	return 0;
}
