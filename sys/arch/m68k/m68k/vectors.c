/*	$NetBSD: vectors.c,v 1.2 2024/01/13 18:42:11 thorpej Exp $	*/

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
 * All rights reserved.         
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.          
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

#ifdef _KERNEL_OPT
#include "opt_compat_netbsd.h"
#include "opt_compat_sunos.h"
#endif 

#include <sys/types.h>

#include <machine/cpu.h>
#include <machine/vectors.h>

#include <m68k/cacheops.h>
#include <m68k/frame.h>

extern char illinst[];
extern char zerodiv[];
extern char chkinst[];
extern char trapvinst[];
extern char privinst[];
extern char trace[];
extern char fpfline[];
extern char badtrap[];
extern char coperr[];
extern char fmterr[];
extern char trap0[];
#if defined(COMPAT_13) || defined(COMPAT_SUNOS)
extern char trap1[];
#define	TRAP1_HANDLER		trap1
#else
#define	TRAP1_HANDLER		illinst
#endif /* COMPAT_13 || COMPAT_SUNOS */
extern char trap2[];
#ifdef COMPAT_16
extern char trap3[];
#define	TRAP3_HANDLER		trap3
#else
#define	TRAP3_HANDLER		illinst
#endif /* COMPAT_16 */
extern char trap12[];
extern char trap15[];
extern char fpfault[];
extern char fpunsupp[];

extern char MACHINE_AV0_HANDLER[];
extern char MACHINE_AV1_HANDLER[];
extern char MACHINE_AV2_HANDLER[];
extern char MACHINE_AV3_HANDLER[];
extern char MACHINE_AV4_HANDLER[];
extern char MACHINE_AV5_HANDLER[];
extern char MACHINE_AV6_HANDLER[];
extern char MACHINE_AV7_HANDLER[];

#ifdef MACHINE_BUSERR_HANDLER
extern char MACHINE_BUSERR_HANDLER[];
#endif

#ifdef MACHINE_ADDRERR_HANDLER
extern char MACHINE_ADDRERR_HANDLER[];
#endif

#ifdef __mc68010__

#define	BUSERR_HANDLER		MACHINE_BUSERR_HANDLER
#define	ADDRERR_HANDLER		MACHINE_ADDRERR_HANDLER

#else

#if defined(M68020) || defined(M68030)
#ifdef MACHINE_BUSERR_HANDLER
#define	BUSERR_HANDLER2030	MACHINE_BUSERR_HANDLER
#else
extern char buserr2030[];
#define	BUSERR_HANDLER2030	buserr2030
#endif
#ifdef MACHINE_ADDRERR_HANDLER
#define	ADDRERR_HANDLER2030	MACHINE_ADDRERR_HANDLER
#else
extern char addrerr2030[];
#define	ADDRERR_HANDLER2030	addrerr2030
#endif
#if defined(M68040) || defined(M68060)
#define	NEED_FIXUP_2030
#else
#define	BUSERR_HANDLER		BUSERR_HANDLER2030
#define	ADDRERR_HANDLER		ADDRERR_HANDLER2030
#endif
#endif /* M68020 || M68030 */

#if defined(M68040)
extern char buserr40[], addrerr4060[];
#ifdef FPSP
extern char bsun[], inex[], dz[], unfl[], operr[], ovfl[], snan[];
#define	FP_BSUN_HANDLER40	bsun
#define	FP_INEX_HANDLER40	inex
#define	FP_DZ_HANDLER40		dz
#define	FP_UNFL_HANDLER40	unfl
#define	FP_OPERR_HANDLER40	operr
#define	FP_OVFL_HANDLER40	ovfl
#define	FP_SNAN_HANDLER40	snan
#else
#define	FP_BSUN_HANDLER40	fpfault
#define	FP_INEX_HANDLER40	fpfault
#define	FP_DZ_HANDLER40		fpfault
#define	FP_UNFL_HANDLER40	fpfault
#define	FP_OPERR_HANDLER40	fpfault
#define	FP_OVFL_HANDLER40	fpfault
#define	FP_SNAN_HANDLER40	fpfault
#endif
#if defined(M68020) || defined(M68030) || defined(M68060)
#define	NEED_FIXUP_40
#else
#define	BUSERR_HANDLER		buserr40
#define	ADDRERR_HANDLER		addrerr4060
#define	FP_BSUN_HANDLER		FP_BSUN_HANDLER40
#define	FP_INEX_HANDLER		FP_INEX_HANDLER40
#define	FP_DZ_HANDLER		FP_DZ_HANDLER40
#define	FP_UNFL_HANDLER		FP_UNFL_HANDLER40
#define	FP_OPERR_HANDLER	FP_OPERR_HANDLER40
#define	FP_OVFL_HANDLER		FP_OVFL_HANDLER40
#define	FP_SNAN_HANDLER		FP_SNAN_HANDLER40
#endif
#endif /* M68040 */

