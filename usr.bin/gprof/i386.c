/*	$NetBSD: i386.c,v 1.6 1998/02/22 12:55:45 christos Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: i386.c,v 1.6 1998/02/22 12:55:45 christos Exp $");
#endif /* not lint */

#include "gprof.h"

/*
 * gprof -c isn't currently supported...
 */
void
findcall( parentp , p_lowpc , p_highpc )
    nltype		*parentp;
    unsigned long	p_lowpc;
    unsigned long	p_highpc;
{
}
