/*	$NetBSD: weasel.c,v 1.2 2001/04/26 17:58:28 thorpej Exp $	*/

/*-
 * Copyright (c) 2000 Zembu Labs, Inc.
 * All rights reserved.
 *
 * Author: Jason R. Thorpe <thorpej@zembu.com>
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
 *	This product includes software developed by Zembu Labs, Inc.
 * 4. Neither the name of Zembu Labs nor the names of its employees may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ZEMBU LABS, INC. ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WAR-
 * RANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DIS-
 * CLAIMED.  IN NO EVENT SHALL ZEMBU LABS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device driver for the Middle Digital, Inc. PC-Weasel serial
 * console board.
 *
 * We're glued into the MDA display driver (`pcdisplay'), and
 * handle things like the watchdog timer.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/wdog.h>

#include <machine/bus.h>

#include <dev/isa/weaselreg.h>
#include <dev/isa/weaselvar.h>

#include <dev/sysmon/sysmonvar.h>

int	weasel_wdog_setmode(struct sysmon_wdog *);
int	weasel_wdog_tickle(struct sysmon_wdog *);

int	weasel_wdog_toggle(struct weasel_handle *);

void	pcweaselattach(int);

/* ARGSUSED */
void
pcweaselattach(int count)
{

	/* Nothing to do; pseudo-device glue. */
}

void
weasel_init(struct weasel_handle *wh)
{
	struct weasel_config_block cfg;
	int i, j; 
	u_int8_t *cp, sum; 
	const char *vers, *mode;

	/*
	 * Write a NOP to the command register and see if it
	 * reverts back to READY within 1.5 seconds.
	 */
	bus_space_write_1(wh->wh_st, wh->wh_sh, WEASEL_MISC_COMMAND, OS_NOP);
	for (i = 0; i < 1500; i++) {
		delay(1000);
		sum = bus_space_read_1(wh->wh_st, wh->wh_sh,
		    WEASEL_MISC_COMMAND);
		if (sum == OS_READY)
			break;
	}
	if (sum != OS_READY) {
		/* This is not a Weasel. */
		return;
	}

	/*
	 * It can take a while for the config block to be copied
	 * into the offscreen area, as the Weasel may be busy
	 * sending data to the terminal.  Wait up to 3 seconds,
	 * reading the block each time, and breaking out of the
	 * loop once the checksum passes.
	 */

	bus_space_write_1(wh->wh_st, wh->wh_sh, WEASEL_MISC_COMMAND,
	    OS_CONFIG_COPY);

	/* ...one second to get it started... */
	delay(1000 * 1000);

	/* ...two seconds to let it finish... */
	for (i = 0; i < 2000; i++) {
		delay(1000);
		bus_space_read_region_1(wh->wh_st, wh->wh_sh,
		    WEASEL_CONFIG_BLOCK, (u_int8_t *) &cfg, sizeof(cfg));
		/*
		 * Compute the checksum of the config block.
		 */
		for (cp = (u_int8_t *)&cfg, j = 0, sum = 1;
		     j < (sizeof(cfg) - 1); j++)
			sum += cp[j];
		if (sum == cfg.cksum)
			break;
	}

	if (sum != cfg.cksum) {
		/*
		 * Checksum doesn't match; either it's not a Weasel,
		 * or something is wrong with it.
		 */
		printf("%s: PC-Weasel config block checksum mismatch "
		    "0x%02x != 0x%02x\n", wh->wh_parent->dv_xname,
		    sum, cfg.cksum);
		return;
	}

	switch (cfg.cfg_version) {
	case CFG_VERSION_1_0:
		vers = "1.0";
		switch (cfg.enable_duart_switching) {
		case 0:
			mode = "emulation";
			break;

		case 1:
			mode = "autoswitch";
			break;

		default:
			mode = "unknown";
		}
		break;

	case CFG_VERSION_1_1:
		vers = "1.1";
		switch (cfg.enable_duart_switching) {
		case 0:
			mode = "emulation";
			break;

		case 1:
			mode = "serial";
			break;

		case 2:
			mode = "autoswitch";
			break;

		default:
			mode = "unknown";
		}
		break;

	default:
		vers = mode = NULL;
	}

	printf("%s: PC-Weasel, ", wh->wh_parent->dv_xname);
	if (vers != NULL)
		printf("version %s, %s mode", vers, mode);
	else
		printf("unknown version 0x%x", cfg.cfg_version);
	printf("\n");

	printf("%s: break passthrough %s", wh->wh_parent->dv_xname,
	    cfg.break_passthru ? "enabled" : "disabled");
	if (cfg.wdt_allow) {
		if (cfg.wdt_msec == 0) {
			/*
			 * Old firmware -- these Weasels have
			 * a 3000ms watchdog period.
			 */
			cfg.wdt_msec = 3000;
		}

		/*
		 * There's no way to determine what mode the watchdog
		 * is in, but we can safely assume that it starts off
		 * disarmed.
		 */
		wh->wh_wdog_armed = 0;
		wh->wh_wdog_period = cfg.wdt_msec / 1000;

		printf(", watchdog interval %d sec.", wh->wh_wdog_period);
	}
	printf("\n");

	if (cfg.wdt_allow) {
		wh->wh_smw.smw_name = "weasel";
		wh->wh_smw.smw_cookie = wh;
		wh->wh_smw.smw_setmode = weasel_wdog_setmode;
		wh->wh_smw.smw_tickle = weasel_wdog_tickle;
		wh->wh_smw.smw_period = wh->wh_wdog_period;

		if (sysmon_wdog_register(&wh->wh_smw) != 0)
			printf("%s: unable to register PC-Weasel watchdog "
			    "with sysmon\n", wh->wh_parent->dv_xname);
	}
}

