/*	$NetBSD: powerpc_machdep.c,v 1.8.6.2 2001/11/05 19:46:18 briggs Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/exec.h>
#include <sys/lwp.h>
#include <sys/pool.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/sysctl.h>
#include <sys/ucontext.h>
#include <sys/user.h>

int cpu_timebase;
int cpu_printfataltraps;

/*
 * Set set up registers on exec.
 */
void
setregs(l, pack, stack)
	struct lwp *l;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = trapframe(l);
	struct ps_strings arginfo;

	memset(tf, 0, sizeof *tf);
	tf->fixreg[1] = -roundup(-stack + 8, 16);

	/*
	 * XXX Machine-independent code has already copied arguments and
	 * XXX environment to userland.  Get them back here.
	 */
	(void)copyin((char *)PS_STRINGS, &arginfo, sizeof (arginfo));

	/*
	 * Set up arguments for _start():
	 *	_start(argc, argv, envp, obj, cleanup, ps_strings);
	 *
	 * Notes:
	 *	- obj and cleanup are the auxiliary and termination
	 *	  vectors.  They are fixed up by ld.elf_so.
	 *	- ps_strings is a NetBSD extension, and will be
	 * 	  ignored by executables which are strictly
	 *	  compliant with the SVR4 ABI.
	 *
	 * XXX We have to set both regs and retval here due to different
	 * XXX calling convention in trap.c and init_main.c.
	 */
	tf->fixreg[3] = arginfo.ps_nargvstr;
	tf->fixreg[4] = (register_t)arginfo.ps_argvstr;
	tf->fixreg[5] = (register_t)arginfo.ps_envstr;
	tf->fixreg[6] = 0;			/* auxillary vector */
	tf->fixreg[7] = 0;			/* termination vector */
	tf->fixreg[8] = (register_t)PS_STRINGS;	/* NetBSD extension */

	tf->srr0 = pack->ep_entry;
	tf->srr1 = PSL_MBO | PSL_USERSET | PSL_FE_DFLT;
	l->l_addr->u_pcb.pcb_flags = 0;
}

/*
 * Machine dependent system variables.
 */
int
cpu_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return ENOTDIR;

	switch (name[0]) {
	case CPU_CACHELINE:
		return sysctl_rdint(oldp, oldlenp, newp, CACHELINESIZE);
	case CPU_TIMEBASE:
		if (cpu_timebase)
			return sysctl_rdint(oldp, oldlenp, newp, cpu_timebase);
		break;
	case CPU_PRINTFATALTRAPS:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &cpu_printfataltraps);
	default:
		break;
	}
	return EOPNOTSUPP;
}

/*
 * Crash dump handling.
 */
u_long dumpmag = 0x8fca0101;		/* magic number */
int dumpsize = 0;			/* size of dump in pages */
long dumplo = -1;			/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 */
void
cpu_dumpconf()
{
	int nblks;		/* size of dump device */
	int skip;
	int maj;

	if (dumpdev == NODEV)
		return;
	maj = major(dumpdev);
	if (maj < 0 || maj >= nblkdev)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdevsw[maj].d_psize == NULL)
		return;
	nblks = (*bdevsw[maj].d_psize)(dumpdev);
	if (nblks <= ctod(1))
		return;

	dumpsize = physmem;

	/* Skip enough blocks at start of disk to preserve an eventual disklabel. */
	skip = LABELSECTOR + 1;
	skip += ctod(1) - 1;
	skip = ctod(dtoc(skip));
	if (dumplo < skip)
		dumplo = skip;

	/* Put dump at end of partition */
	if (dumpsize > dtoc(nblks - dumplo))
		dumpsize = dtoc(nblks - dumplo);
	if (dumplo < nblks - ctod(dumpsize))
		dumplo = nblks - ctod(dumpsize);
}

ucontext_t *
cpu_stashcontext(struct lwp *lwp)
{
	ucontext_t u, *up;
	void *stack;
	struct trapframe *tf;
 
	tf = trapframe(lwp);
	stack = (char *) tf->fixreg[1] - sizeof(ucontext_t);
	getucontext(lwp, &u);
	up = stack;

	if (copyout(&u, stack, sizeof(ucontext_t)) != 0) {
		/* Copying onto the stack didn't work. Die. */
#ifdef DIAGNOSTIC
		printf("cpu_stashcontext: couldn't copyout context of %d.%d\n",
		    lwp->l_proc->p_pid, lwp->l_lid);
#endif   
		sigexit(lwp, SIGILL);
		/* NOTREACHED */
	}
 
	return up;
}

