/*	$NetBSD: sysctl.c,v 1.16 2004/03/24 15:34:56 atatat Exp $	*/

/*-
 * Copyright (c) 1993
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
static char sccsid[] = "@(#)sysctl.c	8.2 (Berkeley) 1/4/94";
#else
__RCSID("$NetBSD: sysctl.c,v 1.16 2004/03/24 15:34:56 atatat Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"
#include <sys/param.h>
#include <sys/sysctl.h>

#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "extern.h"

#ifdef __weak_alias
__weak_alias(sysctl,_sysctl)
#endif

/*
 * handles requests off the user subtree
 */
static int user_sysctl(int *, u_int, void *, size_t *, const void *, size_t);

#include <stdlib.h>

int
sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	unsigned int namelen;
	void *oldp;
	const void *newp;
	size_t *oldlenp, newlen;
{
	size_t oldlen, savelen;
	int error;

	if (getenv("SYSCTL_TRACE") != NULL) {
		char trb[128];
		uint l, i;

		l = snprintf(trb, sizeof(trb), "sysctl(%p {", name);
		write(2, &trb[0], l);
		for (i = 0; i < namelen; i++) {
			l = snprintf(trb, sizeof(trb), "%s%d", i ? "." : "",
				     name[i]);
			write(2, &trb[0], l);
		}
		l = snprintf(trb, sizeof(trb), "}, %d, %p, %p (%lu), %p, %lu)\n",
			     namelen,
			     oldp,
			     oldlenp, (ulong)(oldlenp ? *oldlenp : 0),
			     newp,
			     (ulong)newlen);
		write(2, &trb[0], l);
	}

	if (name[0] != CTL_USER)
		/* LINTED will fix when sysctl interface gets corrected */
		/* XXX when will that be? */
		return (__sysctl(name, namelen, oldp, oldlenp,
				 (void *)newp, newlen));

	oldlen = (oldlenp == NULL) ? 0 : *oldlenp;
	savelen = oldlen;
	error = user_sysctl(name + 1, namelen - 1, oldp, &oldlen, newp, newlen);

	if (error != 0) {
		errno = error;
		return (-1);
	}

	if (oldlenp != NULL) {
		*oldlenp = oldlen;
		if (oldp != NULL && oldlen > savelen) {
			errno = ENOMEM;
			return (-1);
		}
	}

	return (0);
}

