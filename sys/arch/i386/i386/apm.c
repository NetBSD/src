/*	$NetBSD: apm.c,v 1.60.2.4 2001/09/13 01:13:45 thorpej Exp $ */

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
#include "opt_compat_mach.h"	/* Needed to get the right segment def */

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
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/user.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/conf.h>

#include <uvm/uvm_extern.h>

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

#else	/* APMDEBUG */
#define	DPRINTF(f, x)		/**/
#endif	/* APMDEBUG */

#define APM_NEVENTS 16

struct apm_softc {
	struct device sc_dev;
	struct selinfo sc_rsel;
	int	sc_flags;
	int	event_count;
	int	event_ptr;
	int	sc_power_state;
	int 	sc_err_count;
	int	sc_err_type;
	struct proc *sc_thread;
	struct lock sc_lock;
	struct	apm_event_info event_list[APM_NEVENTS];
};
#define	SCFLAG_OREAD	0x0000001
#define	SCFLAG_OWRITE	0x0000002
#define	SCFLAG_OPEN	(SCFLAG_OREAD|SCFLAG_OWRITE)

#define	APMUNIT(dev)	(minor(dev)&0xf0)
#define	APMDEV(dev)	(minor(dev)&0x0f)
#define APMDEV_NORMAL	0
#define APMDEV_CTL	8

/*
 * A brief note on the locking protocol: it's very simple; we
 * assert an exclusive lock any time thread context enters the
 * APM module.  This is both the APM thread itself, as well as
 * user context.
 */
#define	APM_LOCK(apmsc)							\
	(void) lockmgr(&(apmsc)->sc_lock, LK_EXCLUSIVE, NULL)
#define	APM_UNLOCK(apmsc)						\
	(void) lockmgr(&(apmsc)->sc_lock, LK_RELEASE, NULL)

static void	apmattach __P((struct device *, struct device *, void *));
static int	apmmatch __P((struct device *, struct cfdata *, void *));

#if 0
static void	apm_devpowmgt_enable __P((int, u_int));
static void	apm_disconnect __P((void *));
#endif
static int	apm_event_handle __P((struct apm_softc *, struct bioscallregs *));
static int	apm_get_event __P((struct bioscallregs *));
static int	apm_get_powstat __P((struct bioscallregs *, u_int));
static void	apm_get_powstate __P((u_int));
static int	apm_periodic_check __P((struct apm_softc *));
static void	apm_create_thread __P((void *));
static void	apm_thread __P((void *));
static void	apm_perror __P((const char *, struct bioscallregs *, ...))
		    __attribute__((__format__(__printf__,1,3)));
#ifdef APM_POWER_PRINT
static void	apm_power_print __P((struct apm_softc *, struct bioscallregs *));
#endif
static void	apm_powmgt_enable __P((int));
static void	apm_powmgt_engage __P((int, u_int));
static int	apm_record_event __P((struct apm_softc *, u_int));
static void	apm_get_capabilities __P((struct bioscallregs *));
static void	apm_set_ver __P((struct apm_softc *));
static void	apm_standby __P((struct apm_softc *));
static const char *apm_strerror __P((int));
static void	apm_suspend __P((struct apm_softc *));
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
#ifdef APM_ALLOW_BOGUS_SEGMENTS
int	apm_allow_bogus_segments = 1;
#else
int	apm_allow_bogus_segments = 0;
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
int	apm_evindex;

/* set if we should standby/suspend at the end of next periodic event */
int apm_standby_now, apm_suspend_now;

/* 
 * set if kernel is planning on doing a standby/suspend, or if we are
 * waiting for an external program to process a standby/suspend event.
 */
int apm_standby_pending, apm_suspend_pending;

static int apm_spl;		/* saved spl while suspended */

#ifdef APMDEBUG
int	apmcall_debug(int, struct bioscallregs *, int);
static	void acallpr(int, char *, struct bioscallregs *);

/* bitmask defns for printing apm call args/results */
#define ACPF_AX		0x00000001
#define ACPF_AX_HI	0x00000002
#define ACPF_EAX	0x00000004
#define ACPF_BX 	0x00000008
#define ACPF_BX_HI 	0x00000010
#define ACPF_EBX 	0x00000020
#define ACPF_CX 	0x00000040
#define ACPF_CX_HI 	0x00000080
#define ACPF_ECX 	0x00000100
#define ACPF_DX 	0x00000200
#define ACPF_DX_HI 	0x00000400
#define ACPF_EDX 	0x00000800
#define ACPF_SI 	0x00001000
#define ACPF_SI_HI 	0x00002000
#define ACPF_ESI 	0x00004000
#define ACPF_DI 	0x00008000
#define ACPF_DI_HI 	0x00010000
#define ACPF_EDI 	0x00020000
#define ACPF_FLAGS 	0x00040000
#define ACPF_FLAGS_HI	0x00080000
#define ACPF_EFLAGS 	0x00100000

