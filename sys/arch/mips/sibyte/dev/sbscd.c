/* $NetBSD: sbscd.c,v 1.15.8.2 2011/03/05 15:09:51 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sbscd.c,v 1.15.8.2 2011/03/05 15:09:51 bouyer Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/sibyte/include/sb1250_int.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/dev/sbscdvar.h>

#include "locators.h"

static int	sbscd_match(device_t, cfdata_t, void *);
static void	sbscd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sbscd, 0,
    sbscd_match, sbscd_attach, NULL, NULL);

static int	sbscd_print(void *, const char *);
static const char *sbscd_device_type_name(enum sbscd_device_type type);

static const struct sbscd_attach_locs sb1250_sbscd_devs[] = {
#if 0
	{ A_IMR_CPU0_BASE,
	  { -1, -1 },
	  SBSCD_DEVTYPE_ICU },
#endif
	{ A_SCD_WDOG_0,
	  { K_INT_WATCHDOG_TIMER_0, -1 },
	  SBSCD_DEVTYPE_WDOG },
	{ A_SCD_WDOG_1,
	  { K_INT_WATCHDOG_TIMER_1, -1 },
	  SBSCD_DEVTYPE_WDOG },
	{ A_SCD_TIMER_0,
	  { K_INT_TIMER_0, -1 },
	  SBSCD_DEVTYPE_TIMER },
	{ A_SCD_TIMER_1,
	  { K_INT_TIMER_1, -1 },
	  SBSCD_DEVTYPE_TIMER },
	{ A_SCD_TIMER_2,
	  { K_INT_TIMER_2, -1 },
	  SBSCD_DEVTYPE_TIMER },
	{ A_SCD_TIMER_3,
	  { K_INT_TIMER_3, -1 },
	  SBSCD_DEVTYPE_TIMER },
	{ A_SCD_JTAG_BASE + 0x1FFA0, 	/* XXX magic number for jtag addr */
	  { -1, -1 },
	  SBSCD_DEVTYPE_JTAGCONS },
	/* XXX others */
};
static const int sb1250_sbscd_dev_count = __arraycount(sb1250_sbscd_devs);

static int
sbscd_match(device_t parent, cfdata_t match, void *aux)
{
	struct zbbus_attach_args *zap = aux;

	if (zap->za_locs.za_type != ZBBUS_ENTTYPE_SCD)
		return (0);

	return 1;
}

static void
sbscd_attach(device_t parent, device_t self, void *aux)
{
	struct sbscd_attach_args sa;
	int i;
	int locs[SBSCDCF_NLOCS];

	aprint_normal("\n");

	for (i = 0; i < sb1250_sbscd_dev_count; i++) {
		memset(&sa, 0, sizeof sa);
		sa.sa_locs = sb1250_sbscd_devs[i];

		locs[SBSCDCF_OFFSET] = sb1250_sbscd_devs[i].sa_offset;
		locs[SBSCDCF_INTR + 0] =
			sb1250_sbscd_devs[i].sa_intr[0];
		locs[SBSCDCF_INTR + 1] =
			sb1250_sbscd_devs[i].sa_intr[1];

		config_found_sm_loc(self, "sbscd", locs, &sa,
				    sbscd_print, config_stdsubmatch);
	}
	return;
}

int
sbscd_print(void *aux, const char *pnp)
{
	struct sbscd_attach_args *sap = aux;
	int i;

	if (pnp)
		aprint_normal("%s at %s",
		    sbscd_device_type_name(sap->sa_locs.sa_type), pnp);
	aprint_normal(" offset 0x%lx", sap->sa_locs.sa_offset);
	for (i = 0; i < 2; i++) {
		if (sap->sa_locs.sa_intr[i] != -1)
			aprint_normal("%s%d", i == 0 ? " intr " : ",",
			    sap->sa_locs.sa_intr[i]);
	}
	return (UNCONF);
}

static const char *
sbscd_device_type_name(enum sbscd_device_type type)
{

	switch (type) {
	case SBSCD_DEVTYPE_ICU:
		return ("sbicu");
	case SBSCD_DEVTYPE_WDOG:
		return ("sbwdog");
	case SBSCD_DEVTYPE_TIMER:
		return ("sbtimer");
	case SBSCD_DEVTYPE_JTAGCONS:
		return ("sbjcn");

	}
	panic("sbscd_device_type_name");
	return ("panic");
}
