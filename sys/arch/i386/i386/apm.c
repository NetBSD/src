/*	$NetBSD: apm.c,v 1.43 1999/11/10 16:55:25 drochner Exp $ */

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Kohl and Christopher G. Demetriou.
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

#include "apm.h"
#if NAPM > 1
#error only one APM device may be configured
#endif

#include "opt_apm.h"

#ifdef APM_NOIDLE
#error APM_NOIDLE option deprecated; use APM_NO_IDLE instead
#endif

#if defined(DEBUG) && !defined(APMDEBUG)
#define	APMDEBUG
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/signalvar.h>
#include <sys/kernel.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/conf.h>

#include <machine/bus.h>
#include <machine/stdarg.h>
#include <machine/cpu.h>
#include <machine/cpufunc.h>
#include <machine/gdt.h>
#include <machine/psl.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <i386/isa/nvram.h>

#include <machine/bioscall.h>
#include <machine/apmvar.h>

#if defined(APMDEBUG)
#define	DPRINTF(f, x)		do { if (apmdebug & (f)) printf x; } while (0)

#define	APMDEBUG_INFO		0x01
#define	APMDEBUG_APMCALLS	0x02
#define	APMDEBUG_EVENTS		0x04
#define	APMDEBUG_PROBE		0x10
#define	APMDEBUG_ATTACH		0x40
#define	APMDEBUG_DEVICE		0x20
#define	APMDEBUG_ANOM		0x40

#ifdef APMDEBUG_VALUE
int	apmdebug = APMDEBUG_VALUE;
#else
int	apmdebug = 0;
#endif
#else
#define	DPRINTF(f, x)		/**/
#endif

#define APM_NEVENTS 16

struct apm_softc {
	struct device sc_dev;
	struct selinfo sc_rsel;
	struct selinfo sc_xsel;
	int	sc_flags;
	int	event_count;
	int	event_ptr;
	struct	apm_event_info event_list[APM_NEVENTS];
};
#define	SCFLAG_OREAD	0x0000001
#define	SCFLAG_OWRITE	0x0000002
#define	SCFLAG_OPEN	(SCFLAG_OREAD|SCFLAG_OWRITE)

#define	APMUNIT(dev)	(minor(dev)&0xf0)
#define	APMDEV(dev)	(minor(dev)&0x0f)
#define APMDEV_NORMAL	0
#define APMDEV_CTL	8

static void	apmattach __P((struct device *, struct device *, void *));
static int	apmmatch __P((struct device *, struct cfdata *, void *));

#if 0
static void	apm_devpowmgt_enable __P((int, u_int));
static void	apm_disconnect __P((void *));
#endif
static void	apm_event_handle __P((struct apm_softc *, struct bioscallregs *));
static int	apm_get_event __P((struct bioscallregs *));
static int	apm_get_powstat __P((struct bioscallregs *));
#if 0
static void	apm_get_powstate __P((u_int));
#endif
static void	apm_periodic_check __P((void *));
static void	apm_perror __P((const char *, struct bioscallregs *, ...))
		    __kprintf_attribute__((__format__(__printf__,1,3)));
static void	apm_power_print __P((struct apm_softc *, struct bioscallregs *));
static void	apm_powmgt_enable __P((int));
static void	apm_powmgt_engage __P((int, u_int));
static int	apm_record_event __P((struct apm_softc *, u_int));
static void	apm_get_capabilities __P((void));
static void	apm_set_ver __P((struct apm_softc *));
static void	apm_standby __P((void));
static const char *apm_strerror __P((int));
static void	apm_suspend __P((void));
static void	apm_resume __P((struct apm_softc *, struct bioscallregs *));

cdev_decl(apm);

struct cfattach apm_ca = {
	sizeof(struct apm_softc), apmmatch, apmattach
};

extern struct cfdriver apm_cd;

/* configurable variables */
int	apm_bogus_bios = 0;
#ifdef APM_DISABLE
int	apm_enabled = 0;
#else
int	apm_enabled = 1;
#endif
#ifdef APM_FORCE_64K_SEGMENTS
int	apm_force_64k_segments = 1;
#else
int	apm_force_64k_segments = 0;
#endif
#ifdef APM_NO_IDLE
int	apm_do_idle = 0;
#else
int	apm_do_idle = 1;
#endif
#ifdef APM_NO_STANDBY
int	apm_do_standby = 0;
#else
int	apm_do_standby = 1;
#endif
#ifdef APM_V10_ONLY
int	apm_v11_enabled = 0;
#else
int	apm_v11_enabled = 1;
#endif
#ifdef APM_NO_V12
int	apm_v12_enabled = 0;
#else
int	apm_v12_enabled = 1;
#endif

/* variables used during operation (XXX cgd) */
struct apm_connect_info apminfo;
u_char	apm_majver, apm_minver;
int	apm_inited;
int	apm_standbys, apm_userstandbys, apm_suspends, apm_battlow;
int	apm_damn_fool_bios, apm_op_inprog;
int	apm_evindex;

#ifdef APMDEBUG
int	apmcall_debug(int, struct bioscallregs *, int);

int
apmcall_debug(func, regs, line)
	int func;
	struct bioscallregs *regs;
	int line;
{
	int rv;

	DPRINTF(APMDEBUG_APMCALLS, ("apmcall: func %d from line %d",
	    func, line));
    	rv = apmcall(func, regs);
	DPRINTF(APMDEBUG_APMCALLS, (" -> 0x%x\n", rv));
	return (rv);
}

#define apmcall(f, r)	apmcall_debug((f), (r), __LINE__)
#endif


