/*	$NetBSD: setjmp.h,v 1.4 1999/01/15 03:43:56 castor Exp $	*/

/*
 * mips/setjmp.h: machine dependent setjmp-related information.
 *
 * For the size of this, refer to <machine/signal.h> as this uses the 
 * struct sigcontext to restore it.
 */

#ifndef _JBLEN		/* Size in longs of jmp_buf */
#include <machine/pubassym.h> 
#endif
