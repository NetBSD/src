/*	$NetBSD: iclockmod.c,v 1.7 2007/04/04 01:50:15 rmind Exp $ */
/*      $OpenBSD: p4tcc.c,v 1.13 2006/12/20 17:50:40 gwk Exp $ */

/*
 * Copyright (c) 2007 Juan Romero Pardines
 * Copyright (c) 2003 Ted Unangst
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

/*
 * On-Demand Clock Modulation driver, to modulate the clock duty cycle
 * by software. Avaible on Pentium M and later models (feature TM).
 *
 * References:
 * Intel Developer's manual v.3 #245472-012
 *
 * On some models, the cpu can hang if it's running at a slow speed.
 * Workarounds included below.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: iclockmod.c,v 1.7 2007/04/04 01:50:15 rmind Exp $");

#include "opt_intel_odcm.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/once.h>

#include <machine/cpu.h>
#include <machine/cpuvar.h>
#include <machine/specialreg.h>

#include <x86/cpu_msr.h>

#define ODCM_ENABLE		(1 << 4) /* Enable bit 4 */
#define ODCM_REGOFFSET		1
#define ODCM_MAXSTATES		8

static struct msr_cpu_broadcast mcb;
static int clockmod_level;
static int clockmod_state_target;
static int clockmod_state_current;

static struct {
	int level;
	int reg;
	int errata;
} state[] = {
	{ .level = 7, .reg = 0, .errata = 0 },
	{ .level = 6, .reg = 7, .errata = 0 },
	{ .level = 5, .reg = 6, .errata = 0 },
	{ .level = 4, .reg = 5, .errata = 0 },
	{ .level = 3, .reg = 4, .errata = 0 },
	{ .level = 2, .reg = 3, .errata = 0 },
	{ .level = 1, .reg = 2, .errata = 0 },
	{ .level = 0, .reg = 1, .errata = 0 }
};

static int	clockmod_getstate(void);
static void	clockmod_setstate(int);
static int	clockmod_sysctl_helper(SYSCTLFN_PROTO);
static void	clockmod_init_main(void);
static int	clockmod_init_once(void);

static int
clockmod_getstate(void)
{
	uint64_t msr;
	int i, val = 0;

	msr = rdmsr(MSR_THERM_CONTROL);
	msr = (msr >> ODCM_REGOFFSET) & (ODCM_MAXSTATES - 1);

	for (i = 0; i < __arraycount(state); i++) {
		if (msr == state[i].reg) {
			val = state[i].level;
			break;
		}
	}
	return val;
}

static void
clockmod_setstate(int level)
{
	int i;

	for (i = 0; i < __arraycount(state); i++) {
		if (level == state[i].level && !state[i].errata)
			break;
	}

	mcb.msr_read = true;
	mcb.msr_type = MSR_THERM_CONTROL;
	mcb.msr_mask = 0x1e;

	if (state[i].reg != 0)	/* bit 0 reserved */
		mcb.msr_value = (state[i].reg << ODCM_REGOFFSET) | ODCM_ENABLE;
	else
		mcb.msr_value = 0; /* max state */

	msr_cpu_broadcast(&mcb);
}

static int
clockmod_init_once(void)
{
	clockmod_init_main();
	return 0;
}

void
clockmod_init(void)
{
	int error;
	static ONCE_DECL(clockmod_initialized);

	error = RUN_ONCE(&clockmod_initialized, clockmod_init_once);
	if (__predict_false(error != 0))
		return;
}

static void
clockmod_init_main(void)
{
	const struct sysctlnode *node, *odcmnode;
	uint32_t regs[4];
	size_t len, freq_len;
	char *freq_names;
	int i;

	CPUID(1, regs[0], regs[1], regs[2], regs[3]);

	if ((regs[3] & (CPUID_ACPI|CPUID_TM)) != (CPUID_ACPI|CPUID_TM))
		return;

	switch (CPUID2STEPPING(regs[0])) {
	case 0x22:	/* errata O50 P44 and Z21 */
	case 0x24:
	case 0x25:
	case 0x27:
	case 0x29:
		/* hang with 12.5 */
		state[__arraycount(state) - 1].errata = 1;
		break;
	case 0x07:	/* errata N44 and P18 */
	case 0x0a:
	case 0x12:
	case 0x13:
		/* hang at 12.5 and 25 */
		state[__arraycount(state) - 1].errata = 1;
		state[__arraycount(state) - 2].errata = 1;
		break;
	default:
		break;
	}

	freq_len = state[0].level  * (sizeof("9999 ")-1) + 1;
	freq_names = malloc(freq_len, M_SYSCTLDATA, M_WAITOK);
	freq_names[0] = '\0';
	len = 0;

	for (i = 0; i < __arraycount(state); i++) {
		/* skip the state if errata matches */
		if (state[i].errata)
			continue;
		len += snprintf(freq_names + len, freq_len - len, "%d%s",
		    state[i].level, i < __arraycount(state) ? " " : "");
	}

	/* Get current value */
	clockmod_level = clockmod_getstate();

	aprint_normal("%s: Intel(R) On Demand Clock Modulation (state %s)\n",
	    curcpu()->ci_dev->dv_xname, clockmod_level == 0 ? "disabled" :
	    "enabled");

	/* Create sysctl machdep.clockmod subtree */
	sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	sysctl_createv(NULL, 0, &node, &odcmnode,
	    0,
	    CTLTYPE_NODE, "clockmod", NULL,
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, &odcmnode, &node,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "target",
	    SYSCTL_DESCR("target duty cycle (0 = lowest, 7 highest)"),
	    clockmod_sysctl_helper, 0, &clockmod_level, 0,
	    CTL_CREATE, CTL_EOL);

	clockmod_state_target = node->sysctl_num;

	sysctl_createv(NULL, 0, &odcmnode, &node,
	    0,
	    CTLTYPE_INT, "current",
	    SYSCTL_DESCR("current duty cycle"),
	    clockmod_sysctl_helper, 0, &clockmod_level, 0,
	    CTL_CREATE, CTL_EOL);

	clockmod_state_current = node->sysctl_num;

	sysctl_createv(NULL, 0, &odcmnode, &node,
	    0,
	    CTLTYPE_STRING, "available",
	    SYSCTL_DESCR("list of duty cycles available"),
	    NULL, 0, freq_names, freq_len,
	    CTL_CREATE, CTL_EOL);
}

static int
clockmod_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int lvl, oldlvl, error;

	node = *rnode;
	node.sysctl_data = &lvl;

	oldlvl = 0;
	if (rnode->sysctl_num == clockmod_state_target)
		lvl = oldlvl = clockmod_level;
	else if (rnode->sysctl_num == clockmod_state_current)
		lvl = clockmod_getstate();
	else
		return EOPNOTSUPP;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* invalid level? */
	if (lvl < 0 || lvl >= __arraycount(state))
		return EINVAL;

	if (rnode->sysctl_num == clockmod_state_target && lvl != oldlvl) {
		/* Ok, switch to new level */
		clockmod_setstate(lvl);
		clockmod_level = lvl;
	}
	
	return 0;
}
