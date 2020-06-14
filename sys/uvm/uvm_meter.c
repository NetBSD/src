/*	$NetBSD: uvm_meter.c,v 1.80 2020/06/14 21:41:42 ad Exp $	*/

/*
 * Copyright (c) 1997 Charles D. Cranor and Washington University.
 * Copyright (c) 1982, 1986, 1989, 1993
 *      The Regents of the University of California.
 *
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
 *      @(#)vm_meter.c  8.4 (Berkeley) 1/4/94
 * from: Id: uvm_meter.c,v 1.1.2.1 1997/08/14 19:10:35 chuck Exp
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uvm_meter.c,v 1.80 2020/06/14 21:41:42 ad Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>

#include <uvm/uvm.h>
#include <uvm/uvm_pdpolicy.h>

/*
 * maxslp: ???? XXXCDC
 */

int maxslp = MAXSLP;	/* patchable ... */
struct loadavg averunnable;

static void uvm_total(struct vmtotal *);

/*
 * sysctl helper routine for the vm.vmmeter node.
 */
static int
sysctl_vm_meter(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct vmtotal vmtotals;

	node = *rnode;
	node.sysctl_data = &vmtotals;
	uvm_total(&vmtotals);

	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for the vm.uvmexp node.
 */
static int
sysctl_vm_uvmexp(SYSCTLFN_ARGS)
{
	struct sysctlnode node;

	uvm_update_uvmexp();

	node = *rnode;
	if (oldlenp)
		node.sysctl_size = uimin(*oldlenp, node.sysctl_size);

	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

static int
sysctl_vm_uvmexp2(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct uvmexp_sysctl u;
	int active, inactive;

	uvm_estimatepageable(&active, &inactive);

	memset(&u, 0, sizeof(u));

	/* uvm_availmem() will sync the counters if old. */
	u.free = uvm_availmem(true);
	u.pagesize = uvmexp.pagesize;
	u.pagemask = uvmexp.pagemask;
	u.pageshift = uvmexp.pageshift;
	u.npages = uvmexp.npages;
	u.active = active;
	u.inactive = inactive;
	u.paging = uvmexp.paging;
	u.wired = uvmexp.wired;
	u.reserve_pagedaemon = uvmexp.reserve_pagedaemon;
	u.reserve_kernel = uvmexp.reserve_kernel;
	u.freemin = uvmexp.freemin;
	u.freetarg = uvmexp.freetarg;
	u.inactarg = 0; /* unused */
	u.wiredmax = uvmexp.wiredmax;
	u.nswapdev = uvmexp.nswapdev;
	u.swpages = uvmexp.swpages;
	u.swpginuse = uvmexp.swpginuse;
	u.swpgonly = uvmexp.swpgonly;
	u.nswget = uvmexp.nswget;
	u.cpuhit = cpu_count_get(CPU_COUNT_CPUHIT);
	u.cpumiss = cpu_count_get(CPU_COUNT_CPUMISS);
	u.faults = cpu_count_get(CPU_COUNT_NFAULT);
	u.traps = cpu_count_get(CPU_COUNT_NTRAP);
	u.intrs = cpu_count_get(CPU_COUNT_NINTR);
	u.swtch = cpu_count_get(CPU_COUNT_NSWTCH);
	u.softs = cpu_count_get(CPU_COUNT_NSOFT);
	u.syscalls = cpu_count_get(CPU_COUNT_NSYSCALL);
	u.pageins = cpu_count_get(CPU_COUNT_PAGEINS);
	u.pgswapin = 0; /* unused */
	u.pgswapout = uvmexp.pgswapout;
	u.forks = cpu_count_get(CPU_COUNT_FORKS);
	u.forks_ppwait = cpu_count_get(CPU_COUNT_FORKS_PPWAIT);
	u.forks_sharevm = cpu_count_get(CPU_COUNT_FORKS_SHAREVM);
	u.zeroaborts = uvmexp.zeroaborts;
	u.fltnoram = cpu_count_get(CPU_COUNT_FLTNORAM);
	u.fltnoanon = cpu_count_get(CPU_COUNT_FLTNOANON);
	u.fltpgwait = cpu_count_get(CPU_COUNT_FLTPGWAIT);
	u.fltpgrele = cpu_count_get(CPU_COUNT_FLTPGRELE);
	u.fltrelck = cpu_count_get(CPU_COUNT_FLTRELCK);
	u.fltrelckok = cpu_count_get(CPU_COUNT_FLTRELCKOK);
	u.fltanget = cpu_count_get(CPU_COUNT_FLTANGET);
	u.fltanretry = cpu_count_get(CPU_COUNT_FLTANRETRY);
	u.fltamcopy = cpu_count_get(CPU_COUNT_FLTAMCOPY);
	u.fltnamap = cpu_count_get(CPU_COUNT_FLTNAMAP);
	u.fltnomap = cpu_count_get(CPU_COUNT_FLTNOMAP);
	u.fltlget = cpu_count_get(CPU_COUNT_FLTLGET);
	u.fltget = cpu_count_get(CPU_COUNT_FLTGET);
	u.flt_anon = cpu_count_get(CPU_COUNT_FLT_ANON);
	u.flt_acow = cpu_count_get(CPU_COUNT_FLT_ACOW);
	u.flt_obj = cpu_count_get(CPU_COUNT_FLT_OBJ);
	u.flt_prcopy = cpu_count_get(CPU_COUNT_FLT_PRCOPY);
	u.flt_przero = cpu_count_get(CPU_COUNT_FLT_PRZERO);
	u.pdwoke = uvmexp.pdwoke;
	u.pdrevs = uvmexp.pdrevs;
	u.pdfreed = uvmexp.pdfreed;
	u.pdscans = uvmexp.pdscans;
	u.pdanscan = uvmexp.pdanscan;
	u.pdobscan = uvmexp.pdobscan;
	u.pdreact = uvmexp.pdreact;
	u.pdbusy = uvmexp.pdbusy;
	u.pdpageouts = uvmexp.pdpageouts;
	u.pdpending = uvmexp.pdpending;
	u.pddeact = uvmexp.pddeact;
	u.execpages = cpu_count_get(CPU_COUNT_EXECPAGES);
	u.colorhit = cpu_count_get(CPU_COUNT_COLORHIT);
	u.colormiss = cpu_count_get(CPU_COUNT_COLORMISS);
	u.ncolors = uvmexp.ncolors;
	u.bootpages = uvmexp.bootpages;
	u.poolpages = pool_totalpages();
	u.countsyncall = cpu_count_get(CPU_COUNT_SYNC);
	u.anonunknown = cpu_count_get(CPU_COUNT_ANONUNKNOWN);
	u.anonclean = cpu_count_get(CPU_COUNT_ANONCLEAN);
	u.anondirty = cpu_count_get(CPU_COUNT_ANONDIRTY);
	u.fileunknown = cpu_count_get(CPU_COUNT_FILEUNKNOWN);
	u.fileclean = cpu_count_get(CPU_COUNT_FILECLEAN);
	u.filedirty = cpu_count_get(CPU_COUNT_FILEDIRTY);
	u.fltup = cpu_count_get(CPU_COUNT_FLTUP);
	u.fltnoup = cpu_count_get(CPU_COUNT_FLTNOUP);
	u.anonpages = u.anonclean + u.anondirty + u.anonunknown;
	u.filepages = u.fileclean + u.filedirty + u.fileunknown - u.execpages;

	node = *rnode;
	node.sysctl_data = &u;
	node.sysctl_size = sizeof(u);
	if (oldlenp)
		node.sysctl_size = uimin(*oldlenp, node.sysctl_size);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for uvm_pctparam.
 */
static int
uvm_sysctlpctparam(SYSCTLFN_ARGS)
{
	int t, error;
	struct sysctlnode node;
	struct uvm_pctparam *pct;

	pct = rnode->sysctl_data;
	t = pct->pct_pct;

	node = *rnode;
	node.sysctl_data = &t;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (t < 0 || t > 100)
		return EINVAL;

	error = uvm_pctparam_check(pct, t);
	if (error) {
		return error;
	}
	uvm_pctparam_set(pct, t);

	return (0);
}

/*
 * uvm_sysctl: sysctl hook into UVM system.
 */
SYSCTL_SETUP(sysctl_vm_setup, "sysctl vm subtree setup")
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "vmmeter",
		       SYSCTL_DESCR("Simple system-wide virtual memory "
				    "statistics"),
		       sysctl_vm_meter, 0, NULL, sizeof(struct vmtotal),
		       CTL_VM, VM_METER, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "loadavg",
		       SYSCTL_DESCR("System load average history"),
		       NULL, 0, &averunnable, sizeof(averunnable),
		       CTL_VM, VM_LOADAVG, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "uvmexp",
		       SYSCTL_DESCR("Detailed system-wide virtual memory "
				    "statistics"),
		       sysctl_vm_uvmexp, 0, &uvmexp, sizeof(uvmexp),
		       CTL_VM, VM_UVMEXP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "uvmexp2",
		       SYSCTL_DESCR("Detailed system-wide virtual memory "
				    "statistics (MI)"),
		       sysctl_vm_uvmexp2, 0, NULL, 0,
		       CTL_VM, VM_UVMEXP2, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT, CTLTYPE_INT, "maxslp",
		       SYSCTL_DESCR("Maximum process sleep time before being "
				    "swapped"),
		       NULL, 0, &maxslp, 0,
		       CTL_VM, VM_MAXSLP, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_INT, "uspace",
		       SYSCTL_DESCR("Number of bytes allocated for a kernel "
				    "stack"),
		       NULL, USPACE, NULL, 0,
		       CTL_VM, VM_USPACE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_LONG, "minaddress",
		       SYSCTL_DESCR("Minimum user address"),
		       NULL, VM_MIN_ADDRESS, NULL, 0,
		       CTL_VM, VM_MINADDRESS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_IMMEDIATE,
		       CTLTYPE_LONG, "maxaddress",
		       SYSCTL_DESCR("Maximum user address"),
		       NULL, VM_MAX_ADDRESS, NULL, 0,
		       CTL_VM, VM_MAXADDRESS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_UNSIGNED,
		       CTLTYPE_INT, "guard_size",
		       SYSCTL_DESCR("Guard size of main thread"),
		       NULL, 0, &user_stack_guard_size, 0,
		       CTL_VM, VM_GUARD_SIZE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_UNSIGNED|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "thread_guard_size",
		       SYSCTL_DESCR("Guard size of other threads"),
		       NULL, 0, &user_thread_stack_guard_size, 0,
		       CTL_VM, VM_THREAD_GUARD_SIZE, CTL_EOL);
#ifdef PMAP_DIRECT
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_BOOL, "ubc_direct",
		       SYSCTL_DESCR("Use direct map for UBC I/O"),
		       NULL, 0, &ubc_direct, 0,
		       CTL_VM, CTL_CREATE, CTL_EOL);
#endif

	uvmpdpol_sysctlsetup();
}

