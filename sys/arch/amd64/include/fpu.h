/*	$NetBSD: fpu.h,v 1.5.38.1 2013/01/23 00:05:38 yamt Exp $	*/

#ifndef	_AMD64_FPU_H_
#define	_AMD64_FPU_H_

/*
 * NetBSD/amd64 only uses the extended save/restore format used
 * by fxsave/fsrestore, to always deal with the SSE registers,
 * which are part of the ABI to pass floating point values.
 *
 * The memory used for the 'fsave' instruction must be 16 byte aligned,
 * but the definition here isn't aligned to avoid padding elsewhere.
 */

struct fxsave64 {
	uint16_t  fx_fcw;           /* 0: FPU control word */
	uint16_t  fx_fsw;           /* 2: FPU status word */
	uint8_t   fx_ftw;           /* 4: Abridged FPU tag word */
	uint8_t   fx_reserved1;     /* 5: */
	uint16_t  fx_fop;           /* 6: Low 11 bits are FPU opcode */
	uint64_t  fx_rip;           /* 8: Address of faulting instruction */
	uint64_t  fx_rdp;           /* 16: Data address associated with fault */
	uint32_t  fx_mxcsr;         /* 24: SIMD control & status */
	uint32_t  fx_mxcsr_mask;    /* 28: */
	uint64_t  fx_st[8][2];      /* 32: 8 normal FP regs (80 bit) */
	uint64_t  fx_xmm[16][2];    /* 160: 16 SSE2 registers */
	uint8_t   fx_reserved2[48]; /* 416: */
	uint8_t   fx_available[48]; /* 464: could be used by kernel */
};

__CTASSERT(sizeof (struct fxsave64) == 512);

struct savefpu {
	struct fxsave64 fp_fxsave;	/* see above */
};

struct savefpu_i387 {
	uint16_t fp_ex_sw;		/* saved status from last exception */
	uint16_t fp_ex_tw;		/* saved tag from last exception */
};

/*
 * The i387 defaults to Intel extended precision mode and round to nearest,
 * with all exceptions masked.
 */
#define	__INITIAL_NPXCW__	0x037f
#define __INITIAL_MXCSR__ 	0x1f80
#define __INITIAL_MXCSR_MASK__	0xffbf

/* NetBSD uses IEEE double precision. */
#define	__NetBSD_NPXCW__	0x127f
/* Linux just uses the default control word. */
#define	__Linux_NPXCW__		0x037f

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
 */

#ifdef _KERNEL
/*
 * XXX
 */
struct trapframe;
struct cpu_info;

void fpuinit(struct cpu_info *);
void fpudrop(void);
void fpusave(struct lwp *);
void fpudiscard(struct lwp *);
void fputrap(struct trapframe *);
void fpusave_lwp(struct lwp *, bool);
void fpusave_cpu(bool);

#endif

#endif /* _AMD64_FPU_H_ */
