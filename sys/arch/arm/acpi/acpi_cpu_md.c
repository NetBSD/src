/* $NetBSD: acpi_cpu_md.c,v 1.1 2020/12/07 10:57:41 jmcneill Exp $ */

/*-
 * Copyright (c) 2020 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_cpu_md.c,v 1.1 2020/12/07 10:57:41 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/cpufreq.h>
#include <sys/device.h>
#include <sys/sysctl.h>

#include <dev/acpi/acpica.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_cpu.h>
#include <dev/acpi/acpi_util.h>

static struct sysctllog *acpicpu_log = NULL;

/*
 * acpicpu_md_match --
 *
 * 	Match against an ACPI processor device node (either a device
 * 	with HID "ACPI0007" or a processor node) and return a pointer
 * 	to the corresponding CPU device's 'cpu_info' struct.
 *
 */
struct cpu_info *
acpicpu_md_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args * const aa = aux;

	return acpi_match_cpu_handle(aa->aa_node->ad_handle);
}

/*
 * acpicpu_md_attach --
 *
 * 	Return a pointer to the CPU device's 'cpu_info' struct
 * 	corresponding with this device.
 *
 */
struct cpu_info *
acpicpu_md_attach(device_t parent, device_t self, void *aux)
{
	struct acpi_attach_args * const aa = aux;

	return acpi_match_cpu_handle(aa->aa_node->ad_handle);
}

/*
 * acpicpu_md_flags --
 *
 * 	Return a bitmask of ACPICPU_FLAG_* platform specific quirks.
 *
 */
uint32_t
acpicpu_md_flags(void)
{
	return 0;
}

/*
 * acpicpu_md_cstate_start --
 *
 * 	Not implemented.
 *
 */
int
acpicpu_md_cstate_start(struct acpicpu_softc *sc)
{
	return EINVAL;
}

/*
 * acpicpu_md_cstate_stop --
 *
 * 	Not implemented.
 *
 */
int
acpicpu_md_cstate_stop(void)
{
	return EALREADY;
}

/*
 * acpicpu_md_cstate_enter --
 *
 * 	Not implemented.
 *
 */
void
acpicpu_md_cstate_enter(int method, int state)
{
}

/*
 * acpicpu_md_pstate_init --
 *
 * 	MD initialization for P-state support. Nothing to do here.
 *
 */
int
acpicpu_md_pstate_init(struct acpicpu_softc *sc)
{
	return 0;
}

/*
 * acpicpu_md_pstate_sysctl_current --
 *
 * 	sysctl(9) callback for retrieving the current CPU frequency.
 *
 */
static int
acpicpu_md_pstate_sysctl_current(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t freq;
	int error;

	freq = cpufreq_get(curcpu());
	if (freq == 0)
		return ENXIO;

	node = *rnode;
	node.sysctl_data = &freq;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

/*
 * acpicpu_md_pstate_sysctl_target --
 *
 * 	sysctl(9) callback for setting the target CPU frequency.
 *
 */
static int
acpicpu_md_pstate_sysctl_target(SYSCTLFN_ARGS)
{
	struct sysctlnode node;
	uint32_t freq;
	int error;

	freq = cpufreq_get(curcpu());
	if (freq == 0)
		return ENXIO;

	node = *rnode;
	node.sysctl_data = &freq;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	cpufreq_set_all(freq);

	return 0;
}

/*
 * acpicpu_md_pstate_sysctl_available --
 *
 * 	sysctl(9) callback for returning a list of supported CPU frequencies.
 *
 */
static int
acpicpu_md_pstate_sysctl_available(SYSCTLFN_ARGS)
{
	struct acpicpu_softc * const sc = rnode->sysctl_data;
	struct sysctlnode node;
	char buf[1024];
	size_t len;
	uint32_t i;
	int error;

	memset(buf, 0, sizeof(buf));

	mutex_enter(&sc->sc_mtx);
	for (len = 0, i = sc->sc_pstate_max; i < sc->sc_pstate_count; i++) {
		if (sc->sc_pstate[i].ps_freq == 0)
			continue;
		if (len >= sizeof(buf))
			break;
		len += snprintf(buf + len, sizeof(buf) - len, "%u%s",
		    sc->sc_pstate[i].ps_freq,
		    i < (sc->sc_pstate_count - 1) ? " " : "");
	}
	mutex_exit(&sc->sc_mtx);

	node = *rnode;
	node.sysctl_data = buf;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	return 0;
}

/*
 * acpicpu_md_pstate_start --
 *
 * 	MD startup for P-state support. Create sysctls here.
 *
 */
int
acpicpu_md_pstate_start(struct acpicpu_softc *sc)
{
	const struct sysctlnode *mnode, *cnode, *fnode, *node;
	int error;

	error = sysctl_createv(&acpicpu_log, 0, NULL, &mnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error) {
		goto teardown;
	}

	error = sysctl_createv(&acpicpu_log, 0, &mnode, &cnode,
	    0, CTLTYPE_NODE, "cpu", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error) {
		goto teardown;
	}

	error = sysctl_createv(&acpicpu_log, 0, &cnode, &fnode,
	    0, CTLTYPE_NODE, "frequency", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error) {
		goto teardown;
	}

	error = sysctl_createv(&acpicpu_log, 0, &fnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    acpicpu_md_pstate_sysctl_target, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error) {
		goto teardown;
	}

	error = sysctl_createv(&acpicpu_log, 0, &fnode, &node,
	    CTLFLAG_READONLY, CTLTYPE_INT, "current", NULL,
	    acpicpu_md_pstate_sysctl_current, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error) {
		goto teardown;
	}

	error = sysctl_createv(&acpicpu_log, 0, &fnode, &node,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "available", NULL,
	    acpicpu_md_pstate_sysctl_available, 0, (void *)sc, 0, CTL_CREATE,
	    CTL_EOL);
	if (error) {
		goto teardown;
	}

	return 0;

teardown:
	if (acpicpu_log != NULL) {
		sysctl_teardown(&acpicpu_log);
		acpicpu_log = NULL;
	}

	return error;
}

/*
 * acpicpu_md_pstate_stop --
 *
 * 	MD shutdown for P-state support. Destroy sysctls here.
 *
 */
int
acpicpu_md_pstate_stop(void)
{
	if (acpicpu_log != NULL) {
		sysctl_teardown(&acpicpu_log);
		acpicpu_log = NULL;
	}

	return 0;
}

/*
 * acpicpu_md_pstate_get --
 *
 * 	Fixed hardware access method for getting current processor P-state.
 * 	Not implemented.
 *
 */
int
acpicpu_md_pstate_get(struct acpicpu_softc *sc, uint32_t *freq)
{
	return EINVAL;
}

/*
 * acpicpu_md_pstate_set --
 *
 * 	Fixed hardware access method for setting current processor P-state.
 * 	Not implemented.
 *
 */
int
acpicpu_md_pstate_set(struct acpicpu_pstate *ps)
{
	return EINVAL;
}

/*
 * acpicpu_md_tstate_get --
 *
 * 	Fixed hardware access method for getting current processor T-state.
 * 	Not implemented.
 *
 */
int
acpicpu_md_tstate_get(struct acpicpu_softc *sc, uint32_t *percent)
{
	return EINVAL;
}

/*
 * acpicpu_md_tstate_set --
 *
 * 	Fixed hardware access method for setting current processor T-state.
 * 	Not implemented.
 *
 */
int
acpicpu_md_tstate_set(struct acpicpu_tstate *ts)
{
	return EINVAL;
}
