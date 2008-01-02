/*	$NetBSD: main.c,v 1.17 2008/01/02 17:23:31 yamt Exp $ */

/*
 * Copyright (c) 2002, 2003 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
__RCSID("$NetBSD: main.c,v 1.17 2008/01/02 17:23:31 yamt Exp $");
#endif

#include <sys/param.h>

#ifndef __NetBSD_Version__
#error go away, you fool
#elif (__NetBSD_Version__ < 105000000)
#error only works with uvm
#endif

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#include "pmap.h"
#include "main.h"

struct cache_head lcache;
struct nchashhead *nchashtbl;
void *uvm_vnodeops, *uvm_deviceops, *aobj_pager, *ubc_pager;
void *kernel_floor;
struct vm_map *kmem_map, *mb_map, *phys_map, *exec_map, *pager_map;
struct vm_map *st_map, *pt_map, *lkm_map, *buf_map;
u_long nchash_addr, nchashtbl_addr, kernel_map_addr;
int debug, verbose, recurse, page_size;
int print_all, print_map, print_maps, print_solaris, print_ddb;
rlim_t maxssiz;

struct nlist ksyms[] = {
	{ "_maxsmap" },
#define NL_MAXSSIZ		0
	{ "_uvm_vnodeops" },
#define NL_UVM_VNODEOPS		1
	{ "_uvm_deviceops" },
#define NL_UVM_DEVICEOPS	2
	{ "_aobj_pager" },
#define NL_AOBJ_PAGER		3
	{ "_ubc_pager" },
#define NL_UBC_PAGER		4
	{ "_kernel_map" },
#define NL_KERNEL_MAP		5
	{ "_nchashtbl" },
#define NL_NCHASHTBL		6
	{ "_nchash" },
#define NL_NCHASH		7
	{ "_kernel_text" },
#define NL_KENTER		8
	{ NULL }
};

struct nlist kmaps[] = {
	{ "_kmem_map" },
#define NL_kmem_map		0
	{ "_mb_map" },
#define NL_mb_map		1
	{ "_phys_map" },
#define NL_phys_map		2
	{ "_exec_map" },
#define NL_exec_map		3
	{ "_pager_map" },
#define NL_pager_map		4
	{ "_st_map" },
#define NL_st_map		5
	{ "_pt_map" },
#define NL_pt_map		6
	{ "_lkm_map" },
#define NL_lkm_map		7
	{ "_buf_map" },
#define NL_buf_map		8
	{ NULL }
};

#define VMSPACE_ADDRESS		1
#define VM_MAP_ADDRESS		2
#define VM_MAP_ENTRY_ADDRESS	3
#define AMAP_ADDRESS		4

void check_fd(int);
void load_symbols(kvm_t *);
void cache_enter(int, struct namecache *);

int
main(int argc, char *argv[])
{
	kvm_t *kd;
	pid_t pid;
	int which, many, ch, rc;
	char errbuf[_POSIX2_LINE_MAX + 1];
	struct kinfo_proc2 *kproc;
	char *kmem, *kernel, *t;
	gid_t egid;
	struct kbit kbit, *vmspace;
	u_long address;

	egid = getegid();
	if (setegid(getgid()) == -1)
		err(1, "failed to reset privileges");

	check_fd(STDIN_FILENO);
	check_fd(STDOUT_FILENO);
	check_fd(STDERR_FILENO);

	pid = -1;
	which = verbose = debug = 0;
	print_all = print_map = print_maps = print_solaris = print_ddb = 0;
	recurse = 0;
	kmem = kernel = NULL;
	address = 0;
	vmspace = &kbit;

	while ((ch = getopt(argc, argv, "A:aD:dE:lM:mN:Pp:RrS:sV:vx")) != -1) {
		switch (ch) {
		case 'A':
		case 'E':
		case 'S':
		case 'V':
			if (which != 0)
				errx(1, "use only one of -A, -E, -S, or -V");
			errno = 0;
			address = strtoul(optarg, &t, 0);
			if (*t != '\0')
				errx(1, "%s is not a valid address", optarg);
			if (errno != 0)
				err(1, "%s is not a valid address", optarg);
			switch (ch) {
			case 'A':	which = AMAP_ADDRESS;		break;
			case 'E':	which = VM_MAP_ENTRY_ADDRESS;	break;
			case 'S':	which = VMSPACE_ADDRESS;	break;
			case 'V':	which = VM_MAP_ADDRESS;		break;
			}
			break;
		case 'a':
			print_all = 1;
			break;
		case 'd':
			print_ddb = 1;
			break;
		case 'D':
			errno = 0;
			debug = strtoul(optarg, &t, 0);
			if (*t != '\0')
				errx(1, "%s is not a valid number", optarg);
			if (errno != 0)
				err(1, "%s is not a valid number", optarg);
			break;
		case 'l':
			print_maps = 1;
			break;
		case 'm':
			print_map = 1;
			break;
		case 'M':
			kmem = optarg;
			break;
		case 'N':
			kernel = optarg;
			break;
		case 'p':
			errno = 0;
			pid = strtol(optarg, &t, 0);
			if (pid < 0)
				errno = EINVAL;
			if (*t != '\0')
				errx(1, "%s is not a valid pid", optarg);
			if (errno != 0)
				err(1, "%s is not a valid pid", optarg);
			break;
		case 'P':
			pid = getpid();
			break;
		case 'R':
			recurse = 1;
			break;
		case 's':
			print_solaris = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'r':
		case 'x':
			errx(1, "-%c option not implemented, sorry", optopt);
			/*NOTREACHED*/
		case '?':
		default:
			fprintf(stderr, "usage: %s [-adlmPRsv] [-A address] "
				"[-D number] [-E address] [-M core]\n"
				"\t[-N system] [-p pid] [-S address] "
				"[-V address] [pid ...]\n",
				getprogname());
			exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	/* more than one "process" to dump? */
	many = (argc > 1 - (pid == -1 ? 0 : 1)) ? 1 : 0;

	/* apply default */
	if (print_all + print_map + print_maps + print_solaris +
	    print_ddb == 0)
		print_solaris = 1;

	/* get privs back if it appears to be safe, otherwise toss them */
	if (kernel == NULL && kmem == NULL && address == 0)
		rc = setegid(egid);
	else
		rc = setgid(getgid());
	if (rc == -1)
		err(1, "failed to reset privileges");

	/* start by opening libkvm */
	kd = kvm_openfiles(kernel, kmem, NULL, O_RDONLY, errbuf);

	/* we're completely done with privileges now */
	rc = setgid(getgid());
	if (rc == -1)
		err(1, "failed to reset privileges");

	/* print the kvm_open error, if any */
	errbuf[_POSIX2_LINE_MAX] = '\0';
	if (kd == NULL)
		errx(1, "%s", errbuf);

	/* get "bootstrap" addresses from kernel */
	load_symbols(kd);

	if (address) {
		struct kbit kbit2, *at = &kbit2;

		memset(vmspace, 0, sizeof(*vmspace));
		A(at) = address;
		S(at) = (size_t)-1;

		switch (which) {
		    case VMSPACE_ADDRESS:
			/* (kd, kproc, vmspace, thing) */
			(*process_map)(kd, NULL, at, "vm_map");
			break;
		    case VM_MAP_ADDRESS:
			/* (kd, proc, vmspace, vm_map, thing) */
			(*dump_vm_map)(kd, NULL, vmspace, at, "vm_map");
			break;
		    case VM_MAP_ENTRY_ADDRESS:
			/* (kd, proc, vmspace, vm_map_entry, 0) */
			(*dump_vm_map_entry)(kd, NULL, vmspace, at, 0);
			break;
		    case AMAP_ADDRESS:
			/* (kd, amap) */
			(*dump_amap)(kd, at);
			break;
		}
		exit(0);
	}

	do {
		if (pid == -1) {
			if (argc == 0)
				pid = getppid();
			else {
				errno = 0;
				pid = strtol(argv[0], &t, 0);
				if (pid < 0)
					errno = EINVAL;
				if (*t != '\0')
					errx(1, "%s is not a valid pid",
					    argv[0]);
				if (errno != 0)
					err(1, "%s is not a valid pid",
					    argv[0]);
				argv++;
				argc--;
			}
		}

		/* find the process id */
		if (pid == 0)
			kproc = NULL;
		else {
			kproc = kvm_getproc2(kd, KERN_PROC_PID, pid,
					     sizeof(struct kinfo_proc2), &rc);
			if (kproc == NULL || rc == 0) {
				errno = ESRCH;
				warn("%d", pid);
				pid = -1;
				continue;
			}
		}

		/* dump it */
		if (many) {
			if (kproc)
				printf("process %d:\n", kproc->p_pid);
			else
				printf("kernel:\n");
		}

		(*process_map)(kd, kproc, vmspace, NULL);
		pid = -1;
	} while (argc > 0);

	/* done.  go away. */
	rc = kvm_close(kd);
	if (rc == -1)
		err(1, "kvm_close");

	return (0);
}

