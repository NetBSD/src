/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(__catclose,catclose);
#else

#include <nl_types.h>

extern void __catclose __P((nl_catd));

void
catclose(catd)
	nl_catd catd;
{
	__catclose(catd);
}

#endif
