/* $NetBSD: wcstod_test.c,v 1.1.2.2 2006/04/28 21:54:19 riz Exp $ */

/*-
 * Copyright (c)2005 Citrus Project,
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
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

typedef struct {
	const wchar_t *wcs;
	size_t exp_len;
	double exp_val;
	int exp_errno;
} unitcase_t;

#define	ALT_HUGE_VAL		-1
#define	ALT_MINUS_HUGE_VAL	-2
#define	ALT_NAN			-3

static const unitcase_t unitcases[] = {

{ L"IN",		0,	0,			0 },
{ L"+IN",		0,	0,			0 },
{ L"-IN",		0,	0,			0 },
{ L"INX",		0,	0,			0 },
{ L"+INX",		0,	0,			0 },
{ L"-INX",		0,	0,			0 },
{ L"INF",		3,	ALT_HUGE_VAL,		0 },
{ L"+INF",		4,	ALT_HUGE_VAL,		0 },
{ L"-INF",		4,	ALT_MINUS_HUGE_VAL,	0 },
{ L"INFX",		3,	ALT_HUGE_VAL,		0 },
{ L"+INFX",		4,	ALT_HUGE_VAL,		0 },
{ L"-INFX",		4,	ALT_MINUS_HUGE_VAL,	0 },
{ L"     IN",		0,	0,			0 },
{ L"     +IN",		0,	0,			0 },
{ L"     -IN",		0,	0,			0 },
{ L"     INX",		0,	0,			0 },
{ L"     +INX",		0,	0,			0 },
{ L"     -INX",		0,	0,			0 },
{ L"+     INF",		0,	0,			0 },
{ L"-     INF",		0,	0,			0 },
{ L"     INF",		8,	ALT_HUGE_VAL,		0 },
{ L"     +INF",		9,	ALT_HUGE_VAL,		0 },
{ L"     -INF",		9,	ALT_MINUS_HUGE_VAL,	0 },
{ L"     INFX",		8,	ALT_HUGE_VAL,		0 },
{ L"     +INFX",	9,	ALT_HUGE_VAL,		0 },
{ L"     -INFX",	9,	ALT_MINUS_HUGE_VAL,	0 },
{ L"     INFINIT",	8,	ALT_HUGE_VAL,		0 },
{ L"     +INFINIT",	9,	ALT_HUGE_VAL,		0 },
{ L"     -INFINIT",	9,	ALT_MINUS_HUGE_VAL,	0 },
{ L"     INFINITY",	13,	ALT_HUGE_VAL,		0 },
{ L"     +INFINITY",	14,	ALT_HUGE_VAL,		0 },
{ L"     -INFINITY",	14,	ALT_MINUS_HUGE_VAL,	0 },
{ L"     INFINITYX",	13,	ALT_HUGE_VAL,		0 },
{ L"     +INFINITYX",	14,	ALT_HUGE_VAL,		0 },
{ L"     -INFINITYX",	14,	ALT_MINUS_HUGE_VAL,	0 },

/* NAN */
{ L"NA",		0,	0,			0 },
{ L"+NA",		0,	0,			0 },
{ L"-NA",		0,	0,			0 },
{ L"NAX",		0,	0,			0 },
{ L"+NAX",		0,	0,			0 },
{ L"-NAX",		0,	0,			0 },
{ L"NAN",		3,	ALT_NAN,		0 },
{ L"+NAN",		4,	ALT_NAN,		0 },
{ L"-NAN",		4,	ALT_NAN,		0 },
{ L"NANX",		3,	ALT_NAN,		0 },
{ L"+NANX",		4,	ALT_NAN,		0 },
{ L"-NANX",		4,	ALT_NAN,		0 },
{ L"     NA",		0,	0,			0 },
{ L"     +NA",		0,	0,			0 },
{ L"     -NA",		0,	0,			0 },
{ L"     NAX",		0,	0,			0 },
{ L"     +NAX",		0,	0,			0 },
{ L"     -NAX",		0,	0,			0 },
{ L"+     NAN",		0,	0,			0 },
{ L"-     NAN",		0,	0,			0 },
{ L"     NAN",		8,	ALT_NAN,		0 },
{ L"     +NAN",		9,	ALT_NAN,		0 },
{ L"     -NAN",		9,	ALT_NAN,		0 },
{ L"     NANX",		8,	ALT_NAN,		0 },
{ L"     +NANX",	9,	ALT_NAN,		0 },
{ L"     -NANX",	9,	ALT_NAN,		0 },

