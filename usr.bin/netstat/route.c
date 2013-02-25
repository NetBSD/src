/*	$NetBSD: route.c,v 1.79.2.1 2013/02/25 00:30:37 tls Exp $	*/

/*
 * Copyright (c) 1983, 1988, 1993
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
 */

#include <sys/cdefs.h>

__RCSID("$NetBSD: route.c,v 1.79.2.1 2013/02/25 00:30:37 tls Exp $");

#include <sys/sysctl.h>
#include <net/route.h>
#include <err.h>

#include "netstat.h"

/*
 * Print routing statistics
 */
void
rt_stats(u_long off)
{
	struct rtstat rtstats;

	if (use_sysctl) {
		size_t rtsize = sizeof(rtstats);

		if (sysctlbyname("net.route.stats", &rtstats, &rtsize,
		    NULL, 0) == -1)
			err(1, "rt_stats: sysctl");
	} else 	if (off == 0) {
		printf("rtstat: symbol not in namelist\n");
		return;
	} else
		kread(off, (char *)&rtstats, sizeof(rtstats));

	printf("routing:\n");
	printf("\t%llu bad routing redirect%s\n",
		(unsigned long long)rtstats.rts_badredirect,
		plural(rtstats.rts_badredirect));
	printf("\t%llu dynamically created route%s\n",
		(unsigned long long)rtstats.rts_dynamic,
		plural(rtstats.rts_dynamic));
	printf("\t%llu new gateway%s due to redirects\n",
		(unsigned long long)rtstats.rts_newgateway,
		plural(rtstats.rts_newgateway));
	printf("\t%llu destination%s found unreachable\n",
		(unsigned long long)rtstats.rts_unreach,
		plural(rtstats.rts_unreach));
	printf("\t%llu use%s of a wildcard route\n",
		(unsigned long long)rtstats.rts_wildcard,
		plural(rtstats.rts_wildcard));
}
