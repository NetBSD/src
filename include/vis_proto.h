/*-
 * Copyright (c) 2025 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nia Alarie.
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

/*
 * This should be compatible with what was shipped with SunPro.
 * 
 * VIS Instruction Set User's Manual
 * Sun Microsystems
 * Part Number: 805-1394-03
 * May 2001
 *
 * Version of available VIS instructions can be detected through
 * the `machdep.vis` sysctl. A value of "0" means that such
 * instructions are unavailable. All SPARCv9 hardware should support
 * at least VIS 1, while VIS 2 requires UltraSPARC-III or newer.
 *
 * GCC needs -mvis for VIS, and -mvis2 for VIS 2. However, its
 * builtins are incomplete and some cause problematic typing issues
 * with Sun's API, so they're mostly avoided.
 */

#ifndef _VIS_PROTO_H
#define _VIS_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vis_types.h"

#define _VISATTR \
	__attribute__((__gnu_inline__, __always_inline__, __artificial__))

/* 4.6.1 Arithmetic - addition and subtraction */

_VISATTR
static __inline vis_d64
vis_fpadd16(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;

	__asm("fpadd16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fpsub16(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;

	__asm("fpsub16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fpadd32(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;

	__asm("fpadd32 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fpsub32(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;

	__asm("fpsub32 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fpadd16s(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;

	__asm("fpadd16s %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fpsub16s(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;

	__asm("fpsub16s %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fpadd32s(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;

	__asm("fpadd32s %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fpsub32s(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;

	__asm("fpsub32s %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

/* 4.7 Pixel formatting - packing */

_VISATTR
static __inline vis_f32
vis_fpack16(vis_d64 r1)
{
	vis_f32 out;

	__asm("fpack16 %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fpack32(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;

	__asm("fpack32 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fpackfix(vis_d64 r1)
{
	vis_f32 out;

	__asm("fpackfix %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fexpand(vis_f32 r1)
{
	vis_d64 out;

	__asm("fexpand %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fpmerge(vis_f32 r1, vis_f32 r2)
{
	vis_d64 out;

	__asm("fpmerge %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

/* 4.7.6 Aligned address calculation */

_VISATTR
static __inline void *
vis_alignaddr(void *addr, int offset)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_alignaddr(addr, offset);
#else
	void *out;

	__asm("alginaddr %1,%2,%0"
	    : "=r"(out)
	    : "r"(addr), "r"(offset));
	return out;
#endif
}

_VISATTR
static __inline vis_d64
vis_faligndata(vis_d64 hi, vis_d64 lo)
{
	vis_d64 out;

	__asm("faligndata %1,%2,%0"
	    : "=f"(out)
	    : "f"(hi), "f"(lo));
	return out;
}

/* 4.7.7 Edge handling */

_VISATTR
static __inline vis_s32
vis_edge8(void *a1, void *a2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_edge8(a1, a2);
#else
	vis_s32 out;

	__asm("edge8 %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "f"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge16(void *a1, void *a2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_edge16(a1, a2);
#else
	vis_s32 out;

	__asm("edge16 %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "f"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge32(void *a1, void *a2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_edge32(a1, a2);
#else
	vis_s32 out;

	__asm("edge32 %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "f"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge8l(void *a1, void *a2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_edge8l(a1, a2);
#else
	vis_s32 out;

	__asm("edge8l %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "f"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge16l(void *a1, void *a2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_edge16l(a1, a2);
#else
	vis_s32 out;

	__asm("edge16l %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "f"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge32l(void *a1, void *a2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_edge32l(a1, a2);
#else
	vis_s32 out;

	__asm("edge32l %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "f"(a2));
	return out;
#endif
}

/* 4.9 Array coordinate translation */

_VISATTR
static __inline vis_addr
_VISATTR
vis_array8(vis_u64 d1, vis_s32 d2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_array8(d1, d2);
#else
	vis_addr out;

	__asm("array8 %1,%2,%0"
	    : "=r"(out)
	    : "r"(d1), "f"(d2));
	return out;
#endif
}

_VISATTR
static __inline vis_addr
vis_array16(vis_u64 d1, vis_s32 d2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_array16(d1, d2);
#else
	vis_addr out;

	__asm("array16 %1,%2,%0"
	    : "=r"(out)
	    : "r"(d1), "f"(d2));
	return out;
#endif
}

_VISATTR
static __inline vis_addr
vis_array32(vis_u64 d1, vis_s32 d2)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_array32(d1, d2);
#else
	vis_addr out;

	__asm("array32 %1,%2,%0"
	    : "=r"(out)
	    : "r"(d1), "r"(d2));
	return out;
#endif
}

/* 4.3.1 Graphics Status Register manipulation */

_VISATTR
static __inline vis_u64
vis_read_gsr64(void)
{
#if defined(__VIS__) && defined(__GNUC__)
	return __builtin_vis_read_gsr();
#else
	vis_u64 out;

	__asm("rd %%gsr,%0"
	    : "=r"(out));
	return out;
#endif
}

_VISATTR
static __inline void
vis_write_gsr64(vis_u64 gsr)
{
#if defined(__VIS__) && defined(__GNUC__)
	__builtin_vis_write_gsr(gsr);
#else
	__asm("mov %0,%%gsr"
	    :
	    : "r"(gsr));
#endif
}

_VISATTR
static __inline vis_u32
vis_read_gsr32(void)
{
	return vis_read_gsr64();
}

_VISATTR
static __inline void
vis_write_gsr32(vis_u32 gsr)
{
	vis_write_gsr64(gsr);
}

/* 4.3.2 Read and write to upper/lower components */

_VISATTR
static __inline vis_f32
vis_read_hi(vis_d64 var)
{
	vis_u64 reg = *((vis_u64 *)&var);
	vis_u32 hi = (reg >> 32) & 0xffffffff;
	vis_f32 out = *((vis_f32 *)&hi);
	return out;
}

_VISATTR
static __inline vis_f32
vis_read_lo(vis_d64 var)
{
	vis_u64 reg = *((vis_u64 *)&var);
	vis_u32 lo = reg & 0xffffffff;
	vis_f32 out = *((vis_f32 *)&lo);
	return out;
}

_VISATTR
static __inline vis_d64
vis_write_lo(vis_d64 in, vis_f32 lower)
{
	vis_u64 out = *((vis_u64 *)&in);
	vis_u32 hi = (out >> 32) & 0xffffffff;
	vis_u32 lo = *((vis_u32 *)&lower);

	out = ((vis_u64)hi << 32ULL) | lo;
	return *((vis_d64 *)&out);
}

_VISATTR
static __inline vis_d64
vis_write_hi(vis_d64 in, vis_f32 upper)
{
	vis_u64 out = *((vis_u64 *)&in);
	vis_u32 hi = *((vis_u32 *)&upper);
	vis_u32 lo = out & 0xffffffff;

	out = ((vis_u64)hi << 32ULL) | lo;
	return *((vis_d64 *)&out);
}

/* 4.3.3 Join two variables into a single */

_VISATTR
static __inline vis_d64
vis_freg_pair(vis_f32 f1, vis_f32 f2)
{
	vis_u64 out;
	vis_u32 r1 = *((vis_u32 *)&f1);
	vis_u32 r2 = *((vis_u32 *)&f2);

	out = ((vis_u64)r1 << 32ULL) | r2;
	return *((vis_d64 *)&out);
}

/* 4.3.4 Place ints into FP register */

_VISATTR
static __inline vis_f32
vis_to_float(vis_u32 data)
{
	return *((vis_f32 *)&data);
}

_VISATTR
static __inline vis_d64
vis_to_double(vis_u32 d1, vis_u32 d2)
{
	vis_u64 out;

	out = ((vis_u64)d1 << 32ULL) | d2;
	return *((vis_d64 *)&out);
}

_VISATTR
static __inline vis_d64
vis_to_double_dup(vis_u32 data)
{
	return vis_to_double(data, data);
}

_VISATTR
static __inline vis_d64
vis_ll_to_double(vis_u64 data)
{
	return *((vis_d64 *)&data);
}

/* 4.6.2 Arithmetic - multiplication */

_VISATTR
static __inline vis_d64
vis_fmul8x16(vis_f32 pixels, vis_d64 scale)
{
	vis_d64 out;

	__asm("fmul8x16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(pixels), "f"(scale));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fmul8x16au(vis_f32 pixels, vis_f32 scale)
{
	vis_d64 out;

	__asm("fmul8x16au %1,%2,%0"
	    : "=f"(out)
	    : "f"(pixels), "f"(scale));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fmul8x16al(vis_f32 pixels, vis_f32 scale)
{
	vis_d64 out;

	__asm("fmul8x16al %1,%2,%0"
	    : "=f"(out)
	    : "f"(pixels), "f"(scale));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fmul8sux16(vis_d64 d1, vis_d64 d2)
{
	vis_d64 out;

	__asm("fmul8sux16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fmul8ulx16(vis_d64 d1, vis_d64 d2)
{
	vis_d64 out;

	__asm("fmul8ulx16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fmuld8sux16(vis_f32 d1, vis_f32 d2)
{
	vis_d64 out;

	__asm("fmuld8sux16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fmuld8ulx16(vis_f32 d1, vis_f32 d2)
{
	vis_d64 out;

	__asm("fmuld8ulx16 %1,%2,%0"
	    : "=f"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

/* 4.5 Pixel compare */

_VISATTR
static __inline int
vis_fcmpgt16(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmpgt16 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmple16(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmple16 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmpeq16(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmpeq16 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmpne16(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmpne16 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmpgt32(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmpgt32 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmple32(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmple32 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmpeq32(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmpeq32 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmpne32(vis_d64 d1, vis_d64 d2)
{
	int out;

	__asm("fcmpne32 %1,%2,%0"
	    : "=r"(out)
	    : "f"(d1), "f"(d2));
	return out;
}

_VISATTR
static __inline int
vis_fcmplt16(vis_d64 d1, vis_d64 d2)
{
	return vis_fcmpgt16(d2, d1);
}

_VISATTR
static __inline int
vis_fcmpge16(vis_d64 d1, vis_d64 d2)
{
	return vis_fcmple16(d2, d1);
}

_VISATTR
static __inline int
vis_fcmplt32(vis_d64 d1, vis_d64 d2)
{
	return vis_fcmpgt32(d2, d1);
}

_VISATTR
static __inline int
vis_fcmpge32(vis_d64 d1, vis_d64 d2)
{
	return vis_fcmple32(d2, d1);
}

/* 4.10 Pixel distance */

_VISATTR
static __inline vis_d64
vis_pdist(vis_d64 pixels1, vis_d64 pixels2, vis_d64 acc)
{
	__asm("pdist %1,%2,%0"
	    : "+f"(acc)
	    : "f"(pixels1), "f"(pixels2));

	return acc;
}

/* 4.4.1 Logical instructions - fill variables */

_VISATTR
static __inline vis_d64
vis_fzero(void)
{
	vis_d64 out;

	__asm("fzero %0"
	    : "=f"(out));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fone(void)
{
	vis_d64 out;

	__asm("fone %0"
	    : "=f"(out));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fzeros(void)
{
	vis_f32 out;

	__asm("fzeros %0"
	    : "=f"(out));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fones(void)
{
	vis_f32 out;

	__asm("fones %0"
	    : "=f"(out));
	return out;
}

/* 4.4.2 Logical instructions - copies and complements */

_VISATTR
static __inline vis_d64
vis_fsrc(vis_d64 r1)
{
	vis_d64 out;

	__asm("fsrc1 %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fnot(vis_d64 r1)
{
	vis_d64 out;

	__asm("fnot1 %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fsrcs(vis_f32 r1)
{
	vis_f32 out;

	__asm("fsrc1s %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fnots(vis_f32 r1)
{
	vis_f32 out;

	__asm("fnot1s %1,%0"
	    : "=f"(out)
	    : "f"(r1));
	return out;
}

/* 4.3 Logical instructions - bitwise */

_VISATTR
static __inline vis_d64
vis_for(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("for %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fand(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fand %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fxor(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fxor %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fnor(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fnor %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fnand(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fnand %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fxnor(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fxnor %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fornot(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fornot1 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_d64
vis_fandnot(vis_d64 r1, vis_d64 r2)
{
	vis_d64 out;
	__asm("fandnot1 %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fors(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fors %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fands(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fands %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fxors(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fxors %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fnors(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fnors %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fnands(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fnands %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fxnors(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fxnors %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fornots(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fornot1s %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

_VISATTR
static __inline vis_f32
vis_fandnots(vis_f32 r1, vis_f32 r2)
{
	vis_f32 out;
	__asm("fandnot1s %1,%2,%0"
	    : "=f"(out)
	    : "f"(r1), "f"(r2));
	return out;
}

/* 4.8.1 Partial Stores */

_VISATTR
static __inline void
vis_pst_8(vis_d64 data, void *addr, vis_u8 mask)
{
	__asm("stda %1,[%0]%2,0xc0"
	    : "=r"(addr)
	    : "f"(data), "r"(mask));
}

_VISATTR
static __inline void
vis_pst_16(vis_d64 data, void *addr, vis_u8 mask)
{
	__asm("stda %1,[%0]%2,0xc2"
	    : "=r"(addr)
	    : "f"(data), "r"(mask));
}

_VISATTR
static __inline void
vis_pst_32(vis_d64 data, void *addr, vis_u8 mask)
{
	__asm("stda %1,[%0]%2,0xc4"
	    : "=r"(addr)
	    : "f"(data), "r"(mask));
}

/* 4.8.2 Byte/Short Loads and Stores */

_VISATTR
static __inline void
vis_st_u8(vis_u64 data, void *addr)
{
	__asm("stda %1,[%0]0xd0"
	    : "=r"(addr)
	    : "f"(data));
}

_VISATTR
static __inline void
vis_st_u8_le(vis_d64 data, void *addr)
{
	__asm("stda %1,[%0]0xd8"
	    : "=r"(addr)
	    : "f"(data));
}

_VISATTR
static __inline void
vis_st_u16(vis_d64 data, void *addr)
{
	__asm("stda %1,[%0]0xd2"
	    : "=r"(addr)
	    : "f"(data));
}

_VISATTR
static __inline void
vis_st_u16_le(vis_d64 data, void *addr)
{
	__asm("stda %1,[%0]0xda"
	    : "=r"(addr)
	    : "f"(data));
}

_VISATTR
static __inline void
vis_st_u8_i(vis_d64 data, void *addr, long idx)
{
	vis_u8 *ptr = addr;
	vis_st_u8(data, ptr + idx);
}

_VISATTR
static __inline void
vis_st_u16_i(vis_d64 data, void *addr, long idx)
{
	vis_u8 *ptr = addr;
	vis_st_u16(data, ptr + idx);
}

_VISATTR
static __inline vis_d64
vis_ld_u8(void *addr)
{
	vis_u8 val;
	vis_d64 out;

	val = *((vis_u8 *)addr);
	*((vis_u8 *)&out) = val;

	return out;
}

_VISATTR
static __inline vis_d64
vis_ld_u16(void *addr)
{
	vis_u16 val;
	vis_d64 out;

	val = *((vis_u16 *)addr);
	*((vis_u16 *)&out) = val;

	return out;
}

_VISATTR
static __inline vis_d64
vis_ld_u8_i(void *addr, long idx)
{
	vis_u8 *ptr = addr;
	return vis_ld_u8(ptr + idx);
}

_VISATTR
static __inline vis_d64
vis_ld_u16_i(void *addr, long idx)
{
	vis_u8 *ptr = addr;
	return vis_ld_u16(ptr + idx);
}

/*
 * VIS 2.0 instructions
 */

_VISATTR
static __inline vis_u32
vis_read_bmask(void)
{
	vis_u32 out;

	__asm("rd %%gsr,%0"
	    "srlx %0,32,%0"
	    : "+f"(out));
	return out;
}

_VISATTR
static __inline void
vis_write_bmask(vis_u32 mask1, vis_u32 mask2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	(void)__builtin_vis_bmask(mask1, mask2);
#else
	vis_u32 out;

	__asm("bmask %1,%2,%0"
	    : "=r"(out)
	    : "r"(mask1), "r"(mask2));

	(void)out;
#endif
}

_VISATTR
static __inline vis_d64
vis_bshuffle(vis_d64 pixels1, vis_d64 pixels2)
{
	vis_d64 out;

	__asm("bshuffle %1,%2,%0"
	    : "=f"(out)
	    : "f"(pixels1), "f"(pixels2));
	return out;
}

_VISATTR
static __inline vis_s32
vis_edge8n(void *a1, void *a2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	return __builtin_vis_edge8n(a1, a2);
#else
	vis_s32 out;

	__asm("edge8n %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "r"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge16n(void *a1, void *a2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	return __builtin_vis_edge16n(a1, a2);
#else
	vis_s32 out;

	__asm("edge16n %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "r"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge32n(void *a1, void *a2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	return __builtin_vis_edge32n(a1, a2);
#else
	vis_s32 out;

	__asm("edge32n %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "r"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge8ln(void *a1, void *a2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	return __builtin_vis_edge8ln(a1, a2);
#else
	vis_s32 out;

	__asm("edge8ln %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "r"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge16ln(void *a1, void *a2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	return __builtin_vis_edge16ln(a1, a2);
#else
	vis_s32 out;

	__asm("edge16ln %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "r"(a2));
	return out;
#endif
}

_VISATTR
static __inline vis_s32
vis_edge32ln(void *a1, void *a2)
{
#if defined(__VIS__) && __VIS__ >= 0x200 && defined(__GNUC__)
	return __builtin_vis_edge32ln(a1, a2);
#else
	vis_s32 out;

	__asm("edge32ln %1,%2,%0"
	    : "=r"(out)
	    : "r"(a1), "r"(a2));
	return out;
#endif
}

#ifdef __cplusplus
}
#endif

#endif
