/*	$NetBSD: sysctl.c,v 1.63 2003/01/22 17:12:41 dsl Exp $	*/

/*
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT(
"@(#) Copyright (c) 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)sysctl.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: sysctl.c,v 1.63 2003/01/22 17:12:41 dsl Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/gmon.h>
#include <sys/stat.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/mount.h>
#include <sys/mbuf.h>
#include <sys/resource.h>
#include <uvm/uvm_param.h>
#include <machine/cpu.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/icmp_var.h>
#include <netinet/ip_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/udp6.h>
#include <netinet6/udp6_var.h>
#ifdef TCP6
#include <netinet6/tcp6.h>
#include <netinet6/tcp6_timer.h>
#include <netinet6/tcp6_var.h>
#endif
#include <netinet6/pim6_var.h>
#endif /* INET6 */

#include "../../sys/compat/linux/common/linux_exec.h"
#include "../../sys/compat/irix/irix_sysctl.h"
#include "../../sys/compat/darwin/darwin_sysctl.h"

#ifdef IPSEC
#include <net/route.h>
#include <netinet6/ipsec.h>
#include <netkey/key_var.h>
#endif /* IPSEC */

#include <sys/pipe.h>

#include <err.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct ctlname topname[] = CTL_NAMES;
struct ctlname kernname[] = CTL_KERN_NAMES;
struct ctlname vmname[] = CTL_VM_NAMES;
struct ctlname vfsname[] = CTL_VFS_NAMES;
struct ctlname netname[] = CTL_NET_NAMES;
struct ctlname hwname[] = CTL_HW_NAMES;
struct ctlname username[] = CTL_USER_NAMES;
struct ctlname ddbname[] = CTL_DDB_NAMES;
struct ctlname debugname[CTL_DEBUG_MAXID];
#ifdef CTL_MACHDEP_NAMES
struct ctlname machdepname[] = CTL_MACHDEP_NAMES;
#endif
struct ctlname emulname[] = CTL_EMUL_NAMES;
struct ctlname vendorname[] = { { 0, 0 } };

/* this one is dummy, it's used only for '-a' or '-A' */
struct ctlname procname[] = { {0, 0}, {"curproc", CTLTYPE_NODE} };

char names[BUFSIZ];

struct list {
	struct	ctlname *list;
	int	size;
};
struct list toplist = { topname, CTL_MAXID };
struct list secondlevel[] = {
	{ 0, 0 },			/* CTL_UNSPEC */
	{ kernname, KERN_MAXID },	/* CTL_KERN */
	{ vmname, VM_MAXID },		/* CTL_VM */
	{ vfsname, VFS_MAXID },		/* CTL_VFS */
	{ netname, NET_MAXID },		/* CTL_NET */
	{ 0, CTL_DEBUG_MAXID },		/* CTL_DEBUG */
	{ hwname, HW_MAXID },		/* CTL_HW */
#ifdef CTL_MACHDEP_NAMES
	{ machdepname, CPU_MAXID },	/* CTL_MACHDEP */
#else
	{ 0, 0 },			/* CTL_MACHDEP */
#endif
	{ username, USER_MAXID },	/* CTL_USER_NAMES */
	{ ddbname, DDBCTL_MAXID },	/* CTL_DDB_NAMES */
	{ procname, 2 },		/* dummy name */
	{ vendorname, 0 },		/* CTL_VENDOR_NAMES */
	{ emulname, EMUL_MAXID },	/* CTL_EMUL_NAMES */

	{ 0, 0},
};

int	Aflag, aflag, nflag, qflag, wflag;

/*
 * Variables requiring special processing.
 */
#define	CLOCK		0x00000001
#define	BOOTTIME	0x00000002
#define	CONSDEV		0x00000004
#define	DISKINFO	0x00000008
#define	CPTIME		0x00000010

/*
 * A dummy type for limits, which requires special parsing
 */
#define CTLTYPE_LIMIT	((~0x1) << 31)

int main(int, char *[]);

