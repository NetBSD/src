/*	$NetBSD: getgrent.c,v 1.50 2004/10/24 14:52:46 lukem Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2004 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1989, 1993
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

/*
 * Portions Copyright (c) 1994, Jason Downs. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getgrent.c	8.2 (Berkeley) 3/21/94";
#else
__RCSID("$NetBSD: getgrent.c,v 1.50 2004/10/24 14:52:46 lukem Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>

#include <assert.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <nsswitch.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <threadlib.h>


#ifdef HESIOD
#include <hesiod.h>
#endif

#ifdef YP
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#endif

#define _GROUP_COMPAT	/* "group" defaults to compat, so always provide it */

#define	GETGR_R_SIZE_MAX	1024	/* XXXLUKEM: move to {grp,unistd}.h ? */

#ifdef __weak_alias
__weak_alias(endgrent,_endgrent)
__weak_alias(getgrent,_getgrent)
__weak_alias(getgrgid,_getgrgid)
__weak_alias(getgrgid_r,_getgrgid_r)
__weak_alias(getgrnam,_getgrnam)
__weak_alias(getgrnam_r,_getgrnam_r)
__weak_alias(setgrent,_setgrent)
__weak_alias(setgroupent,_setgroupent)
#endif
 
#ifdef _REENTRANT
static 	mutex_t			_grmutex = MUTEX_INITIALIZER;
#endif

static const ns_src defaultcompat[] = {
	{ NSSRC_COMPAT,	NS_SUCCESS },
	{ 0 }
};

static const ns_src defaultcompat_forceall[] = {
	{ NSSRC_COMPAT,	NS_SUCCESS | NS_FORCEALL },
	{ 0 }
};

static const ns_src defaultnis[] = {
	{ NSSRC_NIS,	NS_SUCCESS },
	{ 0 }
};

static const ns_src defaultnis_forceall[] = {
	{ NSSRC_NIS,	NS_SUCCESS | NS_FORCEALL },
	{ 0 }
};


/*
 * _gr_memfrombuf
 *	Obtain want bytes from buffer (of size buflen) and return a pointer
 *	to the available memory after adjusting buffer/buflen.
 *	Returns NULL if there is insufficient space.
 */
static char *
_gr_memfrombuf(size_t want, char **buffer, size_t *buflen)
{
	char	*rv;

	if (want > *buflen) {
		errno = ERANGE;
		return NULL;
	}
	rv = *buffer;
	*buffer += want;
	*buflen -= want;
	return rv;
}

/*
 * _gr_parse
 *	Parses entry as a line per group(5) (without the trailing \n)
 *	and fills in grp with corresponding values; memory for strings
 *	and arrays will be allocated from buf (of size buflen).
 *	Returns 1 if parsed successfully, 0 on parse failure.
 */
static int
_gr_parse(const char *entry, struct group *grp, char *buf, size_t buflen)
{
	unsigned long	id;
	const char	*bp;
	char		*ep;
	size_t		count;
	int		memc;

	_DIAGASSERT(entry != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buf != NULL);

#define COPYTOBUF(to) \
	do { \
		(to) = _gr_memfrombuf(count+1, &buf, &buflen); \
		if ((to) == NULL) \
			return 0; \
		memmove((to), entry, count); \
		to[count] = '\0'; \
	} while (0)	/* LINTED */

#if 0
	if (*entry == '+')			/* fail on compat `+' token */
		return 0;
#endif

	count = strcspn(entry, ":");		/* parse gr_name */
	if (entry[count] == '\0')
		return 0;
	COPYTOBUF(grp->gr_name);
	entry += count + 1;

	count = strcspn(entry, ":");		/* parse gr_passwd */
	if (entry[count] == '\0')
		return 0;
	COPYTOBUF(grp->gr_passwd);
	entry += count + 1;

	count = strcspn(entry, ":");		/* parse gr_gid */
	if (entry[count] == '\0')
		return 0;
	id = strtoul(entry, &ep, 10);
	if (id > GID_MAX || *ep != ':')
		return 0;
	grp->gr_gid = (gid_t)id;
	entry += count + 1;

	memc = 1;				/* for final NULL */
	if (*entry != '\0')
		memc++;				/* for first item */
	for (bp = entry; *bp != '\0'; bp++) {
		if (*bp == ',')
			memc++;
	}
				/* grab ALIGNed char **gr_mem from buf */
	ep = _gr_memfrombuf(memc * sizeof(char *) + ALIGNBYTES, &buf, &buflen);
	grp->gr_mem = (char **)ALIGN(ep);
	if (grp->gr_mem == NULL)
		return 0;

	for (memc = 0; *entry != '\0'; memc++) {
		count = strcspn(entry, ",");	/* parse member */
		COPYTOBUF(grp->gr_mem[memc]);
		entry += count;
		if (*entry == ',')
			entry++;
	}

#undef COPYTOBUF

	grp->gr_mem[memc] = NULL;
	return 1;
}

/*
 * _gr_copy
 *	Copy the contents of fromgrp to grp; memory for strings
 *	and arrays will be allocated from buf (of size buflen).
 *	Returns 1 if copied successfully, 0 on copy failure.
 *	NOTE: fromgrp must not use buf for its own pointers.
 */