static const char *
apm_strerror(code)
	int code;
{
	switch (code) {
	case APM_ERR_PM_DISABLED:
		return ("power management disabled");
	case APM_ERR_REALALREADY:
		return ("real mode interface already connected");
	case APM_ERR_NOTCONN:
		return ("interface not connected");
	case APM_ERR_16ALREADY:
		return ("16-bit interface already connected");
	case APM_ERR_16NOTSUPP:
		return ("16-bit interface not supported");
	case APM_ERR_32ALREADY:
		return ("32-bit interface already connected");
	case APM_ERR_32NOTSUPP:
		return ("32-bit interface not supported");
	case APM_ERR_UNRECOG_DEV:
		return ("unrecognized device ID");
	case APM_ERR_ERANGE:
		return ("parameter out of range");
	case APM_ERR_NOTENGAGED:
		return ("interface not engaged");
	case APM_ERR_UNABLE:
		return ("unable to enter requested state");
	case APM_ERR_NOEVENTS:
		return ("no pending events");
	case APM_ERR_NOT_PRESENT:
		return ("no APM present");
	default:
		return ("unknown error code");
	}
}

static void
apm_perror(const char *str, struct bioscallregs *regs, ...) /* XXX cgd */
{
	va_list ap;

	va_start(ap, regs);
	printf("APM %:: %s (0x%x)\n", str, ap, /* XXX cgd */
	    apm_strerror(APM_ERR_CODE(regs)), regs->AX);
	va_end(ap);
}

static void
apm_power_print(sc, regs)
	struct apm_softc *sc;
	struct bioscallregs *regs;
{

	if (APM_BATT_LIFE(regs) != APM_BATT_LIFE_UNKNOWN) {
		printf("%s: battery life expectancy: %d%%\n",
		    sc->sc_dev.dv_xname, APM_BATT_LIFE(regs));
	}
	printf("%s: A/C state: ", sc->sc_dev.dv_xname);
	switch (APM_AC_STATE(regs)) {
	case APM_AC_OFF:
		printf("off\n");
		break;
	case APM_AC_ON:
		printf("on\n");
		break;
	case APM_AC_BACKUP:
		printf("backup power\n");
		break;
	default:
	case APM_AC_UNKNOWN:
		printf("unknown\n");
		break;
	}
	printf("%s: battery charge state:", sc->sc_dev.dv_xname);
	if (apm_minver == 0)
		switch (APM_BATT_STATE(regs)) {
		case APM_BATT_HIGH:
			printf("high\n");
			break;
		case APM_BATT_LOW:
			printf("low\n");
			break;
		case APM_BATT_CRITICAL:
			printf("critical\n");
			break;
		case APM_BATT_CHARGING:
			printf("charging\n");
			break;
		case APM_BATT_UNKNOWN:
			printf("unknown\n");
			break;
		default:
			printf("undecoded state %x\n", APM_BATT_STATE(regs));
			break;
		}
	else if (apm_minver >= 1) {
		if (APM_BATT_FLAGS(regs) & APM_BATT_FLAG_NO_SYSTEM_BATTERY)
			printf(" no battery");
		else {
			if (APM_BATT_FLAGS(regs) & APM_BATT_FLAG_HIGH)
				printf(" high");
			if (APM_BATT_FLAGS(regs) & APM_BATT_FLAG_LOW)
				printf(" low");
			if (APM_BATT_FLAGS(regs) & APM_BATT_FLAG_CRITICAL)
				printf(" critical");
			if (APM_BATT_FLAGS(regs) & APM_BATT_FLAG_CHARGING)
				printf(" charging");
		}
		printf("\n");
		if (APM_BATT_REM_VALID(regs)) {
			printf("%s: estimated ", sc->sc_dev.dv_xname);
			if (APM_BATT_REMAINING(regs) / 60)
				printf("%dh ", APM_BATT_REMAINING(regs) / 60);
			printf("%dm\n", APM_BATT_REMAINING(regs) % 60);
		}
	}
	return;
}

#if 0 /* currently unused */
static void
apm_get_powstate(dev)
	u_int dev;
{
	struct bioscallregs regs;
	int rval;

	regs.BX = dev;
	rval = apmcall(APM_GET_POWER_STATE, &regs);
	if (rval == 0) { /* XXX cgd */
		printf("apm dev %04x state %04x\n", dev, regs.CX);
	}
}
#endif

static void
apm_suspend()
{
	dopowerhooks(PWR_SUSPEND);

	/* XXX cgd */
	(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_SYS_SUSPEND);
}

static void
apm_standby()
{
	dopowerhooks(PWR_STANDBY);

	/* XXX cgd */
	(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_SYS_STANDBY);
}

static void
apm_resume(sc, regs)
	struct apm_softc *sc;
	struct bioscallregs *regs;
{

	/*
	 * Some system requires its clock to be initialized after hybernation.
	 */
	initrtclock();

	inittodr(time.tv_sec);
	dopowerhooks(PWR_RESUME);
	apm_record_event(sc, regs->BX);
}

/*
 * return 0 if the user will notice and handle the event,
 * return 1 if the kernel driver should do so.
 */
static int
apm_record_event(sc, event_type)
	struct apm_softc *sc;
	u_int event_type;
{
	struct apm_event_info *evp;

	if ((sc->sc_flags & SCFLAG_OPEN) == 0)
		return 1;		/* no user waiting */
	if (sc->event_count == APM_NEVENTS)
		return 1;			/* overflow */
	evp = &sc->event_list[sc->event_ptr];
	sc->event_count++;
	sc->event_ptr++;
	sc->event_ptr %= APM_NEVENTS;
	evp->type = event_type;
	evp->index = ++apm_evindex;
	selwakeup(&sc->sc_rsel);
	return (sc->sc_flags & SCFLAG_OWRITE) ? 0 : 1; /* user may handle */
}

static void
apm_event_handle(sc, regs)
	struct apm_softc *sc;
	struct bioscallregs *regs;
{
	int error;
	struct bioscallregs nregs;
	char *code;

