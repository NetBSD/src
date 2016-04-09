/*	$NetBSD: profile.c,v 1.6 2016/04/09 15:17:58 riastradh Exp $	*/

/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 *
 * Portions Copyright 2006-2008 John Birrell jb@freebsd.org
 *
 * $FreeBSD: src/sys/cddl/dev/profile/profile.c,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $
 *
 */

/*
 * Copyright 2006 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/cpuvar.h>
#include <sys/fcntl.h>
#include <sys/filio.h>
#ifdef __FreeBSD__
#include <sys/kdb.h>
#endif
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/kthread.h>
#include <sys/limits.h>
#include <sys/linker.h>
#include <sys/lock.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/proc.h>
#include <sys/selinfo.h>
#ifdef __FreeBSD__
#include <sys/smp.h>
#endif
#include <sys/uio.h>
#include <sys/unistd.h>

#ifdef __NetBSD__
#include <sys/atomic.h>
#include <sys/cpu.h>
#define ASSERT(x) KASSERT(x)
#endif

#include <sys/cyclic.h>
#include <sys/dtrace.h>
#include <sys/dtrace_bsd.h>

#define	PROF_NAMELEN		15

#define	PROF_PROFILE		0
#define	PROF_TICK		1
#define	PROF_PREFIX_PROFILE	"profile-"
#define	PROF_PREFIX_TICK	"tick-"

/*
 * Regardless of platform, there are five artificial frames in the case of the
 * profile provider:
 *
 *	profile_fire
 *	cyclic_expire
 *	cyclic_fire
 *	[ cbe ]
 *	[ locore ]
 *
 * On amd64, there are two frames associated with locore:  one in locore, and
 * another in common interrupt dispatch code.  (i386 has not been modified to
 * use this common layer.)  Further, on i386, the interrupted instruction
 * appears as its own stack frame.  All of this means that we need to add one
 * frame for amd64, and then take one away for both amd64 and i386.
 *
 * On SPARC, the picture is further complicated because the compiler
 * optimizes away tail-calls -- so the following frames are optimized away:
 *
 * 	profile_fire
 *	cyclic_expire
 *
 * This gives three frames.  However, on DEBUG kernels, the cyclic_expire
 * frame cannot be tail-call eliminated, yielding four frames in this case.
 *
 * All of the above constraints lead to the mess below.  Yes, the profile
 * provider should ideally figure this out on-the-fly by hiting one of its own
 * probes and then walking its own stack trace.  This is complicated, however,
 * and the static definition doesn't seem to be overly brittle.  Still, we
 * allow for a manual override in case we get it completely wrong.
 */
#ifdef __FreeBSD__
#ifdef __amd64
#define	PROF_ARTIFICIAL_FRAMES	7
#else
#ifdef __i386
#define	PROF_ARTIFICIAL_FRAMES	6
#else
#ifdef __sparc
#ifdef DEBUG
#define	PROF_ARTIFICIAL_FRAMES	4
#else
#define	PROF_ARTIFICIAL_FRAMES	3
#endif
#endif
#endif
#endif
#endif

#ifdef __NetBSD__
#define	PROF_ARTIFICIAL_FRAMES	3
#endif

typedef struct profile_probe {
	char		prof_name[PROF_NAMELEN];
	dtrace_id_t	prof_id;
	int		prof_kind;
	hrtime_t	prof_interval;
	cyclic_id_t	prof_cyclic;
} profile_probe_t;

typedef struct profile_probe_percpu {
	hrtime_t	profc_expected;
	hrtime_t	profc_interval;
	profile_probe_t	*profc_probe;
} profile_probe_percpu_t;

#ifdef __FreeBSD__
static d_open_t	profile_open;
#endif
static int	profile_unload(void);
static void	profile_create(hrtime_t, char *, int);
static void	profile_destroy(void *, dtrace_id_t, void *);
static int	profile_enable(void *, dtrace_id_t, void *);
static void	profile_disable(void *, dtrace_id_t, void *);
static void	profile_load(void *);
static void	profile_provide(void *, const dtrace_probedesc_t *);

static int profile_rates[] = {
    97, 199, 499, 997, 1999,
    4001, 4999, 0, 0, 0,
    0, 0, 0, 0, 0,
    0, 0, 0, 0, 0
};

