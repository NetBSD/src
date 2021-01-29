/* $NetBSD: acpi_cppc.c,v 1.2 2021/01/29 15:49:55 thorpej Exp $ */

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

/*
 * ACPI Collaborative Processor Performance Control support.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: acpi_cppc.c,v 1.2 2021/01/29 15:49:55 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/sysctl.h>

#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_pcc.h>

#include <external/bsd/acpica/dist/include/amlresrc.h>

/* _CPC package elements */
typedef enum CPCPackageElement {
	CPCNumEntries,
	CPCRevision,
	CPCHighestPerformance,
	CPCNominalPerformance,
	CPCLowestNonlinearPerformance,
	CPCLowestPerformance,
	CPCGuaranteedPerformanceReg,
	CPCDesiredPerformanceReg,
	CPCMinimumPerformanceReg,
	CPCMaximumPerformanceReg,
	CPCPerformanceReductionToleranceReg,
	CPCTimeWindowReg,
	CPCCounterWraparoundTime,
	CPCReferencePerformanceCounterReg,
	CPCDeliveredPerformanceCounterReg,
	CPCPerformanceLimitedReg,
	CPCCPPCEnableReg,
	CPCAutonomousSelectionEnable,
	CPCAutonomousActivityWindowReg,
	CPCEnergyPerformancePreferenceReg,
	CPCReferencePerformance,
	CPCLowestFrequency,
	CPCNominalFrequency,
} CPCPackageElement;

/* PCC command numbers */
#define	CPPC_PCC_READ	0x00
#define	CPPC_PCC_WRITE	0x01

struct cppc_softc {
	device_t		sc_dev;
	struct cpu_info	*	sc_cpuinfo;
	ACPI_HANDLE		sc_handle;
	ACPI_OBJECT *		sc_cpc;
	u_int			sc_ncpc;

	char *			sc_available;
	int			sc_node_target;
	int			sc_node_current;
	ACPI_INTEGER		sc_max_target;
	ACPI_INTEGER		sc_min_target;
};

static int		cppc_match(device_t, cfdata_t, void *);
static void		cppc_attach(device_t, device_t, void *);

static ACPI_STATUS 	cppc_parse_cpc(struct cppc_softc *);
static ACPI_STATUS 	cppc_cpufreq_init(struct cppc_softc *);
static ACPI_STATUS	cppc_read(struct cppc_softc *, CPCPackageElement,
				  ACPI_INTEGER *);
static ACPI_STATUS	cppc_write(struct cppc_softc *, CPCPackageElement,
				   ACPI_INTEGER);

CFATTACH_DECL_NEW(acpicppc, sizeof(struct cppc_softc),
    cppc_match, cppc_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ACPI0007" },	/* ACPI Processor Device */
	DEVICE_COMPAT_EOL
};

static int
cppc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct acpi_attach_args * const aa = aux;
	ACPI_HANDLE handle;
	ACPI_STATUS rv;

	if (acpi_compatible_match(aa, compat_data) == 0)
		return 0;

	rv = AcpiGetHandle(aa->aa_node->ad_handle, "_CPC", &handle);
	if (ACPI_FAILURE(rv)) {
		return 0;
	}

	if (acpi_match_cpu_handle(aa->aa_node->ad_handle) == NULL) {
		return 0;
	}

	/* When CPPC and P-states/T-states are both available, prefer CPPC */
	return ACPI_MATCHSCORE_CID_MAX + 1;
}

static void
cppc_attach(device_t parent, device_t self, void *aux)
{
	struct cppc_softc * const sc = device_private(self);
	struct acpi_attach_args * const aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	struct cpu_info *ci;
	ACPI_STATUS rv;

	ci = acpi_match_cpu_handle(handle);
	KASSERT(ci != NULL);

	aprint_naive("\n");
	aprint_normal(": Processor Performance Control (%s)\n", cpu_name(ci));

	sc->sc_dev = self;
	sc->sc_cpuinfo = ci;
	sc->sc_handle = handle;

	rv = cppc_parse_cpc(sc);
	if (ACPI_FAILURE(rv)) {
		aprint_error_dev(self, "failed to parse CPC package: %s\n",
		    AcpiFormatException(rv));
		return;
	}

	cppc_cpufreq_init(sc);
}