	switch (regs->BX) {
	case APM_USER_STANDBY_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: user standby request\n"));
		if (apm_do_standby) {
			if (apm_record_event(sc, regs->BX))
				apm_userstandbys++;
			apm_op_inprog++;
			(void)apm_set_powstate(APM_DEV_ALLDEVS,
			    APM_LASTREQ_INPROG);
		} else {
			(void)apm_set_powstate(APM_DEV_ALLDEVS,
			    APM_LASTREQ_REJECTED);
			/* in case BIOS hates being spurned */
			apm_powmgt_enable(1);
		}
		break;

	case APM_STANDBY_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: system standby request\n"));
		if (apm_standbys || apm_suspends) {
			DPRINTF(APMDEBUG_EVENTS | APMDEBUG_ANOM,
			    ("damn fool BIOS did not wait for answer\n"));
			/* just give up the fight */
			apm_damn_fool_bios = 1;
		}
		if (apm_do_standby) {
			if (apm_record_event(sc, regs->BX))
				apm_standbys++;
			apm_op_inprog++;
			(void)apm_set_powstate(APM_DEV_ALLDEVS,
			    APM_LASTREQ_INPROG);
		} else {
			(void)apm_set_powstate(APM_DEV_ALLDEVS,
			    APM_LASTREQ_REJECTED);
			/* in case BIOS hates being spurned */
			apm_powmgt_enable(1);
		}
		break;

	case APM_USER_SUSPEND_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: user suspend request\n"));
		if (apm_record_event(sc, regs->BX))
			apm_suspends++;
		apm_op_inprog++;
		(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_LASTREQ_INPROG);
		break;

	case APM_SUSPEND_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: system suspend request\n"));
		if (apm_standbys || apm_suspends) {
			DPRINTF(APMDEBUG_EVENTS | APMDEBUG_ANOM,
			    ("damn fool BIOS did not wait for answer\n"));
			/* just give up the fight */
			apm_damn_fool_bios = 1;
		}
		if (apm_record_event(sc, regs->BX))
			apm_suspends++;
		apm_op_inprog++;
		(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_LASTREQ_INPROG);
		break;

	case APM_POWER_CHANGE:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: power status change\n"));
		error = apm_get_powstat(&nregs);
#ifndef APM_NO_POWER_PRINT
		/* only print if nobody is catching events. */
		if (error == 0 &&
		    (sc->sc_flags & (SCFLAG_OREAD|SCFLAG_OWRITE)) == 0)
			apm_power_print(sc, &nregs);
#endif
		apm_record_event(sc, regs->BX);
		break;

	case APM_NORMAL_RESUME:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: resume system\n"));
		apm_resume(sc, regs);
		break;

	case APM_CRIT_RESUME:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: critical resume system"));
		apm_resume(sc, regs);
		break;

	case APM_SYS_STANDBY_RESUME:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: system standby resume\n"));
		apm_resume(sc, regs);
		break;

	case APM_UPDATE_TIME:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: update time\n"));
		apm_resume(sc, regs);
		break;

	case APM_CRIT_SUSPEND_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: critical system suspend\n"));
		apm_record_event(sc, regs->BX);
		apm_suspend();
		break;

	case APM_BATTERY_LOW:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: battery low\n"));
		apm_battlow++;
		apm_record_event(sc, regs->BX);
		break;

	case APM_CAP_CHANGE:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: capability change\n"));
		if (apm_minver < 2) {
			DPRINTF(APMDEBUG_EVENTS, ("apm: unexpected event\n"));
		} else {
			apm_get_capabilities();
			apm_get_powstat(&nregs); /* XXX */
		}
		break;

	default:
		switch (regs->BX >> 8) {
			case 0:
				code = "reserved system";
				break;
			case 1:
				code = "reserved device";
				break;
			case 2:
				code = "OEM defined";
				break;
			default:
				code = "reserved";
				break;
		}	
		printf("APM: %s event code %x\n", code, regs->BX);
	}
}

static int
apm_get_event(regs)
	struct bioscallregs *regs;
{

	return (apmcall(APM_GET_PM_EVENT, regs));
}

static void
apm_periodic_check(arg)
	void *arg;
{
	struct bioscallregs regs;
	struct apm_softc *sc = arg;

	/*
	 * tell the BIOS we're working on it, if asked to do a
	 * suspend/standby
	 */
	if (apm_op_inprog)
		apm_set_powstate(APM_DEV_ALLDEVS, APM_LASTREQ_INPROG);

	while (apm_get_event(&regs) == 0 && !apm_damn_fool_bios)
		apm_event_handle(sc, &regs);

	if (APM_ERR_CODE(&regs) != APM_ERR_NOEVENTS)
		apm_perror("get event", &regs);
	if (apm_suspends) {
		apm_op_inprog = 0;
		apm_suspend();
	} else if (apm_standbys || apm_userstandbys) {
		apm_op_inprog = 0;
		apm_standby();
	}
	apm_suspends = apm_standbys = apm_battlow = apm_userstandbys = 0;
	apm_damn_fool_bios = 0;
	timeout(apm_periodic_check, sc, hz);
}

static void
apm_powmgt_enable(onoff)
	int onoff;
{
	struct bioscallregs regs;

	regs.BX = apm_minver == 0 ? APM_MGT_ALL : APM_DEV_ALLDEVS;
	regs.CX = onoff ? APM_MGT_ENABLE : APM_MGT_DISABLE;
	if (apmcall(APM_PWR_MGT_ENABLE, &regs) != 0)
		apm_perror("power management enable all <%s>", &regs,
		    onoff ? "enable" : "disable");
}

static void
apm_powmgt_engage(onoff, dev)
	int onoff;
	u_int dev;
{
	struct bioscallregs regs;

	if (apm_minver == 0)
		return;
	regs.BX = dev;
	regs.CX = onoff ? APM_MGT_ENGAGE : APM_MGT_DISENGAGE;
	if (apmcall(APM_PWR_MGT_ENGAGE, &regs) != 0)
		apm_perror("power mgmt engage (device %x)\n", &regs, dev);
}

