/*	$NetBSD: nfsstat.c,v 1.18 2003/08/07 11:15:22 agc Exp $	*/

/*
 * Copyright (c) 1983, 1989, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)nfsstat.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: nfsstat.c,v 1.18 2003/08/07 11:15:22 agc Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/sysctl.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct nlist nl[] = {
#define	N_NFSSTAT	0
	{ "_nfsstats" },
	{ "" },
};


void	catchalarm __P((int));
void	getstats __P((struct nfsstats *));
void	intpr __P((void));
int	main __P((int, char **));
void	printhdr __P((void));
void	sidewaysintpr __P((u_int));
void	usage __P((void));

kvm_t  *kd;
int     printall, clientinfo, serverinfo;
u_long	nfsstataddr;

int
main(argc, argv)
	int argc;
	char **argv;
{
	u_int interval;
	int ch;
	char *memf, *nlistf;
	char errbuf[_POSIX2_LINE_MAX];

	interval = 0;
	memf = nlistf = NULL;
	printall = 1;
	while ((ch = getopt(argc, argv, "M:N:w:cs")) != -1)
		switch(ch) {
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'w':
			interval = atoi(optarg);
			break;
		case 's':
		        serverinfo = 1;
			printall = 0;
			break;
		case 'c':
		        clientinfo = 1;
			printall = 0;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#define	BACKWARD_COMPATIBILITY
#ifdef	BACKWARD_COMPATIBILITY
	if (*argv) {
		interval = atoi(*argv);
		if (*++argv) {
			nlistf = *argv;
			if (*++argv)
				memf = *argv;
		}
	}
#endif
	if (nlistf || memf) {
		if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, errbuf))
		    == 0)
			errx(1, "kvm_openfiles: %s", errbuf);

		if (kvm_nlist(kd, nl) != 0)
			errx(1, "kvm_nlist: can't get names");
		nfsstataddr = nl[N_NFSSTAT].n_value;
	} else {
		kd = NULL;
	}

	if (interval)
		sidewaysintpr(interval);
	else
		intpr();
	exit(0);
}

void
getstats(ns)
	struct nfsstats *ns;
{
	size_t size;
	int mib[3];

	if (kd) {
		if (kvm_read(kd, (u_long)nfsstataddr, ns, sizeof(*ns))
		    != sizeof(*ns))
			errx(1, "kvm_read failed");
	} else {
		mib[0] = CTL_VFS;
		mib[1] = 2;	/* XXX from CTL_VFS_NAMES in <sys/mount.h> */
		mib[2] = NFS_NFSSTATS;

		size = sizeof(*ns);
		if (sysctl(mib, 3, ns, &size, NULL, 0) == -1)
			err(1, "sysctl(NFS_NFSSTATS) failed");
	}
}

/*
 * Print a description of the nfs stats.
 */
