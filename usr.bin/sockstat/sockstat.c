/*	$NetBSD: sockstat.c,v 1.14.8.1 2009/05/13 19:20:05 jym Exp $ */

/*
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Brown.
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
__RCSID("$NetBSD: sockstat.c,v 1.14.8.1 2009/05/13 19:20:05 jym Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <net/route.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_pcb_hdr.h>
#include <netinet/tcp_fsm.h>

#define _KERNEL
/* want DTYPE_* defines */
#include <sys/file.h>
#undef _KERNEL

#include <arpa/inet.h>

#include <bitstring.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#define satosun(sa)	((struct sockaddr_un *)(sa))
#define satosin(sa)	((struct sockaddr_in *)(sa))
#ifdef INET6
#define satosin6(sa)	((struct sockaddr_in6 *)(sa))
#endif

void	parse_ports(const char *);
int	get_num(const char *, const char **, const char **);
void	get_sockets(const char *);
void	get_files(void);
int	sort_files(const void *, const void *);
void	sysctl_sucker(int *, u_int, void **, size_t *);
void	socket_add_hash(struct kinfo_pcb *, int);
int	isconnected(struct kinfo_pcb *);
int	islistening(struct kinfo_pcb *);
struct kinfo_pcb *pick_socket(struct kinfo_file *);
int	get_proc(struct kinfo_proc2 *, int);
int	print_socket(struct kinfo_file *, struct kinfo_pcb *,
		     struct kinfo_proc2 *);
void	print_addr(int, int, int, struct sockaddr *); 

LIST_HEAD(socklist, sockitem);
#define HASHSIZE 1009
struct socklist sockhash[HASHSIZE];
struct sockitem {
	LIST_ENTRY(sockitem) s_list;
	struct kinfo_pcb *s_sock;
};

struct kinfo_file *flist;
size_t flistc;

int pf_list, only, nonames;
bitstr_t *portmap;

#define PF_LIST_INET	1
#ifdef INET6
#define PF_LIST_INET6	2
#endif
#define PF_LIST_LOCAL	4
#define ONLY_CONNECTED	1
#define ONLY_LISTEN	2