/*
 * cppc_parse_cpc --
 *
 *	Read and verify the contents of the _CPC package.
 */
static ACPI_STATUS
cppc_parse_cpc(struct cppc_softc *sc)
{
	ACPI_BUFFER buf;
	ACPI_STATUS rv;

	buf.Pointer = NULL;
	buf.Length = ACPI_ALLOCATE_BUFFER;
	rv = AcpiEvaluateObjectTyped(sc->sc_handle, "_CPC", NULL, &buf,
	    ACPI_TYPE_PACKAGE);
	if (ACPI_FAILURE(rv)) {
		return rv;
	}

	sc->sc_cpc = (ACPI_OBJECT *)buf.Pointer;
	if (sc->sc_cpc->Package.Count == 0) {
		return AE_NOT_EXIST;
	}
	if (sc->sc_cpc->Package.Elements[CPCNumEntries].Type !=
	    ACPI_TYPE_INTEGER) {
		return AE_TYPE;
	}
	sc->sc_ncpc =
	    sc->sc_cpc->Package.Elements[CPCNumEntries].Integer.Value;

	return AE_OK;
}

/*
 * cppc_cpufreq_sysctl --
 *
 *	sysctl helper function for machdep.cpu.cpuN.{target,current}
 *	nodes.
 */
static int
cppc_cpufreq_sysctl(SYSCTLFN_ARGS)
{
	struct cppc_softc * const sc = rnode->sysctl_data;
	struct sysctlnode node;
	u_int fq, oldfq = 0;
	ACPI_INTEGER val;
	ACPI_STATUS rv;
	int error;

	node = *rnode;
	node.sysctl_data = &fq;

	if (rnode->sysctl_num == sc->sc_node_target) {
		rv = cppc_read(sc, CPCDesiredPerformanceReg, &val);
	} else {
		/*
		 * XXX We should measure the delivered performance and
		 *     report it here. For now, just report the desired
		 *     performance level.
		 */
		rv = cppc_read(sc, CPCDesiredPerformanceReg, &val);
	}
	if (ACPI_FAILURE(rv)) {
		return EIO;
	}
	if (val > UINT32_MAX) {
		return ERANGE;
	}
	fq = (u_int)val;

	if (rnode->sysctl_num == sc->sc_node_target) {
		oldfq = fq;
	}

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error != 0 || newp == NULL) {
		return error;
	}

	if (fq == oldfq || rnode->sysctl_num != sc->sc_node_target) {
		return 0;
	}

	if (fq < sc->sc_min_target || fq > sc->sc_max_target) {
		return EINVAL;
	}

	rv = cppc_write(sc, CPCDesiredPerformanceReg, fq);
	if (ACPI_FAILURE(rv)) {
		return EIO;
	}

	return 0;
}

/*
 * cppc_cpufreq_init --
 *
 *	Create sysctl machdep.cpu.cpuN.* sysctl tree.
 */