int
weasel_wdog_setmode(struct sysmon_wdog *smw)
{
	struct weasel_handle *wh = smw->smw_cookie;
	int error = 0;

	if ((smw->smw_mode & WDOG_MODE_MASK) == WDOG_MODE_DISARMED) {
		if (wh->wh_wdog_armed)
			error = weasel_wdog_toggle(wh);
	} else {
		if (smw->smw_period == WDOG_PERIOD_DEFAULT)
			smw->smw_period = wh->wh_wdog_period;
		else if (smw->smw_period != wh->wh_wdog_period) {
			/* Can't change the period on the Weasel. */
			return (EINVAL);
		}
		if (wh->wh_wdog_armed == 0)
			error = weasel_wdog_toggle(wh);
		else
			weasel_wdog_tickle(smw);
	}

	return (error);
}

int
weasel_wdog_tickle(struct sysmon_wdog *smw)
{
	struct weasel_handle *wh = smw->smw_cookie; 
	u_int8_t reg;
	int s;

	s = splhigh();
	reg = bus_space_read_1(wh->wh_st, wh->wh_sh, WEASEL_WDT_SEMAPHORE);
	bus_space_write_1(wh->wh_st, wh->wh_sh, WEASEL_WDT_SEMAPHORE, ~reg);
	splx(s);

	return (0);
}

int
weasel_wdog_toggle(struct weasel_handle *wh)
{
	u_int8_t reg;
	int i, s, new_state, error = 0;

	s = splhigh();

	for (i = 0, new_state = wh->wh_wdog_armed;
	     new_state == wh->wh_wdog_armed && i < 5000; i++) {
		bus_space_write_1(wh->wh_st, wh->wh_sh,
		    WEASEL_WDT_SEMAPHORE, 0x22);
		delay(1500);
		reg = bus_space_read_1(wh->wh_st, wh->wh_sh,
		    WEASEL_WDT_SEMAPHORE);
		if (reg == 0xea) {
			bus_space_write_1(wh->wh_st, wh->wh_sh,
			    WEASEL_WDT_SEMAPHORE, 0x2f);
			delay(1500);
			reg = bus_space_read_1(wh->wh_st, wh->wh_sh,
			    WEASEL_WDT_SEMAPHORE);
			if (reg == 0xae) {
				bus_space_write_1(wh->wh_st, wh->wh_sh,
				    WEASEL_WDT_SEMAPHORE, 0x37);
				delay(1500);
				new_state = bus_space_read_1(wh->wh_st,
				    wh->wh_sh, WEASEL_WDT_SEMAPHORE);
			}
		}
	}

	/* Canonicalize. */
	if (new_state)
		new_state = 1;

	if (new_state == wh->wh_wdog_armed)
		error = EIO;
	else
		wh->wh_wdog_armed = new_state;

	splx(s);

	return (error);
}