static int
_gr_copy(struct group *fromgrp, struct group *grp, char *buf, size_t buflen)
{
	char	*ep;
	int	memc;

	_DIAGASSERT(fromgrp != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buf != NULL);

#define COPYSTR(to, from) \
	do { \
		size_t count = strlen((from)); \
		(to) = _gr_memfrombuf(count+1, &buf, &buflen); \
		if ((to) == NULL) \
			return 0; \
		memmove((to), (from), count); \
		to[count] = '\0'; \
	} while (0)	/* LINTED */

	COPYSTR(grp->gr_name, fromgrp->gr_name);
	COPYSTR(grp->gr_passwd, fromgrp->gr_passwd);
	grp->gr_gid = fromgrp->gr_gid;

	for (memc = 0; fromgrp->gr_mem[memc]; memc++)
		continue;
	memc++;					/* for final NULL */

				/* grab ALIGNed char **gr_mem from buf */
	ep = _gr_memfrombuf(memc * sizeof(char *) + ALIGNBYTES, &buf, &buflen);
	grp->gr_mem = (char **)ALIGN(ep);
	if (grp->gr_mem == NULL)
		return 0;

	for (memc = 0; fromgrp->gr_mem[memc]; memc++) {
		COPYSTR(grp->gr_mem[memc], fromgrp->gr_mem[memc]);
	}

#undef COPYSTR

	grp->gr_mem[memc] = NULL;
	return 1;
}


		/*
		 *	files methods
		 */

	/* state shared between files methods */
struct files_state {
	int	 stayopen;	/* see getgroupent(3) */
	FILE	*fp;		/* groups file handle */
};

static struct files_state	_files_state;
					/* storage for non _r functions */
static struct group		_files_group;
static char			_files_groupbuf[GETGR_R_SIZE_MAX];

static int
_files_start(struct files_state *state)
{

	_DIAGASSERT(state != NULL);

	if (state->fp == NULL) {
		state->fp = fopen(_PATH_GROUP, "r");
		if (state->fp == NULL)
			return NS_UNAVAIL;
	} else {
		rewind(state->fp);
	}
	return NS_SUCCESS;
}

static int
_files_end(struct files_state *state)
{

	_DIAGASSERT(state != NULL);

	if (state->fp) {
		(void) fclose(state->fp);
		state->fp = NULL;
	}
	return NS_SUCCESS;
}

/*
 * _files_grscan
 *	Scan state->fp for the next desired entry.
 *	If search is zero, return the next entry.
 *	If search is non-zero, look for a specific name (if name != NULL),
 *	or a specific gid (if name == NULL).
 *	Sets *retval to the errno if the result is not NS_SUCCESS.
 */
static int
_files_grscan(int *retval, struct group *grp, char *buffer, size_t buflen,
	struct files_state *state, int search, const char *name, gid_t gid)
{
	int	rv;
	char	filebuf[GETGR_R_SIZE_MAX], *ep;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(state != NULL);
	/* name is NULL to indicate searching for gid */

	*retval = 0;

	if (state->fp == NULL) {	/* only start if file not open yet */
		rv = _files_start(state);
		if (rv != NS_SUCCESS)
			goto filesgrscan_out;
	}

	rv = NS_NOTFOUND;

							/* scan line by line */
	while (fgets(filebuf, sizeof(filebuf), state->fp) != NULL) {
		ep = strchr(filebuf, '\n');
		if (ep == NULL) {	/* fail on lines that are too big */
			int ch;

			while ((ch = getc(state->fp)) != '\n' && ch != EOF)
				continue;
			rv = NS_UNAVAIL;
			break;
		}
		*ep = '\0';				/* clear trailing \n */

		if (filebuf[0] == '+')			/* skip compat line */
			continue;

							/* validate line */
		if (! _gr_parse(filebuf, grp, buffer, buflen)) {
			rv = NS_UNAVAIL;
			break;
		}
		if (! search) {				/* just want this one */
			rv = NS_SUCCESS;
			break;
		}
							/* want specific */
		if ((name && strcmp(name, grp->gr_name) == 0) ||
		    (!name && gid == grp->gr_gid)) {
			rv = NS_SUCCESS;
			break;
		}
	}

 filesgrscan_out:
	if (rv != NS_SUCCESS)
		*retval = errno;
	return rv;
}

/*ARGSUSED*/
static int
_files_setgrent(void *nsrv, void *nscb, va_list ap)
{

	_files_state.stayopen = 0;
	return _files_start(&_files_state);
}

/*ARGSUSED*/
static int
_files_setgroupent(void *nsrv, void *nscb, va_list ap)
{
	int	*retval		= va_arg(ap, int *);
	int	 stayopen	= va_arg(ap, int);

	int	rv;

	_files_state.stayopen = stayopen;
	rv = _files_start(&_files_state);
	*retval = (rv == NS_SUCCESS);
	return rv;
}

/*ARGSUSED*/
static int
_files_endgrent(void *nsrv, void *nscb, va_list ap)
{

	_files_state.stayopen = 0;
	return _files_end(&_files_state);
}

/*ARGSUSED*/
static int
_files_getgrent(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _files_grscan(&rerror, &_files_group,
	    _files_groupbuf, sizeof(_files_groupbuf),
	    &_files_state, 0, NULL, 0);
	if (rv == NS_SUCCESS)
		*retval = &_files_group;
	return rv;
}

/*ARGSUSED*/
static int
_files_getgrgid(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	gid_t		 gid	= va_arg(ap, gid_t);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _files_start(&_files_state);
	if (rv != NS_SUCCESS)
		return rv;
	rv = _files_grscan(&rerror, &_files_group,
	    _files_groupbuf, sizeof(_files_groupbuf),
	    &_files_state, 1, NULL, gid);
	if (!_files_state.stayopen)
		_files_end(&_files_state);
	if (rv == NS_SUCCESS)
		*retval = &_files_group;
	return rv;
}

