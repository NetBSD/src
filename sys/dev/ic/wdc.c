/*	$NetBSD: wdc.c,v 1.134 2003/09/25 19:29:49 mycroft Exp $ */

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

/*-
 * Copyright (c) 1998, 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum, by Onno van der Linden and by Manuel Bouyer.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * CODE UNTESTED IN THE CURRENT REVISION:
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: wdc.c,v 1.134 2003/09/25 19:29:49 mycroft Exp $");

#ifndef WDCDEBUG
#define WDCDEBUG
#endif /* WDCDEBUG */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/pool.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <machine/intr.h>
#include <machine/bus.h>

#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_write_multi_stream_2	bus_space_write_multi_2
#define bus_space_write_multi_stream_4	bus_space_write_multi_4
#define bus_space_read_multi_stream_2	bus_space_read_multi_2
#define bus_space_read_multi_stream_4	bus_space_read_multi_4
#endif /* __BUS_SPACE_HAS_STREAM_METHODS */

#include <dev/ata/atavar.h>
#include <dev/ata/wdvar.h>
#include <dev/ata/atareg.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#include "ataraid.h"
#include "atapibus.h"
#include "wd.h"

#if NATARAID > 0
#include <dev/ata/ata_raidvar.h>
#endif

#define WDCDELAY  100 /* 100 microseconds */
#define WDCNDELAY_RST (WDC_RESET_WAIT * 1000 / WDCDELAY)
#if 0
/* If you enable this, it will report any delays more than WDCDELAY * N long. */
#define WDCNDELAY_DEBUG	50
#endif

struct pool wdc_xfer_pool;

#if NWD > 0
extern const struct ata_bustype wdc_ata_bustype; /* in ata_wdc.c */
#else
/* A fake one, the autoconfig will print "wd at foo ... not configured */
const struct ata_bustype wdc_ata_bustype = {
	SCSIPI_BUSTYPE_ATA,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};
#endif

static void  __wdcerror	  __P((struct channel_softc*, char *));
static int   __wdcwait_reset  __P((struct channel_softc *, int));
void  __wdccommand_done __P((struct channel_softc *, struct wdc_xfer *));
void  __wdccommand_start __P((struct channel_softc *, struct wdc_xfer *));	
int   __wdccommand_intr __P((struct channel_softc *, struct wdc_xfer *, int));
int   wdprint __P((void *, const char *));
void wdc_finish_attach __P((struct device *));
void wdc_channel_attach __P((struct channel_softc *));

#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#define DEBUG_DETACH 0x20
#define DEBUG_DELAY  0x40
#ifdef WDCDEBUG
int wdcdebug_mask = 0;
int wdc_nxfer = 0;
#define WDCDEBUG_PRINT(args, level)  if (wdcdebug_mask & (level)) printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif

int
wdprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct ata_device *adev = aux;
	if (pnp)
		aprint_normal("wd at %s", pnp);
	aprint_normal(" channel %d drive %d", adev->adev_channel,
	    adev->adev_drv_data->drive);
	return (UNCONF);
}

/* Test to see controller with at last one attached drive is there.
 * Returns a bit for each possible drive found (0x01 for drive 0,
 * 0x02 for drive 1).
 * Logic:
 * - If a status register is at 0xff, assume there is no drive here
 *   (ISA has pull-up resistors).  Similarly if the status register has
 *   the value we last wrote to the bus (for IDE interfaces without pullups).
 *   If no drive at all -> return.
 * - reset the controller, wait for it to complete (may take up to 31s !).
 *   If timeout -> return.
 * - test ATA/ATAPI signatures. If at last one drive found -> return.
 * - try an ATA command on the master.
 */

int
wdcprobe(chp)
	struct channel_softc *chp;
{
	u_int8_t st0, st1, sc, sn, cl, ch;
	u_int8_t ret_value = 0x03;
	u_int8_t drive;

	/*
	 * Sanity check to see if the wdc channel responds at all.
	 */

	if (chp->wdc == NULL ||
	    (chp->wdc->cap & WDC_CAPABILITY_NO_EXTRA_RESETS) == 0) {

		if (chp->wdc && (chp->wdc->cap & WDC_CAPABILITY_SELECT))
			chp->wdc->select(chp,0);
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM);
		delay(10);	/* 400ns delay */
		st0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		
		if (chp->wdc && (chp->wdc->cap & WDC_CAPABILITY_SELECT))
			chp->wdc->select(chp,1);
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM | 0x10);
		delay(10);	/* 400ns delay */
		st1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);

		WDCDEBUG_PRINT(("%s:%d: before reset, st0=0x%x, st1=0x%x\n",
		    chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe",
		    chp->channel, st0, st1), DEBUG_PROBE);

		if ((st0 & 0x7f) == 0x7f || st0 == WDSD_IBM)
			ret_value &= ~0x01;
		if ((st1 & 0x7f) == 0x7f || st1 == (WDSD_IBM | 0x10))
			ret_value &= ~0x02;

		/* Register writability test, drive 0. */
		if (ret_value & 0x01) {
			if (chp->wdc && (chp->wdc->cap & WDC_CAPABILITY_SELECT))
				chp->wdc->select(chp,0);
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
			    WDSD_IBM);
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo,
			    0x02);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) != 0x02)
				ret_value &= ~0x01;
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo,
			    0x01);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) != 0x01)
				ret_value &= ~0x01;
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sector,
			    0x01);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_sector) != 0x01)
				ret_value &= ~0x01;
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sector,
			    0x02);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_sector) != 0x02)
				ret_value &= ~0x01;
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) != 0x01)
				ret_value &= ~0x01;
		}

		/* Register writability test, drive 1. */
		if (ret_value & 0x02) {
			if (chp->wdc && (chp->wdc->cap & WDC_CAPABILITY_SELECT))
				chp->wdc->select(chp,1);
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
			    WDSD_IBM | 0x10);
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo,
			    0x02);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) != 0x02)
				ret_value &= ~0x02;
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo,
			    0x01);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) != 0x01)
				ret_value &= ~0x02;
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sector,
			    0x01);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_sector) != 0x01)
				ret_value &= ~0x02;
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sector,
			    0x02);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_sector) != 0x02)
				ret_value &= ~0x02;
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) != 0x01)
				ret_value &= ~0x02;
		}

		if (ret_value == 0)
			return 0;

	}

	if (chp->wdc && (chp->wdc->cap & WDC_CAPABILITY_SELECT))
		chp->wdc->select(chp,0);
	/* assert SRST, wait for reset to complete */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM);
	delay(10);	/* 400ns delay */
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_RST | WDCTL_IDS | WDCTL_4BIT); 
	delay(2000);
	(void) bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr, WDCTL_4BIT);
	delay(10);	/* 400ns delay */

	ret_value = __wdcwait_reset(chp, ret_value);
	WDCDEBUG_PRINT(("%s:%d: after reset, ret_value=0x%d\n",
	    chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe", chp->channel,
	    ret_value), DEBUG_PROBE);

	/* if reset failed, there's nothing here */
	if (ret_value == 0)
		return 0;

	/*
	 * Test presence of drives. First test register signatures looking for
	 * ATAPI devices. If it's not an ATAPI and reset said there may be
	 * something here assume it's ATA or OLD. Ghost will be killed later in
	 * attach routine.
	 */
	for (drive = 0; drive < 2; drive++) {
		if ((ret_value & (0x01 << drive)) == 0)
			continue;
		if (chp->wdc && chp->wdc->cap & WDC_CAPABILITY_SELECT)
			chp->wdc->select(chp,drive);
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM | (drive << 4));
		delay(10);	/* 400ns delay */
		/* Save registers contents */
		sc = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt);
		sn = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_sector);
		cl = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo);
		ch = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_hi);

		WDCDEBUG_PRINT(("%s:%d:%d: after reset, sc=0x%x sn=0x%x "
		    "cl=0x%x ch=0x%x\n",
		    chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe",
	    	    chp->channel, drive, sc, sn, cl, ch), DEBUG_PROBE);
		/*
		 * sc & sn are supposted to be 0x1 for ATAPI but in some cases
		 * we get wrong values here, so ignore it.
		 */
		if (cl == 0x14 && ch == 0xeb) {
			chp->ch_drive[drive].drive_flags |= DRIVE_ATAPI;
		} else {
			chp->ch_drive[drive].drive_flags |= DRIVE_ATA;
			if (chp->wdc == NULL ||
			    (chp->wdc->cap & WDC_CAPABILITY_PREATA) != 0)
				chp->ch_drive[drive].drive_flags |= DRIVE_OLD;
		}
	}
	return (ret_value);	
}

