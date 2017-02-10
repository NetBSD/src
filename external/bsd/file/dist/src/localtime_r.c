/*	$NetBSD: localtime_r.c,v 1.1.1.1 2017/02/10 17:42:57 christos Exp $	*/

/*	$File: localtime_r.c,v 1.2 2015/07/11 14:41:37 christos Exp $	*/

#include "file.h"
#ifndef	lint
#if 0
FILE_RCSID("@(#)$File: localtime_r.c,v 1.2 2015/07/11 14:41:37 christos Exp $")
#else
__RCSID("$NetBSD: localtime_r.c,v 1.1.1.1 2017/02/10 17:42:57 christos Exp $");
#endif
#endif	/* lint */
#include <time.h>
#include <string.h>

/* asctime_r is not thread-safe anyway */
struct tm *
localtime_r(const time_t *t, struct tm *tm)
{
	struct tm *tmp = localtime(t);
	if (tmp == NULL)
		return NULL;
	memcpy(tm, tmp, sizeof(*tm));
	return tmp;
}
