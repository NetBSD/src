#ifndef lint
static char rcsid[] = "$Id: sparc.c,v 1.1 1993/12/02 19:12:22 pk Exp $";
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