void
intpr()
{
	struct nfsstats nfsstats;
	int64_t	total;
	int	i;

#define PCT(x,y)	((y) == 0 ? 0 : (int)((int64_t)(x) * 100 / (y)))
#define NUMPCT(x,y)	(x), PCT(x, (x)+(y))
#define	RPCSTAT(x)	(x), PCT(x, total)

	getstats(&nfsstats);

	if (printall || clientinfo) {
		total = 0;
		for (i = 0; i < NFS_NPROCS; i++)
			total += nfsstats.rpccnt[i];
		printf("Client Info:\n");
		printf("RPC Counts: (%lld call%s)\n", (long long)total,
		    total == 1 ? "" : "s");

		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "null", "getattr", "setattr", "lookup", "access");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_NULL]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_GETATTR]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_SETATTR]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_LOOKUP]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_ACCESS]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "readlink", "read", "write", "create", "mkdir");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_READLINK]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_READ]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_WRITE]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_CREATE]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_MKDIR]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "symlink", "mknod", "remove", "rmdir", "rename");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_SYMLINK]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_MKNOD]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_REMOVE]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_RMDIR]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_RENAME]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "link", "readdir", "readdirplus", "fsstat", "fsinfo");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_LINK]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_READDIR]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_READDIRPLUS]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_FSSTAT]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_FSINFO]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "pathconf", "commit", "getlease", "vacated", "evicted");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_PATHCONF]),
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_COMMIT]),
		    RPCSTAT(nfsstats.rpccnt[NQNFSPROC_GETLEASE]),
		    RPCSTAT(nfsstats.rpccnt[NQNFSPROC_VACATED]),
		    RPCSTAT(nfsstats.rpccnt[NQNFSPROC_EVICTED]));
		printf("%10s\n", "noop");
		printf("%10d %2d%%\n",
		    RPCSTAT(nfsstats.rpccnt[NFSPROC_NOOP]));

		printf("RPC Info:\n");
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "timeout", "invalid", "unexpected", "retries", "requests");
		printf("%10d  %14d  %14d  %14d  %14d\n",
		    nfsstats.rpctimeouts,
		    nfsstats.rpcinvalid,
		    nfsstats.rpcunexpected,
		    nfsstats.rpcretries,
		    nfsstats.rpcrequests);

		printf("Cache Info:\n");
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "attrcache", "lookupcache", "read", "write", "readlink");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    NUMPCT(nfsstats.attrcache_hits,
			nfsstats.attrcache_misses),
		    NUMPCT(nfsstats.lookupcache_hits,
			nfsstats.lookupcache_misses),
		    NUMPCT(nfsstats.biocache_reads - nfsstats.read_bios,
			nfsstats.read_bios),
		    NUMPCT(nfsstats.biocache_writes - nfsstats.write_bios,
			nfsstats.write_bios),
		    NUMPCT(nfsstats.biocache_readlinks - nfsstats.readlink_bios,
			nfsstats.readlink_bios));
		printf("%10s  %14s\n",
		    "readdir", "direofcache");
		printf("%10d %2d%%  %10d %2d%%\n",
		    NUMPCT(nfsstats.biocache_readdirs - nfsstats.readdir_bios,
			nfsstats.readdir_bios),
		    NUMPCT(nfsstats.direofcache_hits,
			nfsstats.direofcache_misses));
	}

	if (printall || (clientinfo && serverinfo))
		printf("\n");

	if (printall || serverinfo) {
		total = 0;
		for (i = 0; i < NFS_NPROCS; i++)
			total += nfsstats.srvrpccnt[i];
		printf("Server Info:\n");
		printf("RPC Counts: (%lld call%s)\n", (long long)total,
		    total == 1 ? "" : "s");

		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "null", "getattr", "setattr", "lookup", "access");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_NULL]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_GETATTR]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_SETATTR]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_LOOKUP]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_ACCESS]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "readlink", "read", "write", "create", "mkdir");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_READLINK]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_READ]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_WRITE]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_CREATE]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_MKDIR]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "symlink", "mknod", "remove", "rmdir", "rename");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_SYMLINK]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_MKNOD]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_REMOVE]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_RMDIR]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_RENAME]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "link", "readdir", "readdirplus", "fsstat", "fsinfo");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_LINK]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_READDIR]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_READDIRPLUS]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_FSSTAT]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_FSINFO]));
		printf("%10s  %14s  %14s  %14s  %14s\n",
		    "pathconf", "commit", "getlease", "vacated", "evicted");
		printf(
	    "%10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%  %10d %2d%%\n",
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_PATHCONF]),
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_COMMIT]),
		    RPCSTAT(nfsstats.srvrpccnt[NQNFSPROC_GETLEASE]),
		    RPCSTAT(nfsstats.srvrpccnt[NQNFSPROC_VACATED]),
		    RPCSTAT(nfsstats.srvrpccnt[NQNFSPROC_EVICTED]));
		printf("%10s\n", "noop");
		printf("%10d %2d%%\n",
		    RPCSTAT(nfsstats.srvrpccnt[NFSPROC_NOOP]));

		printf("Server Errors:\n");
		printf("%10s  %14s\n",
		    "RPC errors", "faults");
		printf("%10d  %14d\n",
		    nfsstats.srvrpc_errs,
		    nfsstats.srv_errs);
		printf("Server Cache Stats:\n");
		printf("%10s  %14s  %14s  %14s\n",
		    "inprogress", "idem", "non-idem", "misses");
		printf("%10d  %14d  %14d  %14d\n",
		    nfsstats.srvcache_inproghits,
		    nfsstats.srvcache_idemdonehits,
		    nfsstats.srvcache_nonidemdonehits,
		    nfsstats.srvcache_misses);
		printf("Server Lease Stats:\n");
		printf("%10s  %14s  %14s\n",
		    "leases", "maxleases", "getleases");
		printf("%10d  %14d  %14d\n",
		    nfsstats.srvnqnfs_leases,
		    nfsstats.srvnqnfs_maxleases,
		    nfsstats.srvnqnfs_getleases);
		printf("Server Write Gathering:\n");
		printf("%10s  %14s  %14s\n",
		    "writes", "write RPC", "OPs saved");
		printf("%10d  %14d  %14d %2d%%\n",
		    nfsstats.srvvop_writes,
		    nfsstats.srvrpccnt[NFSPROC_WRITE],
		    NUMPCT(
		      nfsstats.srvrpccnt[NFSPROC_WRITE]-nfsstats.srvvop_writes,
		      nfsstats.srvrpccnt[NFSPROC_WRITE]));
	}
}

