/* $NetBSD: lkminit_emul.c,v 1.2 2002/03/27 20:54:29 kent Exp $ */

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Michael Graff <explorer@flame.org>.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
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
__KERNEL_RCSID(0, "$NetBSD: lkminit_emul.c,v 1.2 2002/03/27 20:54:29 kent Exp $");

#include <sys/param.h>
#include <sys/syscall.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/lkm.h>

#include <compat/pecoff/pecoff_exec.h>
extern struct sysent sysent[];	/* sys/kern/init_sysent.c */
extern struct sysent pecoff_sysent[];	/* sys/compat/pecoff/pecoff_sysent.c*/

int compat_pecoff_lkmload(struct lkm_table *lkmtp, int cmd);
int compat_pecoff_lkmentry __P((struct lkm_table *, int, int));

/*
 * declare the emulation
 */
MOD_COMPAT("compat_pecoff", -1, &emul_pecoff);

int
compat_pecoff_lkmload(struct lkm_table *lkmtp, int cmd)
{
	/*
	 * Copy some syscall entries.
	 */
	pecoff_sysent[SYS_compat_10_oshmsys] = sysent[SYS_compat_10_oshmsys];
	pecoff_sysent[SYS_shmat] = sysent[SYS_shmat];
	pecoff_sysent[SYS_shmdt] = sysent[SYS_shmdt];
	pecoff_sysent[SYS_shmget] = sysent[SYS_shmget];
	pecoff_sysent[SYS___shmctl13] = sysent[SYS___shmctl13];
	return 0;
}

/*
 * entry point
 */
int
compat_pecoff_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;	
	int cmd;
	int ver;
{

	DISPATCH(lkmtp, cmd, ver, compat_pecoff_lkmload,
		 lkm_nofunc, lkm_nofunc);
}
