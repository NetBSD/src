/*	$NetBSD: t_bitops.c,v 1.1 2012/12/01 16:27:27 christos Exp $	*/

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__RCSID("$NetBSD: t_bitops.c,v 1.1 2012/12/01 16:27:27 christos Exp $");

#include <stdlib.h>
#include <sys/bitops.h>
#include <atf-c.h>

ATF_TC(bitmap_basic);

ATF_TC_HEAD(bitmap_basic, tc)
{
        atf_tc_set_md_var(tc, "descr", "A basic test of __BITMAP_*");
}
     
ATF_TC_BODY(bitmap_basic, tc)
{
	uint32_t bm[__BITMAP_SIZE(uint32_t, 65536)];
	__BITMAP_ZERO(bm);

	ATF_REQUIRE(__BITMAP_SIZE(uint32_t, 65536) == 2048);

	ATF_REQUIRE(__BITMAP_SHIFT(uint32_t) == 5);

	ATF_REQUIRE(__BITMAP_MASK(uint32_t) == 31);

	for (size_t i = 0; i < 65536; i += 2)
		__BITMAP_SET(i, bm);

	for (size_t i = 0; i < 2048; i++)
		ATF_REQUIRE(bm[i] == 0x55555555);

	for (size_t i = 0; i < 65536; i++)
		if (i & 1)
			ATF_REQUIRE(!__BITMAP_ISSET(i, bm));
		else {
			ATF_REQUIRE(__BITMAP_ISSET(i, bm));
			__BITMAP_CLR(i, bm);
		}

	for (size_t i = 0; i < 65536; i += 2)
		ATF_REQUIRE(!__BITMAP_ISSET(i, bm));
}

ATF_TP_ADD_TCS(tp)
{
        ATF_TP_ADD_TC(tp, bitmap_basic);
	return atf_no_error();
}
