/* $NetBSD: sbobio.c,v 1.22.30.1 2016/10/05 20:55:32 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sbobio.c,v 1.22.30.1 2016/10/05 20:55:32 skrll Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <mips/locore.h>

#include <mips/sibyte/include/sb1250_int.h>
#include <mips/sibyte/include/sb1250_regs.h>
#include <mips/sibyte/include/sb1250_scd.h>
#include <mips/sibyte/include/zbbusvar.h>
#include <mips/sibyte/dev/sbobiovar.h>

#include "locators.h"

static int	sbobio_match(device_t, cfdata_t, void *);
static void	sbobio_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(sbobio, 0,
    sbobio_match, sbobio_attach, NULL, NULL);

static int	sbobio_print(void *, const char *);
static const char *sbobio_device_type_name(enum sbobio_device_type type);

/*
 * XXXsimonb:
 *	DUART register addresses seem to offset by 0x100 for some
 *	reason I can't fathom so we just can't use A_DUART for the
 *	address of the DUART.  Eg,
 *		A_DUART_CHANREG(0, 0) = 0x0010060000
 *	   and	R_DUART_MODE_REG_1 = 0x100 (first reg in the duart)
 *	but the 1125/1250 manual says the DUART starts at
 *	0x0010060100.
 *	We define our own R_DUART_REGBASE to R_DUART_MODE_REG_1
 *	so that we can use a symbolic name to refer to the start
 *	of the duart register space.
 */

static const struct sbobio_attach_locs sb1250_rev1_sbobio_devs[] = {
	{ A_SMB_0,
	  { K_INT_SMB_0, -1 },
	  SBOBIO_DEVTYPE_SMBUS },
	{ A_SMB_1,
	  { K_INT_SMB_1, -1 },
	  SBOBIO_DEVTYPE_SMBUS },
	{ A_DUART_CHANREG(0, R_DUART_REGBASE),
	  { K_INT_UART_0, K_INT_UART_1 },
	  SBOBIO_DEVTYPE_DUART },
	{ A_SER_BASE_0,
	  { K_INT_SER_0, -1 },
	  SBOBIO_DEVTYPE_SYNCSERIAL },
	{ A_SER_BASE_1,
	  { K_INT_SER_1, -1 },
	  SBOBIO_DEVTYPE_SYNCSERIAL },
#if 0
	{ A_IO_EXT_BASE,
	  { -1, -1 },
	  SBOBIO_DEVTYPE_GBUS },
#endif
	{ A_MAC_BASE_0,
	  { K_INT_MAC_0, -1 },
	  SBOBIO_DEVTYPE_MAC },
	{ A_MAC_BASE_1,
	  { K_INT_MAC_1, -1 },
	  SBOBIO_DEVTYPE_MAC },
	{ A_MAC_BASE_2,
	  { K_INT_MAC_2, -1 },
	  SBOBIO_DEVTYPE_MAC },
};
static const int sb1250_rev1_sbobio_dev_count =
    sizeof sb1250_rev1_sbobio_devs / sizeof sb1250_rev1_sbobio_devs[0];

static const struct sbobio_attach_locs sb1250_sbobio_devs[] = {
	{ A_SMB_0,
	  { K_INT_SMB_0, -1 },
	  SBOBIO_DEVTYPE_SMBUS },
	{ A_SMB_1,
	  { K_INT_SMB_1, -1 },
	  SBOBIO_DEVTYPE_SMBUS },
	{ A_DUART_CHANREG(0, R_DUART_REGBASE),
	  { K_INT_UART_0, K_INT_UART_1},
	  SBOBIO_DEVTYPE_DUART },
	{ A_SER_BASE_0,
	  { K_INT_SER_0, -1 },
	  SBOBIO_DEVTYPE_SYNCSERIAL },
	{ A_SER_BASE_1,
	  { K_INT_SER_1, -1 },
	  SBOBIO_DEVTYPE_SYNCSERIAL },
#if 0
	{ A_IO_EXT_BASE,
	  { -1, -1 },
	  SBOBIO_DEVTYPE_GBUS },
#endif
	{ A_MAC_BASE_0,
	  { K_INT_MAC_0, K_INT_MAC_0_CH1 },
	  SBOBIO_DEVTYPE_MAC },
	{ A_MAC_BASE_1,
	  { K_INT_MAC_1, K_INT_MAC_1_CH1 },
	  SBOBIO_DEVTYPE_MAC },
	{ A_MAC_BASE_2,
	  { K_INT_MAC_2, K_INT_MAC_2_CH1 },
	  SBOBIO_DEVTYPE_MAC },
};
static const int sb1250_sbobio_dev_count =
    sizeof sb1250_sbobio_devs / sizeof sb1250_sbobio_devs[0];

