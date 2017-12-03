/* $NetBSD: tegra_cpufreq.c,v 1.5.10.2 2017/12/03 11:35:54 jdolecek Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_cpufreq.c,v 1.5.10.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/atomic.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <arm/nvidia/tegra_var.h>

static u_int cpufreq_busy;
static struct sysctllog *cpufreq_log;
static int cpufreq_node_target, cpufreq_node_current, cpufreq_node_available;

static const struct tegra_cpufreq_func *cpufreq_func = NULL;

static int	tegra_cpufreq_freq_helper(SYSCTLFN_PROTO);
static char 	tegra_cpufreq_available[TEGRA_CPUFREQ_MAX * 5];

#define cpufreq_set_rate	cpufreq_func->set_rate
#define cpufreq_get_rate	cpufreq_func->get_rate
#define cpufreq_get_available	cpufreq_func->get_available

void
tegra_cpufreq_register(const struct tegra_cpufreq_func *cf)
{
	const struct sysctlnode *node, *cpunode, *freqnode;
	u_int availfreq[TEGRA_CPUFREQ_MAX];
	size_t nfreq;
	int error;

	KASSERT(cpufreq_func == NULL);
	cpufreq_func = cf;

	nfreq = cpufreq_get_available(availfreq, TEGRA_CPUFREQ_MAX);
	if (nfreq == 0)
		return;

	KASSERT(nfreq <= TEGRA_CPUFREQ_MAX);

	for (int i = 0; i < nfreq; i++) {
		char buf[6];
		snprintf(buf, sizeof(buf), i ? " %u" : "%u", availfreq[i]);
		strcat(tegra_cpufreq_available, buf);
	}

	error = sysctl_createv(&cpufreq_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&cpufreq_log, 0, &node, &cpunode,
	    0, CTLTYPE_NODE, "cpu", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	error = sysctl_createv(&cpufreq_log, 0, &cpunode, &freqnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    tegra_cpufreq_freq_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_target = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "current", NULL,
	    tegra_cpufreq_freq_helper, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_current = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, tegra_cpufreq_available, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	cpufreq_node_available = node->sysctl_num;

	device_printf(curcpu()->ci_dev,
	    "setting speed to %d MHz\n", availfreq[0]);
	cpufreq_set_rate(availfreq[0]);

	return;

sysctl_failed:
	aprint_error("cpufreq: couldn't create sysctl nodes (%d)\n", error);
	sysctl_teardown(&cpufreq_log);
}

static int
tegra_cpufreq_freq_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int fq, oldfq = 0, error;

	node = *rnode;
	node.sysctl_data = &fq;

	fq = cpufreq_get_rate();
	if (rnode->sysctl_num == cpufreq_node_target)
		oldfq = fq;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (fq == oldfq || rnode->sysctl_num != cpufreq_node_target)
		return 0;

	if (atomic_cas_uint(&cpufreq_busy, 0, 1) != 0)
		return EBUSY;

	error = cpufreq_set_rate(fq);
	if (error == 0) {
		pmf_event_inject(NULL, PMFE_SPEED_CHANGED);
	}

	atomic_dec_uint(&cpufreq_busy);

	return error;
}