struct acallinfo {
	char *name;
	int inflag;
	int outflag;
};

static struct acallinfo aci[] = {
  { "install_check", ACPF_BX, ACPF_AX|ACPF_BX|ACPF_CX },
  { "connectreal", ACPF_BX, 0 },
  { "connect16", ACPF_BX, ACPF_AX|ACPF_BX|ACPF_CX|ACPF_SI|ACPF_DI },
  { "connect32", ACPF_BX, ACPF_AX|ACPF_EBX|ACPF_CX|ACPF_DX|ACPF_ESI|ACPF_DI },
  { "disconnect", ACPF_BX, 0 },
  { "cpu_idle", 0, 0 },
  { "cpu_busy", 0, 0 },
  { "set_power_state", ACPF_BX|ACPF_CX, 0 },
  { "enable_power_state", ACPF_BX|ACPF_CX, 0 }, 
  { "restore_defaults", ACPF_BX, 0 },
  { "get_power_status", ACPF_BX, ACPF_BX|ACPF_CX|ACPF_DX|ACPF_SI },
  { "get_event", 0, ACPF_BX|ACPF_CX },
  { "get_power_state" , ACPF_BX, ACPF_CX }, 
  { "enable_dev_power_mgt", ACPF_BX|ACPF_CX, 0 },
  { "driver_version", ACPF_BX|ACPF_CX, ACPF_AX },
  { "engage_power_mgt",  ACPF_BX|ACPF_CX, 0 },
  { "get_caps", ACPF_BX, ACPF_BX|ACPF_CX },
  { "resume_timer", ACPF_BX|ACPF_CX|ACPF_SI|ACPF_DI, ACPF_CX|ACPF_SI|ACPF_DI },
  { "resume_ring", ACPF_BX|ACPF_CX, ACPF_CX },
  { "timer_reqs", ACPF_BX|ACPF_CX, ACPF_CX },
};

static void acallpr(int flag, char *tag, struct bioscallregs *b) {
  if (!flag) return;
  printf("%s ", tag);
  if (flag & ACPF_AX) 		printf("ax=%#x ", b->AX);
  if (flag & ACPF_AX_HI) 	printf("ax_hi=%#x ", b->AX_HI);
  if (flag & ACPF_EAX) 		printf("eax=%#x ", b->EAX);
  if (flag & ACPF_BX ) 		printf("bx=%#x ", b->BX);
  if (flag & ACPF_BX_HI ) 	printf("bx_hi=%#x ", b->BX_HI);
  if (flag & ACPF_EBX ) 	printf("ebx=%#x ", b->EBX);
  if (flag & ACPF_CX ) 		printf("cx=%#x ", b->CX);
  if (flag & ACPF_CX_HI ) 	printf("cx_hi=%#x ", b->CX_HI);
  if (flag & ACPF_ECX ) 	printf("ecx=%#x ", b->ECX);
  if (flag & ACPF_DX ) 		printf("dx=%#x ", b->DX);
  if (flag & ACPF_DX_HI ) 	printf("dx_hi=%#x ", b->DX_HI);
  if (flag & ACPF_EDX ) 	printf("edx=%#x ", b->EDX);
  if (flag & ACPF_SI ) 		printf("si=%#x ", b->SI);
  if (flag & ACPF_SI_HI ) 	printf("si_hi=%#x ", b->SI_HI);
  if (flag & ACPF_ESI ) 	printf("esi=%#x ", b->ESI);
  if (flag & ACPF_DI ) 		printf("di=%#x ", b->DI);
  if (flag & ACPF_DI_HI ) 	printf("di_hi=%#x ", b->DI_HI);
  if (flag & ACPF_EDI ) 	printf("edi=%#x ", b->EDI);
  if (flag & ACPF_FLAGS ) 	printf("flags=%#x ", b->FLAGS);
  if (flag & ACPF_FLAGS_HI) 	printf("flags_hi=%#x ", b->FLAGS_HI);
  if (flag & ACPF_EFLAGS ) 	printf("eflags=%#x ", b->EFLAGS);
}