void
wdcattach(wdc)
	struct wdc_softc *wdc;
{

	config_interrupts(&wdc->sc_dev, wdc_finish_attach);
}

void
wdc_finish_attach(self)
	struct device *self;
{
	struct wdc_softc *wdc = (void *)self;
	int i;

	for (i = 0; i < wdc->nchannels; i++)
		wdc_channel_attach(wdc->channels[i]);
}

void
wdc_channel_attach(chp)
	struct channel_softc *chp;
{
	int ctrl_flags, i, error;
	struct ataparams params;
	static int inited = 0;

	if (chp->ch_flags & WDCF_DISABLED)
		return;

	callout_init(&chp->ch_callout);

	if ((error = wdc_addref(chp)) != 0) {
		aprint_error("%s: unable to enable controller\n",
		    chp->wdc->sc_dev.dv_xname);
		return;
	}

	if (wdcprobe(chp) == 0)
		/* If no drives, abort attach here. */
		goto out;

	/* initialise global data */
	if (inited == 0) {
		/* Initialize the wdc_xfer pool. */
		pool_init(&wdc_xfer_pool, sizeof(struct wdc_xfer), 0,
		    0, 0, "wdcspl", NULL);
		inited++;
	}
	TAILQ_INIT(&chp->ch_queue->sc_xfer);

	for (i = 0; i < 2; i++) {
		chp->ch_drive[i].chnl_softc = chp;
		chp->ch_drive[i].drive = i;
		/*
		 * Init error counter so that an error withing the first xfers
		 * will trigger a downgrade
		 */
		chp->ch_drive[i].n_dmaerrs = NERRS_MAX-1;

		/* If controller can't do 16bit flag the drives as 32bit */
		if ((chp->wdc->cap &
		    (WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32)) ==
		    WDC_CAPABILITY_DATA32)
			chp->ch_drive[i].drive_flags |= DRIVE_CAP32;
		if ((chp->ch_drive[i].drive_flags & DRIVE) == 0)
			continue;

		/*
		 * Wait a bit, some devices are weird just after a reset.
		 * Then issue a IDENTIFY command, to try to detect slave ghost
		 */
		delay(5000);
		error = ata_get_params(&chp->ch_drive[i], AT_WAIT, &params);
		if (error != CMD_OK) {
			delay(1000000);
			error = ata_get_params(&chp->ch_drive[i], AT_WAIT,
			    &params);
		}
		if (error == CMD_OK) {
			/* If IDENTIFY succeded, this is not an OLD ctrl */
			chp->ch_drive[0].drive_flags &= ~DRIVE_OLD;
			chp->ch_drive[1].drive_flags &= ~DRIVE_OLD;
		} else {
			chp->ch_drive[i].drive_flags &=
			    ~(DRIVE_ATA | DRIVE_ATAPI);
			WDCDEBUG_PRINT(("%s:%d:%d: IDENTIFY failed (%d)\n",
			    chp->wdc->sc_dev.dv_xname,
			    chp->channel, i, error), DEBUG_PROBE);
			if ((chp->ch_drive[i].drive_flags & DRIVE_OLD) == 0)
				continue;
			/*
			 * Pre-ATA drive ?
			 * Test registers writability (Error register not
			 * writable, but cyllo is), then try an ATA command.
			 */
			if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
				chp->wdc->select(chp,i);
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
			    WDSD_IBM | (i << 4));
			delay(10);	/* 400ns delay */
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_error, 0x58);
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo, 0xa5);
			if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			        wd_error) == 0x58 ||
			    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
				wd_cyl_lo) != 0xa5) {
				WDCDEBUG_PRINT(("%s:%d:%d: register "
				    "writability failed\n",
				    chp->wdc->sc_dev.dv_xname,
				    chp->channel, i), DEBUG_PROBE);
				    chp->ch_drive[i].drive_flags &= ~DRIVE_OLD;
			}
			if (wait_for_ready(chp, 10000) != 0) {
				WDCDEBUG_PRINT(("%s:%d:%d: not ready\n",
				    chp->wdc->sc_dev.dv_xname,
				    chp->channel, i), DEBUG_PROBE);
				chp->ch_drive[i].drive_flags &= ~DRIVE_OLD;
				continue;
			}
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_command, WDCC_RECAL);
			delay(10);	/* 400ns delay */
			if (wait_for_ready(chp, 10000) != 0) {
				WDCDEBUG_PRINT(("%s:%d:%d: WDCC_RECAL failed\n",
				    chp->wdc->sc_dev.dv_xname,
				    chp->channel, i), DEBUG_PROBE);
				chp->ch_drive[i].drive_flags &= ~DRIVE_OLD;
			}
		}
	}
	ctrl_flags = chp->wdc->sc_dev.dv_cfdata->cf_flags;

	WDCDEBUG_PRINT(("wdcattach: ch_drive_flags 0x%x 0x%x\n",
	    chp->ch_drive[0].drive_flags, chp->ch_drive[1].drive_flags),
	    DEBUG_PROBE);

	/* If no drives, abort here */
	if ((chp->ch_drive[0].drive_flags & DRIVE) == 0 &&
	    (chp->ch_drive[1].drive_flags & DRIVE) == 0)
		goto out;

	/*
	 * Attach an ATAPI bus, if needed.
	 */
	if ((chp->ch_drive[0].drive_flags & DRIVE_ATAPI) ||
	    (chp->ch_drive[1].drive_flags & DRIVE_ATAPI)) {
#if NATAPIBUS > 0
		wdc_atapibus_attach(chp);
#else
		/*
		 * Fake the autoconfig "not configured" message
		 */
		aprint_normal("atapibus at %s channel %d not configured\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel);
		chp->atapibus = NULL;
#endif
	}

	for (i = 0; i < 2; i++) {
		struct ata_device adev;
		if ((chp->ch_drive[i].drive_flags &
		    (DRIVE_ATA | DRIVE_OLD)) == 0) {
			continue;
		}
		memset(&adev, 0, sizeof(struct ata_device));
		adev.adev_bustype = &wdc_ata_bustype;
		adev.adev_channel = chp->channel;
		adev.adev_openings = 1;
		adev.adev_drv_data = &chp->ch_drive[i];
		chp->ata_drives[i] = config_found(&chp->wdc->sc_dev,
		    &adev, wdprint);
		if (chp->ata_drives[i] != NULL)
			wdc_probe_caps(&chp->ch_drive[i]);
	}

	/*
	 * reset drive_flags for unnatached devices, reset state for attached
	 *  ones
	 */
	for (i = 0; i < 2; i++) {
		if (chp->ch_drive[i].drv_softc == NULL)
			chp->ch_drive[i].drive_flags = 0;
		else
			chp->ch_drive[i].state = 0;
	}

	/*
	 * Reset channel. The probe, with some combinations of ATA/ATAPI
	 * devices keep it in a mostly working, but strange state (with busy
	 * led on)
	 */
	if ((chp->wdc->cap & WDC_CAPABILITY_NO_EXTRA_RESETS) == 0) {
		delay(50);
		wdcreset(chp, VERBOSE);
		/*
		 * Read status registers to avoid spurious interrupts.
		 */
		for (i = 1; i >= 0; i--) {
			if (chp->ch_drive[i].drive_flags & DRIVE) {
				if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
					chp->wdc->select(chp,i);
				bus_space_write_1(chp->cmd_iot, chp->cmd_ioh,
				    wd_sdh, WDSD_IBM | (i << 4));
				if (wait_for_unbusy(chp, 10000) < 0)
					aprint_error("%s:%d:%d: device busy\n",
					    chp->wdc->sc_dev.dv_xname,
					    chp->channel, i);
			}
		}
	}

	if (chp->wdc->cap & WDC_CAPABILITY_MODE) {
		chp->wdc->set_modes(chp);
		wdc_print_modes(chp);
	}

#if NATARAID > 0
	if (chp->wdc->cap & WDC_CAPABILITY_RAID)
		for (i = 0; i < 2; i++)
			if (chp->ata_drives[i] != NULL)
				ata_raid_check_component(chp->ata_drives[i]);
#endif /* NATARAID > 0 */

out:
	wdc_delref(chp);
}

