/*	$NetBSD: bf_enc_x86.S,v 1.2.2.2 2002/08/29 16:39:52 he Exp $	*/

/*
 * Written by Jason R. Thorpe <thorpej@zembu.com>
 * Public domain.
 */

#if defined(__pentiumpro__) || defined(__tune_pentiumpro__) || \
    defined(__tune_i686__)
#include "bf_enc_686.S"
#else
#include "bf_enc_586.S"
#endif
