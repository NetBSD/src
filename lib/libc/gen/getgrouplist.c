/*	$NetBSD: getgrouplist.c,v 1.20 2004/09/28 10:46:19 lukem Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getgrouplist.c	8.2 (Berkeley) 12/8/94";
#else
__RCSID("$NetBSD: getgrouplist.c,v 1.20 2004/09/28 10:46:19 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * calculate group access list
 */

#include "namespace.h"
#include <sys/param.h>

#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <nsswitch.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HESIOD
#include <hesiod.h>
#endif

#ifdef __weak_alias
__weak_alias(getgrouplist,_getgrouplist)
#endif

#ifdef HESIOD

/*ARGSUSED*/
static int
_nss_dns_getgrouplist(void *retval, void *cb_data, va_list ap)
{
	int		*result	= va_arg(ap, int *);
	const char 	*uname	= va_arg(ap, const char *);
	gid_t		 agroup	= va_arg(ap, gid_t);
	gid_t		*groups	= va_arg(ap, gid_t *);
	int		*grpcnt	= va_arg(ap, int *);

	unsigned long	id;
	void		*context;
	char		**hp, *cp, *ep;
	int		rv, ret, ngroups, maxgroups;

	hp = NULL;
	rv = NS_NOTFOUND;
	ret = 0;

	if (hesiod_init(&context) == -1)		/* setup hesiod */
		return NS_UNAVAIL;

	hp = hesiod_resolve(context, uname, "grplist");	/* find grplist */
	if (hp == NULL) {
		if (errno != ENOENT)
			rv = NS_NOTFOUND;
		goto dnsgrouplist_out;
	}

	if ((ep = strchr(hp[0], '\n')) != NULL)
		*ep = '\0';				/* clear trailing \n */

	ret = 0;
	ngroups = 0;
	maxgroups = *grpcnt;

	if (ngroups < maxgroups)			/* add primary gid */
		groups[ngroups] = agroup;
	else
		ret = -1;
	ngroups++;

	for (cp = hp[0]; *cp != '\0'; ) {		/* parse grplist */
		if ((cp = strchr(cp, ':')) == NULL)	/* skip grpname */
			break;
		cp++;
		id = strtoul(cp, &ep, 10);		/* parse gid */
		if (id > GID_MAX || (*ep != ':' && *ep != '\0')) {
			rv = NS_UNAVAIL;
			goto dnsgrouplist_out;
		}
		cp = ep;
		if (*cp == ':')
			cp++;
		if (ngroups < maxgroups)		/* add this gid */
			groups[ngroups] = (gid_t)id;
		else
			ret = -1;
		ngroups++;
	}

	*result = ret;
	*grpcnt = ngroups;
	rv = NS_SUCCESS;

 dnsgrouplist_out:
	if (hp)
		hesiod_free_list(context, hp);
	hesiod_end(context);
	return rv;
}

#endif /* HESIOD */

int
getgrouplist(const char *uname, gid_t agroup, gid_t *groups, int *grpcnt)
{
	struct group *grp;
	int i, ngroups, maxgroups, ret;

	static const ns_dtab dtab[] = {
		NS_DNS_CB(_nss_dns_getgrouplist, NULL)
		{ 0 }
	};

	_DIAGASSERT(uname != NULL);
	/* groups may be NULL if just sizing when invoked with *grpcnt = 0 */
	_DIAGASSERT(grpcnt != NULL);

			/* first, try source-specific optimized getgrouplist */
	i = nsdispatch(NULL, dtab, NSDB_GROUP, "getgrouplist",
	    __nsdefaultsrc,
	    &ret, uname, agroup, groups, grpcnt);
	if (i == NS_SUCCESS)
		return ret;

			/* fallback to scan the group(5) database */
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
			if (strcmp(grp->gr_mem[i], uname) != 0)
				continue;
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
	endgrent();
	*grpcnt = ngroups;
	return ret;
}
