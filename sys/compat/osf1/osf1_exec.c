/* $NetBSD: osf1_exec.c,v 1.1 1999/04/26 18:34:05 cgd Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/exec_ecoff.h>
#include <sys/signalvar.h>
#include <sys/fcntl.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscall.h>

extern struct sysent osf1_sysent[];
extern char *osf1_syscallnames[];
extern void cpu_exec_ecoff_setregs __P((struct proc *, struct exec_package *,
					u_long));
extern char sigcode[], esigcode[];

struct emul emul_osf1 = {
	"osf1",
	netbsd_to_osf1_errno,
	sendsig,
	OSF1_SYS_syscall,
	OSF1_SYS_MAXSYSCALL,
	osf1_sysent,
	osf1_syscallnames,
#if 0 /* XXX notyet */
	MAX_AUX_ENTRIES * sizeof (struct osf1_auxv) +
	    2 * (MAXPATHLEN + 1),		/* exec name & loader name */
	osf1_copyargs,
#else
	0,
	copyargs,
#endif
	cpu_exec_ecoff_setregs,
	sigcode,
	esigcode,
};
