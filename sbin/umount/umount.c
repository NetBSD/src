/*	$NetBSD: umount.c,v 1.34 2004/04/21 15:20:40 christos Exp $	*/

/*-
 * Copyright (c) 1980, 1989, 1993
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
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)umount.c	8.8 (Berkeley) 5/8/95";
#else
__RCSID("$NetBSD: umount.c,v 1.34 2004/04/21 15:20:40 christos Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/time.h>
#ifndef SMALL
#include <sys/socket.h>

#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <nfs/rpcv2.h>
#endif /* !SMALL */

#include <err.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { MNTANY, MNTON, MNTFROM } mntwhat;

#ifndef SMALL
#include "vfslist.h"

static int	 fake, verbose;
static char	*nfshost;
static struct addrinfo *nfshost_ai = NULL;

static int	 namematch(const struct addrinfo *);
static int	 sacmp(const struct sockaddr *, const struct sockaddr *);
static int	 xdr_dir(XDR *, char *);
#endif /* !SMALL */

static int	 fflag, raw;
static char	*getmntname(const char *, mntwhat, char **);
static int	 umountfs(const char *, const char **);
static void	 usage(void) __attribute__((__noreturn__));

int	 main(int, char *[]);

int
main(int argc, char *argv[])
{
	int ch, errs, all = 0;
#ifndef SMALL
	int mnts;
	struct statvfs *mntbuf;
	struct addrinfo hints;
#endif /* SMALL */
	const char **typelist = NULL;

	/* Start disks transferring immediately. */
	sync();

#ifdef SMALL
#define OPTS "fr"
#else
#define OPTS "AaFfh:Rt:v"
#endif
	while ((ch = getopt(argc, argv, OPTS)) != -1)
		switch (ch) {
		case 'f':
			fflag = MNT_FORCE;
			break;
		case 'R':
			raw = 1;
			break;
#ifndef SMALL
		case 'A':
		case 'a':
			all = 1;
			break;
		case 'F':
			fake = 1;
			break;
		case 'h':	/* -h implies -A. */
			all = 1;
			nfshost = optarg;
			break;
		case 't':
			if (typelist != NULL)
				errx(1, "only one -t option may be specified.");
			typelist = makevfslist(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
#endif /* !SMALL */
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if ((argc == 0 && !all) || (argc != 0 && all) || (all && raw))
		usage();

#ifndef SMALL
	/* -h implies "-t nfs" if no -t flag. */
	if ((nfshost != NULL) && (typelist == NULL))
		typelist = makevfslist("nfs");

	if (nfshost != NULL) {
		memset(&hints, 0, sizeof hints);
		getaddrinfo(nfshost, NULL, &hints, &nfshost_ai);
	}
		
	errs = 0;
	if (all) {
		if ((mnts = getmntinfo(&mntbuf, ST_NOWAIT)) == 0) {
			warn("getmntinfo");
			errs = 1;
		}
		for (errs = 0, mnts--; mnts > 0; mnts--) {
			if (checkvfsname(mntbuf[mnts].f_fstypename, typelist))
				continue;
			if (umountfs(mntbuf[mnts].f_mntonname, typelist) != 0)
				errs = 1;
		}
	} else 
#endif /* !SMALL */
		for (errs = 0; *argv != NULL; ++argv)
			if (umountfs(*argv, typelist) != 0)
				errs = 1;
	return errs;
}

static int
umountfs(const char *name, const char **typelist)
{
#ifndef SMALL
	enum clnt_stat clnt_stat;
	struct timeval try;
	CLIENT *clp;
	char *hostp = NULL;
	struct addrinfo *ai = NULL, hints;
#endif /* !SMALL */
	const char *mntpt;
	char *type, rname[MAXPATHLEN];
	mntwhat what;
	struct stat sb;

	if (raw) {
		mntpt = name;
	} else {

		what = MNTANY;
		if (realpath(name, rname) != NULL) {
			name = rname;

			if (stat(name, &sb) == 0) {
				if (S_ISBLK(sb.st_mode))
					what = MNTON;
				else if (S_ISDIR(sb.st_mode))
					what = MNTFROM;
			}
		}
#ifdef SMALL
		else {
 			warn("%s", rname);
 			return 1;
		}
#endif /* SMALL */
		mntpt = name;

		switch (what) {
		case MNTON:
			if ((mntpt = getmntname(name, MNTON, &type)) == NULL) {
				warnx("%s: not currently mounted", name);
				return (1);
			}
			break;
		case MNTFROM:
			if ((name = getmntname(mntpt, MNTFROM, &type)) == NULL) {
				warnx("%s: not currently mounted", mntpt);
				return (1);
			}
			break;
		default:
			if ((name = getmntname(mntpt, MNTFROM, &type)) == NULL) {
				name = mntpt;
				if ((mntpt = getmntname(name, MNTON, &type)) == NULL) {
					warnx("%s: not currently mounted", name);
					return 1;
				}
			}
		}

#ifndef SMALL
		if (checkvfsname(type, typelist))
			return 1;

		(void)memset(&hints, 0, sizeof hints);
		if (!strncmp(type, MOUNT_NFS, MFSNAMELEN)) {
			char *delimp;
			/* look for host:mountpoint */
			if ((delimp = strrchr(name, ':')) != NULL) {
				int len = delimp - name;
				hostp = malloc(len + 1);
				if (hostp == NULL)
				    	return 1;
				memcpy(hostp, name, len);
				hostp[len] = 0;
				name += len + 1;
				getaddrinfo(hostp, NULL, &hints, &ai);
			}
		}

		if (!namematch(ai))
			return 1;
#endif /* ! SMALL */
	}

#ifndef SMALL
	if (verbose)
		(void)printf("%s: unmount from %s\n", name, mntpt);
	if (fake)
		return 0;
#endif /* ! SMALL */

	if (unmount(mntpt, fflag) < 0) {
		warn("%s", mntpt);
		return 1;
	}

#ifndef SMALL
	if (ai != NULL && !(fflag & MNT_FORCE)) {
		clp = clnt_create(hostp, RPCPROG_MNT, RPCMNT_VER1, "udp");
		if (clp  == NULL) {
			clnt_pcreateerror("Cannot MNT PRC");
			return 1;
		}
		clp->cl_auth = authsys_create_default();
		try.tv_sec = 20;
		try.tv_usec = 0;
		clnt_stat = clnt_call(clp,
		    RPCMNT_UMOUNT, xdr_dir, name, xdr_void, (caddr_t)0, try);
		if (clnt_stat != RPC_SUCCESS) {
			clnt_perror(clp, "Bad MNT RPC");
			return 1;
		}
		auth_destroy(clp->cl_auth);
		clnt_destroy(clp);
	}
#endif /* ! SMALL */
	return 0;
}

static char *
getmntname(const char *name, mntwhat what, char **type)
{
	static struct statvfs *mntbuf;
	static int mntsize;
	int i;

	if (mntbuf == NULL &&
	    (mntsize = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0) {
		warn("getmntinfo");
		return (NULL);
	}
	for (i = mntsize - 1; i >= 0; i--) {
		if ((what == MNTON) && !strcmp(mntbuf[i].f_mntfromname, name)) {
			if (type)
				*type = mntbuf[i].f_fstypename;
			return (mntbuf[i].f_mntonname);
		}
		if ((what == MNTFROM) && !strcmp(mntbuf[i].f_mntonname, name)) {
			if (type)
				*type = mntbuf[i].f_fstypename;
			return (mntbuf[i].f_mntfromname);
		}
	}
	return (NULL);
}

#ifndef SMALL
static int
sacmp(const struct sockaddr *sa1, const struct sockaddr *sa2)
{
	void *p1, *p2;
	int len;

	if (sa1->sa_family != sa2->sa_family)
		return 1;

	switch (sa1->sa_family) {
	case AF_INET:
		p1 = &((struct sockaddr_in *)sa1)->sin_addr;
		p2 = &((struct sockaddr_in *)sa2)->sin_addr;
		len = 4;
		break;
	case AF_INET6:
		p1 = &((struct sockaddr_in6 *)sa1)->sin6_addr;
		p2 = &((struct sockaddr_in6 *)sa2)->sin6_addr;
		len = 16;
		if (((struct sockaddr_in6 *)sa1)->sin6_scope_id !=
		    ((struct sockaddr_in6 *)sa2)->sin6_scope_id)
			return 1;
		break;
	default:
		return 1;
	}

	return memcmp(p1, p2, len);
}

static int
namematch(const struct addrinfo *ai)
{
	struct addrinfo *aip;

	if (nfshost == NULL || nfshost_ai == NULL)
		return (1);

	while (ai != NULL) {
		aip = nfshost_ai;
		while (aip != NULL) {
			if (sacmp(ai->ai_addr, aip->ai_addr) == 0)
				return 1;
			aip = aip->ai_next;
		}
		ai = ai->ai_next;
	}

	return 0;
}

/*
 * xdr routines for mount rpc's
 */
static int
xdr_dir(XDR *xdrsp, char *dirp)
{
	return xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN);
}
#endif /* !SMALL */

static void
usage(void)
{
#ifdef SMALL
	(void)fprintf(stderr,
	    "Usage: %s [-fR]  special | node\n", getprogname());
#else
	(void)fprintf(stderr,
	    "Usage: %s [-fvFR] [-t fstypelist] special | node\n"
	    "\t %s -a[fvF] [-h host] [-t fstypelist]\n", getprogname(),
	    getprogname());
#endif /* SMALL */
	exit(1);
}
