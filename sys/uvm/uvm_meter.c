/*	$NetBSD: uvm_meter.c,v 1.30 2004/03/24 07:50:49 junyoung Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Charles D. Cranor,
 *      Washington University, and the University of California, Berkeley
 *      and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__KERNEL_RCSID(0, "$NetBSD: uvm_meter.c,v 1.30 2004/03/24 07:50:49 junyoung Exp $");

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <uvm/uvm_extern.h>
#include <sys/sysctl.h>

/*
 * maxslp: ???? XXXCDC
 */

int maxslp = MAXSLP;	/* patchable ... */
struct loadavg averunnable;

/*
 * constants for averages over 1, 5, and 15 minutes when sampling at
 * 5 second intervals.
 */

static fixpt_t cexp[3] = {
	0.9200444146293232 * FSCALE,	/* exp(-1/12) */
	0.9834714538216174 * FSCALE,	/* exp(-1/60) */
	0.9944598480048967 * FSCALE,	/* exp(-1/180) */
};

/*
 * prototypes
 */

static void uvm_loadav(struct loadavg *);
static void uvm_total(struct vmtotal *);

/*
 * uvm_meter: calculate load average and wake up the swapper (if needed)
 */
void
uvm_meter()
{
	if ((time.tv_sec % 5) == 0)
		uvm_loadav(&averunnable);
	if (lwp0.l_slptime > (maxslp / 2))
		wakeup(&proc0);
}

/*
 * uvm_loadav: compute a tenex style load average of a quantity on
 * 1, 5, and 15 minute internvals.
 */
static void
uvm_loadav(avg)
	struct loadavg *avg;
{
	int i, nrun;
	struct lwp *l;

	proclist_lock_read();
	nrun = 0;
	LIST_FOREACH(l, &alllwp, l_list) {
		switch (l->l_stat) {
		case LSSLEEP:
			if (l->l_priority > PZERO || l->l_slptime > 1)
				continue;
		/* fall through */
		case LSRUN:
		case LSONPROC:
		case LSIDL:
			nrun++;
		}
	}
	proclist_unlock_read();
	for (i = 0; i < 3; i++)
		avg->ldavg[i] = (cexp[i] * avg->ldavg[i] +
		    nrun * FSCALE * (FSCALE - cexp[i])) >> FSHIFT;
}

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

	node = *rnode;
	if (oldp)
		node.sysctl_size = min(*oldlenp, node.sysctl_size);

	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