#if defined(M68060)
extern char buserr60[], addrerr4060[];
#ifdef M060SP
extern char FP_CALL_TOP[], I_CALL_TOP[];
#define	LINE111_HANDLER60	&FP_CALL_TOP[128 + 0x30]
#define	FP_INEX_HANDLER60	&FP_CALL_TOP[128 + 0x28]
#define	FP_DZ_HANDLER60		&FP_CALL_TOP[128 + 0x20]
#define	FP_UNFL_HANDLER60	&FP_CALL_TOP[128 + 0x18]
#define	FP_OPERR_HANDLER60	&FP_CALL_TOP[128 + 0x08]
#define	FP_OVFL_HANDLER60	&FP_CALL_TOP[128 + 0x10]
#define	FP_SNAN_HANDLER60	&FP_CALL_TOP[128 + 0x00]
#define	UNIMP_FP_DATA_HANDLER60	&FP_CALL_TOP[128 + 0x38]
#define	UNIMP_EA_HANDLER60	&FP_CALL_TOP[128 + 0x40]
#define	UNIMP_II_HANDLER60	&I_CALL_TOP[128 + 0x00]
#else
#define	LINE111_HANDLER60	fpfline
#define	FP_INEX_HANDLER60	fpfault
#define	FP_DZ_HANDLER60		fpfault
#define	FP_UNFL_HANDLER60	fpfault
#define	FP_OPERR_HANDLER60	fpfault
#define	FP_OVFL_HANDLER60	fpfault
#define	FP_SNAN_HANDLER60	fpfault
#define	UNIMP_FP_DATA_HANDLER60	fpunsupp
#define	UNIMP_EA_HANDLER60	badtrap		/* XXX illinst? */
#define	UNIMP_II_HANDLER60	illinst
#endif
#if defined(M68020) || defined(M68030) || defined(M68040)
#define	NEED_FIXUP_60
#else
#define	BUSERR_HANDLER		buserr60
#define	ADDRERR_HANDLER		addrerr4060
#define	LINE111_HANDLER		LINE111_HANDLER60
#define	FP_INEX_HANDLER		FP_INEX_HANDLER60
#define	FP_DZ_HANDLER		FP_DZ_HANDLER60
#define	FP_UNFL_HANDLER		FP_UNFL_HANDLER60
#define	FP_OPERR_HANDLER	FP_OPERR_HANDLER60
#define	FP_OVFL_HANDLER		FP_OVFL_HANDLER60
#define	FP_SNAN_HANDLER		FP_SNAN_HANDLER60
#define	UNIMP_FP_DATA_HANDLER	UNIMP_FP_DATA_HANDLER60
#define	UNIMP_EA_HANDLER	UNIMP_EA_HANDLER60
#define	UNIMP_II_HANDLER	UNIMP_II_HANDLER60
#endif
#endif /* M68060 */

#endif /* __mc68010__ */

#ifndef MACHINE_RESET_SP
#define	MACHINE_RESET_SP	NULL
#endif

#ifndef MACHINE_RESET_PC
#define	MACHINE_RESET_PC	NULL
#endif

#ifndef BUSERR_HANDLER
#define	BUSERR_HANDLER		badtrap
#endif

#ifndef ADDRERR_HANDLER
#define	ADDRERR_HANDLER		badtrap
#endif

#ifndef LINE1111_HANDLER
#define	LINE1111_HANDLER	fpfline
#endif

#ifndef FP_BSUN_HANDLER
#define	FP_BSUN_HANDLER		fpfault
#endif

