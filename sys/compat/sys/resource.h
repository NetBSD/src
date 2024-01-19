/*	$NetBSD: resource.h,v 1.6 2024/01/19 18:39:15 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)resource.h	8.4 (Berkeley) 1/9/95
 */

#ifndef _COMPAT_SYS_RESOURCE_H_
#define	_COMPAT_SYS_RESOURCE_H_

#include <sys/featuretest.h>
#include <sys/time.h>
#include <compat/sys/time.h>

struct	rusage50 {
	struct timeval50 ru_utime;	/* user time used */
	struct timeval50 ru_stime;	/* system time used */
	long	ru_maxrss;		/* max resident set size */
	long	ru_ixrss;		/* integral shared memory size */
	long	ru_idrss;		/* integral unshared data " */
	long	ru_isrss;		/* integral unshared stack " */
	long	ru_minflt;		/* page reclaims */
	long	ru_majflt;		/* page faults */
	long	ru_nswap;		/* swaps */
	long	ru_inblock;		/* block input operations */
	long	ru_oublock;		/* block output operations */
	long	ru_msgsnd;		/* messages sent */
	long	ru_msgrcv;		/* messages received */
	long	ru_nsignals;		/* signals received */
	long	ru_nvcsw;		/* voluntary context switches */
	long	ru_nivcsw;		/* involuntary " */
};

static __inline void
rusage_to_rusage50(const struct rusage *ru, struct rusage50 *ru50)
{
	memset(ru50, 0, sizeof(*ru50));
	(void)memcpy(&ru50->ru_first, &ru->ru_first,
	    (char *)&ru50->ru_last - (char *)&ru50->ru_first +
	    sizeof(ru50->ru_last));
	ru50->ru_maxrss = ru->ru_maxrss;
	timeval_to_timeval50(&ru->ru_utime, &ru50->ru_utime);
	timeval_to_timeval50(&ru->ru_stime, &ru50->ru_stime);
}

#ifndef _KERNEL
__BEGIN_DECLS
int	__compat_getrusage(int, struct rusage50 *) __dso_hidden;
int	__getrusage50(int, struct rusage *);
__END_DECLS
#endif	/* _KERNEL */
#endif	/* !_COMPAT_SYS_RESOURCE_H_ */