static int profile_ticks[] = {
    1, 10, 100, 500, 1000,
    5000, 0, 0, 0, 0,
    0, 0, 0, 0, 0
};

/*
 * profile_max defines the upper bound on the number of profile probes that
 * can exist (this is to prevent malicious or clumsy users from exhausing
 * system resources by creating a slew of profile probes). At mod load time,
 * this gets its value from PROFILE_MAX_DEFAULT or profile-max-probes if it's
 * present in the profile.conf file.
 */
#define	PROFILE_MAX_DEFAULT	1000	/* default max. number of probes */
static uint32_t profile_max = PROFILE_MAX_DEFAULT;
					/* maximum number of profile probes */
static uint32_t profile_total;		/* current number of profile probes */

#ifdef __FreeBSD__
static struct cdevsw profile_cdevsw = {
	.d_version	= D_VERSION,
	.d_open		= profile_open,
	.d_name		= "profile",
};
#endif

static dtrace_pattr_t profile_attr = {
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_UNKNOWN },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
{ DTRACE_STABILITY_EVOLVING, DTRACE_STABILITY_EVOLVING, DTRACE_CLASS_COMMON },
{ DTRACE_STABILITY_PRIVATE, DTRACE_STABILITY_PRIVATE, DTRACE_CLASS_ISA },
};

static dtrace_pops_t profile_pops = {
	profile_provide,
	NULL,
	profile_enable,
	profile_disable,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	profile_destroy
};

#ifdef __FreeBSD__
static struct cdev		*profile_cdev;
#endif
static dtrace_provider_id_t	profile_id;
static hrtime_t			profile_interval_min = NANOSEC / 5000;	/* 5000 hz */
static int			profile_aframes = 0;			/* override */

static void
profile_fire(void *arg)
{
	profile_probe_percpu_t *pcpu = arg;
	profile_probe_t *prof = pcpu->profc_probe;
	hrtime_t late;
	solaris_cpu_t *c = &solaris_cpu[cpu_number()];

	late = gethrtime() - pcpu->profc_expected;
	pcpu->profc_expected += pcpu->profc_interval;

	dtrace_probe(prof->prof_id, c->cpu_profile_pc,
	    c->cpu_profile_upc, late, 0, 0);
}

static void
profile_tick(void *arg)
{
	profile_probe_t *prof = arg;
	solaris_cpu_t *c = &solaris_cpu[cpu_number()];

	dtrace_probe(prof->prof_id, c->cpu_profile_pc,
	    c->cpu_profile_upc, 0, 0, 0);
}

static void
profile_create(hrtime_t interval, char *name, int kind)
{
	profile_probe_t *prof;

	if (interval < profile_interval_min)
		return;

	if (dtrace_probe_lookup(profile_id, NULL, NULL, name) != 0)
		return;

	atomic_add_32(&profile_total, 1);
	if (profile_total > profile_max) {
		atomic_add_32(&profile_total, -1);
		return;
	}

	prof = kmem_zalloc(sizeof (profile_probe_t), KM_SLEEP);
	(void) strcpy(prof->prof_name, name);
	prof->prof_interval = interval;
	prof->prof_cyclic = CYCLIC_NONE;
	prof->prof_kind = kind;
	prof->prof_id = dtrace_probe_create(profile_id,
	    NULL, NULL, name,
	    profile_aframes ? profile_aframes : PROF_ARTIFICIAL_FRAMES, prof);
}

