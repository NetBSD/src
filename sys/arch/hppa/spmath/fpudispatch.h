/*	$NetBSD: fpudispatch.h,v 1.1.2.2 2002/06/23 17:37:14 jdolecek Exp $	*/

#include <sys/cdefs.h>

/* Prototypes for the dispatcher. */
int decode_0c __P((unsigned ir, unsigned class, unsigned subop, unsigned *fpregs));
int decode_0e __P((unsigned ir, unsigned class, unsigned subop, unsigned *fpregs));
int decode_06 __P((unsigned ir, unsigned *fpregs));
int decode_26 __P((unsigned ir, unsigned *fpregs));
