/* $NetBSD: cpu.c,v 1.18.16.9 2010/09/01 01:07:26 matt Exp $ */

/*
 * Copyright 2000, 2001
 * Broadcom Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and copied only
 * in accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * modified or unmodified copies of this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and
 *    retain this copyright notice and list of conditions as they appear in
 *    the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Broadcom Corporation.  The "Broadcom Corporation" name may not be
 *    used to endorse or promote products derived from this software
 *    without the prior written permission of Broadcom Corporation.
 *
 * 3) THIS SOFTWARE IS PROVIDED "AS-IS" AND ANY EXPRESS OR IMPLIED
 *    WARRANTIES, INCLUDING BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, OR
 *    NON-INFRINGEMENT ARE DISCLAIMED. IN NO EVENT SHALL BROADCOM BE LIABLE
 *    FOR ANY DAMAGES WHATSOEVER, AND IN PARTICULAR, BROADCOM SHALL NOT BE
 *    LIABLE FOR DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *    BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *    OR OTHERWISE), EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.18.16.9 2010/09/01 01:07:26 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <mips/locore.h>
#include <mips/cache.h>

#include <machine/cpu.h>
#include <machine/cpuvar.h>

#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/dev/sbscdvar.h>
#include <mips/cfe/cfe_api.h>

#define	READ_REG(rp)		(mips3_ld((volatile uint64_t *)(rp)))

static int	cpu_match(device_t, cfdata_t, void *);
static void	cpu_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(cpu, sizeof(struct cpu_softc),
    cpu_match, cpu_attach, NULL, NULL);

static u_int found = 0;

static int
cpu_match(device_t parent, cfdata_t match, void *aux)
{
	struct zbbus_attach_args *zap = aux;
	int part;

	if (zap->za_locs.za_type != ZBBUS_ENTTYPE_CPU)
		return (0);

	/*
	 * The 3rd hex digit of the part number is the number of CPUs;
	 * ref Table 26, p38 1250-UM101-R.
	 */
	part = G_SYS_PART(READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_REVISION)));
	return (found < ((part >> 8) & 0xf));
}

static void
cpu_attach(device_t parent, device_t self, void *aux)
{
	struct cpu_info *ci;
	struct cpu_softc * const cpu = device_private(self);
	const char * const xname = device_xname(self);
	uint32_t config;
	int plldiv;

	found++;

	/* XXX this code must run on the target CPU */
	config = mips3_cp0_config_read();
	KASSERT((config & MIPS3_CONFIG_K0_MASK) == 5);

	/* Determine CPU frequency */

	/* XXX:  We should determine the CPU frequency from a time source
	 *       not coupled with the CPU crystal, like the RTC.  Unfortunately
	 *       we don't attach that yet...
	 */
	plldiv = G_SYS_PLL_DIV(READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_CFG)));
	if (plldiv == 0) {
		aprint_normal(": PLL_DIV of zero found, assuming 6 (300MHz)\n");
		plldiv = 6;

		aprint_normal("%s", xname);
	}

	if (found == 1) {
		ci = curcpu();
		ci->ci_cpu_freq = 50000000 * plldiv;
		/* Compute the delay divisor. */
		ci->ci_divisor_delay = (ci->ci_cpu_freq + 500000) / 1000000;
		/* Compute clock cycles per hz */
		ci->ci_cycles_per_hz = (ci->ci_cpu_freq + hz / 2 ) / hz;

		aprint_normal(": %lu.%02luMHz (hz cycles = %lu, delay divisor = %lu)\n",
		    ci->ci_cpu_freq / 1000000,
		    (ci->ci_cpu_freq % 1000000) / 10000,
		    ci->ci_cycles_per_hz, ci->ci_divisor_delay);

		KASSERT(ci->ci_cpuid == 0);

		cpu->sb1cpu_dev = self;
		cpu->sb1cpu_ci = ci;
		ci->ci_softc = cpu;

		sb1250_cpu_init(cpu);
	} else {
#if defined(MULTIPROCESSOR)
		int status;
		ci = cpu_info_alloc(NULL, found - 1, 0, found - 1, 0);
		KASSERT(ci);

		cpu->sb1cpu_dev = self;
		cpu->sb1cpu_ci = ci;
		ci->ci_softc = cpu;

		sb1250_cpu_init(cpu);

		status = cfe_cpu_start(ci->ci_cpuid, cpu_trampoline,
		    (long) ci->ci_data.cpu_idlelwp->l_md.md_utf, 0,
		    (long) ci);
		if (status != 0) {
			aprint_error(": CFE call to start failed: %d\n",
			    status);
		}
		const u_long cpu_mask = 1L << cpu_index(ci);
		for (size_t i = 0; i < 10000; i++) {
			if (cpus_hatched & cpu_mask)
				 break;
			DELAY(100);
		}
		if ((cpus_hatched & cpu_mask) == 0) {
			aprint_error(": failed to hatch!\n");
			return;
		}
#else
		aprint_normal_dev(self,
		    "processor off-line; "
		    "multiprocessor support not present in kernel\n");
		return;
#endif
	}

	/*
	 * Announce ourselves.
	 */
	aprint_normal("%s: ", xname);
	cpu_identify(self);

	cpu_attach_common(self, ci);
}