static void listall(const char *, struct list *);
static void parse(char *, int);
static void debuginit(void);
static int sysctl_inet(char *, char **, int[], int, int *);
#ifdef INET6
static int sysctl_inet6(char *, char **, int[], int, int *);
#endif
static int sysctl_vfs(char *, char **, int[], int, int *);
static int sysctl_proc(char *, char **, int[], int, int *);
static int sysctl_3rd(struct list *, char *, char **, int[], int, int *);

#ifdef IPSEC
struct ctlname keynames[] = KEYCTL_NAMES;
struct list keyvars = { keynames, KEYCTL_MAXID };
#endif /*IPSEC*/
struct ctlname vfsgenname[] = CTL_VFSGENCTL_NAMES;
struct list vfsgenvars = { vfsgenname, VFSGEN_MAXID };
struct ctlname mbufnames[] = CTL_MBUF_NAMES;
struct list mbufvars = { mbufnames, MBUF_MAXID };
struct ctlname pipenames[] = CTL_PIPE_NAMES;
struct list pipevars = { pipenames, KERN_PIPE_MAXID };
struct ctlname tkstatnames[] = KERN_TKSTAT_NAMES;
struct list tkstatvars = { tkstatnames, KERN_TKSTAT_MAXID };

static int sysctl_linux(char *, char **, int[], int, int *);
static int sysctl_irix(char *, char **, int[], int, int *);
static int sysctl_darwin(char *, char **, int[], int, int *);
static int findname(char *, char *, char **, struct list *);
static void usage(void);

#define USEAPP(s, a) \
    if (flags) printf("%s: use '%s' to view this information\n", s, a)


int
main(int argc, char *argv[])
{
	char *fn = NULL;
	int ch, lvl1;

	while ((ch = getopt(argc, argv, "Aaf:nqw")) != -1) {
		switch (ch) {

		case 'A':
			Aflag = 1;
			break;

		case 'a':
			aflag = 1;
			break;

		case 'f':
			fn = optarg;
			wflag = 1;
			break;

		case 'n':
			nflag = 1;
			break;

		case 'q':
			qflag = 1;
			break;

		case 'w':
			wflag = 1;
			break;

		default:
			usage();
		}
	}

	if (qflag && !wflag)
		usage();

	argc -= optind;
	argv += optind;

	if (Aflag || aflag) {
		debuginit();
		for (lvl1 = 1; lvl1 < CTL_MAXID; lvl1++)
			listall(topname[lvl1].ctl_name, &secondlevel[lvl1]);
		return 0;
	}

	if (fn) {
		FILE *fp;
		char *l;

		fp = fopen(fn, "r");
		if (fp == NULL) {
			err(1, "%s", fn);
		} else {
			for (; (l = fparseln(fp, NULL, NULL, NULL, 0)) != NULL;
			    free(l)) {
				if (*l)
					parse(l, 1);
			}
			fclose(fp);
		}
	} else {
		if (argc == 0)
			usage();
		while (argc-- > 0)
			parse(*argv++, 1);
	}
	return 0;
}

/*
 * List all variables known to the system.
 */
static void
listall(const char *prefix, struct list *lp)
{
	int lvl2;
	char *cp, name[BUFSIZ];

	if (lp->list == 0)
		return;
	strcpy(name, prefix);
	cp = &name[strlen(name)];
	*cp++ = '.';
	for (lvl2 = 0; lvl2 < lp->size; lvl2++) {
		if (lp->list[lvl2].ctl_name == 0)
			continue;
		strcpy(cp, lp->list[lvl2].ctl_name);
		parse(name, Aflag);
	}
}

/*
 * Parse a name into a MIB entry.
 * Lookup and print out the MIB entry if it exists.
 * Set a new value if requested.
 */