void
check_fd(int fd)
{
	struct stat st;
	int n;

	if (fstat(fd, &st) == -1) {
		(void)close(fd);
		n = open("/dev/null", O_RDWR);
		if (n == fd || n == -1)
			/* we're either done or we can do no more */
			return;
		/* if either of these fail, there's not much we can do */
		(void)dup2(n, fd);
		(void)close(n);
		/* XXX should we exit if it fails? */
	}
}

void
load_symbols(kvm_t *kd)
{
	int rc, i, mib[2];
	size_t sz;

	rc = kvm_nlist(kd, &ksyms[0]);
	if (rc != 0) {
		for (i = 0; ksyms[i].n_name != NULL; i++)
			if (ksyms[i].n_value == 0)
				warnx("symbol %s: not found", ksyms[i].n_name);
		exit(1);
	}

	uvm_vnodeops =	(void*)ksyms[NL_UVM_VNODEOPS].n_value;
	uvm_deviceops =	(void*)ksyms[NL_UVM_DEVICEOPS].n_value;
	aobj_pager =	(void*)ksyms[NL_AOBJ_PAGER].n_value;
	ubc_pager =	(void*)ksyms[NL_UBC_PAGER].n_value;

	kernel_floor =	(void*)ksyms[NL_KENTER].n_value;
	nchash_addr =	ksyms[NL_NCHASH].n_value;

	_KDEREF(kd, ksyms[NL_MAXSSIZ].n_value, &maxssiz,
		sizeof(maxssiz));
	_KDEREF(kd, ksyms[NL_NCHASHTBL].n_value, &nchashtbl_addr,
	       sizeof(nchashtbl_addr));
	_KDEREF(kd, ksyms[NL_KERNEL_MAP].n_value, &kernel_map_addr,
		sizeof(kernel_map_addr));

	/*
	 * Some of these may be missing from some platforms, for
	 * example sparc, sh3, and most powerpc platforms don't
	 * have a "phys_map", etc.
	 */
	(void)kvm_nlist(kd, &kmaps[0]);

#define get_map_address(m) do {\
	if (kmaps[__CONCAT(NL_,m)].n_value != 0) \
		_KDEREF(kd, kmaps[__CONCAT(NL_,m)].n_value, &m, sizeof(m)); \
	} while (0/*CONSTCOND*/)

	get_map_address(kmem_map);
	get_map_address(mb_map);
	get_map_address(phys_map);
	get_map_address(exec_map);
	get_map_address(pager_map);
	get_map_address(st_map);
	get_map_address(pt_map);
	get_map_address(lkm_map);
	get_map_address(buf_map);

	mib[0] = CTL_HW;
	mib[1] = HW_PAGESIZE;
	sz = sizeof(page_size);
	if (sysctl(&mib[0], 2, &page_size, &sz, NULL, 0) == -1)
		err(1, "sysctl: hw.pagesize");
}

