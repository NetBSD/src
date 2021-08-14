/*	$NetBSD: subr_prof.c,v 1.50 2021/08/14 17:51:20 ryo Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)subr_prof.c	8.4 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_prof.c,v 1.50 2021/08/14 17:51:20 ryo Exp $");

#ifdef _KERNEL_OPT
#include "opt_gprof.h"
#include "opt_multiprocessor.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/sysctl.h>

#include <sys/cpu.h>

#ifdef GPROF
#include <sys/malloc.h>
#include <sys/gmon.h>
#include <sys/xcall.h>

MALLOC_DEFINE(M_GPROF, "gprof", "kernel profiling buffer");

static int sysctl_kern_profiling(SYSCTLFN_ARGS);
#ifdef MULTIPROCESSOR
void _gmonparam_merge(struct gmonparam *, struct gmonparam *);
#endif

/*
 * Froms is actually a bunch of unsigned shorts indexing tos
 */
struct gmonparam _gmonparam = { .state = GMON_PROF_OFF };

/* Actual start of the kernel text segment. */
extern char kernel_text[];

extern char etext[];


void
kmstartup(void)
{
	char *cp;
	struct gmonparam *p = &_gmonparam;
	unsigned long size;
	/*
	 * Round lowpc and highpc to multiples of the density we're using
	 * so the rest of the scaling (here and in gprof) stays in ints.
	 */
	p->lowpc = rounddown(((u_long)kernel_text),
		HISTFRACTION * sizeof(HISTCOUNTER));
	p->highpc = roundup((u_long)etext,
		HISTFRACTION * sizeof(HISTCOUNTER));
	p->textsize = p->highpc - p->lowpc;
	printf("Profiling kernel, textsize=%ld [%lx..%lx]\n",
	       p->textsize, p->lowpc, p->highpc);
	p->kcountsize = p->textsize / HISTFRACTION;
	p->hashfraction = HASHFRACTION;
	p->fromssize = p->textsize / HASHFRACTION;
	p->tolimit = p->textsize * ARCDENSITY / 100;
	if (p->tolimit < MINARCS)
		p->tolimit = MINARCS;
	else if (p->tolimit > MAXARCS)
		p->tolimit = MAXARCS;
	p->tossize = p->tolimit * sizeof(struct tostruct);

	size = p->kcountsize + p->fromssize + p->tossize;
#ifdef MULTIPROCESSOR
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;
	for (CPU_INFO_FOREACH(cii, ci)) {
		p = malloc(sizeof(struct gmonparam) + size, M_GPROF,
		    M_NOWAIT | M_ZERO);
		if (p == NULL) {
			printf("No memory for profiling on %s\n",
			    cpu_name(ci));
			/* cannot profile on this cpu */
			continue;
		}
		memcpy(p, &_gmonparam, sizeof(_gmonparam));
		ci->ci_gmon = p;

		/*
		 * To allow profiling to be controlled only by the global
		 * _gmonparam.state, set the default value for each CPU to
		 * GMON_PROF_ON. If _gmonparam.state is not ON, mcount will
		 * not be executed.
		 * This is For compatibility of the kgmon(8) kmem interface.
		 */
		p->state = GMON_PROF_ON;

		cp = (char *)(p + 1);
		p->tos = (struct tostruct *)cp;
		p->kcount = (u_short *)(cp + p->tossize);
		p->froms = (u_short *)(cp + p->tossize + p->kcountsize);
	}

	sysctl_createv(NULL, 0, NULL, NULL,
	    0, CTLTYPE_NODE, "percpu",
	    SYSCTL_DESCR("per cpu profiling information"),
	    NULL, 0, NULL, 0,
	    CTL_KERN, KERN_PROF, GPROF_PERCPU, CTL_EOL);

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_gmon == NULL)
			continue;

		sysctl_createv(NULL, 0, NULL, NULL,
		    0, CTLTYPE_NODE, cpu_name(ci),
		    NULL,
		    NULL, 0, NULL, 0,
		    CTL_KERN, KERN_PROF, GPROF_PERCPU, cpu_index(ci), CTL_EOL);

		sysctl_createv(NULL, 0, NULL, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_INT, "state",
		    SYSCTL_DESCR("Profiling state"),
		    sysctl_kern_profiling, 0, (void *)ci, 0,
		    CTL_KERN, KERN_PROF, GPROF_PERCPU, cpu_index(ci),
		    GPROF_STATE, CTL_EOL);
		sysctl_createv(NULL, 0, NULL, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_STRUCT, "count",
		    SYSCTL_DESCR("Array of statistical program counters"),
		    sysctl_kern_profiling, 0, (void *)ci, 0,
		    CTL_KERN, KERN_PROF, GPROF_PERCPU, cpu_index(ci),
		    GPROF_COUNT, CTL_EOL);
		sysctl_createv(NULL, 0, NULL, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_STRUCT, "froms",
		    SYSCTL_DESCR("Array indexed by program counter of "
		    "call-from points"),
		    sysctl_kern_profiling, 0, (void *)ci, 0,
		    CTL_KERN, KERN_PROF, GPROF_PERCPU, cpu_index(ci),
		    GPROF_FROMS, CTL_EOL);
		sysctl_createv(NULL, 0, NULL, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_STRUCT, "tos",
		    SYSCTL_DESCR("Array of structures describing "
		    "destination of calls and their counts"),
		    sysctl_kern_profiling, 0, (void *)ci, 0,
		    CTL_KERN, KERN_PROF, GPROF_PERCPU, cpu_index(ci),
		    GPROF_TOS, CTL_EOL);
		sysctl_createv(NULL, 0, NULL, NULL,
		    CTLFLAG_READWRITE, CTLTYPE_STRUCT, "gmonparam",
		    SYSCTL_DESCR("Structure giving the sizes of the above "
		    "arrays"),
		    sysctl_kern_profiling, 0, (void *)ci, 0,
		    CTL_KERN, KERN_PROF, GPROF_PERCPU, cpu_index(ci),
		    GPROF_GMONPARAM, CTL_EOL);
	}

	/*
	 * For minimal compatibility of the kgmon(8) kmem interface,
	 * the _gmonparam and cpu0:ci_gmon share buffers.
	 */
	p = curcpu()->ci_gmon;
	if (p != NULL) {
		_gmonparam.tos = p->tos;
		_gmonparam.kcount = p->kcount;
		_gmonparam.froms = p->froms;
	}
