/*	$NetBSD: prom_iface.h,v 1.1.8.2 2011/06/06 09:05:21 jruoho Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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

#define PROM_MAGIC 0x30464354

typedef void *(unused_vector)(void);

struct callback {
	unused_vector *vec00;
	unused_vector *vec04;
	unused_vector *vec08;
	unused_vector *vec0c;
	unused_vector *vec10;
	unused_vector *vec14;
	unused_vector *vec18;
	unused_vector *vec1c;
	unused_vector *vec20;
	int	(*_getchar) (void);						/* 24 */
	unused_vector *vec28;
	unused_vector *vec2c;
	void	(*_printf) (const char *, ...);		/* 30 */
	unused_vector *vec34;
	unused_vector *vec38;
	unused_vector *vec3c;
	unused_vector *vec40;
	unused_vector *vec44;
	unused_vector *vec48;
	unused_vector *vec4c;
	unused_vector *vec50;
	unused_vector *vec54;
	unused_vector *vec58;
	unused_vector *vec5c;
	unused_vector *vec60;
	unused_vector *vec64;
	unused_vector *vec68;
	unused_vector *vec6c;
	unused_vector *vec70;
	unused_vector *vec74;
	unused_vector *vec78;
	unused_vector *vec7c;
	int	(*_getsysid) (void);					/* 80 */
	unused_vector *vec84;
	unused_vector *vec88;
	unused_vector *vec8c;
	unused_vector *vec90;
	unused_vector *vec94;
	unused_vector *vec98;
	void	(*_halt) (int *, int);				/* 9c */
};

#define MAKESYSID(_hw_rev_,_fw_rev_,_systype_,_cputype_)         \
        ((_hw_rev_ & 0xff) << 0) | ((_fw_rev_ & 0xff) << 8) |    \
        ((_systype_ & 0xff) << 16) | ((_cputype_ & 0xff) << 24)

/*
 * Mother board type byte of "systype" environment variable.
 */
/* 0 reserved */
#define XS_ML50x        0x1     /* Xilinx ML505/6/7 devboard */
/* 2-7 free */
#define XS_ML40x        0x8     /* Xilinx ML401/2/3 devboard */
#define XS_BE3          0x9     /* BeCube BE3 */
/* b-ff free */


