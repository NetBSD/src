#ifndef lint
static char rcsid[] = "$Id: m68k.c,v 1.2 1994/05/17 03:36:10 cgd Exp $";
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