static int
sysctl_vm_uvmexp2(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct uvmexp_sysctl u;

	memset(&u, 0, sizeof(u));

	/* Entries here are in order of uvmexp_sysctl, not uvmexp */
	u.pagesize = uvmexp.pagesize;
	u.pagemask = uvmexp.pagemask;
	u.pageshift = uvmexp.pageshift;
	u.npages = uvmexp.npages;
	u.free = uvmexp.free;
	u.active = uvmexp.active;
	u.inactive = uvmexp.inactive;
	u.paging = uvmexp.paging;
	u.wired = uvmexp.wired;
	u.zeropages = uvmexp.zeropages;
	u.reserve_pagedaemon = uvmexp.reserve_pagedaemon;
	u.reserve_kernel = uvmexp.reserve_kernel;
	u.freemin = uvmexp.freemin;
	u.freetarg = uvmexp.freetarg;
	u.inactarg = uvmexp.inactarg;
	u.wiredmax = uvmexp.wiredmax;
	u.nswapdev = uvmexp.nswapdev;
	u.swpages = uvmexp.swpages;
	u.swpginuse = uvmexp.swpginuse;
	u.swpgonly = uvmexp.swpgonly;
	u.nswget = uvmexp.nswget;
	u.nanon = uvmexp.nanon;
	u.nanonneeded = uvmexp.nanonneeded;
	u.nfreeanon = uvmexp.nfreeanon;
	u.faults = uvmexp.faults;
	u.traps = uvmexp.traps;
	u.intrs = uvmexp.intrs;
	u.swtch = uvmexp.swtch;
	u.softs = uvmexp.softs;
	u.syscalls = uvmexp.syscalls;
	u.pageins = uvmexp.pageins;
	u.swapins = uvmexp.swapins;
	u.swapouts = uvmexp.swapouts;
	u.pgswapin = uvmexp.pgswapin;
	u.pgswapout = uvmexp.pgswapout;
	u.forks = uvmexp.forks;
	u.forks_ppwait = uvmexp.forks_ppwait;
	u.forks_sharevm = uvmexp.forks_sharevm;
	u.pga_zerohit = uvmexp.pga_zerohit;
	u.pga_zeromiss = uvmexp.pga_zeromiss;
	u.zeroaborts = uvmexp.zeroaborts;
	u.fltnoram = uvmexp.fltnoram;
	u.fltnoanon = uvmexp.fltnoanon;
	u.fltpgwait = uvmexp.fltpgwait;
	u.fltpgrele = uvmexp.fltpgrele;
	u.fltrelck = uvmexp.fltrelck;
	u.fltrelckok = uvmexp.fltrelckok;
	u.fltanget = uvmexp.fltanget;
	u.fltanretry = uvmexp.fltanretry;
	u.fltamcopy = uvmexp.fltamcopy;
	u.fltnamap = uvmexp.fltnamap;
	u.fltnomap = uvmexp.fltnomap;
	u.fltlget = uvmexp.fltlget;
	u.fltget = uvmexp.fltget;
	u.flt_anon = uvmexp.flt_anon;
	u.flt_acow = uvmexp.flt_acow;
	u.flt_obj = uvmexp.flt_obj;
	u.flt_prcopy = uvmexp.flt_prcopy;
	u.flt_przero = uvmexp.flt_przero;
	u.pdwoke = uvmexp.pdwoke;
	u.pdrevs = uvmexp.pdrevs;
	u.pdswout = uvmexp.pdswout;
	u.pdfreed = uvmexp.pdfreed;
	u.pdscans = uvmexp.pdscans;
	u.pdanscan = uvmexp.pdanscan;
	u.pdobscan = uvmexp.pdobscan;
	u.pdreact = uvmexp.pdreact;
	u.pdbusy = uvmexp.pdbusy;
	u.pdpageouts = uvmexp.pdpageouts;
	u.pdpending = uvmexp.pdpending;
	u.pddeact = uvmexp.pddeact;
	u.anonpages = uvmexp.anonpages;
	u.filepages = uvmexp.filepages;
	u.execpages = uvmexp.execpages;
	u.colorhit = uvmexp.colorhit;
	u.colormiss = uvmexp.colormiss;

	node = *rnode;
	node.sysctl_data = &u;
	node.sysctl_size = sizeof(u);
	return (sysctl_lookup(SYSCTLFN_CALL(&node)));
}

/*
 * sysctl helper routine for the vm.{anon,exec,file}{min,max} nodes.
 * makes sure that they all correlate properly and none are set too
 * large.
 */
static int
sysctl_vm_updateminmax(SYSCTLFN_ARGS)
{
	int t, error;
	struct sysctlnode node;

	node = *rnode;
	node.sysctl_data = &t;
	t = *(int*)rnode->sysctl_data;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if (t < 0 || t > 100)
		return (EINVAL);

#define UPDATEMIN(a, ap, bp, cp, tp) do { \
		if (tp + uvmexp.bp + uvmexp.cp > 95) \
			return (EINVAL); \
		uvmexp.ap = tp; \
		uvmexp.a = uvmexp.ap * 256 / 100; \
	} while (0/*CONSTCOND*/)

#define UPDATEMAX(a, ap, tp) do { \
		uvmexp.ap = tp; \
		uvmexp.a = tp * 256 / 100; \
	} while (0/*CONSTCOND*/)

	switch (rnode->sysctl_num) {
	case VM_ANONMIN:
		UPDATEMIN(anonmin, anonminpct, fileminpct, execminpct, t);
		break;
	case VM_EXECMIN:
		UPDATEMIN(execmin, execminpct, anonminpct, fileminpct, t);
		break;
	case VM_FILEMIN:
		UPDATEMIN(filemin, fileminpct, execminpct, anonminpct, t);
		break;
	case VM_ANONMAX:
		UPDATEMAX(anonmax, anonmaxpct, t);
		break;
	case VM_EXECMAX:
		UPDATEMAX(execmax, execmaxpct, t);
		break;
	case VM_FILEMAX:
		UPDATEMAX(filemax, filemaxpct, t);
		break;
	default:
		return (EINVAL);
	}

#undef UPDATEMIN
#undef UPDATEMAX

	return (0);
}

/*
 * uvm_sysctl: sysctl hook into UVM system.
 */
