/* $NetBSD: sbscd.c,v 1.8 2003/07/15 02:43:40 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sbscd.c,v 1.8 2003/07/15 02:43:40 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/dev/sbscdvar.h>

#include "locators.h"

static int	sbscd_match(struct device *, struct cfdata *, void *);
static void	sbscd_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sbscd, sizeof(struct device),
    sbscd_match, sbscd_attach, NULL, NULL);

static int	sbscd_print(void *, const char *);
static int	sbscd_submatch(struct device *, struct cfdata *, void *);
static const char *sbscd_device_type_name(enum sbscd_device_type type);

static const struct sbscd_attach_locs sb1250_sbscd_devs[] = {
#if 0
	{ 0x20000, {-1,-1},	SBSCD_DEVTYPE_ICU,		},
#endif
	{ 0x20050, {0,-1},	SBSCD_DEVTYPE_WDOG,		},
	{ 0x20150, {1,-1},	SBSCD_DEVTYPE_WDOG,		},
	{ 0x20070, {2,-1},	SBSCD_DEVTYPE_TIMER,		},
	{ 0x20078, {3,-1},	SBSCD_DEVTYPE_TIMER,		},
	{ 0x20170, {4,-1},	SBSCD_DEVTYPE_TIMER,		},
	{ 0x20178, {5,-1},	SBSCD_DEVTYPE_TIMER,		},
	{ 0x1FFA0, {-1,-1},	SBSCD_DEVTYPE_JTAGCONS,		},
	/* XXX others */
};
static const int sb1250_sbscd_dev_count =
    sizeof sb1250_sbscd_devs / sizeof sb1250_sbscd_devs[0];

static int
sbscd_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct zbbus_attach_args *zap = aux;

	if (zap->za_locs.za_type != ZBBUS_ENTTYPE_SCD)
		return (0);

	return 1;
}

static void
sbscd_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbscd_attach_args sa;
	int i;

	printf("\n");

	for (i = 0; i < sb1250_sbscd_dev_count; i++) {
		memset(&sa, 0, sizeof sa);
		sa.sa_base = 0x10000000;			/* XXXCGD */
		sa.sa_locs = sb1250_sbscd_devs[i];
		config_found_sm(self, &sa, sbscd_print, sbscd_submatch);
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
	aprint_normal(" offset 0x%lx", (long)sap->sa_locs.sa_offset);
	for (i = 0; i < 2; i++) {
		if (sap->sa_locs.sa_intr[i] != -1)
			aprint_normal("%s%ld", i == 0 ? " intr " : ",",
			    (long)sap->sa_locs.sa_intr[i]);
	}
	return (UNCONF);
}

static int
sbscd_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbscd_attach_args *sap = aux;
	int i;

	if (cf->cf_loc[SBSCDCF_OFFSET] != SBSCDCF_OFFSET_DEFAULT &&
	    cf->cf_loc[SBSCDCF_OFFSET] != sap->sa_locs.sa_offset)
		return (0);

	for (i = 0; i < 2; i++) {
		if (cf->cf_loc[SBSCDCF_INTR + i] != SBSCDCF_INTR_DEFAULT &&
		    cf->cf_loc[SBSCDCF_INTR + i] != sap->sa_locs.sa_intr[i])
			return (0);
	}

	return (config_match(parent, cf, aux));
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
