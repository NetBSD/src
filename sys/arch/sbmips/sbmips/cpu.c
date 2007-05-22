/* $NetBSD: cpu.c,v 1.14.38.1 2007/05/22 17:27:23 matt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: cpu.c,v 1.14.38.1 2007/05/22 17:27:23 matt Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/cpu.h>

#include <mips/locore.h>
#include <mips/cache.h>

#include <machine/cpu.h>

#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/dev/sbscdvar.h>

#define	READ_REG(rp)		(mips3_ld((uint64_t *)(rp)))

static int	cpu_match(struct device *, struct cfdata *, void *);
static void	cpu_attach(struct device *, struct device *, void *);

CFATTACH_DECL(cpu, sizeof(struct device),
    cpu_match, cpu_attach, NULL, NULL);

static int found = 0;

static int
cpu_match(struct device *parent, struct cfdata *match, void *aux)
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
cpu_attach(struct device *parent, struct device *self, void *aux)
{
	int plldiv;
	uint32_t config;

	/* XXX this code must run on the target CPU */
	config = mips3_cp0_config_read();
	config &= ~MIPS3_CONFIG_K0_MASK;
	config |= 0x05;				/* XXX.  cacheable coherent */
	mips3_cp0_config_write(config);

	found++;

	/*
	 * Flush all of the caches, so that any lines marked non-coherent will
	 * be flushed.  Don't need to worry about L2; it's always
	 * coherent (XXX???).
	 */
	mips_icache_sync_all();
	mips_dcache_wbinv_all();

	/* Determine CPU frequency */

	/* XXX:  We should determine the CPU frequency from a time source
	 *       not coupled with the CPU crystal, like the RTC.  Unfortunately
	 *       we don't attach that yet...
	 */
	plldiv = G_SYS_PLL_DIV(READ_REG(MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_CFG)));
	if (plldiv == 0) {
		printf("PLL_DIV of zero found, assuming 6 (300MHz)\n");
		plldiv = 6;

		printf("%s", self->dv_xname);
	}

	curcpu()->ci_cpu_freq = 50000000 * plldiv;
	/* Compute the delay divisor and reciprical. */
	curcpu()->ci_divisor_delay = curcpu()->ci_cpu_freq / 1000000;
	MIPS_SET_CI_RECIPRICAL(curcpu());
	/* Compute clock cycles per hz */
	curcpu()->ci_cycles_per_hz = curcpu()->ci_cpu_freq / hz;

	printf(": %lu.%02luMHz (hz cycles = %lu, delay divisor = %lu)\n",
	    curcpu()->ci_cpu_freq / 1000000,
	    (curcpu()->ci_cpu_freq % 1000000) / 10000,
	    curcpu()->ci_cycles_per_hz, curcpu()->ci_divisor_delay);

	/*
	 * If we're the primary CPU, no more work to do; we're already
	 * running!
	 */
	if (found == 1) {
		printf("%s: ", self->dv_xname);
		cpu_identify();
	} else {
#if defined(MULTIPROCESSOR)
# error!
#else
		printf("%s: processor off-line; multiprocessor support "
		    "not present in kernel\n", /* sc->sc_dev. */self->dv_xname);
#endif
	}
}