{ L"0",			1,	0,			0 },
{ L"+0",		2,	0,			0 },
{ L"-0",		2,	0,			0 },
{ L"          0",	11,	0,			0 },
{ L"          +0",	12,	0,			0 },
{ L"          -0",	12,	0,			0 },
{ L"+          0",	0,	0,			0 },
{ L"-          0",	0,	0,			0 },

{ L".",			0,	0,			0 },
{ L".0",		2,	0,			0 },
{ L".00",		3,	0,			0 },
{ L".000",		4,	0,			0 },

{ L"0.",		2,	0,			0 },
{ L"+0.",		3,	0,			0 },
{ L"-0.",		3,	0,			0 },
{ L"          0.",	12,	0,			0 },
{ L"          +0.",	13,	0,			0 },
{ L"          -0.",	13,	0,			0 },

{ L"0.0",		3,	0,			0 },
{ L"+0.0",		4,	0,			0 },
{ L"-0.0",		4,	0,			0 },
{ L"          0.0",	13,	0,			0 },
{ L"          +0.0",	14,	0,			0 },
{ L"          -0.0",	14,	0,			0 },

{ L"000",		3,	0,			0 },
{ L"+000",		4,	0,			0 },
{ L"-000",		4,	0,			0 },
{ L"          000",	13,	0,			0 },
{ L"          +000",	14,	0,			0 },
{ L"          -000",	14,	0,			0 },

{ L"000.",		4,	0,			0 },
{ L"+000.",		5,	0,			0 },
{ L"-000.",		5,	0,			0 },
{ L"          000.",	14,	0,			0 },
{ L"          +000.",	15,	0,			0 },
{ L"          -000.",	15,	0,			0 },

{ L"000.0",		5,	0,			0 },
{ L"+000.0",		6,	0,			0 },
{ L"-000.0",		6,	0,			0 },
{ L"          000.0",	15,	0,			0 },
{ L"          +000.0",	16,	0,			0 },
{ L"          -000.0",	16,	0,			0 },


{ L"0.0.",		3,	0,			0 },
{ L"+0.0.",		4,	0,			0 },
{ L"-0.0.",		4,	0,			0 },
{ L"          0.0.",	13,	0,			0 },
{ L"          +0.0.",	14,	0,			0 },
{ L"          -0.0.",	14,	0,			0 },

{ L"0.0.0",		3,	0,			0 },
{ L"+0.0.0",		4,	0,			0 },
{ L"-0.0.0",		4,	0,			0 },
{ L"          0.0.0",	13,	0,			0 },
{ L"          +0.0.0",	14,	0,			0 },
{ L"          -0.0.0",	14,	0,			0 },

/* XXX: FIXME */
#if defined(__NetBSD__)
{ L"0X",		0,	0,			0 },
{ L"+0X",		0,	0,			0 },
{ L"-0X",		0,	0,			0 },
#elif defined(__linux__)
{ L"0X",		2,	0,			0 },
{ L"+0X",		3,	0,			0 },
{ L"-0X",		3,	0,			0 },
#else
{ L"0X",		1,	0,			0 },
{ L"+0X",		2,	0,			0 },
{ L"-0X",		2,	0,			0 },
#endif

/* XXX: SunOS 5.8's wcstod(3) doesn't accept hex */
#if !defined(__SunOS__)
/* XXX: FIXME */
#if defined(__NetBSD__)
{ L"0X.",		0,	0,			0 },
{ L"+0X.",		0,	0,			0 },
{ L"-0X.",		0,	0,			0 },
{ L"          0X.",	0,	0,			0 },
{ L"          +0X.",	0,	0,			0 },
{ L"          -0X.",	0,	0,			0 },
#elif defined(__linux__)
{ L"0X.",		3,	0,			0 },
{ L"+0X.",		4,	0,			0 },
{ L"-0X.",		4,	0,			0 },
{ L"          0X.",	13,	0,			0 },
{ L"          +0X.",	14,	0,			0 },
{ L"          -0X.",	14,	0,			0 },
#else
{ L"0X.",		1,	0,			0 },
{ L"+0X.",		2,	0,			0 },
{ L"-0X.",		2,	0,			0 },
{ L"          0X.",	11,	0,			0 },
{ L"          +0X.",	12,	0,			0 },
{ L"          -0X.",	12,	0,			0 },
#endif
/* XXX: FIXME */
#if defined(__NetBSD__) || defined(__linux__)
{ L"0X.0",		4,	0,			0 },
{ L"+0X.0",		5,	0,			0 },
{ L"-0X.0",		5,	0,			0 },
{ L"          0X.0",	14,	0,			0 },
{ L"          +0X.0",	15,	0,			0 },
{ L"          -0X.0",	15,	0,			0 },

