/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas <matt@3am-software.com>.
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

#include <sys/types.h>

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <inttypes.h>
#else
#include <lib/libkern/libkern.h>
#include <machine/limits.h>
#endif 

#include <sys/endian.h>
#include <machine/types.h>

#ifdef _FORTIFY_SOURCE
#undef bzero
#undef memset
#endif

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: memset2.c,v 1.1.2.1 2009/08/17 17:24:25 matt Exp $");    
#endif /* LIBC_SCCS and not lint */

/*
 * Assume register_t is the widest non-synthetic type.
 */
typedef register_t memword_t;

#ifdef BZERO
static inline
#define	memset memset0
#endif

void *
memset(void *addr, int c, size_t len)
{
	memword_t *dstp = addr;
	memword_t *edstp;
	memword_t fill;
#ifndef __OPTIMIZE_SIZE__
	memword_t keep_mask = 0;
#endif
	size_t fill_count;

	_DIAGASSERT(addr != 0);

	if (__predict_false(len == 0))
		return addr;

	/*
	 * Pad out the fill byte (v) across a memword_t.
	 * The conditional at the end prevents GCC from complaing about
	 * shift count >= width of type 
	 */
	fill = c;
	fill |= fill << 8;
	fill |= fill << 16;
	fill |= fill << (sizeof(c) < sizeof(fill) ? 32 : 0);

	/*
	 * Get the number of unaligned bytes to fill in the first word.
	 */
	fill_count = -(uintptr_t)addr & (sizeof(memword_t) - 1);

	if (__predict_false(fill_count != 0)) {
#ifndef __OPTIMIZE_SIZE__
		/*
		 * We want to clear <fill_count> trailing bytes in the word.
		 * On big/little endian, these are the least/most significant,
		 * bits respectively.  So as we shift, the keep_mask will only
		 * have bits set for the bytes we won't be filling.
		 */
#if BYTE_ORDER == BIG_ENDIAN
		keep_mask = ~(memword_t)0U << (fill_count * 8);
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
		keep_mask = ~(memword_t)0U << (fill_count * 8);
#endif
		/*
		 * Make sure dstp is aligned to a memword_t boundary.
		 */
		dstp = (memword_t *)((uintptr_t)addr & -sizeof(memword_t));
		if (len >= fill_count) {
			/*
			 * If we can fill the rest of this word, then we mask
			 * off the bytes we are filling and then fill in those
			 * bytes with the new fill value.
			 */
			*dstp = (*dstp & keep_mask) | (fill & ~keep_mask);
			len -= fill_count;
			if (__predict_false(len == 0))
				return addr;
			/*
			 * Since we were able to fill the rest of this word,
			 * we will advance to the next word and thus have no
			 * bytes to preserve.
			 *
			 * If we don't have enough to fill the rest of this
			 * word, we will fall through the following loop
			 * (since there are no full words to fill).  Then we
			 * use the keep_mask above to preserve the leading
			 * bytes of word.
			 */
			dstp++;
			keep_mask = 0;
		}
#else /* __OPTIMIZE_SIZE__ */
		uint8_t *dp, *ep;
		for (dp = (uint8_t *)dstp, ep = dp + fill_count;
		     dp != ep; dp++)
			*dp = fill;
		dstp = (memword_t *)ep;
		len -= fill_count;
#endif /* __OPTIMIZE_SIZE__ */
	}

	/*
	 * Simply fill memory one word at time (for as many full words we have
	 * to write).
	 */
	for (edstp = dstp + len / sizeof(memword_t); dstp != edstp; dstp++)
		*dstp = fill;

	/*
	 * We didn't subtract out the full words we just filled since we know
	 * by the time we get here we will have less than a words worth to
	 * write.  So we can concern ourselves with only the subword len bits.
	 */
	len &= sizeof(memword_t)-1;
	if (len > 0) {
#ifndef __OPTIMIZE_SIZE__
		/*
		 * We want to clear <len> leading bytes in the word.
		 * On big/little endian, these are the most/least significant
		 * bits, respectively,  But as we want the mask of the bytes to
		 * keep, we have to complement the mask.  So after we shift,
		 * the keep_mask will only have bits set for the bytes we won't
		 * be filling.
		 *
		 * But the keep_mask could already have bytes to preserve
		 * if the amount to fill was less than the amount of traiing
		 * space in the first word.
		 */
#if BYTE_ORDER == BIG_ENDIAN
		keep_mask |= ~(~(memword_t)0U >> (len * 8));
#endif
#if BYTE_ORDER == LITTLE_ENDIAN
		keep_mask |= ~(~(memword_t)0U << (len * 8));
#endif
		/*
		 * Now we mask off the bytes we are filling and then fill in
		 * those bytes with the new fill value.
		 */
		*dstp = (*dstp & keep_mask) | (fill & ~keep_mask);
#else /* __OPTIMIZE_SIZE__ */
		uint8_t *dp, *ep;
		for (dp = (uint8_t *)dstp, ep = dp + len;
		     dp != ep; dp++)
			*dp = fill;
#endif /* __OPTIMIZE_SIZE__ */
	}

	/*
	 * Return the initial addr
	 */
	return addr;
}

#ifdef BZERO
/*
 * For bzero, simply inline memset and let the compiler optimize things away.
 */
void
bzero(void *addr, size_t len)
{
	memset(addr, 0, len);
}
#endif
