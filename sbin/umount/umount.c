/*	$NetBSD: umount.c,v 1.29 2003/08/07 10:04:41 agc Exp $	*/

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
__RCSID("$NetBSD: umount.c,v 1.29 2003/08/07 10:04:41 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <nfs/rpcv2.h>

#include <err.h>
#include <fstab.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef enum { MNTANY, MNTON, MNTFROM } mntwhat;

int	fake, fflag, verbose, raw;
char	*nfshost;
struct addrinfo *nfshost_ai = NULL;

int	 checkvfsname __P((const char *, char **));
char	*getmntname __P((char *, mntwhat, char **));
char	**makevfslist __P((char *));
int	 main __P((int, char *[]));
int	 namematch __P((struct addrinfo *));
int	 sacmp __P((struct sockaddr *, struct sockaddr *));
int	 selected __P((int));
int	 umountfs __P((char *, char **));
void	 usage __P((void));
int	 xdr_dir __P((XDR *, char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int all, ch, errs, mnts;
	char **typelist = NULL;
	struct statfs *mntbuf;
	struct addrinfo hints;

	/* Start disks transferring immediately. */
	sync();

	all = 0;
	while ((ch = getopt(argc, argv, "AaFfRh:t:v")) != -1)
		switch (ch) {
		case 'A':
		case 'a':
			all = 1;
			break;
		case 'F':
			fake = 1;
			break;
		case 'f':
			fflag = MNT_FORCE;
			break;
		case 'h':	/* -h implies -A. */
			all = 1;
			nfshost = optarg;
			break;
		case 'R':
			raw = 1;
			break;
		case 't':
			if (typelist != NULL)
				errx(1, "only one -t option may be specified.");
			typelist = makevfslist(optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	argc -= optind;
	argv += optind;

	if ((argc == 0 && !all) || (argc != 0 && all) || (all && raw))
		usage();

	/* -h implies "-t nfs" if no -t flag. */
	if ((nfshost != NULL) && (typelist == NULL))
		typelist = makevfslist("nfs");

	if (nfshost != NULL) {
		memset(&hints, 0, sizeof hints);
		getaddrinfo(nfshost, NULL, &hints, &nfshost_ai);
	}
		
	errs = 0;
	if (all) {
		if ((mnts = getmntinfo(&mntbuf, MNT_NOWAIT)) == 0) {
			warn("getmntinfo");
			errs = 1;
		}
		for (errs = 0, mnts--; mnts > 0; mnts--) {
			if (checkvfsname(mntbuf[mnts].f_fstypename, typelist))
				continue;
			if (umountfs(mntbuf[mnts].f_mntonname, typelist) != 0)
				errs = 1;
		}
	} else {
		for (errs = 0; *argv != NULL; ++argv)
			if (umountfs(*argv, typelist) != 0)
				errs = 1;
	}
	exit(errs);
}

int
umountfs(name, typelist)
	char *name;
	char **typelist;
{
	enum clnt_stat clnt_stat;
	struct stat sb;
	struct timeval try;
	CLIENT *clp;
	char *type, *delimp, *hostp, *mntpt, rname[MAXPATHLEN];
	mntwhat what;
	struct addrinfo *ai, hints;

	delimp = NULL;

	if (raw) {
		mntpt = name;
	} else {

		if (realpath(name, rname) == NULL) {
			warn("%s", rname);
			return (1);
		}

		what = MNTANY;
		mntpt = name = rname;

		if (stat(name, &sb) == 0) {
			if (S_ISBLK(sb.st_mode))
				what = MNTON;
			else if (S_ISDIR(sb.st_mode))
				what = MNTFROM;
		}

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
				name = rname;
				if ((mntpt = getmntname(name, MNTON, &type)) == NULL) {
					warnx("%s: not currently mounted", name);
					return (1);
				}
			}
		}

		if (checkvfsname(type, typelist))
			return (1);

		memset(&hints, 0, sizeof hints);
		ai = NULL;
		if (!strncmp(type, MOUNT_NFS, MFSNAMELEN)) {
			if ((delimp = strchr(name, '@')) != NULL) {
				hostp = delimp + 1;
				*delimp = '\0';
				getaddrinfo(hostp, NULL, &hints, &ai);
				*delimp = '@';
			} else if ((delimp = strrchr(name, ':')) != NULL) {
				*delimp = '\0';
				hostp = name;
				getaddrinfo(hostp, NULL, &hints, &ai);
				name = delimp + 1;
				*delimp = ':';
			}
		}

		if (!namematch(ai))
			return (1);
	}

	if (verbose)
		(void)printf("%s: unmount from %s\n", name, mntpt);
	if (fake)
		return (0);

	if (unmount(mntpt, fflag) < 0) {
		warn("%s", mntpt);
		return (1);
	}

	if (!raw && !strncmp(type, MOUNT_NFS, MFSNAMELEN) &&
	    (ai != NULL) && !(fflag & MNT_FORCE)) {
		*delimp = '\0';
		clp = clnt_create(hostp, RPCPROG_MNT, RPCMNT_VER1, "udp");
		if (clp  == NULL) {
			clnt_pcreateerror("Cannot MNT PRC");
			return (1);
		}
		clp->cl_auth = authsys_create_default();
		try.tv_sec = 20;
		try.tv_usec = 0;
		clnt_stat = clnt_call(clp,
		    RPCMNT_UMOUNT, xdr_dir, name, xdr_void, (caddr_t)0, try);
		if (clnt_stat != RPC_SUCCESS) {
			clnt_perror(clp, "Bad MNT RPC");
			return (1);
		}
		auth_destroy(clp->cl_auth);
		clnt_destroy(clp);
	}
	return (0);
}

char *
getmntname(name, what, type)
	char *name;
	mntwhat what;
	char **type;
{
	static struct statfs *mntbuf;
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

int
sacmp(struct sockaddr *sa1, struct sockaddr *sa2)
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

int
namematch(ai)
	struct addrinfo *ai;
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
int
xdr_dir(xdrsp, dirp)
	XDR *xdrsp;
	char *dirp;
{
	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

void
usage()
{
	(void)fprintf(stderr,
	    "usage: %s\n       %s\n",
	    "umount [-fvFR] [-t fstypelist] special | node",
	    "umount -a[fvF] [-h host] [-t fstypelist]");
	exit(1);
}