#ifndef	FP_INEX_HANDLER
#define	FP_INEX_HANDLER		fpfault
#endif

#ifndef FP_DZ_HANDLER
#define	FP_DZ_HANDLER		fpfault
#endif

#ifndef FP_UNFL_HANDLER
#define	FP_UNFL_HANDLER		fpfault
#endif

#ifndef FP_OPERR_HANDLER
#define	FP_OPERR_HANDLER	fpfault
#endif

#ifndef FP_OVFL_HANDLER
#define	FP_OVFL_HANDLER		fpfault
#endif

#ifndef FP_SNAN_HANDLER
#define	FP_SNAN_HANDLER		fpfault
#endif

#ifndef UNIMP_FP_DATA_HANDLER
#define	UNIMP_FP_DATA_HANDLER	fpunsupp
#endif

#ifndef UNIMP_EA_HANDLER
#define	UNIMP_EA_HANDLER	badtrap
#endif

#ifndef UNIMP_II_HANDLER
#define	UNIMP_II_HANDLER	badtrap
#endif

void *vectab[NVECTORS] = {
	[VECI_RIISP]		=	MACHINE_RESET_SP,
	[VECI_RIPC]		=	MACHINE_RESET_PC,
	[VECI_BUSERR]		=	BUSERR_HANDLER,
	[VECI_ADDRERR]		=	ADDRERR_HANDLER,
	[VECI_ILLINST]		=	illinst,
	[VECI_ZERODIV]		=	zerodiv,
	[VECI_CHK]		=	chkinst,
	[VECI_TRAPcc]		=	trapvinst,
	[VECI_PRIV]		=	privinst,
	[VECI_TRACE]		=	trace,
	[VECI_LINE1010]		=	illinst,
	[VECI_LINE1111]		=	LINE1111_HANDLER,
	[VECI_rsvd12]		=	badtrap,
	[VECI_CPV]		=	coperr,
	[VECI_FORMATERR]	=	fmterr,
	[VECI_UNINT_INTR]	=	badtrap,
	[VECI_rsvd16]		=	badtrap,
	[VECI_rsvd17]		=	badtrap,
	[VECI_rsvd18]		=	badtrap,
	[VECI_rsvd19]		=	badtrap,
	[VECI_rsvd20]		=	badtrap,
	[VECI_rsvd21]		=	badtrap,
	[VECI_rsvd22]		=	badtrap,
	[VECI_rsvd23]		=	badtrap,
	[VECI_INTRAV0]		=	MACHINE_AV0_HANDLER,
	[VECI_INTRAV1]		=	MACHINE_AV1_HANDLER,
	[VECI_INTRAV2]		=	MACHINE_AV2_HANDLER,
	[VECI_INTRAV3]		=	MACHINE_AV3_HANDLER,
	[VECI_INTRAV4]		=	MACHINE_AV4_HANDLER,
	[VECI_INTRAV5]		=	MACHINE_AV5_HANDLER,
	[VECI_INTRAV6]		=	MACHINE_AV6_HANDLER,
	[VECI_INTRAV7]		=	MACHINE_AV7_HANDLER,
	[VECI_TRAP0]		=	trap0,		/* syscalls */
	[VECI_TRAP1]		=	TRAP1_HANDLER,	/* 1.3 sigreturn */
	[VECI_TRAP2]		=	trap2,		/* trace */
	[VECI_TRAP3]		=	TRAP3_HANDLER,	/* 1.6 sigreturn */
	[VECI_TRAP4]		=	illinst,
	[VECI_TRAP5]		=	illinst,
	[VECI_TRAP6]		=	illinst,
	[VECI_TRAP7]		=	illinst,
	[VECI_TRAP8]		=	illinst,
	[VECI_TRAP9]		=	illinst,
	[VECI_TRAP10]		=	illinst,
	[VECI_TRAP11]		=	illinst,
	[VECI_TRAP12]		=	trap12,
	[VECI_TRAP13]		=	illinst,
	[VECI_TRAP14]		=	illinst,
	[VECI_TRAP15]		=	trap15,
	[VECI_FP_BSUN]		=	FP_BSUN_HANDLER,
	[VECI_FP_INEX]		=	FP_INEX_HANDLER,
	[VECI_FP_DZ]		=	FP_DZ_HANDLER,
	[VECI_FP_UNFL]		=	FP_UNFL_HANDLER,
	[VECI_FP_OPERR]		=	FP_OPERR_HANDLER,
	[VECI_FP_OVFL]		=	FP_OVFL_HANDLER,
	[VECI_FP_SNAN]		=	FP_SNAN_HANDLER,
	[VECI_UNIMP_FP_DATA]	=	UNIMP_FP_DATA_HANDLER,
	[VECI_PMMU_CONF]	=	badtrap,
	[VECI_PMMU_ILLOP]	=	badtrap,
	[VECI_PMMU_ACCESS]	=	badtrap,
	[VECI_rsvd59]		=	badtrap,
	[VECI_UNIMP_EA]		=	UNIMP_EA_HANDLER,
	[VECI_UNIMP_II]		=	UNIMP_II_HANDLER,
	[VECI_rsvd62]		=	badtrap,
	[VECI_rsvd63]		=	badtrap,
	[VECI_USRVEC_START+0]	=	badtrap,
	[VECI_USRVEC_START+1]	=	badtrap,
	[VECI_USRVEC_START+2]	=	badtrap,
	[VECI_USRVEC_START+3]	=	badtrap,
	[VECI_USRVEC_START+4]	=	badtrap,
	[VECI_USRVEC_START+5]	=	badtrap,
	[VECI_USRVEC_START+6]	=	badtrap,
	[VECI_USRVEC_START+7]	=	badtrap,
	[VECI_USRVEC_START+8]	=	badtrap,
	[VECI_USRVEC_START+9]	=	badtrap,
	[VECI_USRVEC_START+10]	=	badtrap,
	[VECI_USRVEC_START+11]	=	badtrap,
	[VECI_USRVEC_START+12]	=	badtrap,
	[VECI_USRVEC_START+13]	=	badtrap,
	[VECI_USRVEC_START+14]	=	badtrap,
	[VECI_USRVEC_START+15]	=	badtrap,
	[VECI_USRVEC_START+16]	=	badtrap,
	[VECI_USRVEC_START+17]	=	badtrap,
	[VECI_USRVEC_START+18]	=	badtrap,
	[VECI_USRVEC_START+19]	=	badtrap,
	[VECI_USRVEC_START+20]	=	badtrap,
	[VECI_USRVEC_START+21]	=	badtrap,
	[VECI_USRVEC_START+22]	=	badtrap,
	[VECI_USRVEC_START+23]	=	badtrap,
	[VECI_USRVEC_START+24]	=	badtrap,
	[VECI_USRVEC_START+25]	=	badtrap,
	[VECI_USRVEC_START+26]	=	badtrap,
	[VECI_USRVEC_START+27]	=	badtrap,
	[VECI_USRVEC_START+28]	=	badtrap,
	[VECI_USRVEC_START+29]	=	badtrap,
	[VECI_USRVEC_START+30]	=	badtrap,
	[VECI_USRVEC_START+31]	=	badtrap,
	[VECI_USRVEC_START+32]	=	badtrap,
	[VECI_USRVEC_START+33]	=	badtrap,
	[VECI_USRVEC_START+34]	=	badtrap,
	[VECI_USRVEC_START+35]	=	badtrap,
	[VECI_USRVEC_START+36]	=	badtrap,
	[VECI_USRVEC_START+37]	=	badtrap,
	[VECI_USRVEC_START+38]	=	badtrap,
	[VECI_USRVEC_START+39]	=	badtrap,
	[VECI_USRVEC_START+40]	=	badtrap,
	[VECI_USRVEC_START+41]	=	badtrap,
	[VECI_USRVEC_START+42]	=	badtrap,
	[VECI_USRVEC_START+43]	=	badtrap,
	[VECI_USRVEC_START+44]	=	badtrap,
	[VECI_USRVEC_START+45]	=	badtrap,
	[VECI_USRVEC_START+46]	=	badtrap,
	[VECI_USRVEC_START+47]	=	badtrap,
	[VECI_USRVEC_START+48]	=	badtrap,
	[VECI_USRVEC_START+49]	=	badtrap,
	[VECI_USRVEC_START+50]	=	badtrap,
	[VECI_USRVEC_START+51]	=	badtrap,
	[VECI_USRVEC_START+52]	=	badtrap,
	[VECI_USRVEC_START+53]	=	badtrap,
	[VECI_USRVEC_START+54]	=	badtrap,
	[VECI_USRVEC_START+55]	=	badtrap,
	[VECI_USRVEC_START+56]	=	badtrap,
	[VECI_USRVEC_START+57]	=	badtrap,
	[VECI_USRVEC_START+58]	=	badtrap,
	[VECI_USRVEC_START+59]	=	badtrap,
	[VECI_USRVEC_START+60]	=	badtrap,
	[VECI_USRVEC_START+61]	=	badtrap,
	[VECI_USRVEC_START+62]	=	badtrap,
	[VECI_USRVEC_START+63]	=	badtrap,
	[VECI_USRVEC_START+64]	=	badtrap,
	[VECI_USRVEC_START+65]	=	badtrap,
	[VECI_USRVEC_START+66]	=	badtrap,
	[VECI_USRVEC_START+67]	=	badtrap,
	[VECI_USRVEC_START+68]	=	badtrap,
	[VECI_USRVEC_START+69]	=	badtrap,
	[VECI_USRVEC_START+70]	=	badtrap,
	[VECI_USRVEC_START+71]	=	badtrap,
	[VECI_USRVEC_START+72]	=	badtrap,
	[VECI_USRVEC_START+73]	=	badtrap,
	[VECI_USRVEC_START+74]	=	badtrap,
	[VECI_USRVEC_START+75]	=	badtrap,
	[VECI_USRVEC_START+76]	=	badtrap,
	[VECI_USRVEC_START+77]	=	badtrap,
	[VECI_USRVEC_START+78]	=	badtrap,
	[VECI_USRVEC_START+79]	=	badtrap,
	[VECI_USRVEC_START+80]	=	badtrap,
	[VECI_USRVEC_START+81]	=	badtrap,
	[VECI_USRVEC_START+82]	=	badtrap,
	[VECI_USRVEC_START+83]	=	badtrap,
	[VECI_USRVEC_START+84]	=	badtrap,
	[VECI_USRVEC_START+85]	=	badtrap,
	[VECI_USRVEC_START+86]	=	badtrap,
	[VECI_USRVEC_START+87]	=	badtrap,
	[VECI_USRVEC_START+88]	=	badtrap,
	[VECI_USRVEC_START+89]	=	badtrap,
	[VECI_USRVEC_START+90]	=	badtrap,
	[VECI_USRVEC_START+91]	=	badtrap,
	[VECI_USRVEC_START+92]	=	badtrap,
	[VECI_USRVEC_START+93]	=	badtrap,
	[VECI_USRVEC_START+94]	=	badtrap,
	[VECI_USRVEC_START+95]	=	badtrap,
	[VECI_USRVEC_START+96]	=	badtrap,
	[VECI_USRVEC_START+97]	=	badtrap,
	[VECI_USRVEC_START+98]	=	badtrap,
	[VECI_USRVEC_START+99]	=	badtrap,
	[VECI_USRVEC_START+100]	=	badtrap,
	[VECI_USRVEC_START+101]	=	badtrap,
	[VECI_USRVEC_START+102]	=	badtrap,
	[VECI_USRVEC_START+103]	=	badtrap,
	[VECI_USRVEC_START+104]	=	badtrap,
	[VECI_USRVEC_START+105]	=	badtrap,
	[VECI_USRVEC_START+106]	=	badtrap,
	[VECI_USRVEC_START+107]	=	badtrap,
	[VECI_USRVEC_START+108]	=	badtrap,
	[VECI_USRVEC_START+109]	=	badtrap,
	[VECI_USRVEC_START+110]	=	badtrap,
	[VECI_USRVEC_START+111]	=	badtrap,
	[VECI_USRVEC_START+112]	=	badtrap,
	[VECI_USRVEC_START+113]	=	badtrap,
	[VECI_USRVEC_START+114]	=	badtrap,
	[VECI_USRVEC_START+115]	=	badtrap,
	[VECI_USRVEC_START+116]	=	badtrap,
	[VECI_USRVEC_START+117]	=	badtrap,
	[VECI_USRVEC_START+118]	=	badtrap,
	[VECI_USRVEC_START+119]	=	badtrap,
	[VECI_USRVEC_START+120]	=	badtrap,
	[VECI_USRVEC_START+121]	=	badtrap,
	[VECI_USRVEC_START+122]	=	badtrap,
	[VECI_USRVEC_START+123]	=	badtrap,
	[VECI_USRVEC_START+124]	=	badtrap,
	[VECI_USRVEC_START+125]	=	badtrap,
	[VECI_USRVEC_START+126]	=	badtrap,
	[VECI_USRVEC_START+127]	=	badtrap,
	[VECI_USRVEC_START+128]	=	badtrap,
	[VECI_USRVEC_START+129]	=	badtrap,
	[VECI_USRVEC_START+130]	=	badtrap,
	[VECI_USRVEC_START+131]	=	badtrap,
	[VECI_USRVEC_START+132]	=	badtrap,
	[VECI_USRVEC_START+133]	=	badtrap,
	[VECI_USRVEC_START+134]	=	badtrap,
	[VECI_USRVEC_START+135]	=	badtrap,
	[VECI_USRVEC_START+136]	=	badtrap,
	[VECI_USRVEC_START+137]	=	badtrap,
	[VECI_USRVEC_START+138]	=	badtrap,
	[VECI_USRVEC_START+139]	=	badtrap,
	[VECI_USRVEC_START+140]	=	badtrap,
	[VECI_USRVEC_START+141]	=	badtrap,
	[VECI_USRVEC_START+142]	=	badtrap,
	[VECI_USRVEC_START+143]	=	badtrap,
	[VECI_USRVEC_START+144]	=	badtrap,
	[VECI_USRVEC_START+145]	=	badtrap,
	[VECI_USRVEC_START+146]	=	badtrap,
	[VECI_USRVEC_START+147]	=	badtrap,
	[VECI_USRVEC_START+148]	=	badtrap,
	[VECI_USRVEC_START+149]	=	badtrap,
	[VECI_USRVEC_START+150]	=	badtrap,
	[VECI_USRVEC_START+151]	=	badtrap,
	[VECI_USRVEC_START+152]	=	badtrap,
	[VECI_USRVEC_START+153]	=	badtrap,
	[VECI_USRVEC_START+154]	=	badtrap,
	[VECI_USRVEC_START+155]	=	badtrap,
	[VECI_USRVEC_START+156]	=	badtrap,
	[VECI_USRVEC_START+157]	=	badtrap,
	[VECI_USRVEC_START+158]	=	badtrap,
	[VECI_USRVEC_START+159]	=	badtrap,
	[VECI_USRVEC_START+160]	=	badtrap,
	[VECI_USRVEC_START+161]	=	badtrap,
	[VECI_USRVEC_START+162]	=	badtrap,
	[VECI_USRVEC_START+163]	=	badtrap,
	[VECI_USRVEC_START+164]	=	badtrap,
	[VECI_USRVEC_START+165]	=	badtrap,
	[VECI_USRVEC_START+166]	=	badtrap,
	[VECI_USRVEC_START+167]	=	badtrap,
	[VECI_USRVEC_START+168]	=	badtrap,
	[VECI_USRVEC_START+169]	=	badtrap,
	[VECI_USRVEC_START+170]	=	badtrap,
	[VECI_USRVEC_START+171]	=	badtrap,
	[VECI_USRVEC_START+172]	=	badtrap,
	[VECI_USRVEC_START+173]	=	badtrap,
	[VECI_USRVEC_START+174]	=	badtrap,
	[VECI_USRVEC_START+175]	=	badtrap,
	[VECI_USRVEC_START+176]	=	badtrap,
	[VECI_USRVEC_START+177]	=	badtrap,
	[VECI_USRVEC_START+178]	=	badtrap,
	[VECI_USRVEC_START+179]	=	badtrap,
	[VECI_USRVEC_START+180]	=	badtrap,
	[VECI_USRVEC_START+181]	=	badtrap,
	[VECI_USRVEC_START+182]	=	badtrap,
	[VECI_USRVEC_START+183]	=	badtrap,
	[VECI_USRVEC_START+184]	=	badtrap,
	[VECI_USRVEC_START+185]	=	badtrap,
	[VECI_USRVEC_START+186]	=	badtrap,
	[VECI_USRVEC_START+187]	=	badtrap,
	[VECI_USRVEC_START+188]	=	badtrap,
	[VECI_USRVEC_START+189]	=	badtrap,
	[VECI_USRVEC_START+190]	=	badtrap,
	[VECI_USRVEC_START+191]	=	badtrap,
};

