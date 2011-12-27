/* $NetBSD: t_crypt.c,v 1.1 2011/12/27 00:47:23 christos Exp $ */

/*
 * Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com) All rights
 * reserved.
 * 
 * This package is an SSL implementation written by Eric Young
 * (eay@cryptsoft.com). The implementation was written so as to conform with
 * Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as the
 * following conditions are aheared to.  The following conditions apply to
 * all code found in this distribution, be it the RC4, RSA, lhash, DES, etc.,
 * code; not just the SSL code.  The SSL documentation included with this
 * distribution is covered by the same copyright terms except that the holder
 * is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in the code
 * are not to be removed. If this package is used in a product, Eric Young
 * should be given attribution as the author of the parts of the library
 * used. This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the copyright notice,
 * this list of conditions and the following disclaimer. 2. Redistributions
 * in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other
 * materials provided with the distribution. 3. All advertising materials
 * mentioning features or use of this software must display the following
 * acknowledgement: "This product includes cryptographic software written by
 * Eric Young (eay@cryptsoft.com)" The word 'cryptographic' can be left out
 * if the rouines from the library being used are not cryptographic related
 * :-). 4. If you include any Windows specific code (or a derivative thereof)
 * from the apps directory (application code) you must include an
 * acknowledgement: "This product includes software written by Tim Hudson
 * (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply
 * be copied and put under another distribution licence [including the GNU
 * Public Licence.]
 */

#include <atf-c.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static const struct {
	const char *hash;
	const char *pw;
} tests[] = {
/* "new"-style */
/*  0 */	{ "_J9..CCCCXBrJUJV154M", "U*U*U*U*" },
/*  1 */	{ "_J9..CCCCXUhOBTXzaiE", "U*U***U" },
/*  2 */	{ "_J9..CCCC4gQ.mB/PffM", "U*U***U*" },
/*  3 */	{ "_J9..XXXXvlzQGqpPPdk", "*U*U*U*U" },
/*  4 */	{ "_J9..XXXXsqM/YSSP..Y", "*U*U*U*U*" },
/*  5 */	{ "_J9..XXXXVL7qJCnku0I", "*U*U*U*U*U*U*U*U" },
/*  6 */	{ "_J9..XXXXAj8cFbP5scI", "*U*U*U*U*U*U*U*U*" },
/*  7 */	{ "_J9..SDizh.vll5VED9g", "ab1234567" },
/*  8 */	{ "_J9..SDizRjWQ/zePPHc", "cr1234567" },
/*  9 */	{ "_J9..SDizxmRI1GjnQuE", "zxyDPWgydbQjgq" },
/* 10 */	{ "_K9..SaltNrQgIYUAeoY", "726 even" },
/* 11 */	{ "_J9..SDSD5YGyRCr4W4c", "" },
/* "old"-style, valid salts */
/* 12 */	{ "CCNf8Sbh3HDfQ", "U*U*U*U*" },
/* 13 */	{ "CCX.K.MFy4Ois", "U*U***U" },
/* 14 */	{ "CC4rMpbg9AMZ.", "U*U***U*" },
/* 15 */	{ "XXxzOu6maQKqQ", "*U*U*U*U" },
/* 16 */	{ "SDbsugeBiC58A", "" },
/* 17 */	{ "./xZjzHv5vzVE", "password" },
/* 18 */	{ "0A2hXM1rXbYgo", "password" },
/* 19 */	{ "A9RXdR23Y.cY6", "password" },
/* 20 */	{ "ZziFATVXHo2.6", "password" },
/* 21 */	{ "zZDDIZ0NOlPzw", "password" },
/* "old"-style, "reasonable" invalid salts, UFC-crypt behavior expected */
/* 22 */	{ "\001\002wyd0KZo65Jo", "password" },
/* 23 */	{ "a_C10Dk/ExaG.", "password" },
/* 24 */	{ "~\377.5OTsRVjwLo", "password" },
/* The below are erroneous inputs, so NULL return is expected/required */
/* 25 */	{ "", "" }, /* no salt */
/* 26 */	{ " ", "" }, /* setting string is too short */
/* 27 */	{ "a:", "" }, /* unsafe character */
/* 28 */	{ "\na", "" }, /* unsafe character */
/* 29 */	{ "_/......", "" }, /* setting string is too short for its type */
/* 30 */	{ "_........", "" }, /* zero iteration count */
/* 31 */	{ "_/!......", "" }, /* invalid character in count */
/* 32 */	{ "_/......!", "" }, /* invalid character in salt */
/* 33 */	{ NULL, NULL }
};

ATF_TC(crypt_salts);

ATF_TC_HEAD(crypt_salts, tc)
{

	atf_tc_set_md_var(tc, "descr", "crypt(3) salt consistency checks");
}

ATF_TC_BODY(crypt_salts, tc)
{
	for (size_t i = 0; tests[i].hash; i++) {
		char *hash = crypt(tests[i].pw, tests[i].hash);
		if (!hash && strlen(tests[i].hash) < 13)
			continue; /* expected failure */
		if (!hash || strcmp(hash, tests[i].hash))
			ATF_CHECK_MSG(0, "Test %zu %s != %s\n",
			    i, hash, tests[i].hash);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, crypt_salts);
	return atf_no_error();
}