#if 0
static void
apm_devpowmgt_enable(onoff, dev)
	int onoff;
	u_int dev;
{
	struct bioscallregs regs;

	if (apm_minver == 0)
	    return;
	regs.BX = dev;

	/*
	 * enable is auto BIOS managment.
	 * disable is program control.
	 */
	regs.CX = onoff ? APM_MGT_ENABLE : APM_MGT_DISABLE;
	if (apmcall(APM_DEVICE_MGMT_ENABLE, &regs) != 0)
		printf("APM device engage (device %x): %s (%d)\n",
		    dev, apm_strerror(APM_ERR_CODE(&regs)),
		    APM_ERR_CODE(&regs));
}
#endif

int
apm_set_powstate(dev, state)
	u_int dev, state;
{
	struct bioscallregs regs;
	if (!apm_inited || (apm_minver == 0 && state > APM_SYS_OFF))
		return EINVAL;

	regs.BX = dev;
	regs.CX = state;
	if (apmcall(APM_SET_PWR_STATE, &regs) != 0) {
		apm_perror("set power state <%x,%x>", &regs, dev, state);
		return EIO;
	}
	return 0;
}

void
apm_cpu_busy()
{
	struct bioscallregs regs;

	if (!apm_inited || !apm_do_idle)
	    return;
	if ((apminfo.apm_detail & APM_IDLE_SLOWS) &&
	    apmcall(APM_CPU_BUSY, &regs) != 0) {
		/*
		 * XXX BIOSes use to set carry without valid
		 * error number
		 */
#ifdef APMDEBUG
		apm_perror("set CPU busy", &regs);
#endif
	}
}

void
apm_cpu_idle()
{
	struct bioscallregs regs;

	if (!apm_inited || !apm_do_idle)
	    return;
	if (apmcall(APM_CPU_IDLE, &regs) != 0) {
		/*
		 * XXX BIOSes use to set carry without valid
		 * error number
		 */
#ifdef APMDEBUG
		apm_perror("set CPU idle", &regs);
#endif
	}
}

/* V1.2 */
static void
apm_get_capabilities()
{
	struct bioscallregs regs;

	regs.BX = APM_DEV_APM_BIOS;
	if (apmcall(APM_GET_CAPABILITIES, &regs) != 0) {
		apm_perror("get capabilities", &regs);
		return;
	}

#ifdef APMDEBUG
	/* print out stats */
	printf("apm: %d batteries", APM_NBATTERIES(&regs));
	if (regs.CX & APM_GLOBAL_STANDBY)
	    printf(", global standby");
	if (regs.CX & APM_GLOBAL_SUSPEND)
	    printf(", global suspend");
	if (regs.CX & APM_RTIMER_STANDBY)
	    printf(", rtimer standby");
	if (regs.CX & APM_RTIMER_SUSPEND)
	    printf(", rtimer suspend");
	if (regs.CX & APM_IRRING_STANDBY)
	    printf(", internal standby");
	if (regs.CX & APM_IRRING_SUSPEND)
	    printf(", internal suspend");
	if (regs.CX & APM_PCRING_STANDBY)
	    printf(", pccard standby");
	if (regs.CX & APM_PCRING_SUSPEND)
	    printf(", pccard suspend");
	printf("\n");
#endif
}

static void
apm_set_ver(self)
	struct apm_softc *self;
{
	struct bioscallregs regs;
	int error;

	regs.CX = 0x0102;	/* APM Version 1.2 */
	regs.BX = APM_DEV_APM_BIOS;
	
	if (apm_v12_enabled &&
	    (error = apmcall(APM_DRIVER_VERSION, &regs)) == 0) {
		apm_majver = 1;
		apm_minver = 2;
		goto ok;
	}

	regs.CX = 0x0101;	/* APM Version 1.1 */
	regs.BX = APM_DEV_APM_BIOS;
	
	if (apm_v11_enabled &&
	    (error = apmcall(APM_DRIVER_VERSION, &regs)) == 0) {
		apm_majver = 1;
		apm_minver = 1;
	} else {
		apm_majver = 1;
		apm_minver = 0;
	}
ok:
	printf("Power Management spec V%d.%d", apm_majver, apm_minver);
	apm_inited = 1;
	if (apminfo.apm_detail & APM_IDLE_SLOWS) {
#ifdef DIAGNOSTIC
		/* not relevant often */
		printf(" (slowidle)");
#endif
		/* leave apm_do_idle at its user-configured setting */
	} else
		apm_do_idle = 0;
#ifdef DIAGNOSTIC
	if (apminfo.apm_detail & APM_BIOS_PM_DISABLED)
		printf(" (BIOS mgmt disabled)");
	if (apminfo.apm_detail & APM_BIOS_PM_DISENGAGED)
		printf(" (BIOS managing devices)");
#endif
}

static int
apm_get_powstat(regs)
	struct bioscallregs *regs;
{

	regs->BX = APM_DEV_ALLDEVS;
	return apmcall(APM_POWER_STATUS, regs);
}

#if 0
static void
apm_disconnect(xxx)
	void *xxx;
{
	struct bioscallregs regs;

	regs.BX = apm_minver == 1 ? APM_DEV_ALLDEVS : APM_DEFAULTS_ALL;
	if (apmcall(APM_SYSTEM_DEFAULTS, &regs))
		apm_perror("system defaults (%s) failed", &regs,
		    apm_minver == 1 ? "alldevs" : "all");

	regs.BX = APM_DEV_APM_BIOS;
	if (apmcall(APM_DISCONNECT, &regs))
		apm_perror("disconnect failed", &regs);
	else
		printf("APM disconnected\n");
}
#endif

