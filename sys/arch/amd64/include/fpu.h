/*	$NetBSD: fpu.h,v 1.5 2008/04/16 21:51:03 cegger Exp $	*/

#ifndef	_AMD64_FPU_H_
#define	_AMD64_FPU_H_

/*
 * NetBSD/amd64 only uses the extended save/restore format used
 * by fxsave/fsrestore, to always deal with the SSE registers,
 * which are part of the ABI to pass floating point values.
 * Must be stored in memory on a 16-byte boundary.
 */

struct fxsave64 {
	uint16_t  fx_fcw;
	uint16_t  fx_fsw;
	uint8_t   fx_ftw;
	uint8_t   fx_unused1;
	uint16_t  fx_fop;
	uint64_t  fx_rip;
	uint64_t  fx_rdp;
	uint32_t  fx_mxcsr;
	uint32_t  fx_mxcsr_mask;
	uint64_t  fx_st[8][2];   /* 8 normal FP regs */
	uint64_t  fx_xmm[16][2]; /* 16 SSE2 registers */
	uint8_t   fx_unused3[96];
} __packed;

struct savefpu {
	struct fxsave64 fp_fxsave;	/* see above */
	uint16_t fp_ex_sw;		/* saved status from last exception */
	uint16_t fp_ex_tw;		/* saved tag from last exception */
} __aligned(16);

#ifdef _KERNEL

/*
 * This one only used for backward compat coredumping.
 */
struct oldfsave {
	uint16_t	fs_control;
	uint16_t	fs_unused0;
	uint16_t	fs_status;
	uint16_t	fs_unused1;
	uint16_t	fs_tag;
	uint16_t	fs_unused2;
	uint32_t	fs_ipoff;
	uint16_t	fs_ipsel;
	uint16_t	fs_op;
	uint32_t	fs_opoff;
	uint16_t	fs_opsel;
} __attribute__ ((packed));

#endif


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
