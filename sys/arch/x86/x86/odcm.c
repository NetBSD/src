/*	$NetBSD: odcm.c,v 1.2.2.2 2017/12/03 11:36:50 jdolecek Exp $ */
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
 * by software. Available on Pentium M and later models (feature TM).
 *
 * References:
 * Intel Developer's manual v.3 #245472-012
 *
 * On some models, the cpu can hang if it's running at a slow speed.
 * Workarounds included below.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: odcm.c,v 1.2.2.2 2017/12/03 11:36:50 jdolecek Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/module.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>
#include <sys/xcall.h>

#include <x86/cpuvar.h>
#include <x86/cpu_msr.h>
#include <x86/specialreg.h>

#define ODCM_ENABLE		(1 << 4) /* Enable bit 4 */
#define ODCM_REGOFFSET		1
#define ODCM_MAXSTATES		8

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

struct odcm_softc {
	device_t		 sc_dev;
	struct sysctllog	*sc_log;
	char			*sc_names;
	size_t			 sc_names_len;
	int			 sc_level;
	int			 sc_target;
	int			 sc_current;
};

static int	odcm_match(device_t, cfdata_t, void *);
static void	odcm_attach(device_t, device_t, void *);
static int	odcm_detach(device_t, int);
static void	odcm_quirks(void);
static bool	odcm_init(device_t);
static bool	odcm_sysctl(device_t);
static int	odcm_state_get(void);
static void	odcm_state_set(int);
static int	odcm_sysctl_helper(SYSCTLFN_ARGS);

CFATTACH_DECL_NEW(odcm, sizeof(struct odcm_softc),
    odcm_match, odcm_attach, odcm_detach, NULL);

static int
odcm_match(device_t parent, cfdata_t cf, void *aux)
{
	const uint32_t odcm = CPUID_ACPI | CPUID_TM;
	struct cpufeature_attach_args *cfaa = aux;
	uint32_t regs[4];

	if (strcmp(cfaa->name, "frequency") != 0)
		return 0;

	if (cpuid_level < 1)
		return 0;
	else {
		x86_cpuid(1, regs);

		return ((regs[3] & odcm) != odcm) ? 0 : 1;
	}
}