/* XXX cgd: this doesn't belong here. */
#define I386_FLAGBITS "\020\017NT\014OVFL\0130UP\012IEN\011TF\010NF\007ZF\005AF\003PF\001CY"

int
apm_busprobe()
{
	struct bioscallregs regs;
#ifdef APMDEBUG
	char bits[128];
#endif

	regs.AX = APM_BIOS_FN(APM_INSTALLATION_CHECK);
	regs.BX = APM_DEV_APM_BIOS;
	regs.CX = regs.DX = 0;
	regs.ESI = regs.EDI = regs.EFLAGS = 0;
	bioscall(APM_SYSTEM_BIOS, &regs);
	DPRINTF(APMDEBUG_PROBE, ("apm: bioscall return: %x %x %x %x %s %x %x\n",
	    regs.AX, regs.BX, regs.CX, regs.DX,
	    bitmask_snprintf(regs.EFLAGS, I386_FLAGBITS, bits, sizeof(bits)),
	    regs.ESI, regs.EDI));

	if (regs.FLAGS & PSL_C) {
		DPRINTF(APMDEBUG_PROBE, ("apm: carry set means no APM bios\n"));
		return 0;	/* no carry -> not installed */
	}
	if (regs.BX != APM_INSTALL_SIGNATURE) {
		DPRINTF(APMDEBUG_PROBE, ("apm: PM signature not found\n"));
		return 0;
	}
	if ((regs.CX & APM_32BIT_SUPPORT) == 0) {
		DPRINTF(APMDEBUG_PROBE, ("apm: no 32bit support (busprobe)\n"));
		return 0;
	}

	return 1;  /* OK to continue probe & complain if something fails */
}

static int
apmmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct apm_attach_args *aaa = aux;

	/* These are not the droids you're looking for. */
	if (strcmp(aaa->aaa_busname, "apm") != 0)
		return (0);

	/* There can be only one! */
	if (apm_inited)
		return 0;

	/*
	 * apm_busprobe() said 'go' or we wouldn't be here.
	 * APM might not be useful (or might be too weird)
	 * on this machine, but that's handled in attach.
	 *
	 * The apm_enabled global variable is used to allow
	 * users to patch kernels to disable APM support.
	 */
	return (apm_enabled);
}

#define	DPRINTF_BIOSRETURN(regs, bits)					\
	DPRINTF(APMDEBUG_ATTACH,					\
	    ("bioscall return: %x %x %x %x %s %x %x",			\
	    (regs).EAX, (regs).EBX, (regs).ECX, (regs).EDX,		\
	    bitmask_snprintf((regs).EFLAGS, I386_FLAGBITS,		\
	    (bits), sizeof(bits)), (regs).ESI, (regs).EDI))

static void
apmattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	extern int biosbasemem;
	struct apm_softc *apmsc = (void *)self;
	struct bioscallregs regs;
	int error, apm_data_seg_ok;
	u_int okbases[] = { 0, biosbasemem*1024 };
	u_int oklimits[] = { NBPG, IOM_END};
	u_int i;
#ifdef APMDEBUG
	char bits[128];
