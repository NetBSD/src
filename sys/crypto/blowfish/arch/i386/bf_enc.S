/*	$NetBSD: bf_enc.S,v 1.1.24.3 2004/09/21 13:26:15 skrll Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@zembu.com> and Thor Lancelot Simon
 * <tls@NetBSD.org>.  Public domain.
 */

/*
 * XXX Should use CPP symbols defined as a result of
 * XXX `cc -mcpu=pentiumpro'.
 */
#if defined(I386_CPU) || defined(I486_CPU) || defined(I586_CPU)
#include "bf_enc_586.S"
#else
#include "bf_enc_686.S"
#endif
