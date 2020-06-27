
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
