/* $NetBSD: cpufreq_dt.c,v 1.2.2.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2015-2017 Jared McNeill <jmcneill@invisible.ca>
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpufreq_dt.c,v 1.2.2.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/atomic.h>
#include <sys/xcall.h>
#include <sys/sysctl.h>

#include <dev/fdt/fdtvar.h>

struct cpufreq_dt_opp {
	u_int		freq_khz;
	u_int		voltage_uv;
};

struct cpufreq_dt_softc {
	device_t		sc_dev;
	int			sc_phandle;
	struct clk		*sc_clk;
	struct fdtbus_regulator	*sc_supply;

	struct cpufreq_dt_opp	*sc_opp;
	ssize_t			sc_nopp;
	int			sc_latency;

	u_int			sc_freq_target;
	bool			sc_freq_throttle;

	u_int			sc_busy;

	char			*sc_freq_available;
	int			sc_node_target;
	int			sc_node_current;
	int			sc_node_available;
};

static void
cpufreq_dt_change_cb(void *arg1, void *arg2)
{
#if notyet
	struct cpu_info *ci = curcpu();
	ci->ci_data.cpu_cc_freq = cpufreq_get_rate() * 1000000;
#endif
}

static int
cpufreq_dt_set_rate(struct cpufreq_dt_softc *sc, u_int freq_khz)
{
	struct cpufreq_dt_opp *opp = NULL;
	u_int old_rate, new_rate, old_uv, new_uv;
	uint64_t xc;
	int error;
	ssize_t n;

	for (n = 0; n < sc->sc_nopp; n++)
		if (sc->sc_opp[n].freq_khz == freq_khz) {
			opp = &sc->sc_opp[n];
			break;
		}
	if (opp == NULL)
		return EINVAL;

	old_rate = clk_get_rate(sc->sc_clk);
	new_rate = freq_khz * 1000;

	if (old_rate == new_rate)
		return 0;

	error = fdtbus_regulator_get_voltage(sc->sc_supply, &old_uv);
	if (error != 0)
		return error;
	new_uv = opp->voltage_uv;

	if (new_uv > old_uv) {
		error = fdtbus_regulator_set_voltage(sc->sc_supply,
		    new_uv, new_uv);
		if (error != 0)
			return error;
	}

	error = clk_set_rate(sc->sc_clk, new_rate);
	if (error != 0)
		return error;

	if (new_uv < old_uv) {
		error = fdtbus_regulator_set_voltage(sc->sc_supply,
		    new_uv, new_uv);
		if (error != 0)
			return error;
	}

	if (error == 0) {
		xc = xc_broadcast(0, cpufreq_dt_change_cb, sc, NULL);
		xc_wait(xc);

		pmf_event_inject(NULL, PMFE_SPEED_CHANGED);
	}

	return 0;
}

static void
cpufreq_dt_throttle_enable(device_t dev)
{
	struct cpufreq_dt_softc * const sc = device_private(dev);

	if (sc->sc_freq_throttle)
		return;

	const u_int freq_khz = sc->sc_opp[sc->sc_nopp - 1].freq_khz;

	while (atomic_cas_uint(&sc->sc_busy, 0, 1) != 0)
		kpause("throttle", false, 1, NULL);

	if (cpufreq_dt_set_rate(sc, freq_khz) == 0) {
		aprint_debug_dev(sc->sc_dev, "throttle enabled (%u.%03u MHz)\n",
		    freq_khz / 1000, freq_khz % 1000);
		sc->sc_freq_throttle = true;
		if (sc->sc_freq_target == 0)
			sc->sc_freq_target = clk_get_rate(sc->sc_clk) / 1000000;
	}

	atomic_dec_uint(&sc->sc_busy);
}

static void
cpufreq_dt_throttle_disable(device_t dev)
{
	struct cpufreq_dt_softc * const sc = device_private(dev);

	if (!sc->sc_freq_throttle)
		return;

	while (atomic_cas_uint(&sc->sc_busy, 0, 1) != 0)
		kpause("throttle", false, 1, NULL);

	const u_int freq_khz = sc->sc_freq_target * 1000;

	if (cpufreq_dt_set_rate(sc, freq_khz) == 0) {
		aprint_debug_dev(sc->sc_dev, "throttle disabled (%u.%03u MHz)\n",
		    freq_khz / 1000, freq_khz % 1000);
		sc->sc_freq_throttle = false;
	}

	atomic_dec_uint(&sc->sc_busy);
}

static int
cpufreq_dt_sysctl_helper(SYSCTLFN_ARGS)
{
	struct cpufreq_dt_softc * const sc = rnode->sysctl_data;
	struct sysctlnode node;
	u_int fq, oldfq = 0;
	int error, n;

	node = *rnode;
	node.sysctl_data = &fq;

	if (rnode->sysctl_num == sc->sc_node_target) {
		if (sc->sc_freq_target == 0)
			sc->sc_freq_target = clk_get_rate(sc->sc_clk) / 1000000;
		fq = sc->sc_freq_target;
	} else
		fq = clk_get_rate(sc->sc_clk) / 1000000;

	if (rnode->sysctl_num == sc->sc_node_target)
		oldfq = fq;

	if (sc->sc_freq_target == 0)
		sc->sc_freq_target = fq;

	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return error;

	if (fq == oldfq || rnode->sysctl_num != sc->sc_node_target)
		return 0;

	for (n = 0; n < sc->sc_nopp; n++)
		if (sc->sc_opp[n].freq_khz / 1000 == fq)
			break;
	if (n == sc->sc_nopp)
		return EINVAL;

	if (atomic_cas_uint(&sc->sc_busy, 0, 1) != 0)
		return EBUSY;

	sc->sc_freq_target = fq;

	if (sc->sc_freq_throttle)
		error = 0;
	else
		error = cpufreq_dt_set_rate(sc, fq * 1000);

	atomic_dec_uint(&sc->sc_busy);

	return error;
}

static void
cpufreq_dt_init_sysctl(struct cpufreq_dt_softc *sc)
{
	const struct sysctlnode *node, *cpunode, *freqnode;
	struct sysctllog *cpufreq_log = NULL;
	int error, i;

	sc->sc_freq_available = kmem_zalloc(strlen("XXXX ") * sc->sc_nopp, KM_SLEEP);
	for (i = 0; i < sc->sc_nopp; i++) {
		char buf[6];
		snprintf(buf, sizeof(buf), i ? " %u" : "%u", sc->sc_opp[i].freq_khz / 1000);
		strcat(sc->sc_freq_available, buf);
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
	    cpufreq_dt_sysctl_helper, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	sc->sc_node_target = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    CTLFLAG_READWRITE, CTLTYPE_INT, "current", NULL,
	    cpufreq_dt_sysctl_helper, 0, (void *)sc, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	sc->sc_node_current = node->sysctl_num;

	error = sysctl_createv(&cpufreq_log, 0, &freqnode, &node,
	    0, CTLTYPE_STRING, "available", NULL,
	    NULL, 0, sc->sc_freq_available, 0,
	    CTL_CREATE, CTL_EOL);
	if (error)
		goto sysctl_failed;
	sc->sc_node_available = node->sysctl_num;

	return;

sysctl_failed:
	aprint_error_dev(sc->sc_dev, "couldn't create sysctl nodes: %d\n", error);
	sysctl_teardown(&cpufreq_log);
}

static int
cpufreq_dt_parse(struct cpufreq_dt_softc *sc)
{
	const int phandle = sc->sc_phandle;
	const u_int *opp;
	int len, i;
	u_int lat;

	sc->sc_supply = fdtbus_regulator_acquire(phandle, "cpu-supply");
	if (sc->sc_supply == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't acquire cpu-supply\n");
		return ENXIO;
	}
	sc->sc_clk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_clk == NULL) {
		aprint_error_dev(sc->sc_dev, "couldn't acquire clock\n");
		return ENXIO;
	}

	opp = fdtbus_get_prop(phandle, "operating-points", &len);
	if (len < 8)
		return ENXIO;

	sc->sc_nopp = len / 8;
	sc->sc_opp = kmem_zalloc(sizeof(*sc->sc_opp) * sc->sc_nopp, KM_SLEEP);
	for (i = 0; i < sc->sc_nopp; i++, opp += 2) {
		sc->sc_opp[i].freq_khz = be32toh(opp[0]);
		sc->sc_opp[i].voltage_uv = be32toh(opp[1]);

		aprint_verbose_dev(sc->sc_dev, "%u.%03u MHz, %u uV\n",
		    sc->sc_opp[i].freq_khz / 1000,
		    sc->sc_opp[i].freq_khz % 1000,
		    sc->sc_opp[i].voltage_uv);
	}

	if (of_getprop_uint32(phandle, "clock-latency", &lat) == 0)
		sc->sc_latency = lat;
	else
		sc->sc_latency = -1;

	return 0;
}

static int
cpufreq_dt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;

	if (fdtbus_get_reg(phandle, 0, &addr, NULL) != 0)
		return 0;
	/* Generic DT cpufreq driver properties must be defined under /cpus/cpu@0 */
	if (addr != 0)
		return 0;

	if (!of_hasprop(phandle, "operating-points") ||
	    !of_hasprop(phandle, "clocks") ||
	    !of_hasprop(phandle, "cpu-supply"))
		return 0;

	return 1;
}

static void
cpufreq_dt_init(device_t self)
{
	struct cpufreq_dt_softc * const sc = device_private(self);
	int error;

	if ((error = cpufreq_dt_parse(sc)) != 0)
		return;

	cpufreq_dt_init_sysctl(sc);
}

static void
cpufreq_dt_attach(device_t parent, device_t self, void *aux)
{
	struct cpufreq_dt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal("\n");

	pmf_event_register(self, PMFE_THROTTLE_ENABLE, cpufreq_dt_throttle_enable, true);
	pmf_event_register(self, PMFE_THROTTLE_DISABLE, cpufreq_dt_throttle_disable, true);

	config_interrupts(self, cpufreq_dt_init);
}

CFATTACH_DECL_NEW(cpufreq_dt, sizeof(struct cpufreq_dt_softc),
    cpufreq_dt_match, cpufreq_dt_attach, NULL, NULL);
