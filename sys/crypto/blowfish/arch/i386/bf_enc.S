/*	$NetBSD: bf_enc.S,v 1.3.56.1 2007/12/26 19:56:59 ad Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@zembu.com> and Thor Lancelot Simon
 * <tls@NetBSD.org>.  Public domain.
 */

#include <i386/include/asm.h>
__KERNEL_RCSID(0, "$NetBSD: bf_enc.S,v 1.3.56.1 2007/12/26 19:56:59 ad Exp $");

/*
 * XXX Should use CPP symbols defined as a result of
 * XXX `cc -mcpu=pentiumpro'.
 */
#if defined(I386_CPU) || defined(I486_CPU) || defined(I586_CPU)
#include "bf_enc_586.S"
#else
#include "bf_enc_686.S"
#endif
