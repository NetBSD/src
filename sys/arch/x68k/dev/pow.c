/*	$NetBSD: pow.c,v 1.17.50.1 2009/01/19 13:17:03 skrll Exp $	*/

/*
 * Copyright (c) 1995 MINOURA Makoto.
 * All rights reserved.
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
 *      This product includes software developed by Minoura Makoto.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 * Power switch device driver.
 * Useful for
 *  1. accessing boot information.
 *  2. looking at the front or external power switch.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pow.c,v 1.17.50.1 2009/01/19 13:17:03 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/signalvar.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <machine/cpu.h>
#include <machine/powioctl.h>
#include <x68k/dev/intiovar.h>
#include <x68k/dev/mfp.h>
#include <x68k/dev/powvar.h>
#include <x68k/x68k/iodevice.h>

#define sramtop (IODEVbase->io_sram)
#define rtc (IODEVbase->io_rtc)

extern struct cfdriver pow_cd;

static int  pow_match(device_t, cfdata_t, void *);
static void pow_attach(device_t, device_t, void *);
static int  powintr(void *);
static int setalarm(struct x68k_alarminfo *);
static void pow_pressed_event(void *);
static void pow_check_switch(void *);

CFATTACH_DECL_NEW(pow, sizeof(struct pow_softc),
    pow_match, pow_attach, NULL, NULL);

static int pow_attached;

dev_type_open(powopen);
dev_type_close(powclose);
dev_type_ioctl(powioctl);

const struct cdevsw pow_cdevsw = {
	powopen, powclose, noread, nowrite, powioctl,
	nostop, notty, nopoll, nommap, nokqfilter,
};

static int
pow_match(device_t parent, cfdata_t cf, void *aux)
{
	if (pow_attached)
		return 0;

	return 1;
}

static void
pow_attach(device_t parent, device_t self, void *aux)
{
	struct pow_softc *sc = device_private(self);
	struct mfp_softc *mfp = device_private(parent);

	sc->sw = ~mfp_get_gpip() & 7;
	/* disable mfp power switch interrupt */
	mfp_bit_clear_ierb(7);
	mfp_bit_clear_aer(7);

	sc->status = POW_FREE;

	if (sc->sw) {
		mfp_bit_set_aer(sc->sw);
		mfp_bit_set_ierb(sc->sw);
	}

	sysmon_task_queue_init();
	sc->smpsw.smpsw_name = device_xname(self);
	sc->smpsw.smpsw_type = PSWITCH_TYPE_POWER;
	if (sysmon_pswitch_register(&sc->smpsw) != 0)
		panic("can't register with sysmon");

	/* MFP interrupt #1 for ext, #2 for front power switch */
	if (intio_intr_establish(mfp->sc_intr + 1, "powext", powintr, sc) < 0)
		panic("%s: can't establish interrupt", device_xname(self));
	if (intio_intr_establish(mfp->sc_intr + 2, "powfrt", powintr, sc) < 0)
		panic("%s: can't establish interrupt", device_xname(self));

	shutdownhook_establish(pow_check_switch, 0);

	printf("\n");
	printf("%s: started by ", device_xname(self));
	if (sc->sw & POW_EXTERNALSW)
		printf("external power switch.\n");
	else if (sc->sw & POW_FRONTSW)
		printf("front power switch.\n");
	/* XXX: I don't know why POW_ALARMSW should not be checked */
#if 0
	else if ((sc->sw & POW_ALARMSW) && sramtop[0x26] == 0)
		printf("RTC alarm.\n");
	else
		printf("???.\n");
#else
	else
		printf("RTC alarm.\n");
#endif

	pow_attached = 1;
}


/*ARGSUSED*/
int
powopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct pow_softc *sc;
  
	sc = device_lookup_private(&pow_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->status == POW_BUSY)
		return EBUSY;

	if (sc->status == POW_FREE)
		sc->status = POW_BUSY;

	sc->rw = (flags & (FREAD|FWRITE));

	return 0;
}

/*ARGSUSED*/
int
powclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	struct pow_softc *sc;

	sc = device_lookup_private(&pow_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	if (sc->status == POW_BUSY)
		sc->status = POW_FREE;

	return 0;
}

#define SRAMINT(offset)	(*((volatile int *) (&sramtop[offset])))
#define RTCWAIT DELAY(100)