static void
parse(char *string, int flags)
{
	int indx, type, state, len;
	int special = 0;
	void *newval = 0;
	int intval, newsize = 0;
	quad_t quadval;
	size_t size;
	struct list *lp;
	int mib[CTL_MAXNAME];
	char *cp, *bufp, buf[BUFSIZ];
	double loads[3];

	bufp = buf;
	snprintf(buf, BUFSIZ, "%s", string);
	if ((cp = strchr(string, '=')) != NULL) {
		if (!wflag)
			errx(2, "Must specify -w to set variables");
		*strchr(buf, '=') = '\0';
		*cp++ = '\0';
		while (isspace((unsigned char) *cp))
			cp++;
		newval = cp;
		newsize = strlen(cp);
	}
	if ((indx = findname(string, "top", &bufp, &toplist)) == -1)
		return;
	mib[0] = indx;
	if (indx == CTL_DEBUG)
		debuginit();
	if (mib[0] == CTL_PROC) {
		type = CTLTYPE_NODE;
		len = 1;
	} else {
		lp = &secondlevel[indx];
		if (lp->list == 0) {
			warnx("Class `%s' is not implemented",
			    topname[indx].ctl_name);
			return;
		}
		if (bufp == NULL) {
			listall(topname[indx].ctl_name, lp);
			return;
		}
		if ((indx = findname(string, "second", &bufp, lp)) == -1)
			return;
		mib[1] = indx;
		type = lp->list[indx].ctl_type;
		len = 2;
	}
	switch (mib[0]) {

	case CTL_KERN:
		switch (mib[1]) {
		case KERN_PROF:
			mib[2] = GPROF_STATE;
			size = sizeof state;
			if (sysctl(mib, 3, &state, &size, NULL, 0) < 0) {
				if (flags == 0)
					return;
				if (!nflag)
					printf("%s: ", string);
				printf(
				    "kernel is not compiled for profiling\n");
				return;
			}
			if (!nflag)
				printf("%s: %s\n", string,
				    state == GMON_PROF_OFF ? "off" : "running");
			return;
		case KERN_VNODE:
		case KERN_FILE:
			USEAPP(string, "pstat");
			return;
		case KERN_PROC:
		case KERN_PROC2:
		case KERN_PROC_ARGS:
			USEAPP(string, "ps");
			return;
		case KERN_CLOCKRATE:
			special |= CLOCK;
			break;
		case KERN_BOOTTIME:
			special |= BOOTTIME;
			break;
		case KERN_NTPTIME:
			USEAPP(string, "ntpdc -c kerninfo");
			return;
		case KERN_MBUF:
			len = sysctl_3rd(&mbufvars, string, &bufp, mib, flags,
			    &type);
			if (len < 0)
				return;
			break;
		case KERN_CP_TIME:
			special |= CPTIME;
			break;
		case KERN_MSGBUF:
			USEAPP(string, "dmesg");
			return;
		case KERN_CONSDEV:
			special |= CONSDEV;
			break;
		case KERN_PIPE:
			len = sysctl_3rd(&pipevars, string, &bufp, mib, flags,
			    &type);
			if (len < 0)
				return;
			break;
		case KERN_TKSTAT:
			len = sysctl_3rd(&tkstatvars, string, &bufp, mib, flags,
			    &type);
			if (len < 0)
				return;
			break;
		}
		break;

	case CTL_HW:
		switch (mib[1]) {
		case HW_DISKSTATS:
			USEAPP(string, "iostat");
			return;
		}
		break;

	case CTL_VM:
		switch (mib[1]) {
		case VM_LOADAVG:
			getloadavg(loads, 3);
			if (!nflag)
				printf("%s: ", string);
			printf("%.2f %.2f %.2f\n", loads[0], loads[1],
			    loads[2]);
			return;

		case VM_METER:
		case VM_UVMEXP:
		case VM_UVMEXP2:
			USEAPP(string, "vmstat' or 'systat");
			return;
		}
		break;

	case CTL_NET:
		if (mib[1] == PF_INET) {
			len = sysctl_inet(string, &bufp, mib, flags, &type);
			if (len >= 0)
				break;
			return;
		}
#ifdef INET6
		else if (mib[1] == PF_INET6) {
			len = sysctl_inet6(string, &bufp, mib, flags, &type);
			if (len >= 0)
				break;
			return;
		}
#endif /* INET6 */
#ifdef IPSEC
		else if (mib[1] == PF_KEY) {
			len = sysctl_3rd(&keyvars, string, &bufp, mib, flags,
			    &type);
			if (len >= 0)
				break;
			return;
		}
#endif /* IPSEC */
		if (flags == 0)
			return;
		USEAPP(string, "netstat");
		return;

	case CTL_DEBUG:
		mib[2] = CTL_DEBUG_VALUE;
		len = 3;
		break;

	case CTL_MACHDEP:
#ifdef CPU_CONSDEV
		if (mib[1] == CPU_CONSDEV)
			special |= CONSDEV;
#endif
#ifdef CPU_DISKINFO
		if (mib[1] == CPU_DISKINFO)
			special |= DISKINFO;
#endif
		break;

	case CTL_VFS:
		if (mib[1] == VFS_GENERIC) {
			len = sysctl_3rd(&vfsgenvars, string, &bufp, mib, flags,
			    &type);
			/* Don't bother with VFS_CONF. */
			if (mib[2] == VFS_CONF)
				len = -1;
		} else
			len = sysctl_vfs(string, &bufp, mib, flags, &type);
		if (len < 0)
			return;

		/* XXX Special-case for NFS stats. */
		if (mib[1] == 2 && mib[2] == NFS_NFSSTATS) {
			USEAPP(string, "nfsstat");
			return;
		}
		break;

	case CTL_VENDOR:
	case CTL_USER:
	case CTL_DDB:
		break;
	case CTL_PROC:
		len = sysctl_proc(string, &bufp, mib, flags, &type);
		if (len < 0)
			return;
		break;
	case CTL_EMUL:
		switch (mib[1]) {
		case EMUL_IRIX:
		    len = sysctl_irix(string, &bufp, mib, flags, &type);
		    break;

		case EMUL_LINUX:
		    len = sysctl_linux(string, &bufp, mib, flags, &type);
		    break;

		case EMUL_DARWIN:
		    len = sysctl_darwin(string, &bufp, mib, flags, &type);
		    break;

		default:
		    warnx("Illegal emul level value: %d", mib[0]);
		    break;
		}
		if (len < 0)
			return;
		break;
	default:
		warnx("Illegal top level value: %d", mib[0]);
		return;

	}
	if (bufp) {
		warnx("Name %s in %s is unknown", bufp, string);
		return;
	}
	if (newsize > 0) {
		switch (type) {
		case CTLTYPE_INT:
			intval = atoi(newval);
			newval = &intval;
			newsize = sizeof intval;
			break;

		case CTLTYPE_LIMIT:
			if (strcmp(newval, "unlimited") == 0) {
				quadval = RLIM_INFINITY;
				newval = &quadval;
				newsize = sizeof quadval;
				break;
			}
			/* FALLTHROUGH */
		case CTLTYPE_QUAD:
			sscanf(newval, "%lld", (long long *)&quadval);
			newval = &quadval;
			newsize = sizeof quadval;
			break;
		}
	}
	size = BUFSIZ;
	if (sysctl(mib, len, buf, &size, newsize ? newval : 0, newsize) == -1) {
		if (flags == 0)
			return;
		switch (errno) {
		case EOPNOTSUPP:
			printf("%s: the value is not available\n", string);
			return;
		case ENOTDIR:
			printf("%s: the specification is incomplete\n", string);
			return;
		case ENOMEM:
			printf("%s: this type is unknown to this program\n",
			    string);
			return;
		default:
			printf("%s: sysctl() failed with %s\n",
			    string, strerror(errno));
			return;
		}
	}
	if (qflag && (newsize > 0))
		return;
	if (special & CLOCK) {
		struct clockinfo *clkp = (struct clockinfo *)buf;

		if (!nflag)
			printf("%s: ", string);
		printf(
		    "tick = %d, tickadj = %d, hz = %d, profhz = %d, stathz = %d\n",
		    clkp->tick, clkp->tickadj, clkp->hz, clkp->profhz, clkp->stathz);
		return;
	}
	if (special & BOOTTIME) {
		struct timeval *btp = (struct timeval *)buf;
		time_t boottime;

		if (!nflag) {
			boottime = btp->tv_sec;
			/* ctime() provides the trailing newline */
			printf("%s = %s", string, ctime(&boottime));
		} else
			printf("%ld\n", (long) btp->tv_sec);
		return;
	}
	if (special & CONSDEV) {
		dev_t dev = *(dev_t *)buf;

		if (!nflag)
			printf("%s = %s\n", string, devname(dev, S_IFCHR));
		else
			printf("0x%x\n", dev);
		return;
	}
	if (special & DISKINFO) {
#ifdef CPU_DISKINFO
		/* Don't know a good way to deal with this i386 specific one */
		/* OTOH this does pass the info out... */
		struct disklist *dl = (void *)buf;
		struct biosdisk_info *bi;
		struct nativedisk_info *ni;
		uint i, b, lim;
		if (!nflag)
			printf("%s: ", string);
		lim = dl->dl_nbiosdisks;
		if (lim > MAX_BIOSDISKS)
			lim = MAX_BIOSDISKS;
		for (bi = dl->dl_biosdisks, i = 0; i < lim; bi++, i++)
			printf("%x:%lld(%d/%d/%d),%x ",
				bi->bi_dev, bi->bi_lbasecs, bi->bi_cyl,
				bi->bi_head, bi->bi_sec, bi->bi_flags);
		lim = dl->dl_nnativedisks;
		ni = dl->dl_nativedisks;
		bi = dl->dl_biosdisks;
		if ((char *)&ni[lim] != (char *)buf + size) {
			printf("size mismatch\n");
			return;
		}
		for (i = 0; i < lim; ni++, i++) {
			char sep = ':';
			printf(" %.*s", (int)sizeof ni->ni_devname,
				ni->ni_devname);
			for (b = 0; b < ni->ni_nmatches; sep = ',', b++)
				printf("%c%x", sep,
					bi[ni->ni_biosmatches[b]].bi_dev);
		}
		printf("\n");
#endif
		return;
	}
	if (special & CPTIME) {
		u_int64_t *cp_time = (u_int64_t *)buf;

		if (!nflag)
			printf("%s: ", string);
		printf("user = %llu, nice = %llu, sys = %llu, intr = %llu, "
		    "idle = %llu\n", (unsigned long long) cp_time[0],
		    (unsigned long long) cp_time[1],
		    (unsigned long long) cp_time[2],
		    (unsigned long long) cp_time[3],
		    (unsigned long long) cp_time[4]);
		return;
	}

	switch (type) {
	case CTLTYPE_INT:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%d\n", *(int *)buf);
		} else {
			if (!nflag)
				printf("%s: %d -> ", string, *(int *)buf);
			printf("%d\n", *(int *)newval);
		}
		return;

	case CTLTYPE_STRING:
		/* If sysctl() didn't return any data, don't print a string. */
		if (size == 0)
			buf[0] = '\0';
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%s\n", buf);
		} else {
			if (!nflag)
				printf("%s: %s -> ", string, buf);
			printf("%s\n", (char *) newval);
		}
		return;

	case CTLTYPE_LIMIT:
