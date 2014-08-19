/*	$NetBSD: mknetid.c,v 1.18.8.1 2014/08/20 00:05:19 tls Exp $	*/

/*
 * Copyright (c) 1996 Mats O Jansson <moj@stacken.kth.se>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: mknetid.c,v 1.18.8.1 2014/08/20 00:05:19 tls Exp $");
#endif

/*
 * Originally written by Mats O Jansson <moj@stacken.kth.se>
 * Simplified a bit by Jason R. Thorpe <thorpej@NetBSD.org>
 */

#include <sys/param.h>
#include <sys/queue.h>
#include <ctype.h>
#include <err.h>
#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <rpcsvc/ypclnt.h>

#include "protos.h"

struct user {
	char 	*usr_name;		/* user name */
	int	usr_uid;		/* user uid */
	int	usr_gid;		/* user gid */
	int	gid_count;		/* number of gids */
	int	gid[NGROUPS];		/* additional gids */
	TAILQ_ENTRY(user) read;		/* links in read order */
	TAILQ_ENTRY(user) hash;		/* links in hash order */
};

#define HASHMAX 55

static void	add_group(const char *, const char *);
static void	add_user(const char *, const char *, const char *);
static int	hashidx(char);
static int	isgsep(char);
static void	print_hosts(const char *, const char *);
static void	print_netid(const char *);
static void	print_passwd_group(int, const char *);
static void	read_group(const char *);
static void	read_passwd(const char *);
__dead static void	usage(void);

TAILQ_HEAD(user_list, user);
static struct user_list root;
static struct user_list hroot[HASHMAX];

int
main(int argc, char *argv[])
{
	const char *HostFile = _PATH_HOSTS;
	const char *PasswdFile = _PATH_PASSWD;
	const char *GroupFile = _PATH_GROUP;
	const char *NetidFile = "/etc/netid";

	int qflag, ch;
	char *domain;

	TAILQ_INIT(&root);
	for (ch = 0; ch < HASHMAX; ch++)
		TAILQ_INIT((&hroot[ch]));

	qflag = 0;
	domain = NULL;

	while ((ch = getopt(argc, argv, "d:g:h:m:p:q")) != -1) {
		switch (ch) {
		case 'd':
			domain = optarg;
			break;

		case 'g':
			GroupFile = optarg;
			break;

		case 'h':
			HostFile = optarg;
			break;

		case 'm':
			NetidFile = optarg;
			break;

		case 'p':
			PasswdFile = optarg;
			break;

		case 'q':
			qflag++;
			break;

		default:
			usage();
		}
	}
	if (argc != optind)
		usage();

	if (domain == NULL)
		if (yp_get_default_domain(&domain))
			errx(1, "Can't get YP domain name");

	read_passwd(PasswdFile);
	read_group(GroupFile);

	print_passwd_group(qflag, domain);
	print_hosts(HostFile, domain);
	print_netid(NetidFile);

	exit (0);
}

static int
hashidx(char key)
{
	if (key < 'A')
		return(0);

	if (key <= 'Z')
		return(1 + key - 'A');

	if (key < 'a')
		return(27);

	if (key <= 'z')
		return(28 + key - 'a');

	return(54);
}

static void
add_user(const char *username, const char *uid, const char *gid)
{
	struct user *u;
	int idx;

	idx = hashidx(username[0]);

	u = (struct user *)malloc(sizeof(struct user));
	if (u == NULL)
		err(1, "can't allocate user");
	memset(u, 0, sizeof(struct user));

	u->usr_name = strdup(username);
	if (u->usr_name == NULL)
		err(1, "can't allocate user name");

	u->usr_uid = atoi(uid);
	u->usr_gid = atoi(gid);
	u->gid_count = -1;

	TAILQ_INSERT_TAIL(&root, u, read);
	TAILQ_INSERT_TAIL((&hroot[idx]), u, hash);
}

static void
add_group(const char *username, const char *gid)
{
	struct user *u;
	int g, idx;

	g = atoi(gid);
	idx = hashidx(username[0]);

	for (u = hroot[idx].tqh_first;
	    u != NULL; u = u->hash.tqe_next) {
		if (strcmp(username, u->usr_name) == 0) {
			if (g != u->usr_gid) {
				u->gid_count++;
				if (u->gid_count < NGROUPS)
					u->gid[u->gid_count] = g;
			}
			return;
		}
	}
}