/*ARGSUSED*/
static int
_files_getgrgid_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	gid_t		 gid	= va_arg(ap, gid_t);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct files_state state;
	int	rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	rv = _files_grscan(retval, grp, buffer, buflen, &state, 1, NULL, gid);
	_files_end(&state);
	if (rv == NS_SUCCESS)
		*result = grp;
	return rv;
}

/*ARGSUSED*/
static int
_files_getgrnam(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	const char	*name	= va_arg(ap, const char *);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _files_start(&_files_state);
	if (rv != NS_SUCCESS)
		return rv;
	rv = _files_grscan(&rerror, &_files_group,
	    _files_groupbuf, sizeof(_files_groupbuf),
	    &_files_state, 1, name, 0);
	if (!_files_state.stayopen)
		_files_end(&_files_state);
	if (rv == NS_SUCCESS)
		*retval = &_files_group;
	return rv;
}

/*ARGSUSED*/
static int
_files_getgrnam_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	const char	*name	= va_arg(ap, const char *);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct files_state state;
	int	rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	rv = _files_grscan(retval, grp, buffer, buflen, &state, 1, name, 0);
	_files_end(&state);
	if (rv == NS_SUCCESS)
		*result = grp;
	return rv;
}


#ifdef HESIOD
		/*
		 *	dns methods
		 */

	/* state shared between dns methods */
struct dns_state {
	int	 stayopen;		/* see getgroupent(3) */
	void	*context;		/* Hesiod context */
	int	 num;			/* group index, -1 if no more */
};

static struct dns_state		_dns_state;
					/* storage for non _r functions */
static struct group		_dns_group;
static char			_dns_groupbuf[GETGR_R_SIZE_MAX];

static int
_dns_start(struct dns_state *state)
{

	_DIAGASSERT(state != NULL);

	state->num = 0;
	if (state->context == NULL) {			/* setup Hesiod */
		if (hesiod_init(&state->context) == -1)
			return NS_UNAVAIL;
	}

	return NS_SUCCESS;
}

static int
_dns_end(struct dns_state *state)
{

	_DIAGASSERT(state != NULL);

	state->num = 0;
	if (state->context) {
		hesiod_end(state->context);
		state->context = NULL;
	}
	return NS_SUCCESS;
}

/*
 * _dns_grscan
 *	Look for the Hesiod name provided in buffer in the NULL-terminated
 *	list of zones,
 *	and decode into grp/buffer/buflen.
 */
static int
_dns_grscan(int *retval, struct group *grp, char *buffer, size_t buflen,
	struct dns_state *state, const char **zones)
{
	const char	**curzone;
	char		**hp, *ep;
	int		rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(state != NULL);
	_DIAGASSERT(zones != NULL);

	*retval = 0;

	if (state->context == NULL) {	/* only start if Hesiod not setup */
		rv = _dns_start(state);
		if (rv != NS_SUCCESS)
			return rv;
	}

	hp = NULL;
	rv = NS_NOTFOUND;

	for (curzone = zones; *curzone; curzone++) {	/* search zones */
		hp = hesiod_resolve(state->context, buffer, *curzone);
		if (hp != NULL)
			break;
		if (errno != ENOENT) {
			rv = NS_UNAVAIL;
			goto dnsgrscan_out;
		}
	}
	if (*curzone == NULL)
		goto dnsgrscan_out;

	if ((ep = strchr(hp[0], '\n')) != NULL)
		*ep = '\0';				/* clear trailing \n */
	if (_gr_parse(hp[0], grp, buffer, buflen))	/* validate line */
		rv = NS_SUCCESS;
	else
		rv = NS_UNAVAIL;

 dnsgrscan_out:
	if (rv != NS_SUCCESS)
		*retval = errno;
	if (hp)
		hesiod_free_list(state->context, hp);
	return rv;
}

/*ARGSUSED*/
static int
_dns_setgrent(void *nsrv, void *nscb, va_list ap)
{

	_dns_state.stayopen = 0;
	return _dns_start(&_dns_state);
}

/*ARGSUSED*/
static int
_dns_setgroupent(void *nsrv, void *nscb, va_list ap)
{
	int	*retval		= va_arg(ap, int *);
	int	 stayopen	= va_arg(ap, int);

	int	rv;

	_dns_state.stayopen = stayopen;
	rv = _dns_start(&_dns_state);
	*retval = (rv == NS_SUCCESS);
	return rv;
}

/*ARGSUSED*/
static int
_dns_endgrent(void *nsrv, void *nscb, va_list ap)
{

	_dns_state.stayopen = 0;
	return _dns_end(&_dns_state);
}

/*ARGSUSED*/
static int
_dns_getgrent(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);

	char	**hp, *ep;
	int	  rv;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;

	if (_dns_state.num == -1)			/* exhausted search */
		return NS_NOTFOUND;

	if (_dns_state.context == NULL) {
			/* only start if Hesiod not setup */
		rv = _dns_start(&_dns_state);
		if (rv != NS_SUCCESS)
			return rv;
	}

	hp = NULL;
	rv = NS_NOTFOUND;

							/* find group-NNN */
	snprintf(_dns_groupbuf, sizeof(_dns_groupbuf),
	    "group-%u", _dns_state.num);
	_dns_state.num++;

	hp = hesiod_resolve(_dns_state.context, _dns_groupbuf, "group");
	if (hp == NULL) {
		if (errno == ENOENT)
			_dns_state.num = -1;
		else
			rv = NS_UNAVAIL;
	} else {
		if ((ep = strchr(hp[0], '\n')) != NULL)
			*ep = '\0';			/* clear trailing \n */
							/* validate line */
		if (_gr_parse(hp[0], &_dns_group,
		    _dns_groupbuf, sizeof(_dns_groupbuf)))
			rv = NS_SUCCESS;
		else
			rv = NS_UNAVAIL;
	}

	if (hp)
		hesiod_free_list(_dns_state.context, hp);
	if (rv == NS_SUCCESS)
		*retval = &_dns_group;
	return rv;
}

