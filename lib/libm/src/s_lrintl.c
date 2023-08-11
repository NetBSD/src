#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: head/lib/msun/src/s_lrintl.c 175309 2008-01-14 02:12:07Z das $");
#else
__RCSID("$NetBSD: s_lrintl.c,v 1.2.22.1 2023/08/11 14:44:19 martin Exp $");
#endif

#include <math.h>

#ifdef __HAVE_LONG_DOUBLE
#define stype		long double
#define	roundit		rintl
#define dtype		long
#define	fn		lrintl

#include "s_lrint.c"
#endif
