/* $NetBSD: sysarch.h,v 1.3.2.2 1999/04/30 00:58:33 ross Exp $ */

#ifndef _ALPHA_SYSARCH_H_
#define _ALPHA_SYSARCH_H_

#include <machine/ieeefp.h>

/*
 * Architecture specific syscalls (ALPHA)
 */
#define	ALPHA_FPGETMASK   0
#define	ALPHA_FPSETMASK   1
#define	ALPHA_FPSETSTICKY 2

struct alpha_fp_except_args {
	fp_except mask;
};

#ifndef _KERNEL
int sysarch __P((int, void *));
#endif

#endif /* !_ALPHA_SYSARCH_H_ */