static const char *_dns_gid_zones[] = {
	"gid",
	"group",
	NULL
};

/*ARGSUSED*/
static int
_dns_getgrgid(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	gid_t		 gid	= va_arg(ap, gid_t);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _dns_start(&_dns_state);
	if (rv != NS_SUCCESS)
		return rv;
	snprintf(_dns_groupbuf, sizeof(_dns_groupbuf), "%u", (unsigned int)gid);
	rv = _dns_grscan(&rerror, &_dns_group,
	    _dns_groupbuf, sizeof(_dns_groupbuf), &_dns_state, _dns_gid_zones);
	if (!_dns_state.stayopen)
		_dns_end(&_dns_state);
	if (rv == NS_SUCCESS && gid == _dns_group.gr_gid)
		*retval = &_dns_group;
	return rv;
}

/*ARGSUSED*/
static int
_dns_getgrgid_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	gid_t		 gid	= va_arg(ap, gid_t);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct dns_state state;
	int	rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	snprintf(buffer, buflen, "%u", (unsigned int)gid);
	rv = _dns_grscan(retval, grp, buffer, buflen, &state, _dns_gid_zones);
	_dns_end(&state);
	if (rv != NS_SUCCESS)
		return rv;
	if (gid == grp->gr_gid) {
		*result = grp;
		return NS_SUCCESS;
	} else
		return NS_NOTFOUND;
}

static const char *_dns_nam_zones[] = {
	"group",
	NULL
};

/*ARGSUSED*/
static int
_dns_getgrnam(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	const char	*name	= va_arg(ap, const char *);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _dns_start(&_dns_state);
	if (rv != NS_SUCCESS)
		return rv;
	snprintf(_dns_groupbuf, sizeof(_dns_groupbuf), "%s", name);
	rv = _dns_grscan(&rerror, &_dns_group,
	    _dns_groupbuf, sizeof(_dns_groupbuf), &_dns_state, _dns_nam_zones);
	if (!_dns_state.stayopen)
		_dns_end(&_dns_state);
	if (rv == NS_SUCCESS && strcmp(name, _dns_group.gr_name) == 0)
		*retval = &_dns_group;
	return rv;
}

/*ARGSUSED*/
static int
_dns_getgrnam_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	const char	*name	= va_arg(ap, const char *);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct dns_state state;
	int	rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	snprintf(buffer, buflen, "%s", name);
	rv = _dns_grscan(retval, grp, buffer, buflen, &state, _dns_nam_zones);
	_dns_end(&state);
	if (rv != NS_SUCCESS)
		return rv;
	if (strcmp(name, grp->gr_name) == 0) {
		*result = grp;
		return NS_SUCCESS;
	} else
		return NS_NOTFOUND;
}

#endif /* HESIOD */


#ifdef YP
		/*
		 *	nis methods
		 */
	/* state shared between nis methods */
struct nis_state {
	int	 stayopen;		/* see getgroupent(3) */
	char	*domain;		/* NIS domain */
	int	 done;			/* non-zero if search exhausted */
	char	*current;		/* current first/next match */
	int	 currentlen;		/* length of _nis_current */
};

static struct nis_state		_nis_state;
					/* storage for non _r functions */
static struct group		_nis_group;
static char			_nis_groupbuf[GETGR_R_SIZE_MAX];

static int
_nis_start(struct nis_state *state)
{

	_DIAGASSERT(state != NULL);

	state->done = 0;
	if (state->current) {
		free(state->current);
		state->current = NULL;
	}
	if (state->domain == NULL) {			/* setup NIS */
		switch (yp_get_default_domain(&state->domain)) {
		case 0:
			break;
		case YPERR_RESRC:
			return NS_TRYAGAIN;
		default:
			return NS_UNAVAIL;
		}
	}
	return NS_SUCCESS;
}

static int
_nis_end(struct nis_state *state)
{

	_DIAGASSERT(state != NULL);

	if (state->domain) {
		state->domain = NULL;
	}
	state->done = 0;
	if (state->current) {
		free(state->current);
		state->current = NULL;
	}
	return NS_SUCCESS;
}

/*
 * _nis_grscan
 *	Look for the yp key provided in buffer from map,
 *	and decode into grp/buffer/buflen.
 */
static int
_nis_grscan(int *retval, struct group *grp, char *buffer, size_t buflen,
	struct nis_state *state, const char *map)
{
	char	*data;
	int	nisr, rv, datalen;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(state != NULL);
	_DIAGASSERT(map != NULL);

	*retval = 0;

	if (state->domain == NULL) {	/* only start if NIS not setup */
		rv = _nis_start(state);
		if (rv != NS_SUCCESS)
			return rv;
	}

	data = NULL;
	rv = NS_NOTFOUND;

							/* search map */
	nisr = yp_match(state->domain, map, buffer, (int)strlen(buffer),
	    &data, &datalen);
	switch (nisr) {
	case 0:
		data[datalen] = '\0';			/* clear trailing \n */
		if (_gr_parse(data, grp, buffer, buflen))
			rv = NS_SUCCESS;		/* validate line */
		else
			rv = NS_UNAVAIL;
		break;
	case YPERR_KEY:
		break;
	default:
		rv = NS_UNAVAIL;
		break;
	}

	if (rv != NS_SUCCESS)
		*retval = errno;
	if (data)
		free(data);
	return rv;
}

