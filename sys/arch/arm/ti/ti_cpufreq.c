/* $NetBSD: ti_cpufreq.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

#include "opt_soc.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ti_cpufreq.c,v 1.4 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

static bool		ti_opp_probed = false;
static bool		(*ti_opp_supportedfn)(const int, const int);
static struct syscon	*ti_opp_syscon;

#ifdef SOC_AM33XX

#define	AM33XX_REV_OFFSET	0x0600
#define	AM33XX_REV_MASK		0xf0000000
#define	AM33XX_EFUSE_OFFSET	0x07fc
#define	AM33XX_EFUSE_MASK	0x00001fff
#define	AM33XX_EFUSE_DEFAULT	0x1e2f

static const struct device_compatible_entry am33xx_compat_data[] = {
	{ .compat = "ti,am33xx" },
	DEVICE_COMPAT_EOL
};

static bool
am33xx_opp_supported(const int opp_table, const int opp_node)
{
	const u_int *supported_hw;
	uint32_t efuse, rev;
	int len;

	syscon_lock(ti_opp_syscon);
	rev = __BIT(__SHIFTOUT(syscon_read_4(ti_opp_syscon, AM33XX_REV_OFFSET), AM33XX_REV_MASK));
	efuse = __SHIFTOUT(syscon_read_4(ti_opp_syscon, AM33XX_EFUSE_OFFSET), AM33XX_EFUSE_MASK);
	syscon_unlock(ti_opp_syscon);

	if (efuse == 0)
		efuse = AM33XX_EFUSE_DEFAULT;
	efuse = ~efuse;

	supported_hw = fdtbus_get_prop(opp_node, "opp-supported-hw", &len);
	if (len != 8)
		return false;

	if ((rev & be32toh(supported_hw[0])) == 0)
		return false;

	if ((efuse & be32toh(supported_hw[1])) == 0)
		return false;

	return true;
}
#endif

static void
ti_opp_probe(const int opp_table)
{
	if (ti_opp_probed)
		return;
	ti_opp_probed = true;

	ti_opp_syscon = fdtbus_syscon_acquire(opp_table, "syscon");

#ifdef SOC_AM33XX
	if (ti_opp_syscon &&
	    of_compatible_match(OF_finddevice("/"), am33xx_compat_data))
		ti_opp_supportedfn = am33xx_opp_supported;
#endif
}

static bool
ti_opp_supported(const int opp_table, const int opp_node)
{
	ti_opp_probe(opp_table);

	if (!of_hasprop(opp_node, "opp-supported-hw"))
		return true;

	return ti_opp_supportedfn ? ti_opp_supportedfn(opp_table, opp_node) : false;
}

FDT_OPP(ti, "operating-points-v2-ti-cpu", ti_opp_supported);