#else /* MULTIPROCESSOR */
	cp = malloc(size, M_GPROF, M_NOWAIT | M_ZERO);
	if (cp == 0) {
		printf("No memory for profiling.\n");
		return;
	}
	p->tos = (struct tostruct *)cp;
	cp += p->tossize;
	p->kcount = (u_short *)cp;
	cp += p->kcountsize;
	p->froms = (u_short *)cp;
#endif /* MULTIPROCESSOR */
}

#ifdef MULTIPROCESSOR
static void
prof_set_state_xc(void *arg1, void *arg2 __unused)
{
	int state = PTRTOUINT64(arg1);
	struct gmonparam *gp = curcpu()->ci_gmon;

	if (gp != NULL)
		gp->state = state;
}
#endif /* MULTIPROCESSOR */

/*
 * Return kernel profiling information.
 */
/*
 * sysctl helper routine for kern.profiling subtree.  enables/disables
 * kernel profiling and gives out copies of the profiling data.
 */
static int
sysctl_kern_profiling(SYSCTLFN_ARGS)
{
	struct sysctlnode node = *rnode;
	struct gmonparam *gp;
	int error;
#ifdef MULTIPROCESSOR
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci, *target_ci;
	uint64_t where;
	int state;
	bool prof_on, do_merge;

	target_ci = (struct cpu_info *)rnode->sysctl_data;
	do_merge = (oldp != NULL) && (target_ci == NULL) &&
	    ((node.sysctl_num == GPROF_COUNT) ||
	    (node.sysctl_num == GPROF_FROMS) ||
	    (node.sysctl_num == GPROF_TOS));

	if (do_merge) {
		/* kern.profiling.{count,froms,tos} */
		unsigned long size;
		char *cp;

		/* allocate temporary gmonparam, and merge results of all CPU */
		size = _gmonparam.kcountsize + _gmonparam.fromssize +
		    _gmonparam.tossize;
		gp = malloc(sizeof(struct gmonparam) + size, M_GPROF,
		    M_NOWAIT | M_ZERO);
		if (gp == NULL)
			return ENOMEM;
		memcpy(gp, &_gmonparam, sizeof(_gmonparam));
		cp = (char *)(gp + 1);
		gp->tos = (struct tostruct *)cp;
		gp->kcount = (u_short *)(cp + gp->tossize);
		gp->froms = (u_short *)(cp + gp->tossize + gp->kcountsize);

		for (CPU_INFO_FOREACH(cii, ci)) {
			if (ci->ci_gmon == NULL)
				continue;
			_gmonparam_merge(gp, ci->ci_gmon);
		}
	} else if (target_ci != NULL) {
		/* kern.profiling.percpu.* */
		gp = target_ci->ci_gmon;
	} else {
		/* kern.profiling.{state,gmonparam} */
		gp = &_gmonparam;
	}
#else /* MULTIPROCESSOR */
	gp = &_gmonparam;
#endif

	switch (node.sysctl_num) {
	case GPROF_STATE:
#ifdef MULTIPROCESSOR
		/*
		 * if _gmonparam.state is OFF, the state of each CPU is
		 * considered to be OFF, even if it is actually ON.
		 */
		if (_gmonparam.state == GMON_PROF_OFF ||
		    gp->state == GMON_PROF_OFF)
			state = GMON_PROF_OFF;
		else
			state = GMON_PROF_ON;
		node.sysctl_data = &state;
#else
		node.sysctl_data = &gp->state;
#endif
		break;
	case GPROF_COUNT:
		node.sysctl_data = gp->kcount;
		node.sysctl_size = gp->kcountsize;
		break;
	case GPROF_FROMS:
		node.sysctl_data = gp->froms;
		node.sysctl_size = gp->fromssize;
		break;
	case GPROF_TOS:
		node.sysctl_data = gp->tos;
		node.sysctl_size = gp->tossize;
		break;
	case GPROF_GMONPARAM:
		node.sysctl_data = gp;
		node.sysctl_size = sizeof(*gp);
		break;
	default:
		return (EOPNOTSUPP);
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		goto done;

#ifdef MULTIPROCESSOR
	switch (node.sysctl_num) {
	case GPROF_STATE:
		if (target_ci != NULL) {
			where = xc_unicast(0, prof_set_state_xc,
			    UINT64TOPTR(state), NULL, target_ci);
			xc_wait(where);

			/* if even one CPU being profiled, enable perfclock. */
			prof_on = false;
			for (CPU_INFO_FOREACH(cii, ci)) {
				if (ci->ci_gmon == NULL)
					continue;
				if (ci->ci_gmon->state != GMON_PROF_OFF) {
					prof_on = true;
					break;
				}
			}
			mutex_spin_enter(&proc0.p_stmutex);
			if (prof_on)
				startprofclock(&proc0);
			else
				stopprofclock(&proc0);
			mutex_spin_exit(&proc0.p_stmutex);

			if (prof_on) {
				_gmonparam.state = GMON_PROF_ON;
			} else {
				_gmonparam.state = GMON_PROF_OFF;
				/*
				 * when _gmonparam.state and all CPU gmon state
				 * are OFF, all CPU states should be ON so that
				 * the entire CPUs profiling can be controlled
				 * by _gmonparam.state only.
				 */
				for (CPU_INFO_FOREACH(cii, ci)) {
					if (ci->ci_gmon == NULL)
						continue;
					ci->ci_gmon->state = GMON_PROF_ON;
				}
			}
		} else {
			_gmonparam.state = state;
			where = xc_broadcast(0, prof_set_state_xc,
			    UINT64TOPTR(state), NULL);
			xc_wait(where);

			mutex_spin_enter(&proc0.p_stmutex);
			if (state == GMON_PROF_OFF)
				stopprofclock(&proc0);
			else
				startprofclock(&proc0);
			mutex_spin_exit(&proc0.p_stmutex);
		}
		break;
	case GPROF_COUNT:
		/*
		 * if 'kern.profiling.{count,froms,tos}' is written, the same
		 * data will be written to 'kern.profiling.percpu.cpuN.xxx'
		 */
		if (target_ci == NULL) {
			for (CPU_INFO_FOREACH(cii, ci)) {
				if (ci->ci_gmon == NULL)
					continue;
				memmove(ci->ci_gmon->kcount, gp->kcount,
				    newlen);
			}
		}
		break;
	case GPROF_FROMS:
		if (target_ci == NULL) {
			for (CPU_INFO_FOREACH(cii, ci)) {
				if (ci->ci_gmon == NULL)
					continue;
				memmove(ci->ci_gmon->froms, gp->froms, newlen);
			}
		}
		break;
	case GPROF_TOS:
		if (target_ci == NULL) {
			for (CPU_INFO_FOREACH(cii, ci)) {
				if (ci->ci_gmon == NULL)
					continue;
				memmove(ci->ci_gmon->tos, gp->tos, newlen);
			}
		}
		break;
	}
#else
	if (node.sysctl_num == GPROF_STATE) {
		mutex_spin_enter(&proc0.p_stmutex);
		if (gp->state == GMON_PROF_OFF)
			stopprofclock(&proc0);
		else
			startprofclock(&proc0);
		mutex_spin_exit(&proc0.p_stmutex);
	}
#endif

 done:
#ifdef MULTIPROCESSOR
	if (do_merge)
		free(gp, M_GPROF);
#endif
	return error;
}

SYSCTL_SETUP(sysctl_kern_gprof_setup, "sysctl kern.profiling subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "profiling",
		       SYSCTL_DESCR("Profiling information (available)"),
		       NULL, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "state",
		       SYSCTL_DESCR("Profiling state"),
		       sysctl_kern_profiling, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, GPROF_STATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRUCT, "count",
		       SYSCTL_DESCR("Array of statistical program counters"),
		       sysctl_kern_profiling, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, GPROF_COUNT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRUCT, "froms",
		       SYSCTL_DESCR("Array indexed by program counter of "
				    "call-from points"),
		       sysctl_kern_profiling, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, GPROF_FROMS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_STRUCT, "tos",
		       SYSCTL_DESCR("Array of structures describing "
				    "destination of calls and their counts"),
		       sysctl_kern_profiling, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, GPROF_TOS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "gmonparam",
		       SYSCTL_DESCR("Structure giving the sizes of the above "
				    "arrays"),
		       sysctl_kern_profiling, 0, NULL, 0,
		       CTL_KERN, KERN_PROF, GPROF_GMONPARAM, CTL_EOL);
}
#endif /* GPROF */