/*
 * Call activate routine of underlying devices.
 */
int
wdcactivate(self, act)
	struct device *self;
	enum devact act;
{
	struct wdc_softc *wdc = (struct wdc_softc *)self;
	struct channel_softc *chp;
	struct device *sc = 0;
	int s, i, j, error = 0;

	s = splbio();
	switch (act) {
	case DVACT_ACTIVATE:
		error = EOPNOTSUPP;
		break;

	case DVACT_DEACTIVATE:
		for (i = 0; i < wdc->nchannels; i++) {
			chp = wdc->channels[i];

			/*
			 * We might call deactivate routine for
			 * the children of atapibus twice (once via
			 * atapibus, once directly), but since
			 * config_deactivate maintains DVF_ACTIVE flag,
			 * it's safe.
			 */
			sc = chp->atapibus;
			if (sc != NULL) {
				error = config_deactivate(sc);
				if (error != 0)
					goto out;
			}

			for (j = 0; j < 2; j++) {
				sc = chp->ch_drive[j].drv_softc;
				WDCDEBUG_PRINT(("wdcactivate: %s:"
				    " deactivating %s\n", wdc->sc_dev.dv_xname,
				    sc == NULL ? "nodrv" : sc->dv_xname),
				    DEBUG_DETACH);
				if (sc != NULL) {
					error = config_deactivate(sc);
					if (error != 0)
						goto out;
				}
			}
		}
		break;
	}

out:
	splx(s);

#ifdef WDCDEBUG
	if (sc && error != 0)
		WDCDEBUG_PRINT(("wdcactivate: %s: error %d deactivating %s\n",
		    wdc->sc_dev.dv_xname, error, sc->dv_xname), DEBUG_DETACH);
#endif
	return (error);
}

int
wdcdetach(self, flags)
	struct device *self;
	int flags;
{
	struct wdc_softc *wdc = (struct wdc_softc *)self;
	struct channel_softc *chp;
	struct device *sc = 0;
	int i, j, error = 0;

	for (i = 0; i < wdc->nchannels; i++) {
		chp = wdc->channels[i];

		/*
		 * Detach atapibus and its children.
		 */
		sc = chp->atapibus;
		if (sc != NULL) {
			WDCDEBUG_PRINT(("wdcdetach: %s: detaching %s\n",
			    wdc->sc_dev.dv_xname, sc->dv_xname), DEBUG_DETACH);
			error = config_detach(sc, flags);
			if (error != 0)
				goto out;
		}

		/*
		 * Detach our other children.
		 */
		for (j = 0; j < 2; j++) {
			if (chp->ch_drive[j].drive_flags & DRIVE_ATAPI)
				continue;
			sc = chp->ch_drive[j].drv_softc;
			WDCDEBUG_PRINT(("wdcdetach: %s: detaching %s\n",
			    wdc->sc_dev.dv_xname,
			    sc == NULL ? "nodrv" : sc->dv_xname),
			    DEBUG_DETACH);
			if (sc != NULL) {
				error = config_detach(sc, flags);
				if (error != 0)
					goto out;
			}
		}

		wdc_kill_pending(chp);
	}

out:
#ifdef WDCDEBUG
	if (sc && error != 0)
		WDCDEBUG_PRINT(("wdcdetach: %s: error %d detaching %s\n",
		    wdc->sc_dev.dv_xname, error, sc->dv_xname), DEBUG_DETACH);
#endif
	return (error);
}

/*
 * Start I/O on a controller, for the given channel.
 * The first xfer may be not for our channel if the channel queues
 * are shared.
 */
void
wdcstart(chp)
	struct channel_softc *chp;
{
	struct wdc_xfer *xfer;

#ifdef WDC_DIAGNOSTIC
	int spl1, spl2;

	spl1 = splbio();
	spl2 = splbio();
	if (spl2 != spl1) {
		printf("wdcstart: not at splbio()\n");
		panic("wdcstart");
	}
	splx(spl2);
	splx(spl1);
#endif /* WDC_DIAGNOSTIC */

	/* is there a xfer ? */
	if ((xfer = chp->ch_queue->sc_xfer.tqh_first) == NULL)
		return;

	/* adjust chp, in case we have a shared queue */
	chp = xfer->chp;

	if ((chp->ch_flags & WDCF_ACTIVE) != 0 ) {
		return; /* channel aleady active */
	}
#ifdef DIAGNOSTIC
	if ((chp->ch_flags & WDCF_IRQ_WAIT) != 0)
		panic("wdcstart: channel waiting for irq");
#endif
	if (chp->wdc->cap & WDC_CAPABILITY_HWLOCK)
		if (!(*chp->wdc->claim_hw)(chp, 0))
			return;

	WDCDEBUG_PRINT(("wdcstart: xfer %p channel %d drive %d\n", xfer,
	    chp->channel, xfer->drive), DEBUG_XFERS);
	chp->ch_flags |= WDCF_ACTIVE;
	if (chp->ch_drive[xfer->drive].drive_flags & DRIVE_RESET) {
		chp->ch_drive[xfer->drive].drive_flags &= ~DRIVE_RESET;
		chp->ch_drive[xfer->drive].state = 0;
	}
	if (chp->wdc->cap & WDC_CAPABILITY_NOIRQ)
		KASSERT(xfer->c_flags & C_POLL);
	xfer->c_start(chp, xfer);
}

/* restart an interrupted I/O */
void
wdcrestart(v)
	void *v;
{
	struct channel_softc *chp = v;
	int s;

	s = splbio();
	wdcstart(chp);
	splx(s);
}
	

/*
 * Interrupt routine for the controller.  Acknowledge the interrupt, check for
 * errors on the current operation, mark it done if necessary, and start the
 * next request.  Also check for a partially done transfer, and continue with
 * the next chunk if so.
 */