#define PRINTF_LIMIT(lim) { \
if ((lim) == RLIM_INFINITY) \
	printf("unlimited");\
else \
	printf("%lld", (long long)(lim)); \
}

		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			PRINTF_LIMIT((long long)(*(quad_t *)buf));
		} else {
			if (!nflag) {
				printf("%s: ", string);
				PRINTF_LIMIT((long long)(*(quad_t *)buf));
				printf(" -> ");
			}
			PRINTF_LIMIT((long long)(*(quad_t *)newval));
		}
		printf("\n");
		return;
#undef PRINTF_LIMIT

	case CTLTYPE_QUAD:
		if (newsize == 0) {
			if (!nflag)
				printf("%s = ", string);
			printf("%lld\n", (long long)(*(quad_t *)buf));
		} else {
			if (!nflag)
				printf("%s: %lld -> ", string,
				    (long long)(*(quad_t *)buf));
			printf("%lld\n", (long long)(*(quad_t *)newval));
		}
		return;

	case CTLTYPE_STRUCT:
		warnx("%s: unknown structure returned", string);
		return;

	default:
	case CTLTYPE_NODE:
		warnx("%s: unknown type returned", string);
		return;
	}
}

/*
 * Initialize the set of debugging names
 */
static void
debuginit(void)
{
	int mib[3], loc, i;
	size_t size;

	if (secondlevel[CTL_DEBUG].list != 0)
		return;
	secondlevel[CTL_DEBUG].list = debugname;
	mib[0] = CTL_DEBUG;
	mib[2] = CTL_DEBUG_NAME;
	for (loc = 0, i = 0; i < CTL_DEBUG_MAXID; i++) {
		mib[1] = i;
		size = BUFSIZ - loc;
		if (sysctl(mib, 3, &names[loc], &size, NULL, 0) == -1)
			continue;
		debugname[i].ctl_name = &names[loc];
		debugname[i].ctl_type = CTLTYPE_INT;
		loc += size;
	}
}

