/*	$NetBSD: devreg.c,v 1.1 2002/02/28 01:56:58 uch Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sh3/cache_sh3.h>
#include <sh3/cache_sh4.h>
#include <sh3/mmu_sh3.h>
#include <sh3/mmu_sh4.h>
#include <sh3/trapreg.h>
#include <sh3/ubcreg.h>
#include <sh3/rtcreg.h>
#include <sh3/tmureg.h>

/* MMU */
u_int32_t __sh_PTEH;
u_int32_t __sh_TTB;
u_int32_t __sh_TEA;
u_int32_t __sh_TRA;
u_int32_t __sh_EXPEVT;
u_int32_t __sh_INTEVT;

/* UBC */
u_int32_t __sh_BARA;
u_int32_t __sh_BAMRA;
u_int32_t __sh_BASRA;
u_int32_t __sh_BBRA;
u_int32_t __sh_BARB;
u_int32_t __sh_BAMRB;
u_int32_t __sh_BASRB;
u_int32_t __sh_BBRB;
u_int32_t __sh_BDRB;
u_int32_t __sh_BDMRB;
u_int32_t __sh_BRCR;

/* RTC */
u_int32_t __sh_R64CNT;
u_int32_t __sh_RSECCNT;
u_int32_t __sh_RMINCNT;
u_int32_t __sh_RHRCNT;
u_int32_t __sh_RWKCNT;
u_int32_t __sh_RDAYCNT;
u_int32_t __sh_RMONCNT;
u_int32_t __sh_RYRCNT;
u_int32_t __sh_RSECAR;
u_int32_t __sh_RMINAR;
u_int32_t __sh_RHRAR;
u_int32_t __sh_RWKAR;
u_int32_t __sh_RDAYAR;
u_int32_t __sh_RMONAR;
u_int32_t __sh_RCR1;
u_int32_t __sh_RCR2;

/* TMU */
u_int32_t __sh_TOCR;
u_int32_t __sh_TSTR;
u_int32_t __sh_TCOR0;
u_int32_t __sh_TCNT0;
u_int32_t __sh_TCR0;
u_int32_t __sh_TCOR1;
u_int32_t __sh_TCNT1;
u_int32_t __sh_TCR1;
u_int32_t __sh_TCOR2;
u_int32_t __sh_TCNT2;
u_int32_t __sh_TCR2;
u_int32_t __sh_TCPR2;

#define SH3REG(x)	__sh_ ## x = SH3_ ## x
#define SH4REG(x)	__sh_ ## x = SH4_ ## x

#define SHREG(x)							\
do {									\
/* Exception */								\
SH ## x ## REG(TRA);							\
SH ## x ## REG(EXPEVT);							\
SH ## x ## REG(INTEVT);							\
/* UBC */								\
SH ## x ## REG(BARA);							\
SH ## x ## REG(BAMRA);							\
SH ## x ## REG(BASRA);							\
SH ## x ## REG(BBRA);							\
SH ## x ## REG(BARB);							\
SH ## x ## REG(BAMRB);							\
SH ## x ## REG(BASRB);							\
SH ## x ## REG(BBRB);							\
SH ## x ## REG(BDRB);							\
SH ## x ## REG(BDMRB);							\
SH ## x ## REG(BRCR);							\
/* MMU */								\
SH ## x ## REG(PTEH);							\
SH ## x ## REG(TEA);							\
SH ## x ## REG(TTB);							\
/* RTC */								\
SH ## x ## REG(R64CNT);							\
SH ## x ## REG(RSECCNT);						\
SH ## x ## REG(RMINCNT);						\
SH ## x ## REG(RHRCNT);							\
SH ## x ## REG(RWKCNT);							\
SH ## x ## REG(RDAYCNT);						\
SH ## x ## REG(RMONCNT);						\
SH ## x ## REG(RYRCNT);							\
SH ## x ## REG(RSECAR);							\
SH ## x ## REG(RMINAR);							\
SH ## x ## REG(RHRAR);							\
SH ## x ## REG(RWKAR);							\
SH ## x ## REG(RDAYAR);							\
SH ## x ## REG(RMONAR);							\
SH ## x ## REG(RCR1);							\
SH ## x ## REG(RCR2);							\
/* TMU */								\
SH ## x ## REG(TOCR);							\
SH ## x ## REG(TSTR);							\
SH ## x ## REG(TCOR0);							\
SH ## x ## REG(TCNT0);							\
SH ## x ## REG(TCR0);							\
SH ## x ## REG(TCOR1);							\
SH ## x ## REG(TCNT1);							\
SH ## x ## REG(TCR1);							\
SH ## x ## REG(TCOR2);							\
SH ## x ## REG(TCNT2);							\
SH ## x ## REG(TCR2);							\
SH ## x ## REG(TCPR2);							\
} while (/*CONSTCOND*/0)

void
sh_devreg_init()
{

	if (CPU_IS_SH3)
		SHREG(3);
	
	if (CPU_IS_SH4)
		SHREG(4);

}
