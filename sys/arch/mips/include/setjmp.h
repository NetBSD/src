/*	$NetBSD: setjmp.h,v 1.3 1999/01/14 18:45:45 castor Exp $	*/

/*
 * mips/setjmp.h: machine dependent setjmp-related information.
 *
 * For the size of this, refer to <machine/signal.h> as this uses the 
 * struct sigcontext to restore it.
 */

#ifndef __JBLEN
#include <machine/pubassym.h>
#endif

#define _JBLEN __JBLEN	/* Size in longs of jmp_buf */