const char *
mapname(void *addr)
{

	if (addr == (void*)kernel_map_addr)
		return ("kernel_map");
	else if (addr == kmem_map)
		return ("kmem_map");
	else if (addr == mb_map)
		return ("mb_map");
	else if (addr == phys_map)
		return ("phys_map");
	else if (addr == exec_map)
		return ("exec_map");
	else if (addr == pager_map)
		return ("pager_map");
	else if (addr == st_map)
		return ("st_map");
	else if (addr == pt_map)
		return ("pt_map");
	else if (addr == lkm_map)
		return ("lkm_map");
	else if (addr == buf_map)
		return ("buf_map");
	else
		return (NULL);
}

void
load_name_cache(kvm_t *kd)
{
	struct namecache _ncp, *ncp, *oncp;
	struct nchashhead _ncpp, *ncpp; 
	u_long nchash;
	int i;

	LIST_INIT(&lcache);

	_KDEREF(kd, nchash_addr, &nchash, sizeof(nchash));
	nchashtbl = malloc(sizeof(nchashtbl) * (int)(nchash + 1));
	_KDEREF(kd, nchashtbl_addr, nchashtbl,
		sizeof(nchashtbl) * (int)(nchash + 1));

	ncpp = &_ncpp;

	for (i = 0; i <= nchash; i++) {
		ncpp = &nchashtbl[i];
		oncp = NULL;
		LIST_FOREACH(ncp, ncpp, nc_hash) {
			if (ncp == oncp ||
			    (void*)ncp < kernel_floor ||
			    ncp == (void*)0xdeadbeef)
				break;
			oncp = ncp;
			_KDEREF(kd, (u_long)ncp, &_ncp, sizeof(*ncp));
			ncp = &_ncp;
			if ((void*)ncp->nc_vp > kernel_floor &&
			    ncp->nc_nlen > 0) {
				if (ncp->nc_nlen > 2 ||
				    ncp->nc_name[0] != '.' ||
				    (ncp->nc_name[1] != '.' &&
				     ncp->nc_nlen != 1))
					cache_enter(i, ncp);
			}
		}
	}
}

void
cache_enter(int i, struct namecache *ncp)
{
	struct cache_entry *ce;

	if (debug & DUMP_NAMEI_CACHE)
		printf("[%d] ncp->nc_vp %10p, ncp->nc_dvp %10p, "
		       "ncp->nc_nlen %3d [%.*s]\n",
		       i, ncp->nc_vp, ncp->nc_dvp,
		       ncp->nc_nlen, ncp->nc_nlen, ncp->nc_name);

	ce = malloc(sizeof(struct cache_entry));
	
	ce->ce_vp = ncp->nc_vp;
	ce->ce_pvp = ncp->nc_dvp;
	ce->ce_nlen = ncp->nc_nlen;
	strncpy(ce->ce_name, ncp->nc_name, sizeof(ce->ce_name));
	ce->ce_name[MIN(ce->ce_nlen, sizeof(ce->ce_name) - 1)] = '\0';

	LIST_INSERT_HEAD(&lcache, ce, ce_next);
}