static void
odcm_attach(device_t parent, device_t self, void *aux)
{
	struct odcm_softc *sc = device_private(self);

	sc->sc_dev = self;
	sc->sc_log = NULL;
	sc->sc_names = NULL;

	odcm_quirks();

	if (odcm_init(self) != true) {
		aprint_normal(": failed to initialize\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": on-demand clock modulation (state %s)\n",
	    sc->sc_level == (ODCM_MAXSTATES - 1) ? "disabled" : "enabled");

	(void)pmf_device_register(self, NULL, NULL);
}

static int
odcm_detach(device_t self, int flags)
{
	struct odcm_softc *sc = device_private(self);

	/*
	 * Make sure the CPU operates with
	 * 100 % duty cycle after we are done.
	 */
	odcm_state_set(7);

	if (sc->sc_log != NULL)
		sysctl_teardown(&sc->sc_log);

	if (sc->sc_names != NULL)
		kmem_free(sc->sc_names, sc->sc_names_len);

	pmf_device_deregister(self);

	return 0;
}

static void
odcm_quirks(void)
{
	uint32_t regs[4];

	x86_cpuid(1, regs);

	switch (CPUID_TO_STEPPING(regs[0])) {

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
	}
}

static bool
odcm_init(device_t self)
{
	struct odcm_softc *sc = device_private(self);
	size_t i, len;

	sc->sc_names_len = state[0].level  * (sizeof("9999 ") - 1) + 1;
	sc->sc_names = kmem_zalloc(sc->sc_names_len, KM_SLEEP);

	for (i = len = 0; i < __arraycount(state); i++) {

		if (state[i].errata)
			continue;

		len += snprintf(sc->sc_names + len,
		    sc->sc_names_len - len, "%d%s", state[i].level,
		    i < __arraycount(state) ? " " : "");
		if (len > sc->sc_names_len)
			break;
	}

	/*
	 * Get the current value and create
	 * sysctl machdep.clockmod subtree.
	 */
	sc->sc_level = odcm_state_get();

	return odcm_sysctl(self);
}

static bool
odcm_sysctl(device_t self)
{
	struct odcm_softc *sc = device_private(self);
	const struct sysctlnode *node, *odcmnode;
	int rv;

	rv = sysctl_createv(&sc->sc_log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &node, &odcmnode,
	    0, CTLTYPE_NODE, "clockmod", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	rv = sysctl_createv(&sc->sc_log, 0, &odcmnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target",
	    SYSCTL_DESCR("target duty cycle (0 = lowest, 7 highest)"),
	    odcm_sysctl_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	sc->sc_target = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &odcmnode, &node,
	    0, CTLTYPE_INT, "current",
	    SYSCTL_DESCR("current duty cycle"),
	    odcm_sysctl_helper, 0, (void *)sc, 0, CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	sc->sc_current = node->sysctl_num;

	rv = sysctl_createv(&sc->sc_log, 0, &odcmnode, &node,
	    0, CTLTYPE_STRING, "available",
	    SYSCTL_DESCR("list of duty cycles available"),
	    NULL, 0, sc->sc_names, sc->sc_names_len,
	    CTL_CREATE, CTL_EOL);

	if (rv != 0)
		goto fail;

	return true;

fail:
	sysctl_teardown(&sc->sc_log);
	sc->sc_log = NULL;

	return false;
}

static int
odcm_sysctl_helper(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	struct odcm_softc *sc;
	int level, old, err;
	size_t i;

	node = *rnode;
	sc = node.sysctl_data;

	level = old = 0;
	node.sysctl_data = &level;

	if (rnode->sysctl_num == sc->sc_target)
		level = old = sc->sc_level;
	else if (rnode->sysctl_num == sc->sc_current)
		level = odcm_state_get();
	else
		return EOPNOTSUPP;

	err = sysctl_lookup(SYSCTLFN_CALL(&node));

	if (err || newp == NULL)
		return err;

	/*
	 * Check for an invalid level.
	 */
	for (i = 0; i < __arraycount(state); i++) {

		if (level == state[i].level && !state[i].errata)
			break;
	}

	if (i == __arraycount(state))
		return EINVAL;

	if (rnode->sysctl_num == sc->sc_target && level != old) {
		odcm_state_set(level);
		sc->sc_level = level;
	}

	return 0;
}

static int
odcm_state_get(void)
{
	uint64_t msr;
	size_t i;
	int val;

	msr = rdmsr(MSR_THERM_CONTROL);

	if ((msr & ODCM_ENABLE) == 0)
		return (ODCM_MAXSTATES - 1);

	msr = (msr >> ODCM_REGOFFSET) & (ODCM_MAXSTATES - 1);

	for (val = -1, i = 0; i < __arraycount(state); i++) {

		KASSERT(msr < INT_MAX);

		if ((int)msr == state[i].reg) {
			val = state[i].level;
			break;
		}
	}

	KASSERT(val != -1);

	return val;
}

static void
odcm_state_set(int level)
{
	struct msr_rw_info msr;
	uint64_t xc;
	size_t i;

	for (i = 0; i < __arraycount(state); i++) {

		if (level == state[i].level && !state[i].errata)
			break;
	}

	KASSERT(i != __arraycount(state));

	msr.msr_read = true;
	msr.msr_type = MSR_THERM_CONTROL;
	msr.msr_mask = 0x1e;

	if (state[i].reg != 0)	/* bit 0 reserved */
		msr.msr_value = (state[i].reg << ODCM_REGOFFSET) | ODCM_ENABLE;
	else
		msr.msr_value = 0; /* max state */

	xc = xc_broadcast(0, (xcfunc_t)x86_msr_xcall, &msr, NULL);
	xc_wait(xc);
}

MODULE(MODULE_CLASS_DRIVER, odcm, NULL);

#ifdef _MODULE
#include "ioconf.c"
#endif

static int
odcm_modcmd(modcmd_t cmd, void *aux)
{
	int error = 0;

	switch (cmd) {
	case MODULE_CMD_INIT:
#ifdef _MODULE
		error = config_init_component(cfdriver_ioconf_odcm,
		    cfattach_ioconf_odcm, cfdata_ioconf_odcm);
#endif
		return error;
	case MODULE_CMD_FINI:
#ifdef _MODULE
		error = config_fini_component(cfdriver_ioconf_odcm,
		    cfattach_ioconf_odcm, cfdata_ioconf_odcm);
#endif
		return error;
	default:
		return ENOTTY;
	}
}
