/* $NetBSD: sbobio.c,v 1.12 2003/07/15 02:43:40 lukem Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sbobio.c,v 1.12 2003/07/15 02:43:40 lukem Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <machine/locore.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/dev/sbobiovar.h>

#include "locators.h"

static int	sbobio_match(struct device *, struct cfdata *, void *);
static void	sbobio_attach(struct device *, struct device *, void *);

CFATTACH_DECL(sbobio, sizeof(struct device),
    sbobio_match, sbobio_attach, NULL, NULL);

static int	sbobio_print(void *, const char *);
static int	sbobio_submatch(struct device *, struct cfdata *, void *);
static const char *sbobio_device_type_name(enum sbobio_device_type type);

static const struct sbobio_attach_locs sb1250_rev1_sbobio_devs[] = {
	{ 0x0000, {6,-1},	SBOBIO_DEVTYPE_SMBUS		},
	{ 0x0008, {7,-1},	SBOBIO_DEVTYPE_SMBUS		},
	{ 0x0100, {8,9},	SBOBIO_DEVTYPE_DUART		},
	{ 0x0400, {10,-1},	SBOBIO_DEVTYPE_SYNCSERIAL	},
	{ 0x0800, {11,-1},	SBOBIO_DEVTYPE_SYNCSERIAL	},
#if 0
	{ 0x1000, {-1,-1},	SBOBIO_DEVTYPE_GBUS,		}, /* XXXCGD ??? */
#endif
	{ 0x4000, {19,-1},	SBOBIO_DEVTYPE_MAC,		},
	{ 0x5000, {20,-1},	SBOBIO_DEVTYPE_MAC,		},
	{ 0x6000, {21,-1},	SBOBIO_DEVTYPE_MAC,		},
};
static const int sb1250_rev1_sbobio_dev_count =
    sizeof sb1250_rev1_sbobio_devs / sizeof sb1250_rev1_sbobio_devs[0];

static const struct sbobio_attach_locs sb1250_sbobio_devs[] = {
	{ 0x0000, {6,-1},	SBOBIO_DEVTYPE_SMBUS		},
	{ 0x0008, {7,-1},	SBOBIO_DEVTYPE_SMBUS		},
	{ 0x0100, {8,9},	SBOBIO_DEVTYPE_DUART		},
	{ 0x0400, {10,-1},	SBOBIO_DEVTYPE_SYNCSERIAL	},
	{ 0x0800, {11,-1},	SBOBIO_DEVTYPE_SYNCSERIAL	},
#if 0
	{ 0x1000, {-1,-1},	SBOBIO_DEVTYPE_GBUS,		}, /* XXXCGD ??? */
#endif
	{ 0x4000, {19,61},	SBOBIO_DEVTYPE_MAC,		},
	{ 0x5000, {20,62},	SBOBIO_DEVTYPE_MAC,		},
	{ 0x6000, {21,63},	SBOBIO_DEVTYPE_MAC,		},
};
static const int sb1250_sbobio_dev_count =
    sizeof sb1250_sbobio_devs / sizeof sb1250_sbobio_devs[0];

static const struct sbobio_attach_locs sb112x_sbobio_devs[] = {
	{ 0x0000, {6,-1},	SBOBIO_DEVTYPE_SMBUS		},
	{ 0x0008, {7,-1},	SBOBIO_DEVTYPE_SMBUS		},
	{ 0x0100, {8,9},	SBOBIO_DEVTYPE_DUART		},
	{ 0x0400, {10,-1},	SBOBIO_DEVTYPE_SYNCSERIAL	},
	{ 0x0800, {11,-1},	SBOBIO_DEVTYPE_SYNCSERIAL	},
#if 0
	{ 0x1000, {-1,-1},	SBOBIO_DEVTYPE_GBUS,		}, /* XXXCGD ??? */
#endif
	{ 0x4000, {19,61},	SBOBIO_DEVTYPE_MAC,		},
	{ 0x5000, {20,62},	SBOBIO_DEVTYPE_MAC,		},
};
static const int sb112x_sbobio_dev_count =
    sizeof sb112x_sbobio_devs / sizeof sb112x_sbobio_devs[0];

