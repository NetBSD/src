/*	$NetBSD: powerpc.c,v 1.1 1998/05/06 22:02:18 mycroft Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: powerpc.c,v 1.1 1998/05/06 22:02:18 mycroft Exp $");
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