#endif

	printf(": ");

	regs.AX = APM_BIOS_FN(APM_INSTALLATION_CHECK);
	regs.BX = APM_DEV_APM_BIOS;
	regs.CX = regs.DX = regs.SI = regs.DI = regs.FLAGS = 0;
	bioscall(APM_SYSTEM_BIOS, &regs);
	DPRINTF_BIOSRETURN(regs, bits);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));

	apminfo.apm_detail = (u_int)regs.AX | ((u_int)regs.CX << 16);

	/*
	 * call a disconnect in case it was already connected
	 * by some previous code.
	 */
	regs.AX = APM_BIOS_FN(APM_DISCONNECT);
	regs.BX = APM_DEV_APM_BIOS;
	regs.CX = regs.DX = regs.SI = regs.DI = regs.FLAGS = 0;
	bioscall(APM_SYSTEM_BIOS, &regs);
	DPRINTF_BIOSRETURN(regs, bits);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));

	if ((apminfo.apm_detail & APM_32BIT_SUPPORTED) == 0) {
		printf("no 32-bit APM support");
		goto bail_disconnected;
	}

	/*
	 * And connect to it.
	 */
	regs.AX = APM_BIOS_FN(APM_32BIT_CONNECT);
	regs.BX = APM_DEV_APM_BIOS;
	regs.CX = regs.DX = regs.DI = regs.FLAGS = 0;
	regs.ESI = 0;
	bioscall(APM_SYSTEM_BIOS, &regs);
	DPRINTF_BIOSRETURN(regs, bits);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));

	apminfo.apm_code32_seg_base = regs.AX << 4;
	apminfo.apm_entrypt = regs.BX; /* spec says EBX, can't map >=64k */
	apminfo.apm_code16_seg_base = regs.CX << 4;
	apminfo.apm_data_seg_base = regs.DX << 4;
	apminfo.apm_code32_seg_len = regs.SI;
	apminfo.apm_code16_seg_len = regs.SI_HI;
	apminfo.apm_data_seg_len = regs.DI;


	if (apm_force_64k_segments) {
		apminfo.apm_code32_seg_len = 65536;
		apminfo.apm_code16_seg_len = 65536;
		apminfo.apm_data_seg_len = 65536;
	} else {
		switch ((APM_MAJOR_VERS(apminfo.apm_detail) << 8) +
			APM_MINOR_VERS(apminfo.apm_detail)) {
		case 0x0100:
			apminfo.apm_code32_seg_len = 65536;
			apminfo.apm_code16_seg_len = 65536;
			apminfo.apm_data_seg_len = 65536;
			apm_v11_enabled = 0;
			apm_v12_enabled = 0;
			break;
		case 0x0101:
			apminfo.apm_code16_seg_len = apminfo.apm_code32_seg_len;
			apm_v12_enabled = 0;
			/* fall through */
		case 0x0102:
		default:
			if (apminfo.apm_code32_seg_len == 0) {
				/*
				 * some BIOSes are lame, even if v1.1.
				 * (Or maybe they want 64k even though they can
				 * only ask for 64k-1?)
				 */
				apminfo.apm_code32_seg_len = 65536;
				DPRINTF(APMDEBUG_ATTACH,
				    ("lame v%d.%d bios gave zero len code32, pegged to 64k\n%s: ",
				    APM_MAJOR_VERS(apminfo.apm_detail),
				    APM_MINOR_VERS(apminfo.apm_detail),
				    apmsc->sc_dev.dv_xname));
			}
			if (apminfo.apm_code16_seg_len == 0) {
				/*
				 * some BIOSes are lame, even if v1.1.
				 * (Or maybe they want 64k even though they can
				 * only ask for 64k-1?)
				 */
				apminfo.apm_code16_seg_len = 65536;
				DPRINTF(APMDEBUG_ATTACH,
				    ("lame v%d.%d bios gave zero len code16, pegged to 64k\n%s: ",
				    APM_MAJOR_VERS(apminfo.apm_detail),
				    APM_MINOR_VERS(apminfo.apm_detail),
				    apmsc->sc_dev.dv_xname));
			}
			if (apminfo.apm_data_seg_len == 0) {
				/*
				 * some BIOSes are lame, even if v1.1.
				 *
				 * leave it alone and assume it does not
				 * want any sensible data segment
				 * mapping, and mark as bogus (but with
				 * expanded size, in case it's in some place
				 * that costs us nothing to map).
				 */
				apm_bogus_bios = 1;
				apminfo.apm_data_seg_len = 65536;
				DPRINTF(APMDEBUG_ATTACH,
				    ("lame v%d.%d bios gave zero len data, tentative 64k\n%s: ",
				    APM_MAJOR_VERS(apminfo.apm_detail),
				    APM_MINOR_VERS(apminfo.apm_detail),
				    apmsc->sc_dev.dv_xname));
			}
			break;
		}
	}
	if (apminfo.apm_code32_seg_len < apminfo.apm_entrypt + 4) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("nonsensical BIOS code length %d ignored (entry point offset is %d)\n%s: ",
		    apminfo.apm_code32_seg_len,
		    apminfo.apm_entrypt,
		    apmsc->sc_dev.dv_xname));
		apminfo.apm_code32_seg_len = 65536;
	}
	if (apminfo.apm_code32_seg_base < IOM_BEGIN ||
	    apminfo.apm_code32_seg_base >= IOM_END) {
		DPRINTF(APMDEBUG_ATTACH, ("code32 segment starts outside ISA hole [%x]\n%s: ",
		    apminfo.apm_code32_seg_base, apmsc->sc_dev.dv_xname));
		printf("bogus 32-bit code segment start");
		goto bail;
	} 
	if (apminfo.apm_code32_seg_base +
	    apminfo.apm_code32_seg_len > IOM_END) {
		DPRINTF(APMDEBUG_ATTACH, ("code32 segment oversized: [%x,%x)\n%s: ",
		    apminfo.apm_code32_seg_base,
		    apminfo.apm_code32_seg_base + apminfo.apm_code32_seg_len - 1,
		    apmsc->sc_dev.dv_xname));
#if 0
		printf("bogus 32-bit code segment size");
		goto bail;
#else
		apminfo.apm_code32_seg_len =
		    IOM_END - apminfo.apm_code32_seg_base;
#endif
	}
	if (apminfo.apm_code16_seg_base < IOM_BEGIN ||
	    apminfo.apm_code16_seg_base >= IOM_END) {
		DPRINTF(APMDEBUG_ATTACH, ("code16 segment starts outside ISA hole [%x]\n%s: ",
		    apminfo.apm_code16_seg_base, apmsc->sc_dev.dv_xname));
		printf("bogus 16-bit code segment start");
		goto bail;
	}
	if (apminfo.apm_code16_seg_base +
	    apminfo.apm_code16_seg_len > IOM_END) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("code16 segment oversized: [%x,%x), giving up\n%s: ",
		    apminfo.apm_code16_seg_base,
		    apminfo.apm_code16_seg_base + apminfo.apm_code16_seg_len - 1,
		    apmsc->sc_dev.dv_xname));
		/*
		 * give up since we may have to trash the
		 * 32bit segment length otherwise.
		 */
		printf("bogus 16-bit code segment size");
		goto bail;
	}
	/*
	 * allow data segment to be zero length, within ISA hole or
	 * at page zero or above biosbasemem and below ISA hole end.
	 * truncate it if it doesn't quite fit in the space
	 * we allow.
	 *
	 * Otherwise, give up if not "apm_bogus_bios".
	 */
	apm_data_seg_ok = 0;
	for (i = 0; i < 2; i++) {
		if (apminfo.apm_data_seg_base >= okbases[i] &&
		    apminfo.apm_data_seg_base < oklimits[i]-1) {
			/* starts OK */
			if (apminfo.apm_data_seg_base +
			    apminfo.apm_data_seg_len > oklimits[i]) {
				DPRINTF(APMDEBUG_ATTACH,
				    ("data segment oversized: [%x,%x)",
				    apminfo.apm_data_seg_base,
				    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len));
				apminfo.apm_data_seg_len =
				    oklimits[i] - apminfo.apm_data_seg_base;
				DPRINTF(APMDEBUG_ATTACH,
				    ("; resized to [%x,%x)\n%s: ",
				    apminfo.apm_data_seg_base,
				    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len,
				    apmsc->sc_dev.dv_xname));
			} else {
				DPRINTF(APMDEBUG_ATTACH,
				    ("data segment fine: [%x,%x)\n%s: ",
				    apminfo.apm_data_seg_base,
				    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len,
				    apmsc->sc_dev.dv_xname));
			}
			apm_data_seg_ok = 1;
			break;
		}
	}
	if (!apm_data_seg_ok && apm_bogus_bios) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("bogus bios data seg location, ignoring\n%s: ",
		    apmsc->sc_dev.dv_xname));
		apminfo.apm_data_seg_base = 0;
		apminfo.apm_data_seg_len = 0;
		apm_data_seg_ok = 1;		/* who are we kidding?! */
	}
	if (!apm_data_seg_ok) {
		DPRINTF(APMDEBUG_ATTACH,
		    ("data segment [%x,%x) not in an available location\n%s: ",
		    apminfo.apm_data_seg_base,
		    apminfo.apm_data_seg_base + apminfo.apm_data_seg_len,
		    apmsc->sc_dev.dv_xname));
		printf("data segment unavailable");
		goto bail;
	}

	/*
	 * set up GDT descriptors for APM
	 */
	apminfo.apm_segsel = GSEL(GAPM32CODE_SEL,SEL_KPL);

	/*
	 * Some bogus APM V1.1 BIOSes do not return any
	 * size limits in the registers they are supposed to.
	 * We forced them to zero before calling the BIOS
	 * (see apm_init.S), so if we see zero limits here
	 * we assume that means they should be 64k (and trimmed
	 * if needed for legitimate memory needs).
	 */
	DPRINTF(APMDEBUG_ATTACH, ("code32len=%x, datalen=%x\n%s: ",
	    apminfo.apm_code32_seg_len,
	    apminfo.apm_data_seg_len,
	    apmsc->sc_dev.dv_xname));
	setsegment(&gdt[GAPM32CODE_SEL].sd,
	    ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
	    apminfo.apm_code32_seg_len - 1,
	    SDT_MEMERA, SEL_KPL, 1, 0);
	setsegment(&gdt[GAPM16CODE_SEL].sd,
	    ISA_HOLE_VADDR(apminfo.apm_code16_seg_base),
	    apminfo.apm_code16_seg_len - 1,
	    SDT_MEMERA, SEL_KPL, 0, 0);
	if (apminfo.apm_data_seg_len == 0) {
		/*
		 *if no data area needed, set up the segment
		 * descriptor to just the first byte of the code
		 * segment, read only.
		 */
		setsegment(&gdt[GAPMDATA_SEL].sd,
		    ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
		    0, SDT_MEMROA, SEL_KPL, 0, 0);
	} else if (apminfo.apm_data_seg_base < IOM_BEGIN) {
		bus_space_handle_t memh;

		/*
		 * need page zero or biosbasemem area mapping.
		 *
		 * XXX cheat and use knowledge of bus_space_map()
		 * implementation on i386 so it can be done without
		 * extent checking.
		 */
		if (_i386_memio_map(I386_BUS_SPACE_MEM,
		    apminfo.apm_data_seg_base,
		    apminfo.apm_data_seg_len, 0, &memh)) {
			printf("couldn't map data segment");
			goto bail;
		}
		DPRINTF(APMDEBUG_ATTACH,
		    ("mapping bios data area %x @ 0x%lx\n%s: ",
		    apminfo.apm_data_seg_base, memh,
		    apmsc->sc_dev.dv_xname));
		setsegment(&gdt[GAPMDATA_SEL].sd,
		    (void *)memh,
		    apminfo.apm_data_seg_len - 1,
		    SDT_MEMRWA, SEL_KPL, 1, 0);
	} else
		setsegment(&gdt[GAPMDATA_SEL].sd,
		    ISA_HOLE_VADDR(apminfo.apm_data_seg_base),
		    apminfo.apm_data_seg_len - 1,
		    SDT_MEMRWA, SEL_KPL, 1, 0);

	DPRINTF(APMDEBUG_ATTACH,
	    ("detail %x 32b:%x/%p/%x 16b:%x/%p/%x data %x/%p/%x ep %x (%x:%p) %p\n%s: ",
	    apminfo.apm_detail,
	    apminfo.apm_code32_seg_base,
	    ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
	    apminfo.apm_code32_seg_len,
	    apminfo.apm_code16_seg_base,
	    ISA_HOLE_VADDR(apminfo.apm_code16_seg_base),
	    apminfo.apm_code16_seg_len,
	    apminfo.apm_data_seg_base,
	    ISA_HOLE_VADDR(apminfo.apm_data_seg_base),
	    apminfo.apm_data_seg_len,
	    apminfo.apm_entrypt,
	    apminfo.apm_segsel,
	    apminfo.apm_entrypt +
	     (char *)ISA_HOLE_VADDR(apminfo.apm_code32_seg_base),
	    &apminfo.apm_segsel,
	    apmsc->sc_dev.dv_xname));

	apm_set_ver(apmsc);		/* prints version info */
	printf("\n");
	if (apm_minver >= 2)
		apm_get_capabilities();

	/*
	 * enable power management if it's disabled.
	 */

	/*
	 * XXX some bogus APM BIOSes don't set the disabled bit in
	 * the connect state, yet still complain about the functions
	 * being disabled when other calls are made.  sigh.
	 */
	if (apminfo.apm_detail & APM_BIOS_PM_DISABLED)
		apm_powmgt_enable(1);

	/*
	 * Engage cooperative power mgt (we get to do it)
	 * on all devices (v1.1).
	 */
	apm_powmgt_engage(1, APM_DEV_ALLDEVS);
