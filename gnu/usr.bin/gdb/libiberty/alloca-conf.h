/*	$Id: alloca-conf.h,v 1.1 1994/01/28 12:42:54 pk Exp $ */

/* "Normal" configuration for alloca.  */

#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not __GNUC__ */
#ifdef sparc
#include <alloca.h>
#else
char *alloca ();
#endif /* sparc */
#endif /* not __GNUC__ */