static const struct sbobio_attach_locs sb112x_sbobio_devs[] = {
	{ A_SMB_0,
	  { K_INT_SMB_0, -1 },
	  SBOBIO_DEVTYPE_SMBUS },
	{ A_SMB_1,
	  { K_INT_SMB_1, -1 },
	  SBOBIO_DEVTYPE_SMBUS },
	{ A_DUART_CHANREG(0, R_DUART_REGBASE),
	  { K_INT_UART_0, K_INT_UART_1 },
	  SBOBIO_DEVTYPE_DUART },
	{ A_SER_BASE_0,
	  { K_INT_SER_0, -1 },
	  SBOBIO_DEVTYPE_SYNCSERIAL },
	{ A_SER_BASE_1,
	  { K_INT_SER_1, -1 },
	  SBOBIO_DEVTYPE_SYNCSERIAL },
#if 0
	{ A_IO_EXT_BASE,
	  { -1, -1 },
	  SBOBIO_DEVTYPE_GBUS },
#endif
	{ A_MAC_BASE_0,
	  { K_INT_MAC_0, K_INT_MAC_0_CH1 },
	  SBOBIO_DEVTYPE_MAC },
	{ A_MAC_BASE_1,
	  { K_INT_MAC_1, K_INT_MAC_1_CH1 },
	  SBOBIO_DEVTYPE_MAC },
};
static const int sb112x_sbobio_dev_count =
    sizeof sb112x_sbobio_devs / sizeof sb112x_sbobio_devs[0];

static int
sbobio_match(device_t parent, cfdata_t match, void *aux)
{
	struct zbbus_attach_args *zap = aux;
	uint64_t sysrev;

	if (zap->za_locs.za_type != ZBBUS_ENTTYPE_OBIO)
		return (0);

	sysrev = mips3_ld((register_t)MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_REVISION));
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
sbobio_attach(device_t parent, device_t self, void *aux)
{
	struct sbobio_attach_args sa;
	const char *dscr;
	const struct sbobio_attach_locs *devs;
	uint64_t sysrev;
	int i, devcount;
	int locs[SBOBIOCF_NLOCS];

	sysrev = mips3_ld((register_t)MIPS_PHYS_TO_KSEG1(A_SCD_SYSTEM_REVISION));
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
	default:
		panic("un-matched in sbobio_attach");
		break;
	}

	aprint_normal(": %s peripherals\n", dscr);

	for (i = 0; i < devcount; i++) {
		memset(&sa, 0, sizeof sa);
		sa.sa_locs = devs[i];

		locs[SBOBIOCF_OFFSET] = devs[i].sa_offset;
		locs[SBOBIOCF_INTR + 0] = devs[i].sa_intr[0];
		locs[SBOBIOCF_INTR + 1] = devs[i].sa_intr[1];

		config_found_sm_loc(self, "sbobio", locs, &sa,
				    sbobio_print, config_stdsubmatch);
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
	aprint_normal(" offset 0x%lx", sap->sa_locs.sa_offset);
	for (i = 0; i < 2; i++) {
		if (sap->sa_locs.sa_intr[i] != SBOBIOCF_INTR_DEFAULT)
			aprint_normal("%s%d", i == 0 ? " intr " : ",",
			    sap->sa_locs.sa_intr[i]);
	}
	return (UNCONF);
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