#if 0
	/* doesn't seem to work, sigh. */
	apm_powmgt_engage(1, APM_DEV_DISPLAY(APM_DEV_ALLUNITS));
	apm_powmgt_engage(1, APM_DEV_DISK(APM_DEV_ALLUNITS));
	apm_powmgt_engage(1, APM_DEV_PARALLEL(APM_DEV_ALLUNITS));
	apm_powmgt_engage(1, APM_DEV_NETWORK(APM_DEV_ALLUNITS));
	apm_powmgt_engage(1, APM_DEV_PCMCIA(APM_DEV_ALLUNITS));
#endif
	memset(&regs, 0, sizeof(regs));
	error = apm_get_powstat(&regs);
	if (error == 0) {
		apm_power_print(apmsc, &regs);
	} else
		apm_perror("get power status", &regs);
	apm_cpu_busy();
	apm_periodic_check(apmsc);

	return;

bail:
	/*
	 * call a disconnect; we're punting.
	 */
	regs.AX = APM_BIOS_FN(APM_DISCONNECT);
	regs.BX = APM_DEV_APM_BIOS;
	regs.CX = regs.DX = regs.SI = regs.DI = regs.FLAGS = 0;
	bioscall(APM_SYSTEM_BIOS, &regs);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));
	DPRINTF_BIOSRETURN(regs, bits);
