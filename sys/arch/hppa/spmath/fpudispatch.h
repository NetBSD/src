/*	$NetBSD: fpudispatch.h,v 1.1 2002/06/05 01:04:25 fredette Exp $	*/

#include <sys/cdefs.h>

/* Prototypes for the dispatcher. */
int decode_0c __P((unsigned ir, unsigned class, unsigned subop, unsigned *fpregs));
int decode_0e __P((unsigned ir, unsigned class, unsigned subop, unsigned *fpregs));
int decode_06 __P((unsigned ir, unsigned *fpregs));
int decode_26 __P((unsigned ir, unsigned *fpregs));