/*ARGSUSED*/
static int
_nis_setgrent(void *nsrv, void *nscb, va_list ap)
{

	_nis_state.stayopen = 0;
	return _nis_start(&_nis_state);
}

/*ARGSUSED*/
static int
_nis_setgroupent(void *nsrv, void *nscb, va_list ap)
{
	int	*retval		= va_arg(ap, int *);
	int	 stayopen	= va_arg(ap, int);

	int	rv;

	_nis_state.stayopen = stayopen;
	rv = _nis_start(&_nis_state);
	*retval = (rv == NS_SUCCESS);
	return rv;
}

/*ARGSUSED*/
static int
_nis_endgrent(void *nsrv, void *nscb, va_list ap)
{

	return _nis_end(&_nis_state);
}


/*ARGSUSED*/
static int
_nis_getgrent(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);

	char	*key, *data;
	int	keylen, datalen, rv, nisr;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;

	if (_nis_state.done)				/* exhausted search */
		return NS_NOTFOUND;
	if (_nis_state.domain == NULL) {
					/* only start if NIS not setup */
		rv = _nis_start(&_nis_state);
		if (rv != NS_SUCCESS)
			return rv;
	}

	key = NULL;
	data = NULL;
	rv = NS_NOTFOUND;

	if (_nis_state.current) {			/* already searching */
		nisr = yp_next(_nis_state.domain, "group.byname",
		    _nis_state.current, _nis_state.currentlen,
		    &key, &keylen, &data, &datalen);
		free(_nis_state.current);
		_nis_state.current = NULL;
		switch (nisr) {
		case 0:
			_nis_state.current = key;
			_nis_state.currentlen = keylen;
			key = NULL;
			break;
		case YPERR_NOMORE:
			_nis_state.done = 1;
			goto nisent_out;
		default:
			rv = NS_UNAVAIL;
			goto nisent_out;
		}
	} else {					/* new search */
		if (yp_first(_nis_state.domain, "group.byname",
		    &_nis_state.current, &_nis_state.currentlen,
		    &data, &datalen)) {
			rv = NS_UNAVAIL;
			goto nisent_out;
		}
	}

	data[datalen] = '\0';				/* clear trailing \n */
							/* validate line */
	if (_gr_parse(data, &_nis_group, _nis_groupbuf, sizeof(_nis_groupbuf)))
		rv = NS_SUCCESS;
	else
		rv = NS_UNAVAIL;

 nisent_out:
	if (key)
		free(key);
	if (data)
		free(data);
	if (rv == NS_SUCCESS)
		*retval = &_nis_group;
	return rv;
}

/*ARGSUSED*/
static int
_nis_getgrgid(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	gid_t		 gid	= va_arg(ap, gid_t);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _nis_start(&_nis_state);
	if (rv != NS_SUCCESS)
		return rv;
	snprintf(_nis_groupbuf, sizeof(_nis_groupbuf), "%u", (unsigned int)gid);
	rv = _nis_grscan(&rerror, &_nis_group,
	    _nis_groupbuf, sizeof(_nis_groupbuf), &_nis_state, "group.bygid");
	if (!_nis_state.stayopen)
		_nis_end(&_nis_state);
	if (rv == NS_SUCCESS && gid == _nis_group.gr_gid)
		*retval = &_nis_group;
	return rv;
}

/*ARGSUSED*/
static int
_nis_getgrgid_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	gid_t		 gid	= va_arg(ap, gid_t);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct nis_state state;
	int	rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	snprintf(buffer, buflen, "%u", (unsigned int)gid);
	rv = _nis_grscan(retval, grp, buffer, buflen, &state, "group.bygid");
	_nis_end(&state);
	if (rv != NS_SUCCESS)
		return rv;
	if (gid == grp->gr_gid) {
		*result = grp;
		return NS_SUCCESS;
	} else
		return NS_NOTFOUND;
}

/*ARGSUSED*/
static int
_nis_getgrnam(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	const char	*name	= va_arg(ap, const char *);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _nis_start(&_nis_state);
	if (rv != NS_SUCCESS)
		return rv;
	snprintf(_nis_groupbuf, sizeof(_nis_groupbuf), "%s", name);
	rv = _nis_grscan(&rerror, &_nis_group,
	    _nis_groupbuf, sizeof(_nis_groupbuf), &_nis_state, "group.byname");
	if (!_nis_state.stayopen)
		_nis_end(&_nis_state);
	if (rv == NS_SUCCESS && strcmp(name, _nis_group.gr_name) == 0)
		*retval = &_nis_group;
	return rv;
}

/*ARGSUSED*/
static int
_nis_getgrnam_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	const char	*name	= va_arg(ap, const char *);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct nis_state state;
	int	rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	snprintf(buffer, buflen, "%s", name);
	memset(&state, 0, sizeof(state));
	rv = _nis_grscan(retval, grp, buffer, buflen, &state, "group.byname");
	_nis_end(&state);
	if (rv != NS_SUCCESS)
		return rv;
	if (strcmp(name, grp->gr_name) == 0) {
		*result = grp;
		return NS_SUCCESS;
	} else
		return NS_NOTFOUND;
}

#endif /* YP */


#ifdef _GROUP_COMPAT
		/*
		 *	compat methods
		 */

	/* state shared between compat methods */
struct compat_state {
	int	 stayopen;		/* see getgroupent(3) */
	FILE	*fp;			/* file handle */
	char	*name;			/* NULL if reading file, */
					/* "" if compat "+", */
					/* name if compat "+name" */
};

static struct compat_state	_compat_state;
					/* storage for non _r functions */
static struct group		_compat_group;
static char			_compat_groupbuf[GETGR_R_SIZE_MAX];