/*
 * uvm_total: calculate the current state of the system.
 */
static void
uvm_total(struct vmtotal *totalp)
{
	struct lwp *l;
#if 0
	struct vm_map_entry *	entry;
	struct vm_map *map;
	int paging;
#endif
	int freepg;
	int active;

	memset(totalp, 0, sizeof *totalp);

	/*
	 * calculate process statistics
	 */
	mutex_enter(&proc_lock);
	LIST_FOREACH(l, &alllwp, l_list) {
		if (l->l_proc->p_flag & PK_SYSTEM)
			continue;
		switch (l->l_stat) {
		case 0:
			continue;

		case LSSLEEP:
		case LSSTOP:
			if ((l->l_flag & LW_SINTR) == 0) {
				totalp->t_dw++;
			} else if (l->l_slptime < maxslp) {
				totalp->t_sl++;
			}
			if (l->l_slptime >= maxslp)
				continue;
			break;

		case LSRUN:
		case LSONPROC:
		case LSIDL:
			totalp->t_rq++;
			if (l->l_stat == LSIDL)
				continue;
			break;
		}
		/*
		 * note active objects
		 */
#if 0
		/*
		 * XXXCDC: BOGUS!  rethink this.  in the mean time
		 * don't do it.
		 */
		paging = 0;
		vm_map_lock(map);
		for (map = &p->p_vmspace->vm_map, entry = map->header.next;
		    entry != &map->header; entry = entry->next) {
			if (entry->is_a_map || entry->is_sub_map ||
			    entry->object.uvm_obj == NULL)
				continue;
			/* XXX how to do this with uvm */
		}
		vm_map_unlock(map);
		if (paging)
			totalp->t_pw++;
#endif
	}
	mutex_exit(&proc_lock);

	/*
	 * Calculate object memory usage statistics.
	 */
	freepg = uvm_availmem(true);
	uvm_estimatepageable(&active, NULL);
	totalp->t_free = freepg;
	totalp->t_vm = uvmexp.npages - freepg + uvmexp.swpginuse;
	totalp->t_avm = active + uvmexp.swpginuse;	/* XXX */
	totalp->t_rm = uvmexp.npages - freepg;
	totalp->t_arm = active;
	totalp->t_vmshr = 0;		/* XXX */
	totalp->t_avmshr = 0;		/* XXX */
	totalp->t_rmshr = 0;		/* XXX */
	totalp->t_armshr = 0;		/* XXX */
}

