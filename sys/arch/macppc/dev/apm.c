/*	$NetBSD: apm.c,v 1.17.4.1 2007/12/13 21:54:49 bouyer Exp $	*/
/*	$OpenBSD: apm.c,v 1.5 2002/06/07 07:13:59 miod Exp $	*/

/*-
 * Copyright (c) 2001 Alexander Guy.  All rights reserved.
 * Copyright (c) 1998-2001 Michael Shalayeff. All rights reserved.
 * Copyright (c) 1995 John T. Kohl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the authors nor the names of contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: apm.c,v 1.17.4.1 2007/12/13 21:54:49 bouyer Exp $");

#include "apm.h"

#if NAPM > 1
#error only one APM emulation device may be configured
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mutex.h>
#ifdef __OpenBSD__
#include <sys/event.h>
#endif
#ifdef __NetBSD__
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/conf.h>
#endif

#ifdef __OpenBSD__
#include <machine/conf.h>
#endif
#include <machine/cpu.h>
#include <machine/apmvar.h>

#include <macppc/dev/adbvar.h>
#include <macppc/dev/pm_direct.h>

#if defined(APMDEBUG)
#define DPRINTF(x)	printf x
#else
#define	DPRINTF(x)	/**/
#endif

#define APM_NEVENTS 16

struct apm_softc {
	struct device sc_dev;
	struct selinfo sc_rsel;
#ifdef __OpenBSD__
	struct klist sc_note;
#endif
	int    sc_flags;
	int	event_count;
	int	event_ptr;
	kmutex_t sc_lock;
	struct	apm_event_info event_list[APM_NEVENTS];
};

/*
 * A brief note on the locking protocol: it's very simple; we
 * assert an exclusive lock any time thread context enters the
 * APM module.  This is both the APM thread itself, as well as
 * user context.
 */
#ifdef __NetBSD__
#define	APM_LOCK(apmsc)		mutex_enter(&(apmsc)->sc_lock)
#define	APM_UNLOCK(apmsc)	mutex_exit(&(apmsc)->sc_lock)
#else
#define APM_LOCK(apmsc)
#define APM_UNLOCK(apmsc)
#endif

int apmmatch(struct device *, struct cfdata *, void *);
void apmattach(struct device *, struct device *, void *);

#ifdef __NetBSD__
#if 0
static int	apm_record_event __P((struct apm_softc *, u_int));
#endif
#endif

CFATTACH_DECL(apm, sizeof(struct apm_softc),
    apmmatch, apmattach, NULL, NULL);

#ifdef __OpenBSD__
struct cfdriver apm_cd = {
	NULL, "apm", DV_DULL
};
#else
extern struct cfdriver apm_cd;

dev_type_open(apmopen);
dev_type_close(apmclose);
dev_type_ioctl(apmioctl);
dev_type_poll(apmpoll);
dev_type_kqfilter(apmkqfilter);

const struct cdevsw apm_cdevsw = {
	apmopen, apmclose, noread, nowrite, apmioctl,
	nostop, notty, apmpoll, nommap, apmkqfilter,
};
#endif

int	apm_evindex;

#define	APMUNIT(dev)	(minor(dev)&0xf0)
#define	APMDEV(dev)	(minor(dev)&0x0f)
#define APMDEV_NORMAL	0
#define APMDEV_CTL	8

/*
 * Flags to control kernel display
 *	SCFLAG_NOPRINT:		do not output APM power messages due to
 *				a power change event.
 *
 *	SCFLAG_PCTPRINT:	do not output APM power messages due to
 *				to a power change event unless the battery
 *				percentage changes.
 */

#define SCFLAG_NOPRINT	0x0008000
#define SCFLAG_PCTPRINT	0x0004000
#define SCFLAG_PRINT	(SCFLAG_NOPRINT|SCFLAG_PCTPRINT)

#define	SCFLAG_OREAD 	(1 << 0)
#define	SCFLAG_OWRITE	(1 << 1)
#define	SCFLAG_OPEN	(SCFLAG_OREAD|SCFLAG_OWRITE)


int
apmmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct adb_attach_args *aa = (void *)aux;		
	if (aa->origaddr != ADBADDR_APM ||
	    aa->handler_id != ADBADDR_APM ||
	    aa->adbaddr != ADBADDR_APM)
		return 0;

	if (adbHardware != ADB_HW_PMU)
		return 0;

	return 1;
}

void
apmattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct apm_softc *sc = (struct apm_softc *) self;
	struct pmu_battery_info info;

	pm_battery_info(0, &info);

	printf(": battery flags 0x%X, ", info.flags);
	printf("%d%% charged\n", ((info.cur_charge * 100) / info.max_charge));

	sc->sc_flags = 0;
	sc->event_ptr = 0;
	sc->event_count = 0;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
}

int
apmopen(dev, flag, mode, l)
	dev_t dev;
	int flag, mode;
	struct lwp *l;
{
	struct apm_softc *sc;
	int error = 0;

	/* apm0 only */
	if (!apm_cd.cd_ndevs || APMUNIT(dev) != 0 ||
	    !(sc = apm_cd.cd_devs[APMUNIT(dev)]))
		return ENXIO;

	DPRINTF(("apmopen: dev %d pid %d flag %x mode %x\n",
	    APMDEV(dev), l->l_proc->p_pid, flag, mode));

	APM_LOCK(sc);
	switch (APMDEV(dev)) {
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
	return error;
}

int
apmclose(dev, flag, mode, l)
	dev_t dev;
	int flag, mode;
	struct lwp *l;
{
	struct apm_softc *sc;

	/* apm0 only */
	if (!apm_cd.cd_ndevs || APMUNIT(dev) != 0 ||
	    !(sc = apm_cd.cd_devs[APMUNIT(dev)]))
		return ENXIO;

