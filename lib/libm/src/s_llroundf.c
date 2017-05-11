#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: head/lib/msun/src/s_llroundf.c 144771 2005-04-08 00:52:27Z das $");
#else
__RCSID("$NetBSD: s_llroundf.c,v 1.1.2.2 2017/05/11 02:58:33 pgoyette Exp $");
#endif

#define stype		float
#define	roundit		roundf
#define dtype		long long
#define	DTYPE_MIN	LLONG_MIN
#define	DTYPE_MAX	LLONG_MAX
#define	fn		llroundf

#include "s_lround.c"
