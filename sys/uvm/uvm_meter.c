/*	$NetBSD: uvm_meter.c,v 1.10.2.3 2001/03/12 13:32:12 bouyer Exp $	*/

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

static void uvm_loadav __P((struct loadavg *));
static void uvm_total __P((struct vmtotal *));
static int sysctl_uvmexp __P((void *, size_t *));

/*
 * uvm_meter: calculate load average and wake up the swapper (if needed)
 */
void
uvm_meter()
{
	if ((time.tv_sec % 5) == 0)
		uvm_loadav(&averunnable);
	if (proc0.p_slptime > (maxslp / 2))
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
	struct proc *p;

	proclist_lock_read();
	nrun = 0;
	LIST_FOREACH(p, &allproc, p_list) {
		switch (p->p_stat) {
		case SSLEEP:
			if (p->p_priority > PZERO || p->p_slptime > 1)
				continue;
		/* fall through */
		case SRUN:
		case SONPROC:
		case SIDL:
			nrun++;
		}
	}
	proclist_unlock_read();
	for (i = 0; i < 3; i++)
		avg->ldavg[i] = (cexp[i] * avg->ldavg[i] +
		    nrun * FSCALE * (FSCALE - cexp[i])) >> FSHIFT;
}

/*
 * uvm_sysctl: sysctl hook into UVM system.
 */
int
uvm_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct vmtotal vmtotals;
	int rv, t;

	/* all sysctl names at this level are terminal */
	if (namelen != 1)
		return (ENOTDIR);		/* overloaded */

	switch (name[0]) {
	case VM_LOADAVG:
		return (sysctl_rdstruct(oldp, oldlenp, newp, &averunnable,
		    sizeof(averunnable)));

	case VM_METER:
		uvm_total(&vmtotals);
		return (sysctl_rdstruct(oldp, oldlenp, newp, &vmtotals,
		    sizeof(vmtotals)));

	case VM_UVMEXP:
		return (sysctl_rdminstruct(oldp, oldlenp, newp, &uvmexp,
		    sizeof(uvmexp)));
	case VM_UVMEXP2:
		if (newp)
			return (EPERM);
		return (sysctl_uvmexp(oldp, oldlenp));

	case VM_NKMEMPAGES:
		return (sysctl_rdint(oldp, oldlenp, newp, nkmempages));

	case VM_ANONMIN:
		t = uvmexp.anonminpct;
		rv = sysctl_int(oldp, oldlenp, newp, newlen, &t);
		if (rv) {
			return rv;
		}
		if (t + uvmexp.vtextminpct + uvmexp.vnodeminpct > 95 || t < 0) {
			return EINVAL;
		}
		uvmexp.anonminpct = t;
		uvmexp.anonmin = t * 256 / 100;
		return rv;

	case VM_VTEXTMIN:
		t = uvmexp.vtextminpct;
		rv = sysctl_int(oldp, oldlenp, newp, newlen, &t);
		if (rv) {
			return rv;
		}
		if (uvmexp.anonminpct + t + uvmexp.vnodeminpct > 95 || t < 0) {
			return EINVAL;
		}
		uvmexp.vtextminpct = t;
		uvmexp.vtextmin = t * 256 / 100;
		return rv;

	case VM_VNODEMIN:
		t = uvmexp.vnodeminpct;
		rv = sysctl_int(oldp, oldlenp, newp, newlen, &t);
		if (rv) {
			return rv;
		}
		if (uvmexp.anonminpct + uvmexp.vtextminpct + t > 95 || t < 0) {
			return EINVAL;
		}
		uvmexp.vnodeminpct = t;
		uvmexp.vnodemin = t * 256 / 100;
		return rv;

	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}

static int
sysctl_uvmexp(oldp, oldlenp)
	void *oldp;
	size_t *oldlenp;
{
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
	u.vnodepages = uvmexp.vnodepages;
	u.vtextpages = uvmexp.vtextpages;

	return (sysctl_rdminstruct(oldp, oldlenp, NULL, &u, sizeof(u)));
}

/*
 * uvm_total: calculate the current state of the system.
 */
static void
uvm_total(totalp)
	struct vmtotal *totalp;
{
	struct proc *p;
#if 0
	vm_map_entry_t	entry;
	vm_map_t map;
	int paging;
#endif

	memset(totalp, 0, sizeof *totalp);

	/*
	 * calculate process statistics
	 */

	proclist_lock_read();
	LIST_FOREACH(p, &allproc, p_list) {
		if (p->p_flag & P_SYSTEM)
			continue;
		switch (p->p_stat) {
		case 0:
			continue;

		case SSLEEP:
		case SSTOP:
			if (p->p_flag & P_INMEM) {
				if (p->p_priority <= PZERO)
					totalp->t_dw++;
				else if (p->p_slptime < maxslp)
					totalp->t_sl++;
			} else if (p->p_slptime < maxslp)
				totalp->t_sw++;
			if (p->p_slptime >= maxslp)
				continue;
			break;

		case SRUN:
		case SONPROC:
		case SIDL:
			if (p->p_flag & P_INMEM)
				totalp->t_rq++;
			else
				totalp->t_sw++;
			if (p->p_stat == SIDL)
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
