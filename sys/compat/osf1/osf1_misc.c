/* $NetBSD: osf1_misc.c,v 1.62.2.6 2002/04/17 00:05:16 nathanw Exp $ */

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

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_misc.c,v 1.62.2.6 2002/04/17 00:05:16 nathanw Exp $");

#if defined(_KERNEL_OPT)
#include "opt_syscall_debug.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/signal.h>
#include <sys/signalvar.h>
#include <sys/reboot.h>
#include <sys/syscallargs.h>
#include <sys/exec.h>
#include <sys/vnode.h>
#include <sys/socketvar.h>
#include <sys/resource.h>
#include <sys/resourcevar.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <machine/alpha.h>
#include <machine/cpuconf.h>
#include <machine/rpb.h>
#include <machine/fpu.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>

#ifdef SYSCALL_DEBUG
extern int scdebug;
#endif

int
osf1_sys_classcntl(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{

	/* XXX */
	return (ENOSYS);
}

int
osf1_sys_reboot(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_reboot_args *uap = v;
	struct sys_reboot_args a;
	unsigned long leftovers;

	/* translate opt */
	SCARG(&a, opt) = emul_flags_translate(osf1_reboot_opt_xtab,
	    SCARG(uap, opt), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	SCARG(&a, bootstr) = NULL;

	return sys_reboot(l, &a, retval);
}

int
osf1_sys_set_program_attributes(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_set_program_attributes_args *uap = v;
	struct proc *p = l->l_proc;
	segsz_t tsize, dsize;

	tsize = btoc(SCARG(uap, tsize));
	dsize = btoc(SCARG(uap, dsize));

	if (dsize > p->p_rlimit[RLIMIT_DATA].rlim_cur)
		return (ENOMEM);
	if (tsize > MAXTSIZ)
		return (ENOMEM);

	p->p_vmspace->vm_taddr = SCARG(uap, taddr);
	p->p_vmspace->vm_tsize = tsize;
	p->p_vmspace->vm_daddr = SCARG(uap, daddr);
	p->p_vmspace->vm_dsize = dsize;

	return (0);
}

int
osf1_sys_getsysinfo(struct lwp *l, void *v, register_t *retval)
{
	extern int ncpus;
	struct osf1_sys_getsysinfo_args *uap = v;
	int error;
	int unit;
	long percpu;
	long proctype;
	u_int64_t fpflags;
	struct osf1_cpu_info cpuinfo;

	error = 0;

	switch(SCARG(uap, op))
	{
	case OSF_GET_MAX_UPROCS:
		error = copyout(&maxproc, SCARG(uap, buffer), sizeof(maxproc));
		retval[0] = 1;
		break;
	case OSF_GET_PHYSMEM:
		error = copyout(&physmem, SCARG(uap, buffer),
		    sizeof(physmem));
		retval[0] = 1;
		break;
	case OSF_GET_MAX_CPU:
	case OSF_GET_CPUS_IN_BOX:
		error = copyout(&ncpus, SCARG(uap, buffer), sizeof(ncpus));
		retval[0] = 1;
		break;
	case OSF_GET_IEEE_FP_CONTROL:
		if (((fpflags = alpha_read_fp_c(l)) & IEEE_INHERIT) != 0) {
			fpflags |= 1ULL << 63;
			fpflags &= ~IEEE_INHERIT;
		}
		error = copyout(&fpflags, SCARG(uap, buffer), sizeof fpflags);
		retval[0] = 1;
		break;
	case OSF_GET_CPU_INFO:

		if (SCARG(uap, nbytes) < sizeof(cpuinfo))
			error = EINVAL;
		else {
			memset(&cpuinfo, 0, sizeof(cpuinfo));
#ifdef __alpha__
			unit = alpha_pal_whami();
#else
			unit = 0; /* XXX */
#endif
			cpuinfo.current_cpu = unit;
			cpuinfo.cpus_in_box = ncpus;
			cpuinfo.cpu_type = 
			    LOCATE_PCS(hwrpb, unit)->pcs_proc_type;
			cpuinfo.ncpus = ncpus;
			cpuinfo.cpus_present = ncpus;
			cpuinfo.cpus_running = ncpus;
			cpuinfo.cpu_binding = 1;
			cpuinfo.cpu_ex_binding = 0;
			cpuinfo.mhz = hwrpb->rpb_cc_freq / 1000000;
			error = copyout(&cpuinfo, SCARG(uap, buffer),
			    sizeof(cpuinfo));
			retval[0] = 1;
		}
		break;
	case OSF_GET_PROC_TYPE:
		if(SCARG(uap, nbytes) < sizeof(proctype))
			error = EINVAL;
		else {
#ifdef __alpha__
			unit = alpha_pal_whami();
			proctype = LOCATE_PCS(hwrpb, unit)->pcs_proc_type;
#else
			proctype = 0;	/* XXX */
#endif
			error = copyout (&proctype, SCARG(uap, buffer),
			    sizeof(percpu));
			retval[0] = 1;
		}
	break;
	case OSF_GET_HWRPB: {  /* note -- osf/1 doesn't have rpb_tbhint[8] */
		unsigned long rpb_size;
		rpb_size = (unsigned long)hwrpb->rpb_size;
		if (SCARG(uap, nbytes) < rpb_size){
			uprintf("nbytes = %ld, sizeof(struct rpb) = %ld\n",
			    SCARG(uap, nbytes), rpb_size);
			error = EINVAL;
		}
		else {
			error = copyout(hwrpb, SCARG(uap, buffer), rpb_size);
			retval[0] = 1;
		}
	}
		break;
	case OSF_GET_PLATFORM_NAME:
		error = copyout(platform.model, SCARG(uap, buffer),
		    strlen(platform.model));
		retval[0] = 1;
		break;
	default:
		printf("osf1_getsysinfo called with unknown op=%ld\n",
		       SCARG(uap, op));
		//return EINVAL;
		return 0;
	}
	return(error);
}

int
osf1_sys_setsysinfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_setsysinfo_args *uap = v;
	u_int64_t temp;
	int error;

	error = 0;

	switch(SCARG(uap, op)) {
	case OSF_SET_IEEE_FP_CONTROL:

		if ((error = copyin(SCARG(uap, buffer), &temp, sizeof(temp))))
			break;
		if (temp >> 63 != 0)
			temp |= IEEE_INHERIT;
		alpha_write_fp_c(l, temp);
		break;
	default:
		uprintf("osf1_setsysinfo called with op=%ld\n", SCARG(uap, op));
		//error = EINVAL;
	}
	retval[0] = 0;
	return (error);
}

int
osf1_sys_sysinfo(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_sysinfo_args *uap = v;
	const char *string;
	int error;

	error = 0;
	switch (SCARG(uap, cmd)) {
	case OSF1_SI_SYSNAME:
		string = ostype;
		break;

	case OSF1_SI_HOSTNAME:
		string = hostname;
		break;

	case OSF1_SI_RELEASE:
		string = osrelease;
		break;

	case OSF1_SI_VERSION:
		goto should_handle;

	case OSF1_SI_MACHINE:
		string = MACHINE;
		break;

	case OSF1_SI_ARCHITECTURE:
		string = MACHINE_ARCH;
		break;

	case OSF1_SI_HW_SERIAL:
		string = "666";			/* OSF/1 emulation?  YES! */
		break;

	case OSF1_SI_HW_PROVIDER:
		string = "unknown";
		break;

	case OSF1_SI_SRPC_DOMAIN:
		goto dont_care;

	case OSF1_SI_SET_HOSTNAME:
		goto should_handle;

	case OSF1_SI_SET_SYSNAME:
		goto should_handle;

	case OSF1_SI_SET_SRPC_DOMAIN:
		goto dont_care;

	default:
should_handle:
		printf("osf1_sys_sysinfo(%d, %p, 0x%lx)\n", SCARG(uap, cmd),
		    SCARG(uap, buf), SCARG(uap,len));
dont_care:
		error = EINVAL;
		break;
	};

	if (error == 0)
		error = copyoutstr(string, SCARG(uap, buf), SCARG(uap, len),
		    NULL);

	return (error);
}

int
osf1_sys_uname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_uname_args *uap = v;
        struct osf1_utsname u;
        const char *cp;
        char *dp, *ep;

	/* XXX would use stackgap, but our struct utsname is too big! */

        strncpy(u.sysname, ostype, sizeof(u.sysname));
        strncpy(u.nodename, hostname, sizeof(u.nodename));
        strncpy(u.release, osrelease, sizeof(u.release));
        dp = u.version;
        ep = &u.version[sizeof(u.version) - 1];
        for (cp = version; *cp && *cp != '('; cp++)
                ;
        for (cp++; *cp && *cp != ')' && dp < ep; cp++)
                *dp++ = *cp;
        for (; *cp && *cp != '#'; cp++)
                ;
        for (; *cp && *cp != ':' && dp < ep; cp++)
                *dp++ = *cp;
        *dp = '\0';
        strncpy(u.machine, MACHINE, sizeof(u.machine));
        return (copyout((caddr_t)&u, (caddr_t)SCARG(uap, name), sizeof u));
}

