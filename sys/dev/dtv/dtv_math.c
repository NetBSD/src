/* $NetBSD: dtv_math.c,v 1.2 2011/07/16 16:13:13 jmcneill Exp $ */

/*-
 * Copyright (c) 2011 Alan Barrett <apb@NetBSD.org>
 * All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: dtv_math.c,v 1.2 2011/07/16 16:13:13 jmcneill Exp $");

#include <sys/types.h>
#include <sys/bitops.h>

#include <dev/dtv/dtvif.h>


#define LOG10_2_x24 5050445	/* floor(log10(2.0) * 2**24 */

/*
 * dtv_intlog10 -- return an approximation to log10(x) * 1<<24,
 * using integer arithmetic.
 *
 * As a special case, returns 0 when x == 0.
 *
 * Results should be approximately as follows, bearing in
 * mind that this function returns only an approximation
 * to the exact results.
 *
 * dtv_intlog10(0) = 0 (special case; the mathematical value is undefined)
 * dtv_intlog10(1) = 0
 * dtv_intlog10(2) = 5050445 (approx 0.30102999 * 2**24)
 * dtv_intlog10(10) = 16777216 (1.0 * 2**24)
 * dtv_intlog10(100) = 33554432 (2.0 * 2**24)
 * dtv_intlog10(1000) = 50331648 (3.0 * 2**24)
 * dtv_intlog10(10000) = 67108864 (4.0 * 2**24)
 * dtv_intlog10(100000) = 83886080 (5.0 * 2**24)
 * dtv_intlog10(1000000) = 100663296 (6.0 * 2**24)
 * dtv_intlog10(10000000) = 117440512 (7.0 * 2**24)
 * dtv_intlog10(100000000) = 134217728 (8.0 * 2**24)
 * dtv_intlog10(1000000000) = 150994944 (9.0 * 2**24)
 * dtv_intlog10(4294967295) = 161614248 (approx 9.63295986 * 2**24)
 */
uint32_t
dtv_intlog10(uint32_t x)
{
	if (__predict_false(x == 0))
		return 0;
	/*
	 * all we do is find log2(x), as an integer between 0 and 31,
	 * and scale it.  Thus, there are only 32 values that this
	 * function will ever return.  To do a better job, we would
	 * need a lookup table and interpolation.
	 */
	return (uint32_t)(LOG10_2_x24) * (uint32_t)ilog2(x);
}