bail_disconnected:
	printf("\n%s: kernel APM support disabled\n", apmsc->sc_dev.dv_xname);
}

#undef DPRINTF_BIOSRETURN

int
apmopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = APMUNIT(dev);
	int ctl = APMDEV(dev);
	struct apm_softc *sc;

	if (unit >= apm_cd.cd_ndevs)
		return ENXIO;
	sc = apm_cd.cd_devs[unit];
	if (!sc)
		return ENXIO;

	if (!apm_inited)
		return ENXIO;
	
	DPRINTF(APMDEBUG_DEVICE,
	    ("apmopen: pid %d flag %x mode %x\n", p->p_pid, flag, mode));

	switch (ctl) {
	case APMDEV_CTL:
		if (!(flag & FWRITE))
			return EINVAL;
		if (sc->sc_flags & SCFLAG_OWRITE)
			return EBUSY;
		sc->sc_flags |= SCFLAG_OWRITE;
		break;
	case APMDEV_NORMAL:
		if (!(flag & FREAD) || (flag & FWRITE))
			return EINVAL;
		sc->sc_flags |= SCFLAG_OREAD;
		break;
	default:
		return ENXIO;
		break;
	}
	return 0;
}

int
apmclose(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	int ctl = APMDEV(dev);

	DPRINTF(APMDEBUG_DEVICE,
	    ("apmclose: pid %d flag %x mode %x\n", p->p_pid, flag, mode));

	switch (ctl) {
	case APMDEV_CTL:
		sc->sc_flags &= ~SCFLAG_OWRITE;
		break;
	case APMDEV_NORMAL:
		sc->sc_flags &= ~SCFLAG_OREAD;
		break;
	}
	if ((sc->sc_flags & SCFLAG_OPEN) == 0) {
		sc->event_count = 0;
		sc->event_ptr = 0;
	}
	return 0;
}

int
apmioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	struct apm_power_info *powerp;
	struct apm_event_info *evp;
	struct bioscallregs regs;
#if 0
	struct apm_ctl *actl;
#endif
	int i;

	switch (cmd) {
	case APM_IOC_STANDBY:
		if (!apm_do_standby)
			return (EOPNOTSUPP);

		if ((flag & FWRITE) == 0)
			return (EBADF);
		apm_userstandbys++;
		return (0);

	case APM_IOC_SUSPEND:
		if ((flag & FWRITE) == 0)
			return (EBADF);
		apm_suspends++;
		return (0);

#if 0 /* is this used at all? */
	case APM_IOC_DEV_CTL:
		actl = (struct apm_ctl *)data;
		if ((flag & FWRITE) == 0)
			return (EBADF);
		apm_get_powstate(actl->dev); /* XXX */
		return (apm_set_powstate(actl->dev, actl->mode));
#endif

	case APM_IOC_NEXTEVENT:
		if (!sc->event_count)
			return (EAGAIN);
		else {
			evp = (struct apm_event_info *)data;
			i = sc->event_ptr + APM_NEVENTS - sc->event_count;
			i %= APM_NEVENTS;
			*evp = sc->event_list[i];
			sc->event_count--;
			return (0);
		}

	case APM_IOC_GETPOWER:
		powerp = (struct apm_power_info *)data;
		if (apm_get_powstat(&regs)) {
			apm_perror("ioctl get power status", &regs);
			return (EIO);
		}

		memset(powerp, 0, sizeof(*powerp));
		if (APM_BATT_LIFE(&regs) != APM_BATT_LIFE_UNKNOWN)
			powerp->battery_life = APM_BATT_LIFE(&regs);
		powerp->ac_state = APM_AC_STATE(&regs);
		switch (apm_minver) {
		case 0:
			powerp->battery_state = APM_BATT_STATE(&regs);
			break;
		case 1:
		default:
			powerp->battery_state = APM_BATT_UNKNOWN;
			if (APM_BATT_FLAGS(&regs) & APM_BATT_FLAG_HIGH)
				powerp->battery_state = APM_BATT_HIGH;
			else if (APM_BATT_FLAGS(&regs) & APM_BATT_FLAG_LOW)
				powerp->battery_state = APM_BATT_LOW;
			else if (APM_BATT_FLAGS(&regs) & APM_BATT_FLAG_CRITICAL)
				powerp->battery_state = APM_BATT_CRITICAL;
			else if (APM_BATT_FLAGS(&regs) & APM_BATT_FLAG_CHARGING)
				powerp->battery_state = APM_BATT_CHARGING;
			else if (APM_BATT_FLAGS(&regs) & APM_BATT_FLAG_NO_SYSTEM_BATTERY)
				powerp->battery_state = APM_BATT_ABSENT;
			if (APM_BATT_REM_VALID(&regs))
				powerp->minutes_left =
				    APM_BATT_REMAINING(&regs);
		}
		return (0);
		
	default:
		return (ENOTTY);
	}
}

int
apmpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	int revents = 0;

	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}

	return (revents);
}
