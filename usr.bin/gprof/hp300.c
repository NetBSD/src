#ifndef lint
static char rcsid[] = "$Id: hp300.c,v 1.2 1993/08/02 17:54:32 mycroft Exp $";
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
