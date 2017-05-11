#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: head/lib/msun/src/s_llrintf.c 140088 2005-01-11 23:12:55Z das $");
#else
__RCSID("$NetBSD: s_llrintf.c,v 1.1.2.2 2017/05/11 02:58:33 pgoyette Exp $");
#endif

#define stype		float
#define	roundit		rintf
#define dtype		long long
#define	fn		llrintf

#include "s_lrint.c"
