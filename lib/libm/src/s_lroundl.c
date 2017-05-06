#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: head/lib/msun/src/s_lroundl.c 144772 2005-04-08 01:24:08Z das $");
#else
__RCSID("$NetBSD: s_lroundl.c,v 1.1 2017/05/06 18:03:24 christos Exp $");
#endif

#define stype		long double
#define	roundit		roundl
#define dtype		long
#define	DTYPE_MIN	LONG_MIN
#define	DTYPE_MAX	LONG_MAX
#define	fn		lroundl

#include "s_lround.c"
