/*	$NetBSD: sysctl.c,v 1.86.2.1 2004/04/07 05:04:46 jmc Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 *	All rights reserved.
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
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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
__COPYRIGHT(
"@(#) Copyright (c) 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)sysctl.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: sysctl.c,v 1.86.2.1 2004/04/07 05:04:46 jmc Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sched.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>
#include <netinet/icmp6.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsproto.h>
#include <nfs/nfs.h>
#include <machine/cpu.h>
#include <netkey/key_var.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * this needs to be able to do the printing and the setting
 */
#define HANDLER_PROTO const char *, const char *, char *, \
	int *, u_int, const struct sysctlnode *, \
	u_int, void *
#define HANDLER_ARGS const char *sname, const char *dname, char *value, \
	int *name, u_int namelen, const struct sysctlnode *pnode, \
	u_int type, void *v
#define DISPLAY_VALUE	0
#define DISPLAY_OLD	1
#define DISPLAY_NEW	2

/*
 * generic routines
 */
static struct handlespec *findprinter(const int *, u_int);
static struct handlespec *findwriter(const int *, u_int);
static void print_tree(int *, u_int, struct sysctlnode *, u_int, int);
static void write_number(int *, u_int, struct sysctlnode *, char *);
static void write_string(int *, u_int, struct sysctlnode *, char *);
static void display_number(const struct sysctlnode *, const char *,
			   const void *, size_t, int);
static void display_string(const struct sysctlnode *, const char *,
			   const void *, size_t, int);
static void display_struct(const struct sysctlnode *, const char *,
			   const void *, size_t, int);
static void hex_dump(const unsigned char *, size_t);
static void usage(void);
static void parse(char *);
static void parse_create(char *);
static void parse_destroy(char *);
static void parse_describe(char *);
static void getdesc1(int *, u_int, struct sysctlnode *);
static void getdesc(int *, u_int, struct sysctlnode *);
static void sysctlerror(int);

/*
 * "borrowed" from libc:sysctlgetmibinfo.c
 */
int __learn_tree(int *, u_int, struct sysctlnode *);

/*
 * "handlers"
 */
static void printother(HANDLER_PROTO);
static void kern_clockrate(HANDLER_PROTO);
static void kern_boottime(HANDLER_PROTO);
static void kern_consdev(HANDLER_PROTO);
static void kern_cp_time(HANDLER_PROTO);
static void vm_loadavg(HANDLER_PROTO);
static void proc_limit(HANDLER_PROTO);
#ifdef CPU_DISKINFO
static void machdep_diskinfo(HANDLER_PROTO);
#endif /* CPU_DISKINFO */

struct handlespec {
	int ps_name[CTL_MAXNAME];
	void (*ps_p)(HANDLER_PROTO);
	void (*ps_w)(HANDLER_PROTO);
	void *ps_d;
} handlers[] = {
	{ { CTL_KERN, KERN_CLOCKRATE },	kern_clockrate },
	{ { CTL_KERN, KERN_VNODE },	printother,	NULL,	"pstat" },
	{ { CTL_KERN, KERN_PROC },	printother,	NULL,	"ps" },
	{ { CTL_KERN, KERN_PROC2 },	printother,	NULL,	"ps" },
	{ { CTL_KERN, KERN_PROC_ARGS },	printother,	NULL,	"ps" },
	{ { CTL_KERN, KERN_FILE },	printother,	NULL,	"pstat" },
	{ { CTL_KERN, KERN_NTPTIME },	printother,	NULL,
	  "ntpdc -c kerninfo" },
	{ { CTL_KERN, KERN_MSGBUF },	printother,	NULL,	"dmesg" },
	{ { CTL_KERN, KERN_BOOTTIME },	kern_boottime },
	{ { CTL_KERN, KERN_CONSDEV },	kern_consdev },
	{ { CTL_KERN, KERN_CP_TIME, -1 }, kern_cp_time },
	{ { CTL_KERN, KERN_CP_TIME },	kern_cp_time },
	{ { CTL_KERN, KERN_SYSVIPC_INFO }, printother,	NULL,	"ipcs" },
	{ { CTL_VM,   VM_METER },	printother,	NULL,
	  "vmstat' or 'systat" },
	{ { CTL_VM,   VM_LOADAVG },	vm_loadavg },
	{ { CTL_VM,   VM_UVMEXP },	printother,	NULL,
	  "vmstat' or 'systat" },
	{ { CTL_VM,   VM_UVMEXP2 },	printother,	NULL,
	  "vmstat' or 'systat" },
	{ { CTL_VFS,  2 /* NFS */, NFS_NFSSTATS },
					printother,	NULL,	"nfsstat" },
	{ { CTL_NET },			printother,	NULL,	NULL },
	{ { CTL_NET, PF_LOCAL },	printother,	NULL,	NULL },
	{ { CTL_NET, PF_INET, IPPROTO_TCP, TCPCTL_IDENT },
					printother,	NULL,	"identd" },
	{ { CTL_NET, PF_INET6, IPPROTO_TCP, TCPCTL_IDENT },
					printother,	NULL,	"identd" },

	{ { CTL_NET, PF_INET6, IPPROTO_ICMPV6, ICMPV6CTL_ND6_DRLIST },
					printother,	NULL,	"ndp" },
	{ { CTL_NET, PF_INET6, IPPROTO_ICMPV6, ICMPV6CTL_ND6_PRLIST },
					printother,	NULL,	"ndp" },


	{ { CTL_NET, PF_KEY, KEYCTL_DUMPSA },
					printother,	NULL,	"setkey" },
	{ { CTL_NET, PF_KEY, KEYCTL_DUMPSP },
					printother,	NULL,	"setkey" },
	/* { { CTL_DEBUG },		printother,	NULL,	NULL }, */
	{ { CTL_HW,   HW_DISKSTATS },	printother,	NULL,	"iostat" },
#ifdef CPU_CONSDEV
	{ { CTL_MACHDEP, CPU_CONSDEV },	kern_consdev },
#endif /* CPU_CONSDEV */
#ifdef CPU_DISKINFO
	{ { CTL_MACHDEP, CPU_DISKINFO },machdep_diskinfo },
#endif /* CPU_CONSDEV */
	{ { CTL_DDB },			printother,	NULL,	NULL },
	{ { CTL_PROC, -1, PROC_PID_LIMIT, -1, -1 }, proc_limit, proc_limit },
	{ { CTL_UNSPEC }, },
};

struct sysctlnode my_root = {
#if defined(lint)
	0
#else /* defined(lint) */
	.sysctl_flags = SYSCTL_VERSION|CTLFLAG_ROOT|CTLTYPE_NODE,
	sysc_init_field(_sysctl_size, sizeof(struct sysctlnode)),
	.sysctl_num = 0,
	.sysctl_name = "(prog_root)",
#endif /* defined(lint) */
};

int	Aflag, aflag, dflag, Mflag, nflag, qflag, rflag, wflag, xflag;
int	req;
FILE	*warnfp = stderr;

/*
 * vah-riables n stuff
 */
char gsname[SYSCTL_NAMELEN * CTL_MAXNAME + CTL_MAXNAME],
	gdname[10 * CTL_MAXNAME + CTL_MAXNAME];
char sep[2] = ".", *eq = " = ";
const char *lname[] = {
	"top", "second", "third", "fourth", "fifth", "sixth",
	"seventh", "eighth", "ninth", "tenth", "eleventh", "twelfth"
};

/*
 * you've heard of main, haven't you?
 */