struct ctlname inetname[] = CTL_IPPROTO_NAMES;
struct ctlname ipname[] = IPCTL_NAMES;
struct ctlname icmpname[] = ICMPCTL_NAMES;
struct ctlname tcpname[] = TCPCTL_NAMES;
struct ctlname udpname[] = UDPCTL_NAMES;
#ifdef IPSEC
struct ctlname ipsecname[] = IPSECCTL_NAMES;
#endif
struct list inetlist = { inetname, IPPROTO_MAXID };
struct list inetvars[] = {
/*0*/	{ ipname, IPCTL_MAXID },	/* ip */
	{ icmpname, ICMPCTL_MAXID },	/* icmp */
	{ 0, 0 },			/* igmp */
	{ 0, 0 },			/* ggmp */
	{ 0, 0 },
	{ 0, 0 },
	{ tcpname, TCPCTL_MAXID },	/* tcp */
	{ 0, 0 },
	{ 0, 0 },			/* egp */
	{ 0, 0 },
/*10*/	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },			/* pup */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ udpname, UDPCTL_MAXID },	/* udp */
	{ 0, 0 },
	{ 0, 0 },
/*20*/	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },			/* idp */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
/*30*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*40*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
#ifdef IPSEC
	{ ipsecname, IPSECCTL_MAXID },	/* esp - for backward compatibility */
	{ ipsecname, IPSECCTL_MAXID },	/* ah */