int
wdcintr(arg)
	void *arg;
{
	struct channel_softc *chp = arg;
	struct wdc_xfer *xfer;
	int ret;

	if ((chp->wdc->sc_dev.dv_flags & DVF_ACTIVE) == 0) {
		WDCDEBUG_PRINT(("wdcintr: deactivated controller\n"),
		    DEBUG_INTR);
		return (0);
	}
	if ((chp->ch_flags & WDCF_IRQ_WAIT) == 0) {
		WDCDEBUG_PRINT(("wdcintr: inactive controller\n"), DEBUG_INTR);
		/* try to clear the pending interrupt anyway */
		(void)bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		return (0);
	}

	WDCDEBUG_PRINT(("wdcintr\n"), DEBUG_INTR);
	xfer = chp->ch_queue->sc_xfer.tqh_first;
	if (chp->ch_flags & WDCF_DMA_WAIT) {
		chp->wdc->dma_status =
		    (*chp->wdc->dma_finish)(chp->wdc->dma_arg, chp->channel,
			xfer->drive, 0);
		if (chp->wdc->dma_status & WDC_DMAST_NOIRQ) {
			/* IRQ not for us, not detected by DMA engine */
			return 0;
		}
		chp->ch_flags &= ~WDCF_DMA_WAIT;
	}
	chp->ch_flags &= ~WDCF_IRQ_WAIT;
	ret = xfer->c_intr(chp, xfer, 1);
	if (ret == 0) /* irq was not for us, still waiting for irq */
		chp->ch_flags |= WDCF_IRQ_WAIT;
	return (ret);
}

/* Put all disk in RESET state */
void
wdc_reset_channel(drvp)
	struct ata_drive_datas *drvp;
{
	struct channel_softc *chp = drvp->chnl_softc;
	int drive;
	WDCDEBUG_PRINT(("ata_reset_channel %s:%d for drive %d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, drvp->drive),
	    DEBUG_FUNCS);
	(void) wdcreset(chp, VERBOSE);
	for (drive = 0; drive < 2; drive++) {
		chp->ch_drive[drive].state = 0;
	}
}

int
wdcreset(chp, verb)
	struct channel_softc *chp;
	int verb;
{
	int drv_mask1, drv_mask2;

	if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
		chp->wdc->select(chp,0);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM); /* master */
	delay(10);	/* 400ns delay */
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_RST | WDCTL_IDS | WDCTL_4BIT);
	delay(2000);
	(void) bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr, WDCTL_4BIT);
	delay(10);	/* 400ns delay */

	drv_mask1 = (chp->ch_drive[0].drive_flags & DRIVE) ? 0x01:0x00;
	drv_mask1 |= (chp->ch_drive[1].drive_flags & DRIVE) ? 0x02:0x00;
	drv_mask2 = __wdcwait_reset(chp, drv_mask1);
	if (verb && drv_mask2 != drv_mask1) {
		printf("%s channel %d: reset failed for",
		    chp->wdc->sc_dev.dv_xname, chp->channel);
		if ((drv_mask1 & 0x01) != 0 && (drv_mask2 & 0x01) == 0)
			printf(" drive 0");
		if ((drv_mask1 & 0x02) != 0 && (drv_mask2 & 0x02) == 0)
			printf(" drive 1");
		printf("\n");
	}
	return  (drv_mask1 != drv_mask2) ? 1 : 0;
}

static int
__wdcwait_reset(chp, drv_mask)
	struct channel_softc *chp;
	int drv_mask;
{
	int timeout;
	u_int8_t st0, er0, st1, er1;
#ifdef WDCDEBUG
	u_int8_t sc0, sn0, cl0, ch0;
	u_int8_t sc1, sn1, cl1, ch1;
#endif
	/* wait for BSY to deassert */
	for (timeout = 0; timeout < WDCNDELAY_RST; timeout++) {
		if (chp->wdc && chp->wdc->cap & WDC_CAPABILITY_SELECT)
			chp->wdc->select(chp,0);
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM); /* master */
		delay(10);
		st0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		er0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error);
#ifdef WDCDEBUG
		sc0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt);
		sn0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_sector);
		cl0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo);
		ch0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_hi);
#endif
		if (chp->wdc && chp->wdc->cap & WDC_CAPABILITY_SELECT)
			chp->wdc->select(chp,1);
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM | 0x10); /* slave */
		delay(10);
		st1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		er1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error);
#ifdef WDCDEBUG
		sc1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt);
		sn1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_sector);
		cl1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo);
		ch1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_hi);
#endif

		if ((drv_mask & 0x01) == 0) {
			/* no master */
			if ((drv_mask & 0x02) != 0 && (st1 & WDCS_BSY) == 0) {
				/* No master, slave is ready, it's done */
				goto end;
			}
		} else if ((drv_mask & 0x02) == 0) {
			/* no slave */
			if ((drv_mask & 0x01) != 0 && (st0 & WDCS_BSY) == 0) {
				/* No slave, master is ready, it's done */
				goto end;
			}
		} else {
			/* Wait for both master and slave to be ready */
			if ((st0 & WDCS_BSY) == 0 && (st1 & WDCS_BSY) == 0) {
				goto end;
			}
		}
		delay(WDCDELAY);
	}
	/* Reset timed out. Maybe it's because drv_mask was not right */
	if (st0 & WDCS_BSY)
		drv_mask &= ~0x01;
	if (st1 & WDCS_BSY)
		drv_mask &= ~0x02;
end:
	if (er0 != 0x01 && er0 != 0x81)
		drv_mask &= ~0x01;
	if (er1 != 0x01)
		drv_mask &= ~0x02;
	WDCDEBUG_PRINT(("%s:%d:0: after reset, sc=0x%x sn=0x%x "
	    "cl=0x%x ch=0x%x\n",
	     chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe",
	     chp->channel, sc0, sn0, cl0, ch0), DEBUG_PROBE);
	WDCDEBUG_PRINT(("%s:%d:1: after reset, sc=0x%x sn=0x%x "
	    "cl=0x%x ch=0x%x\n",
	     chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe",
	     chp->channel, sc1, sn1, cl1, ch1), DEBUG_PROBE);

	WDCDEBUG_PRINT(("%s:%d: wdcwait_reset() end, st0=0x%x er0=0x%x, st1=0x%x er1=0x%x\n",
	    chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe", chp->channel,
	    st0, er0, st1, er1), DEBUG_PROBE);

	return drv_mask;
}

/*
 * Wait for a drive to be !BSY, and have mask in its status register.
 * return -1 for a timeout after "timeout" ms.
 */
int
wdcwait(chp, mask, bits, timeout)
	struct channel_softc *chp;
	int mask, bits, timeout;
{
	u_char status;
	int time = 0;

	WDCDEBUG_PRINT(("wdcwait %s:%d\n", chp->wdc ?chp->wdc->sc_dev.dv_xname
	    :"none", chp->channel), DEBUG_STATUS);
	chp->ch_error = 0;

	timeout = timeout * 1000 / WDCDELAY; /* delay uses microseconds */

	for (;;) {
		chp->ch_status = status =
		    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		if ((status & (WDCS_BSY | mask)) == bits)
			break;
		if (++time > timeout) {
			WDCDEBUG_PRINT(("wdcwait: timeout (time=%d), "
			    "status %x error %x (mask 0x%x bits 0x%x)\n",
			    time, status,
			    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
				wd_error), mask, bits),
			    DEBUG_STATUS | DEBUG_PROBE | DEBUG_DELAY);
			return -1;
		}
		delay(WDCDELAY);
	}
#ifdef WDCDEBUG
	if (time > 0 && (wdcdebug_mask & DEBUG_DELAY))
		printf("wdcwait: did busy-wait, time=%d\n", time);
#endif
	if (status & WDCS_ERR)
		chp->ch_error = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
		    wd_error);
#ifdef WDCNDELAY_DEBUG
	/* After autoconfig, there should be no long delays. */
	if (!cold && time > WDCNDELAY_DEBUG) {
		struct wdc_xfer *xfer = chp->ch_queue->sc_xfer.tqh_first;
		if (xfer == NULL)
			printf("%s channel %d: warning: busy-wait took %dus\n",
			    chp->wdc->sc_dev.dv_xname, chp->channel,
			    WDCDELAY * time);
		else 
			printf("%s:%d:%d: warning: busy-wait took %dus\n",
			    chp->wdc->sc_dev.dv_xname, chp->channel,
			    xfer->drive,
			    WDCDELAY * time);
	}
