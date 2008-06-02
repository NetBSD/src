/*	$NetBSD: fpudispatch.h,v 1.1.112.1 2008/06/02 13:22:13 mjf Exp $	*/

#include <sys/cdefs.h>

/* Prototypes for the dispatcher. */
int decode_0c(unsigned ir, unsigned class, unsigned subop, unsigned *fpregs);
int decode_0e(unsigned ir, unsigned class, unsigned subop, unsigned *fpregs);
int decode_06(unsigned ir, unsigned *fpregs);
int decode_26(unsigned ir, unsigned *fpregs);
