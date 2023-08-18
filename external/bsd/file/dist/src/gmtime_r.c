/*	$NetBSD: gmtime_r.c,v 1.1.1.2 2023/08/18 18:36:49 christos Exp $	*/

/*	$File: gmtime_r.c,v 1.4 2022/09/24 20:30:13 christos Exp $	*/

#include "file.h"
#ifndef	lint
#if 0
FILE_RCSID("@(#)$File: gmtime_r.c,v 1.4 2022/09/24 20:30:13 christos Exp $")
#else
__RCSID("$NetBSD: gmtime_r.c,v 1.1.1.2 2023/08/18 18:36:49 christos Exp $");
#endif
#endif	/* lint */
#include <time.h>
#include <string.h>

/* asctime_r is not thread-safe anyway */
struct tm *
gmtime_r(const time_t *t, struct tm *tm)
{
	struct tm *tmp = gmtime(t);
	if (tmp == NULL)
		return NULL;
	memcpy(tm, tmp, sizeof(*tm));
	return tmp;
}
