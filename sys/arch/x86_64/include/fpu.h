/*	$NetBSD: fpu.h,v 1.1 2001/06/19 00:20:10 fvdl Exp $	*/

#ifndef	_X86_64_FPU_H_
#define	_X86_64_FPU_H_

/*
 * NetBSD/x86_64 only uses the extended save/restore format used
 * by fxsave/fsrestore, to always deal with the SSE registers,
 * which are part of the ABI to pass floating point values.
 */

struct fxsave64 {
	u_int64_t	fx_fcw:16;
	u_int64_t	fx_fsw:16;
	u_int64_t	fx_unused1:8;
	u_int64_t	fx_ftw:8;
	u_int64_t	fx_fop:16;
	u_int64_t	fx_rip;
	u_int64_t	fx_dp;
	u_int64_t	fx_mxcsr:32;
	u_int64_t	fx_unused2:32;
	u_int64_t	fx_st[8 * 2];	/* 8 normal FP regs */
	u_int64_t	fx_xmm[16 * 2];	/* 16 SSE2 registers */
	u_int8_t	fx_unused3[96];
} __attribute__ ((aligned (16)));

/*
 * This one only used for backward compat coredumping.
 */
struct oldfsave {
	u_int16_t	fs_control;
	u_int16_t	fs_unused0;
	u_int16_t	fs_status;
	u_int16_t	fs_unused1;
	u_int16_t	fs_tag;
	u_int16_t	fs_unused2;
	u_int32_t	fs_ipoff;
	u_int16_t	fs_ipsel;
	u_int16_t	fs_op;
	u_int32_t	fs_opoff;
	u_int16_t	fs_opsel;
} __attribute__ ((packed));


/*
 * The i387 defaults to Intel extended precision mode and round to nearest,
 * with all exceptions masked.
 * XXXfvdl check this. This stuff is probably invalid.
 */
#define	__INITIAL_NPXCW__	0x037f
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
/*
 * XXX
 */
struct trapframe;

extern void fpuinit(void);
extern void fpudrop(void);
extern void fpusave(void);
extern void fputrap(struct trapframe *);

extern struct proc *fpuproc;

#endif /* _X86_64_FPU_H_ */
