#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: head/lib/msun/src/s_llrint.c 140088 2005-01-11 23:12:55Z das $");
#else
__RCSID("$NetBSD: s_llrint.c,v 1.1 2017/05/06 18:03:24 christos Exp $");
#endif

#define stype		double
#define	roundit		rint
#define dtype		long long
#define	fn		llrint

#include "s_lrint.c"