void
uvm_pctparam_set(struct uvm_pctparam *pct, int val)
{

	pct->pct_pct = val;
	pct->pct_scaled = val * UVM_PCTPARAM_SCALE / 100;
}

int
uvm_pctparam_get(struct uvm_pctparam *pct)
{

	return pct->pct_pct;
}

int
uvm_pctparam_check(struct uvm_pctparam *pct, int val)
{

	if (pct->pct_check == NULL) {
		return 0;
	}
	return (*pct->pct_check)(pct, val);
}

void
uvm_pctparam_init(struct uvm_pctparam *pct, int val,
    int (*fn)(struct uvm_pctparam *, int))
{

	pct->pct_check = fn;
	uvm_pctparam_set(pct, val);
}

int
uvm_pctparam_createsysctlnode(struct uvm_pctparam *pct, const char *name,
    const char *desc)
{

	return sysctl_createv(NULL, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, name, SYSCTL_DESCR(desc),
	    uvm_sysctlpctparam, 0, (void *)pct, 0, CTL_VM, CTL_CREATE, CTL_EOL);
}

/*
 * Update uvmexp with aggregate values from the per-CPU counters.
 */
void
uvm_update_uvmexp(void)
{

	/* uvm_availmem() will sync the counters if old. */
	uvmexp.free = (int)uvm_availmem(true);
	uvmexp.cpuhit = (int)cpu_count_get(CPU_COUNT_CPUHIT);
	uvmexp.cpumiss = (int)cpu_count_get(CPU_COUNT_CPUMISS);
	uvmexp.faults = (int)cpu_count_get(CPU_COUNT_NFAULT);
	uvmexp.traps = (int)cpu_count_get(CPU_COUNT_NTRAP);
	uvmexp.intrs = (int)cpu_count_get(CPU_COUNT_NINTR);
	uvmexp.swtch = (int)cpu_count_get(CPU_COUNT_NSWTCH);
	uvmexp.softs = (int)cpu_count_get(CPU_COUNT_NSOFT);
	uvmexp.syscalls = (int)cpu_count_get(CPU_COUNT_NSYSCALL);
	uvmexp.pageins = (int)cpu_count_get(CPU_COUNT_PAGEINS);
	uvmexp.forks = (int)cpu_count_get(CPU_COUNT_FORKS);
	uvmexp.forks_ppwait = (int)cpu_count_get(CPU_COUNT_FORKS_PPWAIT);
	uvmexp.forks_sharevm = (int)cpu_count_get(CPU_COUNT_FORKS_SHAREVM);
	uvmexp.fltnoram = (int)cpu_count_get(CPU_COUNT_FLTNORAM);
	uvmexp.fltnoanon = (int)cpu_count_get(CPU_COUNT_FLTNOANON);
	uvmexp.fltpgwait = (int)cpu_count_get(CPU_COUNT_FLTPGWAIT);
	uvmexp.fltpgrele = (int)cpu_count_get(CPU_COUNT_FLTPGRELE);
	uvmexp.fltrelck = (int)cpu_count_get(CPU_COUNT_FLTRELCK);
	uvmexp.fltrelckok = (int)cpu_count_get(CPU_COUNT_FLTRELCKOK);
	uvmexp.fltanget = (int)cpu_count_get(CPU_COUNT_FLTANGET);
	uvmexp.fltanretry = (int)cpu_count_get(CPU_COUNT_FLTANRETRY);
	uvmexp.fltamcopy = (int)cpu_count_get(CPU_COUNT_FLTAMCOPY);
	uvmexp.fltnamap = (int)cpu_count_get(CPU_COUNT_FLTNAMAP);
	uvmexp.fltnomap = (int)cpu_count_get(CPU_COUNT_FLTNOMAP);
	uvmexp.fltlget = (int)cpu_count_get(CPU_COUNT_FLTLGET);
	uvmexp.fltget = (int)cpu_count_get(CPU_COUNT_FLTGET);
	uvmexp.flt_anon = (int)cpu_count_get(CPU_COUNT_FLT_ANON);
	uvmexp.flt_acow = (int)cpu_count_get(CPU_COUNT_FLT_ACOW);
	uvmexp.flt_obj = (int)cpu_count_get(CPU_COUNT_FLT_OBJ);
	uvmexp.flt_prcopy = (int)cpu_count_get(CPU_COUNT_FLT_PRCOPY);
	uvmexp.flt_przero = (int)cpu_count_get(CPU_COUNT_FLT_PRZERO);
	uvmexp.anonpages = (int)(cpu_count_get(CPU_COUNT_ANONCLEAN) +
	    cpu_count_get(CPU_COUNT_ANONDIRTY) +
	    cpu_count_get(CPU_COUNT_ANONUNKNOWN));
    	uvmexp.filepages = (int)(cpu_count_get(CPU_COUNT_FILECLEAN) +
	    cpu_count_get(CPU_COUNT_FILEDIRTY) +
	    cpu_count_get(CPU_COUNT_FILEUNKNOWN) -
	    cpu_count_get(CPU_COUNT_EXECPAGES));
	uvmexp.execpages = (int)cpu_count_get(CPU_COUNT_EXECPAGES);
	uvmexp.colorhit = (int)cpu_count_get(CPU_COUNT_COLORHIT);
	uvmexp.colormiss = (int)cpu_count_get(CPU_COUNT_COLORMISS);
}