int
main(int argc, char *argv[])
{
	char *fn = NULL;
	int name[CTL_MAXNAME];
	int ch;

	while ((ch = getopt(argc, argv, "Aabdef:Mnqrwx")) != -1) {
		switch (ch) {
		case 'A':
			Aflag++;
			break;
		case 'a':
			aflag++;
			break;
		case 'd':
			dflag++;
			break;
		case 'e':
			eq = "=";
			break;
		case 'f':
			fn = optarg;
			wflag++;
			break;
		case 'M':
			Mflag++;
			break;
		case 'n':
			nflag++;
			break;
		case 'q':
			qflag++;
			break;
		case 'b':	/* FreeBSD compat */
		case 'r':
			rflag++; 
			break;
		case 'w':
			wflag++;
			break;
		case 'x':
			xflag++;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (qflag && !wflag)
		usage();
	if (xflag && rflag)
		usage();
	/* if ((xflag || rflag) && wflag)
		usage(); */
	/* if (aflag && Mflag)
		usage(); */
	if ((Aflag || Mflag || dflag) && argc == 0 && fn == NULL)
		aflag = 1;

	if (Aflag)
		warnfp = stdout;
	req = 0;

	if (aflag) {
		print_tree(&name[0], 0, NULL, CTLTYPE_NODE, 1);
		/* if (argc == 0) */
		return (0);
	}

	if (fn) {
		FILE *fp;
		char *l;

		fp = fopen(fn, "r");
		if (fp == NULL) {
			err(1, "%s", fn);
		} else {
			while ((l = fparseln(fp, NULL, NULL, NULL, 0)) != NULL)
			{
				if (*l) {
					parse(l);
					free(l);
				}
			}
			fclose(fp);
		}
		return (0);
	}

	if (argc == 0)
		usage();

	while (argc-- > 0)
		parse(*argv++);

	return (0);
}

/*
 * ********************************************************************
 * how to find someone special to handle the reading (or maybe even
 * writing) of a particular node
 * ********************************************************************
 */
static struct handlespec *
findprinter(const int *name, u_int namelen)
{
	struct handlespec *p;
	int i, j;

	if (namelen < 1)
		return (NULL);

	p = &handlers[0];
	for (i = 0; p[i].ps_name[0] != CTL_UNSPEC; i++) {
		for (j = 0; j < namelen; j++)
			if (p[i].ps_name[j] != name[j] &&
			    p[i].ps_name[j] != -1)
				break;
		if (j == namelen && p[i].ps_p != NULL)
			return (&p[i]);
	}

	return (NULL);
}

static struct handlespec *
findwriter(const int *name, u_int namelen)
{
	struct handlespec *p;
	int i, j;

        if (namelen < 1)
                return (NULL);

	p = &handlers[0];
	for (i = 0; p[i].ps_name[0] != CTL_UNSPEC; i++) {
		for (j = 0; j < namelen; j++)
			if (p[i].ps_name[j] != name[j] &&
			    p[i].ps_name[j] != -1)
				break;
		if (j == namelen && p[i].ps_w != NULL)
			return (&p[i]);
	}

	return (NULL);
}

/*
 * ********************************************************************
 * convert this special number to a special string so we can print the
 * mib
 * ********************************************************************
 */
static const char *
sf(u_int f)
{
	static char s[256];
	char *c;

	s[0] = '\0';
	c = "";

#define print_flag(_f, _s, _c, _q, _x) \
	if (((_f) & (__CONCAT(CTLFLAG_,_x))) == (__CONCAT(CTLFLAG_,_q))) { \
		strlcat((_s), (_c), sizeof(_s)); \
		strlcat((_s), __STRING(_q), sizeof(_s)); \
		(_c) = ","; \
		(_f) &= ~(__CONCAT(CTLFLAG_,_x)); \
	}
	print_flag(f, s, c, READONLY,  READWRITE);
	print_flag(f, s, c, READONLY1, READWRITE);
	print_flag(f, s, c, READONLY2, READWRITE);
	print_flag(f, s, c, READWRITE, READWRITE);
	print_flag(f, s, c, ANYWRITE,  ANYWRITE);
	print_flag(f, s, c, PRIVATE,   PRIVATE);
	print_flag(f, s, c, PERMANENT, PERMANENT);
	print_flag(f, s, c, OWNDATA,   OWNDATA);
	print_flag(f, s, c, IMMEDIATE, IMMEDIATE);
	print_flag(f, s, c, HEX,       HEX);
	print_flag(f, s, c, ROOT,      ROOT);
	print_flag(f, s, c, ANYNUMBER, ANYNUMBER);
	print_flag(f, s, c, HIDDEN,    HIDDEN);
	print_flag(f, s, c, ALIAS,     ALIAS);
#undef print_flag

	if (f) {
		char foo[9];
		snprintf(foo, sizeof(foo), "%x", f);
		strlcat(s, c, sizeof(s));
		strlcat(s, foo, sizeof(s));
	}

	return (s);
}

static const char *
st(u_int t)
{

	switch (t) {
	case CTLTYPE_NODE:
		return "NODE";
	case CTLTYPE_INT:
		return "INT";
	case CTLTYPE_STRING:
		return "STRING";
	case CTLTYPE_QUAD:
                return "QUAD";
	case CTLTYPE_STRUCT:
		return "STRUCT";
	}

	return "???";
}

/*
 * ********************************************************************
 * print this node and any others underneath it
 * ********************************************************************
 */
static void
print_tree(int *name, u_int namelen, struct sysctlnode *pnode, u_int type,
	   int add)
{
	struct sysctlnode *node;
	int rc, ni;
	size_t sz;
	char *sp, *dp, n[20];
	struct handlespec *p;

	sp = &gsname[strlen(gsname)];
	dp = &gdname[strlen(gdname)];

	if (sp != &gsname[0] && dp == &gdname[0]) {
		/*
		 * aw...shucks.  now we must play catch up
		 */
		for (ni = 0; ni < namelen; ni++) {
			(void)snprintf(n, sizeof(n), "%d", name[ni]);
			if (ni > 0)
				strncat(gdname, ".", sizeof(gdname));
			strncat(gdname, n, sizeof(gdname));
		}
	}

	if (pnode == NULL)
		pnode = &my_root;
	else if (add) {
		snprintf(n, sizeof(n), "%d", pnode->sysctl_num);
		if (namelen > 1) {
			strncat(gsname, sep, sizeof(gsname));
			strncat(gdname, ".", sizeof(gdname));
		}
		strncat(gsname, pnode->sysctl_name, sizeof(gsname));
		strncat(gdname, n, sizeof(gdname));
	}

	if (Mflag && pnode != &my_root) {
		if (nflag)
			printf("%s: ", gdname);
		else
			printf("%s (%s): ", gsname, gdname);
		printf("CTLTYPE_%s", st(type));
		if (type == CTLTYPE_NODE) {
			if (SYSCTL_FLAGS(pnode->sysctl_flags) & CTLFLAG_ALIAS)
				printf(", alias %d",
				       pnode->sysctl_alias);
			else
				printf(", children %d/%d",
				       pnode->sysctl_clen,
				       pnode->sysctl_csize);
		}
		printf(", size %zu", pnode->sysctl_size);
		printf(", flags 0x%x<%s>",
		       SYSCTL_FLAGS(pnode->sysctl_flags),
		       sf(SYSCTL_FLAGS(pnode->sysctl_flags)));
		if (pnode->sysctl_func)
			printf(", func=%p", pnode->sysctl_func);
		printf(", ver=%d", pnode->sysctl_ver);
		printf("\n");
		if (type != CTLTYPE_NODE) {
			*sp = *dp = '\0';
			return;
		}
	}

	if (dflag && pnode != &my_root) {
		if (Aflag || type != CTLTYPE_NODE) {
			if (pnode->sysctl_desc == NULL)
				getdesc1(name, namelen, pnode);
			if (Aflag ||
			    (pnode->sysctl_desc != NULL &&
			     pnode->sysctl_desc != (const char*)-1)) {
				if (!nflag)
					printf("%s: ", gsname);
				if (pnode->sysctl_desc == NULL ||
				    pnode->sysctl_desc == (const char*)-1)
					printf("(no description)\n");
				else
					printf("%s\n", pnode->sysctl_desc);
			}
		}

		if (type != CTLTYPE_NODE) {
			*sp = *dp = '\0';
			return;
		}
	}

	/*
	 * if this is an alias and we added our name, that means we
	 * got here by recursing down into the tree, so skip it.  The
	 * only way to print an aliased node is with either -M or by
	 * name specifically.
	 */
	if (SYSCTL_FLAGS(pnode->sysctl_flags) & CTLFLAG_ALIAS && add) {
		*sp = *dp = '\0';
		return;
	}

	p = findprinter(name, namelen);
	if (type != CTLTYPE_NODE && p != NULL) {
		(*p->ps_p)(gsname, gdname, NULL, name, namelen, pnode, type,
			   p->ps_d);
		*sp = *dp = '\0';
		return;
	}

	if (type != CTLTYPE_NODE && pnode->sysctl_size == 0) {
		rc = sysctl(&name[0], namelen, NULL, &sz, NULL, 0);
		if (rc == -1) {
			sysctlerror(1);
			*sp = *dp = '\0';
			return;
		}
		if (sz == 0) {
			if ((Aflag || req) && !Mflag)
				printf("%s: node contains no data\n", gsname);
			*sp = *dp = '\0';
			return;
		}
	}
	else
		sz = pnode->sysctl_size;

	switch (type) {
	case CTLTYPE_NODE: {
		__learn_tree(name, namelen, pnode);
		node = pnode->sysctl_child;
		if (node == NULL) {
			if (p != NULL)
				(*p->ps_p)(gsname, gdname, NULL, name, namelen,
					   pnode, type, p->ps_d);
			else if ((Aflag || req) && !Mflag && !dflag)
				printf("%s: no children\n", gsname);
		}
		else {
			if (dflag)
				/*
				 * get all descriptions for next level
				 * in one chunk
				 */
				getdesc(name, namelen, pnode);
			req = 0;
			for (ni = 0; ni < pnode->sysctl_clen; ni++) {
				name[namelen] = node[ni].sysctl_num;
				if ((node[ni].sysctl_flags & CTLFLAG_HIDDEN) &&
				    !(Aflag || req))
					continue;
				print_tree(name, namelen + 1, &node[ni],
					   SYSCTL_TYPE(node[ni].sysctl_flags),
					   1);
			}
		}
		break;
	}
	case CTLTYPE_INT: {
		int i;
		rc = sysctl(name, namelen, &i, &sz, NULL, 0);
		if (rc == -1) {
			sysctlerror(1);
			break;
		}
		display_number(pnode, gsname, &i, sizeof(i), DISPLAY_VALUE);
		break;
	}
	case CTLTYPE_STRING: {
		unsigned char buf[1024], *tbuf;
		tbuf = buf;
		sz = sizeof(buf);
		rc = sysctl(&name[0], namelen, tbuf, &sz, NULL, 0);
		if (rc == -1 && errno == ENOMEM) {
			tbuf = malloc(sz);
			if (tbuf == NULL) {
				sysctlerror(1);
				break;
			}
			rc = sysctl(&name[0], namelen, tbuf, &sz, NULL, 0);
		}
		if (rc == -1)
			sysctlerror(1);
		else
			display_string(pnode, gsname, buf, sz, DISPLAY_VALUE);
		if (tbuf != buf)
			free(tbuf);
		break;
	}
	case CTLTYPE_QUAD: {
		u_quad_t q;
		sz = sizeof(q);
		rc = sysctl(&name[0], namelen, &q, &sz, NULL, 0);
		if (rc == -1) {
			sysctlerror(1);
			break;
		}
		display_number(pnode, gsname, &q, sizeof(q), DISPLAY_VALUE);
		break;
	}
	case CTLTYPE_STRUCT: {
		/*
		 * we shouldn't actually get here, but if we
		 * do, would it be nice to have *something* to
		 * do other than completely ignore the
		 * request.
		 */
		unsigned char *d;
		if ((d = malloc(sz)) == NULL) {
			fprintf(warnfp, "%s: !malloc failed!\n", gsname);
			break;
		}
		rc = sysctl(&name[0], namelen, d, &sz, NULL, 0);
		if (rc == -1) {
			sysctlerror(1);
			break;
		}
		display_struct(pnode, gsname, d, sz, DISPLAY_VALUE);
		free(d);
		break;
	}
	default:
		/* should i print an error here? */
		break;
	}

	*sp = *dp = '\0';
}

/*
 * ********************************************************************
 * parse a request, possibly determining that it's a create or destroy
 * request
 * ********************************************************************
 */
static void
parse(char *l)
{
	struct sysctlnode *node;
	struct handlespec *w;
	int name[CTL_MAXNAME], dodesc = 0;
	u_int namelen, type;
	char *key, *value, *dot;
	size_t sz;

	req = 1;
	key = l;
	value = strchr(l, '=');
	if (value != NULL)
		*value++ = '\0';

	if ((dot = strpbrk(key, "./")) == NULL)
		sep[0] = '.';
	else
		sep[0] = dot[0];
	sep[1] = '\0';

	while (key[0] == sep[0] && key[1] == sep[0]) {
		if (value != NULL)
			value[-1] = '=';
		if (strncmp(key + 2, "create", 6) == 0 &&
		    (key[8] == '=' || key[8] == sep[0]))
			parse_create(key + 8 + (key[8] == '='));
		else if (strncmp(key + 2, "destroy", 7) == 0 &&
			 (key[9] == '=' || key[9] == sep[0]))
			parse_destroy(key + 9 + (key[9] == '='));
		else if (strncmp(key + 2, "describe", 8) == 0 &&
			 (key[10] == '=' || key[10] == sep[0])) {
			key += 10 + (key[10] == '=');
			if ((value = strchr(key, '=')) != NULL)
				parse_describe(key);
			else {
				if (!dflag)
					dodesc = 1;
				break;
			}
		}
		else
			fprintf(warnfp, "%s: unable to parse '%s'\n",
				getprogname(), key);
		return;
	}

	node = &my_root;
	namelen = CTL_MAXNAME;
	sz = sizeof(gsname);

	if (sysctlgetmibinfo(key, &name[0], &namelen, gsname, &sz, &node,
			     SYSCTL_VERSION) == -1) {
		fprintf(warnfp, "%s: %s level name '%s' in '%s' is invalid\n",
			getprogname(), lname[namelen], gsname, l);
		exit(1);
	}

	type = SYSCTL_TYPE(node->sysctl_flags);

	if (value == NULL) {
		if (dodesc)
			dflag = 1;
		print_tree(&name[0], namelen, node, type, 0);
		if (dodesc)
			dflag = 0;
		gsname[0] = '\0';
		return;
	}

	if (!wflag) {
		fprintf(warnfp, "%s: Must specify -w to set variables\n",
			getprogname());
		exit(1);
	}

	if (type != CTLTYPE_NODE && (w = findwriter(name, namelen)) != NULL) {
		(*w->ps_w)(gsname, gdname, value, name, namelen, node, type,
			   w->ps_d);
		gsname[0] = '\0';
		return;
	}

	switch (type) {
	case CTLTYPE_NODE:
		/*
		 * XXX old behavior is to print.  should we error instead?
		 */
		print_tree(&name[0], namelen, node, CTLTYPE_NODE, 1);
		break;
	case CTLTYPE_INT:
		write_number(&name[0], namelen, node, value);
		break;
	case CTLTYPE_STRING:
		write_string(&name[0], namelen, node, value);
		break;
	case CTLTYPE_QUAD:
		write_number(&name[0], namelen, node, value);
		break;
	case CTLTYPE_STRUCT:
		/*
		 * XXX old behavior is to print.  should we error instead?
		 */
		/* fprintf(warnfp, "you can't write to %s\n", gsname); */
		print_tree(&name[0], namelen, node, type, 0);
		break;
	}
}

/*

  //create=foo.bar.whatever...,
  [type=(int|quad|string|struct|node),]
  [size=###,]
  [n=###,]
  [flags=(iohxparw12),]
  [addr=0x####,|symbol=...|value=...]

  size is optional for some types.  type must be set before anything
  else.  nodes can have [r12whp], but nothing else applies.  if no
  size or type is given, node is asserted.  writeable is the default,
  with [r12w] being read-only, writeable below securelevel 1,
  writeable below securelevel 2, and unconditionally writeable
  respectively.  if you specify addr, it is assumed to be the name of
  a kernel symbol, if value, CTLFLAG_OWNDATA will be asserted for
  strings, CTLFLAG_IMMEDIATE for ints and u_quad_ts.  you cannot
  specify both value and addr.

*/

static void
parse_create(char *l)
{
	struct sysctlnode node;
	size_t sz;
	char *nname, *key, *value, *data, *addr, *c, *t;
	int name[CTL_MAXNAME], i, rc, method, flags, rw;
	u_int namelen, type;
	u_quad_t q;
	long li, lo;

	if (!wflag) {
		fprintf(warnfp, "%s: Must specify -w to create nodes\n",
			getprogname());
		exit(1);
	}

	/*
	 * these are the pieces that make up the description of a new
	 * node
	 */
	memset(&node, 0, sizeof(node));
	node.sysctl_num = CTL_CREATE;  /* any number is fine */
	flags = 0;
	rw = -1;
	type = 0;
	sz = 0;
	data = addr = NULL;
	memset(name, 0, sizeof(name));
	namelen = 0;
	method = 0;

	/*
	 * misc stuff used when constructing
	 */
	i = 0;
	q = 0;
	key = NULL;
	value = NULL;

	/*
	 * the name of the thing we're trying to create is first, so
	 * pick it off.
	 */
	nname = l;
	if ((c = strchr(nname, ',')) != NULL)
		*c++ = '\0';

	while (c != NULL) {

		/*
		 * pull off the next "key=value" pair
		 */
		key = c;
		if ((t = strchr(key, '=')) != NULL) {
			*t++ = '\0';
			value = t;
		}
		else
			value = NULL;

		/*
		 * if the "key" is "value", then that eats the rest of
		 * the string, so we're done, otherwise bite it off at
		 * the next comma.
		 */
		if (strcmp(key, "value") == 0) {
			c = NULL;
			data = value;
			break;
		}
		else {
			if ((c = strchr(value, ',')) != NULL)
				*c++ = '\0';
		}

		/*
		 * note that we (mostly) let the invoker of sysctl(8)
		 * play rampant here and depend on the kernel to tell
		 * them that they were wrong.  well...within reason.
		 * we later check the various parameters against each
		 * other to make sure it makes some sort of sense.
		 */
		if (strcmp(key, "addr") == 0) {
			/*
			 * we can't check these two.  only the kernel
			 * can tell us when it fails to find the name
			 * (or if the address is invalid).
			 */
			if (method != 0) {
				fprintf(warnfp,
				    "%s: %s: already have %s for new node\n",
				    getprogname(), nname,
				    method == CTL_CREATE ? "addr" : "symbol");
				exit(1);
			}
			errno = 0;
			addr = (void*)strtoul(value, &t, 0);
			if (*t != '\0' || errno != 0) {
				fprintf(warnfp,
				    "%s: %s: '%s' is not a valid address\n",
				    getprogname(), nname, value);
				exit(1);
			}
			method = CTL_CREATE;
		}
		else if (strcmp(key, "symbol") == 0) {
			if (method != 0) {
				fprintf(warnfp,
				    "%s: %s: already have %s for new node\n",
				    getprogname(), nname,
				    method == CTL_CREATE ? "addr" : "symbol");
				exit(1);
			}
			addr = value;
			method = CTL_CREATESYM;
		}
		else if (strcmp(key, "type") == 0) {
			if (strcmp(value, "node") == 0)
				type = CTLTYPE_NODE;
			else if (strcmp(value, "int") == 0) {
				sz = sizeof(int);
				type = CTLTYPE_INT;
			}
			else if (strcmp(value, "string") == 0)
				type = CTLTYPE_STRING;
			else if (strcmp(value, "quad") == 0) {
				sz = sizeof(u_quad_t);
				type = CTLTYPE_QUAD;
			}
			else if (strcmp(value, "struct") == 0)
				type = CTLTYPE_STRUCT;
			else {
				fprintf(warnfp,
					"%s: %s: '%s' is not a valid type\n",
					getprogname(), nname, value);
				exit(1);
			}
		}
		else if (strcmp(key, "size") == 0) {
			errno = 0;
			/*
			 * yes, i know size_t is not an unsigned long,
			 * but we can all agree that it ought to be,
			 * right?
			 */
			sz = strtoul(value, &t, 0);
			if (*t != '\0' || errno != 0) {
				fprintf(warnfp,
					"%s: %s: '%s' is not a valid size\n",
					getprogname(), nname, value);
				exit(1);
			}
		}
		else if (strcmp(key, "n") == 0) {
			errno = 0;
			li = strtol(value, &t, 0);
			node.sysctl_num = li;
			lo = node.sysctl_num;
			if (*t != '\0' || errno != 0 || li != lo || lo < 0) {
				fprintf(warnfp,
				    "%s: %s: '%s' is not a valid mib number\n",
				    getprogname(), nname, value);
				exit(1);
			}
		}
		else if (strcmp(key, "flags") == 0) {
			t = value;
			while (*t != '\0') {
				switch (*t) {
				case 'a':
					flags |= CTLFLAG_ANYWRITE;
					break;
				case 'h':
					flags |= CTLFLAG_HIDDEN;
					break;
				case 'i':
					flags |= CTLFLAG_IMMEDIATE;
					break;
				case 'o':
					flags |= CTLFLAG_OWNDATA;
					break;
				case 'p':
					flags |= CTLFLAG_PRIVATE;
					break;
				case 'x':
					flags |= CTLFLAG_HEX;
					break;

				case 'r':
					rw = CTLFLAG_READONLY;
					break;
				case '1':
					rw = CTLFLAG_READONLY1;
					break;
				case '2':
					rw = CTLFLAG_READONLY2;
					break;
				case 'w':
					rw = CTLFLAG_READWRITE;
					break;
				default:
					fprintf(warnfp,
					   "%s: %s: '%c' is not a valid flag\n",
					    getprogname(), nname, *t);
					exit(1);
				}
				t++;
			}
		}
		else {
			fprintf(warnfp, "%s: %s: unrecognized keyword '%s'\n",
				getprogname(), nname, key);
			exit(1);
		}
	}

	/*
	 * now that we've finished parsing the given string, fill in
	 * anything they didn't specify
	 */
	if (type == 0)
		type = CTLTYPE_NODE;

	/*
	 * the "data" can be interpreted various ways depending on the
	 * type of node we're creating, as can the size
	 */
	if (data != NULL) {
		if (addr != NULL) {
			fprintf(warnfp,
				"%s: %s: cannot specify both value and "
				"address\n", getprogname(), nname);
			exit(1);
		}

		switch (type) {
		case CTLTYPE_INT:
			errno = 0;
			li = strtol(data, &t, 0);
			i = li;
			lo = i;
			if (*t != '\0' || errno != 0 || li != lo || lo < 0) {
				fprintf(warnfp,
					"%s: %s: '%s' is not a valid integer\n",
					getprogname(), nname, value);
				exit(1);
			}
			if (!(flags & CTLFLAG_OWNDATA)) {
				flags |= CTLFLAG_IMMEDIATE;
				node.sysctl_idata = i;
			}
			else
				node.sysctl_data = &i;
			if (sz == 0)
				sz = sizeof(int);
			break;
		case CTLTYPE_STRING:
			flags |= CTLFLAG_OWNDATA;
			node.sysctl_data = data;
			if (sz == 0)
				sz = strlen(data) + 1;
			else if (sz < strlen(data) + 1) {
				fprintf(warnfp, "%s: %s: ignoring size=%zu for "
					"string node, too small for given "
					"value\n", getprogname(), nname, sz);
				sz = strlen(data) + 1;
			}
			break;
		case CTLTYPE_QUAD:
			errno = 0;
			q = strtouq(data, &t, 0);
			if (*t != '\0' || errno != 0) {
				fprintf(warnfp,
					"%s: %s: '%s' is not a valid quad\n",
					getprogname(), nname, value);
				exit(1);
			}
			if (!(flags & CTLFLAG_OWNDATA)) {
				flags |= CTLFLAG_IMMEDIATE;
				node.sysctl_qdata = q;
			}
			else
				node.sysctl_data = &q;
			if (sz == 0)
				sz = sizeof(u_quad_t);
			break;
		case CTLTYPE_STRUCT:
			fprintf(warnfp,
				"%s: %s: struct not initializable\n",
				getprogname(), nname);
			exit(1);
		}

		/*
		 * these methods have all provided local starting
		 * values that the kernel must copy in
		 */
	}

	/*
	 * hmm...no data, but we have an address of data.  that's
	 * fine.
	 */
	else if (addr != 0)
		node.sysctl_data = (void*)addr;

	/*
	 * no data and no address?  well...okay.  we might be able to
	 * manage that.
	 */
	else if (type != CTLTYPE_NODE) {
		if (sz == 0) {
			fprintf(warnfp,
				"%s: %s: need a size or a starting value\n",
				getprogname(), nname);
                        exit(1);
                }
		if (!(flags & CTLFLAG_IMMEDIATE))
			flags |= CTLFLAG_OWNDATA;
	}

	/*
	 * now we do a few sanity checks on the description we've
	 * assembled
	 */
	if ((flags & CTLFLAG_IMMEDIATE) &&
	    (type == CTLTYPE_STRING || type == CTLTYPE_STRUCT)) {
		fprintf(warnfp,
			"%s: %s: cannot make an immediate %s\n", 
			getprogname(), nname,
			(type == CTLTYPE_STRING) ? "string" : "struct");
		exit(1);
	}
	if (type == CTLTYPE_NODE && node.sysctl_data != NULL) {
		fprintf(warnfp, "%s: %s: nodes do not have data\n",
			getprogname(), nname);
		exit(1);
	}
	
	/*
	 * some types must have a particular size
	 */
	if (sz != 0) {
		if ((type == CTLTYPE_INT && sz != sizeof(int)) ||
		    (type == CTLTYPE_QUAD && sz != sizeof(u_quad_t)) ||
		    (type == CTLTYPE_NODE && sz != 0)) {
			fprintf(warnfp, "%s: %s: wrong size for type\n",
				getprogname(), nname);
			exit(1);
		}
	}
	else if (type == CTLTYPE_STRUCT) {
		fprintf(warnfp, "%s: %s: struct must have size\n",
			getprogname(), nname);
		exit(1);
	}

	/*
	 * now...if no one said anything yet, we default nodes or
	 * any type that owns data being writeable, and everything
	 * else being readonly.
	 */
	if (rw == -1) {
		if (type == CTLTYPE_NODE ||
		    (flags & (CTLFLAG_OWNDATA|CTLFLAG_IMMEDIATE)))
			rw = CTLFLAG_READWRITE;
		else
			rw = CTLFLAG_READONLY;
	}

	/*
	 * if a kernel address was specified, that can't be made
	 * writeable by us.
	if (rw != CTLFLAG_READONLY && addr) {
		fprintf(warnfp, "%s: %s: kernel data can only be readable\n",
			getprogname(), nname);
		exit(1);
	}
	 */

	/*
	 * what separator were they using in the full name of the new
	 * node?
	 */
	if ((t = strpbrk(nname, "./")) == NULL)
		sep[0] = '.';
	else
		sep[0] = t[0];
	sep[1] = '\0';

	/*
	 * put it all together, now.  t'ain't much, is it?
	 */
	node.sysctl_flags = SYSCTL_VERSION|flags|rw|type;
	node.sysctl_size = sz;
	t = strrchr(nname, sep[0]);
	if (t != NULL)
		strlcpy(node.sysctl_name, t + 1, sizeof(node.sysctl_name));
	else
		strlcpy(node.sysctl_name, nname, sizeof(node.sysctl_name));
	if (t == nname)
		t = NULL;

	/*
	 * if this is a new top-level node, then we don't need to find
	 * the mib for its parent
	 */
	if (t == NULL) {
		namelen = 0;
		gsname[0] = '\0';
	}

	/*
	 * on the other hand, if it's not a top-level node...
	 */
	else {
		namelen = sizeof(name) / sizeof(name[0]);
		sz = sizeof(gsname);
		*t = '\0';
		rc = sysctlgetmibinfo(nname, &name[0], &namelen,
				      gsname, &sz, NULL, SYSCTL_VERSION);
		*t = sep[0];
		if (rc == -1) {
			fprintf(warnfp,
				"%s: %s level name '%s' in '%s' is invalid\n",
				getprogname(), lname[namelen], gsname, nname);
			exit(1);
		}
	}

	/*
	 * yes, a new node is being created
	 */
	if (method != 0)
		name[namelen++] = method;
	else
		name[namelen++] = CTL_CREATE;

	sz = sizeof(node);
	rc = sysctl(&name[0], namelen, &node, &sz, &node, sizeof(node));

	if (rc == -1) {
		fprintf(warnfp,
			"%s: %s: CTL_CREATE failed: %s\n",
			getprogname(), nname, strerror(errno));
		exit(1);
	}
	else if (!qflag && !nflag)
		printf("%s(%s): (created)\n", nname, st(type));
}

static void
parse_destroy(char *l)
{
	struct sysctlnode node;
	size_t sz;
	int name[CTL_MAXNAME], rc;
	u_int namelen;

	if (!wflag) {
		fprintf(warnfp, "%s: Must specify -w to destroy nodes\n",
			getprogname());
		exit(1);
	}

	memset(name, 0, sizeof(name));
	namelen = sizeof(name) / sizeof(name[0]);
	sz = sizeof(gsname);
	rc = sysctlgetmibinfo(l, &name[0], &namelen, gsname, &sz, NULL,
			      SYSCTL_VERSION);
	if (rc == -1) {
		fprintf(warnfp,
			"%s: %s level name '%s' in '%s' is invalid\n",
			getprogname(), lname[namelen], gsname, l);
		exit(1);
	}

	memset(&node, 0, sizeof(node));
	node.sysctl_flags = SYSCTL_VERSION;
	node.sysctl_num = name[namelen - 1];
	name[namelen - 1] = CTL_DESTROY;

	sz = sizeof(node);
	rc = sysctl(&name[0], namelen, &node, &sz, &node, sizeof(node));

	if (rc == -1) {
		fprintf(warnfp,
			"%s: %s: CTL_DESTROY failed: %s\n",
			getprogname(), l, strerror(errno));
		exit(1);
	}
	else if (!qflag && !nflag)
		printf("%s(%s): (destroyed)\n", gsname,
		       st(SYSCTL_TYPE(node.sysctl_flags)));
}

static void
parse_describe(char *l)
{
	struct sysctlnode newdesc;
	char buf[1024], *value;
	struct sysctldesc *d = (void*)&buf[0];
	int name[CTL_MAXNAME], rc;
	u_int namelen;
	size_t sz;

	if (!wflag) {
		fprintf(warnfp,
			"%s: Must specify -w to set descriptions\n",
			getprogname());
		exit(1);
	}

	value = strchr(l, '=');
	*value++ = '\0';

	memset(name, 0, sizeof(name));
	namelen = sizeof(name) / sizeof(name[0]);
	sz = sizeof(gsname);
	rc = sysctlgetmibinfo(l, &name[0], &namelen, gsname, &sz, NULL,
			      SYSCTL_VERSION);
	if (rc == -1) {
		fprintf(warnfp,
			"%s: %s level name '%s' in '%s' is invalid\n",
			getprogname(), lname[namelen], gsname, l);
		exit(1);
	}

	sz = sizeof(buf);
	memset(&newdesc, 0, sizeof(newdesc));
	newdesc.sysctl_flags = SYSCTL_VERSION|CTLFLAG_OWNDESC;
	newdesc.sysctl_num = name[namelen - 1];
	newdesc.sysctl_desc = value;
	name[namelen - 1] = CTL_DESCRIBE;
	rc = sysctl(name, namelen, d, &sz, &newdesc, sizeof(newdesc));
	if (rc == -1)
		fprintf(warnfp, "%s: %s: CTL_DESCRIBE failed: %s\n",
			getprogname(), gsname, strerror(errno));
	else if (d->descr_len == 1)
		fprintf(warnfp, "%s: %s: description not set\n",
			getprogname(), gsname);
	else if (!qflag && !nflag)
		printf("%s: %s\n", gsname, d->descr_str);
}

/*
 * ********************************************************************
 * when things go wrong...
 * ********************************************************************
 */
static void
usage(void)
{
	const char *progname = getprogname();

	(void)fprintf(stderr,
		      "usage:\t%s %s\n"
		      "\t%s %s\n"
		      "\t%s %s\n"
		      "\t%s %s\n"
		      "\t%s %s\n"
		      "\t%s %s\n",
		      progname, "[-dne] [-x[x]|-r] variable ...",
		      progname, "[-ne] [-q] -w variable=value ...",
		      progname, "[-dne] -a",
		      progname, "[-dne] -A",
		      progname, "[-ne] -M",
		      progname, "[-dne] [-q] -f file");
	exit(1);
}

static void
getdesc1(int *name, u_int namelen, struct sysctlnode *pnode)
{
	struct sysctlnode node;
	char buf[1024], *desc;
	struct sysctldesc *d = (void*)buf;
	size_t sz = sizeof(buf);
	int rc;

	memset(&node, 0, sizeof(node));
	node.sysctl_flags = SYSCTL_VERSION;
	node.sysctl_num = name[namelen - 1];
	name[namelen - 1] = CTL_DESCRIBE;
	rc = sysctl(name, namelen, d, &sz, &node, sizeof(node));

	if (rc == -1 ||
	    d->descr_len == 1 ||
	    d->descr_num != pnode->sysctl_num ||
	    d->descr_ver != pnode->sysctl_ver)
		desc = (char *)-1;
	else
		desc = malloc(d->descr_len);

	if (desc == NULL)
		desc = (char *)-1;
	if (desc != (char *)-1)
		memcpy(desc, &d->descr_str[0], sz);
	name[namelen - 1] = node.sysctl_num;
	if (pnode->sysctl_desc != NULL &&
	    pnode->sysctl_desc != (const char *)-1)
		free((void*)pnode->sysctl_desc);
	pnode->sysctl_desc = desc;
}

static void
getdesc(int *name, u_int namelen, struct sysctlnode *pnode)
{
	struct sysctlnode *node = pnode->sysctl_child;
	struct sysctldesc *d, *p;
	char *desc;
	size_t sz;
	int rc, i;

	sz = 128 * pnode->sysctl_clen;
	name[namelen] = CTL_DESCRIBE;

	/*
	 * attempt *twice* to get the description chunk.  if two tries
	 * doesn't work, give up.
	 */
	i = 0;
	do {
		d = malloc(128 * pnode->sysctl_clen);
		if (d == NULL)
			return;
		rc = sysctl(name, namelen + 1, d, &sz, NULL, 0);
		if (rc == -1) {
			free(d);
			d = NULL;
			if (i == 0 && errno == ENOMEM)
				i = 1;
			else
				return;
		}
	} while (d == NULL);

	/*
	 * hokey nested loop here, giving O(n**2) behavior, but should
	 * suffice for now
	 */
	for (i = 0; i < pnode->sysctl_clen; i++) {
		node = &pnode->sysctl_child[i];
		for (p = d; (char *)p < (char *)d + sz; p = NEXT_DESCR(p))
			if (node->sysctl_num == p->descr_num)
				break;
		if ((char *)p < (char *)d + sz &&
		    node[i].sysctl_ver == p->descr_ver) {
			/*
			 * match found, attempt to attach description
			 */
			if (p->descr_len == 1)
				desc = NULL;
			else
				desc = malloc(p->descr_len);
			if (desc == NULL)
				desc = (char *)-1;
			else
				memcpy(desc, &p->descr_str[0], sz);
			node->sysctl_desc = desc;
		}
	}

	free(d);
}

void
sysctlerror(int soft)
{
	if (soft) {
		switch (errno) {
		case ENOENT:
		case ENOPROTOOPT:
		case ENOTDIR:
		case EOPNOTSUPP:
		case EPROTONOSUPPORT:
			if (Aflag || req)
				fprintf(warnfp,
					"%s: the value is not available\n",
					gsname);
			return;
		}
	}

	fprintf(warnfp, "%s: sysctl() failed with %s\n",
		gsname, strerror(errno));
	if (!soft)
		exit(1);
}

/*
 * ********************************************************************
 * how to write to a "simple" node
 * ********************************************************************
 */
static void
write_number(int *name, u_int namelen, struct sysctlnode *node, char *value)
{
	int ii, io;
	u_quad_t qi, qo;
	size_t si, so;
	int rc;
	void *i, *o;
	char *t;

	si = so = 0;
	i = o = NULL;
	errno = 0;
	qi = strtouq(value, &t, 0);
	if (errno != 0) {
		fprintf(warnfp, "%s: value too large\n", value);
		exit(1);
	}
	if (*t != '\0') {
		fprintf(warnfp, "%s: not a number\n", value);
		exit(1);
	}

	switch (SYSCTL_TYPE(node->sysctl_flags)) {
	    case CTLTYPE_INT:
		ii = (int)qi;
		qo = ii;
		if (qo != qi) {
			fprintf(warnfp, "%s: value too large\n", value);
			exit(1);
		}
		o = &io;
		so = sizeof(io);
		i = &ii;
		si = sizeof(ii);
		break;
	case CTLTYPE_QUAD:
		o = &qo;
		so = sizeof(qo);
		i = &qi;
		si = sizeof(qi);
		break;
	}

	rc = sysctl(name, namelen, o, &so, i, si);
	if (rc == -1)
		sysctlerror(0);

	switch (SYSCTL_TYPE(node->sysctl_flags)) {
	case CTLTYPE_INT:
		display_number(node, gsname, &io, sizeof(io), DISPLAY_OLD);
		display_number(node, gsname, &ii, sizeof(ii), DISPLAY_NEW);
		break;
	case CTLTYPE_QUAD:
		display_number(node, gsname, &qo, sizeof(qo), DISPLAY_OLD);
		display_number(node, gsname, &qi, sizeof(qi), DISPLAY_NEW);
		break;
	}
}

static void
write_string(int *name, u_int namelen, struct sysctlnode *node, char *value)
{
	char *i, *o;
	size_t si, so;
	int rc;

	i = value;
	si = strlen(i) + 1;
	so = node->sysctl_size;
	if (si > so && so != 0) {
		fprintf(warnfp, "%s: string too long\n", value);
		exit(1);
	}
	o = malloc(so);
	if (o == NULL) {
		fprintf(warnfp, "%s: !malloc failed!\n", gsname);
		exit(1);
	}

	rc = sysctl(name, namelen, o, &so, i, si);
	if (rc == -1)
		sysctlerror(0);

	display_string(node, gsname, o, so, DISPLAY_OLD);
	display_string(node, gsname, i, si, DISPLAY_NEW);
	free(o);
}

/*
 * ********************************************************************
 * simple ways to print stuff consistently
 * ********************************************************************
 */
static void
display_number(const struct sysctlnode *node, const char *name,
	       const void *data, size_t sz, int n)
{
	u_quad_t q;
	int i;

	if (qflag)
		return;
	if ((nflag || rflag) && (n == DISPLAY_OLD))
		return;

	if (rflag && n != DISPLAY_OLD) {
		fwrite(data, sz, 1, stdout);
		return;
	}

	if (!nflag) {
		if (n == DISPLAY_VALUE)
			printf("%s%s", name, eq);
		else if (n == DISPLAY_OLD)
			printf("%s: ", name);
	}

	if (xflag > 1) {
		if (n != DISPLAY_NEW)
			printf("\n");
		hex_dump(data, sz);
		return;
	}

	switch (SYSCTL_TYPE(node->sysctl_flags)) {
	case CTLTYPE_INT:
		memcpy(&i, data, sz);
		if (xflag)
			printf("0x%0*x", (int)sz * 2, i);
		else if (node->sysctl_flags & CTLFLAG_HEX)
			printf("%#x", i);
		else
			printf("%d", i);
		break;
	case CTLTYPE_QUAD:
		memcpy(&q, data, sz);
		if (xflag)
			printf("0x%0*" PRIx64, (int)sz * 2, q);
		else if (node->sysctl_flags & CTLFLAG_HEX)
			printf("%#" PRIx64, q);
		else
			printf("%" PRIu64, q);
		break;
	}

	if (n == DISPLAY_OLD)
		printf(" -> ");
	else
		printf("\n");
}

static void
display_string(const struct sysctlnode *node, const char *name,
	       const void *data, size_t sz, int n)
{
	const unsigned char *buf = data;
	int ni;

	if (qflag)
		return;
	if ((nflag || rflag) && (n == DISPLAY_OLD))
		return;

	if (rflag && n != DISPLAY_OLD) {
		fwrite(data, sz, 1, stdout);
		return;
	}

	if (!nflag) {
		if (n == DISPLAY_VALUE)
			printf("%s%s", name, eq);
		else if (n == DISPLAY_OLD)
			printf("%s: ", name);
	}

	if (xflag > 1) {
		if (n != DISPLAY_NEW)
			printf("\n");
		hex_dump(data, sz);
		return;
	}

	if (xflag || node->sysctl_flags & CTLFLAG_HEX) {
		for (ni = 0; ni < (int)sz; ni++) {
			if (xflag)
				printf("%02x", buf[ni]);
			if (buf[ni] == '\0')
				break;
			if (!xflag)
				printf("\\x%2.2x", buf[ni]);
		}
	}
	else
		printf("%.*s", (int)sz, buf);

	if (n == DISPLAY_OLD)
		printf(" -> ");
	else
		printf("\n");
}

/*ARGSUSED*/
static void
display_struct(const struct sysctlnode *node, const char *name,
	       const void *data, size_t sz, int n)
{
	const unsigned char *buf = data;
	int ni;
	size_t more;

	if (qflag)
		return;
	if (!(xflag || rflag)) {
		if (Aflag || req)
			fprintf(warnfp,
				"%s: this type is unknown to this program\n",
				gsname);
		return;
	}
	if ((nflag || rflag) && (n == DISPLAY_OLD))
		return;

	if (rflag && n != DISPLAY_OLD) {
		fwrite(data, sz, 1, stdout);
		return;
	}

        if (!nflag) {
                if (n == DISPLAY_VALUE)
                        printf("%s%s", name, eq);
                else if (n == DISPLAY_OLD)
                        printf("%s: ", name);
        }

	if (xflag > 1) {
		if (n != DISPLAY_NEW)
			printf("\n");
		hex_dump(data, sz);
		return;
	}

	if (sz > 16) {
		more = sz - 16;
		sz = 16;
	}
	else
		more = 0;
	for (ni = 0; ni < (int)sz; ni++)
		printf("%02x", buf[ni]);
	if (more)
		printf("...(%zu more bytes)", more);
	printf("\n");
}

static void
hex_dump(const unsigned char *buf, size_t len)
{
	int i, j;
	char line[80], tmp[12];

	memset(line, ' ', sizeof(line));
	for (i = 0, j = 15; i < len; i++) {
		j = i % 16;
		/* reset line */
		if (j == 0) {
			line[58] = '|';
			line[77] = '|';
			line[78] = 0;
			snprintf(tmp, sizeof(tmp), "%07d", i);
			memcpy(&line[0], tmp, 7);
		}
		/* copy out hex version of byte */
		snprintf(tmp, sizeof(tmp), "%02x", buf[i]);
		memcpy(&line[9 + j * 3], tmp, 2);
		/* copy out plain version of byte */
		line[60 + j] = (isprint(buf[i])) ? buf[i] : '.';
		/* print a full line and erase it */
		if (j == 15) {
			printf("%s\n", line);
			memset(line, ' ', sizeof(line));
		}
	}
	if (line[0] != ' ')
		printf("%s\n", line);
	printf("%07zu bytes\n", len);
}

/*
 * ********************************************************************
 * functions that handle particular nodes
 * ********************************************************************
 */
/*ARGSUSED*/
static void
printother(HANDLER_ARGS)
{
	int rc;
	void *p;
	size_t sz1, sz2;

	if (!(Aflag || req) || Mflag)
		return;

	/*
	 * okay...you asked for it, so let's give it a go
	 */
	while (type != CTLTYPE_NODE && (xflag || rflag)) {
		rc = sysctl(name, namelen, NULL, &sz1, NULL, 0);
		if (rc == -1 || sz1 == 0)
			break;
		p = malloc(sz1);
		if (p == NULL)
			break;
		sz2 = sz1;
		rc = sysctl(name, namelen, p, &sz2, NULL, 0);
		if (rc == -1 || sz1 != sz2) {
			free(p);
			break;
		}
		display_struct(pnode, gsname, p, sz1, DISPLAY_VALUE);
		free(p);
		return;
	}

	/*
	 * that didn't work...do we have a specific message for this
	 * thing?
	 */
	if (v != NULL) {
		fprintf(warnfp, "%s: use '%s' to view this information\n",
			gsname, (const char *)v);
		return;
	}

	/*
	 * hmm...i wonder if we have any generic hints?
	 */
	switch (name[0]) {
	case CTL_NET:
		fprintf(warnfp, "%s: use 'netstat' to view this information\n",
			sname);
		break;
	case CTL_DEBUG:
		fprintf(warnfp, "%s: missing 'options DEBUG' from kernel?\n",
			sname);
		break;
	case CTL_DDB:
		fprintf(warnfp, "%s: missing 'options DDB' from kernel?\n",
			sname);
		break;
	}
}

/*ARGSUSED*/
static void
kern_clockrate(HANDLER_ARGS)
{
	struct clockinfo clkinfo;
	size_t sz;
	int rc;

	sz = sizeof(clkinfo);
	rc = sysctl(name, namelen, &clkinfo, &sz, NULL, 0);
	if (rc == -1) {
		sysctlerror(1);
		return;
	}
	if (sz != sizeof(clkinfo))
		errx(1, "%s: !returned size wrong!", sname);

	if (xflag || rflag) {
		display_struct(pnode, sname, &clkinfo, sz,
			       DISPLAY_VALUE);
		return;
	}
	else if (!nflag)
		printf("%s: ", sname);
	printf("tick = %d, tickadj = %d, hz = %d, profhz = %d, stathz = %d\n",
	       clkinfo.tick, clkinfo.tickadj,
	       clkinfo.hz, clkinfo.profhz, clkinfo.stathz);
}

/*ARGSUSED*/
static void
kern_boottime(HANDLER_ARGS)
{
	struct timeval timeval;
	time_t boottime;
	size_t sz;
	int rc;

	sz = sizeof(timeval);
	rc = sysctl(name, namelen, &timeval, &sz, NULL, 0);
	if (rc == -1) {
		sysctlerror(1);
		return;
	}
	if (sz != sizeof(timeval))
		errx(1, "%s: !returned size wrong!", sname);

	boottime = timeval.tv_sec;
	if (xflag || rflag)
		display_struct(pnode, sname, &timeval, sz,
			       DISPLAY_VALUE);
	else if (!nflag)
		/* ctime() provides the \n */
		printf("%s%s%s", sname, eq, ctime(&boottime));
	else if (nflag == 1)
		printf("%ld\n", (long)boottime);
	else
		printf("%ld.%06ld\n", (long)timeval.tv_sec,
		       (long)timeval.tv_usec);
}

/*ARGSUSED*/
static void
kern_consdev(HANDLER_ARGS)
{
	dev_t cons;
	size_t sz;
	int rc;

	sz = sizeof(cons);
	rc = sysctl(name, namelen, &cons, &sz, NULL, 0);
	if (rc == -1) {
		sysctlerror(1);
		return;
	}
	if (sz != sizeof(cons))
		errx(1, "%s: !returned size wrong!", sname);

	if (xflag || rflag)
		display_struct(pnode, sname, &cons, sz,
			       DISPLAY_VALUE);
	else if (!nflag)
		printf("%s%s%s\n", sname, eq, devname(cons, S_IFCHR));
	else
		printf("0x%x\n", cons);
}

/*ARGSUSED*/
static void
kern_cp_time(HANDLER_ARGS)
{
	u_int64_t *cp_time;
	size_t sz, osz;
	int rc, i, n;
	char s[sizeof("kern.cp_time.nnnnnn")];
	const char *tname;

	/*
	 * three things to do here.
	 * case 1: get sum (no Aflag and namelen == 2)
	 * case 2: get specific processor (namelen == 3)
	 * case 3: get all processors (Aflag and namelen == 2)
	 */

	if (namelen == 2 && Aflag) {
		sz = sizeof(n);
		rc = sysctlbyname("hw.ncpu", &n, &sz, NULL, 0);
		if (rc != 0)
			return; /* XXX print an error, eh? */
		n++; /* Add on space for the sum. */
		sz = n * sizeof(u_int64_t) * CPUSTATES;
	}
	else {
		n = -1; /* Just print one data set. */
		sz = sizeof(u_int64_t) * CPUSTATES;
	}

	cp_time = malloc(sz);
	if (cp_time == NULL) {
		sysctlerror(1);
		return;
	}

	osz = sz;
	rc = sysctl(name, namelen, cp_time + (n != -1) * CPUSTATES, &osz,
		    NULL, 0);

	if (rc == -1) {
		sysctlerror(1);
		free(cp_time);
		return;
	}

	/*
	 * Check, but account for space we'll occupy with the sum.
	 */
	if (osz != sz - (n != -1) * CPUSTATES * sizeof(u_int64_t))
		errx(1, "%s: !returned size wrong!", sname);

	/*
	 * Compute the actual sum.  Two calls would be easier (we
	 * could just call ourselves recursively above), but the
	 * numbers wouldn't add up.
	 */
	if (n != -1) {
		memset(cp_time, 0, sizeof(u_int64_t) * CPUSTATES);
		for (i = 1; i < n; i++) {
			cp_time[CP_USER] += cp_time[i * CPUSTATES + CP_USER];
                        cp_time[CP_NICE] += cp_time[i * CPUSTATES + CP_NICE];
                        cp_time[CP_SYS] += cp_time[i * CPUSTATES + CP_SYS];
                        cp_time[CP_INTR] += cp_time[i * CPUSTATES + CP_INTR];
                        cp_time[CP_IDLE] += cp_time[i * CPUSTATES + CP_IDLE];
		}
	}

	tname = sname;
	for (i = 0; n == -1 || i < n; i++) {
		if (i > 0) {
			(void)snprintf(s, sizeof(s), "%s%s%d", sname, sep,
				       i - 1);
			tname = s;
		}
		if (xflag || rflag)
			display_struct(pnode, tname, cp_time + (i * CPUSTATES),
				       sizeof(u_int64_t) * CPUSTATES,
				       DISPLAY_VALUE);
		else {
			if (!nflag)
				printf("%s: ", tname);
			printf("user = %" PRIu64
			       ", nice = %" PRIu64
			       ", sys = %" PRIu64
			       ", intr = %" PRIu64
			       ", idle = %" PRIu64
			       "\n",
			       cp_time[i * CPUSTATES + CP_USER],
			       cp_time[i * CPUSTATES + CP_NICE],
			       cp_time[i * CPUSTATES + CP_SYS],
			       cp_time[i * CPUSTATES + CP_INTR],
			       cp_time[i * CPUSTATES + CP_IDLE]);
		}
		/*
		 * Just printing the one node.
		 */
		if (n == -1)
			break;
	}

	free(cp_time);
}

/*ARGSUSED*/
static void
vm_loadavg(HANDLER_ARGS)
{
	struct loadavg loadavg;
	size_t sz;
	int rc;

	sz = sizeof(loadavg);
	rc = sysctl(name, namelen, &loadavg, &sz, NULL, 0);
	if (rc == -1) {
		sysctlerror(1);
		return;
	}
	if (sz != sizeof(loadavg))
		errx(1, "%s: !returned size wrong!", sname);

	if (xflag || rflag) {
		display_struct(pnode, sname, &loadavg, sz,
			       DISPLAY_VALUE);
		return;
	}
	if (!nflag)
		printf("%s: ", sname);
	printf("%.2f %.2f %.2f\n",
	       (double) loadavg.ldavg[0] / loadavg.fscale,
	       (double) loadavg.ldavg[1] / loadavg.fscale,
	       (double) loadavg.ldavg[2] / loadavg.fscale);
}

/*ARGSUSED*/
static void
proc_limit(HANDLER_ARGS)
{
	u_quad_t olim, *newp, nlim;
	size_t osz, nsz;
	char *t;
	int rc;

	osz = sizeof(olim);
	if (value != NULL) {
		nsz = sizeof(nlim);
		newp = &nlim;
		if (strcmp(value, "unlimited") == 0)
			nlim = RLIM_INFINITY;
		else {
			errno = 0;
			nlim = strtouq(value, &t, 0);
			if (*t != '\0' || errno != 0) {
				fprintf(warnfp,
					"%s: %s: '%s' is not a valid limit\n",
					getprogname(), sname, value);
				exit(1);
			}
		}
	}
	else {
		nsz = 0;
		newp = NULL;
	}

	rc = sysctl(name, namelen, &olim, &osz, newp, nsz);
	if (rc == -1) {
		sysctlerror(newp == NULL);
		return;
	}

	if (newp && qflag)
		return;

	if (rflag || xflag || olim != RLIM_INFINITY)
		display_number(pnode, sname, &olim, sizeof(olim),
			       newp ? DISPLAY_OLD : DISPLAY_VALUE);
	else
		display_string(pnode, sname, "unlimited", 10,
			       newp ? DISPLAY_OLD : DISPLAY_VALUE);

	if (newp) {
		if (rflag || xflag || nlim != RLIM_INFINITY)
			display_number(pnode, sname, &nlim, sizeof(nlim),
				       DISPLAY_NEW);
		else
			display_string(pnode, sname, "unlimited", 10,
				       DISPLAY_NEW);
	}
}

#ifdef CPU_DISKINFO
/*ARGSUSED*/
static void
machdep_diskinfo(HANDLER_ARGS)
{
	struct disklist *dl;
	struct biosdisk_info *bi;
	struct nativedisk_info *ni;
	int rc;
	size_t sz;
	uint i, b, lim;

	rc = sysctl(name, namelen, NULL, &sz, NULL, 0);
	if (rc == -1) {
		sysctlerror(1);
		return;
	}
	dl = malloc(sz);
	if (dl == NULL) {
		sysctlerror(1);
		return;
	}
	rc = sysctl(name, namelen, dl, &sz, NULL, 0);
	if (rc == -1) {
		sysctlerror(1);
		return;
	}

	if (!nflag)
		printf("%s: ", sname);
	lim = dl->dl_nbiosdisks;
	if (lim > MAX_BIOSDISKS)
		lim = MAX_BIOSDISKS;
	for (bi = dl->dl_biosdisks, i = 0; i < lim; bi++, i++)
		printf("%x:%" PRIu64 "(%d/%d/%d),%x ",
		       bi->bi_dev, bi->bi_lbasecs,
		       bi->bi_cyl, bi->bi_head, bi->bi_sec,
		       bi->bi_flags);
	lim = dl->dl_nnativedisks;
	ni = dl->dl_nativedisks;
	bi = dl->dl_biosdisks;
	/* LINTED -- pointer casts are tedious */
	if ((char *)&ni[lim] != (char *)dl + sz) {
		fprintf(warnfp, "size mismatch\n");
		return;
	}
	for (i = 0; i < lim; ni++, i++) {
		char t = ':';
		printf(" %.*s", (int)sizeof ni->ni_devname,
		       ni->ni_devname);
		for (b = 0; b < ni->ni_nmatches; t = ',', b++)
			printf("%c%x", t,
			       bi[ni->ni_biosmatches[b]].bi_dev);
	}
	printf("\n");
}
#endif /* CPU_DISKINFO */