static void **saved_vbr;

/*
 * vec_init --
 *	Initialize the vector table and Vector Base Register.
 *
 *	N.B. We rely on the cputype variable having been initialized
 *	and the MMU having been enabled.
 */
void
vec_init(void)
{
#if !defined(__mc68010__) && \
    (defined(NEED_FIXUP_2030) || defined(NEED_FIXUP_40) || \
     defined(NEED_FIXUP_60))
	switch (cputype) {
#ifdef NEED_FIXUP_2030
	case CPU_68020:
	case CPU_68030:
		vectab[VECI_BUSERR]        = BUSERR_HANDLER2030;
		vectab[VECI_ADDRERR]       = ADDRERR_HANDLER2030;
		break;
#endif /* NEED_FIXUP_2030 */

#ifdef NEED_FIXUP_40
	case CPU_68040:
		vectab[VECI_BUSERR]        = buserr40;
		vectab[VECI_ADDRERR]       = addrerr4060;
		vectab[VECI_FP_BSUN]       = FP_BSUN_HANDLER40;
		vectab[VECI_FP_INEX]       = FP_INEX_HANDLER40;
		vectab[VECI_FP_DZ]         = FP_DZ_HANDLER40;
		vectab[VECI_FP_UNFL]       = FP_UNFL_HANDLER40;
		vectab[VECI_FP_OPERR]      = FP_OPERR_HANDLER40;
		vectab[VECI_FP_OVFL]       = FP_OVFL_HANDLER40;
		vectab[VECI_FP_SNAN]       = FP_SNAN_HANDLER40;
		DCIS();
		break;
#endif /* NEED_FIXUP_40 */

#ifdef NEED_FIXUP_60
	case CPU_68060:
		vectab[VECI_BUSERR]        = buserr60;
		vectab[VECI_ADDRERR]       = addrerr4060;
		vectab[VECI_FP_INEX]       = FP_INEX_HANDLER60;
		vectab[VECI_FP_DZ]         = FP_DZ_HANDLER60;
		vectab[VECI_FP_UNFL]       = FP_UNFL_HANDLER60;
		vectab[VECI_FP_OPERR]      = FP_OPERR_HANDLER60;
		vectab[VECI_FP_OVFL]       = FP_OVFL_HANDLER60;
		vectab[VECI_FP_SNAN]       = FP_SNAN_HANDLER60;
		vectab[VECI_UNIMP_FP_DATA] = UNIMP_FP_DATA_HANDLER60;
		vectab[VECI_UNIMP_EA]      = UNIMP_EA_HANDLER60;
		vectab[VECI_UNIMP_II]      = UNIMP_II_HANDLER60;
		DCIS();
		break;
#endif /* NEED_FIXUP_60 */
	default:
		panic("vec_init");
		break;
	}
#endif
	saved_vbr = getvbr();
	setvbr(vectab);
}

/*
 * vec_reset --
 *	Reset the Vector Base Register.
 */
void
vec_reset(void)
{
	setvbr(saved_vbr);
}

/*
 * vec_get_entry --
 *	Get a vector table entry.
 */
void *
vec_get_entry(int vec)
{
	KASSERT(vec >= 0);
	KASSERT(vec < NVECTORS);
	return vectab[vec];
}

/*
 * vec_set_entry --
 *	Set a vector table entry.
 */
void
vec_set_entry(int vec, void *addr)
{
	KASSERT(vec >= 0);
	KASSERT(vec < NVECTORS);
	vectab[vec] = addr;
#if defined(M68040) || defined(M68060)
	if (cputype == CPU_68040 || cputype == CPU_68060) {
		DCIS();
	}
#endif
}
