/* $NetBSD: t_crypt.c,v 1.6 2021/10/12 13:24:21 nia Exp $ */

/*
 * This version is derived from the original implementation of FreeSec
 * (release 1.1) by David Burren.  I've reviewed the changes made in
 * OpenBSD (as of 2.7) and modified the original code in a similar way
 * where applicable.  I've also made it reentrant and made a number of
 * other changes.
 * - Solar Designer <solar at openwall.com>
 */

/*
 * FreeSec: libcrypt for NetBSD
 *
 * Copyright (c) 1994 David Burren
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
 * 3. Neither the name of the author nor the names of other contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	$Owl: Owl/packages/glibc/crypt_freesec.c,v 1.6 2010/02/20 14:45:06 solar Exp $
 *	Id: crypt.c,v 1.15 1994/09/13 04:58:49 davidb Exp
 *
 * This is an original implementation of the DES and the crypt(3) interfaces
 * by David Burren <davidb at werj.com.au>.
 *
 * An excellent reference on the underlying algorithm (and related
 * algorithms) is:
 *
 *	B. Schneier, Applied Cryptography: protocols, algorithms,
 *	and source code in C, John Wiley & Sons, 1994.
 *
 * Note that in that book's description of DES the lookups for the initial,
 * pbox, and final permutations are inverted (this has been brought to the
 * attention of the author).  A list of errata for this book has been
 * posted to the sci.crypt newsgroup by the author and is available for FTP.
 *
 * ARCHITECTURE ASSUMPTIONS:
 *	This code used to have some nasty ones, but these have been removed
 *	by now.	 The code requires a 32-bit integer type, though.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: t_crypt.c,v 1.6 2021/10/12 13:24:21 nia Exp $");

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
#if defined(HAVE_ARGON2)
/* Argon2i version number 16 */
/* 33 */	{ "$argon2i$v=16$m=256,t=2,p=1$c29tZXNhbHQ$/U3YPXYsSb3q9XxHvc0MLxur+GP960kN9j7emXX8zwY", "password" },
/* 34 */	{ "$argon2i$v=16$m=65536,t=1,p=1$c29tZXNhbHQ$gWMFUrjzsfSM2xmSxMZ4ZD1JCytetP9sSzQ4tWIXJLI", "password" },
/* 35 */	{ "$argon2i$v=16$m=65536,t=2,p=1$ZGlmZnNhbHQ$eaEDuQ/orvhXDLMfyLIiWXeJFvgza3vaw4kladTxxJc", "password" },
/* 36 */	{ "$argon2i$v=16$m=65536,t=2,p=1$c29tZXNhbHQ$6ckCB0tnVFMaOgvlGeW69ASzDOabPwGsO/ISKZYBCaM", "differentpassword" },
/* Argon2i version number 19 */
/* 37 */	{ "$argon2i$v=19$m=256,t=2,p=1$c29tZXNhbHQ$iekCn0Y3spW+sCcFanM2xBT63UP2sghkUoHLIUpWRS8", "password" },
/* 38 */	{ "$argon2i$v=19$m=65536,t=1,p=1$c29tZXNhbHQ$0WgHXE2YXhPr6uVgz4uUw7XYoWxRkWtvSsLaOsEbvs8", "password" },
/* 39 */	{ "$argon2i$v=19$m=65536,t=2,p=1$ZGlmZnNhbHQ$sDV8zPvvkfOGCw26RHsjSMvv7K2vmQq/6cxAcmxSEnE", "password" },
/* 40 */	{ "$argon2i$v=19$m=65536,t=2,p=1$c29tZXNhbHQ$FK6NoBr+qHAMI1jc73xTWNkCEoK9iGY6RWL1n7dNIu4", "differentpassword" },
/* Argon2id version number 19 */
/* 41 */	{ "$argon2id$v=19$m=256,t=2,p=2$c29tZXNhbHQ$bQk8UB/VmZZF4Oo79iDXuL5/0ttZwg2f/5U52iv1cDc", "password" },
/* 42 */	{ "$argon2id$v=19$m=65536,t=4,p=1$c29tZXNhbHQ$kCXUjmjvc5XMqQedpMTsOv+zyJEf5PhtGiUghW9jFyw", "password" },
/* 43 */	{ "$argon2id$v=19$m=65536,t=2,p=1$ZGlmZnNhbHQ$vfMrBczELrFdWP0ZsfhWsRPaHppYdP3MVEMIVlqoFBw", "password" },
/* 44 */	{ "$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHQ$C4TWUs9rDEvq7w3+J4umqA32aWKB1+DSiRuBfYxFj94", "differentpassword" },
#endif
/* 45 */	{ NULL, NULL }
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
		if (!hash) {
			ATF_CHECK_MSG(0, "Test %zu NULL\n", i);
			continue;
		}
		if (strcmp(hash, "*0") == 0 && strlen(tests[i].hash) < 13)
			continue; /* expected failure */
		if (strcmp(hash, tests[i].hash))
			ATF_CHECK_MSG(0, "Test %zu %s != %s\n",
			    i, hash, tests[i].hash);
	}
}

ATF_TP_ADD_TCS(tp)
{

	ATF_TP_ADD_TC(tp, crypt_salts);
	return atf_no_error();
}
