/*	$NetBSD: t_hash.c,v 1.1.4.2 2023/08/09 16:16:40 martin Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
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

#include <atf-c.h>
#include <stdint.h>

#include "hash.h"

/* known-answer tests */
struct kat {
	const char *in;
	unsigned long long out;
};

/*
 * From the SysV spec, with uint64_t instead of unsigned long to
 * illustrate the problem on all systems, not just LP64 ones.
 *
 * https://www.sco.com/developers/devspecs/gabi41.pdf#page=95
 */
static uint64_t
sysv_broken_hash(const char *name)
{
	uint64_t h = 0, g;

	while (*name) {
		h = (h << 4) + *name++;
		if ((g = h & 0xf0000000) != 0)
			h ^= g >> 24;
		h &= ~g;
	}

	return h;
}

ATF_TC(sysv);
ATF_TC_HEAD(sysv, tc)
{
	atf_tc_set_md_var(tc, "descr", "SysV hash (32-bit)");
}
ATF_TC_BODY(sysv, tc)
{
	static const struct kat kat[] = {
		{ "", 0x00000000 },
		{ "a", 0x00000061 },
		{ "aa", 0x00000671 },
		{ "aaa", 0x00006771 },
		{ "aaaa", 0x00067771 },
		{ "aaaaa", 0x00677771 },
		{ "aaaaaa", 0x06777771 },
		{ "aaaaaaa", 0x07777711 },
		{ "aaaaaaaa", 0x07777101 },
		{ "aaaaaaaaa", 0x07771001 },
		{ "ab", 0x00000672 },
		{ "abc", 0x00006783 },
		{ "abcd", 0x00067894 },
		{ "abcde", 0x006789a5 },
		{ "abcdef", 0x06789ab6 },
		{ "abcdefg", 0x0789aba7 },
		{ "abcdefgh", 0x089abaa8 },
		{ "abcdefghi", 0x09abaa69 },
		/* https://maskray.me/blog/2023-04-12-elf-hash-function */
		{ "Z", 0x0000005a },
		{ "ZZ", 0x000005fa },
		{ "ZZZ", 0x00005ffa },
		{ "ZZZZ", 0x0005fffa },
		{ "ZZZZZ", 0x005ffffa },
		{ "ZZZZZW", 0x05fffff7 },
		{ "ZZZZZW9", 0x0ffffff9 },
		{ "ZZZZZW9p", 0x00000000 },
		{ "pneumonoultramicroscopicsilicovolcanoconiosis",
		  0x051706b3 },
	};
	unsigned i;

	for (i = 0; i < __arraycount(kat); i++) {
		unsigned long long h = _rtld_elf_hash(kat[i].in);

		ATF_CHECK_EQ_MSG(h, kat[i].out,
		    "[%u] _rtld_elf_hash(\"%s\") = 0x%08llx != 0x%08llx",
		    i, kat[i].in, h, kat[i].out);
	}
}

ATF_TC(sysv_broken);
ATF_TC_HEAD(sysv_broken, tc)
{
	atf_tc_set_md_var(tc, "descr",
	    "SysV hash (broken with 64-bit unsigned long)");
}
ATF_TC_BODY(sysv_broken, tc)
{
	static const struct kat kat[] = {
		{ "", 0x00000000 },
		{ "a", 0x00000061 },
		{ "aa", 0x00000671 },
		{ "aaa", 0x00006771 },
		{ "aaaa", 0x00067771 },
		{ "aaaaa", 0x00677771 },
		{ "aaaaaa", 0x06777771 },
		{ "aaaaaaa", 0x07777711 },
		{ "aaaaaaaa", 0x07777101 },
		{ "aaaaaaaaa", 0x07771001 },
		{ "ab", 0x00000672 },
		{ "abc", 0x00006783 },
		{ "abcd", 0x00067894 },
		{ "abcde", 0x006789a5 },
		{ "abcdef", 0x06789ab6 },
		{ "abcdefg", 0x0789aba7 },
		{ "abcdefgh", 0x089abaa8 },
		{ "abcdefghi", 0x09abaa69 },
		/* https://maskray.me/blog/2023-04-12-elf-hash-function */
		{ "Z", 0x0000005a },
		{ "ZZ", 0x000005fa },
		{ "ZZZ", 0x00005ffa },
		{ "ZZZZ", 0x0005fffa },
		{ "ZZZZZ", 0x005ffffa },
		{ "ZZZZZW", 0x05fffff7 },
		{ "ZZZZZW9", 0x0ffffff9 },
		{ "ZZZZZW9p", 0x100000000 }, /* XXX */
		{ "pneumonoultramicroscopicsilicovolcanoconiosis",
		  0x051706b3 },
	};
	unsigned i;

	for (i = 0; i < __arraycount(kat); i++) {
		unsigned long long h = sysv_broken_hash(kat[i].in);

		ATF_CHECK_EQ_MSG(h, kat[i].out,
		    "[%u] sysv_broken_hash(\"%s\") = 0x%08llx != 0x%08llx",
		    i, kat[i].in, h, kat[i].out);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, sysv);
	ATF_TP_ADD_TC(tp, sysv_broken);

	return atf_no_error();
}
