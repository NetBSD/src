/*	$NetBSD: powerpc_machdep.c,v 1.5.2.4 2002/06/23 17:39:47 jdolecek Exp $	*/

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
#include <sys/sysctl.h>
#include <sys/user.h>

int cpu_timebase;
int cpu_printfataltraps;

/*
 * Set set up registers on exec.
 */
void
setregs(p, pack, stack)
	struct proc *p;
	struct exec_package *pack;
	u_long stack;
{
	struct trapframe *tf = trapframe(p);
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
	tf->srr1 = PSL_MBO | PSL_USERSET | PSL_FE_DFLT;
	p->p_addr->u_pcb.pcb_flags = 0;
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
		/* Deprecated */
		return sysctl_rdint(oldp, oldlenp, newp, CACHELINESIZE);
	case CPU_TIMEBASE:
		if (cpu_timebase)
			return sysctl_rdint(oldp, oldlenp, newp, cpu_timebase);
		break;
	case CPU_PRINTFATALTRAPS:
		return sysctl_int(oldp, oldlenp, newp, newlen,
				  &cpu_printfataltraps);
	case CPU_CACHEINFO:
		/* Use this instead of CPU_CACHELINE */
		return sysctl_rdstruct(oldp, oldlenp, newp,
			&curcpu()->ci_ci, 
			sizeof(curcpu()->ci_ci));
	default:
		break;
	}
	return EOPNOTSUPP;
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