static int
_compat_start(struct compat_state *state)
{

	_DIAGASSERT(state != NULL);

	if (state->fp == NULL) {
		state->fp = fopen(_PATH_GROUP, "r");
		if (state->fp == NULL)
			return NS_UNAVAIL;
	} else {
		rewind(state->fp);
	}
	return NS_SUCCESS;
}

static int
_compat_end(struct compat_state *state)
{

	_DIAGASSERT(state != NULL);

	if (state->name) {
		free(state->name);
		state->name = NULL;
	}
	if (state->fp) {
		(void) fclose(state->fp);
		state->fp = NULL;
	}
	return NS_SUCCESS;
}


/*
 * _compat_grbad
 *	log an error if "files" or "compat" is specified in
 *	group_compat database
 */
/*ARGSUSED*/
static int
_compat_grbad(void *nsrv, void *nscb, va_list ap)
{
	static int warned;

	_DIAGASSERT(cb_data != NULL);

	if (!warned) {
		syslog(LOG_ERR,
			"nsswitch.conf group_compat database can't use '%s'",
			(char *)nscb);
	}
	warned = 1;
	return NS_UNAVAIL;
}

/*
 * _compat_grscan
 *	Scan state->fp for the next desired entry.
 *	If search is zero, return the next entry.
 *	If search is non-zero, look for a specific name (if name != NULL),
 *	or a specific gid (if name == NULL).
 *	Sets *retval to the errno if the result is not NS_SUCCESS.
 */
static int
_compat_grscan(int *retval, struct group *grp, char *buffer, size_t buflen,
	struct compat_state *state, int search, const char *name, gid_t gid)
{
	int		rv;
	char		filebuf[GETGR_R_SIZE_MAX], *ep;

	static const ns_dtab compatentdtab[] = {
		NS_FILES_CB(_compat_grbad, "files")
		NS_DNS_CB(_dns_getgrent, NULL)
		NS_NIS_CB(_nis_getgrent, NULL)
		NS_COMPAT_CB(_compat_grbad, "compat")
		{ 0 }
	};
	static const ns_dtab compatgiddtab[] = {
		NS_FILES_CB(_compat_grbad, "files")
		NS_DNS_CB(_dns_getgrgid_r, NULL)
		NS_NIS_CB(_nis_getgrgid_r, NULL)
		NS_COMPAT_CB(_compat_grbad, "compat")
		{ 0 }
	};
	static const ns_dtab compatnamdtab[] = {
		NS_FILES_CB(_compat_grbad, "files")
		NS_DNS_CB(_dns_getgrnam_r, NULL)
		NS_NIS_CB(_nis_getgrnam_r, NULL)
		NS_COMPAT_CB(_compat_grbad, "compat")
		{ 0 }
	};

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(state != NULL);
	/* name is NULL to indicate searching for gid */

	*retval = 0;

	if (state->fp == NULL) {	/* only start if file not open yet */
		rv = _compat_start(state);
		if (rv != NS_SUCCESS)
			goto compatgrscan_out;
	}
	rv = NS_NOTFOUND;

	for (;;) {					/* loop through file */
		if (state->name != NULL) {
					/* processing compat entry */
			int		crv, cretval;
			struct group	cgrp, *cgrpres;

			if (state->name[0]) {		/* specific +group: */
				crv = nsdispatch(NULL, compatnamdtab,
				    NSDB_GROUP_COMPAT, "getgrnam_r", defaultnis,
				    &cretval, state->name,
				    &cgrp, filebuf, sizeof(filebuf), &cgrpres);
				free(state->name);	/* (only check 1 grp) */
				state->name = NULL;
			} else if (!search) {		/* any group */
	/* XXXLUKEM: need to implement and use getgrent_r() */
				crv = nsdispatch(NULL, compatentdtab,
				    NSDB_GROUP_COMPAT, "getgrent", defaultnis,
				    &cgrpres);
			} else if (name) {		/* specific group */
				crv = nsdispatch(NULL, compatnamdtab,
				    NSDB_GROUP_COMPAT, "getgrnam_r", defaultnis,
				    &cretval, name,
				    &cgrp, filebuf, sizeof(filebuf), &cgrpres);
			} else {			/* specific gid */
				crv = nsdispatch(NULL, compatgiddtab,
				    NSDB_GROUP_COMPAT, "getgrgid_r", defaultnis,
				    &cretval, gid,
				    &cgrp, filebuf, sizeof(filebuf), &cgrpres);
			}
			if (crv != NS_SUCCESS) {	/* not found */
				free(state->name);
				state->name = NULL;
				continue;		/* try next line */
			}
			if (!_gr_copy(cgrpres, grp, buffer, buflen)) {
				rv = NS_UNAVAIL;
				break;
			}
			goto compatgrscan_cmpgrp;	/* skip to grp test */
		}

							/* get next file line */
		if (fgets(filebuf, sizeof(filebuf), state->fp) == NULL)
			break;

		ep = strchr(filebuf, '\n');
		if (ep == NULL) {	/* fail on lines that are too big */
			int ch;

			while ((ch = getc(state->fp)) != '\n' && ch != EOF)
				continue;
			rv = NS_UNAVAIL;
			break;
		}
		*ep = '\0';				/* clear trailing \n */

		if (filebuf[0] == '+') {		/* parse compat line */
			if (state->name)
				free(state->name);
			state->name = NULL;
			switch(filebuf[1]) {
			case ':':
			case '\0':
				state->name = strdup("");
				break;
			default:
				ep = strchr(filebuf + 1, ':');
				if (ep == NULL)
					break;
				*ep = '\0';
				state->name = strdup(filebuf + 1);
				break;
			}
			if (state->name == NULL) {
				rv = NS_UNAVAIL;
				break;
			}
			continue;
		}

							/* validate line */
		if (! _gr_parse(filebuf, grp, buffer, buflen)) {
			rv = NS_UNAVAIL;
			break;
		}

 compatgrscan_cmpgrp:
		if (! search) {				/* just want this one */
			rv = NS_SUCCESS;
			break;
		}
							/* want specific */
		if ((name && strcmp(name, grp->gr_name) == 0) ||
		    (!name && gid == grp->gr_gid)) {
			rv = NS_SUCCESS;
			break;
		}

	}