{ L"0X.0P",		4,	0,			0 },
{ L"+0X.0P",		5,	0,			0 },
{ L"-0X.0P",		5,	0,			0 },
{ L"          0X.0P",	14,	0,			0 },
{ L"          +0X.0P",	15,	0,			0 },
{ L"          -0X.0P",	15,	0,			0 },
#else
{ L"0X.0",		1,	0,			0 },
{ L"+0X.0",		2,	0,			0 },
{ L"-0X.0",		2,	0,			0 },
{ L"          0X.0",	11,	0,			0 },
{ L"          +0X.0",	12,	0,			0 },
{ L"          -0X.0",	12,	0,			0 },

{ L"0X.0P",		1,	0,			0 },
{ L"+0X.0P",		2,	0,			0 },
{ L"-0X.0P",		2,	0,			0 },
{ L"          0X.0P",	11,	0,			0 },
{ L"          +0X.0P",	12,	0,			0 },
{ L"          -0X.0P",	12,	0,			0 },
#endif

{ L"0X0",		3,	0,			0 },
{ L"+0X0",		4,	0,			0 },
{ L"-0X0",		4,	0,			0 },
{ L"          0X0",	13,	0,			0 },
{ L"          +0X0",	14,	0,			0 },
{ L"          -0X0",	14,	0,			0 },

{ L"00X0.0",		2,	0,			0 },
{ L"+00X0.0",		3,	0,			0 },
{ L"-00X0.0",		3,	0,			0 },
{ L"          00X0.0",	12,	0,			0 },
{ L"          +00X0.0",	13,	0,			0 },
{ L"          -00X0.0",	13,	0,			0 },

{ L"0X0P",		3,	0,			0 },
{ L"+0X0P",		4,	0,			0 },
{ L"-0X0P",		4,	0,			0 },
{ L"          0X0P",	13,	0,			0 },
{ L"          +0X0P",	14,	0,			0 },
{ L"          -0X0P",	14,	0,			0 },

{ L"0X0.",		4,	0,			0 },
{ L"+0X0.",		5,	0,			0 },
{ L"-0X0.",		5,	0,			0 },
{ L"          0X0.",	14,	0,			0 },
{ L"          +0X0.",	15,	0,			0 },
{ L"          -0X0.",	15,	0,			0 },

{ L"0X0.0",		5,	0,			0 },
{ L"+0X0.0",		6,	0,			0 },
{ L"-0X0.0",		6,	0,			0 },
{ L"          0X0.0",	15,	0,			0 },
{ L"          +0X0.0",	16,	0,			0 },
{ L"          -0X0.0",	16,	0,			0 },

{ L"0X0.P",		4,	0,			0 },
{ L"+0X0.P",		5,	0,			0 },
{ L"-0X0.P",		5,	0,			0 },
{ L"          0X0.P",	14,	0,			0 },
{ L"          +0X0.P",	15,	0,			0 },
{ L"          -0X0.P",	15,	0,			0 },

{ L"0X0.P",		4,	0,			0 },
{ L"+0X0.P",		5,	0,			0 },
{ L"-0X0.P",		5,	0,			0 },
{ L"          0X0.P",	14,	0,			0 },
{ L"          +0X0.P",	15,	0,			0 },
{ L"          -0X0.P",	15,	0,			0 },

#endif
{ L"0.12345678",	10,	0.12345678,		0 },
{ L"+0.12345678",	11,	+0.12345678,		0 },
{ L"-0.12345678",	11,	-0.12345678,		0 },
{ L"     0.12345678",	15,	0.12345678,		0 },
{ L"     +0.12345678",	16,	+0.12345678,		0 },
{ L"     -0.12345678",	16,	-0.12345678,		0 },

{ L"0.12345E67",	10,	0.12345E67,		0 },
{ L"+0.12345E67",	11,	+0.12345E67,		0 },
{ L"-0.12345E67",	11,	-0.12345E67,		0 },
{ L"     0.12345E67",	15,	0.12345E67,		0 },
{ L"     +0.12345E67",	16,	+0.12345E67,		0 },
{ L"     -0.12345E67",	16,	-0.12345E67,		0 },

{ L"0.12345E+6",	10,	0.12345E+6,		0 },
{ L"+0.12345E+6",	11,	+0.12345E+6,		0 },
{ L"-0.12345E+6",	11,	-0.12345E+6,		0 },
{ L"     0.12345E+6",	15,	0.12345E+6,		0 },
{ L"     +0.12345E+6",	16,	+0.12345E+6,		0 },
{ L"     -0.12345E+6",	16,	-0.12345E+6,		0 },

