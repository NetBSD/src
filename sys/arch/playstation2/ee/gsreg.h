/*	$NetBSD: gsreg.h,v 1.4.6.2 2014/08/20 00:03:18 tls Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#define GS_S_PMODE_REG		MIPS_PHYS_TO_KSEG1(0x12000000)
#define GS_S_SMODE1_REG		MIPS_PHYS_TO_KSEG1(0x12000010)
#define GS_S_SMODE2_REG		MIPS_PHYS_TO_KSEG1(0x12000020)
#define GS_S_SRFSH_REG		MIPS_PHYS_TO_KSEG1(0x12000030)
#define GS_S_SYNCH1_REG		MIPS_PHYS_TO_KSEG1(0x12000040)
#define GS_S_SYNCH2_REG		MIPS_PHYS_TO_KSEG1(0x12000050)
#define GS_S_SYNCV_REG		MIPS_PHYS_TO_KSEG1(0x12000060)
#define GS_S_DISPFB1_REG	MIPS_PHYS_TO_KSEG1(0x12000070)
#define GS_S_DISPLAY1_REG	MIPS_PHYS_TO_KSEG1(0x12000080)
#define GS_S_DISPFB2_REG	MIPS_PHYS_TO_KSEG1(0x12000090)
#define GS_S_DISPLAY2_REG	MIPS_PHYS_TO_KSEG1(0x120000a0)
#define GS_S_EXTBUF_REG		MIPS_PHYS_TO_KSEG1(0x120000b0)
#define GS_S_EXTDATA_REG	MIPS_PHYS_TO_KSEG1(0x120000c0)
#define GS_S_EXTWRITE_REG	MIPS_PHYS_TO_KSEG1(0x120000d0)
#define GS_S_BGCOLOR_REG	MIPS_PHYS_TO_KSEG1(0x120000e0)
#define GS_S_CSR_REG		MIPS_PHYS_TO_KSEG1(0x12001000)
#define GS_S_IMR_REG		MIPS_PHYS_TO_KSEG1(0x12001010)
#define GS_S_BUSDIR_REG		MIPS_PHYS_TO_KSEG1(0x12001040)
#define GS_S_SIGLBLID_REG	MIPS_PHYS_TO_KSEG1(0x12001080)

#define SMODE1(vhp, vcksel, slck2, nvck, clksel, pevs, pehs, pvs, phs,	\
    gcont, spml, pck2, xpck, sint, prst, ex, cmod, slck, t1248, lc, rc)	\
	(((u_int64_t)(vhp)	<< 36) |				\
	((u_int64_t)(vcksel)	<< 34) |				\
	((u_int64_t)(slck2)	<< 33) |				\
	((u_int64_t)(nvck)	<< 32) |				\
	((u_int64_t)(clksel)	<< 30) |				\
	((u_int64_t)(pevs)	<< 29) |				\
	((u_int64_t)(pehs)	<< 28) |				\
	((u_int64_t)(pvs)	<< 27) |				\
	((u_int64_t)(phs)	<< 26) |				\
	((u_int64_t)(gcont)	<< 25) |				\
	((u_int64_t)(spml)	<< 21) |				\
	((u_int64_t)(pck2)	<< 19) |				\
	((u_int64_t)(xpck)	<< 18) |				\
	((u_int64_t)(sint)	<< 17) |				\
	((u_int64_t)(prst)	<< 16) |				\
	((u_int64_t)(ex)	<< 15) |				\
	((u_int64_t)(cmod)	<< 13) |				\
	((u_int64_t)(slck)	<< 12) |				\
	((u_int64_t)(t1248)	<< 10) |				\
	((u_int64_t)(lc)	<<  3) |				\
	((u_int64_t)(rc)	<<  0))

#define SMODE2(dpms, ffmd, inter)					\
	(((u_int64_t)(dpms)	<< 2)	|				\
	 ((u_int64_t)(ffmd)	<< 1)	|				\
	 ((u_int64_t)(inter)	<< 0))

#define SRFSH(x)	(x)

#define SYNCH1(hs, hsvs, hseq, hbp, hfp)				\
	(((u_int64_t)(hs)	<< 43) |				\
	((u_int64_t)(hsvs)	<< 32) |				\
	((u_int64_t)(hseq)	<< 22) |				\
	((u_int64_t)(hbp)	<< 11) |				\
	((u_int64_t)(hfp)	<< 0))

#define SYNCH2(hb, hf)							\
	(((u_int64_t)(hb)	<< 11) |				\
	((u_int64_t)(hf)	<< 0))

#define SYNCV(vs, vdp, vbpe, vbp, vfpe, vfp)				\
	(((u_int64_t)(vs)	<< 53) |				\
	((u_int64_t)(vdp)	<< 42) |				\
	((u_int64_t)(vbpe)	<< 32) |				\
	((u_int64_t)(vbp)	<< 20) |				\
	((u_int64_t)(vfpe)	<< 10) |				\
	((u_int64_t)(vfp)	<< 0))

#define DISPLAY(dh, dw, magv, magh, dy, dx)				\
	(((u_int64_t)(dh)	<< 44)	|				\
	 ((u_int64_t)(dw)	<< 32)	|				\
	 ((u_int64_t)(magv)	<< 27)	|				\
	 ((u_int64_t)(magh)	<< 23)	|				\
	 ((u_int64_t)(dy)	<< 12)	|				\
	 ((u_int64_t)(dx)	<<  0))
