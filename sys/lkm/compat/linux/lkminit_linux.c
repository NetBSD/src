/* $NetBSD: lkminit_linux.c,v 1.4 1997/05/19 22:11:17 jtc Exp $ */

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

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
#include <sys/lkm.h>
#include <sys/file.h>
#include <sys/errno.h>

#include <machine/cpu.h>
#include <machine/reg.h>

#include <compat/linux/linux_exec.h>

extern int exec_linux_aout_makecmds();
extern struct emul *emul_linux_elf;
extern struct emul *emul_linux_aout;
extern struct emul _emul_linux_elf;
extern struct emul _emul_linux_aout;

struct execsw linux_lkm_execsw = {
  LINUX_AOUT_HDR_SIZE, exec_linux_aout_makecmds,
};

/*
 * declare the filesystem
 */
MOD_EXEC("compat_linux", -1, &linux_lkm_execsw);

/*
 * entry point
 */
int
compat_linux_lkmentry(lkmtp, cmd, ver)
	struct lkm_table *lkmtp;	
	int cmd;
	int ver;
{
	DISPATCH(lkmtp, cmd, ver,
		 compat_linux_lkmload,
		 compat_linux_lkmunload,
		 lkm_nofunc);
}

int
compat_linux_lkmload(lkmtp, cmd)
	struct lkm_table *lkmtp;	
	int cmd;
{
	if (elf_probe_funcs_insert(linux_elf32_probe))
		return 1;  /* Failure! */
	emul_linux_aout = &_emul_linux_aout;
	emul_linux_elf = &_emul_linux_elf;

	return 0;
}

/*
 * Note that it is NOT safe to unload this at all...
 */
int
compat_linux_lkmunload(lkmtp, cmd)
	struct lkm_table *lkmtp;	
	int cmd;
{
	if (elf_probe_funcs_insert(linux_elf32_probe))
		return 1;  /* Failure! */
	emul_linux_aout = NULL;
	emul_linux_elf = NULL;

	return 0;
}
