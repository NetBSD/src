/*	$NetBSD: netbsd32_exec.c,v 1.16 1999/10/11 01:36:22 eeh Exp $	*/
/*	from: NetBSD: exec_aout.c,v 1.15 1996/09/26 23:34:46 cgd Exp */

/*
 * Copyright (c) 1998 Matthew R. Green.
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
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
 *      This product includes software developed by Christopher G. Demetriou.
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

#include "opt_compat_sunos.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/resourcevar.h>
#include <sys/signal.h>
#include <sys/signalvar.h>

#include <vm/vm.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_exec.h>
#include <compat/netbsd32/netbsd32_syscall.h>

#include <machine/frame.h>
#include <machine/netbsd32_machdep.h>

const char netbsd32_emul_path[] = "/emul/netbsd32";
extern char netbsd32_sigcode[], netbsd32_esigcode[];
extern struct sysent netbsd32_sysent[];
#ifdef SYSCALL_DEBUG
extern char *netbsd32_syscallnames[];
#endif
void *netbsd32_copyargs __P((struct exec_package *, struct ps_strings *,
	void *, void *));
int netbsd32_copyinargs __P((struct ps_strings *, void *, size_t,
			      const void *, const void *));

static int netbsd32_exec_aout_prep_zmagic __P((struct proc *,
	struct exec_package *));
static int netbsd32_exec_aout_prep_nmagic __P((struct proc *,
	struct exec_package *));
static int netbsd32_exec_aout_prep_omagic __P((struct proc *,
	struct exec_package *));

struct emul emul_netbsd32 = {
	"netbsd32",
	NULL,
	netbsd32_sendsig,	/* XXX needs to be written */
	netbsd32_SYS_syscall,
	netbsd32_SYS_MAXSYSCALL,
	netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	netbsd32_syscallnames,
#else
	NULL,
#endif
	0,
#if FIXED
	netbsd32_copyinargs,
#endif
	netbsd32_copyargs,
	netbsd32_setregs,	/* XXX needs to be written?? */
	netbsd32_sigcode,
	netbsd32_esigcode,
};

/*
 * exec_netbsd32_makecmds(): Check if it's an netbsd32 a.out format
 * executable.
 *
 * Given a proc pointer and an exec package pointer, see if the referent
 * of the epp is in netbsd32 a.out format.  Check 'standard' magic
 * numbers for this architecture.
 *
 * This function, in the former case, or the hook, in the latter, is
 * responsible for creating a set of vmcmds which can be used to build
 * the process's vm space and inserting them into the exec package.
 */

int
exec_netbsd32_makecmds(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	netbsd32_u_long midmag, magic;
	u_short mid;
	int error;
	struct netbsd32_exec *execp = epp->ep_hdr;

	if (epp->ep_hdrvalid < sizeof(struct netbsd32_exec))
		return ENOEXEC;

	midmag = (netbsd32_u_long)ntohl(execp->a_midmag);
	mid = (midmag >> 16) & 0x3ff;
	magic = midmag & 0xffff;

	midmag = mid << 16 | magic;

	switch (midmag) {
	case (MID_SPARC << 16) | ZMAGIC:
		error = netbsd32_exec_aout_prep_zmagic(p, epp);
		break;
	case (MID_SPARC << 16) | NMAGIC:
		error = netbsd32_exec_aout_prep_nmagic(p, epp);
		break;
	case (MID_SPARC << 16) | OMAGIC:
		error = netbsd32_exec_aout_prep_omagic(p, epp);
		break;
	default:
		/* Invalid magic */
		error = ENOEXEC;
		break;
	}

	if (error == 0) {
		/* set up our emulation information */
		epp->ep_emul = &emul_netbsd32;
	} else
		kill_vmcmds(&epp->ep_vmcmds);

	return error;
}

/*
 * netbsd32_exec_aout_prep_zmagic(): Prepare a 'native' ZMAGIC binary's
 * exec package
 *
 * First, set of the various offsets/lengths in the exec package.
 *
 * Then, mark the text image busy (so it can be demand paged) or error
 * out if this is not possible.  Finally, set up vmcmds for the
 * text, data, bss, and stack segments.
 */

int
netbsd32_exec_aout_prep_zmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct netbsd32_exec *execp = epp->ep_hdr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/*
	 * check if vnode is in open for writing, because we want to
	 * demand-page out of it.  if it is, don't do it, for various
	 * reasons
	 */
	if ((execp->a_text != 0 || execp->a_data != 0) &&
	    epp->ep_vp->v_writecount != 0) {
#ifdef DIAGNOSTIC
		if (epp->ep_vp->v_flag & VTEXT)
			panic("exec: a VTEXT vnode has writecount != 0\n");
#endif
		return ETXTBSY;
	}
	epp->ep_vp->v_flag |= VTEXT;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, 0, VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_pagedvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, execp->a_bss,
	    epp->ep_daddr + execp->a_data, NULLVP, 0,
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * netbsd32_exec_aout_prep_nmagic(): Prepare a 'native' NMAGIC binary's
 * exec package
 */

