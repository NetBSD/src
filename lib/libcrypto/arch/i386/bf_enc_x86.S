/*	$NetBSD: bf_enc_x86.S,v 1.1 2000/07/31 18:39:06 thorpej Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@zembu.com>
 * Public domain.
 */

/*
 * XXX Should use CPP symbols defined as a result of
 * XXX `cc -mcpu=pentiumpro'.
 */
#if 0
#include "bf_enc_686.S"
#else
#include "bf_enc_586.S"
#endif
