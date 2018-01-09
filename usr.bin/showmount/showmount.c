/*	$NetBSD: showmount.c,v 1.23 2018/01/09 03:31:15 christos Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1995
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
__COPYRIGHT("@(#) Copyright (c) 1989, 1993, 1995\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)showmount.c	8.3 (Berkeley) 3/29/95";
#endif
__RCSID("$NetBSD: showmount.c,v 1.23 2018/01/09 03:31:15 christos Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/socket.h>

#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <nfs/rpcv2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <vis.h>

/* Constant defs */
#define	ALL	1
#define	DIRS	2

#define	DODUMP		0x1
#define	DOEXPORTS	0x2
#define	DOPARSABLEEXPORTS	0x4

struct mountlist {
	struct mountlist *ml_left;
	struct mountlist *ml_right;
	char	ml_host[RPCMNT_NAMELEN+1];
	char	ml_dirp[RPCMNT_PATHLEN+1];
};

struct grouplist {
	struct grouplist *gr_next;
	char	gr_name[RPCMNT_NAMELEN+1];
};

struct exportslist {
	struct exportslist *ex_next;
	struct grouplist *ex_groups;
	char	ex_dirp[RPCMNT_PATHLEN+1];
};

static struct mountlist *mntdump;
static struct exportslist *exports;
static int type = 0;

static void	print_dump(struct mountlist *);
__dead static void	usage(void);
static int	xdr_mntdump(XDR *, struct mountlist **);
static int	xdr_exports(XDR *, struct exportslist **);
static int	tcp_callrpc(const char *host, int prognum, int versnum,
    int procnum, xdrproc_t inproc, char *in, xdrproc_t outproc, char *out);

/*
 * This command queries the NFS mount daemon for its mount list and/or
 * its exports list and prints them out.
 * See "NFS: Network File System Protocol Specification, RFC1094, Appendix A"
 * and the "Network File System Protocol XXX.."
 * for detailed information on the protocol.
 */
int
main(int argc, char **argv)
{
	struct exportslist *exp;
	struct grouplist *grp;
	int estat, rpcs = 0, mntvers = 1;
	const char *host;
	int ch;
	int len;
	int nbytes;
	char strvised[1024 * 4 + 1];

	while ((ch = getopt(argc, argv, "adEe3")) != -1)
		switch((char)ch) {
		case 'a':
			if (type == 0) {
				type = ALL;
				rpcs |= DODUMP;
			} else
				usage();
			break;
		case 'd':
			if (type == 0) {
				type = DIRS;
				rpcs |= DODUMP;
			} else
				usage();
			break;
		case 'E':
			rpcs |= DOPARSABLEEXPORTS;
			break;
		case 'e':
			rpcs |= DOEXPORTS;
			break;
		case '3':
			mntvers = 3;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if ((rpcs & DOPARSABLEEXPORTS) != 0) {
		if ((rpcs & DOEXPORTS) != 0)
			errx(1, "-E cannot be used with -e");
		if ((rpcs & DODUMP) != 0)
			errx(1, "-E cannot be used with -a or -d");
	}

	if (argc > 0)
		host = *argv;
	else
		host = "localhost";

	if (rpcs == 0)
		rpcs = DODUMP;

	if (rpcs & DODUMP)
		if ((estat = tcp_callrpc(host, RPCPROG_MNT, mntvers,
			 RPCMNT_DUMP, (xdrproc_t)xdr_void, NULL,
			 (xdrproc_t)xdr_mntdump, (char *)&mntdump)) != 0) {
			fprintf(stderr, "showmount: Can't do Mountdump rpc: ");
			clnt_perrno(estat);
			exit(1);
		}
	if (rpcs & (DOEXPORTS | DOPARSABLEEXPORTS))
		if ((estat = tcp_callrpc(host, RPCPROG_MNT, mntvers,
			 RPCMNT_EXPORT, (xdrproc_t)xdr_void, NULL,
			 (xdrproc_t)xdr_exports, (char *)&exports)) != 0) {
			fprintf(stderr, "showmount: Can't do Exports rpc: ");
			clnt_perrno(estat);
			exit(1);
		}

	/* Now just print out the results */
	if (rpcs & DODUMP) {
		switch (type) {
		case ALL:
			printf("All mount points on %s:\n", host);
			break;
		case DIRS:
			printf("Directories on %s:\n", host);
			break;
		default:
			printf("Hosts on %s:\n", host);
			break;
		};
		print_dump(mntdump);
	}
	if (rpcs & DOEXPORTS) {
		printf("Exports list on %s:\n", host);
		exp = exports;
		while (exp) {
			len = printf("%-35s", exp->ex_dirp);
			if (len > 35)
				printf("\t");
			grp = exp->ex_groups;
			if (grp == NULL) {
				printf("Everyone\n");
			} else {
				while (grp) {
					printf("%s ", grp->gr_name);
					grp = grp->gr_next;
				}
				printf("\n");
			}
			exp = exp->ex_next;
		}
	}
	if (rpcs & DOPARSABLEEXPORTS) {
		exp = exports;
		while (exp) {
			nbytes = strnvis(strvised, sizeof(strvised),
			    exp->ex_dirp, VIS_GLOB | VIS_NL);
			if (nbytes == -1)
				err(1, "strsnvis");
			printf("%s\n", strvised);
			exp = exp->ex_next;
		}
	}
	exit(0);
}

/*
 * tcp_callrpc has the same interface as callrpc, but tries to
 * use tcp as transport method in order to handle large replies.
 */

static int
tcp_callrpc(const char *host, int prognum, int versnum, int procnum,
    xdrproc_t inproc, char *in, xdrproc_t outproc, char *out)
{
	CLIENT *client;
	struct timeval timeout;
	int rval;

	if ((client = clnt_create(host, prognum, versnum, "tcp")) == NULL &&
	    (client = clnt_create(host, prognum, versnum, "udp")) == NULL)
		return ((int) rpc_createerr.cf_stat);

	timeout.tv_sec = 25;
	timeout.tv_usec = 0;
	rval = (int) clnt_call(client, procnum, 
			       inproc, in,
			       outproc, out,
			       timeout);
	clnt_destroy(client);
 	return rval;
}

static void
mountlist_free(struct mountlist *ml)
{
	if (ml == NULL)
		return;
	mountlist_free(ml->ml_left);
	mountlist_free(ml->ml_right);
	free(ml);
}

/*
 * Xdr routine for retrieving the mount dump list
 */
static int
xdr_mntdump(XDR *xdrsp, struct mountlist **mlp)
{
	struct mountlist *mp, **otp, *tp;
	int bool_int, val, val2;
	char *strp;

	otp = NULL;
	*mlp = NULL;
	if (!xdr_bool(xdrsp, &bool_int))
		return 0;
	while (bool_int) {
		mp = malloc(sizeof(*mp));
		if (mp == NULL)
			goto out;
		mp->ml_left = mp->ml_right = NULL;
		strp = mp->ml_host;
		if (!xdr_string(xdrsp, &strp, RPCMNT_NAMELEN)) {
			free(mp);
			goto out;
		}
		strp = mp->ml_dirp;
		if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN)) {
			free(mp);
			goto out;
		}

		/*
		 * Build a binary tree on sorted order of either host or dirp.
		 * Drop any duplications.
		 */
		if (*mlp == NULL) {
			*mlp = mp;
		} else {
			tp = *mlp;
			while (tp) {
				val = strcmp(mp->ml_host, tp->ml_host);
				val2 = strcmp(mp->ml_dirp, tp->ml_dirp);
				switch (type) {
				case ALL:
					if (val == 0) {
						if (val2 == 0) {
							free(mp);
							goto next;
						}
						val = val2;
					}
					break;
				case DIRS:
					if (val2 == 0) {
						free(mp);
						goto next;
					}
					val = val2;
					break;
				default:
					if (val == 0) {
						free(mp);
						goto next;
					}
					break;
				};
				if (val < 0) {
					otp = &tp->ml_left;
					tp = tp->ml_left;
				} else {
					otp = &tp->ml_right;
					tp = tp->ml_right;
				}
			}
			*otp = mp;
		}
next:
		if (!xdr_bool(xdrsp, &bool_int))
			goto out;
	}
	return 1;
