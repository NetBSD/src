/*	$NetBSD: acl.c,v 1.5 1999/01/18 23:42:38 lukem Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: acl.c,v 1.5 1999/01/18 23:42:38 lukem Exp $");
#endif

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>

#include "ypdef.h"
#include "acl.h"

struct aclent {
	TAILQ_ENTRY(aclent) list;
	aclallow_t	allow;
	u_int32_t	addr;
	u_int32_t	mask;
};

/* The Access Control List. */
TAILQ_HEAD(, aclent)	ac_list;

int	acl_securenet_has_entries;

void	acl_translate __P((const char *, acladdr_t, u_int32_t *));
void	acl_securenet_parse __P((void));

/*
 * Zap the access control list.
 */
void
acl_reset()
{
	struct aclent *p, *q;

	for (p = ac_list.tqh_first; p != NULL; ) {
		q = p;
		p = p->list.tqe_next;
		TAILQ_REMOVE(&ac_list, q, list);
		free(q);
	}

	TAILQ_INIT(&ac_list);
}

/*
 * Check if a host is allowed access.
 */
int
acl_check_host(addr)
	struct in_addr *addr;
{
	struct aclent *p;

	for (p = ac_list.tqh_first; p != NULL; p = p->list.tqe_next)
		if ((addr->s_addr & p->mask) == p->addr)
			return (p->allow);

	return (ACL_ALLOW);
}

/*
 * Add an entry to the in-core access control list.
 */
void
acl_add(addr, mask, atype, allow)
	const char *addr, *mask;
	acladdr_t atype;
	aclallow_t allow;
{
	struct aclent *acl;

	acl = (struct aclent *)malloc(sizeof(struct aclent));
	if (acl == NULL)
		err(1, "can't allocate ACL entry");

	acl->allow = allow;
	acl_translate(addr, atype, &acl->addr);

	/*
	 * If we get a NULL mask, it means we want the
	 * default mask for the address class.
	 */
	if (mask == NULL) {
		NTOHL(acl->addr);
		if (IN_CLASSA(acl->addr))
			acl->mask = IN_CLASSA_NET;
		else if (IN_CLASSB(acl->addr))
			acl->mask = IN_CLASSB_NET;
		else if (IN_CLASSC(acl->addr))
			acl->mask = IN_CLASSC_NET;
		else
			errx(1, "line %d: invalid network class `%s'",
			    acl_line(), addr);
		HTONL(acl->addr);
		HTONL(acl->mask);
	} else
		acl_translate(mask, atype, &acl->mask);

	TAILQ_INSERT_TAIL(&ac_list, acl, list);
}

/*
 * Parse the access control list.  If we're given a name,
 * we have an ACL file.  If we're not, we have a SECURENET file.
 */
void
acl_parse(fname)
	const char *fname;
{

	TAILQ_INIT(&ac_list);

	/*
	 * Check SECURENET first.
	 */
	if (fname == NULL) {
		/* Parse the SECURENET file. */
		acl_securenet_parse();

		/*
		 * Since the purpose of SECURENET is to explicitly
		 * list which networks are allowed to access the YP
		 * server, do a catch-all `deny all' for anyone who
		 * wasn't in the file unless the file contained no
		 * entries (e.g. was a comments-only example file),
		 * in which case we do an `allow all'.
		 */
		if (acl_securenet_has_entries)
			acl_add(ACL_ALL, ACL_ALL, ACLADDR_HOST, ACL_DENY);
		else
			acl_add(ACL_ALL, ACL_ALL, ACLADDR_HOST, ACL_ALLOW);

		return;
	}

	/*
	 * We are dealing with an ACL file.  Since we were
	 * passed the name of this file, if it doesn't exist,
	 * it's a fatal error.
	 */
	if (acl_open(fname))
		err(1, "can't open ACL file `%s'", fname);

	/* Parse the ACL file. */
	yyparse();
	acl_close();

	/*
	 * Always add a last `allow all' if the file doesn't cover
	 * all cases.  If the file specified a `deny all' at the end,
	 * it will match before this one, so this is always safe to do.
	 */
	acl_add(ACL_ALL, ACL_ALL, ACLADDR_HOST, ACL_ALLOW);
}

/*
 * Parse the securenet file; it's a really simple format.
 */
void
acl_securenet_parse()
{
	FILE *f;
	char line[_POSIX2_LINE_MAX];
	char *cp, *addr, *mask;
	int ntok;
	extern int yyline, yychar;

	/*
	 * No SECURENET file?  Just return; the Right Thing
	 * will happen.
	 */
	if ((f = fopen(YP_SECURENET_FILE, "r")) == NULL)
		return;

	/* For simplictity in acl_add(). */
	yychar = yyline = 0;

	while (fgets(line, sizeof(line), f) != NULL) {
		++yyline;
		addr = mask = NULL;

		/* Chop off trailing newline. */
		if ((cp = strrchr(line, '\n')) != NULL)
			*cp = '\0';

		/* Handle blank lines. */
		if (line[0] == '\0')
			continue;

		/* Break line into tokens. */
		for (ntok = 0, cp = line;
		    (cp = strtok(cp, " \t")) != NULL; cp = NULL) {
			/* Handle comments. */
			if (*cp == '#')
				break;

			/* Assign token. */
			switch (++ntok) {
			case 1:
				mask = cp;
				break;

			case 2:
				addr = cp;
				break;

			default:
				errx(1, "line %d: syntax error", yyline);
			}
		}

		/* Add the entry to the list. */
		if (addr != NULL && mask != NULL) {
			acl_add(addr, mask, ACLADDR_NET, ACL_ALLOW);
			/*
			 * Sanity check against an empty (e.g. example-only)
			 * SECURENET file.
			 */
			acl_securenet_has_entries = 1;
		} else if (mask != NULL && addr == NULL)
			errx(1, "line %d: syntax error", yyline);
	}

	/* All done. */
	(void)fclose(f);
}

/*
 * Given a string containing one of:
 *
 *	- IP address
 *
 *	- Host name
 *
 *	- Net name
 *
 * fill in the address or mask as appropriate.
 */
void
acl_translate(str, atype, res)
	const char *str;
	acladdr_t atype;
	u_int32_t *res;
{
	struct hostent *hp;
	struct netent *np;
	struct in_addr ina;

	/*
	 * Were we passed an IP address?  Note, we _reject_
	 * invalid host names with this!
	 */
	if (isdigit(str[0])) {
		if (inet_aton(str, &ina)) {
			*res = ina.s_addr;
			return;
		} else
			errx(1, "invalid IP address `%s' at line %d",
			    str, acl_line());
	}

	/*
	 * Host names - look it up, and use the first address.
	 */
	if (atype == ACLADDR_HOST) {
		hp = gethostbyname(str);
		if (hp == NULL)
			errx(1, "unknown host `%s' at line %d",
			    str, acl_line());

		/* Sanity. */
		if (hp->h_addrtype != AF_INET)
			errx(1, "host `%s' at line %d: not INET?!",
			    str, acl_line());
		if (hp->h_length != sizeof(u_int32_t))
			errx(1,
			    "address for host `%s' at line %d: wrong size?!",
			    str, acl_line());

		memcpy(res, hp->h_addr_list[0], hp->h_length);
		return;
	}

	/*
	 * We have a networks entry.
	 */
	np = getnetbyname(str);
	if (np == NULL)
		errx(1, "unknown net `%s' at line %d",
		    str, acl_line());

	/* Sanity. */
	if (np->n_addrtype != AF_INET)
		errx(1, "net `%s' at line %d: not INET?!",
		    str, acl_line());

	*res = htonl(np->n_net);
}