int
apmcall_debug(func, regs, line)
	int func;
	struct bioscallregs *regs;
	int line;
{
	int rv;
	int print = (apmdebug & APMDEBUG_APMCALLS) != 0;
	char *name;
	int inf, outf;
		
	if (print) {
		if (func >= sizeof(aci) / sizeof(aci[0])) {
			name = 0;
			inf = outf = 0;
		} else {
			name = aci[func].name;
			inf = aci[func].inflag;
			outf = aci[func].outflag;
		}
		inittodr(time.tv_sec);	/* update timestamp */
		if (name)
			printf("apmcall@%03ld: %s/%#x (line=%d) ", 
				time.tv_sec % 1000, name, func, line);
		else
			printf("apmcall@%03ld: %#x (line=%d) ", 
				time.tv_sec % 1000, func, line);
		acallpr(inf, "in:", regs);
	}
    	rv = apmcall(func, regs);
	if (print) {
		if (rv) {
			printf(" => error %#x (%s)\n", regs->AX >> 8,
				apm_strerror(regs->AX >> 8));
		} else {
			printf(" => ");
			acallpr(outf, "out:", regs);
			printf("\n");
		}
	}
	return (rv);
}

#define apmcall(f, r)	apmcall_debug((f), (r), __LINE__)
#endif	/* APMDEBUG */


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

	printf("APM ");

	va_start(ap, regs);
	vprintf(str, ap);			/* XXX cgd */
	va_end(ap);

	printf(": %s (0x%x)\n", apm_strerror(APM_ERR_CODE(regs)), regs->AX);
}

#ifdef APM_POWER_PRINT
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
#endif

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

static void
apm_suspend(sc)
	struct apm_softc *sc;
{

	if (sc->sc_power_state == PWR_SUSPEND) {
#ifdef APMDEBUG
		printf("%s: apm_suspend: already suspended?\n",
		    sc->sc_dev.dv_xname);
#endif
		return;
	}
	sc->sc_power_state = PWR_SUSPEND;

	dopowerhooks(PWR_SOFTSUSPEND);

	apm_spl = splhigh();

	dopowerhooks(PWR_SUSPEND);

	/* XXX cgd */
	(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_SYS_SUSPEND);
}

static void
apm_standby(sc)
	struct apm_softc *sc;
{

	if (sc->sc_power_state == PWR_STANDBY) {
#ifdef APMDEBUG
		printf("%s: apm_standby: already standing by?\n",
		    sc->sc_dev.dv_xname);
#endif
		return;
	}
	sc->sc_power_state = PWR_STANDBY;

	dopowerhooks(PWR_SOFTSTANDBY);

	apm_spl = splhigh();

	dopowerhooks(PWR_STANDBY);

	/* XXX cgd */
	(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_SYS_STANDBY);
}

static void
apm_resume(sc, regs)
	struct apm_softc *sc;
	struct bioscallregs *regs;
{

	if (sc->sc_power_state == PWR_RESUME) {
#ifdef APMDEBUG
		printf("%s: apm_resume: already running?\n",
		    sc->sc_dev.dv_xname);
#endif
		return;
	}
	sc->sc_power_state = PWR_RESUME;

	/*
	 * Some system requires its clock to be initialized after hybernation.
	 */
	initrtclock();

	inittodr(time.tv_sec);
	dopowerhooks(PWR_RESUME);

	splx(apm_spl);

	dopowerhooks(PWR_SOFTRESUME);

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
	if (sc->event_count == APM_NEVENTS) {
		DPRINTF(APMDEBUG_ANOM, ("apm_record_event: queue full!\n"));
		return 1;			/* overflow */
	}
	evp = &sc->event_list[sc->event_ptr];
	sc->event_count++;
	sc->event_ptr++;
	sc->event_ptr %= APM_NEVENTS;
	evp->type = event_type;
	evp->index = ++apm_evindex;
	selnotify(&sc->sc_rsel, 0);
	return (sc->sc_flags & SCFLAG_OWRITE) ? 0 : 1; /* user may handle */
}

/*
 * apm_event_handle: handle an event.  returns 1 if event handled, 0 if
 * event is a duplicate of an event we are already handling.
 */
static int
apm_event_handle(sc, regs)
	struct apm_softc *sc;
	struct bioscallregs *regs;
{
	int error, retval;
	struct bioscallregs nregs;
	char *code;

