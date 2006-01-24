/* $NetBSD: qfile.c,v 1.1 2006/01/24 18:59:23 elad Exp $ */

/*-
 * Copyright 1994 Phil Karn <karn@qualcomm.com>
 * Copyright 1996-1998, 2003 William Allen Simpson <wsimpson@greendragon.com>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * File I/O for qsafe and qsieve.
 * 
 * 1996 May     William Allen Simpson extracted from earlier code by Phil Karn,
 * April 1994. save large primes list for later processing. 1998 May
 * William Allen Simpson parameterized. 2003 Jun     William Allen Simpson
 * move common file i/o to own file for better documentation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <openssl/bn.h>
#include "qfile.h"

/*
 * print moduli out in consistent form,
 * normalizing error returns to printf-like expectations.
 */
int
qfileout(FILE * ofile, uint32_t otype, uint32_t otests, uint32_t otries,
	 uint32_t osize, uint32_t ogenerator, BIGNUM * omodulus)
{
	struct tm      *gtm;
	time_t          time_now;

	time(&time_now);
	gtm = gmtime(&time_now);

	if (0 > fprintf(ofile,
			"%04d%02d%02d%02d%02d%02d %u %u %u %u %x ",
			gtm->tm_year + 1900,
			gtm->tm_mon + 1,
			gtm->tm_mday,
			gtm->tm_hour,
			gtm->tm_min,
			gtm->tm_sec,
			otype,
			otests,
			otries,
			osize,
			ogenerator)) {

		return (-1);
	}

	if (1 > BN_print_fp(ofile, omodulus)) {
		return (-1);
	}

	return (fprintf(ofile, "\n"));
}