static int
sbobio_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct zbbus_attach_args *zap = aux;
	uint64_t sysrev;

	if (zap->za_locs.za_type != ZBBUS_ENTTYPE_OBIO)
		return (0);

	sysrev = mips3_ld((u_int64_t *)MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_REVISION));
	switch (SYS_SOC_TYPE(sysrev)) {
	case K_SYS_SOC_TYPE_BCM1120:
	case K_SYS_SOC_TYPE_BCM1125:
	case K_SYS_SOC_TYPE_BCM1125H:
	case K_SYS_SOC_TYPE_BCM1250:
		break;

	default:
		return (0);
	}

	return (1);
}

static void
sbobio_attach(struct device *parent, struct device *self, void *aux)
{
	struct sbobio_attach_args sa;
	const char *dscr;
	const struct sbobio_attach_locs *devs;
	uint64_t sysrev;
	int i, devcount;

	sysrev = mips3_ld((u_int64_t *)MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_REVISION));
	switch (SYS_SOC_TYPE(sysrev)) {
	case K_SYS_SOC_TYPE_BCM1120:
	case K_SYS_SOC_TYPE_BCM1125:
	case K_SYS_SOC_TYPE_BCM1125H:
		dscr = "BCM112x";
		devs = sb112x_sbobio_devs;
		devcount = sb112x_sbobio_dev_count;
		break;

	case K_SYS_SOC_TYPE_BCM1250:
		if (G_SYS_REVISION(sysrev) >= K_SYS_REVISION_BCM1250_PASS2) {
			dscr = "BCM1250 (rev2 and later)";
			devs = sb1250_sbobio_devs;
			devcount = sb1250_sbobio_dev_count;
		} else {
			dscr = "BCM1250 rev1";
			devs = sb1250_rev1_sbobio_devs;
			devcount = sb1250_rev1_sbobio_dev_count;
		}
		break;
#ifdef DIAGNOSTIC
	default:
		panic("un-matched in sbobio_attach");
		break;
#endif
	}

	printf(": %s peripherals\n", dscr);

	for (i = 0; i < devcount; i++) {
		memset(&sa, 0, sizeof sa);
		sa.sa_base = A_PHYS_IO_SYSTEM;
		sa.sa_locs = devs[i];
		config_found_sm(self, &sa, sbobio_print, sbobio_submatch);
	}
	return;
}

int
sbobio_print(void *aux, const char *pnp)
{
	struct sbobio_attach_args *sap = aux;
	int i;

	if (pnp)
		aprint_normal("%s at %s",
		    sbobio_device_type_name(sap->sa_locs.sa_type), pnp);
	aprint_normal(" offset 0x%lx", (long)sap->sa_locs.sa_offset);
	for (i = 0; i < 2; i++) {
		if (sap->sa_locs.sa_intr[i] != SBOBIOCF_INTR_DEFAULT)
			aprint_normal("%s%ld", i == 0 ? " intr " : ",",
			    (long)sap->sa_locs.sa_intr[i]);
	}
	return (UNCONF);
}

static int
sbobio_submatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sbobio_attach_args *sap = aux;
	int i;

	if (cf->cf_loc[SBOBIOCF_OFFSET] != SBOBIOCF_OFFSET_DEFAULT &&
	    cf->cf_loc[SBOBIOCF_OFFSET] != sap->sa_locs.sa_offset)
		return (0);

	for (i = 0; i < 2; i++) {
		if (cf->cf_loc[SBOBIOCF_INTR + i] != SBOBIOCF_INTR_DEFAULT &&
		    cf->cf_loc[SBOBIOCF_INTR + i] != sap->sa_locs.sa_intr[i])
			return (0);
	}

	return (config_match(parent, cf, aux));
}

static const char *
sbobio_device_type_name(enum sbobio_device_type type)
{

	switch (type) {
	case SBOBIO_DEVTYPE_SMBUS:
		return ("sbsmbus");
	case SBOBIO_DEVTYPE_DUART:
		return ("sbscn");
	case SBOBIO_DEVTYPE_SYNCSERIAL:
		return ("sbsync");
	case SBOBIO_DEVTYPE_GBUS:
		return ("sbgbus");
	case SBOBIO_DEVTYPE_MAC:
		return ("sbmac");
	}
	panic("sbobio_device_type_name");
	return ("panic");
}