#else
	{ 0, 0 },
	{ 0, 0 },
#endif
};

/*
 * handle internet requests
 */
static int
sysctl_inet(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	struct list *lp;
	int indx;

	if (*bufpp == NULL) {
		listall(string, &inetlist);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, &inetlist)) == -1)
		return (-1);
	mib[2] = indx;
	if (indx <= IPPROTO_MAXID && inetvars[indx].list != NULL)
		lp = &inetvars[indx];
	else if (!flags)
		return (-1);
	else {
		printf("%s: no variables defined for protocol\n", string);
		return (-1);
	}
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "fourth", bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	*typep = lp->list[indx].ctl_type;
	return (4);
}

#ifdef INET6
struct ctlname inet6name[] = CTL_IPV6PROTO_NAMES;
struct ctlname ip6name[] = IPV6CTL_NAMES;
struct ctlname icmp6name[] = ICMPV6CTL_NAMES;
struct ctlname udp6name[] = UDP6CTL_NAMES;
struct ctlname pim6name[] = PIM6CTL_NAMES;
struct ctlname ipsec6name[] = IPSEC6CTL_NAMES;
struct list inet6list = { inet6name, IPV6PROTO_MAXID };
struct list inet6vars[] = {
/*0*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 },
	{ tcpname, TCPCTL_MAXID },	/* tcp6 */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
/*10*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ udp6name, UDP6CTL_MAXID },	/* udp6 */
	{ 0, 0 },
	{ 0, 0 },
/*20*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*30*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*40*/	{ 0, 0 },
	{ ip6name, IPV6CTL_MAXID },	/* ipv6 */
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
#ifdef IPSEC
/*50*/	{ ipsec6name, IPSECCTL_MAXID },	/* esp6 - for backward compatibility */
	{ ipsec6name, IPSECCTL_MAXID },	/* ah6 */
#else
	{ 0, 0 },
	{ 0, 0 },
#endif
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ icmp6name, ICMPV6CTL_MAXID },	/* icmp6 */
	{ 0, 0 },
/*60*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*70*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*80*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*90*/	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
/*100*/	{ 0, 0 },
	{ 0, 0 },
	{ 0, 0 },
	{ pim6name, PIM6CTL_MAXID },	/* pim6 */
};

