/*	$NetBSD: getent.c,v 1.2 2004/11/26 04:52:45 lukem Exp $	*/

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

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: getent.c,v 1.2 2004/11/26 04:52:45 lukem Exp $");
#endif /* not lint */

#include <sys/socket.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <netinet/in.h>		/* for INET6_ADDRSTRLEN */

static int	usage(void);
static int	parsenum(const char *, unsigned long *);
static int	group(int, char *[]);
static int	hosts(int, char *[]);
static int	networks(int, char *[]);
static int	passwd(int, char *[]);
static int	shells(int, char *[]);

enum {
	RV_OK		= 0,
	RV_USAGE	= 1,
	RV_NOTFOUND	= 2,
	RV_NOENUM	= 3,
};

int
main(int argc, char *argv[])
{
	static struct {
		const char	*name;
		int		(*callback)(int, char *[]);
	} *curdb, dbs[] = {
		{	"group",	group,		},
		{	"hosts",	hosts,		},
		{	"networks",	networks,	},
		{	"passwd",	passwd,		},
		{	"shells",	shells,		},

		{	NULL,		NULL,		},
	};

	setprogname(argv[0]);

	if (argc < 2)
		usage();
	for (curdb = dbs; curdb->name != NULL; curdb++) {
		if (strcmp(curdb->name, argv[1]) == 0) {
			exit(curdb->callback(argc, argv));
			break;
		}
	}
	fprintf(stderr, "Unknown database: %s\n", argv[1]);
	usage();
	/* NOTREACHED */
	return RV_USAGE;
}

static int
usage(void)
{

	fprintf(stderr, "Usage: %s database [key ...]\n",
	    getprogname());
	exit(RV_USAGE);
	/* NOTREACHED */
}

static int
parsenum(const char *word, unsigned long *result)
{
	unsigned long	num;
	char		*ep;

	assert(word != NULL);
	assert(result != NULL);

	if (!isdigit((unsigned char)word[0]))
		return 0;
	errno = 0;
	num = strtoul(word, &ep, 10);
	if (num == ULONG_MAX && errno == ERANGE)
		return 0;
	if (*ep != '\0')
		return 0;
	*result = num;
	return 1;
}


		/*
		 * group
		 */

static void
groupprint(const struct group *gr)
{
	char	prefix;
	int	i;
	
	assert(gr != NULL);
	printf("%s:%s:%u",
	    gr->gr_name, gr->gr_passwd, gr->gr_gid);
	prefix = ':';
	for (i = 0; gr->gr_mem[i] != NULL; i++) {
		printf("%c%s", prefix, gr->gr_mem[i]);
		prefix = ',';
	}
	printf("\n");
}

static int
group(int argc, char *argv[])
{
	struct group	*gr;
	unsigned long	id;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	setgroupent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((gr = getgrent()) != NULL)
			groupprint(gr);
	} else {
		for (i = 2; i < argc; i++) {
			if (parsenum(argv[i], &id))
				gr = getgrgid((gid_t)id);
			else
				gr = getgrnam(argv[i]);
			if (gr != NULL)
				groupprint(gr);
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endgrent();
	return rv;
}

		/*
		 * hosts
		 */

static void
hostsprint(const struct hostent *he)
{
	char	buf[INET6_ADDRSTRLEN];
	int	i;

	assert(he != NULL);
	if (inet_ntop(he->h_addrtype, he->h_addr, buf, sizeof(buf)) == NULL)
		strlcpy(buf, "# unknown", sizeof(buf));
	printf("%s\t%s",
	    buf, he->h_name);
	for (i = 0; he->h_aliases[i] != NULL; i++) {
		printf(" %s", he->h_aliases[i]);
	}
	printf("\n");
}

static int
hosts(int argc, char *argv[])
{
	struct hostent	*he;
	char		addr[IN6ADDRSZ];
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	sethostent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((he = gethostent()) != NULL)
			hostsprint(he);
	} else {
		for (i = 2; i < argc; i++) {
			if (inet_pton(AF_INET6, argv[i], (void *)addr) > 0)
				he = gethostbyaddr(addr, IN6ADDRSZ, AF_INET6);
			else if (inet_pton(AF_INET, argv[i], (void *)addr) > 0)
				he = gethostbyaddr(addr, INADDRSZ, AF_INET);
			else
				he = gethostbyname(argv[i]);
			if (he != NULL)
				hostsprint(he);
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endhostent();
	return rv;
}


		/*
		 * networks
		 */

static void
networksprint(const struct netent *ne)
{
	char		buf[INET6_ADDRSTRLEN];
	struct	in_addr	ianet;
	int		i;
	char		prefix;

	assert(ne != NULL);
	ianet = inet_makeaddr(ne->n_net, 0);
	if (inet_ntop(ne->n_addrtype, &ianet, buf, sizeof(buf)) == NULL)
		strlcpy(buf, "# unknown", sizeof(buf));
	printf("%s\t%s",
	    ne->n_name, buf);
	prefix = '\t';
	for (i = 0; ne->n_aliases[i] != NULL; i++) {
		printf("%c%s", prefix, ne->n_aliases[i]);
		prefix = ' ';
	}
	printf("\n");
}

static int
networks(int argc, char *argv[])
{
	struct netent	*ne;
	in_addr_t	net;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	setnetent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((ne = getnetent()) != NULL)
			networksprint(ne);
	} else {
		for (i = 2; i < argc; i++) {
			net = inet_network(argv[i]);
			if (net != INADDR_NONE)
				ne = getnetbyaddr(net, AF_INET);
			else
				ne = getnetbyname(argv[i]);
			if (ne != NULL)
				networksprint(ne);
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endnetent();
	return rv;
}

		/*
		 * passwd
		 */

static void
passwdprint(const struct passwd *pw)
{
	
	assert(pw != NULL);
	printf("%s:%s:%u:%u:%s:%s:%s\n",
	    pw->pw_name, pw->pw_passwd, pw->pw_uid, pw->pw_gid,
	    pw->pw_gecos, pw->pw_dir, pw->pw_shell);
}

static int
passwd(int argc, char *argv[])
{
	struct passwd	*pw;
	unsigned long	id;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	setpassent(1);
	rv = RV_OK;
	if (argc == 2) {
		while ((pw = getpwent()) != NULL)
			passwdprint(pw);
	} else {
		for (i = 2; i < argc; i++) {
			if (parsenum(argv[i], &id))
				pw = getpwuid((uid_t)id);
			else
				pw = getpwnam(argv[i]);
			if (pw != NULL)
				passwdprint(pw);
			else {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endpwent();
	return rv;
}

		/*
		 * shells
		 */

static int
shells(int argc, char *argv[])
{
	const char	*sh;
	int		i, rv;

	assert(argc > 1);
	assert(argv != NULL);

	setusershell();
	rv = RV_OK;
	if (argc == 2) {
		while ((sh = getusershell()) != NULL)
			printf("%s\n", sh);
	} else {
		for (i = 2; i < argc; i++) {
			setusershell();
			while ((sh = getusershell()) != NULL) {
				if (strcmp(sh, argv[i]) == 0) {
					printf("%s\n", sh);
					break;
				}
			}
			if (sh == NULL) {
				rv = RV_NOTFOUND;
				break;
			}
		}
	}
	endusershell();
	return rv;
}