/*
 * Profiling system call.
 *
 * The scale factor is a fixed point number with 16 bits of fraction, so that
 * 1.0 is represented as 0x10000.  A scale factor of 0 turns off profiling.
 */
/* ARGSUSED */
int
sys_profil(struct lwp *l, const struct sys_profil_args *uap, register_t *retval)
{
	/* {
		syscallarg(char *) samples;
		syscallarg(size_t) size;
		syscallarg(u_long) offset;
		syscallarg(u_int) scale;
	} */
	struct proc *p = l->l_proc;
	struct uprof *upp;

	if (SCARG(uap, scale) > (1 << 16))
		return (EINVAL);
	if (SCARG(uap, scale) == 0) {
		mutex_spin_enter(&p->p_stmutex);
		stopprofclock(p);
		mutex_spin_exit(&p->p_stmutex);
		return (0);
	}
	upp = &p->p_stats->p_prof;

	/* Block profile interrupts while changing state. */
	mutex_spin_enter(&p->p_stmutex);
	upp->pr_off = SCARG(uap, offset);
	upp->pr_scale = SCARG(uap, scale);
	upp->pr_base = SCARG(uap, samples);
	upp->pr_size = SCARG(uap, size);
	startprofclock(p);
	mutex_spin_exit(&p->p_stmutex);

	return (0);
}

