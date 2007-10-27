/*	$NetBSD: kern_cpu.c,v 1.6.4.3 2007/10/27 11:35:21 yamt Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c)2007 YAMAMOTO Takashi,
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: kern_cpu.c,v 1.6.4.3 2007/10/27 11:35:21 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/idle.h>
#include <sys/sched.h>
#include <sys/intr.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/cpuio.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/kauth.h>
#include <sys/xcall.h>
#include <sys/pool.h>

#include <uvm/uvm_extern.h>

void	cpuctlattach(int);

static void	cpu_xc_online(struct schedstate_percpu *);
static void	cpu_xc_offline(struct schedstate_percpu *);

dev_type_ioctl(cpuctl_ioctl);

const struct cdevsw cpuctl_cdevsw = {
	nullopen, nullclose, nullread, nullwrite, cpuctl_ioctl,
	nullstop, notty, nopoll, nommap, nokqfilter,
	D_OTHER | D_MPSAFE
};
  
kmutex_t cpu_lock;
int	ncpu;
int	ncpuonline;

int
mi_cpu_attach(struct cpu_info *ci)
{
	struct schedstate_percpu *spc = &ci->ci_schedstate;
	int error;

	ci->ci_index = ncpu;

	mutex_init(&spc->spc_lwplock, MUTEX_SPIN, IPL_SCHED);
	sched_cpuattach(ci);
	uvm_cpu_attach(ci);

	error = create_idle_lwp(ci);
	if (error != 0) {
		/* XXX revert sched_cpuattach */
		return error;
	}

	softint_init(ci);
	xc_init_cpu(ci);
	TAILQ_INIT(&ci->ci_data.cpu_biodone);
	ncpu++;
	ncpuonline++;

	return 0;
}

void
cpuctlattach(int dummy)
{

}

int
cpuctl_ioctl(dev_t dev, u_long cmd, void *data, int flag, lwp_t *l)
{
	CPU_INFO_ITERATOR cii;
	cpustate_t *cs;
	struct cpu_info *ci;
	int error, i;
	u_int id;

	error = 0;

	mutex_enter(&cpu_lock);
	switch (cmd) {
	case IOC_CPU_SETSTATE:
		error = kauth_authorize_generic(l->l_cred,
		    KAUTH_GENERIC_ISSUSER, NULL);
		if (error != 0)
			break;
		cs = data;
		if ((ci = cpu_lookup(cs->cs_id)) == NULL) {
			error = ESRCH;
			break;
		}
		if (!cs->cs_intr) {
			error = EOPNOTSUPP;
			break;
		}
		error = cpu_setonline(ci, cs->cs_online);
		break;

	case IOC_CPU_GETSTATE:
		cs = data;
		id = cs->cs_id;
		memset(cs, 0, sizeof(*cs));
		cs->cs_id = id;
		if ((ci = cpu_lookup(id)) == NULL) {
			error = ESRCH;
			break;
		}
		if ((ci->ci_schedstate.spc_flags & SPCF_OFFLINE) != 0)
			cs->cs_online = false;
		else
			cs->cs_online = true;
		cs->cs_intr = true;
		cs->cs_lastmod = ci->ci_schedstate.spc_lastmod;
		break;

	case IOC_CPU_MAPID:
		i = 0;
		for (CPU_INFO_FOREACH(cii, ci)) {
			if (i++ == *(int *)data)
				break;
		}
		if (ci == NULL)
			error = ESRCH;
		else
			*(int *)data = ci->ci_cpuid;
		break;

	case IOC_CPU_GETCOUNT:
		*(int *)data = ncpu;
		break;

	default:
		error = ENOTTY;
		break;
	}
	mutex_exit(&cpu_lock);

	return error;
}

struct cpu_info *
cpu_lookup(cpuid_t id)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	for (CPU_INFO_FOREACH(cii, ci)) {
		if (ci->ci_cpuid == id)
			return ci;
	}

	return NULL;
}

static void
cpu_xc_offline(struct schedstate_percpu *spc)
{
	int s;

	s = splsched();
	spc->spc_flags |= SPCF_OFFLINE;
	splx(s);
}

static void
cpu_xc_online(struct schedstate_percpu *spc)
{
	int s;

	s = splsched();
	spc->spc_flags &= ~SPCF_OFFLINE;
	splx(s);
}

int
cpu_setonline(struct cpu_info *ci, bool online)
{
	struct schedstate_percpu *spc;
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci2;
	uint64_t where;
	xcfunc_t func;
	int nonline;

	spc = &ci->ci_schedstate;

	KASSERT(mutex_owned(&cpu_lock));

	if (online) {
		if ((spc->spc_flags & SPCF_OFFLINE) == 0)
			return 0;
		func = (xcfunc_t)cpu_xc_online;
		ncpuonline++;
	} else {
		if ((spc->spc_flags & SPCF_OFFLINE) != 0)
			return 0;
		nonline = 0;
		for (CPU_INFO_FOREACH(cii, ci2)) {
			nonline += ((ci2->ci_schedstate.spc_flags &
			    SPCF_OFFLINE) == 0);
		}
		if (nonline == 1)
			return EBUSY;
		func = (xcfunc_t)cpu_xc_offline;
		ncpuonline--;
	}

	where = xc_unicast(0, func, &ci->ci_schedstate, NULL, ci);
	xc_wait(where);
	spc->spc_lastmod = time_second;

	return 0;
}
