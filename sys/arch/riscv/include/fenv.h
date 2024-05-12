/*	$NetBSD: fenv.h,v 1.5 2024/05/12 20:04:12 riastradh Exp $	*/

/*
 * Based on ieeefp.h written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _RISCV_FENV_H_
#define _RISCV_FENV_H_

typedef int fenv_t;		/* FPSCR */
typedef int fexcept_t;

#define	FE_INEXACT	((int)__BIT(0))	/* Result inexact */
#define	FE_UNDERFLOW	((int)__BIT(1))	/* Result underflowed */
#define	FE_OVERFLOW	((int)__BIT(2))	/* Result overflowed */
#define	FE_DIVBYZERO	((int)__BIT(3))	/* divide-by-zero */
#define	FE_INVALID	((int)__BIT(4))	/* Result invalid */

#define	FE_ALL_EXCEPT	\
    (FE_INEXACT | FE_UNDERFLOW | FE_OVERFLOW | FE_DIVBYZERO | FE_INVALID)

#define	FE_TONEAREST	0	/* round to nearest representable number */
#define	FE_TOWARDZERO	1	/* round to zero (truncate) */
#define	FE_DOWNWARD	2	/* round toward negative infinity */
#define	FE_UPWARD	3	/* round toward positive infinity */

__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define FE_DFL_ENV	(&__fe_dfl_env)

__END_DECLS

#endif /* _RISCV_FENV_H_ */