/*
 * Scale is a fixed-point number with the binary point 16 bits
 * into the value, and is <= 1.0.  pc is at most 32 bits, so the
 * intermediate result is at most 48 bits.
 */
#define	PC_TO_INDEX(pc, prof) \
	((int)(((u_quad_t)((pc) - (prof)->pr_off) * \
	    (u_quad_t)((prof)->pr_scale)) >> 16) & ~1)

/*
 * Collect user-level profiling statistics; called on a profiling tick,
 * when a process is running in user-mode.  This routine may be called
 * from an interrupt context.  We schedule an AST that will vector us
 * to trap() with a context in which copyin and copyout will work.
 * Trap will then call addupc_task().
 *
 * XXX We could use ufetch/ustore here if the profile buffers were
 * wired.
 *
 * Note that we may (rarely) not get around to the AST soon enough, and
 * lose profile ticks when the next tick overwrites this one, but in this
 * case the system is overloaded and the profile is probably already
 * inaccurate.
 */
void
addupc_intr(struct lwp *l, u_long pc)
{
	struct uprof *prof;
	struct proc *p;
	u_int i;

	p = l->l_proc;

	KASSERT(mutex_owned(&p->p_stmutex));

	prof = &p->p_stats->p_prof;
	if (pc < prof->pr_off ||
	    (i = PC_TO_INDEX(pc, prof)) >= prof->pr_size)
		return;			/* out of range; ignore */

	mutex_spin_exit(&p->p_stmutex);

	/* XXXSMP */
	prof->pr_addr = pc;
	prof->pr_ticks++;
	cpu_need_proftick(l);

	mutex_spin_enter(&p->p_stmutex);
}

/*
 * Much like before, but we can afford to take faults here.  If the
 * update fails, we simply turn off profiling.
 */
void
addupc_task(struct lwp *l, u_long pc, u_int ticks)
{
	struct uprof *prof;
	struct proc *p;
	void *addr;
	int error;
	u_int i;
	u_short v;

	p = l->l_proc;

	if (ticks == 0)
		return;

	mutex_spin_enter(&p->p_stmutex);
	prof = &p->p_stats->p_prof;

	/* Testing P_PROFIL may be unnecessary, but is certainly safe. */
	if ((p->p_stflag & PST_PROFIL) == 0 || pc < prof->pr_off ||
	    (i = PC_TO_INDEX(pc, prof)) >= prof->pr_size) {
		mutex_spin_exit(&p->p_stmutex);
		return;
	}

	addr = prof->pr_base + i;
	mutex_spin_exit(&p->p_stmutex);
	if ((error = copyin(addr, (void *)&v, sizeof(v))) == 0) {
		v += ticks;
		error = copyout((void *)&v, addr, sizeof(v));
	}
	if (error != 0) {
		mutex_spin_enter(&p->p_stmutex);
		stopprofclock(p);
		mutex_spin_exit(&p->p_stmutex);
	}
}
