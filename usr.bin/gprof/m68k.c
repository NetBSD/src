#ifndef lint
static char rcsid[] = "$Id: m68k.c,v 1.1 1993/12/06 05:28:42 cgd Exp $";
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