/*ARGSUSED*/
static void
profile_provide(void *arg, const dtrace_probedesc_t *desc)
{
	int i, j, rate, kind;
	hrtime_t val = 0, mult = 1, len = 0;
	char *name, *suffix = NULL;

	const struct {
		const char *prefix;
		int kind;
	} types[] = {
		{ PROF_PREFIX_PROFILE, PROF_PROFILE },
		{ PROF_PREFIX_TICK, PROF_TICK },
		{ 0, 0 }
	};

	const struct {
		const char *name;
		hrtime_t mult;
	} suffixes[] = {
		{ "ns", 	NANOSEC / NANOSEC },
		{ "nsec",	NANOSEC / NANOSEC },
		{ "us",		NANOSEC / MICROSEC },
		{ "usec",	NANOSEC / MICROSEC },
		{ "ms",		NANOSEC / MILLISEC },
		{ "msec",	NANOSEC / MILLISEC },
		{ "s",		NANOSEC / SEC },
		{ "sec",	NANOSEC / SEC },
		{ "m",		NANOSEC * (hrtime_t)60 },
		{ "min",	NANOSEC * (hrtime_t)60 },
		{ "h",		NANOSEC * (hrtime_t)(60 * 60) },
		{ "hour",	NANOSEC * (hrtime_t)(60 * 60) },
		{ "d",		NANOSEC * (hrtime_t)(24 * 60 * 60) },
		{ "day",	NANOSEC * (hrtime_t)(24 * 60 * 60) },
		{ "hz",		0 },
		{ NULL,		0 }
	};

	if (desc == NULL) {
		char n[PROF_NAMELEN];

		/*
		 * If no description was provided, provide all of our probes.
		 */
		for (i = 0; i < sizeof (profile_rates) / sizeof (int); i++) {
			if ((rate = profile_rates[i]) == 0)
				continue;

			(void) snprintf(n, PROF_NAMELEN, "%s%d",
			    PROF_PREFIX_PROFILE, rate);
			profile_create(NANOSEC / rate, n, PROF_PROFILE);
		}

		for (i = 0; i < sizeof (profile_ticks) / sizeof (int); i++) {
			if ((rate = profile_ticks[i]) == 0)
				continue;

			(void) snprintf(n, PROF_NAMELEN, "%s%d",
			    PROF_PREFIX_TICK, rate);
			profile_create(NANOSEC / rate, n, PROF_TICK);
		}

		return;
	}

	name = (char *)desc->dtpd_name;

	for (i = 0; types[i].prefix != NULL; i++) {
		len = strlen(types[i].prefix);

		if (strncmp(name, types[i].prefix, len) != 0)
			continue;
		break;
	}

	if (types[i].prefix == NULL)
		return;

	kind = types[i].kind;
	j = strlen(name) - len;

	/*
	 * We need to start before any time suffix.
	 */
	for (j = strlen(name); j >= len; j--) {
		if (name[j] >= '0' && name[j] <= '9')
			break;
		suffix = &name[j];
	}

	ASSERT(suffix != NULL);

	/*
	 * Now determine the numerical value present in the probe name.
	 */
	for (; j >= len; j--) {
		if (name[j] < '0' || name[j] > '9')
			return;

		val += (name[j] - '0') * mult;
		mult *= (hrtime_t)10;
	}

	if (val == 0)
		return;

	/*
	 * Look-up the suffix to determine the multiplier.
	 */
	for (i = 0, mult = 0; suffixes[i].name != NULL; i++) {
		if (strcasecmp(suffixes[i].name, suffix) == 0) {
			mult = suffixes[i].mult;
			break;
		}
	}

	if (suffixes[i].name == NULL && *suffix != '\0')
		return;

	if (mult == 0) {
		/*
		 * The default is frequency-per-second.
		 */
		val = NANOSEC / val;
	} else {
		val *= mult;
	}

	profile_create(val, name, kind);
}

/* ARGSUSED */
static void
profile_destroy(void *arg, dtrace_id_t id, void *parg)
{
	profile_probe_t *prof = parg;

	ASSERT(prof->prof_cyclic == CYCLIC_NONE);
	kmem_free(prof, sizeof (profile_probe_t));

	ASSERT(profile_total >= 1);
	atomic_add_32(&profile_total, -1);
}

/*ARGSUSED*/
static void
profile_online(void *arg, cpu_t *cpu, cyc_handler_t *hdlr, cyc_time_t *when)
{
	profile_probe_t *prof = arg;
	profile_probe_percpu_t *pcpu;

	pcpu = kmem_zalloc(sizeof (profile_probe_percpu_t), KM_SLEEP);
	pcpu->profc_probe = prof;

	hdlr->cyh_func = profile_fire;
	hdlr->cyh_arg = pcpu;

	when->cyt_interval = prof->prof_interval;
	when->cyt_when = gethrtime() + when->cyt_interval;

	pcpu->profc_expected = when->cyt_when;
	pcpu->profc_interval = when->cyt_interval;
}

