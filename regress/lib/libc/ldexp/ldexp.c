/*	$NetBSD: ldexp.c,v 1.2 2002/02/21 07:38:17 itojun Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
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

#include <math.h>
#include <stdio.h>

int
main()
{
	double v;
	int e;

#define	FORMAT	"%23.23lg\n"

	printf("basics:\n");
	v = 1.0; e = 5;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = 1022;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1023); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = 1023;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1022); e = 1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1022); e = 2045;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = -5;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = -1021;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1022); e = 1;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = -1022;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1021); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1023); e = -2045;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1023); e = -1023;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1022); e = 1022;
	printf(FORMAT, ldexp(v, e));
	printf("\n");

	printf("zero:\n");
	v = 0.0; e = -1;
	printf(FORMAT, ldexp(v, e));
	v = 0.0; e = 0;
	printf(FORMAT, ldexp(v, e));
	v = 0.0; e = 1;
	printf(FORMAT, ldexp(v, e));
	v = 0.0; e = 1024;
	printf(FORMAT, ldexp(v, e));
	v = 0.0; e = 1025;
	printf(FORMAT, ldexp(v, e));
	v = 0.0; e = -1023;
	printf(FORMAT, ldexp(v, e));
	v = 0.0; e = -1024;
	printf(FORMAT, ldexp(v, e));
	printf("\n");

	printf("infinity:\n");
	v = ldexp(1.0, 1024); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1024); e = 0;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1024); e = 1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, 1024); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, 1024); e = 0;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, 1024); e = 1;
	printf(FORMAT, ldexp(v, e));
	printf("\n");

	printf("overflow:\n");
	v = 1.0; e = 1024;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1023); e = 1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1022); e = 2046;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = 1025;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = 1024;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, 1023); e = 1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, -1022); e = 2046;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = 1025;
	printf(FORMAT, ldexp(v, e));
	printf("\n");

	printf("denormal:\n");
	v = 1.0; e = -1023;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1022); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1023); e = -2046;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = -1024;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = -1074;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = -1023;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, -1022); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, 1023); e = -2046;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = -1024;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = -1074;
	printf(FORMAT, ldexp(v, e));
	printf("\n");

	printf("underflow:\n");
	v = 1.0; e = -1075;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1074); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, 1023); e = -2098;
	printf(FORMAT, ldexp(v, e));
	v = 1.0; e = -1076;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = -1075;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, -1074); e = -1;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(-1.0, 1023); e = -2098;
	printf(FORMAT, ldexp(v, e));
	v = -1.0; e = -1076;
	printf(FORMAT, ldexp(v, e));
	printf("\n");

	printf("denormal, large exponent:\n");
	v = ldexp(1.0, -1028); e = 1024;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1025;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1026;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1027;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1028;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1029;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1030;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1040;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1050;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1060;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1100;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1200;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1300;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1400;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1500;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1600;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1700;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1800;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 1900;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2000;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2046;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2047;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2048;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2049;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2050;
	printf(FORMAT, ldexp(v, e));
	v = ldexp(1.0, -1028); e = 2051;
	printf(FORMAT, ldexp(v, e));

	exit(0);
}
