/*	$NetBSD: powerpc_machdep.c,v 1.27 2004/04/04 17:05:31 matt Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: powerpc_machdep.c,v 1.27 2004/04/04 17:05:31 matt Exp $");

#include "opt_altivec.h"

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/disklabel.h>
#include <sys/exec.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/sa.h>
#include <sys/savar.h>
#include <sys/signal.h>
#include <sys/sysctl.h>
#include <sys/ucontext.h>
#include <sys/user.h>

int cpu_timebase;
int cpu_printfataltraps;
#ifdef PPC_OEA
extern int powersave;
#endif

extern struct pool siginfo_pool;

/*
 * Set set up registers on exec.
 */
void
setregs(struct lwp *l, struct exec_package *pack, u_long stack)
{
	struct proc *p = l->l_proc;
	struct trapframe *tf = trapframe(l);
	struct ps_strings arginfo;

	memset(tf, 0, sizeof *tf);
	tf->fixreg[1] = -roundup(-stack + 8, 16);

	/*
	 * XXX Machine-independent code has already copied arguments and
	 * XXX environment to userland.  Get them back here.
	 */
	(void)copyin((char *)p->p_psstr, &arginfo, sizeof (arginfo));

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
	tf->fixreg[8] = (register_t)p->p_psstr;	/* NetBSD extension */

	tf->srr0 = pack->ep_entry;
	tf->srr1 = PSL_MBO | PSL_USERSET;
#ifdef ALTIVEC
	tf->tf_xtra[TF_VRSAVE] = 0;
#endif
	l->l_addr->u_pcb.pcb_flags = PSL_FE_DFLT;
}

/*
 * Machine dependent system variables.
 */
static int
sysctl_machdep_cacheinfo(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

	node.sysctl_data = &curcpu()->ci_ci;
	node.sysctl_size = sizeof(curcpu()->ci_ci);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

#ifdef PPC_OEA
static int
sysctl_machdep_powersave(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;

	if (powersave < 0)
		node.sysctl_flags |= ~CTLFLAG_READWRITE;
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}
#endif

SYSCTL_SETUP(sysctl_machdep_setup, "sysctl machdep subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "machdep", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CTL_EOL);

	/* Deprecated */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "cachelinesize", NULL,
		       NULL, CACHELINESIZE, NULL, 0,
		       CTL_MACHDEP, CPU_CACHELINE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_INT, "timebase", NULL,
		       NULL, 0, &cpu_timebase, 0,
		       CTL_MACHDEP, CPU_TIMEBASE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "printfataltraps", NULL,
		       NULL, 0, &cpu_printfataltraps, 0,
		       CTL_MACHDEP, CPU_PRINTFATALTRAPS, CTL_EOL);
	/* Use this instead of CPU_CACHELINE */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "cacheinfo", NULL,
		       sysctl_machdep_cacheinfo, 0, NULL, 0,
		       CTL_MACHDEP, CPU_CACHEINFO, CTL_EOL);
#ifdef PPC_OEA
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "powersave", NULL,
		       sysctl_machdep_powersave, 0, &powersave, 0,
		       CTL_MACHDEP, CPU_POWERSAVE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "altivec", NULL,
		       NULL, cpu_altivec, NULL, 0,
		       CTL_MACHDEP, CPU_ALTIVEC, CTL_EOL);
#else
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "altivec", NULL,
		       NULL, 0, NULL, 0,
		       CTL_MACHDEP, CPU_ALTIVEC, CTL_EOL);
#endif
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRING, "model", NULL,
		       NULL, 0, cpu_model, 0,
		       CTL_MACHDEP, CPU_MODEL, CTL_EOL);
}

/*
 * Crash dump handling.
 */
u_int32_t dumpmag = 0x8fca0101;		/* magic number */
int dumpsize = 0;			/* size of dump in pages */
long dumplo = -1;			/* blocks */

/*
 * This is called by main to set dumplo and dumpsize.
 */
void
cpu_dumpconf(void)
{
	const struct bdevsw *bdev;
	int nblks;		/* size of dump device */
	int skip;

	if (dumpdev == NODEV)
		return;
	bdev = bdevsw_lookup(dumpdev);
	if (bdev == NULL)
		panic("dumpconf: bad dumpdev=0x%x", dumpdev);
	if (bdev->d_psize == NULL)
		return;
	nblks = (*bdev->d_psize)(dumpdev);
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

void 
cpu_upcall(struct lwp *l, int type, int nevents, int ninterrupted,
	void *sas, void *ap, void *sp, sa_upcall_t upcall)
{
	struct trapframe *tf;

	tf = trapframe(l);

	/*
	 * Build context to run handler in.
	 */
	tf->fixreg[1] = (register_t)((struct saframe *)sp - 1);
	tf->lr = 0;
	tf->fixreg[3] = (register_t)type;
	tf->fixreg[4] = (register_t)sas;
	tf->fixreg[5] = (register_t)nevents;
	tf->fixreg[6] = (register_t)ninterrupted;
	tf->fixreg[7] = (register_t)ap;
	tf->srr0 = (register_t)upcall;
}
