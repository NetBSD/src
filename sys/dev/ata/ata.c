/*      $NetBSD: ata.c,v 1.18 2003/01/27 21:27:52 bouyer Exp $      */
/*
 * Copyright (c) 1998, 2001 Manuel Bouyer.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,     
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ata.c,v 1.18 2003/01/27 21:27:52 bouyer Exp $");

#ifndef WDCDEBUG
#define WDCDEBUG
#endif /* WDCDEBUG */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/syslog.h>

#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/ata/atareg.h>
#include <dev/ata/atavar.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#ifdef WDCDEBUG
extern int wdcdebug_mask; /* init'ed in wdc.c */
#define WDCDEBUG_PRINT(args, level) \
        if (wdcdebug_mask & (level)) \
		printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif

/* Get the disk's parameters */
int
ata_get_params(drvp, flags, prms)
	struct ata_drive_datas *drvp;
	u_int8_t flags;
	struct ataparams *prms;
{
	char tb[DEV_BSIZE];
	struct wdc_command wdc_c;

#if BYTE_ORDER == LITTLE_ENDIAN
	int i;
	u_int16_t *p;
#endif

	WDCDEBUG_PRINT(("wdc_ata_get_parms\n"), DEBUG_FUNCS);

	memset(tb, 0, DEV_BSIZE);
	memset(prms, 0, sizeof(struct ataparams));
	memset(&wdc_c, 0, sizeof(struct wdc_command));

	if (drvp->drive_flags & DRIVE_ATA) {
		wdc_c.r_command = WDCC_IDENTIFY;
		wdc_c.r_st_bmask = WDCS_DRDY;
		wdc_c.r_st_pmask = WDCS_DRQ;
		wdc_c.timeout = 3000; /* 3s */
	} else if (drvp->drive_flags & DRIVE_ATAPI) {
		wdc_c.r_command = ATAPI_IDENTIFY_DEVICE;
		wdc_c.r_st_bmask = 0;
		wdc_c.r_st_pmask = WDCS_DRQ;
		wdc_c.timeout = 10000; /* 10s */
	} else {
		WDCDEBUG_PRINT(("wdc_ata_get_parms: no disks\n"),
		    DEBUG_FUNCS|DEBUG_PROBE);
		return CMD_ERR;
	}
	wdc_c.flags = AT_READ | flags;
	wdc_c.data = tb;
	wdc_c.bcount = DEV_BSIZE;
	if (wdc_exec_command(drvp, &wdc_c) != WDC_COMPLETE) {
		WDCDEBUG_PRINT(("wdc_ata_get_parms: wdc_exec_command failed\n"),
		    DEBUG_FUNCS|DEBUG_PROBE);
		return CMD_AGAIN;
	}
	if (wdc_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		WDCDEBUG_PRINT(("wdc_ata_get_parms: wdc_c.flags=0x%x\n",
		    wdc_c.flags), DEBUG_FUNCS|DEBUG_PROBE);
		return CMD_ERR;
	} else {
		/* if we didn't read any data something is wrong */
		if ((wdc_c.flags & AT_XFDONE) == 0)
			return CMD_ERR;
		/* Read in parameter block. */
		memcpy(prms, tb, sizeof(struct ataparams));
#if BYTE_ORDER == LITTLE_ENDIAN
		/*
		 * Shuffle string byte order.
		 * ATAPI Mitsumi and NEC drives don't need this.
		 */
		if ((prms->atap_config & WDC_CFG_ATAPI_MASK) ==
		    WDC_CFG_ATAPI &&
		    ((prms->atap_model[0] == 'N' &&
			prms->atap_model[1] == 'E') ||
		     (prms->atap_model[0] == 'F' &&
			 prms->atap_model[1] == 'X')))
			return 0;
		for (i = 0; i < sizeof(prms->atap_model); i += 2) {
			p = (u_short *)(prms->atap_model + i);
			*p = ntohs(*p);
		}
		for (i = 0; i < sizeof(prms->atap_serial); i += 2) {
			p = (u_short *)(prms->atap_serial + i);
			*p = ntohs(*p);
		}
		for (i = 0; i < sizeof(prms->atap_revision); i += 2) {
			p = (u_short *)(prms->atap_revision + i);
			*p = ntohs(*p);
		}
#endif
		return CMD_OK;
	}
}

int
ata_set_mode(drvp, mode, flags)
	struct ata_drive_datas *drvp;
	u_int8_t mode;
	u_int8_t flags;
{
	struct wdc_command wdc_c;

	WDCDEBUG_PRINT(("wdc_ata_set_mode=0x%x\n", mode), DEBUG_FUNCS);
	memset(&wdc_c, 0, sizeof(struct wdc_command));

	wdc_c.r_command = SET_FEATURES;
	wdc_c.r_st_bmask = 0;
	wdc_c.r_st_pmask = 0;
	wdc_c.r_precomp = WDSF_SET_MODE;
	wdc_c.r_count = mode;
	wdc_c.flags = AT_READ | flags;
	wdc_c.timeout = 1000; /* 1s */
	if (wdc_exec_command(drvp, &wdc_c) != WDC_COMPLETE)
		return CMD_AGAIN;
	if (wdc_c.flags & (AT_ERROR | AT_TIMEOU | AT_DF)) {
		return CMD_ERR;
	}
	return CMD_OK;
}

void
ata_dmaerr(drvp)
	struct ata_drive_datas *drvp;
{
	/*
	 * Downgrade decision: if we get NERRS_MAX in NXFER.
	 * We start with n_dmaerrs set to NERRS_MAX-1 so that the
	 * first error within the first NXFER ops will immediatly trigger
	 * a downgrade.
	 * If we got an error and n_xfers is bigger than NXFER reset counters.
	 */
	drvp->n_dmaerrs++;
	if (drvp->n_dmaerrs >= NERRS_MAX && drvp->n_xfers <= NXFER) {
		wdc_downgrade_mode(drvp);
		drvp->n_dmaerrs = NERRS_MAX-1;
		drvp->n_xfers = 0;
		return;
	}
	if (drvp->n_xfers > NXFER) {
		drvp->n_dmaerrs = 1; /* just got an error */
		drvp->n_xfers = 1; /* restart counting from this error */
	}
}
