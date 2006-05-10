/*	$NetBSD: convfp.c,v 1.5 2006/05/10 19:07:22 mrg Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * This value is representable as an unsigned int, but not as an int.
 * According to ISO C it must survive the convsion back from a double
 * to an unsigned int (everything > -1 and < UINT_MAX+1 has to)
 */ 
#define	UINT_TESTVALUE	(INT_MAX+42U)

/* The same for unsigned long */
#define ULONG_TESTVALUE	(LONG_MAX+42UL)

static void test1();
static void test2();

int
main()
{
	test1();
	test2();
	printf("PASSED\n");
	return 0;
}

static void
test1()
{
	unsigned int ui;
	unsigned long ul;
	long double dt;
	double d;

	/* unsigned int test */
	d = UINT_TESTVALUE;
	ui = (unsigned int)d;

	if (ui != UINT_TESTVALUE) {
		printf("FAILED: unsigned int %u (0x%x) != %u (0x%x)\n",
		    ui, ui, UINT_TESTVALUE, UINT_TESTVALUE);
		exit(1);
	}

	/* unsigned long vs. {long} double test */
	if (sizeof(d) > sizeof(ul)) {
		d = ULONG_TESTVALUE;
		ul = (unsigned long)d;
		printf("testing double vs. long\n");
	} else if (sizeof(dt) > sizeof(ul)) {
		dt = ULONG_TESTVALUE;
		ul = (unsigned long)dt;
		printf("testing long double vs. long\n");
	} else {
		printf("no suitable {long} double type found, skipping "
		    "\"unsigned long\" test\n");
		printf("sizeof(long) = %d, sizeof(double) = %d, "
		    "sizeof(long double) = %d\n", 
		    sizeof(ul), sizeof(d), sizeof(dt));
		return;
	}

	if (ul != ULONG_TESTVALUE) {
		printf("FAILED: unsigned long %lu (0x%lx) != %lu (0x%lx)\n",
		    ul, ul, ULONG_TESTVALUE, ULONG_TESTVALUE);
		exit(1);
	}
}

static void
test2()
{
	double nv;
	unsigned long uv;

	printf("testing double to unsigned long cast\n");
	nv = 5.6;
	uv = (unsigned long)nv;

	if (uv == 5)
		return;

	printf("FAILED: %.3f casted to unsigned long is %lu\n", nv, uv);
	exit(1);
}
