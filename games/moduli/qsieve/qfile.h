/* $NetBSD: qfile.h,v 1.1 2006/01/19 23:23:58 elad Exp $ */

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

#ifndef _QFILE_H_
#define _QFILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <openssl/bn.h>

/* need line long enough for largest moduli plus headers */
#define QLINESIZE               (100+8192)

/*
 * Type: decimal. Specifies the internal structure of the prime modulus.
 */
#define QTYPE_UNKNOWN           (0)
#define QTYPE_UNSTRUCTURED      (1)
#define QTYPE_SAFE              (2)
#define QTYPE_SCHNOOR           (3)
#define QTYPE_SOPHIE_GERMAINE   (4)
#define QTYPE_STRONG            (5)

/*
 * Tests: decimal (bit field). Specifies the methods used in checking for
 * primality. Usually, more than one test is used.
 */
#define QTEST_UNTESTED          (0x00)
#define QTEST_COMPOSITE         (0x01)
#define QTEST_SIEVE             (0x02)
#define QTEST_MILLER_RABIN      (0x04)
#define QTEST_JACOBI            (0x08)
#define QTEST_ELLIPTIC          (0x10)

/*
 * Size: decimal. Specifies the number of the most significant bit (0 to M). *
 * WARNING: internally, usually 1 to N.
 */
#define QSIZE_MINIMUM           (511)


int 
qfileout(FILE *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
	 BIGNUM *);

#endif /* !_QFILE_H_ */
