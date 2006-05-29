/* $NetBSD: compat_ntp_gettime.c,v 1.1 2006/05/29 10:02:56 drochner Exp $ */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: compat_ntp_gettime.c,v 1.1 2006/05/29 10:02:56 drochner Exp $");
#endif /* LIBC_SCCS and not lint */

#define __LIBC12_SOURCE__

#include <sys/time.h>
#include <sys/timex.h>
#include <compat/sys/timex.h>

__warn_references(ntp_gettime,
    "warning: reference to compatibility ntp_gettime(); include <sys/timex.h> for correct reference")

int
ntp_gettime(struct ntptimeval30 *ontvp)
{
	struct ntptimeval ntv;
	int res;

	res = __ntp_gettime30(&ntv);
	TIMESPEC_TO_TIMEVAL(&ontvp->time, &ntv.time);
	ontvp->maxerror = ntv.maxerror;
	ontvp->esterror = ntv.esterror;

	return (res);
}