/*ARGSUSED*/
static void
profile_offline(void *arg, cpu_t *cpu, void *oarg)
{
	profile_probe_percpu_t *pcpu = oarg;

	ASSERT(pcpu->profc_probe == arg);
	kmem_free(pcpu, sizeof (profile_probe_percpu_t));
}

/* ARGSUSED */
static int
profile_enable(void *arg, dtrace_id_t id, void *parg)
{
	profile_probe_t *prof = parg;
	cyc_omni_handler_t omni;
	cyc_handler_t hdlr;
	cyc_time_t when;

	ASSERT(prof->prof_interval != 0);
	ASSERT(MUTEX_HELD(&cpu_lock));

	if (prof->prof_kind == PROF_TICK) {
		hdlr.cyh_func = profile_tick;
		hdlr.cyh_arg = prof;

		when.cyt_interval = prof->prof_interval;
		when.cyt_when = gethrtime() + when.cyt_interval;
	} else {
		ASSERT(prof->prof_kind == PROF_PROFILE);
		omni.cyo_online = profile_online;
		omni.cyo_offline = profile_offline;
		omni.cyo_arg = prof;
	}

	if (prof->prof_kind == PROF_TICK) {
		prof->prof_cyclic = cyclic_add(&hdlr, &when);
	} else {
		prof->prof_cyclic = cyclic_add_omni(&omni);
	}
	return 0;
}

/* ARGSUSED */
static void
profile_disable(void *arg, dtrace_id_t id, void *parg)
{
	profile_probe_t *prof = parg;

	ASSERT(prof->prof_cyclic != CYCLIC_NONE);
	ASSERT(MUTEX_HELD(&cpu_lock));

	cyclic_remove(prof->prof_cyclic);
	prof->prof_cyclic = CYCLIC_NONE;
}

static void
profile_load(void *dummy)
{
#ifdef __FreeBSD__
	/* Create the /dev/dtrace/profile entry. */
	profile_cdev = make_dev(&profile_cdevsw, 0, UID_ROOT, GID_WHEEL, 0600,
	    "dtrace/profile");
#endif

	if (dtrace_register("profile", &profile_attr, DTRACE_PRIV_USER,
	    NULL, &profile_pops, NULL, &profile_id) != 0)
		return;
}


static int
profile_unload()
{
	int error = 0;

	if ((error = dtrace_unregister(profile_id)) != 0)
		return (error);

#ifdef __FreeBSD__
	destroy_dev(profile_cdev);
#endif

	return (error);
}

#ifdef __FreeBSD__

/* ARGSUSED */
static int
profile_modevent(module_t mod __unused, int type, void *data __unused)
{
	int error = 0;

	switch (type) {
	case MOD_LOAD:
		break;

	case MOD_UNLOAD:
		break;

	case MOD_SHUTDOWN:
		break;

	default:
		error = EOPNOTSUPP;
		break;

	}
	return (error);
}

/* ARGSUSED */
static int
profile_open(struct cdev *dev __unused, int oflags __unused, int devtype __unused, struct thread *td __unused)
{
	return (0);
}

SYSINIT(profile_load, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY, profile_load, NULL);
SYSUNINIT(profile_unload, SI_SUB_DTRACE_PROVIDER, SI_ORDER_ANY, profile_unload, NULL);

DEV_MODULE(profile, profile_modevent, NULL);
MODULE_VERSION(profile, 1);
MODULE_DEPEND(profile, dtrace, 1, 1, 1);
MODULE_DEPEND(profile, cyclic, 1, 1, 1);
MODULE_DEPEND(profile, opensolaris, 1, 1, 1);

#endif

#ifdef __NetBSD__

static int
dtrace_profile_modcmd(modcmd_t cmd, void *data)
{
	switch (cmd) {
	case MODULE_CMD_INIT:
		profile_load(NULL);
		return 0;

	case MODULE_CMD_FINI:
		profile_unload();
		return 0;

	case MODULE_CMD_AUTOUNLOAD:
		if (profile_total)
			return EBUSY;
		return 0;

	default:
		return ENOTTY;
	}
}

MODULE(MODULE_CLASS_MISC, dtrace_profile, "dtrace,cyclic");

#endif
