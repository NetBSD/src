/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>
#include <nl_types.h>

extern void _catclose __P((nl_catd));

void
catclose(catd)
	nl_catd catd;
{
	_catclose(catd);
}