void
cpu_upcall(struct lwp *lwp)
{
	extern char sigcode[], upcallcode[];
	struct proc *p = lwp->l_proc;
	struct sadata *sd = p->p_sa;
	struct sa_t **sapp, *sap;
	struct sa_t self_sa, e_sa, int_sa;
	struct sa_t *sas[3];
	struct sadata_upcall *sau;
	struct trapframe *tf;
	void *stack;
	ucontext_t u, *up;
	int i, nsas, nevents, nint;
	int x, y;

	tf = trapframe(lwp);

	KDASSERT(LIST_EMPTY(&sd->sa_upcalls) == 0);

	sau = LIST_FIRST(&sd->sa_upcalls);

	stack = (char *) sau->sau_stack.ss_sp + sau->sau_stack.ss_size;

	self_sa.sa_id = lwp->l_lid;
	self_sa.sa_cpu = ppc_cpuid(lwp->l_cpu);
	sas[0] = &self_sa;
	nsas = 1;

	nevents = 0;
	if (sau->sau_event) {
		e_sa.sa_context = cpu_stashcontext(sau->sau_event);
		e_sa.sa_id = sau->sau_event->l_lid;
		e_sa.sa_cpu = ppc_cpuid(sau->sau_event->l_cpu);
		sas[nsas++] = &e_sa;
		nevents = 1;
	}

	nint = 0;
	if (sau->sau_interrupted) {
		int_sa.sa_context = cpu_stashcontext(sau->sau_interrupted);
		int_sa.sa_id = sau->sau_interrupted->l_lid;
		int_sa.sa_cpu = ppc_cpuid(sau->sau_interrupted->l_cpu);
		sas[nsas++] = &int_sa;
		nint = 1;
	}

	LIST_REMOVE(sau, sau_next);
	if (LIST_EMPTY(&sd->sa_upcalls))
		lwp->l_flag &= ~L_SA_UPCALL;

	/* Copy out the activation's ucontext */
	u.uc_stack = sau->sau_stack;
	u.uc_flags = _UC_STACK;
	up = stack;
	up--;
	if (copyout(&u, up, sizeof(ucontext_t)) != 0) {
		pool_put(&saupcall_pool, sau);
#ifdef DIAGNOSTIC
		printf("cpu_upcall: couldn't copyout activation ucontext"
		    " for %d.%d\n", lwp->l_proc->p_pid, lwp->l_lid);
#endif
		sigexit(lwp, SIGILL);
		/* NOTREACHED */
	}
	sas[0]->sa_context = up;

	/* Next, copy out the sa_t's and pointers to them. */
	sap = (struct sa_t *) up;
	sapp = (struct sa_t **) (sap - nsas);
	for (i = nsas - 1; i >= 0; i--) {
		sap--;
		sapp--;
		if (((x=copyout(sas[i], sap, sizeof(struct sa_t)) != 0)) ||
		    ((y=copyout(&sap, sapp, sizeof(struct sa_t *)) != 0))) {
			/* Copying onto the stack didn't work.  Die. */
			pool_put(&saupcall_pool, sau);
#ifdef DIAGNOSTIC
			printf("cpu_upcall: couldn't copyout sa_t %d for "
			    "%d.%d (x=%d, y=%d)\n",
			    i, lwp->l_proc->p_pid, lwp->l_lid, x, y);
#endif
			sigexit(lwp, SIGILL);
			/* NOTREACHED */
		}
	}

	/*
	 * Build context to run handler in.
	 */
	tf->fixreg[1] = (int)sapp;
	tf->lr = (int)sd->sa_upcall;
	tf->fixreg[3] = (int)sau->sau_type;
	tf->fixreg[4] = (int)sapp;
	tf->fixreg[5] = (int)nevents;
	tf->fixreg[6] = (int)nint;
	tf->fixreg[7] = (int)sau->sau_sig;
	tf->fixreg[8] = (int)sau->sau_code;
	tf->fixreg[9] = (int)sau->sau_arg;
	tf->srr0 = (int)((caddr_t) p->p_sigctx.ps_sigcode + (
	    (caddr_t)upcallcode - (caddr_t)sigcode));

	pool_put(&saupcall_pool, sau);
}

