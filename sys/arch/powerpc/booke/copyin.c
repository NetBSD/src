/*	$NetBSD: copyin.c,v 1.6 2014/07/24 23:27:25 joerg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: copyin.c,v 1.6 2014/07/24 23:27:25 joerg Exp $");

#include <sys/param.h>
#include <sys/lwp.h>

#include <powerpc/pcb.h>

#include <powerpc/booke/cpuvar.h>

static inline uint8_t
copyin_byte(const uint8_t * const usaddr8, register_t ds_msr)
{
	register_t msr;
	uint8_t data;
	__asm volatile(
		"mfmsr	%[msr]; "			/* Save MSR */
		"mtmsr	%[ds_msr]; sync; isync; "	/* DS on */
		"lbz	%[data],0(%[usaddr8]); "	/* fetch user byte */
		"mtmsr	%[msr]; sync; isync; "		/* DS off */
	    : [msr] "=&r" (msr), [data] "=r" (data)
	    : [ds_msr] "r" (ds_msr), [usaddr8] "b" (usaddr8));
	return data;
}

#if 0
static inline uint16_t
copyin_halfword(const uint16_t * const usaddr16, register_t ds_msr)
{
	register_t msr;
	uint16_t data;
	__asm volatile(
		"mfmsr	%[msr]; "			/* Save MSR */
		"mtmsr	%[ds_msr]; sync; isync; "	/* DS on */
		"lhz	%[data],0(%[usaddr16]); "	/* fetch user byte */
		"mtmsr	%[msr]; sync; isync; "		/* DS off */
	    : [msr] "=&r" (msr), [data] "=r" (data)
	    : [ds_msr] "r" (ds_msr), [usaddr16] "b" (usaddr16));
	return data;
}
#endif

static inline uint32_t
copyin_word(const uint32_t * const usaddr32, register_t ds_msr)
{
	register_t msr;
	uint32_t data;
	__asm volatile(
		"mfmsr	%[msr]; "			/* Save MSR */
		"mtmsr	%[ds_msr]; sync; isync; "	/* DS on */
		"lwz	%[data],0(%[usaddr32]); "	/* load user byte */
		"mtmsr	%[msr]; sync; isync; "		/* DS off */
	    : [msr] "=&r" (msr), [data] "=r" (data)
	    : [ds_msr] "r" (ds_msr), [usaddr32] "b" (usaddr32));
	return data;
}

static inline uint32_t
copyin_word_bswap(const uint32_t * const usaddr32, register_t ds_msr)
{
	register_t msr;
	uint32_t data;
	__asm volatile(
		"mfmsr	%[msr]; "			/* Save MSR */
		"mtmsr	%[ds_msr]; sync; isync; "	/* DS on */
		"lwbrx	%[data],0,%[usaddr32]; "	/* load user LE word */
		"mtmsr	%[msr]; sync; isync; "		/* DS off */
	    : [msr] "=&r" (msr), [data] "=r" (data)
	    : [ds_msr] "r" (ds_msr), [usaddr32] "b" (usaddr32));
	return data;
}

static inline void
copyin_8words(const uint32_t *usaddr32, uint32_t *kdaddr32, register_t ds_msr)
{
	register_t msr;
	//uint32_t data[8];
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"lwz	%[data0],0(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data1],4(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data2],8(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data3],12(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data4],16(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data5],20(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data6],24(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data7],28(%[usaddr32])"	/* fetch user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr),
	      [data0] "=&r" (kdaddr32[0]), [data1] "=&r" (kdaddr32[1]),
	      [data2] "=&r" (kdaddr32[2]), [data3] "=&r" (kdaddr32[3]),
	      [data4] "=&r" (kdaddr32[4]), [data5] "=&r" (kdaddr32[5]),
	      [data6] "=&r" (kdaddr32[6]), [data7] "=&r" (kdaddr32[7])
	    : [ds_msr] "r" (ds_msr), [usaddr32] "b" (usaddr32));
}

static inline void
copyin_16words(const uint32_t *usaddr32, uint32_t *kdaddr32, register_t ds_msr)
{
	register_t msr;
	__asm volatile(
		"mfmsr	%[msr]"				/* Save MSR */
	"\n\t"	"mtmsr	%[ds_msr]; sync; isync"		/* DS on */
	"\n\t"	"lwz	%[data0],0(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data1],4(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data2],8(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data3],12(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data4],16(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data5],20(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data6],24(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data7],28(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data8],32(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data9],36(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data10],40(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data11],44(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data12],48(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data13],52(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data14],56(%[usaddr32])"	/* fetch user data */
	"\n\t"	"lwz	%[data15],60(%[usaddr32])"	/* fetch user data */
	"\n\t"	"mtmsr	%[msr]; sync; isync"		/* DS off */
	    : [msr] "=&r" (msr),
	      [data0] "=&r" (kdaddr32[0]), [data1] "=&r" (kdaddr32[1]),
	      [data2] "=&r" (kdaddr32[2]), [data3] "=&r" (kdaddr32[3]),
	      [data4] "=&r" (kdaddr32[4]), [data5] "=&r" (kdaddr32[5]),
	      [data6] "=&r" (kdaddr32[6]), [data7] "=&r" (kdaddr32[7]),
	      [data8] "=&r" (kdaddr32[8]), [data9] "=&r" (kdaddr32[9]),
	      [data10] "=&r" (kdaddr32[10]), [data11] "=&r" (kdaddr32[11]),
	      [data12] "=&r" (kdaddr32[12]), [data13] "=&r" (kdaddr32[13]),
	      [data14] "=&r" (kdaddr32[14]), [data15] "=&r" (kdaddr32[15])
	    : [ds_msr] "r" (ds_msr), [usaddr32] "b" (usaddr32));
}
static inline void
copyin_bytes(vaddr_t usaddr, vaddr_t kdaddr, size_t len, register_t ds_msr)
{
	const uint8_t *usaddr8 = (void *)usaddr;
	uint8_t *kdaddr8 = (void *)kdaddr;
	while (len-- > 0) {
		*kdaddr8++ = copyin_byte(usaddr8++, ds_msr);
	}
}