static int
user_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	unsigned int namelen;
	void *oldp;
	const void *newp;
	size_t *oldlenp, newlen;
{
#define _INT(s, n, v) {						\
	.sysctl_flags = CTLFLAG_IMMEDIATE|CTLFLAG_PERMANENT|	\
			CTLTYPE_INT|SYSCTL_VERSION,		\
	.sysctl_size = sizeof(int),				\
	.sysctl_name = (s),					\
	.sysctl_num = (n),					\
	.sysctl_un = { .scu_idata = (v), }, }

	/*
	 * the nodes under the "user" node
	 */
	static const struct sysctlnode sysctl_usermib[] = {
#if defined(lint)
		/*
		 * lint doesn't like my initializers
		 */
		0
#else /* !lint */
		{
			.sysctl_flags = SYSCTL_VERSION|CTLFLAG_PERMANENT|
				CTLTYPE_STRING,
			.sysctl_size = sizeof(_PATH_STDPATH),
			.sysctl_name = "cs_path",
			.sysctl_num = USER_CS_PATH,
			.sysctl_un = { .scu_data = _PATH_STDPATH, },
		},
		_INT("bc_base_max", USER_BC_BASE_MAX, BC_BASE_MAX),
		_INT("bc_dim_max", USER_BC_DIM_MAX, BC_DIM_MAX),
		_INT("bc_scale_max", USER_BC_SCALE_MAX, BC_SCALE_MAX),
		_INT("bc_string_max", USER_BC_STRING_MAX, BC_STRING_MAX),
		_INT("coll_weights_max", USER_COLL_WEIGHTS_MAX,
		     COLL_WEIGHTS_MAX),
		_INT("expr_nest_max", USER_EXPR_NEST_MAX, EXPR_NEST_MAX),
		_INT("line_max", USER_LINE_MAX, LINE_MAX),
		_INT("re_dup_max", USER_RE_DUP_MAX, RE_DUP_MAX),
		_INT("posix2_version", USER_POSIX2_VERSION, _POSIX2_VERSION),
#ifdef POSIX2_C_BIND
		_INT("posix2_c_bind", USER_POSIX2_C_BIND, 1),
#else
		_INT("posix2_c_bind", USER_POSIX2_C_BIND, 0),
#endif
#ifdef POSIX2_C_DEV
		_INT("posix2_c_dev", USER_POSIX2_C_DEV, 1),
#else
		_INT("posix2_c_dev", USER_POSIX2_C_DEV, 0),
#endif
#ifdef POSIX2_CHAR_TERM
		_INT("posix2_char_term", USER_POSIX2_CHAR_TERM, 1),
#else
		_INT("posix2_char_term", USER_POSIX2_CHAR_TERM, 0),
#endif
#ifdef POSIX2_FORT_DEV
		_INT("posix2_fort_dev", USER_POSIX2_FORT_DEV, 1),
#else
		_INT("posix2_fort_dev", USER_POSIX2_FORT_DEV, 0),
#endif
#ifdef POSIX2_FORT_RUN
		_INT("posix2_fort_run", USER_POSIX2_FORT_RUN, 1),
#else
		_INT("posix2_fort_run", USER_POSIX2_FORT_RUN, 0),
#endif
#ifdef POSIX2_LOCALEDEF
		_INT("posix2_localedef", USER_POSIX2_LOCALEDEF, 1),
#else
		_INT("posix2_localedef", USER_POSIX2_LOCALEDEF, 0),
#endif
#ifdef POSIX2_SW_DEV
		_INT("posix2_sw_dev", USER_POSIX2_SW_DEV, 1),
#else
		_INT("posix2_sw_dev", USER_POSIX2_SW_DEV, 0),
#endif
#ifdef POSIX2_UPE
		_INT("posix2_upe", USER_POSIX2_UPE, 1),
#else
		_INT("posix2_upe", USER_POSIX2_UPE, 0),
#endif
		_INT("stream_max", USER_STREAM_MAX, FOPEN_MAX),
		_INT("tzname_max", USER_TZNAME_MAX, NAME_MAX),
		_INT("atexit_max", USER_ATEXIT_MAX, -1),
#endif /* !lint */
	};
#undef _INT

	static const int clen = sizeof(sysctl_usermib) /
		sizeof(sysctl_usermib[0]);

	const struct sysctlnode *node;
	int ni;
	size_t l, sz;

	/*
	 * none of these nodes are writable and they're all terminal (for now)
	 */
	if (newp != NULL || newlen != 0)
		return (EPERM);
	if (namelen != 1)
		return (EINVAL);

	l = *oldlenp;
	if (name[0] == CTL_QUERY) {
		sz = clen * sizeof(struct sysctlnode);
		l = MIN(l, sz);
		if (oldp != NULL)
			memcpy(oldp, &sysctl_usermib[0], l);
		*oldlenp = sz;
		return (0);
	}
	
	node = &sysctl_usermib[0];
	for (ni = 0; ni	< clen; ni++)
		if (name[0] == node[ni].sysctl_num)
			break;
	if (ni == clen)
		return (EOPNOTSUPP);

	node = &node[ni];
	if (node->sysctl_flags & CTLFLAG_IMMEDIATE) {
		switch (SYSCTL_TYPE(node->sysctl_flags)) {
		case CTLTYPE_INT:
			newp = &node->sysctl_idata;
			break;
		case CTLTYPE_QUAD:
			newp = &node->sysctl_qdata;
			break;
		default:
			return (EINVAL);
		}
	}
	else
		newp = node->sysctl_data;

	l = MIN(l, node->sysctl_size);
	if (oldp != NULL)
		memcpy(oldp, newp, l);
	*oldlenp = node->sysctl_size;

	return (0);
}
