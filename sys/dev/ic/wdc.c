/*	$NetBSD: wdc.c,v 1.24.2.9 1998/06/19 21:57:44 leo Exp $ */


/*
 * Copyright (c) 1998 Manuel Bouyer.  All rights reserved.
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

/*
 * Copyright (c) 1994, 1995, 1998  Charles M. Hannum.  All rights reserved.
 *
 * DMA and multi-sector PIO handling are derived from code contributed by
 * Onno van der Linden.
 *
 * Bus_space-ified by Christopher G. Demetriou.
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
 *    This product includes software developed by Charles M. Hannum.
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

/*
 * CODE UNTESTED IN THE CURRENT REVISION:
 *   
 */

#define WDCDEBUG

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/proc.h>

#include <vm/vm.h>

#include <machine/intr.h>
#include <machine/bus.h>

#ifndef __BUS_SPACE_HAS_STREAM_METHODS
#define bus_space_write_multi_stream_2	bus_space_write_multi_2
#define bus_space_write_multi_stream_4	bus_space_write_multi_4
#define bus_space_read_multi_stream_2	bus_space_read_multi_2
#define bus_space_read_multi_stream_4	bus_space_read_multi_4
#endif /* __BUS_SPACE_HAS_STREAM_METHODS */

#include <dev/ata/atavar.h>
#include <dev/ata/atareg.h>
#include <dev/ic/wdcreg.h>
#include <dev/ic/wdcvar.h>

#include "wd.h"
#include "atapibus.h"

#define WDCDELAY	  100
#define WDCNDELAY	 100000  /* so 10s for a controller state change */
#define WDCNDELAY_RST 310000  /* 31s for reset complete (specs) */
#if 0
/* If you enable this, it will report any delays more than 100us * N long. */
#define WDCNDELAY_DEBUG	50
#endif

LIST_HEAD(xfer_free_list, wdc_xfer) xfer_free_list;

static int   __wdc_init_controller __P((struct channel_softc *));
static void  __wdcerror	  __P((struct channel_softc*, char *));
static int   __wdcwait_reset  __P((struct channel_softc *, int));
void  __wdccommand_done __P((struct channel_softc *, struct wdc_xfer *));
void  __wdccommand_start __P((struct channel_softc *, struct wdc_xfer *));	
int   __wdccommand_intr __P((struct channel_softc *, struct wdc_xfer *));	


#define DEBUG_INTR   0x01
#define DEBUG_XFERS  0x02
#define DEBUG_STATUS 0x04
#define DEBUG_FUNCS  0x08
#define DEBUG_PROBE  0x10
#ifdef WDCDEBUG
int wdcdebug_mask = DEBUG_PROBE;
int wdc_nxfer = 0;
#define WDCDEBUG_PRINT(args, level)  if (wdcdebug_mask & (level)) printf args
#else
#define WDCDEBUG_PRINT(args, level)
#endif

/*
 * Quick test to see if a controller with at last one attached drive
 * is there. Doesn't wait for reset completion here, as it may take
 * up to 31 seconds, so we just test that at last one device asserts
 * busy after the reset. It's really unlikely that we'll find another device
 * that use ports adresses ranges separated by 0x200 and respond in the same
 * way.
 * Returns a bit for each possible drive found (0x01 for drive 0,
 * 0x02 for drive 1).
 */