int
osf1_sys_usleep_thread(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_usleep_thread_args *uap = v;
	struct osf1_timeval otv, endotv;
	struct timeval tv, endtv;
	u_long ticks;
	int error, s;

	if ((error = copyin(SCARG(uap, sleep), &otv, sizeof otv)))
		return (error);
	tv.tv_sec = otv.tv_sec;
	tv.tv_usec = otv.tv_usec;

	ticks = howmany((u_long)tv.tv_sec * 1000000 + tv.tv_usec, tick);
	if (ticks == 0)
		ticks = 1;

	s = splclock();
	tv = time;
	splx(s);

	tsleep(l, PUSER|PCATCH, "uslpthrd", ticks);	/* XXX */

	if (SCARG(uap, slept) != NULL) {
		s = splclock();
		timersub(&time, &tv, &endtv);
		splx(s);
		if (endtv.tv_sec < 0 || endtv.tv_usec < 0)
			endtv.tv_sec = endtv.tv_usec = 0;

		endotv.tv_sec = endtv.tv_sec;
		endotv.tv_usec = endtv.tv_usec;
		error = copyout(&endotv, SCARG(uap, slept), sizeof endotv);
	}
	return (error);
}

int
osf1_sys_wait4(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct osf1_sys_wait4_args *uap = v;
	struct proc *p = l->l_proc;
	struct sys_wait4_args a;
	struct osf1_rusage osf1_rusage;
	struct rusage netbsd_rusage;
	unsigned long leftovers;
	caddr_t sg;
	int error;

	SCARG(&a, pid) = SCARG(uap, pid);
	SCARG(&a, status) = SCARG(uap, status);

	/* translate options */
	SCARG(&a, options) = emul_flags_translate(osf1_wait_options_xtab,
	    SCARG(uap, options), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	if (SCARG(uap, rusage) == NULL)
		SCARG(&a, rusage) = NULL;
	else {
		sg = stackgap_init(p, 0);
		SCARG(&a, rusage) = stackgap_alloc(p, &sg, sizeof netbsd_rusage);
	}

	error = sys_wait4(l, &a, retval);

	if (error == 0 && SCARG(&a, rusage) != NULL) {
		error = copyin((caddr_t)SCARG(&a, rusage),
		    (caddr_t)&netbsd_rusage, sizeof netbsd_rusage);
		if (error == 0) {
			osf1_cvt_rusage_from_native(&netbsd_rusage,
			    &osf1_rusage);
			error = copyout((caddr_t)&osf1_rusage,
			    (caddr_t)SCARG(uap, rusage), sizeof osf1_rusage);
		}
	}

	return (error);
}