/*
 * handle internet6 requests
 */
static int
sysctl_inet6(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	struct list *lp;
	int indx;

	if (*bufpp == NULL) {
		listall(string, &inet6list);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, &inet6list)) == -1)
		return (-1);
	mib[2] = indx;
	if (indx <= sizeof(inet6vars)/sizeof(inet6vars[0])
	 && inet6vars[indx].list != NULL) {
		lp = &inet6vars[indx];
	} else if (!flags) {
		return (-1);
	} else {
		fprintf(stderr, "%s: no variables defined for this protocol\n",
		    string);
		return (-1);
	}
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "fourth", bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	*typep = lp->list[indx].ctl_type;
	return (4);
}
#endif /* INET6 */

struct ctlname ffsname[] = FFS_NAMES;
struct ctlname nfsname[] = NFS_NAMES;
struct list vfsvars[] = {
	{ 0, 0 },			/* generic */
	{ ffsname, FFS_MAXID },		/* FFS */
	{ nfsname, NFS_MAXID },		/* NFS */
	{ 0, 0 },			/* MFS */
	{ 0, 0 },			/* MSDOS */
	{ 0, 0 },			/* LFS */
	{ 0, 0 },			/* old LOFS */
	{ 0, 0 },			/* FDESC */
	{ 0, 0 },			/* PORTAL */
	{ 0, 0 },			/* NULL */
	{ 0, 0 },			/* UMAP */
	{ 0, 0 },			/* KERNFS */
	{ 0, 0 },			/* PROCFS */
	{ 0, 0 },			/* AFS */
	{ 0, 0 },			/* CD9660 */
	{ 0, 0 },			/* UNION */
	{ 0, 0 },			/* ADOSFS */
	{ 0, 0 },			/* EXT2FS */
	{ 0, 0 },			/* CODA */
	{ 0, 0 },			/* FILECORE */
};

/*
 * handle vfs requests
 */
static int
sysctl_vfs(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	struct list *lp = &vfsvars[mib[1]];
	int indx;

	if (lp->list == NULL) {
		if (flags)
			printf("%s: no variables defined for file system\n",
			    string);
		return (-1);
	}
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, lp)) == -1)
		return (-1);
	mib[2] = indx;
	*typep = lp->list[indx].ctl_type;
	return (3);
}

/*
 * handle 3rd level requests.
 */
static int
sysctl_3rd(struct list *lp, char *string, char **bufpp, int mib[], int flags,
    int *typep)
{
	int indx;

	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, lp)) == -1)
		return (-1);
	mib[2] = indx;
	*typep = lp->list[indx].ctl_type;
	return (3);
}

struct ctlname procnames[] = PROC_PID_NAMES;
struct list procvars = {procnames, PROC_PID_MAXID};
struct ctlname proclimitnames[] = PROC_PID_LIMIT_NAMES;
struct list proclimitvars = {proclimitnames, PROC_PID_LIMIT_MAXID};
struct ctlname proclimittypenames[] = PROC_PID_LIMIT_TYPE_NAMES;
struct list proclimittypevars = {proclimittypenames,
    PROC_PID_LIMIT_TYPE_MAXID};
/*
 * handle kern.proc requests
 */
static int
sysctl_proc(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	char *cp, name[BUFSIZ];
	struct list *lp;
	int indx;

	if (*bufpp == NULL) {
		strcpy(name, string);
		cp = &name[strlen(name)];
		*cp++ = '.';
		strcpy(cp, "curproc");
		parse(name, Aflag);
		return (-1);
	}
	cp = strsep(bufpp, ".");
	if (cp == NULL) {
		warnx("%s: incomplete specification", string);
		return (-1);
	}
	if (strcmp(cp, "curproc") == 0) {
		mib[1] = PROC_CURPROC;
	} else {
		mib[1] = atoi(cp);
		if (mib[1] == 0) {
			warnx("second level name %s in %s is invalid", cp,
			    string);
			return (-1);
		}
	}
	*typep = CTLTYPE_NODE;
	lp = &procvars;
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, lp)) == -1)
		return (-1);
	mib[2] = indx;
	*typep = lp->list[indx].ctl_type;
	if (*typep != CTLTYPE_NODE)
		return(3);
	lp = &proclimitvars;
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "fourth", bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	lp = &proclimittypevars;
	if (*bufpp == NULL) {
		listall(string, lp);
		return (-1);
	}
	if ((indx = findname(string, "fifth", bufpp, lp)) == -1)
		return (-1);
	mib[4] = indx;
	*typep = CTLTYPE_LIMIT;
	return(5);
}