static void
read_passwd(const char *fname)
{
	FILE	*pfile;
	size_t	 line_no;
	int	 colon;
	size_t	 len;
	char	*line, *p, *k, *u, *g;

	if ((pfile = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	line_no = 0;
	for (;
	    (line = fparseln(pfile, &len, &line_no, NULL, FPARSELN_UNESCALL));
	    free(line)) {
		if (len == 0) {
			warnx("%s line %lu: empty line", fname,
			    (unsigned long)line_no);
			continue;
		}

		p = line;
		for (k = p, colon = 0; *k != '\0'; k++)
			if (*k == ':')
				colon++;

		if (colon != 6) {
			warnx("%s line %lu: incorrect number of fields",
			    fname, (unsigned long)line_no);
			continue;
		}

		k = p;
		p = strchr(p, ':');
		*p++ = '\0';

		/* If it's a YP entry, skip it. */
		if (*k == '+' || *k == '-')
			continue;

		/* terminate password */
		p = strchr(p, ':');
		*p++ = '\0';

		/* terminate uid */
		u = p;
		p = strchr(p, ':');
		*p++ = '\0';

		/* terminate gid */
		g = p;
		p = strchr(p, ':');
		*p++ = '\0';

		add_user(k, u, g);
	}
	(void)fclose(pfile);
}

static int
isgsep(char ch)
{

	switch (ch) {
	case ',':
	case ' ':
	case '\t':
	case '\0':
		return (1);
	}

	return (0);
}

static void
read_group(const char *fname)
{
	FILE	*gfile;
	size_t	 line_no;
	int	 colon;
	size_t	 len;
	char	*line, *p, *k, *u, *g;

	if ((gfile = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	line_no = 0;
	for (;
	    (line = fparseln(gfile, &len, &line_no, NULL, FPARSELN_UNESCALL));
	    free(line)) {
		if (len == 0) {
			warnx("%s line %lu: empty line", fname,
			    (unsigned long)line_no);
			continue;
		}

		p = line;
		for (k = p, colon = 0; *k != '\0'; k++)
			if (*k == ':')
				colon++;

		if (colon != 3) {
			warnx("%s line %lu: incorrect number of fields",
			    fname, (unsigned long)line_no);
			continue;
		}

		/* terminate key */
		k = p;
		p = strchr(p, ':');
		*p++ = '\0';

		if (*k == '+' || *k == '-')
			continue;

		/* terminate password */
		p = strchr(p, ':');
		*p++ = '\0';

		/* terminate gid */
		g = p;
		p = strchr(p, ':');
		*p++ = '\0';

		/* get the group list */
		for (u = p; *u != '\0'; u = p) {
			/* find separator */
			for (; isgsep(*p) == 0; p++)
				;

			if (*p != '\0') {
				*p = '\0';
				if (u != p)
					add_group(u, g);
				p++;
			} else if (u != p)
				add_group(u, g);
		}
	}
	(void)fclose(gfile);
}

static void
print_passwd_group(int qflag, const char *domain)
{
	struct user *u, *p;
	int i;

	for (u = root.tqh_first; u != NULL; u = u->read.tqe_next) {
		for (p = root.tqh_first; p->usr_uid != u->usr_uid;
		    p = p->read.tqe_next)
			/* empty */ ;
		if (p != u) {
			if (!qflag) {
				warnx("unix.%d@%s %s", u->usr_uid, domain,
				 "multiply defined, ignoring duplicate");
			}
		} else {
			printf("unix.%d@%s %d:%d", u->usr_uid, domain,
			    u->usr_uid, u->usr_gid);
			if (u->gid_count >= 0)
				for (i = 0; i <= u->gid_count; i++)
					printf(",%d", u->gid[i]);
			printf("\n");
		}
	}
}

static void
print_hosts(const char *fname, const char *domain)
{
	FILE	*hfile;
	size_t	 len;
	char	*line, *p, *u;

	if ((hfile = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);

	for (;
	    (line = fparseln(hfile, &len, NULL, NULL, FPARSELN_UNESCALL));
	    free(line)) {
		if (len == 0)
			continue;

		p = line;
		/* Find the key, replace trailing whitespace will <NUL> */
		for (; *p && isspace((unsigned char)*p) == 0; p++)
			;
		while (*p && isspace((unsigned char)*p))
			*p++ = '\0';

		/* Get first hostname. */
		for (u = p; *p && !isspace((unsigned char)*p); p++)
			;
		*p = '\0';

		printf("unix.%s@%s 0:%s\n", u, domain, u);
	}
	(void) fclose(hfile);
}

static void
print_netid(const char *fname)
{
	FILE	*mfile;
	size_t	 len;
	char	*line, *p, *k, *u;

	mfile = fopen(fname, "r");
	if (mfile == NULL)
		return;

	for (;
	    (line = fparseln(mfile, &len, NULL, NULL, FPARSELN_UNESCALL));
	    free(line)) {
		if (len == 0)
			continue;

		p = line;
		/* Find the key, replace trailing whitespace will <NUL> */
		for (k = p; *p && !isspace((unsigned char)*p); p++)
			;
		while (*p && isspace((unsigned char)*p))
			*p++ = '\0';

		/* Get netid entry. */
		for (u = p; *p && !isspace((unsigned char)*p); p++)
			;
		*p = '\0';

		printf("%s %s\n", k, u);
	}
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s %s\n", getprogname(),
	    "[-d domain] [-q] [-p passwdfile] [-g groupfile]");
	fprintf(stderr, "       %s  %s", getprogname(),
	    "[-g groupfile] [-h hostfile] [-m netidfile]");
	exit(1);
}
