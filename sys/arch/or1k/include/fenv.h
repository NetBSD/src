/*	$NetBSD: fenv.h,v 1.1 2014/09/03 19:34:26 matt Exp $	*/

/* 
 * Based on ieeefp.h written by J.T. Conklin, Apr 28, 1995
 * Public domain.
 */

#ifndef _OR1K_FENV_H_
#define _OR1K_FENV_H_

typedef int fenv_t;		/* FPSCR */
typedef int fexcept_t;

#define	FE_OVERFLOW	0x001	/* Result overflowed */
#define	FE_UNDERFLOW	0x002	/* Result underflowed */
#define	FE_SNAN		0x004	/* Result SNaN */
#define	FE_QNAN		0x008	/* Result QNaN */
#define	FE_ZERO		0x010	/* Result zero */
#define	FE_INEXACT	0x020	/* Result inexact */
#define	FE_INVALID	0x040	/* Result invalid */
#define	FE_INFINITY	0x080	/* Result infinite */
#define	FE_DIVBYZERO	0x100	/* divide-by-zero */

#define	FE_ALL_EXCEPT	0x1ff

#define	FE_TONEAREST	0	/* round to nearest representable number */
#define	FE_TOWARDZERO	1	/* round to zero (truncate) */
#define	FE_UPWARD	2	/* round toward positive infinity */
#define	FE_DOWNWARD	3	/* round toward negative infinity */

__BEGIN_DECLS

/* Default floating-point environment */
extern const fenv_t	__fe_dfl_env;
#define FE_DFL_ENV	(&__fe_dfl_env)

__END_DECLS

#endif /* _OR1K_FENV_H_ */