#endif
	return 0;
}

/*
 * Busy-wait for DMA to complete
 */
int
wdc_dmawait(chp, xfer, timeout)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
	int timeout;
{
	int time;
	for (time = 0;  time < timeout * 1000 / WDCDELAY; time++) {
		chp->wdc->dma_status =
		    (*chp->wdc->dma_finish)(chp->wdc->dma_arg,
			chp->channel, xfer->drive, 0);
		if ((chp->wdc->dma_status & WDC_DMAST_NOIRQ) == 0)
			return 0;
		delay(WDCDELAY);
	}
	/* timeout, force a DMA halt */
	chp->wdc->dma_status = (*chp->wdc->dma_finish)(chp->wdc->dma_arg,
	    chp->channel, xfer->drive, 1);
	return 1;
}

void
wdctimeout(arg)
	void *arg;
{
	struct channel_softc *chp = (struct channel_softc *)arg;
	struct wdc_xfer *xfer = chp->ch_queue->sc_xfer.tqh_first;
	int s;

	WDCDEBUG_PRINT(("wdctimeout\n"), DEBUG_FUNCS);

	s = splbio();
	if ((chp->ch_flags & WDCF_IRQ_WAIT) != 0) {
		__wdcerror(chp, "lost interrupt");
		printf("\ttype: %s tc_bcount: %d tc_skip: %d\n",
		    (xfer->c_flags & C_ATAPI) ?  "atapi" : "ata",
		    xfer->c_bcount,
		    xfer->c_skip);
		if (chp->ch_flags & WDCF_DMA_WAIT) {
			chp->wdc->dma_status =
			    (*chp->wdc->dma_finish)(chp->wdc->dma_arg,
				chp->channel, xfer->drive, 1);
			chp->ch_flags &= ~WDCF_DMA_WAIT;
		}
		/*
		 * Call the interrupt routine. If we just missed an interrupt,
		 * it will do what's needed. Else, it will take the needed
		 * action (reset the device).
		 * Before that we need to reinstall the timeout callback,
		 * in case it will miss another irq while in this transfer
		 * We arbitray chose it to be 1s
		 */
		callout_reset(&chp->ch_callout, hz, wdctimeout, chp);
		xfer->c_flags |= C_TIMEOU;
		chp->ch_flags &= ~WDCF_IRQ_WAIT;
		xfer->c_intr(chp, xfer, 1);
	} else
		__wdcerror(chp, "missing untimeout");
	splx(s);
}

/*
 * Probe drive's capabilites, for use by the controller later
 * Assumes drvp points to an existing drive. 
 * XXX this should be a controller-indep function
 */
void
wdc_probe_caps(drvp)
	struct ata_drive_datas *drvp;
{
	struct ataparams params, params2;
	struct channel_softc *chp = drvp->chnl_softc;
	struct device *drv_dev = drvp->drv_softc;
	struct wdc_softc *wdc = chp->wdc;
	int i, printed;
	char *sep = "";
	int cf_flags;

	if (ata_get_params(drvp, AT_WAIT, &params) != CMD_OK) {
		/* IDENTIFY failed. Can't tell more about the device */
		return;
	}
	if ((wdc->cap & (WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32)) ==
	    (WDC_CAPABILITY_DATA16 | WDC_CAPABILITY_DATA32)) {
		/*
		 * Controller claims 16 and 32 bit transfers.
		 * Re-do an IDENTIFY with 32-bit transfers,
		 * and compare results.
		 */
		drvp->drive_flags |= DRIVE_CAP32;
		ata_get_params(drvp, AT_WAIT, &params2);
		if (memcmp(&params, &params2, sizeof(struct ataparams)) != 0) {
			/* Not good. fall back to 16bits */
			drvp->drive_flags &= ~DRIVE_CAP32;
		} else {
			aprint_normal("%s: 32-bit data port\n",
			    drv_dev->dv_xname);
		}
	}
#if 0 /* Some ultra-DMA drives claims to only support ATA-3. sigh */
	if (params.atap_ata_major > 0x01 && 
	    params.atap_ata_major != 0xffff) {
		for (i = 14; i > 0; i--) {
			if (params.atap_ata_major & (1 << i)) {
				aprint_normal("%s: ATA version %d\n",
				    drv_dev->dv_xname, i);
				drvp->ata_vers = i;
				break;
			}
		}
	}
#endif

	/* An ATAPI device is at last PIO mode 3 */
	if (drvp->drive_flags & DRIVE_ATAPI)
		drvp->PIO_mode = 3;

	/*
	 * It's not in the specs, but it seems that some drive 
	 * returns 0xffff in atap_extensions when this field is invalid
	 */
	if (params.atap_extensions != 0xffff &&
	    (params.atap_extensions & WDC_EXT_MODES)) {
		printed = 0;
		/*
		 * XXX some drives report something wrong here (they claim to
		 * support PIO mode 8 !). As mode is coded on 3 bits in
		 * SET FEATURE, limit it to 7 (so limit i to 4).
		 * If higher mode than 7 is found, abort.
		 */
		for (i = 7; i >= 0; i--) {
			if ((params.atap_piomode_supp & (1 << i)) == 0)
				continue;
			if (i > 4)
				return;
			/*
			 * See if mode is accepted.
			 * If the controller can't set its PIO mode,
			 * assume the defaults are good, so don't try
			 * to set it
			 */
			if ((wdc->cap & WDC_CAPABILITY_MODE) != 0)
				if (ata_set_mode(drvp, 0x08 | (i + 3),
				   AT_WAIT) != CMD_OK)
					continue;
			if (!printed) { 
				aprint_normal("%s: drive supports PIO mode %d",
				    drv_dev->dv_xname, i + 3);
				sep = ",";
				printed = 1;
			}
			/*
			 * If controller's driver can't set its PIO mode,
			 * get the highter one for the drive.
			 */
			if ((wdc->cap & WDC_CAPABILITY_MODE) == 0 ||
			    wdc->PIO_cap >= i + 3) {
				drvp->PIO_mode = i + 3;
				drvp->PIO_cap = i + 3;
				break;
			}
		}
		if (!printed) {
			/* 
			 * We didn't find a valid PIO mode.
			 * Assume the values returned for DMA are buggy too
			 */
			return;
		}
		drvp->drive_flags |= DRIVE_MODE;
		printed = 0;
		for (i = 7; i >= 0; i--) {
			if ((params.atap_dmamode_supp & (1 << i)) == 0)
				continue;
			if ((wdc->cap & WDC_CAPABILITY_DMA) &&
			    (wdc->cap & WDC_CAPABILITY_MODE))
				if (ata_set_mode(drvp, 0x20 | i, AT_WAIT)
				    != CMD_OK)
					continue;
			if (!printed) {
				aprint_normal("%s DMA mode %d", sep, i);
				sep = ",";
				printed = 1;
			}
			if (wdc->cap & WDC_CAPABILITY_DMA) {
				if ((wdc->cap & WDC_CAPABILITY_MODE) &&
				    wdc->DMA_cap < i)
					continue;
				drvp->DMA_mode = i;
				drvp->DMA_cap = i;
				drvp->drive_flags |= DRIVE_DMA;
			}
			break;
		}
		if (params.atap_extensions & WDC_EXT_UDMA_MODES) {
			printed = 0;
			for (i = 7; i >= 0; i--) {
				if ((params.atap_udmamode_supp & (1 << i))
				    == 0)
					continue;
				if ((wdc->cap & WDC_CAPABILITY_MODE) &&
				    (wdc->cap & WDC_CAPABILITY_UDMA))
					if (ata_set_mode(drvp, 0x40 | i,
					    AT_WAIT) != CMD_OK)
						continue;
				if (!printed) {
					aprint_normal("%s Ultra-DMA mode %d",
					    sep, i);
					if (i == 2)
						aprint_normal(" (Ultra/33)");
					else if (i == 4)
						aprint_normal(" (Ultra/66)");
					else if (i == 5)
						aprint_normal(" (Ultra/100)");
					else if (i == 6)
						aprint_normal(" (Ultra/133)");
					sep = ",";
					printed = 1;
				}
				if (wdc->cap & WDC_CAPABILITY_UDMA) {
					if ((wdc->cap & WDC_CAPABILITY_MODE) &&
					    wdc->UDMA_cap < i)
						continue;
					drvp->UDMA_mode = i;
					drvp->UDMA_cap = i;
					drvp->drive_flags |= DRIVE_UDMA;
				}
				break;
			}
		}
		aprint_normal("\n");
	}