static inline void
copyin_words(vaddr_t usaddr, vaddr_t kdaddr, size_t len, register_t ds_msr)
{
	KASSERT((kdaddr & 3) == 0);
	KASSERT((usaddr & 3) == 0);
	const uint32_t *usaddr32 = (void *)usaddr;
	uint32_t *kdaddr32 = (void *)kdaddr;
	len >>= 2;
	while (len >= 16) {
		copyin_16words(usaddr32, kdaddr32, ds_msr);
		usaddr32 += 16, kdaddr32 += 16, len -= 16;
	}
	KASSERT(len < 16);
	if (len >= 8) {
		copyin_8words(usaddr32, kdaddr32, ds_msr);
		usaddr32 += 8, kdaddr32 += 8, len -= 8;
	}
	while (len-- > 0) {
		*kdaddr32++ = copyin_word(usaddr32++, ds_msr);
	}
}

uint32_t
ufetch_32(const void *vusaddr)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	struct faultbuf env;

	if (setfault(&env) != 0) {
		pcb->pcb_onfault = NULL;
		return -1;
	}

	uint32_t rv = copyin_word(vusaddr, mfmsr() | PSL_DS);

	pcb->pcb_onfault = NULL;

	return rv;
}

int
copyin(const void *vusaddr, void *vkdaddr, size_t len)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	struct faultbuf env;
	vaddr_t usaddr = (vaddr_t) vusaddr;
	vaddr_t kdaddr = (vaddr_t) vkdaddr;

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
		copyin_bytes(usaddr, kdaddr, len, ds_msr);
		pcb->pcb_onfault = NULL;
		return 0;
	}

	const size_t alignment = (usaddr ^ kdaddr) & 3;
	if (__predict_true(alignment == 0)) {
		size_t slen;
		if (__predict_false(kdaddr & 3)) {
			slen = 4 - (kdaddr & 3);
			copyin_bytes(usaddr, kdaddr, slen, ds_msr);
			usaddr += slen, kdaddr += slen, len -= slen;
		}
		slen = len & ~3;
		if (__predict_true(slen >= 4)) {
			copyin_words(usaddr, kdaddr, slen, ds_msr);
			usaddr += slen, kdaddr += slen, len -= slen;
		}
	}
	if (len > 0) {
		copyin_bytes(usaddr, kdaddr, len, ds_msr);
	}
	pcb->pcb_onfault = NULL;
	return 0;
}

int
copyinstr(const void *usaddr, void *kdaddr, size_t len, size_t *done)
{
	struct pcb * const pcb = lwp_getpcb(curlwp);
	struct faultbuf env;

	if (__predict_false(len == 0)) {
		if (done)
			*done = 0;
		return 0;
	}

	int rv = setfault(&env);
	if (rv != 0) {
		pcb->pcb_onfault = NULL;
		if (done)
			*done = 0;
		return rv;
	}

	const register_t ds_msr = mfmsr() | PSL_DS;
	const uint32_t *usaddr32 = (const void *)((uintptr_t)usaddr & ~3);
	uint8_t *kdaddr8 = kdaddr;
	size_t copylen, wlen;
	uint32_t data;
	size_t uoff = (uintptr_t)usaddr & 3;
	wlen = 4 - uoff;
	/*
	 * We need discard any leading bytes if the address was
	 * unaligned.  We read the words byteswapped so that the LSB
	 * contains the lowest address byte.
	 */
	data = copyin_word_bswap(usaddr32++, ds_msr) >> (8 * uoff);
	for (copylen = 0; copylen < len; copylen++, wlen--, data >>= 8) {
		if (wlen == 0) {
			/*
			 * If we've depleted the data in the word, fetch the
			 * next one.
			 */
			data = copyin_word_bswap(usaddr32++, ds_msr);
			wlen = 4;
		}
		*kdaddr8++ = data;
		if ((uint8_t) data == 0) {
			copylen++;
			break;
		}
	}

	pcb->pcb_onfault = NULL;
	if (done)
		*done = copylen;
	/*
	 * If the last byte is not NUL (0), then the name is too long.
	 */
	return (uint8_t)data ? ENAMETOOLONG : 0;
}