int
netbsd32_exec_aout_prep_nmagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct netbsd32_exec *execp = epp->ep_hdr;
	long bsize, baddr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = roundup(epp->ep_taddr + execp->a_text, __LDPGSZ);
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_text,
	    epp->ep_taddr, epp->ep_vp, sizeof(struct exec),
	    VM_PROT_READ|VM_PROT_EXECUTE);

	/* set up command for data segment */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn, execp->a_data,
	    epp->ep_daddr, epp->ep_vp, execp->a_text + sizeof(struct exec),
	    VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	return exec_aout_setup_stack(p, epp);
}

/*
 * netbsd32_exec_aout_prep_omagic(): Prepare a 'native' OMAGIC binary's
 * exec package
 */

int
netbsd32_exec_aout_prep_omagic(p, epp)
	struct proc *p;
	struct exec_package *epp;
{
	struct netbsd32_exec *execp = epp->ep_hdr;
	long dsize, bsize, baddr;

	epp->ep_taddr = USRTEXT;
	epp->ep_tsize = execp->a_text;
	epp->ep_daddr = epp->ep_taddr + execp->a_text;
	epp->ep_dsize = execp->a_data + execp->a_bss;
	epp->ep_entry = execp->a_entry;

	/* set up command for text and data segments */
	NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_readvn,
	    execp->a_text + execp->a_data, epp->ep_taddr, epp->ep_vp,
	    sizeof(struct exec), VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/* set up command for bss segment */
	baddr = roundup(epp->ep_daddr + execp->a_data, NBPG);
	bsize = epp->ep_daddr + epp->ep_dsize - baddr;
	if (bsize > 0)
		NEW_VMCMD(&epp->ep_vmcmds, vmcmd_map_zero, bsize, baddr,
		    NULLVP, 0, VM_PROT_READ|VM_PROT_WRITE|VM_PROT_EXECUTE);

	/*
	 * Make sure (# of pages) mapped above equals (vm_tsize + vm_dsize);
	 * obreak(2) relies on this fact. Both `vm_tsize' and `vm_dsize' are
	 * computed (in execve(2)) by rounding *up* `ep_tsize' and `ep_dsize'
	 * respectively to page boundaries.
	 * Compensate `ep_dsize' for the amount of data covered by the last
	 * text page. 
	 */
	dsize = epp->ep_dsize + execp->a_text - roundup(execp->a_text, NBPG);
	epp->ep_dsize = (dsize > 0) ? dsize : 0;
	return exec_aout_setup_stack(p, epp);
}

/*
 * We need to copy out all pointers as 32-bit values.
 */
void *
netbsd32_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	u_int32_t *cpp = stack;
	u_int32_t dp;
	u_int32_t nullp = 0;
	char *sp;
	size_t len;
	int argc = arginfo->ps_nargvstr;
	int envc = arginfo->ps_nenvstr;

	if (copyout(&argc, cpp++, sizeof(argc)))
		return NULL;

	dp = (u_long) (cpp + argc + envc + 2 + pack->ep_emul->e_arglen);
	sp = argp;

	/* XXX don't copy them out, remap them! */
	arginfo->ps_argvstr = (caddr_t)(u_long)cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, (char *)(u_long)dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	arginfo->ps_envstr = (caddr_t)(u_long)cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len)
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, (char *)(u_long)dp, ARG_MAX, &len))
			return NULL;

	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	return cpp;
}

/*
 * Copy in args and env passed in by the exec syscall.
 *
 * Since this is a 32-bit emulation, pointers are 32-bits wide so this
 * routine needs to copyin 32-bit pointers and convert them.
 */
int
netbsd32_copyinargs(arginfo, destp, len, argp, envp)
	struct ps_strings *arginfo;
	void *destp;
	size_t len;
	const void *argp;
	const void *envp;
{
	char * const *cpp;
	char *dp;
	u_int32_t sp;
	long argc, envc;
	size_t l;
	int error = 0;

	dp = *(char **)destp;
	argc = 0;
	if ((cpp = argp) == NULL)
		return EINVAL;

	while (1) {
		l = len;
		if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
			return (error);
		if (!sp)
			break;
		if ((error = copyinstr((char *)(u_long)sp, dp, l, &l)) != 0) {
			if (error == ENAMETOOLONG)
				error = E2BIG;
			return (error);
		}
		dp += l;
		len -= l;
		cpp++;
		argc++;
	}

	envc = 0;
	/* environment need not be there */
	if ((cpp = envp) != NULL ) {
		while (1) {
			l = len;
			if ((error = copyin(cpp, &sp, sizeof(sp))) != 0)
				return (error);
			if (!sp)
				break;
			if ((error = copyinstr((char *)(u_long)sp, dp, l, &l)) != 0) {
				if (error == ENAMETOOLONG)
					error = E2BIG;
				return (error);
			}
			dp += l;
			len -= l;
			cpp++;
			envc++;
		}
	}

	/* adjust "active stack depth" for process VSZ */
	*(char**)destp = dp;
	arginfo->ps_nargvstr = argc;
	arginfo->ps_nenvstr = envc;

	return (error);
}