	/* Try to guess ATA version here, if it didn't get reported */
	if (drvp->ata_vers == 0) {
		if (drvp->drive_flags & DRIVE_UDMA)
			drvp->ata_vers = 4; /* should be at last ATA-4 */
		else if (drvp->PIO_cap > 2)
			drvp->ata_vers = 2; /* should be at last ATA-2 */
	}
	cf_flags = drv_dev->dv_cfdata->cf_flags;
	if (cf_flags & ATA_CONFIG_PIO_SET) {
		drvp->PIO_mode =
		    (cf_flags & ATA_CONFIG_PIO_MODES) >> ATA_CONFIG_PIO_OFF;
		drvp->drive_flags |= DRIVE_MODE;
	}
	if ((wdc->cap & WDC_CAPABILITY_DMA) == 0) {
		/* don't care about DMA modes */
		return;
	}
	if (cf_flags & ATA_CONFIG_DMA_SET) {
		if ((cf_flags & ATA_CONFIG_DMA_MODES) ==
		    ATA_CONFIG_DMA_DISABLE) {
			drvp->drive_flags &= ~DRIVE_DMA;
		} else {
			drvp->DMA_mode = (cf_flags & ATA_CONFIG_DMA_MODES) >>
			    ATA_CONFIG_DMA_OFF;
			drvp->drive_flags |= DRIVE_DMA | DRIVE_MODE;
		}
	}
	if ((wdc->cap & WDC_CAPABILITY_UDMA) == 0) {
		/* don't care about UDMA modes */
		return;
	}
	if (cf_flags & ATA_CONFIG_UDMA_SET) {
		if ((cf_flags & ATA_CONFIG_UDMA_MODES) ==
		    ATA_CONFIG_UDMA_DISABLE) {
			drvp->drive_flags &= ~DRIVE_UDMA;
		} else {
			drvp->UDMA_mode = (cf_flags & ATA_CONFIG_UDMA_MODES) >>
			    ATA_CONFIG_UDMA_OFF;
			drvp->drive_flags |= DRIVE_UDMA | DRIVE_MODE;
		}
	}
}

/*
 * downgrade the transfer mode of a drive after an error. return 1 if
 * downgrade was possible, 0 otherwise.
 */
int
wdc_downgrade_mode(drvp)
	struct ata_drive_datas *drvp;
{
	struct channel_softc *chp = drvp->chnl_softc;
	struct device *drv_dev = drvp->drv_softc;
	struct wdc_softc *wdc = chp->wdc;
	int cf_flags = drv_dev->dv_cfdata->cf_flags;

	/* if drive or controller don't know its mode, we can't do much */
	if ((drvp->drive_flags & DRIVE_MODE) == 0 ||
	    (wdc->cap & WDC_CAPABILITY_MODE) == 0)
		return 0;
	/* current drive mode was set by a config flag, let it this way */
	if ((cf_flags & ATA_CONFIG_PIO_SET) ||
	    (cf_flags & ATA_CONFIG_DMA_SET) ||
	    (cf_flags & ATA_CONFIG_UDMA_SET))
		return 0;

	/*
	 * If we were using Ultra-DMA mode > 2, downgrade to mode 2 first.
	 * Maybe we didn't properly notice the cable type
	 * If we were using Ultra-DMA mode 2, downgrade to mode 1 first.
	 * It helps in some cases.
	 */
	if ((drvp->drive_flags & DRIVE_UDMA) && drvp->UDMA_mode >= 2) {
		drvp->UDMA_mode = (drvp->UDMA_mode == 2) ? 1 : 2;
		printf("%s: transfer error, downgrading to Ultra-DMA mode %d\n",
		    drv_dev->dv_xname, drvp->UDMA_mode);
	}

	/*
	 * If we were using ultra-DMA, don't downgrade to multiword DMA
	 * if we noticed a CRC error. It has been noticed that CRC errors
	 * in ultra-DMA lead to silent data corruption in multiword DMA.
	 * Data corruption is less likely to occur in PIO mode.
	 */
	else if ((drvp->drive_flags & DRIVE_UDMA) &&
	    (drvp->drive_flags & DRIVE_DMAERR) == 0) {
		drvp->drive_flags &= ~DRIVE_UDMA;
		drvp->drive_flags |= DRIVE_DMA;
		drvp->DMA_mode = drvp->DMA_cap;
		printf("%s: transfer error, downgrading to DMA mode %d\n",
		    drv_dev->dv_xname, drvp->DMA_mode);
	} else if (drvp->drive_flags & (DRIVE_DMA | DRIVE_UDMA)) {
		drvp->drive_flags &= ~(DRIVE_DMA | DRIVE_UDMA);
		drvp->PIO_mode = drvp->PIO_cap;
		printf("%s: transfer error, downgrading to PIO mode %d\n",
		    drv_dev->dv_xname, drvp->PIO_mode);
	} else /* already using PIO, can't downgrade */
		return 0;

	wdc->set_modes(chp);
	/* reset the channel, which will schedule all drives for setup */
	wdc_reset_channel(drvp);
	return 1;
}

int
wdc_exec_command(drvp, wdc_c)
	struct ata_drive_datas *drvp;
	struct wdc_command *wdc_c;
{
	struct channel_softc *chp = drvp->chnl_softc;
	struct wdc_xfer *xfer;
	int s, ret;