SYSCTL_SETUP(sysctl_vm_setup, "sysctl vm subtree setup")
{

	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_NODE, "vm", NULL,
		       NULL, 0, NULL, 0,
		       CTL_VM, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_STRUCT, "vmmeter", NULL,
		       sysctl_vm_meter, 0, NULL, sizeof(struct vmtotal),
		       CTL_VM, VM_METER, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_STRUCT, "loadavg", NULL,
		       NULL, 0, &averunnable, sizeof(averunnable),
		       CTL_VM, VM_LOADAVG, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_STRUCT, "uvmexp", NULL,
		       sysctl_vm_uvmexp, 0, &uvmexp, sizeof(uvmexp),
		       CTL_VM, VM_UVMEXP, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_INT, "nkmempages", NULL,
		       NULL, 0, &nkmempages, 0,
		       CTL_VM, VM_NKMEMPAGES, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_STRUCT, "uvmexp2", NULL,
		       sysctl_vm_uvmexp2, 0, NULL, 0,
		       CTL_VM, VM_UVMEXP2, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_READWRITE,
		       CTLTYPE_INT, "anonmin", NULL,
		       sysctl_vm_updateminmax, 0, &uvmexp.anonminpct, 0,
		       CTL_VM, VM_ANONMIN, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_READWRITE,
		       CTLTYPE_INT, "execmin", NULL,
		       sysctl_vm_updateminmax, 0, &uvmexp.execminpct, 0,
		       CTL_VM, VM_EXECMIN, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_READWRITE,
		       CTLTYPE_INT, "filemin", NULL,
		       sysctl_vm_updateminmax, 0, &uvmexp.fileminpct, 0,
		       CTL_VM, VM_FILEMIN, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT,
		       CTLTYPE_INT, "maxslp", NULL,
		       NULL, 0, &maxslp, 0,
		       CTL_VM, VM_MAXSLP, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_IMMEDIATE,
		       CTLTYPE_INT, "uspace", NULL,
		       NULL, USPACE, NULL, 0,
		       CTL_VM, VM_USPACE, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_READWRITE,
		       CTLTYPE_INT, "anonmax", NULL,
		       sysctl_vm_updateminmax, 0, &uvmexp.anonmaxpct, 0,
		       CTL_VM, VM_ANONMAX, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_READWRITE,
		       CTLTYPE_INT, "execmax", NULL,
		       sysctl_vm_updateminmax, 0, &uvmexp.execmaxpct, 0,
		       CTL_VM, VM_EXECMAX, CTL_EOL);
	sysctl_createv(SYSCTL_PERMANENT|SYSCTL_READWRITE,
		       CTLTYPE_INT, "filemax", NULL,
		       sysctl_vm_updateminmax, 0, &uvmexp.filemaxpct, 0,
		       CTL_VM, VM_FILEMAX, CTL_EOL);
}

/*
 * uvm_total: calculate the current state of the system.
 */
static void
uvm_total(totalp)
	struct vmtotal *totalp;
{
	struct lwp *l;
#if 0
	struct vm_map_entry *	entry;
	struct vm_map *map;
	int paging;
#endif

	memset(totalp, 0, sizeof *totalp);

	/*
	 * calculate process statistics
	 */

	proclist_lock_read();
	    LIST_FOREACH(l, &alllwp, l_list) {
		if (l->l_proc->p_flag & P_SYSTEM)
			continue;
		switch (l->l_stat) {
		case 0:
			continue;

		case LSSLEEP:
		case LSSTOP:
			if (l->l_flag & L_INMEM) {
				if (l->l_priority <= PZERO)
					totalp->t_dw++;
				else if (l->l_slptime < maxslp)
					totalp->t_sl++;
			} else if (l->l_slptime < maxslp)
				totalp->t_sw++;
			if (l->l_slptime >= maxslp)
				continue;
			break;

		case LSRUN:
		case LSONPROC:
		case LSIDL:
			if (l->l_flag & L_INMEM)
				totalp->t_rq++;
			else
				totalp->t_sw++;
			if (l->l_stat == LSIDL)
				continue;
			break;
		}
		/*
		 * note active objects
		 */
#if 0
		/*
		 * XXXCDC: BOGUS!  rethink this.   in the mean time
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
	proclist_unlock_read();
	/*
	 * Calculate object memory usage statistics.
	 */
	totalp->t_free = uvmexp.free;
	totalp->t_vm = uvmexp.npages - uvmexp.free + uvmexp.swpginuse;
	totalp->t_avm = uvmexp.active + uvmexp.swpginuse;	/* XXX */
	totalp->t_rm = uvmexp.npages - uvmexp.free;
	totalp->t_arm = uvmexp.active;
	totalp->t_vmshr = 0;		/* XXX */
	totalp->t_avmshr = 0;		/* XXX */
	totalp->t_rmshr = 0;		/* XXX */
	totalp->t_armshr = 0;		/* XXX */
}
