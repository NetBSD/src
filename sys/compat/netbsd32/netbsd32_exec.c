/*	$NetBSD: netbsd32_exec.c,v 1.20 2000/06/06 19:04:17 soren Exp $	*/
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

#define	ELFSIZE		32

#include "opt_compat_sunos.h"
#include "opt_syscall_debug.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/vnode.h>
#include <sys/exec.h>
#include <sys/exec_elf.h>
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
void *netbsd32_elf32_copyargs __P((struct exec_package *, struct ps_strings *,
	void *, void *));
int netbsd32_copyinargs __P((struct exec_package *, struct ps_strings *, 
			     void *, size_t, const void *, const void *));

static int netbsd32_exec_aout_prep_zmagic __P((struct proc *,
	struct exec_package *));
static int netbsd32_exec_aout_prep_nmagic __P((struct proc *,
	struct exec_package *));
static int netbsd32_exec_aout_prep_omagic __P((struct proc *,
	struct exec_package *));

#ifdef EXEC_AOUT
struct emul emul_netbsd32 = {
	"netbsd32",
	NULL,
	netbsd32_sendsig,
	netbsd32_SYS_syscall,
	netbsd32_SYS_MAXSYSCALL,
	netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	netbsd32_syscallnames,
#else
	NULL,
#endif
	0,
	netbsd32_copyargs,
	netbsd32_setregs,
	netbsd32_sigcode,
	netbsd32_esigcode,
};
#endif /* EXEC_AOUT */

#ifdef EXEC_ELF32
/* Elf emul structure */
struct emul ELFNAMEEND(emul_netbsd32) = {
	"netbsd32",
	NULL,
	netbsd32_sendsig,
	netbsd32_SYS_syscall,
	netbsd32_SYS_MAXSYSCALL,
	netbsd32_sysent,
#ifdef SYSCALL_DEBUG
	netbsd32_syscallnames,
#else
	NULL,
#endif
	howmany(ELF_AUX_ENTRIES * sizeof(AuxInfo), sizeof(Elf_Addr)),
	netbsd32_elf32_copyargs,
	netbsd32_setregs,
	netbsd32_sigcode,
	netbsd32_esigcode,
};

extern int ELFNAME2(netbsd,signature) __P((struct proc *, struct exec_package *,
    Elf_Ehdr *));
int
ELFNAME2(netbsd32,probe)(p, epp, eh, itp, pos)
	struct proc *p;
	struct exec_package *epp;
	Elf_Ehdr *eh;
	char *itp;
	Elf_Addr *pos;
{
	int error;
	size_t i;
	const char *bp;

	if ((error = ELFNAME2(netbsd,signature)(p, epp, eh)) != 0)
		return error;

	if (itp[0]) {
		if ((error = emul_find(p, NULL, netbsd32_emul_path,
				       itp, &bp, 0)) && 
		    (error = emul_find(p, NULL, "", itp, &bp, 0)))
			return error;
		if ((error = copystr(bp, itp, MAXPATHLEN, &i)) != 0)
			return error;
		free((void *)bp, M_TEMP);
	}
	epp->ep_emul = &ELFNAMEEND(emul_netbsd32);
	epp->ep_flags |= EXEC_32;
	*pos = ELFDEFNNAME(NO_ADDR);
	return 0;
}
#endif

#ifdef EXEC_AOUT
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
		epp->ep_flags |= EXEC_32;
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
	vn_marktext(epp->ep_vp);

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

#endif

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
	arginfo->ps_argvstr = (char **)(u_long)cpp; /* remember location of argv for later */

	for (; --argc >= 0; sp += len, dp += len) {
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, (char *)(u_long)dp, ARG_MAX, &len))
			return NULL;
	}
	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	arginfo->ps_envstr = (char **)(u_long)cpp; /* remember location of envp for later */

	for (; --envc >= 0; sp += len, dp += len) {
		if (copyout(&dp, cpp++, sizeof(dp)) ||
		    copyoutstr(sp, (char *)(u_long)dp, ARG_MAX, &len))
			return NULL;
	}
	if (copyout(&nullp, cpp++, sizeof(nullp)))
		return NULL;

	return cpp;
}

/* round up and down to page boundaries. */
#define	ELF_ROUND(a, b)		(((a) + (b) - 1) & ~((b) - 1))
#define	ELF_TRUNC(a, b)		((a) & ~((b) - 1))

/*
 * Copy arguments onto the stack in the normal way, but add some
 * extra information in case of dynamic binding.
 */
void *
netbsd32_elf32_copyargs(pack, arginfo, stack, argp)
	struct exec_package *pack;
	struct ps_strings *arginfo;
	void *stack;
	void *argp;
{
	size_t len;
	AuxInfo ai[ELF_AUX_ENTRIES], *a;
	struct elf_args *ap;

	stack = netbsd32_copyargs(pack, arginfo, stack, argp);
	if (!stack)
		return NULL;

	a = ai;

	/*
	 * Push extra arguments on the stack needed by dynamically
	 * linked binaries
	 */
	if ((ap = (struct elf_args *)pack->ep_emul_arg)) {

		a->a_type = AT_PHDR;
		a->a_v = ap->arg_phaddr;
		a++;

		a->a_type = AT_PHENT;
		a->a_v = ap->arg_phentsize;
		a++;

		a->a_type = AT_PHNUM;
		a->a_v = ap->arg_phnum;
		a++;

		a->a_type = AT_PAGESZ;
		a->a_v = NBPG;
		a++;

		a->a_type = AT_BASE;
		a->a_v = ap->arg_interp;
		a++;

		a->a_type = AT_FLAGS;
		a->a_v = 0;
		a++;

		a->a_type = AT_ENTRY;
		a->a_v = ap->arg_entry;
		a++;

		free((char *)ap, M_TEMP);
		pack->ep_emul_arg = NULL;
	}

	a->a_type = AT_NULL;
	a->a_v = 0;
	a++;

	len = (a - ai) * sizeof(AuxInfo);
	if (copyout(ai, stack, len))
		return NULL;
	stack = (caddr_t)stack + len;

	return stack;
}