	WDCDEBUG_PRINT(("wdc_exec_command %s:%d:%d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, drvp->drive),
	    DEBUG_FUNCS);

	/* set up an xfer and queue. Wait for completion */
	xfer = wdc_get_xfer(wdc_c->flags & AT_WAIT ? WDC_CANSLEEP :
	    WDC_NOSLEEP);
	if (xfer == NULL) {
		return WDC_TRY_AGAIN;
	 }

	if (chp->wdc->cap & WDC_CAPABILITY_NOIRQ)
		wdc_c->flags |= AT_POLL;
	if (wdc_c->flags & AT_POLL)
		xfer->c_flags |= C_POLL;
	xfer->drive = drvp->drive;
	xfer->databuf = wdc_c->data;
	xfer->c_bcount = wdc_c->bcount;
	xfer->cmd = wdc_c;
	xfer->c_start = __wdccommand_start;
	xfer->c_intr = __wdccommand_intr;
	xfer->c_kill_xfer = __wdccommand_done;

	s = splbio();
	wdc_exec_xfer(chp, xfer);
#ifdef DIAGNOSTIC
	if ((wdc_c->flags & AT_POLL) != 0 &&
	    (wdc_c->flags & AT_DONE) == 0)
		panic("wdc_exec_command: polled command not done");
#endif
	if (wdc_c->flags & AT_DONE) {
		ret = WDC_COMPLETE;
	} else {
		if (wdc_c->flags & AT_WAIT) {
			while ((wdc_c->flags & AT_DONE) == 0) {
				tsleep(wdc_c, PRIBIO, "wdccmd", 0);
			}
			ret = WDC_COMPLETE;
		} else {
			ret = WDC_QUEUED;
		}
	}
	splx(s);
	return ret;
}

void
__wdccommand_start(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{   
	int drive = xfer->drive;
	struct wdc_command *wdc_c = xfer->cmd;

	WDCDEBUG_PRINT(("__wdccommand_start %s:%d:%d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive),
	    DEBUG_FUNCS);

	if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
		chp->wdc->select(chp,drive);

	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (drive << 4));
	if (wdcwait(chp, wdc_c->r_st_bmask | WDCS_DRQ, wdc_c->r_st_bmask,
	    wdc_c->timeout) != 0) {
		wdc_c->flags |= AT_TIMEOU;
		__wdccommand_done(chp, xfer);
		return;
	}
	wdccommand(chp, drive, wdc_c->r_command, wdc_c->r_cyl, wdc_c->r_head,
	    wdc_c->r_sector, wdc_c->r_count, wdc_c->r_precomp);
#if 0
	if (wdc_c->r_command == WDCC_IDENTIFY) {
		/*
		 * This is an IDENTIFY command.  Do an immediate poll of the
		 * status to try to determine if there's actually a device
		 * there.  Since this is a data-bearing command, it should go
		 * to either BSY or DRQ within 400ns.
		 */
		delay(10);	/* 400ns delay */
		if ((bus_space_read_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_altsts) &
		    (WDCS_BSY | WDCS_DRQ | WDCS_ERR)) == 0) {
			__wdccommand_intr(chp, xfer, 0);
			return;
		}
	}
#endif
	if ((wdc_c->flags & AT_POLL) == 0) {
		chp->ch_flags |= WDCF_IRQ_WAIT; /* wait for interrupt */
		callout_reset(&chp->ch_callout, wdc_c->timeout / 1000 * hz,
		    wdctimeout, chp);
		return;
	}
	/*
	 * Polled command. Wait for drive ready or drq. Done in intr().
	 * Wait for at last 400ns for status bit to be valid.
	 */
	delay(10);	/* 400ns delay */
	__wdccommand_intr(chp, xfer, 0);
}

int
__wdccommand_intr(chp, xfer, irq)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
	int irq;
{
	struct wdc_command *wdc_c = xfer->cmd;
	int bcount = wdc_c->bcount;
	char *data = wdc_c->data;

again:
	WDCDEBUG_PRINT(("__wdccommand_intr %s:%d:%d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive), DEBUG_INTR);
	if ((wdc_c->flags & AT_XFDONE) != 0) {
		/*
		 * We have completed a data xfer. The drive should now be
		 * in its initial state
		 */
		if (wdcwait(chp, wdc_c->r_st_bmask | WDCS_DRQ,
		    wdc_c->r_st_bmask, (irq == 0)  ? wdc_c->timeout : 0)) {
			if (irq && (xfer->c_flags & C_TIMEOU) == 0) 
				return 0; /* IRQ was not for us */
			wdc_c->flags |= AT_TIMEOU;
		}
		goto out;
	}
	if (wdcwait(chp, wdc_c->r_st_pmask, wdc_c->r_st_pmask,
	     (irq == 0)  ? wdc_c->timeout : 0)) {
		if (irq && (xfer->c_flags & C_TIMEOU) == 0) 
			return 0; /* IRQ was not for us */
		wdc_c->flags |= AT_TIMEOU;
		goto out;
	}
	if (chp->wdc->cap & WDC_CAPABILITY_IRQACK)
		chp->wdc->irqack(chp);
	if (wdc_c->flags & AT_READ) {
		if ((chp->ch_status & WDCS_DRQ) == 0) {
			wdc_c->flags |= AT_TIMEOU;
			goto out;
		}
		if (chp->ch_drive[xfer->drive].drive_flags & DRIVE_CAP32) {
			bus_space_read_multi_4(chp->data32iot, chp->data32ioh,
			    0, (u_int32_t*)data, bcount >> 2);
			data += bcount & 0xfffffffc;
			bcount = bcount & 0x03;
		}
		if (bcount > 0)
			bus_space_read_multi_2(chp->cmd_iot, chp->cmd_ioh,
			    wd_data, (u_int16_t *)data, bcount >> 1);
		/* at this point the drive should be in its initial state */
		wdc_c->flags |= AT_XFDONE;
	} else if (wdc_c->flags & AT_WRITE) {
		if ((chp->ch_status & WDCS_DRQ) == 0) {
			wdc_c->flags |= AT_TIMEOU;
			goto out;
		}
		if (chp->ch_drive[xfer->drive].drive_flags & DRIVE_CAP32) {
			bus_space_write_multi_4(chp->data32iot, chp->data32ioh,
			    0, (u_int32_t*)data, bcount >> 2);
			data += bcount & 0xfffffffc;
			bcount = bcount & 0x03;
		}
		if (bcount > 0)
			bus_space_write_multi_2(chp->cmd_iot, chp->cmd_ioh,
			    wd_data, (u_int16_t *)data, bcount >> 1);
		wdc_c->flags |= AT_XFDONE;
		if ((wdc_c->flags & AT_POLL) == 0) {
			chp->ch_flags |= WDCF_IRQ_WAIT; /* wait for interrupt */
			callout_reset(&chp->ch_callout,
			    wdc_c->timeout / 1000 * hz, wdctimeout, chp);
			return 1;
		} else {
			goto again;
		}
	}
out:
	__wdccommand_done(chp, xfer);
	return 1;
}

void
__wdccommand_done(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct wdc_command *wdc_c = xfer->cmd;

	WDCDEBUG_PRINT(("__wdccommand_done %s:%d:%d\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, xfer->drive), DEBUG_FUNCS);

	callout_stop(&chp->ch_callout);

	if (chp->ch_status & WDCS_DWF)
		wdc_c->flags |= AT_DF;
	if (chp->ch_status & WDCS_ERR) {
		wdc_c->flags |= AT_ERROR;
		wdc_c->r_error = chp->ch_error;
	}
	wdc_c->flags |= AT_DONE;
	if ((wdc_c->flags & AT_READREG) != 0 &&
	    (chp->wdc->sc_dev.dv_flags & DVF_ACTIVE) != 0 &&
	    (wdc_c->flags & (AT_ERROR | AT_DF)) == 0) {
		wdc_c->r_head = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						 wd_sdh);
		wdc_c->r_cyl = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						wd_cyl_hi) << 8;
		wdc_c->r_cyl |= bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						 wd_cyl_lo);
		wdc_c->r_sector = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						   wd_sector);
		wdc_c->r_count = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						  wd_seccnt);
		wdc_c->r_error = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						  wd_error);
		wdc_c->r_precomp = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
						    wd_precomp);
	}
	wdc_free_xfer(chp, xfer);
	if (wdc_c->flags & AT_WAIT)
		wakeup(wdc_c);
	else if (wdc_c->callback)
		wdc_c->callback(wdc_c->callback_arg);
	wdcstart(chp);
	return;
}

/*
 * Send a command. The drive should be ready.
 * Assumes interrupts are blocked.
 */
void
wdccommand(chp, drive, command, cylin, head, sector, count, precomp)
	struct channel_softc *chp;
	u_int8_t drive;
	u_int8_t command;
	u_int16_t cylin;
	u_int8_t head, sector, count, precomp;
{
	WDCDEBUG_PRINT(("wdccommand %s:%d:%d: command=0x%x cylin=%d head=%d "
	    "sector=%d count=%d precomp=%d\n", chp->wdc->sc_dev.dv_xname,
	    chp->channel, drive, command, cylin, head, sector, count, precomp),
	    DEBUG_FUNCS);

	if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
		chp->wdc->select(chp,drive);

	/* Select drive, head, and addressing mode. */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (drive << 4) | head);
	/* Load parameters. wd_features(ATA/ATAPI) = wd_precomp(ST506) */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_precomp,
	    precomp);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo, cylin);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_hi, cylin >> 8);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sector, sector);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt, count);

	/* Send command. */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_command, command);
	return;
}

