/*	$NetBSD: npx.h,v 1.33 2014/02/04 22:21:35 dsl Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)npx.h	5.3 (Berkeley) 1/18/91
 */

/*
 * 287/387 NPX Coprocessor Data Structures and Constants
 * W. Jolitz 1/90
 */

#ifndef	_I386_NPX_H_
#define	_I386_NPX_H_

/*
 * Layout for code/data pointers relating to FP exceptions.
 * Marked 'packed' because they aren't always 64bit aligned.
 * Since the x86 cpu supports misaligned accesses it isn't
 * worth avoiding the 'packed' attribute.
 */
union fp_addr {
	uint64_t fa_64;	/* Linear address for 64bit systems */
	struct {
		uint32_t fa_off;	/* linear address for 32 bit */
		uint16_t fa_seg;	/* code/data (etc) segment */
		uint16_t fa_opcode;	/* last opcode (sometimes) */
	} fa_32;
} __packed;

/* The x87 registers are 80 bits (in ST(n) order) */
struct fpacc87 {
	uint64_t	f87_mantissa;	/* mantissa */
	uint16_t	f87_exp_sign;	/* exponent and sign */
} __packed;

/* The x87 registers padded out for fxsave */
struct fpaccfx {
	struct fpacc87 r __aligned(16);
};

/*
 * floating point unit registers (fsave instruction).
 * Note that the 'tag word' contains 2 bits per register and is relative
 * to the stack top.
 * The fxsave version is 1 bit per register indexed by register number.
 */
struct save87 {
	uint32_t	s87_cw;		/* control word (16bits) */
	uint32_t	s87_sw;		/* status word (16bits) */
	uint32_t	s87_tw;		/* tag word (16bits) */
	union fp_addr	s87_ip;		/* floating point instruction pointer */
#define s87_opcode s87_ip.fa_32.fa_opcode	/* opcode last executed (11bits) */
	union fp_addr	s87_dp;		/* floating operand offset */
	struct fpacc87	s87_ac[8];	/* accumulator contents, 0-7 */
};
#ifndef __lint__
// Has different packing semantics, adding packed to save87 works
__CTASSERT(sizeof (struct save87) == 108);
#endif

/* SSE/SSE2 registers. */
struct xmmreg {
	uint8_t sse_bytes[16];
};

/* FPU/MMX/SSE/SSE2 context */
struct fxsave {
/*0*/	uint16_t fx_cw;		/* FPU Control Word */
	uint16_t fx_sw;		/* FPU Status Word */
	uint8_t  fx_tw;		/* FPU Tag Word (abridged) */
	uint8_t  fx_reserved1;
	uint16_t fx_opcode;	/* FPU Opcode */
	union fp_addr fx_ip;	/* FPU Instruction Pointer */
/*16*/	union fp_addr fx_dp;	/* FPU Data pointer */
	uint32_t fx_mxcsr;	/* MXCSR Register State */
	uint32_t fx_mxcsr_mask;
	struct fpaccfx fx_87_ac[8];	/* 8 x87 registers */
	struct xmmreg sv_xmmregs[8];	/* XMM regs */
	uint8_t sv_rsvd[16 * 14];
} __aligned(16);
#ifndef __lint__
// lint does not know aligned
__CTASSERT(sizeof (struct fxsave) == 512);
#endif


union savefpu {
	struct save87 sv_87;
	struct fxsave sv_xmm;
};

/*
 * The i387 defaults to Intel extended precision mode and round to nearest,
 * with all exceptions masked.
 */
#define	__INITIAL_NPXCW__	0x037f
/* Modern NetBSD uses the default control word.. */
#define	__NetBSD_NPXCW__	0x037f
/* NetBSD before 6.99.26 forced IEEE double precision. */
#define	__NetBSD_COMPAT_NPXCW__	0x127f
/* FreeBSD leaves some exceptions unmasked as well. */
#define	__FreeBSD_NPXCW__	0x1272
/* iBCS2 goes a bit further and leaves the underflow exception unmasked. */
#define	__iBCS2_NPXCW__		0x0262
/* Linux just uses the default control word. */
#define	__Linux_NPXCW__		0x037f
/* SVR4 uses the same control word as iBCS2. */
#define	__SVR4_NPXCW__		0x0262

/*
 * The default MXCSR value at reset is 0x1f80, IA-32 Instruction
 * Set Reference, pg. 3-369.
 */
#define	__INITIAL_MXCSR__	0x1f80


/*
 * 80387 control word bits
 */
#define EN_SW_INVOP	0x0001  /* Invalid operation */
#define EN_SW_DENORM	0x0002  /* Denormalized operand */
#define EN_SW_ZERODIV	0x0004  /* Divide by zero */
#define EN_SW_OVERFLOW	0x0008  /* Overflow */
#define EN_SW_UNDERFLOW	0x0010  /* Underflow */
#define EN_SW_PRECLOSS	0x0020  /* Loss of precision */
#define EN_SW_DATACHAIN	0x0080	/* Data chain exception */
#define EN_SW_CTL_PREC	0x0300	/* Precision control */
#define EN_SW_CTL_ROUND	0x0c00	/* Rounding control */
#define EN_SW_CTL_INF	0x1000	/* Infinity control */

/*
 * The standard control word from finit is 0x37F, giving:
 *	round to nearest
 *	64-bit precision
 *	all exceptions masked.
 *
 * Now we want:
 *	affine mode (if we decide to support 287's)
 *	round to nearest
 *	53-bit precision
 *	all exceptions masked.
 *
 * 64-bit precision often gives bad results with high level languages
 * because it makes the results of calculations depend on whether
 * intermediate values are stored in memory or in FPU registers.
 *
 * The iBCS control word has underflow, overflow, zero divide, and invalid
 * operation exceptions unmasked.  But that causes an unexpected exception
 * in the test program 'paranoia' and makes denormals useless (DBL_MIN / 2
 * underflows).  It doesn't make a lot of sense to trap underflow without
 * trapping denormals.
 */

#ifdef _KERNEL

int	npx586bug1(int, int);
void 	fpuinit(struct cpu_info *);
void	process_xmm_to_s87(const struct fxsave *, struct save87 *);
void	process_s87_to_xmm(const struct save87 *, struct fxsave *);
struct lwp;
int	npxtrap(struct lwp *);

#endif

#endif /* !_I386_NPX_H_ */
