/*	$NetBSD: nlist.c,v 1.28 2020/08/26 10:54:12 simonb Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Simon Burge.
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

/*
 * Copyright (c) 1990, 1993, 1994
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
#if 0
static char sccsid[] = "@(#)nlist.c	8.4 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: nlist.c,v 1.28 2020/08/26 10:54:12 simonb Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/lwp.h>
#include <sys/proc.h>
#include <sys/resource.h>
#include <sys/sysctl.h>

#include <err.h>
#include <errno.h>
#include <kvm.h>
#include <math.h>
#include <nlist.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ps.h"

struct	nlist psnl[] = {
	{ .n_name = "_fscale" },
#define	X_FSCALE	0
	{ .n_name = "_ccpu" },
#define	X_CCPU		1
	{ .n_name = "_physmem" },
#define	X_PHYSMEM	2
	{ .n_name = "_maxslp" },
#define	X_MAXSLP	3
	{ .n_name = NULL }
};

double	log_ccpu;			/* log of kernel _ccpu variable */
int	nlistread;			/* if nlist already read. */
int	mempages;			/* number of pages of phys. memory */
int	fscale;				/* kernel _fscale variable */
int	maxslp;				/* kernel _maxslp variable */
int	uspace;				/* kernel USPACE value */

/* XXX Hopefully reasonable default */
#define MEMPAGES 	0
#ifndef FSCALE
#define FSCALE		(1 << 8)
#endif
#define LOG_CCPU	(-1.0 / 20.0)
#ifndef MAXSLP
#define MAXSLP		20
#endif
#define DEF_USPACE	(getpagesize())

#define	kread(x, v) \
	kvm_read(kd, psnl[x].n_value, (char *)&v, sizeof v) != sizeof(v)

void
donlist(void)
{
	fixpt_t xccpu;

	nlistread = 1;

	if (kvm_nlist(kd, psnl)) {
		nlisterr(psnl);
		eval = 1;
		fscale = FSCALE;
		mempages = MEMPAGES;
		log_ccpu = LOG_CCPU;
		maxslp = MAXSLP;
		return;
	}

	if (kread(X_FSCALE, fscale)) {
		warnx("fscale: %s", kvm_geterr(kd));
		eval = 1;
		fscale = FSCALE;
	}

	if (kread(X_PHYSMEM, mempages)) {
		warnx("avail_start: %s", kvm_geterr(kd));
		eval = 1;
		mempages = MEMPAGES;
	}

	if (kread(X_CCPU, xccpu)) {
		warnx("ccpu: %s", kvm_geterr(kd));
		eval = 1;
		log_ccpu = LOG_CCPU;
	} else
		log_ccpu = log((double)xccpu / fscale);

	if (kread(X_MAXSLP, maxslp)) {
		warnx("maxslp: %s", kvm_geterr(kd));
		eval = 1;
		maxslp = MAXSLP;
	}
}

void
donlist_sysctl(void)
{
	int mib[2];
	size_t size;
	fixpt_t xccpu;
	uint64_t memsize;

	nlistread = 1;

	mib[0] = CTL_KERN;
	mib[1] = KERN_FSCALE;
	size = sizeof(fscale);
	if (sysctl(mib, 2, &fscale, &size, NULL, 0)) {
		warn("sysctl kern.fscale");
		eval = 1;
		fscale = FSCALE;
	}

	mib[0] = CTL_HW;
	mib[1] = HW_PHYSMEM64;
	size = sizeof(memsize);
	if (sysctl(mib, 2, &memsize, &size, NULL, 0)) {
		warn("sysctl hw.avail_start");
		eval = 1;
		mempages = MEMPAGES;
	} else
		mempages = memsize / getpagesize();

	mib[0] = CTL_KERN;
	mib[1] = KERN_CCPU;
	size = sizeof(xccpu);
	if (sysctl(mib, 2, &xccpu, &size, NULL, 0)) {
		warn("sysctl kern.ccpu");
		eval = 1;
		log_ccpu = LOG_CCPU;
	} else
		log_ccpu = log((double)xccpu / fscale);

	mib[0] = CTL_VM;
	mib[1] = VM_MAXSLP;
	size = sizeof(maxslp);
	if (sysctl(mib, 2, &maxslp, &size, NULL, 0)) {
		warn("sysctl vm.maxslp");
		eval = 1;
		maxslp = MAXSLP;
	}

	mib[0] = CTL_VM;
	mib[1] = VM_USPACE;
	size = sizeof(uspace);
	if (sysctl(mib, 2, &uspace, &size, NULL, 0)) {
		warn("sysctl vm.uspace");
		eval = 1;
		uspace = DEF_USPACE;
	}
}

void
nlisterr(struct nlist nl[])
{
	int i;

	(void)fprintf(stderr, "ps: nlist: can't find following symbols:");
	for (i = 0; nl[i].n_name != NULL; i++)
		if (nl[i].n_value == 0)
			(void)fprintf(stderr, " %s", nl[i].n_name);
	(void)fprintf(stderr, "\n");
}