 compatgrscan_out:
	if (rv != NS_SUCCESS)
		*retval = errno;
	return rv;
}

/*ARGSUSED*/
static int
_compat_setgrent(void *nsrv, void *nscb, va_list ap)
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_compat_grbad, "files")
		NS_DNS_CB(_dns_setgrent, NULL)
		NS_NIS_CB(_nis_setgrent, NULL)
		NS_COMPAT_CB(_compat_grbad, "compat")
		{ 0 }
	};

					/* force group_compat setgrent() */
	(void) nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "setgrent",
	    defaultnis_forceall);

					/* reset state, keep fp open */
	_compat_state.stayopen = 0;
	return _compat_start(&_compat_state);
}

/*ARGSUSED*/
static int
_compat_setgroupent(void *nsrv, void *nscb, va_list ap)
{
	int	*retval		= va_arg(ap, int *);
	int	 stayopen	= va_arg(ap, int);

	int	rv;

	static const ns_dtab dtab[] = {
		NS_FILES_CB(_compat_grbad, "files")
		NS_DNS_CB(_dns_setgroupent, NULL)
		NS_NIS_CB(_nis_setgroupent, NULL)
		NS_COMPAT_CB(_compat_grbad, "compat")
		{ 0 }
	};

					/* force group_compat setgroupent() */
	(void) nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "setgroupent",
	    defaultnis_forceall, &rv, stayopen);

	_compat_state.stayopen = stayopen;
	rv = _compat_start(&_compat_state);
	*retval = (rv == NS_SUCCESS);
	return rv;
}

/*ARGSUSED*/
static int
_compat_endgrent(void *nsrv, void *nscb, va_list ap)
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_compat_grbad, "files")
		NS_DNS_CB(_dns_endgrent, NULL)
		NS_NIS_CB(_nis_endgrent, NULL)
		NS_COMPAT_CB(_compat_grbad, "compat")
		{ 0 }
	};

					/* force group_compat endgrent() */
	(void) nsdispatch(NULL, dtab, NSDB_GROUP_COMPAT, "endgrent",
	    defaultnis_forceall);

					/* reset state, close fp */
	_compat_state.stayopen = 0;
	return _compat_end(&_compat_state);
}

/*ARGSUSED*/
static int
_compat_getgrent(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _compat_grscan(&rerror, &_compat_group,
	    _compat_groupbuf, sizeof(_compat_groupbuf),
	    &_compat_state, 0, NULL, 0);
	if (rv == NS_SUCCESS)
		*retval = &_compat_group;
	return rv;
}

/*ARGSUSED*/
static int
_compat_getgrgid(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	gid_t		 gid	= va_arg(ap, gid_t);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _compat_start(&_compat_state);
	if (rv != NS_SUCCESS)
		return rv;
	rv = _compat_grscan(&rerror, &_compat_group,
	    _compat_groupbuf, sizeof(_compat_groupbuf),
	    &_compat_state, 1, NULL, gid);
	if (!_compat_state.stayopen)
		_compat_end(&_compat_state);
	if (rv == NS_SUCCESS)
		*retval = &_compat_group;
	return rv;
}

/*ARGSUSED*/
static int
_compat_getgrgid_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	gid_t		 gid	= va_arg(ap, gid_t);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct compat_state	state;
	int		rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	rv = _compat_grscan(retval, grp, buffer, buflen, &state, 1, NULL, gid);
	_compat_end(&state);
	if (rv == NS_SUCCESS)
		*result = grp;
	return rv;
}

/*ARGSUSED*/
static int
_compat_getgrnam(void *nsrv, void *nscb, va_list ap)
{
	struct group	**retval = va_arg(ap, struct group **);
	const char	*name	= va_arg(ap, const char *);

	int	rv, rerror;

	_DIAGASSERT(retval != NULL);

	*retval = NULL;
	rv = _compat_start(&_compat_state);
	if (rv != NS_SUCCESS)
		return rv;
	rv = _compat_grscan(&rerror, &_compat_group,
	    _compat_groupbuf, sizeof(_compat_groupbuf),
	    &_compat_state, 1, name, 0);
	if (!_compat_state.stayopen)
		_compat_end(&_compat_state);
	if (rv == NS_SUCCESS)
		*retval = &_compat_group;
	return rv;
}