int
wdcprobe(chp)
	const struct channel_softc *chp;
{
	u_int8_t st0, st1;
	u_int8_t ret_value = 0x03;

	/*
	 * Sanity check to see if the wdc channel responds at all.
	 */

	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM);
	delay(1);
	st0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | 0x10);
	delay(1);
	st1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);

	WDCDEBUG_PRINT(("%s:%d: before reset, st0=0x%x, st1=0x%x\n",
	    chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe", chp->channel,
	    st0, st1), DEBUG_PROBE);

	/*
	 * If there is a drive, we should not have DRDY and DRQ at the
	 * same time. This will catch most cases where there is no drive there.
	 */
	if ((st0 & WDCS_DRDY) && (st0 & WDCS_DRQ))
		ret_value &= ~0x01;
	if ((st1 & WDCS_DRDY) && (st1 & WDCS_DRQ))
		ret_value &= ~0x02;
	if (ret_value == 0)
		return 0;

	/*
	 * When SRST is asserted, at last one device should set BSY to one.
	 * We don't wait for the reset to complete there, it may takes up to
	 * 31 seconds.
	 * Some controllers seems to put all 0 in the registers while SRST
	 * is asserted. So we have to test BSY after SRST has been
	 * deasserted. This assume that the drives will not reset within
	 * 10-15ms.
	 */
	
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM);
	delay(1);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_RST | WDCTL_IDS); 
	DELAY(1000);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_IDS);
	st0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | 0x10);
	delay(1);
	st1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
	delay(1000);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_4BIT);
	WDCDEBUG_PRINT(("%s:%d: after reset, st0=0x%x st1=0x%x\n",
	    chp->wdc ? chp->wdc->sc_dev.dv_xname : "wdcprobe", chp->channel,
	    st0, st1), DEBUG_PROBE);
	/* We should now have at last one device with BSY set */
	if ((st0 & WDCS_BSY) == 0)
		ret_value &= ~0x01;
	if ((st1 & WDCS_BSY) == 0)
		ret_value &= ~0x02;
	return (ret_value);	
}

/*
 * __wdc_init_controller: Does a quick probe/init of the channel.
 *
 * Return values:
 *	0	No controller present (as far as it can tell).
 *	>0	Controller present and seemingly functional.
 *	<0	Controller present, but not working correctly.
 */
static int
__wdc_init_controller(chp)
	struct channel_softc *chp;
{
	int i, drives;

	drives = wdcprobe(chp);

	WDCDEBUG_PRINT(("__wdc_init_controller: wdcprobe retured 0x%x for "
	    "%s:%d\n", drives, chp->wdc->sc_dev.dv_xname, chp->channel),
	    DEBUG_PROBE);

	if (drives == 0) {
		/* No drives here. no need to go further */
		return 0;
	}
	delay(1000);
	/* Reset started. Now wait for known drives to become ready */
	drives = __wdcwait_reset(chp, drives);

	WDCDEBUG_PRINT(("__wdc_init_controller: reset done, drives now 0x%x "
	    "for %s:%d\n", drives, chp->wdc->sc_dev.dv_xname, chp->channel),
	    DEBUG_PROBE);
	
	for (i = 0; i < 2; i++) {
		if ((drives & (0x01 << i)) == 0)
			continue;
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM | (i << 4));
		delay(1);
		if (wait_for_unbusy(chp) != 0)
			continue;
		/* Test ATAPI signature */
		if (bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_lo)
		    == 0x14 &&
		    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_cyl_hi)
		    == 0xeb) {
			chp->ch_drive[i].drive_flags |= DRIVE_ATAPI;
		} else {
			/* Try an ATA command */
			wdccommandshort(chp, i, WDCC_RECAL);
			if (wait_for_ready(chp) == 0)
				chp->ch_drive[i].drive_flags |= DRIVE_ATA;
		}
	}

	/*
	 * If no drives found, but the resets succeeded, we claim to
	 * have the controller, at least.
	 */
	if ((chp->ch_drive[0].drive_flags & DRIVE) == 0 &&
	    (chp->ch_drive[1].drive_flags & DRIVE)  == 0)
		return (1);

	/* Select drive 0 or ATAPI slave device, and start drive diagnostics */
	if (chp->ch_drive[0].drive_flags & DRIVE)
		i = 0;
	else
		i = 1;
	wdccommandshort(chp, i, WDCC_DIAGNOSE);

	/* Wait for command to complete. */
	if (wait_for_unbusy(chp) < 0)
		return (-1);

	return 1;
}

void
wdcattach(chp)
	struct channel_softc *chp;
{
	int channel_flags, ctrl_flags, i;

	LIST_INIT(&xfer_free_list);
	for (i = 0; i < 2; i++) {
		chp->ch_drive[i].chnl_softc = chp;
		chp->ch_drive[i].drive = i;
	}

	if (__wdc_init_controller(chp) <= 0) {
		printf("%s channel %d: controller wouldn't initialize "
		    "properly\n", chp->wdc->sc_dev.dv_xname, chp->channel);
		return;
	}

