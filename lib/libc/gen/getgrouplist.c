/*	$NetBSD: getgrouplist.c,v 1.14 1999/09/20 04:39:00 lukem Exp $	*/

/*
 * Copyright (c) 1991, 1993
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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getgrouplist.c	8.2 (Berkeley) 12/8/94";
#else
__RCSID("$NetBSD: getgrouplist.c,v 1.14 1999/09/20 04:39:00 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * get credential
 */
#include "namespace.h"
#include <sys/param.h>

#include <assert.h>
#include <grp.h>
#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(getgrouplist,_getgrouplist);
#endif

int
getgrouplist(uname, agroup, groups, grpcnt)
	const char *uname;
	gid_t agroup;
	gid_t *groups;
	int *grpcnt;
{
	struct group *grp;
	int i, ngroups;
	int ret, maxgroups;

	_DIAGASSERT(uname != NULL);
	_DIAGASSERT(groups != NULL);
	_DIAGASSERT(grpcnt != NULL);

	ret = 0;
	ngroups = 0;
	maxgroups = *grpcnt;

	/*
	 * install primary group
	 */
	if (ngroups < maxgroups)
		groups[ngroups] = agroup;
	else
		ret = -1;
	ngroups++;

	/*
	 * Scan the group file to find additional groups.
	 */
	setgrent();
 nextgroup:
	while ((grp = getgrent()) != NULL) {
		if (grp->gr_gid == agroup)
			continue;
		for (i = 0; grp->gr_mem[i]; i++) {
			if (!strcmp(grp->gr_mem[i], uname)) {
				for (i = 0; i < MIN(ngroups, maxgroups); i++) {
					if (grp->gr_gid == groups[i])
						goto nextgroup;
				}
				if (ngroups < maxgroups)
					groups[ngroups] = grp->gr_gid;
				else
					ret = -1;
				ngroups++;
				break;
			}
		}
	}
	endgrent();
	*grpcnt = ngroups;
	return (ret);
}