	retval = 1;		/* assume we are going to make progress */

	switch (regs->BX) {
	case APM_USER_STANDBY_REQ:
	case APM_STANDBY_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: %s standby request\n",
		  (regs->BX == APM_STANDBY_REQ) ? "system" : "user"));
		if (apm_do_standby) {
			if (apm_standby_pending)
				retval = 0;		/* duplicate request */
			else {
				if (apm_record_event(sc, regs->BX))
					apm_standby_now++; /* kernel handles */
				apm_standby_pending++;
			}
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
	case APM_SUSPEND_REQ:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: %s suspend request\n",
		  (regs->BX == APM_SUSPEND_REQ) ? "system" : "user"));
		if (apm_suspend_pending)
			retval = 0;		/* duplicate request */
		else {
			if (apm_record_event(sc, regs->BX))
				apm_suspend_now++;	/* kernel handles */
			apm_suspend_pending++;
		}
		(void)apm_set_powstate(APM_DEV_ALLDEVS, APM_LASTREQ_INPROG);
		break;

	case APM_POWER_CHANGE:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: power status change\n"));
		error = apm_get_powstat(&nregs, 0);
#ifdef APM_POWER_PRINT
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
		apm_suspend(sc);
		break;

	case APM_BATTERY_LOW:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: battery low\n"));
		apm_record_event(sc, regs->BX);
		break;

	case APM_CAP_CHANGE:
		DPRINTF(APMDEBUG_EVENTS, ("apmev: capability change\n"));
		if (apm_minver < 2) {
			DPRINTF(APMDEBUG_EVENTS, ("apm: unexpected event\n"));
		} else {
			apm_get_capabilities(&nregs);
			apm_get_powstat(&nregs, 0); /* XXX */
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
	return(retval);
}

static int
apm_get_event(regs)
	struct bioscallregs *regs;
{

	return (apmcall(APM_GET_PM_EVENT, regs));
}

static int
apm_periodic_check(sc)
	struct apm_softc *sc;
{
	struct bioscallregs regs;

	/*
	 * if we are waiting for user (apmd) to process a suspend or 
	 * standby tell the BIOS we are working on it.
	 */
	if (apm_standby_pending || apm_suspend_pending)
		apm_set_powstate(APM_DEV_ALLDEVS, APM_LASTREQ_INPROG);

	/*
	 * continue processing events until we run out or we get a
	 * duplicate.  duplicates occur on some APM BIOS (e.g. IBM
	 * thinkpad) where it keeps posting the standby/suspend event
	 * until forward progress is made.
	 */
	while (1) {
		if (apm_get_event(&regs) != 0) {	/* out of events? */
			if (APM_ERR_CODE(&regs) != APM_ERR_NOEVENTS) {
				apm_perror("get event", &regs);
				if (sc->sc_err_type == regs.AX) {
				     	if (sc->sc_err_count++ >=
					    APM_ERR_LIMIT) {
						printf(
			"apm: Last error 0x%x occurred %d times; giving up.\n",
						    sc->sc_err_type,
						    APM_ERR_LIMIT);
						return -1;
					}
				} else {
					sc->sc_err_count = 0;
					sc->sc_err_type = regs.AX;
				}
			}
			break;
		}
		sc->sc_err_type = 0;
		sc->sc_err_count = 0;
		if (!apm_event_handle(sc, & regs)) {
			DPRINTF(APMDEBUG_EVENTS | APMDEBUG_ANOM,
			  ("apm_periodic_check: duplicate event (break)\n"));
			break;
		}
	}

	if (apm_suspend_now) {
		apm_suspend_pending = 0;
		apm_suspend(sc);
	} else if (apm_standby_now) {
		apm_standby_pending = 0;
		apm_standby(sc);
	}

	/* reset for next loop */
	apm_suspend_now = apm_standby_now = 0;
	return 0;
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
		apm_perror("power mgmt engage (device %x)", &regs, dev);
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
apm_get_capabilities(regs)
	struct bioscallregs *regs;
{