static ACPI_STATUS
cppc_cpufreq_init(struct cppc_softc *sc)
{
	static CPCPackageElement perf_regs[4] = {
		CPCHighestPerformance,
		CPCNominalPerformance,
		CPCLowestNonlinearPerformance,
		CPCLowestPerformance
	};
	ACPI_INTEGER perf[4], last;
	const struct sysctlnode *node, *cpunode;
	struct sysctllog *log = NULL;
	struct cpu_info *ci = sc->sc_cpuinfo;
	ACPI_STATUS rv;
	int error, i, n;

	/*
	 * Read highest, nominal, lowest nonlinear, and lowest performance
	 * levels and advertise this list of performance levels in the
	 * machdep.cpufreq.cpuN.available sysctl.
	 */
	sc->sc_available = kmem_zalloc(
	    strlen("########## ") * __arraycount(perf_regs), KM_SLEEP);
	last = 0;
	for (i = 0, n = 0; i < __arraycount(perf_regs); i++) {
		rv = cppc_read(sc, perf_regs[i], &perf[i]);
		if (ACPI_FAILURE(rv)) {
			return rv;
		}
		if (perf[i] != last) {
			char buf[12];
			snprintf(buf, sizeof(buf), n ? " %u" : "%u",
			    (u_int)perf[i]);
			strcat(sc->sc_available, buf);
			last = perf[i];
			n++;
		}
	}
	sc->sc_max_target = perf[0];
	sc->sc_min_target = perf[3];

	error = sysctl_createv(&log, 0, NULL, &node,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "machdep", NULL,
	    NULL, 0, NULL, 0, CTL_MACHDEP, CTL_EOL);
	if (error != 0) {
		goto sysctl_failed;
	}

	error = sysctl_createv(&log, 0, &node, &node,
	    0, CTLTYPE_NODE, "cpufreq", NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error != 0) {
		goto sysctl_failed;
	}

	error = sysctl_createv(&log, 0, &node, &cpunode,
	    0, CTLTYPE_NODE, cpu_name(ci), NULL,
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error != 0) {
		goto sysctl_failed;
	}

	error = sysctl_createv(&log, 0, &cpunode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "target", NULL,
	    cppc_cpufreq_sysctl, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	if (error != 0) {
		goto sysctl_failed;
	}
	sc->sc_node_target = node->sysctl_num;

	error = sysctl_createv(&log, 0, &cpunode, &node,
	    CTLFLAG_READONLY, CTLTYPE_INT, "current", NULL,
	    cppc_cpufreq_sysctl, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	if (error != 0) {
		goto sysctl_failed;
	}
	sc->sc_node_current = node->sysctl_num;

	error = sysctl_createv(&log, 0, &cpunode, &node,
	    CTLFLAG_READONLY, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, sc->sc_available, 0,
	    CTL_CREATE, CTL_EOL);
	if (error != 0) {
		goto sysctl_failed;
	}

	return AE_OK;

sysctl_failed:
	aprint_error_dev(sc->sc_dev, "couldn't create sysctl nodes: %d\n",
	    error);
	sysctl_teardown(&log);

	return AE_ERROR;
}

/*
 * cppc_read --
 *
 *	Read a value from the CPC package that contains either an integer
 *	or indirect register reference.
 */
static ACPI_STATUS
cppc_read(struct cppc_softc *sc, CPCPackageElement index, ACPI_INTEGER *val)
{
	ACPI_OBJECT *obj;
	ACPI_GENERIC_ADDRESS addr;
	ACPI_STATUS rv;

	if (index >= sc->sc_ncpc) {
		return AE_NOT_EXIST;
	}

	obj = &sc->sc_cpc->Package.Elements[index];
	switch (obj->Type) {
	case ACPI_TYPE_INTEGER:
		*val = obj->Integer.Value;
		return AE_OK;

	case ACPI_TYPE_BUFFER:
		if (obj->Buffer.Length <
		    sizeof(AML_RESOURCE_GENERIC_REGISTER)) {
			return AE_TYPE;
		}
		memcpy(&addr, obj->Buffer.Pointer +
		    sizeof(AML_RESOURCE_LARGE_HEADER), sizeof(addr));
		if (addr.SpaceId == ACPI_ADR_SPACE_PLATFORM_COMM) {
			rv = pcc_message(&addr, CPPC_PCC_READ, PCC_READ, val);
		} else {
			rv = AcpiRead(val, &addr);
		}
		return rv;

	default:
		return AE_SUPPORT;
	}
}

/*
 * cppc_write --
 *
 *	Write a value based on the CPC package to the specified register.
 */
static ACPI_STATUS
cppc_write(struct cppc_softc *sc, CPCPackageElement index, ACPI_INTEGER val)
{
	ACPI_OBJECT *obj;
	ACPI_GENERIC_ADDRESS addr;
	ACPI_STATUS rv;

	if (index >= sc->sc_ncpc) {
		return AE_NOT_EXIST;
	}

	obj = &sc->sc_cpc->Package.Elements[index];
	if (obj->Type != ACPI_TYPE_BUFFER ||
	    obj->Buffer.Length < sizeof(AML_RESOURCE_GENERIC_REGISTER)) {
		return AE_TYPE;
	}

	memcpy(&addr, obj->Buffer.Pointer +
	    sizeof(AML_RESOURCE_LARGE_HEADER), sizeof(addr));
	if (addr.SpaceId == ACPI_ADR_SPACE_PLATFORM_COMM) {
		rv = pcc_message(&addr, CPPC_PCC_WRITE, PCC_WRITE, &val);
	} else {
		rv = AcpiWrite(val, &addr);
	}

	return rv;
}