static int
setalarm(struct x68k_alarminfo *bp)
{
	int s, ontime;

	s = splclock();

	intio_set_sysport_sramwp(0x31);
	if (bp->al_enable) {
		SRAMINT(0x1e) = bp->al_dowhat;
		SRAMINT(0x22) = bp->al_ontime;
		SRAMINT(0x14) = (bp->al_offtime / 60) - 1;
		sramtop[0x26] = 0;
	} else {
		sramtop[0x26] = 7;
	}
	intio_set_sysport_sramwp(0);

	rtc.bank0.mode = 9;
	RTCWAIT;
	rtc.bank1.reset = 5;
	RTCWAIT;

	if (bp->al_enable) {
		ontime = bp->al_ontime;
		if ((ontime & 0x0f) <= 9)
			rtc.bank1.al_min = ontime & 0x0f;
		RTCWAIT;
		ontime >>= 4;
		if ((ontime & 0x0f) <= 6)
			rtc.bank1.al_min10 = ontime & 0x0f;
		RTCWAIT;
		ontime >>= 4;
		if ((ontime & 0x0f) <= 9)
			rtc.bank1.al_hour = ontime & 0x0f;
		RTCWAIT;
		ontime >>= 4;
		if ((ontime & 0x0f) <= 2)
			rtc.bank1.al_hour10 = ontime & 0x0f;
		RTCWAIT;
		ontime >>= 4;
		if ((ontime & 0x0f) <= 9)
			rtc.bank1.al_day = ontime & 0x0f;
		RTCWAIT;
		ontime >>= 4;
		if ((ontime & 0x0f) <= 3)
			rtc.bank1.al_day10 = ontime & 0x0f;
		RTCWAIT;
		ontime >>= 4;
		if ((ontime & 0x0f) <= 6)
			rtc.bank1.al_week = ontime & 0x0f;
		RTCWAIT;
		rtc.bank1.clkout = 0;
		RTCWAIT;
		rtc.bank1.mode = 0x0c;
	} else {
		rtc.bank1.clkout = 7;
		RTCWAIT;
		rtc.bank1.mode = 0x08;
	}
	splx(s);

	return 0;
}


/*ARGSUSED*/
int
powioctl(dev_t dev, u_long cmd, void *addr, int flag, struct lwp *l)
{
	struct pow_softc *sc;

	sc = device_lookup_private(&pow_cd, minor(dev));
	if (sc == NULL)
		return ENXIO;

	switch (cmd) {
	case POWIOCGPOWERINFO:
		{
			struct x68k_powerinfo *bp = (void *)addr;
			if (!(sc->rw & FREAD))
				return EBADF;
			bp->pow_switch_boottime = sc->sw;
			bp->pow_switch_current = ~mfp_get_gpip() & 7;
			bp->pow_boottime = boottime.tv_sec;
			bp->pow_bootcount = SRAMINT(0x44);
			bp->pow_usedtotal = SRAMINT(0x40) * 60;
		}
		break;

	case POWIOCGALARMINFO:
		{
			struct x68k_alarminfo *bp = (void *) addr;
			if (!(sc->rw & FREAD))
				return EBADF;
			bp->al_enable = (sramtop[0x26] == 0);
			bp->al_ontime = SRAMINT(0x22);
			bp->al_dowhat = SRAMINT(0x1e);
			bp->al_offtime = (SRAMINT(0x14) + 1) * 60;
		}
		break;

	case POWIOCSALARMINFO:
		if (!(sc->rw & FWRITE))
			return EBADF;
		return setalarm ((void *) addr);

	default:
		return EINVAL;
	}

	return 0;
}

static int
powintr(void *arg)
{
	struct pow_softc *sc = arg;
	int sw;

	sw = ~mfp_get_gpip() & 6;
	mfp_bit_clear_aer(sw);
	mfp_bit_set_ierb(sw);

	sysmon_task_queue_sched(0, pow_pressed_event, sc);

	return 0;
}

static void
pow_pressed_event(void *arg)
{
	struct pow_softc *sc = arg;

	sysmon_pswitch_event(&sc->smpsw, PSWITCH_EVENT_PRESSED);
}

static void
pow_check_switch(void *dummy)
{
	extern int power_switch_is_off;

	if ((~mfp_get_gpip() & (POW_FRONTSW | POW_EXTERNALSW)) == 0)
		power_switch_is_off = 1;
}