	TAILQ_INIT(&chp->ch_queue->sc_xfer);
	ctrl_flags = chp->wdc->sc_dev.dv_cfdata->cf_flags;
	channel_flags = (ctrl_flags >> (NBBY * chp->channel)) & 0xff;
#if 0
	for (drv = 0; drv < 2; drv++) {
		if ((channel_flags & (0x01 << drv)) &&
		    (chp->ch_drives_mask & DRIVE(drv))) {
			chp->ch_drives_mask |= CAP32(drv);
			printf("%s:%d:%d: unsing 32-bits pio transfer\n",
			    chp->wdc->sc_dev.dv_xname, chp->channel, drv);
		}
	}
#endif

	WDCDEBUG_PRINT(("wdcattach: ch_drive_flags 0x%x 0x%x\n",
	    chp->ch_drive[0].drive_flags, chp->ch_drive[1].drive_flags),
	    DEBUG_PROBE);

	/*
	 * Attach an ATAPI bus, if needed.
	 */
	if ((chp->ch_drive[0].drive_flags & DRIVE_ATAPI) ||
	    (chp->ch_drive[1].drive_flags & DRIVE_ATAPI)) {
#if NATAPIBUS > 0
		wdc_atapibus_attach(chp);
#else
		printf("atapibus at %s channel %d not configured\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel);
#endif
	}
	if ((chp->ch_drive[0].drive_flags & DRIVE_ATA) ||
	    (chp->ch_drive[1].drive_flags & DRIVE_ATA)) {
#if NWD > 0
		wdc_ata_attach(chp);
#else
		printf("wd at %s channel %d not configured\n",
		    chp->wdc->sc_dev.dv_xname, chp->channel);
#endif
	}
	/*
	 * Reset channel. The probe, with some combinations of ATA/ATAPI
	 * devices keep it in a mostly working, but strange state (with busy
	 * led on)
	 */
	wdcreset(chp, VERBOSE);
	/*
	 * Read status registers to avoid spurious interrupts.
	 */
	for (i = 1; i >= 0; i--) {
		if (chp->ch_drive[i].drive_flags & DRIVE) {
			bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
			    WDSD_IBM | (i << 4));
			if (wait_for_unbusy(chp) < 0)
				printf("%s:%d:%d: device busy\n",
				    chp->wdc->sc_dev.dv_xname, chp->channel, i);
		}
	}
}

/*
 * Start I/O on a controller, for the given channel.
 * The first xfer may be not for our channel if the channel queues
 * are shared.
 */
void
wdcstart(wdc, channel)
	struct wdc_softc *wdc;
	int channel;
{
	struct wdc_xfer *xfer;
	struct channel_softc *chp;

	/* is there a xfer ? */
	if ((xfer = wdc->channels[channel].ch_queue->sc_xfer.tqh_first) == NULL)
		return;
	chp = &wdc->channels[xfer->channel];
	if ((chp->ch_flags & WDCF_ACTIVE) != 0 ) {
		return; /* channel aleady active */
	}
#ifdef DIAGNOSTIC
	if ((chp->ch_flags & WDCF_IRQ_WAIT) != 0)
		panic("wdcstart: channel waiting for irq\n");
#endif
	if (wdc->cap & WDC_CAPABILITY_HWLOCK)
		if (!(*wdc->claim_hw)(chp, 0))
			return;

	WDCDEBUG_PRINT(("wdcstart: xfer %p channel %d drive %d\n", xfer,
	    xfer->channel, xfer->drive), DEBUG_XFERS);
	chp->ch_flags |= WDCF_ACTIVE;
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
	wdcstart(chp->wdc, chp->channel);
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

	if ((chp->ch_flags & WDCF_IRQ_WAIT) == 0) {
#if 0
		/* Clear the pending interrupt and abort. */
		u_int8_t s =
		    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
#ifdef WDCDEBUG
		u_int8_t e =
		    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error); 
		u_int8_t i =
		    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt);
#else
		bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error); 
		bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_seccnt);
#endif

		WDCDEBUG_PRINT(("wdcintr: inactive controller, "
		    "punting st=%02x er=%02x irr=%02x\n", s, e, i), DEBUG_INTR);

		if (s & WDCS_DRQ) {
			int len;
			len = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
			    wd_cyl_lo) + 256 * bus_space_read_1(chp->cmd_iot,
			    chp->cmd_ioh, wd_cyl_hi);
			WDCDEBUG_PRINT(("wdcintr: clearing up %d bytes\n",
			    len), DEBUG_INTR);
			wdcbit_bucket (chp, len);
		}
