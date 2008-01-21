/* $NetBSD: lkminit_exec.c,v 1.7.4.2 2008/01/21 09:46:43 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: lkminit_exec.c,v 1.7.4.2 2008/01/21 09:46:43 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/exec.h>
#include <sys/proc.h>
#include <sys/lkm.h>
#include <sys/exec_elf.h>
#include <sys/signalvar.h>

int exec_elf_lkmentry(struct lkm_table *, int, int);

extern const struct emul emul_netbsd;	/* XXX */
extern int netbsd_elf32_probe(struct lwp *, struct exec_package *, void *,
				char *, vaddr_t *);

static struct execsw exec_elf =
	/* Native Elf32 */
	{ sizeof (Elf32_Ehdr),
	  exec_elf32_makecmds,
	  { netbsd_elf32_probe },
	  &emul_netbsd,
	  EXECSW_PRIO_ANY,
	  howmany(ELF_AUX_ENTRIES * sizeof(Aux32Info), sizeof (Elf32_Addr)),
	  elf32_copyargs,
	  NULL,
	  coredump_elf32,
	  exec_setup_stack };		/* XXX XXX XXX */

/*
 * declare the exec
 */
MOD_EXEC("exec_elf", -1, &exec_elf, "elf");

/*
 * entry point
 */
int
exec_elf_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;
	int cmd;
	int ver;
{
	DISPATCH(lkmtp, cmd, ver,
		 lkm_nofunc,
		 lkm_nofunc,
		 lkm_nofunc);
}
