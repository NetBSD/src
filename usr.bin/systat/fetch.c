/*	$NetBSD: fetch.c,v 1.10 2003/01/20 18:24:03 dsl Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)fetch.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: fetch.c,v 1.10 2003/01/20 18:24:03 dsl Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/sched.h>
#include <sys/sysctl.h>

#include <string.h>

#include "systat.h"
#include "extern.h"

ssize_t
kvm_ckread(const void *a, void *b, size_t l)
{
	if (kvm_read(kd, (u_long)a, b, l) != l) {
		if (verbose)
			error("error reading kmem at %lx\n",
			   (u_long) a);
		return (0);
	} else
		return (1);
}

int
fetch_cptime(u_int64_t *cptime)
{
	size_t ssize;
	int mib[2];

	/*
	 * XXX Need to locate the `correct' CPU when looking for this
	 * XXX in crash dumps.  Just don't report it for now, in that
	 * XXX case.
	 */
	ssize = CPUSTATES * sizeof(u_int64_t);
	memset(cptime, 0, ssize);
	if (memf == NULL) {
		mib[0] = CTL_KERN;
		mib[1] = KERN_CP_TIME;
		if (sysctl(mib, 2, cptime, &ssize, NULL, 0) < 0) {
			if (verbose)
				error("error fetching cp_time\n");
			return (0);
		} else
			return (1);
	}
	return (1);
}
