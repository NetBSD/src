#ifndef lint
static char rcsid[] = "$Id: ns32k.c,v 1.1 1994/03/09 00:25:18 phil Exp $";
#endif /* not lint */

#include "gprof.h"

/*
 * gprof -c isn't currently supported...
 */
findcall( parentp , p_lowpc , p_highpc )
    nltype		*parentp;
    unsigned long	p_lowpc;
    unsigned long	p_highpc;
{
}