{ L"0.98765E-4",	10,	0.98765E-4,		0 },
{ L"+0.98765E-4",	11,	+0.98765E-4,		0 },
{ L"-0.98765E-4",	11,	-0.98765E-4,		0 },
{ L"     0.98765E-4",	15,	0.98765E-4,		0 },
{ L"     +0.98765E-4",	16,	+0.98765E-4,		0 },
{ L"     -0.98765E-4",	16,	-0.98765E-4,		0 },

{ L"12345678E9",	10,	12345678E9,		0 },
{ L"+12345678E9",	11,	+12345678E9,		0 },
{ L"-12345678E9",	11,	-12345678E9,		0 },
{ L"     12345678E9",	15,	12345678E9,		0 },
{ L"     +12345678E9",	16,	+12345678E9,		0 },
{ L"     -12345678E9",	16,	-12345678E9,		0 },

/* XXX: SunOS 5.8's wcstod(3) doesn't accept hex */
#if !defined(__SunOS__)
{ L"0x1P+2",		6,	4,			0 },
{ L"+0x1P+2",		7,	+4,			0 },
{ L"-0x1P+2",		7,	-4,			0 },
{ L"     0x1P+2",	11,	4,			0 },
{ L"     +0x1P+2",	12,	+4,			0 },
{ L"     -0x1P+2",	12,	-4,			0 },

{ L"0x1.0P+2",		8,	4,			0 },
{ L"+0x1.0P+2",		9,	+4,			0 },
{ L"-0x1.0P+2",		9,	-4,			0 },
{ L"     0x1.0P+2",	13,	4,			0 },
{ L"     +0x1.0P+2",	14,	+4,			0 },
{ L"     -0x1.0P+2",	14,	-4,			0 },

{ L"0x1P-2",		6,	0.25,			0 },
{ L"+0x1P-2",		7,	+0.25,			0 },
{ L"-0x1P-2",		7,	-0.25,			0 },
{ L"     0x1P-2",	11,	0.25,			0 },
{ L"     +0x1P-2",	12,	+0.25,			0 },
{ L"     -0x1P-2",	12,	-0.25,			0 },

{ L"0x1.0P-2",		8,	0.25,			0 },
{ L"+0x1.0P-2",		9,	+0.25,			0 },
{ L"-0x1.0P-2",		9,	-0.25,			0 },
{ L"     0x1.0P-2",	13,	0.25,			0 },
{ L"     +0x1.0P-2",	14,	+0.25,			0 },
{ L"     -0x1.0P-2",	14,	-0.25,			0 },
#endif

{ NULL }
};

int
main(void)
{
	const unitcase_t *ptr;
	double d;
	size_t n;
	char *buf;
	wchar_t *tail;

	for (ptr = &unitcases[0]; ptr->wcs != NULL; ++ptr) {
		/* we do not supported %ls nor %S yet. */
		n = wcstombs(NULL, ptr->wcs, 0);
		buf = (void *)malloc(n + 1);
		if (buf == NULL)
			abort();
		(void)wcstombs(buf, ptr->wcs, n + 1);
		printf("[%.*s]====================================\n", n, buf);
		errno = 0;
		d = wcstod(ptr->wcs, &tail);
		printf("[errno]\n");
		printf("    real:     %s\n", strerror(errno));
		printf("    expected: %s\n", strerror(ptr->exp_errno));
		assert(errno == ptr->exp_errno);
		n = (size_t)(tail - ptr->wcs);
		printf("[endptr - nptr]\n");
		printf("    real:     %d\n", n);
		printf("    expected: %d\n", ptr->exp_len);
		assert(n == ptr->exp_len);
		printf("[result]\n");
		printf("    real:     %F\n", d);
		if (ptr->exp_val == ALT_HUGE_VAL) {
			printf("    expected: %F\n", HUGE_VAL);
			assert(isinf(d) && d == HUGE_VAL);
		} else if (ptr->exp_val == ALT_MINUS_HUGE_VAL) {
			printf("    expected: %F\n", -HUGE_VAL);
			assert(isinf(d) && d == -HUGE_VAL);
		} else if (ptr->exp_val == ALT_NAN) {
			printf("    expected: %F\n", NAN);
			assert(isnan(d));
		} else {
			printf("    expected: %F\n", ptr->exp_val);
			assert(d == ptr->exp_val);
		}
	}
	return 0;
}