/*
 * Send a 48-bit addressing command. The drive should be ready.
 * Assumes interrupts are blocked.
 */
void
wdccommandext(chp, drive, command, blkno, count)
	struct channel_softc *chp;
	u_int8_t drive;
	u_int8_t command;
	u_int64_t blkno;
	u_int16_t count;
{
	WDCDEBUG_PRINT(("wdccommandext %s:%d:%d: command=0x%x blkno=%d "
	    "count=%d\n", chp->wdc->sc_dev.dv_xname,
	    chp->channel, drive, command, (u_int32_t) blkno, count),
	    DEBUG_FUNCS);

	if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
		chp->wdc->select(chp,drive);

	/* Select drive, head, and addressing mode. */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    (drive << 4) | WDSD_LBA);

	/* previous */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_features, 0);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt, count >> 8);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_lba_hi, blkno >> 40);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_lba_mi, blkno >> 32);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_lba_lo, blkno >> 24);

	/* current */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_features, 0);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt, count);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_lba_hi, blkno >> 16);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_lba_mi, blkno >> 8);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_lba_lo, blkno);

	/* Send command. */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_command, command);
	return;
}

/*
 * Simplified version of wdccommand().  Unbusy/ready/drq must be
 * tested by the caller.
 */
void
wdccommandshort(chp, drive, command)
	struct channel_softc *chp;
	int drive;
	int command;
{

	WDCDEBUG_PRINT(("wdccommandshort %s:%d:%d command 0x%x\n",
	    chp->wdc->sc_dev.dv_xname, chp->channel, drive, command),
	    DEBUG_FUNCS);

	if (chp->wdc->cap & WDC_CAPABILITY_SELECT)
		chp->wdc->select(chp,drive);

	/* Select drive. */
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (drive << 4));

	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_command, command);
}

/* Add a command to the queue and start controller. Must be called at splbio */

void
wdc_exec_xfer(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	WDCDEBUG_PRINT(("wdc_exec_xfer %p channel %d drive %d\n", xfer,
	    chp->channel, xfer->drive), DEBUG_XFERS);

	/* complete xfer setup */
	xfer->chp = chp;

	/*
	 * If we are a polled command, and the list is not empty,
	 * we are doing a dump. Drop the list to allow the polled command
	 * to complete, we're going to reboot soon anyway.
	 */
	if ((xfer->c_flags & C_POLL) != 0 &&
	    chp->ch_queue->sc_xfer.tqh_first != NULL) {
		TAILQ_INIT(&chp->ch_queue->sc_xfer);
	}
	/* insert at the end of command list */
	TAILQ_INSERT_TAIL(&chp->ch_queue->sc_xfer,xfer , c_xferchain);
	WDCDEBUG_PRINT(("wdcstart from wdc_exec_xfer, flags 0x%x\n",
	    chp->ch_flags), DEBUG_XFERS);
	wdcstart(chp);
}

struct wdc_xfer *
wdc_get_xfer(flags)
	int flags;
{
	struct wdc_xfer *xfer;
	int s;

	s = splbio();
	xfer = pool_get(&wdc_xfer_pool,
	    ((flags & WDC_NOSLEEP) != 0 ? PR_NOWAIT : PR_WAITOK));
	splx(s);
	if (xfer != NULL) {
		memset(xfer, 0, sizeof(struct wdc_xfer));
	}
	return xfer;
}

void
wdc_free_xfer(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct wdc_softc *wdc = chp->wdc;
	int s;

	if (wdc->cap & WDC_CAPABILITY_HWLOCK)
		(*wdc->free_hw)(chp);
	s = splbio();
	chp->ch_flags &= ~WDCF_ACTIVE;
	TAILQ_REMOVE(&chp->ch_queue->sc_xfer, xfer, c_xferchain);
	pool_put(&wdc_xfer_pool, xfer);
	splx(s);
}

/*
 * Kill off all pending xfers for a channel_softc.
 *
 * Must be called at splbio().
 */
void
wdc_kill_pending(chp)
	struct channel_softc *chp;
{
	struct wdc_xfer *xfer;

	while ((xfer = TAILQ_FIRST(&chp->ch_queue->sc_xfer)) != NULL) {
		chp = xfer->chp;
		(*xfer->c_kill_xfer)(chp, xfer);
	}
}

static void
__wdcerror(chp, msg) 
	struct channel_softc *chp;
	char *msg;
{
	struct wdc_xfer *xfer = chp->ch_queue->sc_xfer.tqh_first;

	if (xfer == NULL)
		printf("%s:%d: %s\n", chp->wdc->sc_dev.dv_xname, chp->channel,
		    msg);
	else
		printf("%s:%d:%d: %s\n", chp->wdc->sc_dev.dv_xname,
		    chp->channel, xfer->drive, msg);
}

/* 
 * the bit bucket
 */
void
wdcbit_bucket(chp, size)
	struct channel_softc *chp; 
	int size;
{

	for (; size >= 2; size -= 2)
		(void)bus_space_read_2(chp->cmd_iot, chp->cmd_ioh, wd_data);
	if (size)
		(void)bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_data);
}

int
wdc_addref(chp)
	struct channel_softc *chp;
{
	struct wdc_softc *wdc = chp->wdc; 
	struct scsipi_adapter *adapt = &wdc->sc_atapi_adapter._generic;
	int s, error = 0;

	s = splbio();
	if (adapt->adapt_refcnt++ == 0 &&
	    adapt->adapt_enable != NULL) {
		error = (*adapt->adapt_enable)(&wdc->sc_dev, 1);
		if (error)
			adapt->adapt_refcnt--;
	}
	splx(s);
	return (error);
}

void
wdc_delref(chp)
	struct channel_softc *chp;
{
	struct wdc_softc *wdc = chp->wdc;
	struct scsipi_adapter *adapt = &wdc->sc_atapi_adapter._generic;
	int s;

	s = splbio();
	if (adapt->adapt_refcnt-- == 1 &&
	    adapt->adapt_enable != NULL)
		(void) (*adapt->adapt_enable)(&wdc->sc_dev, 0);
	splx(s);
}

void
wdc_print_modes(struct channel_softc *chp)
{
	int drive;
	struct ata_drive_datas *drvp;

	for (drive = 0; drive < 2; drive++) {
		drvp = &chp->ch_drive[drive];
		if ((drvp->drive_flags & DRIVE) == 0)
			continue;
		aprint_normal("%s(%s:%d:%d): using PIO mode %d",
			drvp->drv_softc->dv_xname,
			chp->wdc->sc_dev.dv_xname,
			chp->channel, drive, drvp->PIO_mode);
		if (drvp->drive_flags & DRIVE_DMA)
			aprint_normal(", DMA mode %d", drvp->DMA_mode);
		if (drvp->drive_flags & DRIVE_UDMA) {
			aprint_normal(", Ultra-DMA mode %d", drvp->UDMA_mode);
			if (drvp->UDMA_mode == 2)
				aprint_normal(" (Ultra/33)");
			else if (drvp->UDMA_mode == 4)
				aprint_normal(" (Ultra/66)");
			else if (drvp->UDMA_mode == 5)
				aprint_normal(" (Ultra/100)");
			else if (drvp->UDMA_mode == 6)
				aprint_normal(" (Ultra/133)");
		}
		if (drvp->drive_flags & (DRIVE_DMA | DRIVE_UDMA))
			aprint_normal(" (using DMA data transfers)");
		aprint_normal("\n");
	}
}