int
main(int argc, char *argv[])
{
	struct kinfo_pcb *kp;
	int ch;
	size_t i;
	struct kinfo_proc2 p;

	pf_list = only = 0;

#ifdef INET6
	while ((ch = getopt(argc, argv, "46cf:lnp:u")) != - 1) {
#else
	while ((ch = getopt(argc, argv, "4cf:lnp:u")) != - 1) {
#endif
		switch (ch) {
		case '4':
			pf_list |= PF_LIST_INET;
			break;
#ifdef INET6
		case '6':
			pf_list |= PF_LIST_INET6;
			break;
#endif
		case 'c':
			only |= ONLY_CONNECTED;
			break;
		case 'f':
			if (strcasecmp(optarg, "inet") == 0)
				pf_list |= PF_LIST_INET;
#ifdef INET6
			else if (strcasecmp(optarg, "inet6") == 0)
				pf_list |= PF_LIST_INET6;
#endif
			else if (strcasecmp(optarg, "local") == 0)
				pf_list |= PF_LIST_LOCAL;
			else if (strcasecmp(optarg, "unix") == 0)
				pf_list |= PF_LIST_LOCAL;
			else
				errx(1, "%s: unsupported protocol family",
				    optarg);
			break;
		case 'l':
			only |= ONLY_LISTEN;
			break;
		case 'n':
			nonames++;
			break;
		case 'p':
			parse_ports(optarg);
			break;
		case 'u':
			pf_list |= PF_LIST_LOCAL;
			break;
		default:
			/* usage(); */
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	if ((portmap != NULL) && (pf_list == 0)) {
		pf_list = PF_LIST_INET;
#ifdef INET6
		pf_list |= PF_LIST_INET6;
#endif
	}
	if (pf_list == 0) {
		pf_list = PF_LIST_INET | PF_LIST_LOCAL;
#ifdef INET6
		pf_list |= PF_LIST_INET6;
#endif
	}
	if ((portmap != NULL) && (pf_list & PF_LIST_LOCAL))
		errx(1, "local domain sockets do not have ports");

	if (pf_list & PF_LIST_INET) {
		get_sockets("net.inet.tcp.pcblist");
		get_sockets("net.inet.udp.pcblist");
		if (portmap == NULL)
			get_sockets("net.inet.raw.pcblist");
	}

#ifdef INET6
	if (pf_list & PF_LIST_INET6) {
		get_sockets("net.inet6.tcp6.pcblist");
		get_sockets("net.inet6.udp6.pcblist");
		if (portmap == NULL)
			get_sockets("net.inet6.raw6.pcblist");
	}
#endif

	if (pf_list & PF_LIST_LOCAL) {
		get_sockets("net.local.stream.pcblist");
		get_sockets("net.local.dgram.pcblist");
	}

	get_files();

	p.p_pid = 0;
	for (i = 0; i < flistc; i++)
		if ((kp = pick_socket(&flist[i])) != NULL &&
		    get_proc(&p, flist[i].ki_pid) == 0)
			print_socket(&flist[i], kp, &p);

	return (0);
}

void
parse_ports(const char *l)
{
	struct servent *srv;
	const char *s, *e;
	long i, j;

	if (portmap == NULL) {
		portmap = bit_alloc(65536);
		if (portmap == NULL)
			err(1, "malloc");
	}

	if ((srv = getservbyname(l, NULL)) != NULL) {
		bit_set(portmap, ntohs(srv->s_port));
		return;
	}

	s = e = l;
	while (*s != '\0') {
		i = get_num(l, &s, &e);
		switch (*e) {
		case ',':
			e++;
		case '\0':
			bit_set(portmap, i);
			s = e;
			continue;
		case '-':
			s = ++e;
			j = get_num(l, &s, &e);
			for (; i <= j; i++)
				bit_set(portmap, i);
			break;
		default:
			errno = EINVAL;
			err(1, "%s", l);
		}
	}
}

int
get_num(const char *l, const char **s, const char **e)
{
	long x;
	char *t;

	while (isdigit((u_int)**e))
		(*e)++;
	if (*s != *e) {
		errno = 0;
		x = strtol(*s, &t, 0);
		if (errno == 0 && x >= 0 && x <= 65535 && t == *e)
			return (x);
	}

	errno = EINVAL;
	err(1, "%s", l);
}

void
get_sockets(const char *mib)
{
	void *v;
	size_t sz;
	int rc, n, name[CTL_MAXNAME];
	u_int namelen;

	sz = CTL_MAXNAME;
	rc = sysctlnametomib(mib, &name[0], &sz);
	if (rc == -1) {
		if (errno == ENOENT)
			return;
		err(1, "sysctlnametomib: %s", mib);
	}
	namelen = sz;

	name[namelen++] = PCB_ALL;
	name[namelen++] = 0;		/* XXX all pids */
	name[namelen++] = sizeof(struct kinfo_pcb);
	name[namelen++] = INT_MAX;	/* all of them */

	sysctl_sucker(&name[0], namelen, &v, &sz);
	n = sz / sizeof(struct kinfo_pcb);
	socket_add_hash(v, n);
}

void
get_files(void)
{
	void *v;
	size_t sz;
	int rc, name[CTL_MAXNAME];
	u_int namelen;

	sz = CTL_MAXNAME;
	rc = sysctlnametomib("kern.file2", &name[0], &sz);
	if (rc == -1)
		err(1, "sysctlnametomib");
	namelen = sz;

	name[namelen++] = KERN_FILE_BYPID;
	name[namelen++] = 0;		/* XXX all pids */
	name[namelen++] = sizeof(struct kinfo_file);
	name[namelen++] = INT_MAX;	/* all of them */

	sysctl_sucker(&name[0], namelen, &v, &sz);
	flist = v;
	flistc = sz / sizeof(struct kinfo_file);

	qsort(flist, flistc, sizeof(*flist), sort_files);
}

int
sort_files(const void *a, const void *b)
{
	const struct kinfo_file *ka = a, *kb = b;

	if (ka->ki_pid == kb->ki_pid)
		return (ka->ki_fd - kb->ki_fd);

	return (ka->ki_pid - kb->ki_pid);
}

void
sysctl_sucker(int *name, u_int namelen, void **vp, size_t *szp)
{
	int rc;
	void *v;
	size_t sz;

	/* printf("name %p, namelen %u\n", name, namelen); */

	v = NULL;
	sz = 0;
	do {
		rc = sysctl(&name[0], namelen, v, &sz, NULL, 0);
		if (rc == -1 && errno != ENOMEM)
			err(1, "sysctl");
		if (rc == -1 && v != NULL) {
			free(v);
			v = NULL;
		}
		if (v == NULL) {
			v = malloc(sz);
			rc = -1;
		}
		if (v == NULL)
			err(1, "malloc");
	} while (rc == -1);

	*vp = v;
	*szp = sz;
	/* printf("got %zu at %p\n", sz, v); */
}

void
socket_add_hash(struct kinfo_pcb *kp, int n)
{
	struct sockitem *si;
	int hash, i;

	if (n == 0)
		return;

	si = malloc(sizeof(*si) * n);
	if (si== NULL)
		err(1, "malloc");

	for (i = 0; i < n; i++) {
		si[i].s_sock = &kp[i];
		hash = (int)(kp[i].ki_sockaddr % HASHSIZE);
		LIST_INSERT_HEAD(&sockhash[hash], &si[i], s_list);
	}
}

int
isconnected(struct kinfo_pcb *kp)
{

	if ((kp->ki_sostate & SS_ISCONNECTED) ||
	    (kp->ki_prstate >= INP_CONNECTED) ||
	    (kp->ki_tstate > TCPS_LISTEN) ||
	    (kp->ki_conn != 0))
		return (1);

	return (0);
}

int
islistening(struct kinfo_pcb *kp)
{

	if (isconnected(kp))
		return (0);

	if (kp->ki_tstate == TCPS_LISTEN)
		return (1);

	switch (kp->ki_family) {
	case PF_INET:
		if (kp->ki_type == SOCK_RAW ||
		    (kp->ki_type == SOCK_DGRAM &&
		     ntohs(satosin(&kp->ki_src)->sin_port) != 0))
			return (1);
		break;
#ifdef INET6
	case PF_INET6:
		if (kp->ki_type == SOCK_RAW ||
		    (kp->ki_type == SOCK_DGRAM &&
		     ntohs(satosin6(&kp->ki_src)->sin6_port) != 0))
			return (1);
		break;
#endif
	case PF_LOCAL:
		if (satosun(&kp->ki_src)->sun_path[0] != '\0')
			return (1);
		break;
	default:
		break;
	}

	return (0);
}

struct kinfo_pcb *
pick_socket(struct kinfo_file *f)
{
	struct sockitem *si;
	struct kinfo_pcb *kp;
	int hash;

	if (f->ki_ftype != DTYPE_SOCKET)
		return (NULL);

	hash = (int)(f->ki_fdata % HASHSIZE);
	LIST_FOREACH(si, &sockhash[hash], s_list) {
		if (si->s_sock->ki_sockaddr == f->ki_fdata)
			break;
	}
	if (si == NULL)
		return (NULL);

	kp = si->s_sock;

	if (only) {
		if (isconnected(kp)) {
			/*
			 * connected but you didn't say you wanted
			 * connected sockets
			 */
			if (!(only & ONLY_CONNECTED))
				return (NULL);
		}
		else if (islistening(kp)) {
			/*
			 * listening but you didn't ask for listening
			 * sockets
			 */
			if (!(only & ONLY_LISTEN))
				return (NULL);
		}
		else
			/*
			 * neither connected nor listening, so you
			 * don't get it
			 */
			return (NULL);
	}

	if (portmap) {
		switch (kp->ki_family) {
		case AF_INET:
			if (!bit_test(portmap,
				      ntohs(satosin(&kp->ki_src)->sin_port)) &&
			    !bit_test(portmap,
				      ntohs(satosin(&kp->ki_dst)->sin_port)))
				return (NULL);
			break;
#ifdef INET6
		case AF_INET6:
			if (!bit_test(portmap,
			    ntohs(satosin6(&kp->ki_src)->sin6_port)) &&
			    !bit_test(portmap,
				      ntohs(satosin6(&kp->ki_dst)->sin6_port)))
				return (NULL);
			break;
#endif
		default:
			return (NULL);
		}
	}

	return (kp);
}

int
get_proc(struct kinfo_proc2 *p, int pid)
{
	int name[6];
	u_int namelen;
	size_t sz;

	if (p->p_pid == pid)
		return (0);

	sz = sizeof(*p);
	namelen = 0;
	name[namelen++] = CTL_KERN;
	name[namelen++] = KERN_PROC2;
	name[namelen++] = KERN_PROC_PID;
	name[namelen++] = pid;
	name[namelen++] = sz;
	name[namelen++] = 1;

	return (sysctl(&name[0], namelen, p, &sz, NULL, 0));
}

int
print_socket(struct kinfo_file *kf, struct kinfo_pcb *kp, struct kinfo_proc2 *p)
{
	static int first = 1;
	struct passwd *pw;
	const char *t;
	char proto[22];

	if (first) {
		printf("%-8s " "%-10s "   "%-5s " "%-2s " "%-6s "
		       "%-21s "         "%s\n",
		       "USER", "COMMAND", "PID",  "FD",   "PROTO",
		       "LOCAL ADDRESS", "FOREIGN ADDRESS");
		first = 0;
	}

	if ((pw = getpwuid(p->p_uid)) != NULL)
		printf("%-8s ", pw->pw_name);
	else
		printf("%-8d ", (int)p->p_uid);

	printf("%-10.10s ", p->p_comm);
	printf("%-5d ", (int)kf->ki_pid);
	printf("%2d ", (int)kf->ki_fd);

	snprintf(proto, sizeof(proto), "%d/%d", kp->ki_family, kp->ki_protocol);

	switch (kp->ki_family) {
	case PF_INET:
		switch (kp->ki_protocol) {
		case IPPROTO_TCP:	t = "tcp";	break;
		case IPPROTO_UDP:	t = "udp";	break;
		case IPPROTO_RAW:	t = "raw";	break;
		default:		t = proto;	break;
		}
		break;
#ifdef INET6
	case PF_INET6:
		switch (kp->ki_protocol) {
		case IPPROTO_TCP:	t = "tcp6";	break;
		case IPPROTO_UDP:	t = "udp6";	break;
		case IPPROTO_RAW:	t = "raw6";	break;
		default:		t = proto;	break;
		}
		break;
#endif
	case PF_LOCAL:
		switch (kp->ki_type) {
		case SOCK_STREAM:	t = "stream";	break;
		case SOCK_DGRAM:	t = "dgram";	break;
		case SOCK_RAW:		t = "raw";	break;
		case SOCK_RDM:		t = "rdm";	break;
		case SOCK_SEQPACKET:	t = "seq";	break;
		default:		t = proto;	break;
		}
		break;
	default:
		snprintf(proto, sizeof(proto), "%d/%d/%d",
			 kp->ki_family, kp->ki_type, kp->ki_protocol);
		t = proto;
		break;
	}

	printf("%-6s ", t);

/*
	if (kp->ki_family == PF_LOCAL) {
		if (kp->ki_src.sa_len > 2) {
			print_addr(0, kp->ki_type, kp->ki_pflags, &kp->ki_src);
			if (kp->ki_dst.sa_family == PF_LOCAL)
				printf(" ");
		}
		if (kp->ki_dst.sa_family == PF_LOCAL)
			printf("-> ");
	}
	else */{
		print_addr(21, kp->ki_type, kp->ki_pflags, &kp->ki_src);
		printf(" ");
	}

	if (isconnected(kp))
		print_addr(0, kp->ki_type, kp->ki_pflags, &kp->ki_dst);
	else if (kp->ki_family == PF_INET
#ifdef INET6
	    || kp->ki_family == PF_INET6
#endif
	    )
		printf("%-*s", 0, "*.*");
	/* else if (kp->ki_src.sa_len == 2)
	   printf("%-*s", 0, "-"); */
	else
		printf("-");

	printf("\n");

	return (0);
}

void
print_addr(int l, int t, int f, struct sockaddr *sa)
{
	char sabuf[256], pbuf[32];
	int r = 0;

	if (!(f & INP_ANONPORT))
		f = 0;
	else
		f = NI_NUMERICSERV;
	if (t == SOCK_DGRAM)
		f |= NI_DGRAM;
	if (nonames)
		f |= NI_NUMERICHOST|NI_NUMERICSERV;

	getnameinfo(sa, sa->sa_len, sabuf, sizeof(sabuf),
		    pbuf, sizeof(pbuf), f);

	switch (sa->sa_family) {
	case PF_UNSPEC:
		r = printf("(PF_UNSPEC)");
		break;
	case PF_INET: {
		struct sockaddr_in *si = satosin(sa);
		if (si->sin_addr.s_addr != INADDR_ANY)
			r = printf("%s.%s", sabuf, pbuf);
		else if (ntohs(si->sin_port) != 0)
			r = printf("*.%s", pbuf);
		else
			r = printf("*.*");
		break;
	}
#ifdef INET6
	case PF_INET6: {
		struct sockaddr_in6 *si6 = satosin6(sa);
		if (!IN6_IS_ADDR_UNSPECIFIED(&si6->sin6_addr))
			r = printf("%s.%s", sabuf, pbuf);
		else if (ntohs(si6->sin6_port) != 0)
			r = printf("*.%s", pbuf);
		else
			r = printf("*.*");
		break;
	}
#endif
	case PF_LOCAL: {
		struct sockaddr_un *sun = satosun(sa);
		r = printf("%s", sun->sun_path);
		if (r == 0)
			r = printf("-");
		break;
	}
	default:
		break;
	}

	if (r > 0)
		l -= r;
	if (l > 0)
		printf("%*s", l, "");
}
