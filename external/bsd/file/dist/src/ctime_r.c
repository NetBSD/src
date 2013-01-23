/*	$NetBSD: ctime_r.c,v 1.1.1.1.2.2 2013/01/23 00:04:36 yamt Exp $	*/

/*	$File: ctime_r.c,v 1.1 2012/05/15 17:14:36 christos Exp $	*/

#include "file.h"
#ifndef	lint
#if 0
FILE_RCSID("@(#)$File: ctime_r.c,v 1.1 2012/05/15 17:14:36 christos Exp $")
#else
__RCSID("$NetBSD: ctime_r.c,v 1.1.1.1.2.2 2013/01/23 00:04:36 yamt Exp $");
#endif
#endif	/* lint */
#include <time.h>
#include <string.h>

/* ctime_r is not thread-safe anyway */
char *
ctime_r(const time_t *t, char *dst)
{
	char *p = ctime(t);
	if (p == NULL)
		return NULL;
	memcpy(dst, p, 26);
	return dst;
}