out:
	mountlist_free(*mlp);
	return 0;
}

static void
grouplist_free(struct grouplist *gp)
{
	if (gp == NULL)
		return;
	grouplist_free(gp->gr_next);
	free(gp);
}

static void
exportslist_free(struct exportslist *ep)
{
	if (ep == NULL)
		return;
	exportslist_free(ep->ex_next);
	grouplist_free(ep->ex_groups);
	free(ep);
}

/*
 * Xdr routine to retrieve exports list
 */
static int
xdr_exports(XDR *xdrsp, struct exportslist **exp)
{
	struct exportslist *ep = NULL;
	struct grouplist *gp;
	int bool_int, grpbool;
	char *strp;

	*exp = NULL;
	if (!xdr_bool(xdrsp, &bool_int))
		return 0;
	while (bool_int) {
		ep = malloc(sizeof(*ep));
		if (ep == NULL)
			goto out;
		ep->ex_groups = NULL;
		strp = ep->ex_dirp;
		if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN))
			goto out;
		if (!xdr_bool(xdrsp, &grpbool))
			goto out;
		while (grpbool) {
			gp = malloc(sizeof(*gp));
			if (gp == NULL)
				goto out;
			gp->gr_next = ep->ex_groups;
			ep->ex_groups = gp;
			strp = gp->gr_name;
			if (!xdr_string(xdrsp, &strp, RPCMNT_NAMELEN))
				goto out;
			if (!xdr_bool(xdrsp, &grpbool))
				goto out;
		}
		ep->ex_next = *exp;
		*exp = ep;
		ep = NULL;
		if (!xdr_bool(xdrsp, &bool_int))
			goto out;
	}
	return 1;
out:
	exportslist_free(ep);
	exportslist_free(*exp);
	return 0;
}

static void
usage(void)
{

	fprintf(stderr, "usage: showmount [-ade3] host\n");
	exit(1);
}

/*
 * Print the binary tree in inorder so that output is sorted.
 */
static void
print_dump(struct mountlist *mp)
{

	if (mp == NULL)
		return;
	if (mp->ml_left)
		print_dump(mp->ml_left);
	switch (type) {
	case ALL:
		printf("%s:%s\n", mp->ml_host, mp->ml_dirp);
		break;
	case DIRS:
		printf("%s\n", mp->ml_dirp);
		break;
	default:
		printf("%s\n", mp->ml_host);
		break;
	};
	if (mp->ml_right)
		print_dump(mp->ml_right);
}