#else
		WDCDEBUG_PRINT(("wdcintr: inactive controller\n"), DEBUG_INTR);
#endif
		return 0;
	}

	WDCDEBUG_PRINT(("wdcintr\n"), DEBUG_INTR);
	untimeout(wdctimeout, chp);
	chp->ch_flags &= ~WDCF_IRQ_WAIT;
	xfer = chp->ch_queue->sc_xfer.tqh_first;
	return xfer->c_intr(chp, xfer);
}

/* Put all disk in RESET state */
void wdc_reset_channel(drvp)
	struct ata_drive_datas *drvp;
{
	struct channel_softc *chp = drvp->chnl_softc;
	int drive;
	WDCDEBUG_PRINT(("ata_reset_channel\n"), DEBUG_FUNCS);
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
	u_int8_t st0, st1;
	int drv_mask1, drv_mask2;

	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM); /* master */
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_RST | WDCTL_IDS);
	delay(1000);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_IDS);
	/* Device(s) should assert BSY - see comment in wdcprobe */
	st0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | 0x10); /* slave */
	DELAY(10);
	st1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
	delay(1000);
	(void) bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_error);
	bus_space_write_1(chp->ctl_iot, chp->ctl_ioh, wd_aux_ctlr,
	    WDCTL_4BIT);

	if ((st0 & WDCS_BSY) == 0 && (st1 & WDCS_BSY) == 0) {
		if (verb)
			printf("%s channel %d: device doesn't respond to "
			    "reset\n", chp->wdc->sc_dev.dv_xname,
			    chp->channel);
		return 1;
	}
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
	u_int8_t st0, st1;
	/* wait for BSY to deassert */
	for (timeout = 0; timeout < WDCNDELAY_RST;timeout++) {
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM); /* master */
		delay(1);
		st0 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
		    WDSD_IBM | 0x10); /* slave */
		delay(1);
		st1 = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		/*
		 * If device 0 is present, it will clear its busy bit after
		 *  device 1.
		 * If device 1 is still busy, there there is nothing here.
		 */
		if ((drv_mask & 0x01) != 0) {
			if ((st0 & WDCS_BSY) == 0) {
				if ((st1 & WDCS_BSY) != 0)
					drv_mask &= ~0x02;
			return drv_mask;
			}
		} else if ((drv_mask & 0x02) != 0 && (st1 & WDCS_BSY) == 0) {
			/* No master, slave is ready, it's done */
			return drv_mask;
		}
		delay(WDCDELAY);
	}
	/* Reset timed out. Clear drv_mask accordingly */
	if ((drv_mask & 0x01) && (st0 & WDCS_BSY))
		drv_mask &= ~0x01;
	if ((drv_mask & 0x02) && (st1 & WDCS_BSY))
		drv_mask &= ~0x02;
	return drv_mask;
}

/*
 * Wait for a drive to be !BSY, and have mask in its status register.
 * return -1 for a timeout
 */
int
wdcwait(chp, mask)
	struct channel_softc *chp;
	int mask;
{
	int timeout = 0;
	u_char status;
#ifdef WDCNDELAY_DEBUG
	extern int cold;
#endif

	WDCDEBUG_PRINT(("wdcwait\n"), DEBUG_STATUS);
	chp->ch_error = 0;

	for (;;) {
		chp->ch_status = status =
		    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh, wd_status);
		if ((status & WDCS_BSY) == 0 && (status & mask) == mask)
			break;
		if (++timeout > WDCNDELAY) {
			WDCDEBUG_PRINT(("wdcwait: timeout, status %x "
			    "error %x\n", status,
			    bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
				wd_error)),
			    DEBUG_STATUS);
			return -1;
		}
		delay(WDCDELAY);
	}
	if (status & WDCS_ERR)
		chp->ch_error = bus_space_read_1(chp->cmd_iot, chp->cmd_ioh,
		    wd_error);
