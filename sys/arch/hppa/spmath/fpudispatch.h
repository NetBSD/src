/*	$NetBSD: fpudispatch.h,v 1.2 2008/04/06 08:03:36 skrll Exp $	*/

#include <sys/cdefs.h>

/* Prototypes for the dispatcher. */
int decode_0c(unsigned ir, unsigned class, unsigned subop, unsigned *fpregs);
int decode_0e(unsigned ir, unsigned class, unsigned subop, unsigned *fpregs);
int decode_06(unsigned ir, unsigned *fpregs);
int decode_26(unsigned ir, unsigned *fpregs);
