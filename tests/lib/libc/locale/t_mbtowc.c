/* $NetBSD: t_mbtowc.c,v 1.3 2020/06/30 16:09:40 jruoho Exp $ */

/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
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

/*-
 * Copyright (c)2007 Citrus Project,
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

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

#include <sys/cdefs.h>
__COPYRIGHT("@(#) Copyright (c) 2011\
 The NetBSD Foundation, inc. All rights reserved.");
__RCSID("$NetBSD: t_mbtowc.c,v 1.3 2020/06/30 16:09:40 jruoho Exp $");

#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vis.h>
#include <wchar.h>

#include <atf-c.h>

static void
h_mbtowc(const char *locale, const char *illegal,
    const char *legal, size_t stateful)
{
	char buf[64];
	size_t ret;
	char *str;

	ATF_REQUIRE_STREQ(setlocale(LC_ALL, "C"), "C");
	(void)printf("Trying locale: %s\n", locale);
	ATF_REQUIRE(setlocale(LC_CTYPE, locale) != NULL);

	ATF_REQUIRE((str = setlocale(LC_ALL, NULL)) != NULL);
	(void)printf("Using locale: %s\n", str);
	(void)printf("Locale is state-%sdependent\n",
		!stateful ? "in" : "");

	/* initialize internal state */
	ret = mbtowc(NULL, NULL, 0);
	ATF_REQUIRE(stateful ? ret : !ret);

	(void)strvis(buf, illegal, VIS_WHITE | VIS_OCTAL);
	(void)printf("Checking illegal sequence: \"%s\"\n", buf);

	ret = mbtowc(NULL, illegal, strlen(illegal));
	(void)printf("mbtowc() returned: %zd\n", ret);
	ATF_REQUIRE_EQ(ret, (size_t)-1);
	(void)printf("errno: %s\n", strerror(errno));
	ATF_REQUIRE_EQ(errno, EILSEQ);

	/*
	 * If this is stateless encoding, this
	 * re-initialization is not required.
	 */
	if (stateful) {
		/* re-initialize internal state */
		mbtowc(NULL, NULL, 0);
	}

	/* valid multibyte sequence case */
	(void)strvis(buf, legal, VIS_WHITE | VIS_OCTAL);
	(void)printf("Checking legal sequence: \"%s\"\n", buf);

	errno = 0;
	ret = mbtowc(NULL, legal, strlen(legal));
	(void)printf("mbtowc() returned: %zd\n", ret);
	ATF_REQUIRE(ret != (size_t)-1);
	(void)printf("errno: %s\n", strerror(errno));
	ATF_REQUIRE_EQ(errno, 0);

	(void)printf("Ok.\n");
}

ATF_TC(mbtowc_basic);
ATF_TC_HEAD(mbtowc_basic, tc)
{
	atf_tc_set_md_var(tc, "descr", "A basic test of mbtowc(3)");
}

ATF_TC_BODY(mbtowc_basic, tc)
{
	h_mbtowc("en_US.UTF-8", "\240", "\302\240", 0);
	h_mbtowc("ja_JP.ISO2022-JP", "\033$B", "\033$B$\"\033(B", 1);
	h_mbtowc("ja_JP.SJIS", "\202", "\202\240", 0);
	h_mbtowc("ja_JP.eucJP", "\244", "\244\242", 0);
	h_mbtowc("zh_CN.GB18030", "\241", "\241\241", 0);
	h_mbtowc("zh_TW.Big5", "\241", "\241@", 0);
	h_mbtowc("zh_TW.eucTW", "\241", "\241\241", 0);
}

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

	ATF_TP_ADD_TC(tp, mbtowc_basic);
	ATF_TP_ADD_TC(tp, mbtowc_sign);

	return atf_no_error();
}