#ifdef WDCNDELAY_DEBUG
	/* After autoconfig, there should be no long delays. */
	if (!cold && timeout > WDCNDELAY_DEBUG) {
		struct wdc_xfer *xfer = chp->ch_queue->sc_xfer.tqh_first;
		if (xfer == NULL)
			printf("%s channel %d: warning: busy-wait took %dus\n",
			    chp->wdc->sc_dev.dv_xname, chp->channel,
			    WDCDELAY * timeout);
		else 
			printf("%s:%d:%d: warning: busy-wait took %dus\n",
			    chp->wdc->sc_dev.dv_xname, xfer->channel,
			    xfer->drive,
			    WDCDELAY * timeout);
	}
#endif
	return 0;
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
		printf("\ttype: %s\n", (xfer->c_flags & C_ATAPI) ?
		    "atapi":"ata");
		printf("\tc_bcount: %d\n", xfer->c_bcount);
		printf("\tc_skip: %d\n", xfer->c_skip);
		/*
		 * Call the interrupt routine. If we just missed and interrupt,
		 * it will do what's needed. Else, it will take the needed
		 * action (reset the device).
		 */
		xfer->c_flags |= C_TIMEOU;
		chp->ch_flags &= ~WDCF_IRQ_WAIT;
		xfer->c_intr(chp, xfer);
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

	/*
	 * Probe for 32-bit transfers. Do 2 IDENTIFY cmds, one with 16-bit
	 * and one with 32-bit, and compare results.
	 */
	if (wdc->cap & WDC_CAPABILITY_DATA32) {
		if (ata_get_params(drvp, AT_POLL, &params) != CMD_OK) {
			/* IDENTIFY failed. Can't tell more about the device */
			return;
		}
		/* try again in 32bit */
		drvp->drive_flags |= DRIVE_CAP32;
		ata_get_params(drvp, AT_POLL, &params2);
		if (memcmp(&params, &params2, sizeof(struct ataparams)) != 0) {
			/* Not good. fall back to 16bits */
			drvp->drive_flags &= ~DRIVE_CAP32;
		} else {
			printf("%s: using 32-bits pio transfers\n",
			    drv_dev->dv_xname);
		}
	}
	/*
	 * It's not in the specs, but it seems that some drive 
	 * returns 0xffff in atap_extensions when this field is invalid
	 */
	if (params.atap_extensions != 0xffff &&
		(params.atap_extensions & WDC_EXT_MODES)) {
		printf("%s:", drv_dev->dv_xname);
		printed = 0;
		/*
		 * XXX some drives report something wrong here (they claim to
		 * support PIO mode 8 !). As mode is coded on 3 bits in
		 * SET FEATURE, limit it to 7 (so limit i to 4).
		 */
		for (i = 4; i >= 0; i--) {
			if ((params.atap_piomode_supp & (1 << i)) == 0)
				continue;
			/*
			 * See if mode is accepted.
			 * If the controller can't set its PIO mode,
			 * assume the defaults are good, so don't try
			 * to set it
			 */
			if ((wdc->cap & WDC_CAPABILITY_PIO) != 0)
				if (ata_set_mode(drvp, 0x08 | (i + 3),
				   AT_POLL) != CMD_OK)
					continue;
			if (!printed) { 
				printf(" PIO mode %d", i + 3);
				sep = ",";
				printed = 1;
			}
			/*
			 * If controller's driver can't set its PIO mode,
			 * get the highter one for the drive.
			 */
			if ((wdc->cap & WDC_CAPABILITY_PIO) == 0 ||
			    wdc->pio_mode >= i + 3) {
				drvp->PIO_mode = i + 3;
				break;
			}
		}
		printed = 0;
		for (i = 7; i >= 0; i--) {
			if ((params.atap_dmamode_supp & (1 << i)) == 0)
				continue;
			if (wdc->cap & WDC_CAPABILITY_DMA)
				if (ata_set_mode(drvp, 0x20 | i, AT_POLL)
				    != CMD_OK)
					continue;
			if (!printed) {
				printf("%s DMA mode %d", sep, i);
				sep = ",";
				printed = 1;
			}
			if (wdc->cap & WDC_CAPABILITY_DMA) {
				if (wdc->dma_mode < i)
					continue;
				drvp->DMA_mode = i;
				drvp->drive_flags |= DRIVE_DMA;
			}
			break;
		}
		if (params.atap_extensions & WDC_EXT_UDMA_MODES) {
			for (i = 7; i >= 0; i--) {
				if ((params.atap_udmamode_supp & (1 << i))
				    == 0)
					continue;
				if (ata_set_mode(drvp, 0x40 | i, AT_POLL)
				    != CMD_OK)
					continue;
				printf("%s UDMA mode %d", sep, i);
				sep = ",";
				/*
				 * ATA-4 specs says if a mode is supported,
				 * all lower modes shall be supported.
				 * No need to look further.
				 */
				if (wdc->cap & WDC_CAPABILITY_UDMA) {
					drvp->UDMA_mode = i;
					drvp->drive_flags |= DRIVE_UDMA;
				}
				break;
			}
		}
		printf("\n");
	}
}

