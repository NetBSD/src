#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: head/lib/msun/src/s_llrintl.c 175309 2008-01-14 02:12:07Z das $");
#else
__RCSID("$NetBSD: s_llrintl.c,v 1.2 2017/05/07 11:29:21 martin Exp $");
#endif

#ifdef __HAVE_LONG_DOUBLE
#define stype		long double
#define	roundit		rintl
#define dtype		long long
#define	fn		llrintl

#include "s_lrint.c"
#endif

