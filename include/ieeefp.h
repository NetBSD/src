/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _IEEEFP_H_
#define _IEEEFP_H_

#include <sys/cdefs.h>

typedef int fp_except;
#ifdef __i386__
#define FP_X_INV	0x01	/* invalid operation exception */
#define FP_X_DNML	0x02	/* denormalization exception */
#define FP_X_DZ		0x04	/* divide-by-zero exception */
#define FP_X_OFL	0x08	/* overflow exception */
#define FP_X_UFL	0x10	/* underflow exception */
#define FP_X_IMP	0x20	/* imprecise (loss of precision) */
#endif

#if defined(__i386__)
typedef enum {
    FP_RN=0,			/* round to nearest representable number */
    FP_RP=1,			/* round to positive infinity */
    FP_RM=2,			/* round to negative infinity */
    FP_RZ=3			/* round to zero (truncate) */
} fp_rnd ;

#elif defined(__m68k__)
typedef enum {
    FP_RN=0,			/* round to nearest representable number */
    FP_RZ=1,			/* round to zero (truncate) */
    FP_RM=2,			/* round to negative infinity */
    FP_RP=3			/* round to positive infinity */
} fp_rnd ;

#endif

extern fp_rnd    fpgetround __P((void));
extern fp_rnd    fpsetround __P((fp_rnd));
extern fp_except fpgetmask __P((void));
extern fp_except fpsetmask __P((fp_except));
extern fp_except fpgetsticky __P((void));
extern fp_except fpsetsticky __P((fp_except));

#endif /* _IEEEFP_H_ */