	DPRINTF(("apmclose: pid %d flag %x mode %x\n", l->l_proc->p_pid, flag, mode));

	APM_LOCK(sc);
	switch (APMDEV(dev)) {
	case APMDEV_CTL:
		sc->sc_flags &= ~SCFLAG_OWRITE;
		break;
	case APMDEV_NORMAL:
		sc->sc_flags &= ~SCFLAG_OREAD;
		break;
	}
	APM_UNLOCK(sc);
	return 0;
}

int
apmioctl(dev, cmd, data, flag, l)
	dev_t dev;
	u_long cmd;
	void *data;
	int flag;
	struct lwp *l;
{
	struct apm_softc *sc;
	struct pmu_battery_info batt;
	struct apm_power_info *power;
	int error = 0;

	/* apm0 only */
	if (!apm_cd.cd_ndevs || APMUNIT(dev) != 0 ||
	    !(sc = apm_cd.cd_devs[APMUNIT(dev)]))
		return ENXIO;

	APM_LOCK(sc);
	switch (cmd) {
		/* some ioctl names from linux */
	case APM_IOC_STANDBY:
		if ((flag & FWRITE) == 0)
			error = EBADF;
	case APM_IOC_SUSPEND:
		if ((flag & FWRITE) == 0)
			error = EBADF;			
		break;
	case APM_IOC_PRN_CTL:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		else {
			int op = *(int *)data;
			DPRINTF(( "APM_IOC_PRN_CTL: %d\n", op ));
			switch (op) {
			case APM_PRINT_ON:	/* enable printing */
				sc->sc_flags &= ~SCFLAG_PRINT;
				break;
			case APM_PRINT_OFF: /* disable printing */
				sc->sc_flags &= ~SCFLAG_PRINT;
				sc->sc_flags |= SCFLAG_NOPRINT;
				break;
			case APM_PRINT_PCT: /* disable some printing */
				sc->sc_flags &= ~SCFLAG_PRINT;
				sc->sc_flags |= SCFLAG_PCTPRINT;
				break;
			default:
				error = EINVAL;
				break;
			}
		}
		break;
	case APM_IOC_DEV_CTL:
		if ((flag & FWRITE) == 0)
			error = EBADF;
		break;
	case APM_IOC_GETPOWER:
	        power = (struct apm_power_info *)data;

		pm_battery_info(0, &batt);

		power->ac_state = ((batt.flags & PMU_PWR_AC_PRESENT) ?
		    APM_AC_ON : APM_AC_OFF);
		power->battery_life =
		    ((batt.cur_charge * 100) / batt.max_charge);

		/*
		 * If the battery is charging, return the minutes left until
		 * charging is complete. apmd knows this.
		 */

		if (!(batt.flags & PMU_PWR_BATT_PRESENT)) {
			power->battery_state = APM_BATT_UNKNOWN;
			power->minutes_left = 0;
			power->battery_life = 0;
		} else if ((power->ac_state == APM_AC_ON) &&
			   (batt.draw > 0)) {
			power->minutes_left = batt.secs_remaining / 60;
			power->battery_state = APM_BATT_CHARGING;
		} else {
			power->minutes_left = batt.secs_remaining / 60;

			/* XXX - Arbitrary */
			if (power->battery_life > 60) {
				power->battery_state = APM_BATT_HIGH;
			} else if (power->battery_life < 10) {
				power->battery_state = APM_BATT_CRITICAL;
			} else {
				power->battery_state = APM_BATT_LOW;
			}
		}

		break;
		
	default:
		error = ENOTTY;
	}
	APM_UNLOCK(sc);

	return error;
}

#ifdef __NetBSD__
#if 0
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
		DPRINTF(("apm_record_event: queue full!\n"));
		return 1;			/* overflow */
	}
	evp = &sc->event_list[sc->event_ptr];
	sc->event_count++;
	sc->event_ptr++;
	sc->event_ptr %= APM_NEVENTS;
	evp->type = event_type;
	evp->index = ++apm_evindex;
	selwakeup(&sc->sc_rsel);
	return (sc->sc_flags & SCFLAG_OWRITE) ? 0 : 1; /* user may handle */
}
#endif

int
apmpoll(dev, events, l)
	dev_t dev;
	int events;
	struct lwp *l;
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	int revents = 0;

	APM_LOCK(sc);
	if (events & (POLLIN | POLLRDNORM)) {
		if (sc->event_count)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			selrecord(l, &sc->sc_rsel);
	}
	APM_UNLOCK(sc);

	return (revents);
}
#endif

static void
filt_apmrdetach(struct knote *kn)
{
	struct apm_softc *sc = (struct apm_softc *)kn->kn_hook;

	APM_LOCK(sc);
	SLIST_REMOVE(&sc->sc_rsel.sel_klist, kn, knote, kn_selnext);
	APM_UNLOCK(sc);
}

static int
filt_apmread(struct knote *kn, long hint)
{
	struct apm_softc *sc = kn->kn_hook;

	kn->kn_data = sc->event_count;
	return (kn->kn_data > 0);
}

static struct filterops apmread_filtops =
	{ 1, NULL, filt_apmrdetach, filt_apmread};

int
apmkqfilter(dev, kn)
	dev_t dev;
	struct knote *kn;
{
	struct apm_softc *sc = apm_cd.cd_devs[APMUNIT(dev)];
	struct klist *klist;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &sc->sc_rsel.sel_klist;
		kn->kn_fop = &apmread_filtops;
		break;
	default:
		return (1);
	}

	kn->kn_hook = sc;

	APM_LOCK(sc);
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	APM_UNLOCK(sc);

	return (0);
}
