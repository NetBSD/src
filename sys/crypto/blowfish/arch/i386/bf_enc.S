/*	$NetBSD: bf_enc.S,v 1.3.46.1 2008/01/09 01:52:07 matt Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@zembu.com> and Thor Lancelot Simon
 * <tls@NetBSD.org>.  Public domain.
 */

#include <i386/include/asm.h>
__KERNEL_RCSID(0, "$NetBSD: bf_enc.S,v 1.3.46.1 2008/01/09 01:52:07 matt Exp $");

/*
 * XXX Should use CPP symbols defined as a result of
 * XXX `cc -mcpu=pentiumpro'.
 */
#if defined(I386_CPU) || defined(I486_CPU) || defined(I586_CPU)
#include "bf_enc_586.S"
#else
#include "bf_enc_686.S"
#endif