/*ARGSUSED*/
static int
_compat_getgrnam_r(void *nsrv, void *nscb, va_list ap)
{
	int		*retval	= va_arg(ap, int *);
	const char	*name	= va_arg(ap, const char *);
	struct group	*grp	= va_arg(ap, struct group *);
	char		*buffer	= va_arg(ap, char *);
	size_t		 buflen	= va_arg(ap, size_t);
	struct group   **result	= va_arg(ap, struct group **);

	struct compat_state	state;
	int		rv;

	_DIAGASSERT(retval != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	memset(&state, 0, sizeof(state));
	rv = _compat_grscan(retval, grp, buffer, buflen, &state, 1, name, 0);
	_compat_end(&state);
	if (rv == NS_SUCCESS)
		*result = grp;
	return rv;
}

#endif	/* _GROUP_COMPAT */


		/*
		 *	public functions
		 */

struct group *
getgrent(void)
{
	int		rv;
	struct group	*retval;

	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getgrent, NULL)
		NS_DNS_CB(_dns_getgrent, NULL)
		NS_NIS_CB(_nis_getgrent, NULL)
		NS_COMPAT_CB(_compat_getgrent, NULL)
		{ 0 }
	};

	mutex_lock(&_grmutex);
	rv = nsdispatch(NULL, dtab, NSDB_GROUP, "getgrent", defaultcompat,
	    &retval);
	mutex_unlock(&_grmutex);
	return (rv == NS_SUCCESS) ? retval : NULL;
}

struct group *
getgrgid(gid_t gid)
{
	int		rv;
	struct group	*retval;

	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getgrgid, NULL)
		NS_DNS_CB(_dns_getgrgid, NULL)
		NS_NIS_CB(_nis_getgrgid, NULL)
		NS_COMPAT_CB(_compat_getgrgid, NULL)
		{ 0 }
	};

	mutex_lock(&_grmutex);
	rv = nsdispatch(NULL, dtab, NSDB_GROUP, "getgrgid", defaultcompat,
	    &retval, gid);
	mutex_unlock(&_grmutex);
	return (rv == NS_SUCCESS) ? retval : NULL;
}

int
getgrgid_r(gid_t gid, struct group *grp, char *buffer, size_t buflen,
	struct group **result)
{
	int	rv, retval;

	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getgrgid_r, NULL)
		NS_DNS_CB(_dns_getgrgid_r, NULL)
		NS_NIS_CB(_nis_getgrgid_r, NULL)
		NS_COMPAT_CB(_compat_getgrgid_r, NULL)
		{ 0 }
	};

	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	retval = 0;
	mutex_lock(&_grmutex);
	rv = nsdispatch(NULL, dtab, NSDB_GROUP, "getgrgid_r", defaultcompat,
	    &retval, gid, grp, buffer, buflen, result);
	mutex_unlock(&_grmutex);
	return (rv == NS_SUCCESS) ? 0 : retval ? retval : ENOENT;
}

struct group *
getgrnam(const char *name)
{
	int		rv;
	struct group	*retval;

	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getgrnam, NULL)
		NS_DNS_CB(_dns_getgrnam, NULL)
		NS_NIS_CB(_nis_getgrnam, NULL)
		NS_COMPAT_CB(_compat_getgrnam, NULL)
		{ 0 }
	};

	mutex_lock(&_grmutex);
	rv = nsdispatch(NULL, dtab, NSDB_GROUP, "getgrnam", defaultcompat,
	    &retval, name);
	mutex_unlock(&_grmutex);
	return (rv == NS_SUCCESS) ? retval : NULL;
}

int
getgrnam_r(const char *name, struct group *grp, char *buffer, size_t buflen,
	struct group **result)
{
	int	rv, retval;

	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_getgrnam_r, NULL)
		NS_DNS_CB(_dns_getgrnam_r, NULL)
		NS_NIS_CB(_nis_getgrnam_r, NULL)
		NS_COMPAT_CB(_compat_getgrnam_r, NULL)
		{ 0 }
	};

	_DIAGASSERT(name != NULL);
	_DIAGASSERT(grp != NULL);
	_DIAGASSERT(buffer != NULL);
	_DIAGASSERT(result != NULL);

	*result = NULL;
	retval = 0;
	mutex_lock(&_grmutex);
	rv = nsdispatch(NULL, dtab, NSDB_GROUP, "getgrnam_r", defaultcompat,
	    &retval, name, grp, buffer, buflen, result);
	mutex_unlock(&_grmutex);
	return (rv == NS_SUCCESS) ? 0 : retval ? retval : ENOENT;
}

void
endgrent(void)
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_endgrent, NULL)
		NS_DNS_CB(_dns_endgrent, NULL)
		NS_NIS_CB(_nis_endgrent, NULL)
		NS_COMPAT_CB(_compat_endgrent, NULL)
		{ 0 }
	};

	mutex_lock(&_grmutex);
					/* force all endgrent() methods */
	(void) nsdispatch(NULL, dtab, NSDB_GROUP, "endgrent",
	    defaultcompat_forceall);
	mutex_unlock(&_grmutex);
}

int
setgroupent(int stayopen)
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_setgroupent, NULL)
		NS_DNS_CB(_dns_setgroupent, NULL)
		NS_NIS_CB(_nis_setgroupent, NULL)
		NS_COMPAT_CB(_compat_setgroupent, NULL)
		{ 0 }
	};
	int	rv, retval;

	mutex_lock(&_grmutex);
					/* force all setgroupent() methods */
	rv = nsdispatch(NULL, dtab, NSDB_GROUP, "setgroupent",
	    defaultcompat_forceall, &retval, stayopen);
	mutex_unlock(&_grmutex);
	return (rv == NS_SUCCESS) ? retval : 0;
}

void
setgrent(void)
{
	static const ns_dtab dtab[] = {
		NS_FILES_CB(_files_setgrent, NULL)
		NS_DNS_CB(_dns_setgrent, NULL)
		NS_NIS_CB(_nis_setgrent, NULL)
		NS_COMPAT_CB(_compat_setgrent, NULL)
		{ 0 }
	};

	mutex_lock(&_grmutex);
					/* force all setgrent() methods */
	(void) nsdispatch(NULL, dtab, NSDB_GROUP, "setgrent",
	    defaultcompat_forceall);
	mutex_unlock(&_grmutex);
}
