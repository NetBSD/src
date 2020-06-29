/*	$NetBSD: t_mbtowc.c,v 1.3 2020/06/29 20:53:40 maya Exp $ */

/*-
 * Copyright (c) 2005 Miloslav Trmac
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* From: Miloslav Trmac <mitr@volny.cz> */

#include <atf-c.h>
#include <langinfo.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>

ATF_TC(mbtowc_sign);
ATF_TC_HEAD(mbtowc_sign, tc)
{
	atf_tc_set_md_var(tc, "descr", "Test mbtowc(3) sign conversion");
}

ATF_TC_BODY(mbtowc_sign, tc)
{
	char back[MB_LEN_MAX];
	wchar_t wc;
	size_t i;
	int ret;

	(void)setlocale(LC_ALL, "");
	(void)printf("Charset: %s\n", nl_langinfo(CODESET));
	ret = mbtowc(&wc, "\xe4", 1);
	(void)printf("mbtowc(): %d\n", ret);

	if (ret > 0) {
		(void)printf("Result: 0x%08lX\n",(unsigned long)wc);
		ret = wctomb(back, wc);
		(void)printf("wctomb(): %d\n", ret);
		for(i = 0; ret > 0 && i < (size_t)ret; i++)
			printf("%02X ",(unsigned char)back[i]);
		putchar('\n');
	}

	ATF_REQUIRE(ret > 0);
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, mbtowc_sign);

	return atf_no_error();
}
