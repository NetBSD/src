/*
 * Written by J.T. Conklin, 10/05/94
 * Public domain.
 */

#include <sys/cdefs.h>

#ifdef __weak_reference
__weak_reference(_catopen,catopen);
#else

#include <nl_types.h>

extern nl_catd _catopen __P((char *, int));

nl_catd
catopen(name, oflag)
	char *name;
	int oflag;
{
	return _catopen(name, oflag);
}

#endif
