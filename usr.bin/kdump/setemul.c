/*	$NetBSD: setemul.c,v 1.3.2.1 2000/07/20 22:23:09 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
__RCSID("$NetBSD: setemul.c,v 1.3.2.1 2000/07/20 22:23:09 jdolecek Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "setemul.h"

#include <sys/syscall.h>

#include "../../sys/compat/netbsd32/netbsd32_syscall.h"
#include "../../sys/compat/freebsd/freebsd_syscall.h"
#include "../../sys/compat/hpux/hpux_syscall.h"
#include "../../sys/compat/ibcs2/ibcs2_syscall.h"
#include "../../sys/compat/linux/linux_syscall.h"
#include "../../sys/compat/osf1/osf1_syscall.h"
#include "../../sys/compat/sunos/sunos_syscall.h"
#include "../../sys/compat/svr4/svr4_syscall.h"
#include "../../sys/compat/ultrix/ultrix_syscall.h"

#define KTRACE
#include "../../sys/kern/syscalls.c"

#include "../../sys/compat/netbsd32/netbsd32_syscalls.c"
#include "../../sys/compat/freebsd/freebsd_syscalls.c"
#include "../../sys/compat/hpux/hpux_syscalls.c"
#include "../../sys/compat/ibcs2/ibcs2_syscalls.c"
#include "../../sys/compat/linux/linux_syscalls.c"
#include "../../sys/compat/osf1/osf1_syscalls.c"
#include "../../sys/compat/sunos/sunos_syscalls.c"
#include "../../sys/compat/svr4/svr4_syscalls.c"
#include "../../sys/compat/ultrix/ultrix_syscalls.c"

#include "../../sys/compat/hpux/hpux_errno.c"
#include "../../sys/compat/svr4/svr4_errno.c"
#include "../../sys/compat/ibcs2/ibcs2_errno.c"
#include "../../sys/compat/linux/common/linux_errno.c"
#undef KTRACE

#define NELEM(a) (sizeof(a) / sizeof(a[0]))

static struct emulation emulations[] = {
	{   "netbsd",	       syscallnames,         SYS_MAXSYSCALL,
	        NULL,		        0 },
	{   "netbsd32", netbsd32_syscallnames,	     SYS_MAXSYSCALL,
	        NULL,		        0 },
	{  "freebsd",  freebsd_syscallnames, FREEBSD_SYS_MAXSYSCALL,
	        NULL,		        0 },
	{     "hpux",	  hpux_syscallnames,    HPUX_SYS_MAXSYSCALL,
	  native_to_hpux_errno,  NELEM(native_to_hpux_errno)  },
	{    "ibcs2",    ibcs2_syscallnames,   IBCS2_SYS_MAXSYSCALL,
	 native_to_ibcs2_errno,  NELEM(native_to_ibcs2_errno) },
	{    "linux",    linux_syscallnames,   LINUX_SYS_MAXSYSCALL,
	 native_to_linux_errno,  NELEM(native_to_linux_errno) },
	{     "osf1",     osf1_syscallnames,    OSF1_SYS_MAXSYSCALL,
	        NULL,		        0 },
	{    "sunos",    sunos_syscallnames,   SUNOS_SYS_MAXSYSCALL,
	        NULL,		        0 },
	{     "svr4",     svr4_syscallnames,    SVR4_SYS_MAXSYSCALL,
	  native_to_svr4_errno,  NELEM(native_to_svr4_errno)  },
	{   "ultrix",   ultrix_syscallnames,  ULTRIX_SYS_MAXSYSCALL,
	        NULL,			0 },
	{   "pecoff",	       syscallnames,         SYS_MAXSYSCALL,
	        NULL,		        0 },
	{       NULL,		       NULL,		          0,
	        NULL,			0 }
};

struct emulation_ctx {
	pid_t	pid;
	struct emulation *emulation;
	struct emulation_ctx *next;
};

struct emulation *current;

static struct emulation *default_emul=NULL;

struct emulation_ctx *current_ctx;
struct emulation_ctx *emul_ctx = NULL;

static struct emulation_ctx *ectx_find __P((pid_t));
static void	ectx_update __P((pid_t, struct emulation *));

void
setemul(name, pid, update_ectx)
	const char *name;
	pid_t pid;
	int update_ectx;
{
	int i;
	struct emulation *match = NULL;

	for (i = 0; emulations[i].name != NULL; i++) {
		if (strcmp(emulations[i].name, name) == 0) {
			match = &emulations[i];
			break;
		}
	}

	if (!match) {
		warnx("Emulation `%s' unknown", name);
		return;
	}

	if (update_ectx)
		ectx_update(pid, match);

	if (!default_emul)
		default_emul = match;

	current = match;
}

/*
 * Emulation context list is very simple chained list, not even hashed.
 * We expect the number of separate traced contexts/processes to be
 * fairly low, so it's not worth it to optimize this.
 */

/*
 * Find an emulation context appropriate for the given pid.
 */
static struct emulation_ctx *
ectx_find(pid)
	pid_t pid;
{
	struct emulation_ctx *ctx;

	for(ctx = emul_ctx; ctx != NULL; ctx = ctx->next) {
		if (ctx->pid == pid)
			return ctx;
	}

	return NULL;
}

/*
 * Update emulation context for given pid, or create new if no context
 * for this pid exists.
 */
static void
ectx_update(pid, emul)
	pid_t pid;
	struct emulation *emul;
{
	struct emulation_ctx *ctx;


	if ((ctx = ectx_find(pid)) != NULL) {
		/* found and entry, ensure the emulation is right (exec!) */
		ctx->emulation = emul;
		return;
	}
	
	ctx = (struct emulation_ctx *)malloc(sizeof(struct emulation_ctx));
	ctx->pid = pid;
	ctx->emulation = emul;
	
	/* put the entry on start of emul_ctx chain */
	ctx->next = emul_ctx;
	emul_ctx = ctx;
}

/*
 * Ensure current emulation context is correct for given pid.
 */
void
ectx_sanify(pid)
	pid_t pid;
{
	struct emulation_ctx *ctx;

	if ((ctx = ectx_find(pid)) != NULL)
		current = ctx->emulation;
	else if (default_emul)
		current = default_emul;
	else
		current = &emulations[0]; /* NetBSD */
}