	regs->BX = APM_DEV_APM_BIOS;
	if (apmcall(APM_GET_CAPABILITIES, regs) != 0) {
		apm_perror("get capabilities", regs);
		return;
	}

#ifdef APMDEBUG
	/* print out stats */
	DPRINTF(APMDEBUG_INFO, ("apm: %d batteries", APM_NBATTERIES(regs)));
	if (regs->CX & APM_GLOBAL_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", global standby"));
	if (regs->CX & APM_GLOBAL_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", global suspend"));
	if (regs->CX & APM_RTIMER_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", rtimer standby"));
	if (regs->CX & APM_RTIMER_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", rtimer suspend"));
	if (regs->CX & APM_IRRING_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", internal standby"));
	if (regs->CX & APM_IRRING_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", internal suspend"));
	if (regs->CX & APM_PCRING_STANDBY)
	    DPRINTF(APMDEBUG_INFO, (", pccard standby"));
	if (regs->CX & APM_PCRING_SUSPEND)
	    DPRINTF(APMDEBUG_INFO, (", pccard suspend"));
	DPRINTF(APMDEBUG_INFO, ("\n"));
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
apm_get_powstat(regs, batteryid)
	struct bioscallregs *regs;
	u_int batteryid;
{

	if (batteryid == 0)
		regs->BX = APM_DEV_ALLDEVS;
	else
		regs->BX = APM_DEV_BATTERY(batteryid);
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
	struct apm_softc *apmsc = (void *)self;
	struct bioscallregs regs;
	int error, apm_data_seg_ok;
	u_int okbases[] = { 0, biosbasemem*1024 };
	u_int oklimits[] = { PAGE_SIZE, IOM_END};
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

	apmsc->sc_err_type = 0;
	apmsc->sc_err_count = 0;

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
		if (apm_allow_bogus_segments) {
			DPRINTF(APMDEBUG_ATTACH,
			    ("bogus bios data seg location, continuing\n%s: ",
			    apmsc->sc_dev.dv_xname));
		} else {
			DPRINTF(APMDEBUG_ATTACH,
			    ("bogus bios data seg location, ignoring\n%s: ",
			    apmsc->sc_dev.dv_xname));
			apminfo.apm_data_seg_base = 0;
			apminfo.apm_data_seg_len = 0;
		}
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
#ifdef GAPM16CODE_SEL
	setsegment(&gdt[GAPM16CODE_SEL].sd,
	    ISA_HOLE_VADDR(apminfo.apm_code16_seg_base),
	    apminfo.apm_code16_seg_len - 1,
	    SDT_MEMERA, SEL_KPL, 0, 0);
#endif
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
		apm_get_capabilities(&regs);

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
	error = apm_get_powstat(&regs, 0);
	if (error == 0) {
#ifdef APM_POWER_PRINT
		apm_power_print(apmsc, &regs);
#endif
	} else
		apm_perror("get power status", &regs);
	apm_cpu_busy();

	lockinit(&apmsc->sc_lock, PWAIT, "apmlk", 0, 0);

	/* Initial state is `resumed'. */
	apmsc->sc_power_state = PWR_RESUME;

	/* Do an initial check. */
	(void)apm_periodic_check(apmsc);

	/*
	 * Create a kernel thread to periodically check for APM events,
	 * and notify other subsystems when they occur.
	 */
	kthread_create(apm_create_thread, apmsc);

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

void
apm_create_thread(arg)
	void *arg;
{
	struct apm_softc *apmsc = arg;
	struct bioscallregs regs;
#ifdef APMDEBUG
	char bits[128];
#endif

	if (kthread_create1(apm_thread, apmsc, &apmsc->sc_thread,
	    "%s", apmsc->sc_dev.dv_xname) == 0)
		return;

	/*
	 * We were unable to create the APM thread; bail out.
	 */
	regs.AX = APM_BIOS_FN(APM_DISCONNECT);
	regs.BX = APM_DEV_APM_BIOS;
	regs.CX = regs.DX = regs.SI = regs.DI = regs.FLAGS = 0;
	bioscall(APM_SYSTEM_BIOS, &regs);
	DPRINTF(APMDEBUG_ATTACH, ("\n%s: ", apmsc->sc_dev.dv_xname));
	DPRINTF_BIOSRETURN(regs, bits);
	printf("%s: unable to create thread, kernel APM support disabled\n",
	    apmsc->sc_dev.dv_xname);
}

#undef DPRINTF_BIOSRETURN

void
apm_thread(arg)
	void *arg;
{
	struct apm_softc *apmsc = arg;
	int error;

	/*
	 * Loop forever, doing a periodic check for APM events.
	 */
	for (;;) {
		APM_LOCK(apmsc);
		error = apm_periodic_check(apmsc);
		APM_UNLOCK(apmsc);
		if (error != 0)
			kthread_exit(0);
		(void) tsleep(apmsc, PWAIT, "apmev",  (8 * hz) / 7);
	}
}

int
apmopen(dev, flag, mode, p)
	dev_t dev;
	int flag, mode;
	struct proc *p;
{
	int unit = APMUNIT(dev);
	int ctl = APMDEV(dev);
	int error = 0;
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

	APM_LOCK(sc);
	switch (ctl) {
	case APMDEV_CTL:
		if (!(flag & FWRITE)) {
			error = EINVAL;
			break;
		}
		if (sc->sc_flags & SCFLAG_OWRITE) {
			error = EBUSY;
			break;
		}
		sc->sc_flags |= SCFLAG_OWRITE;
		break;
	case APMDEV_NORMAL:
		if (!(flag & FREAD) || (flag & FWRITE)) {
			error = EINVAL;
			break;
		}
		sc->sc_flags |= SCFLAG_OREAD;
		break;
	default:
		error = ENXIO;
		break;
	}
	APM_UNLOCK(sc);

	return (error);
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

	APM_LOCK(sc);
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
	APM_UNLOCK(sc);
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
	struct apm_ctl *actl;
	int i, error = 0;
	u_int batteryid, nbattery;

	APM_LOCK(sc);
	switch (cmd) {
	case APM_IOC_STANDBY:
		if (!apm_do_standby) {
			error = EOPNOTSUPP;
			break;
		}

		if ((flag & FWRITE) == 0) {
			error = EBADF;
			break;
		}
		apm_standby_now++;	/* flag for periodic event */ 
		break;

	case APM_IOC_SUSPEND:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			break;
		}
		apm_suspend_now++;	/* flag for peroidic event */
		break;

	case APM_IOC_DEV_CTL:
		actl = (struct apm_ctl *)data;
		if ((flag & FWRITE) == 0) {
			error = EBADF;
			break;
		}
		apm_get_powstate(actl->dev); /* XXX */
		error = apm_set_powstate(actl->dev, actl->mode);
		break;

	case APM_IOC_NEXTEVENT:
		if (!sc->event_count)
			error = EAGAIN;
		else {
			evp = (struct apm_event_info *)data;
			i = sc->event_ptr + APM_NEVENTS - sc->event_count;
			i %= APM_NEVENTS;
			*evp = sc->event_list[i];
			sc->event_count--;
		}
		break;

	case OAPM_IOC_GETPOWER:
	case APM_IOC_GETPOWER:
		powerp = (struct apm_power_info *)data;
		if (cmd == OAPM_IOC_GETPOWER)
			batteryid = 0;
		else
			batteryid = powerp->batteryid;
		if (apm_minver >= 2) {
			apm_get_capabilities(&regs);
			if (batteryid > APM_NBATTERIES(&regs)) {
				error = EIO;
				break;
			}
			nbattery = APM_NBATTERIES(&regs);
		} else {
			if (batteryid > 0) {
				error = EIO;
				break;
			}
			nbattery = 0;
		}
		if (apm_get_powstat(&regs, batteryid)) {
			apm_perror("ioctl get power status", &regs);
			error = EIO;
			break;
		}

		memset(powerp, 0, sizeof(*powerp));
		powerp->batteryid = batteryid;
		powerp->nbattery = nbattery;
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
		break;
		
	default:
		error = ENOTTY;
	}
	APM_UNLOCK(sc);

	return (error);
}

int
apmpoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	int revents = 0;

	APM_LOCK(sc);
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(p, &sc->sc_rsel);
	}
	APM_UNLOCK(sc);

	return (revents);
}

static void
filt_apmrdetach(struct knote *kn)
{
	struct apm_softc *sc = (void *) kn->kn_hook;

	APM_LOCK(sc);
	SLIST_REMOVE(&sc->sc_rsel.si_klist, kn, knote, kn_selnext);
	APM_UNLOCK(sc);
}

static int
filt_apmread(struct knote *kn, long hint)
{
	struct apm_softc *sc = (void *) kn->kn_hook;

	kn->kn_data = sc->event_count;
	return (kn->kn_data > 0);
}

static const struct filterops apmread_filtops =
	{ 1, NULL, filt_apmrdetach, filt_apmread };

int
apmkqfilter(dev_t dev, struct knote *kn)
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.si_klist;
		kn->kn_fop = &apmread_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = (void *) sc;

	APM_LOCK(sc);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	APM_UNLOCK(sc);

	return (0);
}
