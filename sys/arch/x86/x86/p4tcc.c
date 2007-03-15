/*	$NetBSD: p4tcc.c,v 1.1 2007/03/15 00:03:24 xtraeme Exp $ */
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
 * Restrict power consumption by using thermal control circuit.
 * This operates independently of speedstep.
 * Found on Pentium 4 and later models (feature TM).
 *
 * References:
 * Intel Developer's manual v.3 #245472-012
 *
 * On some models, the cpu can hang if it's running at a slow speed.
 * Workarounds included below.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: p4tcc.c,v 1.1 2007/03/15 00:03:24 xtraeme Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>

#include <machine/cpu.h>
#include <machine/cpuvar.h>
#include <machine/specialreg.h>

#define TCC_ENABLE_ONDEMAND	(1 << 4)

static int p4tcc_level;
static int p4tcc_throttling_target;
static int p4tcc_throttling_current;

static struct {
	int level;
	int reg;
	int errata;
} tcc[] = {
	{ .level = 7, .reg = 0, .errata = 0 },
	{ .level = 6, .reg = 7, .errata = 0 },
	{ .level = 5, .reg = 6, .errata = 0 },
	{ .level = 4, .reg = 5, .errata = 0 },
	{ .level = 3, .reg = 4, .errata = 0 },
	{ .level = 2, .reg = 3, .errata = 0 },
	{ .level = 1, .reg = 2, .errata = 0 },
	{ .level = 0, .reg = 1, .errata = 0 }
};

static int	p4tcc_getperf(void);
static void	p4tcc_setperf(int);
static int	p4tcc_sysctl_helper(SYSCTLFN_PROTO);

static int
p4tcc_getperf(void)
{
	uint64_t msr;
	int i, val = 0;

	msr = rdmsr(MSR_THERM_CONTROL);
	msr = (msr >> 1) & 0x07;
	for (i = 0; i < __arraycount(tcc); i++) {
		if (msr == tcc[i].reg) {
			val = tcc[i].level;
			break;
		}
	}
	return val;
}

static void
p4tcc_setperf(int level)
{
	uint64_t msr;
	int i;

	for (i = 0; i < __arraycount(tcc); i++) {
		if (level == tcc[i].level && !tcc[i].errata)
			break;
	}

	msr = rdmsr(MSR_THERM_CONTROL);
	msr &= ~0x1e;	/* bit 0 reserved */
	if (tcc[i].reg != 0)
		msr |= tcc[i].reg << 1 | TCC_ENABLE_ONDEMAND;

	wrmsr(MSR_THERM_CONTROL, msr);
}

void
p4tcc_init(struct cpu_info *ci)
{
	const struct sysctlnode *node, *tccnode, *freqnode;
	size_t len, freq_len;
	char *freq_names;
	int i;

	if ((cpu_feature & (CPUID_ACPI|CPUID_TM)) != (CPUID_ACPI|CPUID_TM))
		return;

	switch (cpu_id & 0xf) {
	case 0x22:	/* errata O50 P44 and Z21 */
	case 0x24:
	case 0x25:
	case 0x27:
	case 0x29:
		/* hang with 12.5 */
		tcc[__arraycount(tcc) - 1].errata = 1;
		break;
	case 0x07:	/* errata N44 and P18 */
	case 0x0a:
	case 0x12:
	case 0x13:
		/* hang at 12.5 and 25 */
		tcc[__arraycount(tcc) - 1].errata = 1;
		tcc[__arraycount(tcc) - 2].errata = 1;
		break;
	default:
		break;
	}

	freq_len = tcc[0].level  * (sizeof("9999 ")-1) + 1;
	freq_names = malloc(freq_len, M_SYSCTLDATA, M_WAITOK);
	freq_names[0] = '\0';
	len = 0;
	for (i = 0; i < __arraycount(tcc); i++) {
		if (tcc[i].errata)
			continue;
		len += snprintf(freq_names + len, freq_len - len, "%d%s",
		    tcc[i].level, i < __arraycount(tcc) ? " " : "");
	}

	/* Set the highest level at boot */
	p4tcc_setperf(tcc[0].level);
	p4tcc_level = p4tcc_getperf();

	aprint_verbose("%s: Pentium 4 TCC enabled (state [%d])\n",
	    ci->ci_dev->dv_xname, p4tcc_level);

	/* Create sysctl machdep.p4tcc.throttling subtree */
	sysctl_createv(NULL, 0, NULL, &node,
	    CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0,
	    CTL_MACHDEP, CTL_EOL);

	sysctl_createv(NULL, 0, &node, &tccnode,
	    0,
	    CTLTYPE_NODE, "p4tcc",
	    SYSCTL_DESCR("Pentium 4 Thermal Control Circuitry subtree"),
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, &tccnode, &freqnode,
	    0,
	    CTLTYPE_NODE, "throttling",
	    NULL,
	    NULL, 0, NULL, 0,
	    CTL_CREATE, CTL_EOL);

	sysctl_createv(NULL, 0, &freqnode, &node,
	    CTLFLAG_READWRITE,
	    CTLTYPE_INT, "target",
	    SYSCTL_DESCR("CPU Clock throttling state (0 = lowest, 7 highest)"),
	    p4tcc_sysctl_helper, 0, &p4tcc_level, 0,
	    CTL_CREATE, CTL_EOL);

	p4tcc_throttling_target = node->sysctl_num;

	sysctl_createv(NULL, 0, &freqnode, &node,
	    0,
	    CTLTYPE_INT, "current",
	    SYSCTL_DESCR("current CPU throttling state"),
	    p4tcc_sysctl_helper, 0, &p4tcc_level, 0,
	    CTL_CREATE, CTL_EOL);

	p4tcc_throttling_current = node->sysctl_num;

	sysctl_createv(NULL, 0, &freqnode, &node,
	    0,
	    CTLTYPE_STRING, "available",
	    SYSCTL_DESCR("list of CPU Clock throttling states"),
	    NULL, 0, freq_names, freq_len,
	    CTL_CREATE, CTL_EOL);
}

static int
p4tcc_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	int lvl, oldlvl, error;

	node = *rnode;
	node.sysctl_data = &lvl;

	oldlvl = 0;
	if (rnode->sysctl_num == p4tcc_throttling_target)
		lvl = oldlvl = p4tcc_level;
	else if (rnode->sysctl_num == p4tcc_throttling_current)
		lvl = p4tcc_level;
	else
		return EOPNOTSUPP;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	/* invalid level? */
	if (lvl < 0 || lvl > 7)
		return EINVAL;

	if (rnode->sysctl_num == p4tcc_throttling_target && lvl != oldlvl) {
		/* Ok, switch to new level */
		p4tcc_setperf(lvl);
		p4tcc_level = lvl;
	}
	
	return 0;
}