int
wdc_exec_command(drvp, wdc_c)
	struct ata_drive_datas *drvp;
	struct wdc_command *wdc_c;
{
	struct channel_softc *chp = drvp->chnl_softc;
	struct wdc_xfer *xfer;
	int s, ret;

	WDCDEBUG_PRINT(("wdc_exec_command\n"), DEBUG_FUNCS);

	/* set up an xfer and queue. Wait for completion */
	xfer = wdc_get_xfer(wdc_c->flags & AT_WAIT ? WDC_CANSLEEP :
	    WDC_NOSLEEP);
	if (xfer == NULL) {
		return WDC_TRY_AGAIN;
	 }

	bzero(xfer, sizeof(struct wdc_xfer));
	xfer->c_flags = C_INUSE;
	xfer->drive = drvp->drive;
	xfer->databuf = wdc_c->data;
	xfer->c_bcount = wdc_c->bcount;
	xfer->cmd = wdc_c;
	xfer->c_start = __wdccommand_start;
	xfer->c_intr = __wdccommand_intr;

	s = splbio();
	wdc_exec_xfer(chp, xfer);
#ifdef DIAGNOSTIC
	if ((wdc_c->flags & AT_POLL) != 0 &&
	    (wdc_c->flags & AT_DONE) == 0)
		panic("wdc_exec_command: polled command not done\n");
#endif
	if (wdc_c->flags & AT_DONE) {
		ret = WDC_COMPLETE;
	} else {
		if (wdc_c->flags & AT_WAIT) {
			tsleep(wdc_c, PRIBIO, "wdccmd", 0);
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

	WDCDEBUG_PRINT(("__wdccommand_start\n"), DEBUG_FUNCS);

	bus_space_write_1(chp->cmd_iot, chp->cmd_ioh, wd_sdh,
	    WDSD_IBM | (drive << 4));
	if (wdcwait(chp, wdc_c->r_st_bmask) != 0) {
		wdc_c->flags |= AT_TIMEOU;
		__wdccommand_done(chp, xfer);
	}
	wdccommand(chp, drive, wdc_c->r_command, wdc_c->r_cyl, wdc_c->r_head,
	    wdc_c->r_sector, wdc_c->r_count, wdc_c->r_precomp);
	if ((wdc_c->flags & AT_POLL) == 0) {
		chp->ch_flags |= WDCF_IRQ_WAIT; /* wait for interrupt */
		timeout(wdctimeout, chp, WAITTIME);
		return;
	}
	/*
	 * Polled command. Wait for drive ready or drq. Done in intr().
	 * Wait for at last 400ns for status bit to be valid.
	 */
	delay(10);
	if (__wdccommand_intr(chp, xfer) == 0) {
		wdc_c->flags |= AT_TIMEOU;
		__wdccommand_done(chp, xfer);
	}
}

int
__wdccommand_intr(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	struct wdc_command *wdc_c = xfer->cmd;
	int bcount = wdc_c->bcount;
	char *data = wdc_c->data;

	WDCDEBUG_PRINT(("__wdccommand_intr\n"), DEBUG_INTR);
	if (wdcwait(chp, wdc_c->r_st_pmask)) {
		wdc_c->flags |= AT_ERROR;
		__wdccommand_done(chp, xfer);
		return 1;
	}
	if (wdc_c->flags & AT_READ) {
		if (chp->ch_drive[xfer->drive].drive_flags & DRIVE_CAP32) {
			bus_space_read_multi_4(chp->cmd_iot, chp->cmd_ioh,
			    wd_data, (u_int32_t*)data, bcount >> 2);
			data += bcount & 0xfffffffc;
			bcount = bcount & 0x03;
		}
		if (bcount > 0)
			bus_space_read_multi_2(chp->cmd_iot, chp->cmd_ioh,
			    wd_data, (u_int16_t *)data, bcount >> 1);
	} else if (wdc_c->flags & AT_WRITE) {
		if (chp->ch_drive[xfer->drive].drive_flags & DRIVE_CAP32) {
			bus_space_write_multi_4(chp->cmd_iot, chp->cmd_ioh,
			    wd_data, (u_int32_t*)data, bcount >> 2);
			data += bcount & 0xfffffffc;
			bcount = bcount & 0x03;
		}
		if (bcount > 0)
			bus_space_write_multi_2(chp->cmd_iot, chp->cmd_ioh,
			    wd_data, (u_int16_t *)data, bcount >> 1);
	}
	__wdccommand_done(chp, xfer);
	return 1;
}

void
__wdccommand_done(chp, xfer)
	struct channel_softc *chp;
	struct wdc_xfer *xfer;
{
	int needdone = xfer->c_flags & C_NEEDDONE;
	struct wdc_command *wdc_c = xfer->cmd;

	WDCDEBUG_PRINT(("__wdccommand_done\n"), DEBUG_FUNCS);
	if (chp->ch_status & WDCS_DWF)
		wdc_c->flags |= AT_DF;
	if (chp->ch_status & WDCS_ERR) {
		wdc_c->flags |= AT_ERROR;
		wdc_c->r_error = chp->ch_error;
	}
	wdc_c->flags |= AT_DONE;
	wdc_free_xfer(chp, xfer);
	if (needdone) {
		if (wdc_c->flags & AT_WAIT)
			wakeup(wdc_c);
		else
			wdc_c->callback(wdc_c->callback_arg);
	}
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
	WDCDEBUG_PRINT(("wdc_exec_xfer %p\n", xfer), DEBUG_FUNCS);

	/* complete xfer setup */
	xfer->channel = chp->channel;
	/* insert at the end of command list */
	TAILQ_INSERT_TAIL(&chp->ch_queue->sc_xfer,xfer , c_xferchain);
	WDCDEBUG_PRINT(("wdcstart from wdc_exec_xfer, flags 0x%x\n",
	    chp->ch_flags), DEBUG_FUNCS);
	wdcstart(chp->wdc, chp->channel);
	xfer->c_flags |= C_NEEDDONE; /* we can now call upper level done() */
}

struct wdc_xfer *
wdc_get_xfer(flags)
	int flags;
{
	struct wdc_xfer *xfer;
	int s;

	s = splbio();
	if ((xfer = xfer_free_list.lh_first) != NULL) {
		LIST_REMOVE(xfer, free_list);
		splx(s);
#ifdef DIAGNOSTIC
		if ((xfer->c_flags & C_INUSE) != 0)
			panic("wdc_get_xfer: xfer already in use\n");
#endif
	} else {
		splx(s);
		WDCDEBUG_PRINT(("wdc:making xfer %d\n",wdc_nxfer), DEBUG_XFERS);
		xfer = malloc(sizeof(*xfer), M_DEVBUF,
		    ((flags & WDC_NOSLEEP) != 0 ? M_NOWAIT : M_WAITOK));
		if (xfer == NULL)
			return 0;
#ifdef DIAGNOSTIC
		xfer->c_flags &= ~C_INUSE;
#endif
#ifdef WDCDEBUG
		wdc_nxfer++;
#endif
	}
#ifdef DIAGNOSTIC
	if ((xfer->c_flags & C_INUSE) != 0)
		panic("wdc_get_xfer: xfer already in use\n");
#endif
	bzero(xfer,sizeof(struct wdc_xfer));
	xfer->c_flags = C_INUSE;
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
	xfer->c_flags &= ~C_INUSE;
	LIST_INSERT_HEAD(&xfer_free_list, xfer, free_list);
	splx(s);
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
		    xfer->channel, xfer->drive, msg);
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