struct ctlname linuxnames[] = EMUL_LINUX_NAMES;
struct list linuxvars = { linuxnames, EMUL_LINUX_MAXID };
struct ctlname linuxkernnames[] = EMUL_LINUX_KERN_NAMES;
struct list linuxkernvars = { linuxkernnames, EMUL_LINUX_KERN_MAXID };

static int
sysctl_linux(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	struct list *lp = &linuxvars;
	int indx;
	char name[BUFSIZ], *cp;

	if (*bufpp == NULL) {
		(void)strcpy(name, string);
		cp = &name[strlen(name)];
		*cp++ = '.';
		(void)strcpy(cp, "kern");
		listall(name, &linuxkernvars);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, lp)) == -1)
		return (-1);
	mib[2] = indx;
	lp = &linuxkernvars;
	*typep = lp->list[indx].ctl_type;
	if ((indx = findname(string, "fourth", bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	*typep = lp->list[indx].ctl_type;
	return (4);
}

struct ctlname irixnames[] = EMUL_IRIX_NAMES;
struct list irixvars = { irixnames, EMUL_IRIX_MAXID };
struct ctlname irixkernnames[] = EMUL_IRIX_KERN_NAMES;
struct list irixkernvars = { irixkernnames, EMUL_IRIX_KERN_MAXID };

static int
sysctl_irix(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	struct list *lp = &irixvars;
	int indx;
	char name[BUFSIZ], *cp;

	if (*bufpp == NULL) {
		(void)strcpy(name, string);
		cp = &name[strlen(name)];
		*cp++ = '.';
		(void)strcpy(cp, "kern");
		listall(name, &irixkernvars);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, lp)) == -1)
		return (-1);
	mib[2] = indx;
	lp = &irixkernvars;
	*typep = lp->list[indx].ctl_type;
	if ((indx = findname(string, "fourth", bufpp, lp)) == -1)
		return (-1);
	mib[3] = indx;
	*typep = lp->list[indx].ctl_type;
	return (4);
}

struct ctlname darwinnames[] = EMUL_DARWIN_NAMES;
struct list darwinvars = { darwinnames, EMUL_DARWIN_MAXID };

static int
sysctl_darwin(char *string, char **bufpp, int mib[], int flags, int *typep)
{
	struct list *lp = &darwinvars;
	int indx;

	if (*bufpp == NULL) {
		listall(string, &darwinvars);
		return (-1);
	}
	if ((indx = findname(string, "third", bufpp, lp)) == -1)
		return (-1);
	mib[2] = indx;
	lp = &darwinvars;
	*typep = lp->list[indx].ctl_type;
	return (3);
}

/*
 * Scan a list of names searching for a particular name.
 */
static int
findname(char *string, char *level, char **bufp, struct list *namelist)
{
	char *name;
	int i;

	if (namelist->list == 0 || (name = strsep(bufp, ".")) == NULL) {
		warnx("%s: incomplete specification", string);
		return (-1);
	}
	for (i = 0; i < namelist->size; i++)
		if (namelist->list[i].ctl_name != NULL &&
		    strcmp(name, namelist->list[i].ctl_name) == 0)
			break;
	if (i == namelist->size) {
		warnx("%s level name %s in %s is invalid",
		    level, name, string);
		return (-1);
	}
	return (i);
}

static void
usage(void)
{
	const char *progname = getprogname();

	(void)fprintf(stderr,
	    "Usage:\t%s %s\n\t%s %s\n\t%s %s\n\t%s %s\n\t%s %s\n",
	    progname, "[-n] variable ...",
	    progname, "[-n] [-q] -w variable=value ...",
	    progname, "[-n] -a",
	    progname, "[-n] -A",
	    progname, "[-n] [-q] -f file");
	exit(1);
}