u_char	signalled;			/* set if alarm goes off "early" */

/*
 * Print a running summary of nfs statistics.
 * Repeat display every interval seconds, showing statistics
 * collected over that interval.  Assumes that interval is non-zero.
 * First line printed at top of screen is always cumulative.
 */
void
sidewaysintpr(interval)
	u_int interval;
{
	struct nfsstats nfsstats, lastst;
	int hdrcnt, oldmask;

	(void)signal(SIGALRM, catchalarm);
	signalled = 0;
	(void)alarm(interval);
	memset((caddr_t)&lastst, 0, sizeof(lastst));

	for (hdrcnt = 1;;) {
		if (!--hdrcnt) {
			printhdr();
			hdrcnt = 20;
		}
		getstats(&nfsstats);
		if (printall || clientinfo)
			printf("Client: %8d %8d %8d %8d %8d %8d %8d %8d\n",
			    nfsstats.rpccnt[NFSPROC_GETATTR] -
			    lastst.rpccnt[NFSPROC_GETATTR],
			    nfsstats.rpccnt[NFSPROC_LOOKUP] -
			    lastst.rpccnt[NFSPROC_LOOKUP],
			    nfsstats.rpccnt[NFSPROC_READLINK] -
			    lastst.rpccnt[NFSPROC_READLINK],
			    nfsstats.rpccnt[NFSPROC_READ] -
			    lastst.rpccnt[NFSPROC_READ],
			    nfsstats.rpccnt[NFSPROC_WRITE] -
			    lastst.rpccnt[NFSPROC_WRITE],
			    nfsstats.rpccnt[NFSPROC_RENAME] -
			    lastst.rpccnt[NFSPROC_RENAME],
			    nfsstats.rpccnt[NFSPROC_ACCESS] -
			    lastst.rpccnt[NFSPROC_ACCESS],
			    (nfsstats.rpccnt[NFSPROC_READDIR] -
			    lastst.rpccnt[NFSPROC_READDIR]) +
			    (nfsstats.rpccnt[NFSPROC_READDIRPLUS] -
			    lastst.rpccnt[NFSPROC_READDIRPLUS]));
		if (printall || serverinfo)
			printf("Server: %8d %8d %8d %8d %8d %8d %8d %8d\n",
			    nfsstats.srvrpccnt[NFSPROC_GETATTR] -
			    lastst.srvrpccnt[NFSPROC_GETATTR],
			    nfsstats.srvrpccnt[NFSPROC_LOOKUP] -
			    lastst.srvrpccnt[NFSPROC_LOOKUP],
			    nfsstats.srvrpccnt[NFSPROC_READLINK] -
			    lastst.srvrpccnt[NFSPROC_READLINK],
			    nfsstats.srvrpccnt[NFSPROC_READ] -
			    lastst.srvrpccnt[NFSPROC_READ],
			    nfsstats.srvrpccnt[NFSPROC_WRITE] -
			    lastst.srvrpccnt[NFSPROC_WRITE],
			    nfsstats.srvrpccnt[NFSPROC_RENAME] -
			    lastst.srvrpccnt[NFSPROC_RENAME],
			    nfsstats.srvrpccnt[NFSPROC_ACCESS] -
			    lastst.srvrpccnt[NFSPROC_ACCESS],
			    (nfsstats.srvrpccnt[NFSPROC_READDIR] -
			    lastst.srvrpccnt[NFSPROC_READDIR]) +
			    (nfsstats.srvrpccnt[NFSPROC_READDIRPLUS] -
			    lastst.srvrpccnt[NFSPROC_READDIRPLUS]));
		lastst = nfsstats;
		fflush(stdout);
		oldmask = sigblock(sigmask(SIGALRM));
		if (!signalled)
			sigpause(0);
		sigsetmask(oldmask);
		signalled = 0;
		(void)alarm(interval);
	}
	/*NOTREACHED*/
}

void
printhdr()
{

	printf("        %8.8s %8.8s %8.8s %8.8s %8.8s %8.8s %8.8s %8.8s\n",
	    "Getattr", "Lookup", "Readlink", "Read", "Write", "Rename",
	    "Access", "Readdir");
	fflush(stdout);
}

/*
 * Called if an interval expires before sidewaysintpr has completed a loop.
 * Sets a flag to not wait for the alarm.
 */
void
catchalarm(dummy)
	int dummy;
{

	signalled = 1;
}

void
usage()
{

	(void)fprintf(stderr,
		  "usage: nfsstat [-cs] [-M core] [-N system] [-w interval]\n");
	exit(1);
}
