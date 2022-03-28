/* $NetBSD: subr_autoconf.c,v 1.300 2022/03/28 12:38:24 riastradh Exp $ */

/*
 * Copyright (c) 1996, 2000 Christopher G. Demetriou
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
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
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
 *
 * --(license Id: LICENSE.proto,v 1.1 2000/06/13 21:40:26 cgd Exp )--
 */

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Header: subr_autoconf.c,v 1.12 93/02/01 19:31:48 torek Exp  (LBL)
 *
 *	@(#)subr_autoconf.c	8.3 (Berkeley) 5/17/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: subr_autoconf.c,v 1.300 2022/03/28 12:38:24 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_ddb.h"
#include "drvctl.h"
#endif

#include <sys/param.h>
#include <sys/device.h>
#include <sys/disklabel.h>
#include <sys/conf.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <sys/kthread.h>
#include <sys/buf.h>
#include <sys/dirent.h>
#include <sys/mount.h>
#include <sys/namei.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/lockf.h>
#include <sys/callout.h>
#include <sys/devmon.h>
#include <sys/cpu.h>
#include <sys/sysctl.h>
#include <sys/stdarg.h>
#include <sys/localcount.h>

#include <sys/disk.h>

#include <sys/rndsource.h>

#include <machine/limits.h>

/*
 * Autoconfiguration subroutines.
 */

/*
 * Device autoconfiguration timings are mixed into the entropy pool.
 */
static krndsource_t rnd_autoconf_source;

/*
 * ioconf.c exports exactly two names: cfdata and cfroots.  All system
 * devices and drivers are found via these tables.
 */
extern struct cfdata cfdata[];
extern const short cfroots[];

/*
 * List of all cfdriver structures.  We use this to detect duplicates
 * when other cfdrivers are loaded.
 */
struct cfdriverlist allcfdrivers = LIST_HEAD_INITIALIZER(&allcfdrivers);
extern struct cfdriver * const cfdriver_list_initial[];

/*
 * Initial list of cfattach's.
 */
extern const struct cfattachinit cfattachinit[];

/*
 * List of cfdata tables.  We always have one such list -- the one
 * built statically when the kernel was configured.
 */
struct cftablelist allcftables = TAILQ_HEAD_INITIALIZER(allcftables);
static struct cftable initcftable;

#define	ROOT ((device_t)NULL)

struct matchinfo {
	cfsubmatch_t fn;
	device_t parent;
	const int *locs;
	void	*aux;
	struct	cfdata *match;
	int	pri;
};

struct alldevs_foray {
	int			af_s;
	struct devicelist	af_garbage;
};

/*
 * Internal version of the cfargs structure; all versions are
 * canonicalized to this.
 */
struct cfargs_internal {
	union {
		cfsubmatch_t	submatch;/* submatch function (direct config) */
		cfsearch_t	search;	 /* search function (indirect config) */
	};
	const char *	iattr;		/* interface attribute */
	const int *	locators;	/* locators array */
	devhandle_t	devhandle;	/* devhandle_t (by value) */
};

static char *number(char *, int);
static void mapply(struct matchinfo *, cfdata_t);
static void config_devdelete(device_t);
static void config_devunlink(device_t, struct devicelist *);
static void config_makeroom(int, struct cfdriver *);
static void config_devlink(device_t);
static void config_alldevs_enter(struct alldevs_foray *);
static void config_alldevs_exit(struct alldevs_foray *);
static void config_add_attrib_dict(device_t);
static device_t	config_attach_internal(device_t, cfdata_t, void *,
		    cfprint_t, const struct cfargs_internal *);

static void config_collect_garbage(struct devicelist *);
static void config_dump_garbage(struct devicelist *);

static void pmflock_debug(device_t, const char *, int);

static device_t deviter_next1(deviter_t *);
static void deviter_reinit(deviter_t *);

struct deferred_config {
	TAILQ_ENTRY(deferred_config) dc_queue;
	device_t dc_dev;
	void (*dc_func)(device_t);
};

TAILQ_HEAD(deferred_config_head, deferred_config);

static struct deferred_config_head deferred_config_queue =
	TAILQ_HEAD_INITIALIZER(deferred_config_queue);
static struct deferred_config_head interrupt_config_queue =
	TAILQ_HEAD_INITIALIZER(interrupt_config_queue);
static int interrupt_config_threads = 8;
static struct deferred_config_head mountroot_config_queue =
	TAILQ_HEAD_INITIALIZER(mountroot_config_queue);
static int mountroot_config_threads = 2;
static lwp_t **mountroot_config_lwpids;
static size_t mountroot_config_lwpids_size;
bool root_is_mounted = false;

static void config_process_deferred(struct deferred_config_head *, device_t);

/* Hooks to finalize configuration once all real devices have been found. */
struct finalize_hook {
	TAILQ_ENTRY(finalize_hook) f_list;
	int (*f_func)(device_t);
	device_t f_dev;
};
static TAILQ_HEAD(, finalize_hook) config_finalize_list =
	TAILQ_HEAD_INITIALIZER(config_finalize_list);
static int config_finalize_done;

/* list of all devices */
static struct devicelist alldevs = TAILQ_HEAD_INITIALIZER(alldevs);
static kmutex_t alldevs_lock __cacheline_aligned;
static devgen_t alldevs_gen = 1;
static int alldevs_nread = 0;
static int alldevs_nwrite = 0;
static bool alldevs_garbage = false;

static struct devicelist config_pending =
    TAILQ_HEAD_INITIALIZER(config_pending);
static kmutex_t config_misc_lock;
static kcondvar_t config_misc_cv;

static bool detachall = false;

#define	STREQ(s1, s2)			\
	(*(s1) == *(s2) && strcmp((s1), (s2)) == 0)

static bool config_initialized = false;	/* config_init() has been called. */

static int config_do_twiddle;
static callout_t config_twiddle_ch;

static void sysctl_detach_setup(struct sysctllog **);

int no_devmon_insert(const char *, prop_dictionary_t);
int (*devmon_insert_vec)(const char *, prop_dictionary_t) = no_devmon_insert;

typedef int (*cfdriver_fn)(struct cfdriver *);
static int
frob_cfdrivervec(struct cfdriver * const *cfdriverv,
	cfdriver_fn drv_do, cfdriver_fn drv_undo,
	const char *style, bool dopanic)
{
	void (*pr)(const char *, ...) __printflike(1, 2) =
	    dopanic ? panic : printf;
	int i, error = 0, e2 __diagused;

	for (i = 0; cfdriverv[i] != NULL; i++) {
		if ((error = drv_do(cfdriverv[i])) != 0) {
			pr("configure: `%s' driver %s failed: %d",
			    cfdriverv[i]->cd_name, style, error);
			goto bad;
		}
	}

	KASSERT(error == 0);
	return 0;

 bad:
	printf("\n");
	for (i--; i >= 0; i--) {
		e2 = drv_undo(cfdriverv[i]);
		KASSERT(e2 == 0);
	}

	return error;
}

typedef int (*cfattach_fn)(const char *, struct cfattach *);
static int
frob_cfattachvec(const struct cfattachinit *cfattachv,
	cfattach_fn att_do, cfattach_fn att_undo,
	const char *style, bool dopanic)
{
	const struct cfattachinit *cfai = NULL;
	void (*pr)(const char *, ...) __printflike(1, 2) =
	    dopanic ? panic : printf;
	int j = 0, error = 0, e2 __diagused;

	for (cfai = &cfattachv[0]; cfai->cfai_name != NULL; cfai++) {
		for (j = 0; cfai->cfai_list[j] != NULL; j++) {
			if ((error = att_do(cfai->cfai_name,
			    cfai->cfai_list[j])) != 0) {
				pr("configure: attachment `%s' "
				    "of `%s' driver %s failed: %d",
				    cfai->cfai_list[j]->ca_name,
				    cfai->cfai_name, style, error);
				goto bad;
			}
		}
	}

	KASSERT(error == 0);
	return 0;

 bad:
	/*
	 * Rollback in reverse order.  dunno if super-important, but
	 * do that anyway.  Although the code looks a little like
	 * someone did a little integration (in the math sense).
	 */
	printf("\n");
	if (cfai) {
		bool last;

		for (last = false; last == false; ) {
			if (cfai == &cfattachv[0])
				last = true;
			for (j--; j >= 0; j--) {
				e2 = att_undo(cfai->cfai_name,
				    cfai->cfai_list[j]);
				KASSERT(e2 == 0);
			}
			if (!last) {
				cfai--;
				for (j = 0; cfai->cfai_list[j] != NULL; j++)
					;
			}
		}
	}

	return error;
}

/*
 * Initialize the autoconfiguration data structures.  Normally this
 * is done by configure(), but some platforms need to do this very
 * early (to e.g. initialize the console).
 */
void
config_init(void)
{

	KASSERT(config_initialized == false);

	mutex_init(&alldevs_lock, MUTEX_DEFAULT, IPL_VM);

	mutex_init(&config_misc_lock, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&config_misc_cv, "cfgmisc");

	callout_init(&config_twiddle_ch, CALLOUT_MPSAFE);

	frob_cfdrivervec(cfdriver_list_initial,
	    config_cfdriver_attach, NULL, "bootstrap", true);
	frob_cfattachvec(cfattachinit,
	    config_cfattach_attach, NULL, "bootstrap", true);

	initcftable.ct_cfdata = cfdata;
	TAILQ_INSERT_TAIL(&allcftables, &initcftable, ct_list);

	rnd_attach_source(&rnd_autoconf_source, "autoconf", RND_TYPE_UNKNOWN,
	    RND_FLAG_COLLECT_TIME);

	config_initialized = true;
}

/*
 * Init or fini drivers and attachments.  Either all or none
 * are processed (via rollback).  It would be nice if this were
 * atomic to outside consumers, but with the current state of
 * locking ...
 */
int
config_init_component(struct cfdriver * const *cfdriverv,
	const struct cfattachinit *cfattachv, struct cfdata *cfdatav)
{
	int error;

	KERNEL_LOCK(1, NULL);

	if ((error = frob_cfdrivervec(cfdriverv,
	    config_cfdriver_attach, config_cfdriver_detach, "init", false))!= 0)
		goto out;
	if ((error = frob_cfattachvec(cfattachv,
	    config_cfattach_attach, config_cfattach_detach,
	    "init", false)) != 0) {
		frob_cfdrivervec(cfdriverv,
	            config_cfdriver_detach, NULL, "init rollback", true);
		goto out;
	}
	if ((error = config_cfdata_attach(cfdatav, 1)) != 0) {
		frob_cfattachvec(cfattachv,
		    config_cfattach_detach, NULL, "init rollback", true);
		frob_cfdrivervec(cfdriverv,
	            config_cfdriver_detach, NULL, "init rollback", true);
		goto out;
	}

	/* Success!  */
	error = 0;

out:	KERNEL_UNLOCK_ONE(NULL);
	return error;
}

int
config_fini_component(struct cfdriver * const *cfdriverv,
	const struct cfattachinit *cfattachv, struct cfdata *cfdatav)
{
	int error;

	KERNEL_LOCK(1, NULL);

	if ((error = config_cfdata_detach(cfdatav)) != 0)
		goto out;
	if ((error = frob_cfattachvec(cfattachv,
	    config_cfattach_detach, config_cfattach_attach,
	    "fini", false)) != 0) {
		if (config_cfdata_attach(cfdatav, 0) != 0)
			panic("config_cfdata fini rollback failed");
		goto out;
	}
	if ((error = frob_cfdrivervec(cfdriverv,
	    config_cfdriver_detach, config_cfdriver_attach,
	    "fini", false)) != 0) {
		frob_cfattachvec(cfattachv,
	            config_cfattach_attach, NULL, "fini rollback", true);
		if (config_cfdata_attach(cfdatav, 0) != 0)
			panic("config_cfdata fini rollback failed");
		goto out;
	}

	/* Success!  */
	error = 0;

out:	KERNEL_UNLOCK_ONE(NULL);
	return error;
}

void
config_init_mi(void)
{

	if (!config_initialized)
		config_init();

	sysctl_detach_setup(NULL);
}

void
config_deferred(device_t dev)
{

	KASSERT(KERNEL_LOCKED_P());

	config_process_deferred(&deferred_config_queue, dev);
	config_process_deferred(&interrupt_config_queue, dev);
	config_process_deferred(&mountroot_config_queue, dev);
}

static void
config_interrupts_thread(void *cookie)
{
	struct deferred_config *dc;
	device_t dev;

	mutex_enter(&config_misc_lock);
	while ((dc = TAILQ_FIRST(&interrupt_config_queue)) != NULL) {
		TAILQ_REMOVE(&interrupt_config_queue, dc, dc_queue);
		mutex_exit(&config_misc_lock);

		dev = dc->dc_dev;
		(*dc->dc_func)(dev);
		if (!device_pmf_is_registered(dev))
			aprint_debug_dev(dev,
			    "WARNING: power management not supported\n");
		config_pending_decr(dev);
		kmem_free(dc, sizeof(*dc));

		mutex_enter(&config_misc_lock);
	}
	mutex_exit(&config_misc_lock);

	kthread_exit(0);
}

void
config_create_interruptthreads(void)
{
	int i;

	for (i = 0; i < interrupt_config_threads; i++) {
		(void)kthread_create(PRI_NONE, 0/*XXXSMP */, NULL,
		    config_interrupts_thread, NULL, NULL, "configintr");
	}
}

static void
config_mountroot_thread(void *cookie)
{
	struct deferred_config *dc;

	mutex_enter(&config_misc_lock);
	while ((dc = TAILQ_FIRST(&mountroot_config_queue)) != NULL) {
		TAILQ_REMOVE(&mountroot_config_queue, dc, dc_queue);
		mutex_exit(&config_misc_lock);

		(*dc->dc_func)(dc->dc_dev);
		kmem_free(dc, sizeof(*dc));

		mutex_enter(&config_misc_lock);
	}
	mutex_exit(&config_misc_lock);

	kthread_exit(0);
}

void
config_create_mountrootthreads(void)
{
	int i;

	if (!root_is_mounted)
		root_is_mounted = true;

	mountroot_config_lwpids_size = sizeof(mountroot_config_lwpids) *
				       mountroot_config_threads;
	mountroot_config_lwpids = kmem_alloc(mountroot_config_lwpids_size,
					     KM_NOSLEEP);
	KASSERT(mountroot_config_lwpids);
	for (i = 0; i < mountroot_config_threads; i++) {
		mountroot_config_lwpids[i] = 0;
		(void)kthread_create(PRI_NONE, KTHREAD_MUSTJOIN/* XXXSMP */,
				     NULL, config_mountroot_thread, NULL,
				     &mountroot_config_lwpids[i],
				     "configroot");
	}
}

void
config_finalize_mountroot(void)
{
	int i, error;

	for (i = 0; i < mountroot_config_threads; i++) {
		if (mountroot_config_lwpids[i] == 0)
			continue;

		error = kthread_join(mountroot_config_lwpids[i]);
		if (error)
			printf("%s: thread %x joined with error %d\n",
			       __func__, i, error);
	}
	kmem_free(mountroot_config_lwpids, mountroot_config_lwpids_size);
}

/*
 * Announce device attach/detach to userland listeners.
 */

int
no_devmon_insert(const char *name, prop_dictionary_t p)
{

	return ENODEV;
}

static void
devmon_report_device(device_t dev, bool isattach)
{
	prop_dictionary_t ev, dict = device_properties(dev);
	const char *parent;
	const char *what;
	const char *where;
	device_t pdev = device_parent(dev);

	/* If currently no drvctl device, just return */
	if (devmon_insert_vec == no_devmon_insert)
		return;

	ev = prop_dictionary_create();
	if (ev == NULL)
		return;

	what = (isattach ? "device-attach" : "device-detach");
	parent = (pdev == NULL ? "root" : device_xname(pdev));
	if (prop_dictionary_get_string(dict, "location", &where)) {
		prop_dictionary_set_string(ev, "location", where);
		aprint_debug("ev: %s %s at %s in [%s]\n",
		    what, device_xname(dev), parent, where); 
	}
	if (!prop_dictionary_set_string(ev, "device", device_xname(dev)) ||
	    !prop_dictionary_set_string(ev, "parent", parent)) {
		prop_object_release(ev);
		return;
	}

	if ((*devmon_insert_vec)(what, ev) != 0)
		prop_object_release(ev);
}

/*
 * Add a cfdriver to the system.
 */
int
config_cfdriver_attach(struct cfdriver *cd)
{
	struct cfdriver *lcd;

	/* Make sure this driver isn't already in the system. */
	LIST_FOREACH(lcd, &allcfdrivers, cd_list) {
		if (STREQ(lcd->cd_name, cd->cd_name))
			return EEXIST;
	}

	LIST_INIT(&cd->cd_attach);
	LIST_INSERT_HEAD(&allcfdrivers, cd, cd_list);

	return 0;
}

/*
 * Remove a cfdriver from the system.
 */
int
config_cfdriver_detach(struct cfdriver *cd)
{
	struct alldevs_foray af;
	int i, rc = 0;

	config_alldevs_enter(&af);
	/* Make sure there are no active instances. */
	for (i = 0; i < cd->cd_ndevs; i++) {
		if (cd->cd_devs[i] != NULL) {
			rc = EBUSY;
			break;
		}
	}
	config_alldevs_exit(&af);

	if (rc != 0)
		return rc;

	/* ...and no attachments loaded. */
	if (LIST_EMPTY(&cd->cd_attach) == 0)
		return EBUSY;

	LIST_REMOVE(cd, cd_list);

	KASSERT(cd->cd_devs == NULL);

	return 0;
}

/*
 * Look up a cfdriver by name.
 */
struct cfdriver *
config_cfdriver_lookup(const char *name)
{
	struct cfdriver *cd;

	LIST_FOREACH(cd, &allcfdrivers, cd_list) {
		if (STREQ(cd->cd_name, name))
			return cd;
	}

	return NULL;
}

/*
 * Add a cfattach to the specified driver.
 */
int
config_cfattach_attach(const char *driver, struct cfattach *ca)
{
	struct cfattach *lca;
	struct cfdriver *cd;

	cd = config_cfdriver_lookup(driver);
	if (cd == NULL)
		return ESRCH;

	/* Make sure this attachment isn't already on this driver. */
	LIST_FOREACH(lca, &cd->cd_attach, ca_list) {
		if (STREQ(lca->ca_name, ca->ca_name))
			return EEXIST;
	}

	LIST_INSERT_HEAD(&cd->cd_attach, ca, ca_list);

	return 0;
}

/*
 * Remove a cfattach from the specified driver.
 */
int
config_cfattach_detach(const char *driver, struct cfattach *ca)
{
	struct alldevs_foray af;
	struct cfdriver *cd;
	device_t dev;
	int i, rc = 0;

	cd = config_cfdriver_lookup(driver);
	if (cd == NULL)
		return ESRCH;

	config_alldevs_enter(&af);
	/* Make sure there are no active instances. */
	for (i = 0; i < cd->cd_ndevs; i++) {
		if ((dev = cd->cd_devs[i]) == NULL)
			continue;
		if (dev->dv_cfattach == ca) {
			rc = EBUSY;
			break;
		}
	}
	config_alldevs_exit(&af);

	if (rc != 0)
		return rc;

	LIST_REMOVE(ca, ca_list);

	return 0;
}

/*
 * Look up a cfattach by name.
 */
static struct cfattach *
config_cfattach_lookup_cd(struct cfdriver *cd, const char *atname)
{
	struct cfattach *ca;

	LIST_FOREACH(ca, &cd->cd_attach, ca_list) {
		if (STREQ(ca->ca_name, atname))
			return ca;
	}

	return NULL;
}

/*
 * Look up a cfattach by driver/attachment name.
 */
struct cfattach *
config_cfattach_lookup(const char *name, const char *atname)
{
	struct cfdriver *cd;

	cd = config_cfdriver_lookup(name);
	if (cd == NULL)
		return NULL;

	return config_cfattach_lookup_cd(cd, atname);
}

/*
 * Apply the matching function and choose the best.  This is used
 * a few times and we want to keep the code small.
 */
static void
mapply(struct matchinfo *m, cfdata_t cf)
{
	int pri;

	if (m->fn != NULL) {
		pri = (*m->fn)(m->parent, cf, m->locs, m->aux);
	} else {
		pri = config_match(m->parent, cf, m->aux);
	}
	if (pri > m->pri) {
		m->match = cf;
		m->pri = pri;
	}
}

int
config_stdsubmatch(device_t parent, cfdata_t cf, const int *locs, void *aux)
{
	const struct cfiattrdata *ci;
	const struct cflocdesc *cl;
	int nlocs, i;

	ci = cfiattr_lookup(cfdata_ifattr(cf), parent->dv_cfdriver);
	KASSERT(ci);
	nlocs = ci->ci_loclen;
	KASSERT(!nlocs || locs);
	for (i = 0; i < nlocs; i++) {
		cl = &ci->ci_locdesc[i];
		if (cl->cld_defaultstr != NULL &&
		    cf->cf_loc[i] == cl->cld_default)
			continue;
		if (cf->cf_loc[i] == locs[i])
			continue;
		return 0;
	}

	return config_match(parent, cf, aux);
}

/*
 * Helper function: check whether the driver supports the interface attribute
 * and return its descriptor structure.
 */
static const struct cfiattrdata *
cfdriver_get_iattr(const struct cfdriver *cd, const char *ia)
{
	const struct cfiattrdata * const *cpp;

	if (cd->cd_attrs == NULL)
		return 0;

	for (cpp = cd->cd_attrs; *cpp; cpp++) {
		if (STREQ((*cpp)->ci_name, ia)) {
			/* Match. */
			return *cpp;
		}
	}
	return 0;
}

static int __diagused
cfdriver_iattr_count(const struct cfdriver *cd)
{
	const struct cfiattrdata * const *cpp;
	int i;

	if (cd->cd_attrs == NULL)
		return 0;

	for (i = 0, cpp = cd->cd_attrs; *cpp; cpp++) {
		i++;
	}
	return i;
}

/*
 * Lookup an interface attribute description by name.
 * If the driver is given, consider only its supported attributes.
 */
const struct cfiattrdata *
cfiattr_lookup(const char *name, const struct cfdriver *cd)
{
	const struct cfdriver *d;
	const struct cfiattrdata *ia;

	if (cd)
		return cfdriver_get_iattr(cd, name);

	LIST_FOREACH(d, &allcfdrivers, cd_list) {
		ia = cfdriver_get_iattr(d, name);
		if (ia)
			return ia;
	}
	return 0;
}

/*
 * Determine if `parent' is a potential parent for a device spec based
 * on `cfp'.
 */
static int
cfparent_match(const device_t parent, const struct cfparent *cfp)
{
	struct cfdriver *pcd;

	/* We don't match root nodes here. */
	if (cfp == NULL)
		return 0;

	pcd = parent->dv_cfdriver;
	KASSERT(pcd != NULL);

	/*
	 * First, ensure this parent has the correct interface
	 * attribute.
	 */
	if (!cfdriver_get_iattr(pcd, cfp->cfp_iattr))
		return 0;

	/*
	 * If no specific parent device instance was specified (i.e.
	 * we're attaching to the attribute only), we're done!
	 */
	if (cfp->cfp_parent == NULL)
		return 1;

	/*
	 * Check the parent device's name.
	 */
	if (STREQ(pcd->cd_name, cfp->cfp_parent) == 0)
		return 0;	/* not the same parent */

	/*
	 * Make sure the unit number matches.
	 */
	if (cfp->cfp_unit == DVUNIT_ANY ||	/* wildcard */
	    cfp->cfp_unit == parent->dv_unit)
		return 1;

	/* Unit numbers don't match. */
	return 0;
}

/*
 * Helper for config_cfdata_attach(): check all devices whether it could be
 * parent any attachment in the config data table passed, and rescan.
 */
static void
rescan_with_cfdata(const struct cfdata *cf)
{
	device_t d;
	const struct cfdata *cf1;
	deviter_t di;

	KASSERT(KERNEL_LOCKED_P());

	/*
	 * "alldevs" is likely longer than a modules's cfdata, so make it
	 * the outer loop.
	 */
	for (d = deviter_first(&di, 0); d != NULL; d = deviter_next(&di)) {

		if (!(d->dv_cfattach->ca_rescan))
			continue;

		for (cf1 = cf; cf1->cf_name; cf1++) {

			if (!cfparent_match(d, cf1->cf_pspec))
				continue;

			(*d->dv_cfattach->ca_rescan)(d,
				cfdata_ifattr(cf1), cf1->cf_loc);

			config_deferred(d);
		}
	}
	deviter_release(&di);
}

/*
 * Attach a supplemental config data table and rescan potential
 * parent devices if required.
 */
int
config_cfdata_attach(cfdata_t cf, int scannow)
{
	struct cftable *ct;

	KERNEL_LOCK(1, NULL);

	ct = kmem_alloc(sizeof(*ct), KM_SLEEP);
	ct->ct_cfdata = cf;
	TAILQ_INSERT_TAIL(&allcftables, ct, ct_list);

	if (scannow)
		rescan_with_cfdata(cf);

	KERNEL_UNLOCK_ONE(NULL);

	return 0;
}

/*
 * Helper for config_cfdata_detach: check whether a device is
 * found through any attachment in the config data table.
 */
static int
dev_in_cfdata(device_t d, cfdata_t cf)
{
	const struct cfdata *cf1;

	for (cf1 = cf; cf1->cf_name; cf1++)
		if (d->dv_cfdata == cf1)
			return 1;

	return 0;
}

/*
 * Detach a supplemental config data table. Detach all devices found
 * through that table (and thus keeping references to it) before.
 */
int
config_cfdata_detach(cfdata_t cf)
{
	device_t d;
	int error = 0;
	struct cftable *ct;
	deviter_t di;

	KERNEL_LOCK(1, NULL);

	for (d = deviter_first(&di, DEVITER_F_RW); d != NULL;
	     d = deviter_next(&di)) {
		if (!dev_in_cfdata(d, cf))
			continue;
		if ((error = config_detach(d, 0)) != 0)
			break;
	}
	deviter_release(&di);
	if (error) {
		aprint_error_dev(d, "unable to detach instance\n");
		goto out;
	}

	TAILQ_FOREACH(ct, &allcftables, ct_list) {
		if (ct->ct_cfdata == cf) {
			TAILQ_REMOVE(&allcftables, ct, ct_list);
			kmem_free(ct, sizeof(*ct));
			error = 0;
			goto out;
		}
	}

	/* not found -- shouldn't happen */
	error = EINVAL;

out:	KERNEL_UNLOCK_ONE(NULL);
	return error;
}

/*
 * Invoke the "match" routine for a cfdata entry on behalf of
 * an external caller, usually a direct config "submatch" routine.
 */
int
config_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cfattach *ca;

	KASSERT(KERNEL_LOCKED_P());

	ca = config_cfattach_lookup(cf->cf_name, cf->cf_atname);
	if (ca == NULL) {
		/* No attachment for this entry, oh well. */
		return 0;
	}

	return (*ca->ca_match)(parent, cf, aux);
}

/*
 * Invoke the "probe" routine for a cfdata entry on behalf of
 * an external caller, usually an indirect config "search" routine.
 */
int
config_probe(device_t parent, cfdata_t cf, void *aux)
{
	/*
	 * This is currently a synonym for config_match(), but this
	 * is an implementation detail; "match" and "probe" routines
	 * have different behaviors.
	 *
	 * XXX config_probe() should return a bool, because there is
	 * XXX no match score for probe -- it's either there or it's
	 * XXX not, but some ports abuse the return value as a way
	 * XXX to attach "critical" devices before "non-critical"
	 * XXX devices.
	 */
	return config_match(parent, cf, aux);
}

static struct cfargs_internal *
cfargs_canonicalize(const struct cfargs * const cfargs,
    struct cfargs_internal * const store)
{
	struct cfargs_internal *args = store;

	memset(args, 0, sizeof(*args));

	/* If none specified, are all-NULL pointers are good. */
	if (cfargs == NULL) {
		return args;
	}

	/*
	 * Only one arguments version is recognized at this time.
	 */
	if (cfargs->cfargs_version != CFARGS_VERSION) {
		panic("cfargs_canonicalize: unknown version %lu\n",
		    (unsigned long)cfargs->cfargs_version);
	}

	/*
	 * submatch and search are mutually-exclusive.
	 */
	if (cfargs->submatch != NULL && cfargs->search != NULL) {
		panic("cfargs_canonicalize: submatch and search are "
		      "mutually-exclusive");
	}
	if (cfargs->submatch != NULL) {
		args->submatch = cfargs->submatch;
	} else if (cfargs->search != NULL) {
		args->search = cfargs->search;
	}

	args->iattr = cfargs->iattr;
	args->locators = cfargs->locators;
	args->devhandle = cfargs->devhandle;

	return args;
}

/*
 * Iterate over all potential children of some device, calling the given
 * function (default being the child's match function) for each one.
 * Nonzero returns are matches; the highest value returned is considered
 * the best match.  Return the `found child' if we got a match, or NULL
 * otherwise.  The `aux' pointer is simply passed on through.
 *
 * Note that this function is designed so that it can be used to apply
 * an arbitrary function to all potential children (its return value
 * can be ignored).
 */
static cfdata_t
config_search_internal(device_t parent, void *aux,
    const struct cfargs_internal * const args)
{
	struct cftable *ct;
	cfdata_t cf;
	struct matchinfo m;

	KASSERT(config_initialized);
	KASSERT(!args->iattr ||
		cfdriver_get_iattr(parent->dv_cfdriver, args->iattr));
	KASSERT(args->iattr ||
		cfdriver_iattr_count(parent->dv_cfdriver) < 2);

	m.fn = args->submatch;		/* N.B. union */
	m.parent = parent;
	m.locs = args->locators;
	m.aux = aux;
	m.match = NULL;
	m.pri = 0;

	TAILQ_FOREACH(ct, &allcftables, ct_list) {
		for (cf = ct->ct_cfdata; cf->cf_name; cf++) {

			/* We don't match root nodes here. */
			if (!cf->cf_pspec)
				continue;

			/*
			 * Skip cf if no longer eligible, otherwise scan
			 * through parents for one matching `parent', and
			 * try match function.
			 */
			if (cf->cf_fstate == FSTATE_FOUND)
				continue;
			if (cf->cf_fstate == FSTATE_DNOTFOUND ||
			    cf->cf_fstate == FSTATE_DSTAR)
				continue;

			/*
			 * If an interface attribute was specified,
			 * consider only children which attach to
			 * that attribute.
			 */
			if (args->iattr != NULL &&
			    !STREQ(args->iattr, cfdata_ifattr(cf)))
				continue;

			if (cfparent_match(parent, cf->cf_pspec))
				mapply(&m, cf);
		}
	}
	rnd_add_uint32(&rnd_autoconf_source, 0);
	return m.match;
}

cfdata_t
config_search(device_t parent, void *aux, const struct cfargs *cfargs)
{
	cfdata_t cf;
	struct cfargs_internal store;

	cf = config_search_internal(parent, aux,
	    cfargs_canonicalize(cfargs, &store));

	return cf;
}

/*
 * Find the given root device.
 * This is much like config_search, but there is no parent.
 * Don't bother with multiple cfdata tables; the root node
 * must always be in the initial table.
 */
cfdata_t
config_rootsearch(cfsubmatch_t fn, const char *rootname, void *aux)
{
	cfdata_t cf;
	const short *p;
	struct matchinfo m;

	m.fn = fn;
	m.parent = ROOT;
	m.aux = aux;
	m.match = NULL;
	m.pri = 0;
	m.locs = 0;
	/*
	 * Look at root entries for matching name.  We do not bother
	 * with found-state here since only one root should ever be
	 * searched (and it must be done first).
	 */
	for (p = cfroots; *p >= 0; p++) {
		cf = &cfdata[*p];
		if (strcmp(cf->cf_name, rootname) == 0)
			mapply(&m, cf);
	}
	return m.match;
}

static const char * const msgs[] = {
[QUIET]		=	"",
[UNCONF]	=	" not configured\n",
[UNSUPP]	=	" unsupported\n",
};

/*
 * The given `aux' argument describes a device that has been found
 * on the given parent, but not necessarily configured.  Locate the
 * configuration data for that device (using the submatch function
 * provided, or using candidates' cd_match configuration driver
 * functions) and attach it, and return its device_t.  If the device was
 * not configured, call the given `print' function and return NULL.
 */
device_t
config_found(device_t parent, void *aux, cfprint_t print,
    const struct cfargs * const cfargs)
{
	cfdata_t cf;
	struct cfargs_internal store;
	const struct cfargs_internal * const args =
	    cfargs_canonicalize(cfargs, &store);

	cf = config_search_internal(parent, aux, args);
	if (cf != NULL) {
		return config_attach_internal(parent, cf, aux, print, args);
	}

	if (print) {
		if (config_do_twiddle && cold)
			twiddle();

		const int pret = (*print)(aux, device_xname(parent));
		KASSERT(pret >= 0);
		KASSERT(pret < __arraycount(msgs));
		KASSERT(msgs[pret] != NULL);
		aprint_normal("%s", msgs[pret]);
	}

	return NULL;
}

/*
 * As above, but for root devices.
 */
device_t
config_rootfound(const char *rootname, void *aux)
{
	cfdata_t cf;
	device_t dev = NULL;

	KERNEL_LOCK(1, NULL);
	if ((cf = config_rootsearch(NULL, rootname, aux)) != NULL)
		dev = config_attach(ROOT, cf, aux, NULL, CFARGS_NONE);
	else
		aprint_error("root device %s not configured\n", rootname);
	KERNEL_UNLOCK_ONE(NULL);
	return dev;
}

/* just like sprintf(buf, "%d") except that it works from the end */
static char *
number(char *ep, int n)
{

	*--ep = 0;
	while (n >= 10) {
		*--ep = (n % 10) + '0';
		n /= 10;
	}
	*--ep = n + '0';
	return ep;
}

/*
 * Expand the size of the cd_devs array if necessary.
 *
 * The caller must hold alldevs_lock. config_makeroom() may release and
 * re-acquire alldevs_lock, so callers should re-check conditions such
 * as alldevs_nwrite == 0 and alldevs_nread == 0 when config_makeroom()
 * returns.
 */
static void
config_makeroom(int n, struct cfdriver *cd)
{
	int ondevs, nndevs;
	device_t *osp, *nsp;

	KASSERT(mutex_owned(&alldevs_lock));
	alldevs_nwrite++;

	for (nndevs = MAX(4, cd->cd_ndevs); nndevs <= n; nndevs += nndevs)
		;

	while (n >= cd->cd_ndevs) {
		/*
		 * Need to expand the array.
		 */
		ondevs = cd->cd_ndevs;
		osp = cd->cd_devs;

		/*
		 * Release alldevs_lock around allocation, which may
		 * sleep.
		 */
		mutex_exit(&alldevs_lock);
		nsp = kmem_alloc(sizeof(device_t) * nndevs, KM_SLEEP);
		mutex_enter(&alldevs_lock);

		/*
		 * If another thread moved the array while we did
		 * not hold alldevs_lock, try again.
		 */
		if (cd->cd_devs != osp) {
			mutex_exit(&alldevs_lock);
			kmem_free(nsp, sizeof(device_t) * nndevs);
			mutex_enter(&alldevs_lock);
			continue;
		}

		memset(nsp + ondevs, 0, sizeof(device_t) * (nndevs - ondevs));
		if (ondevs != 0)
			memcpy(nsp, cd->cd_devs, sizeof(device_t) * ondevs);

		cd->cd_ndevs = nndevs;
		cd->cd_devs = nsp;
		if (ondevs != 0) {
			mutex_exit(&alldevs_lock);
			kmem_free(osp, sizeof(device_t) * ondevs);
			mutex_enter(&alldevs_lock);
		}
	}
	KASSERT(mutex_owned(&alldevs_lock));
	alldevs_nwrite--;
}

/*
 * Put dev into the devices list.
 */
static void
config_devlink(device_t dev)
{

	mutex_enter(&alldevs_lock);

	KASSERT(device_cfdriver(dev)->cd_devs[dev->dv_unit] == dev);

	dev->dv_add_gen = alldevs_gen;
	/* It is safe to add a device to the tail of the list while
	 * readers and writers are in the list.
	 */
	TAILQ_INSERT_TAIL(&alldevs, dev, dv_list);
	mutex_exit(&alldevs_lock);
}

static void
config_devfree(device_t dev)
{

	KASSERT(dev->dv_flags & DVF_PRIV_ALLOC);
	KASSERTMSG(dev->dv_pending == 0, "%d", dev->dv_pending);

	if (dev->dv_cfattach->ca_devsize > 0)
		kmem_free(dev->dv_private, dev->dv_cfattach->ca_devsize);
	kmem_free(dev, sizeof(*dev));
}

/*
 * Caller must hold alldevs_lock.
 */
static void
config_devunlink(device_t dev, struct devicelist *garbage)
{
	struct device_garbage *dg = &dev->dv_garbage;
	cfdriver_t cd = device_cfdriver(dev);
	int i;

	KASSERT(mutex_owned(&alldevs_lock));
	KASSERTMSG(dev->dv_pending == 0, "%d", dev->dv_pending);

 	/* Unlink from device list.  Link to garbage list. */
	TAILQ_REMOVE(&alldevs, dev, dv_list);
	TAILQ_INSERT_TAIL(garbage, dev, dv_list);

	/* Remove from cfdriver's array. */
	cd->cd_devs[dev->dv_unit] = NULL;

	/*
	 * If the device now has no units in use, unlink its softc array.
	 */
	for (i = 0; i < cd->cd_ndevs; i++) {
		if (cd->cd_devs[i] != NULL)
			break;
	}
	/* Nothing found.  Unlink, now.  Deallocate, later. */
	if (i == cd->cd_ndevs) {
		dg->dg_ndevs = cd->cd_ndevs;
		dg->dg_devs = cd->cd_devs;
		cd->cd_devs = NULL;
		cd->cd_ndevs = 0;
	}
}

static void
config_devdelete(device_t dev)
{
	struct device_garbage *dg = &dev->dv_garbage;
	device_lock_t dvl = device_getlock(dev);

	KASSERTMSG(dev->dv_pending == 0, "%d", dev->dv_pending);

	if (dg->dg_devs != NULL)
		kmem_free(dg->dg_devs, sizeof(device_t) * dg->dg_ndevs);

	localcount_fini(dev->dv_localcount);
	kmem_free(dev->dv_localcount, sizeof(*dev->dv_localcount));

	cv_destroy(&dvl->dvl_cv);
	mutex_destroy(&dvl->dvl_mtx);

	KASSERT(dev->dv_properties != NULL);
	prop_object_release(dev->dv_properties);

	if (dev->dv_activity_handlers)
		panic("%s with registered handlers", __func__);

	if (dev->dv_locators) {
		size_t amount = *--dev->dv_locators;
		kmem_free(dev->dv_locators, amount);
	}

	config_devfree(dev);
}

static int
config_unit_nextfree(cfdriver_t cd, cfdata_t cf)
{
	int unit = cf->cf_unit;

	if (unit < 0)
		return -1;
	if (cf->cf_fstate == FSTATE_STAR) {
		for (; unit < cd->cd_ndevs; unit++)
			if (cd->cd_devs[unit] == NULL)
				break;
		/*
		 * unit is now the unit of the first NULL device pointer,
		 * or max(cd->cd_ndevs,cf->cf_unit).
		 */
	} else {
		if (unit < cd->cd_ndevs && cd->cd_devs[unit] != NULL)
			unit = -1;
	}
	return unit;
}

static int
config_unit_alloc(device_t dev, cfdriver_t cd, cfdata_t cf)
{
	struct alldevs_foray af;
	int unit;

	config_alldevs_enter(&af);
	for (;;) {
		unit = config_unit_nextfree(cd, cf);
		if (unit == -1)
			break;
		if (unit < cd->cd_ndevs) {
			cd->cd_devs[unit] = dev;
			dev->dv_unit = unit;
			break;
		}
		config_makeroom(unit, cd);
	}
	config_alldevs_exit(&af);

	return unit;
}

static device_t
config_devalloc(const device_t parent, const cfdata_t cf,
    const struct cfargs_internal * const args)
{
	cfdriver_t cd;
	cfattach_t ca;
	size_t lname, lunit;
	const char *xunit;
	int myunit;
	char num[10];
	device_t dev;
	void *dev_private;
	const struct cfiattrdata *ia;
	device_lock_t dvl;

	cd = config_cfdriver_lookup(cf->cf_name);
	if (cd == NULL)
		return NULL;

	ca = config_cfattach_lookup_cd(cd, cf->cf_atname);
	if (ca == NULL)
		return NULL;

	/* get memory for all device vars */
	KASSERT(ca->ca_flags & DVF_PRIV_ALLOC);
	if (ca->ca_devsize > 0) {
		dev_private = kmem_zalloc(ca->ca_devsize, KM_SLEEP);
	} else {
		dev_private = NULL;
	}
	dev = kmem_zalloc(sizeof(*dev), KM_SLEEP);

	dev->dv_handle = args->devhandle;

	dev->dv_class = cd->cd_class;
	dev->dv_cfdata = cf;
	dev->dv_cfdriver = cd;
	dev->dv_cfattach = ca;
	dev->dv_activity_count = 0;
	dev->dv_activity_handlers = NULL;
	dev->dv_private = dev_private;
	dev->dv_flags = ca->ca_flags;	/* inherit flags from class */
	dev->dv_attaching = curlwp;

	myunit = config_unit_alloc(dev, cd, cf);
	if (myunit == -1) {
		config_devfree(dev);
		return NULL;
	}

	/* compute length of name and decimal expansion of unit number */
	lname = strlen(cd->cd_name);
	xunit = number(&num[sizeof(num)], myunit);
	lunit = &num[sizeof(num)] - xunit;
	if (lname + lunit > sizeof(dev->dv_xname))
		panic("config_devalloc: device name too long");

	dvl = device_getlock(dev);

	mutex_init(&dvl->dvl_mtx, MUTEX_DEFAULT, IPL_NONE);
	cv_init(&dvl->dvl_cv, "pmfsusp");

	memcpy(dev->dv_xname, cd->cd_name, lname);
	memcpy(dev->dv_xname + lname, xunit, lunit);
	dev->dv_parent = parent;
	if (parent != NULL)
		dev->dv_depth = parent->dv_depth + 1;
	else
		dev->dv_depth = 0;
	dev->dv_flags |= DVF_ACTIVE;	/* always initially active */
	if (args->locators) {
		KASSERT(parent); /* no locators at root */
		ia = cfiattr_lookup(cfdata_ifattr(cf), parent->dv_cfdriver);
		dev->dv_locators =
		    kmem_alloc(sizeof(int) * (ia->ci_loclen + 1), KM_SLEEP);
		*dev->dv_locators++ = sizeof(int) * (ia->ci_loclen + 1);
		memcpy(dev->dv_locators, args->locators,
		    sizeof(int) * ia->ci_loclen);
	}
	dev->dv_properties = prop_dictionary_create();
	KASSERT(dev->dv_properties != NULL);

	prop_dictionary_set_string_nocopy(dev->dv_properties,
	    "device-driver", dev->dv_cfdriver->cd_name);
	prop_dictionary_set_uint16(dev->dv_properties,
	    "device-unit", dev->dv_unit);
	if (parent != NULL) {
		prop_dictionary_set_string(dev->dv_properties,
		    "device-parent", device_xname(parent));
	}

	dev->dv_localcount = kmem_zalloc(sizeof(*dev->dv_localcount),
	    KM_SLEEP);
	localcount_init(dev->dv_localcount);

	if (dev->dv_cfdriver->cd_attrs != NULL)
		config_add_attrib_dict(dev);

	return dev;
}

/*
 * Create an array of device attach attributes and add it
 * to the device's dv_properties dictionary.
 *
 * <key>interface-attributes</key>
 * <array>
 *    <dict>
 *       <key>attribute-name</key>
 *       <string>foo</string>
 *       <key>locators</key>
 *       <array>
 *          <dict>
 *             <key>loc-name</key>
 *             <string>foo-loc1</string>
 *          </dict>
 *          <dict>
 *             <key>loc-name</key>
 *             <string>foo-loc2</string>
 *             <key>default</key>
 *             <string>foo-loc2-default</string>
 *          </dict>
 *          ...
 *       </array>
 *    </dict>
 *    ...
 * </array>
 */

static void
config_add_attrib_dict(device_t dev)
{
	int i, j;
	const struct cfiattrdata *ci;
	prop_dictionary_t attr_dict, loc_dict;
	prop_array_t attr_array, loc_array;

	if ((attr_array = prop_array_create()) == NULL)
		return;

	for (i = 0; ; i++) {
		if ((ci = dev->dv_cfdriver->cd_attrs[i]) == NULL)
			break;
		if ((attr_dict = prop_dictionary_create()) == NULL)
			break;
		prop_dictionary_set_string_nocopy(attr_dict, "attribute-name",
		    ci->ci_name);

		/* Create an array of the locator names and defaults */

		if (ci->ci_loclen != 0 &&
		    (loc_array = prop_array_create()) != NULL) {
			for (j = 0; j < ci->ci_loclen; j++) {
				loc_dict = prop_dictionary_create();
				if (loc_dict == NULL)
					continue;
				prop_dictionary_set_string_nocopy(loc_dict,
				    "loc-name", ci->ci_locdesc[j].cld_name);
				if (ci->ci_locdesc[j].cld_defaultstr != NULL)
					prop_dictionary_set_string_nocopy(
					    loc_dict, "default",
					    ci->ci_locdesc[j].cld_defaultstr);
				prop_array_set(loc_array, j, loc_dict);
				prop_object_release(loc_dict);
			}
			prop_dictionary_set_and_rel(attr_dict, "locators",
			    loc_array);
		}
		prop_array_add(attr_array, attr_dict);
		prop_object_release(attr_dict);
	}
	if (i == 0)
		prop_object_release(attr_array);
	else
		prop_dictionary_set_and_rel(dev->dv_properties,
		    "interface-attributes", attr_array);

	return;
}

/*
 * Attach a found device.
 */
static device_t
config_attach_internal(device_t parent, cfdata_t cf, void *aux, cfprint_t print,
    const struct cfargs_internal * const args)
{
	device_t dev;
	struct cftable *ct;
	const char *drvname;
	bool deferred;

	KASSERT(KERNEL_LOCKED_P());

	dev = config_devalloc(parent, cf, args);
	if (!dev)
		panic("config_attach: allocation of device softc failed");

	/* XXX redundant - see below? */
	if (cf->cf_fstate != FSTATE_STAR) {
		KASSERT(cf->cf_fstate == FSTATE_NOTFOUND);
		cf->cf_fstate = FSTATE_FOUND;
	}

	config_devlink(dev);

	if (config_do_twiddle && cold)
		twiddle();
	else
		aprint_naive("Found ");
	/*
	 * We want the next two printfs for normal, verbose, and quiet,
	 * but not silent (in which case, we're twiddling, instead).
	 */
	if (parent == ROOT) {
		aprint_naive("%s (root)", device_xname(dev));
		aprint_normal("%s (root)", device_xname(dev));
	} else {
		aprint_naive("%s at %s", device_xname(dev),
		    device_xname(parent));
		aprint_normal("%s at %s", device_xname(dev),
		    device_xname(parent));
		if (print)
			(void) (*print)(aux, NULL);
	}

	/*
	 * Before attaching, clobber any unfound devices that are
	 * otherwise identical.
	 * XXX code above is redundant?
	 */
	drvname = dev->dv_cfdriver->cd_name;
	TAILQ_FOREACH(ct, &allcftables, ct_list) {
		for (cf = ct->ct_cfdata; cf->cf_name; cf++) {
			if (STREQ(cf->cf_name, drvname) &&
			    cf->cf_unit == dev->dv_unit) {
				if (cf->cf_fstate == FSTATE_NOTFOUND)
					cf->cf_fstate = FSTATE_FOUND;
			}
		}
	}
	device_register(dev, aux);

	/* Let userland know */
	devmon_report_device(dev, true);

	/*
	 * Prevent detach until the driver's attach function, and all
	 * deferred actions, have finished.
	 */
	config_pending_incr(dev);

	/* Call the driver's attach function.  */
	(*dev->dv_cfattach->ca_attach)(parent, dev, aux);

	/*
	 * Allow other threads to acquire references to the device now
	 * that the driver's attach function is done.
	 */
	mutex_enter(&config_misc_lock);
	KASSERT(dev->dv_attaching == curlwp);
	dev->dv_attaching = NULL;
	cv_broadcast(&config_misc_cv);
	mutex_exit(&config_misc_lock);

	/*
	 * Synchronous parts of attach are done.  Allow detach, unless
	 * the driver's attach function scheduled deferred actions.
	 */
	config_pending_decr(dev);

	mutex_enter(&config_misc_lock);
	deferred = (dev->dv_pending != 0);
	mutex_exit(&config_misc_lock);

	if (!deferred && !device_pmf_is_registered(dev))
		aprint_debug_dev(dev,
		    "WARNING: power management not supported\n");

	config_process_deferred(&deferred_config_queue, dev);

	device_register_post_config(dev, aux);
	rnd_add_uint32(&rnd_autoconf_source, 0);
	return dev;
}

device_t
config_attach(device_t parent, cfdata_t cf, void *aux, cfprint_t print,
    const struct cfargs *cfargs)
{
	struct cfargs_internal store;

	KASSERT(KERNEL_LOCKED_P());

	return config_attach_internal(parent, cf, aux, print,
	    cfargs_canonicalize(cfargs, &store));
}

/*
 * As above, but for pseudo-devices.  Pseudo-devices attached in this
 * way are silently inserted into the device tree, and their children
 * attached.
 *
 * Note that because pseudo-devices are attached silently, any information
 * the attach routine wishes to print should be prefixed with the device
 * name by the attach routine.
 */
device_t
config_attach_pseudo(cfdata_t cf)
{
	device_t dev;

	KERNEL_LOCK(1, NULL);

	struct cfargs_internal args = { };
	dev = config_devalloc(ROOT, cf, &args);
	if (!dev)
		goto out;

	/* XXX mark busy in cfdata */

	if (cf->cf_fstate != FSTATE_STAR) {
		KASSERT(cf->cf_fstate == FSTATE_NOTFOUND);
		cf->cf_fstate = FSTATE_FOUND;
	}

	config_devlink(dev);

#if 0	/* XXXJRT not yet */
	device_register(dev, NULL);	/* like a root node */
#endif

	/* Let userland know */
	devmon_report_device(dev, true);

	/*
	 * Prevent detach until the driver's attach function, and all
	 * deferred actions, have finished.
	 */
	config_pending_incr(dev);

	/* Call the driver's attach function.  */
	(*dev->dv_cfattach->ca_attach)(ROOT, dev, NULL);

	/*
	 * Allow other threads to acquire references to the device now
	 * that the driver's attach function is done.
	 */
	mutex_enter(&config_misc_lock);
	KASSERT(dev->dv_attaching == curlwp);
	dev->dv_attaching = NULL;
	cv_broadcast(&config_misc_cv);
	mutex_exit(&config_misc_lock);

	/*
	 * Synchronous parts of attach are done.  Allow detach, unless
	 * the driver's attach function scheduled deferred actions.
	 */
	config_pending_decr(dev);

	config_process_deferred(&deferred_config_queue, dev);

out:	KERNEL_UNLOCK_ONE(NULL);
	return dev;
}

/*
 * Caller must hold alldevs_lock.
 */
static void
config_collect_garbage(struct devicelist *garbage)
{
	device_t dv;

	KASSERT(!cpu_intr_p());
	KASSERT(!cpu_softintr_p());
	KASSERT(mutex_owned(&alldevs_lock));

	while (alldevs_nwrite == 0 && alldevs_nread == 0 && alldevs_garbage) {
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
			if (dv->dv_del_gen != 0)
				break;
		}
		if (dv == NULL) {
			alldevs_garbage = false;
			break;
		}
		config_devunlink(dv, garbage);
	}
	KASSERT(mutex_owned(&alldevs_lock));
}

static void
config_dump_garbage(struct devicelist *garbage)
{
	device_t dv;

	while ((dv = TAILQ_FIRST(garbage)) != NULL) {
		TAILQ_REMOVE(garbage, dv, dv_list);
		config_devdelete(dv);
	}
}

static int
config_detach_enter(device_t dev)
{
	int error = 0;

	mutex_enter(&config_misc_lock);

	/*
	 * Wait until attach has fully completed, and until any
	 * concurrent detach (e.g., drvctl racing with USB event
	 * thread) has completed.
	 *
	 * Caller must hold alldevs_nread or alldevs_nwrite (e.g., via
	 * deviter) to ensure the winner of the race doesn't free the
	 * device leading the loser of the race into use-after-free.
	 *
	 * XXX Not all callers do this!
	 */
	while (dev->dv_pending || dev->dv_detaching) {
		KASSERTMSG(dev->dv_detaching != curlwp,
		    "recursively detaching %s", device_xname(dev));
		error = cv_wait_sig(&config_misc_cv, &config_misc_lock);
		if (error)
			goto out;
	}

	/*
	 * Attach has completed, and no other concurrent detach is
	 * running.  Claim the device for detaching.  This will cause
	 * all new attempts to acquire references to block.
	 */
	KASSERT(dev->dv_attaching == NULL);
	KASSERT(dev->dv_detaching == NULL);
	dev->dv_detaching = curlwp;

out:	mutex_exit(&config_misc_lock);
	return error;
}

static void
config_detach_exit(device_t dev)
{

	mutex_enter(&config_misc_lock);
	KASSERT(dev->dv_detaching == curlwp);
	dev->dv_detaching = NULL;
	cv_broadcast(&config_misc_cv);
	mutex_exit(&config_misc_lock);
}

/*
 * Detach a device.  Optionally forced (e.g. because of hardware
 * removal) and quiet.  Returns zero if successful, non-zero
 * (an error code) otherwise.
 *
 * Note that this code wants to be run from a process context, so
 * that the detach can sleep to allow processes which have a device
 * open to run and unwind their stacks.
 */
int
config_detach(device_t dev, int flags)
{
	struct alldevs_foray af;
	struct cftable *ct;
	cfdata_t cf;
	const struct cfattach *ca;
	struct cfdriver *cd;
	device_t d __diagused;
	int rv = 0;

	KERNEL_LOCK(1, NULL);

	cf = dev->dv_cfdata;
	KASSERTMSG((cf == NULL || cf->cf_fstate == FSTATE_FOUND ||
		cf->cf_fstate == FSTATE_STAR),
	    "config_detach: %s: bad device fstate: %d",
	    device_xname(dev), cf ? cf->cf_fstate : -1);

	cd = dev->dv_cfdriver;
	KASSERT(cd != NULL);

	ca = dev->dv_cfattach;
	KASSERT(ca != NULL);

	/*
	 * Only one detach at a time, please -- and not until fully
	 * attached.
	 */
	rv = config_detach_enter(dev);
	if (rv) {
		KERNEL_UNLOCK_ONE(NULL);
		return rv;
	}

	mutex_enter(&alldevs_lock);
	if (dev->dv_del_gen != 0) {
		mutex_exit(&alldevs_lock);
#ifdef DIAGNOSTIC
		printf("%s: %s is already detached\n", __func__,
		    device_xname(dev));
#endif /* DIAGNOSTIC */
		config_detach_exit(dev);
		KERNEL_UNLOCK_ONE(NULL);
		return ENOENT;
	}
	alldevs_nwrite++;
	mutex_exit(&alldevs_lock);

	/*
	 * Call the driver's .ca_detach function, unless it has none or
	 * we are skipping it because it's unforced shutdown time and
	 * the driver didn't ask to detach on shutdown.
	 */
	if (!detachall &&
	    (flags & (DETACH_SHUTDOWN|DETACH_FORCE)) == DETACH_SHUTDOWN &&
	    (dev->dv_flags & DVF_DETACH_SHUTDOWN) == 0) {
		rv = EOPNOTSUPP;
	} else if (ca->ca_detach != NULL) {
		rv = (*ca->ca_detach)(dev, flags);
	} else
		rv = EOPNOTSUPP;

	/*
	 * If it was not possible to detach the device, then we either
	 * panic() (for the forced but failed case), or return an error.
	 */
	if (rv) {
		/*
		 * Detach failed -- likely EOPNOTSUPP or EBUSY.  Driver
		 * must not have called config_detach_commit.
		 */
		KASSERTMSG(!dev->dv_detached,
		    "%s committed to detaching and then backed out",
		    device_xname(dev));
		if (flags & DETACH_FORCE) {
			panic("config_detach: forced detach of %s failed (%d)",
			    device_xname(dev), rv);
		}
		goto out;
	}

	/*
	 * The device has now been successfully detached.
	 */

	/*
	 * If .ca_detach didn't commit to detach, then do that for it.
	 * This wakes any pending device_lookup_acquire calls so they
	 * will fail.
	 */
	config_detach_commit(dev);

	/*
	 * If it was possible to detach the device, ensure that the
	 * device is deactivated.
	 */
	dev->dv_flags &= ~DVF_ACTIVE; /* XXXSMP */

	/*
	 * Wait for all device_lookup_acquire references -- mostly, for
	 * all attempts to open the device -- to drain.  It is the
	 * responsibility of .ca_detach to ensure anything with open
	 * references will be interrupted and release them promptly,
	 * not block indefinitely.  All new attempts to acquire
	 * references will fail, as config_detach_commit has arranged
	 * by now.
	 */
	mutex_enter(&config_misc_lock);
	localcount_drain(dev->dv_localcount,
	    &config_misc_cv, &config_misc_lock);
	mutex_exit(&config_misc_lock);

	/* Let userland know */
	devmon_report_device(dev, false);

#ifdef DIAGNOSTIC
	/*
	 * Sanity: If you're successfully detached, you should have no
	 * children.  (Note that because children must be attached
	 * after parents, we only need to search the latter part of
	 * the list.)
	 */
	mutex_enter(&alldevs_lock);
	for (d = TAILQ_NEXT(dev, dv_list); d != NULL;
	    d = TAILQ_NEXT(d, dv_list)) {
		if (d->dv_parent == dev && d->dv_del_gen == 0) {
			printf("config_detach: detached device %s"
			    " has children %s\n", device_xname(dev),
			    device_xname(d));
			panic("config_detach");
		}
	}
	mutex_exit(&alldevs_lock);
#endif

	/* notify the parent that the child is gone */
	if (dev->dv_parent) {
		device_t p = dev->dv_parent;
		if (p->dv_cfattach->ca_childdetached)
			(*p->dv_cfattach->ca_childdetached)(p, dev);
	}

	/*
	 * Mark cfdata to show that the unit can be reused, if possible.
	 */
	TAILQ_FOREACH(ct, &allcftables, ct_list) {
		for (cf = ct->ct_cfdata; cf->cf_name; cf++) {
			if (STREQ(cf->cf_name, cd->cd_name)) {
				if (cf->cf_fstate == FSTATE_FOUND &&
				    cf->cf_unit == dev->dv_unit)
					cf->cf_fstate = FSTATE_NOTFOUND;
			}
		}
	}

	if (dev->dv_cfdata != NULL && (flags & DETACH_QUIET) == 0)
		aprint_normal_dev(dev, "detached\n");

out:
	config_detach_exit(dev);

	config_alldevs_enter(&af);
	KASSERT(alldevs_nwrite != 0);
	--alldevs_nwrite;
	if (rv == 0 && dev->dv_del_gen == 0) {
		if (alldevs_nwrite == 0 && alldevs_nread == 0)
			config_devunlink(dev, &af.af_garbage);
		else {
			dev->dv_del_gen = alldevs_gen;
			alldevs_garbage = true;
		}
	}
	config_alldevs_exit(&af);

	KERNEL_UNLOCK_ONE(NULL);

	return rv;
}

/*
 * config_detach_commit(dev)
 *
 *	Issued by a driver's .ca_detach routine to notify anyone
 *	waiting in device_lookup_acquire that the driver is committed
 *	to detaching the device, which allows device_lookup_acquire to
 *	wake up and fail immediately.
 *
 *	Safe to call multiple times -- idempotent.  Must be called
 *	during config_detach_enter/exit.  Safe to use with
 *	device_lookup because the device is not actually removed from
 *	the table until after config_detach_exit.
 */
void
config_detach_commit(device_t dev)
{

	mutex_enter(&config_misc_lock);
	KASSERT(dev->dv_detaching == curlwp);
	dev->dv_detached = true;
	cv_broadcast(&config_misc_cv);
	mutex_exit(&config_misc_lock);
}

int
config_detach_children(device_t parent, int flags)
{
	device_t dv;
	deviter_t di;
	int error = 0;

	KASSERT(KERNEL_LOCKED_P());

	for (dv = deviter_first(&di, DEVITER_F_RW); dv != NULL;
	     dv = deviter_next(&di)) {
		if (device_parent(dv) != parent)
			continue;
		if ((error = config_detach(dv, flags)) != 0)
			break;
	}
	deviter_release(&di);
	return error;
}

device_t
shutdown_first(struct shutdown_state *s)
{
	if (!s->initialized) {
		deviter_init(&s->di, DEVITER_F_SHUTDOWN|DEVITER_F_LEAVES_FIRST);
		s->initialized = true;
	}
	return shutdown_next(s);
}

device_t
shutdown_next(struct shutdown_state *s)
{
	device_t dv;

	while ((dv = deviter_next(&s->di)) != NULL && !device_is_active(dv))
		;

	if (dv == NULL)
		s->initialized = false;

	return dv;
}

bool
config_detach_all(int how)
{
	static struct shutdown_state s;
	device_t curdev;
	bool progress = false;
	int flags;

	KERNEL_LOCK(1, NULL);

	if ((how & (RB_NOSYNC|RB_DUMP)) != 0)
		goto out;

	if ((how & RB_POWERDOWN) == RB_POWERDOWN)
		flags = DETACH_SHUTDOWN | DETACH_POWEROFF;
	else
		flags = DETACH_SHUTDOWN;

	for (curdev = shutdown_first(&s); curdev != NULL;
	     curdev = shutdown_next(&s)) {
		aprint_debug(" detaching %s, ", device_xname(curdev));
		if (config_detach(curdev, flags) == 0) {
			progress = true;
			aprint_debug("success.");
		} else
			aprint_debug("failed.");
	}

out:	KERNEL_UNLOCK_ONE(NULL);
	return progress;
}

static bool
device_is_ancestor_of(device_t ancestor, device_t descendant)
{
	device_t dv;

	for (dv = descendant; dv != NULL; dv = device_parent(dv)) {
		if (device_parent(dv) == ancestor)
			return true;
	}
	return false;
}

int
config_deactivate(device_t dev)
{
	deviter_t di;
	const struct cfattach *ca;
	device_t descendant;
	int s, rv = 0, oflags;

	for (descendant = deviter_first(&di, DEVITER_F_ROOT_FIRST);
	     descendant != NULL;
	     descendant = deviter_next(&di)) {
		if (dev != descendant &&
		    !device_is_ancestor_of(dev, descendant))
			continue;

		if ((descendant->dv_flags & DVF_ACTIVE) == 0)
			continue;

		ca = descendant->dv_cfattach;
		oflags = descendant->dv_flags;

		descendant->dv_flags &= ~DVF_ACTIVE;
		if (ca->ca_activate == NULL)
			continue;
		s = splhigh();
		rv = (*ca->ca_activate)(descendant, DVACT_DEACTIVATE);
		splx(s);
		if (rv != 0)
			descendant->dv_flags = oflags;
	}
	deviter_release(&di);
	return rv;
}

/*
 * Defer the configuration of the specified device until all
 * of its parent's devices have been attached.
 */
void
config_defer(device_t dev, void (*func)(device_t))
{
	struct deferred_config *dc;

	if (dev->dv_parent == NULL)
		panic("config_defer: can't defer config of a root device");

	dc = kmem_alloc(sizeof(*dc), KM_SLEEP);

	config_pending_incr(dev);

	mutex_enter(&config_misc_lock);
#ifdef DIAGNOSTIC
	struct deferred_config *odc;
	TAILQ_FOREACH(odc, &deferred_config_queue, dc_queue) {
		if (odc->dc_dev == dev)
			panic("config_defer: deferred twice");
	}
#endif
	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&deferred_config_queue, dc, dc_queue);
	mutex_exit(&config_misc_lock);
}

/*
 * Defer some autoconfiguration for a device until after interrupts
 * are enabled.
 */
void
config_interrupts(device_t dev, void (*func)(device_t))
{
	struct deferred_config *dc;

	/*
	 * If interrupts are enabled, callback now.
	 */
	if (cold == 0) {
		(*func)(dev);
		return;
	}

	dc = kmem_alloc(sizeof(*dc), KM_SLEEP);

	config_pending_incr(dev);

	mutex_enter(&config_misc_lock);
#ifdef DIAGNOSTIC
	struct deferred_config *odc;
	TAILQ_FOREACH(odc, &interrupt_config_queue, dc_queue) {
		if (odc->dc_dev == dev)
			panic("config_interrupts: deferred twice");
	}
#endif
	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&interrupt_config_queue, dc, dc_queue);
	mutex_exit(&config_misc_lock);
}

/*
 * Defer some autoconfiguration for a device until after root file system
 * is mounted (to load firmware etc).
 */
void
config_mountroot(device_t dev, void (*func)(device_t))
{
	struct deferred_config *dc;

	/*
	 * If root file system is mounted, callback now.
	 */
	if (root_is_mounted) {
		(*func)(dev);
		return;
	}

	dc = kmem_alloc(sizeof(*dc), KM_SLEEP);

	mutex_enter(&config_misc_lock);
#ifdef DIAGNOSTIC
	struct deferred_config *odc;
	TAILQ_FOREACH(odc, &mountroot_config_queue, dc_queue) {
		if (odc->dc_dev == dev)
			panic("%s: deferred twice", __func__);
	}
#endif

	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&mountroot_config_queue, dc, dc_queue);
	mutex_exit(&config_misc_lock);
}

/*
 * Process a deferred configuration queue.
 */
static void
config_process_deferred(struct deferred_config_head *queue, device_t parent)
{
	struct deferred_config *dc;

	KASSERT(KERNEL_LOCKED_P());

	mutex_enter(&config_misc_lock);
	dc = TAILQ_FIRST(queue);
	while (dc) {
		if (parent == NULL || dc->dc_dev->dv_parent == parent) {
			TAILQ_REMOVE(queue, dc, dc_queue);
			mutex_exit(&config_misc_lock);

			(*dc->dc_func)(dc->dc_dev);
			config_pending_decr(dc->dc_dev);
			kmem_free(dc, sizeof(*dc));

			mutex_enter(&config_misc_lock);
			/* Restart, queue might have changed */
			dc = TAILQ_FIRST(queue);
		} else {
			dc = TAILQ_NEXT(dc, dc_queue);
		}
	}
	mutex_exit(&config_misc_lock);
}

/*
 * Manipulate the config_pending semaphore.
 */
void
config_pending_incr(device_t dev)
{

	mutex_enter(&config_misc_lock);
	KASSERTMSG(dev->dv_pending < INT_MAX,
	    "%s: excess config_pending_incr", device_xname(dev));
	if (dev->dv_pending++ == 0)
		TAILQ_INSERT_TAIL(&config_pending, dev, dv_pending_list);
#ifdef DEBUG_AUTOCONF
	printf("%s: %s %d\n", __func__, device_xname(dev), dev->dv_pending);
#endif
	mutex_exit(&config_misc_lock);
}

void
config_pending_decr(device_t dev)
{

	mutex_enter(&config_misc_lock);
	KASSERTMSG(dev->dv_pending > 0,
	    "%s: excess config_pending_decr", device_xname(dev));
	if (--dev->dv_pending == 0) {
		TAILQ_REMOVE(&config_pending, dev, dv_pending_list);
		cv_broadcast(&config_misc_cv);
	}
#ifdef DEBUG_AUTOCONF
	printf("%s: %s %d\n", __func__, device_xname(dev), dev->dv_pending);
#endif
	mutex_exit(&config_misc_lock);
}

/*
 * Register a "finalization" routine.  Finalization routines are
 * called iteratively once all real devices have been found during
 * autoconfiguration, for as long as any one finalizer has done
 * any work.
 */
int
config_finalize_register(device_t dev, int (*fn)(device_t))
{
	struct finalize_hook *f;
	int error = 0;

	KERNEL_LOCK(1, NULL);

	/*
	 * If finalization has already been done, invoke the
	 * callback function now.
	 */
	if (config_finalize_done) {
		while ((*fn)(dev) != 0)
			/* loop */ ;
		goto out;
	}

	/* Ensure this isn't already on the list. */
	TAILQ_FOREACH(f, &config_finalize_list, f_list) {
		if (f->f_func == fn && f->f_dev == dev) {
			error = EEXIST;
			goto out;
		}
	}

	f = kmem_alloc(sizeof(*f), KM_SLEEP);
	f->f_func = fn;
	f->f_dev = dev;
	TAILQ_INSERT_TAIL(&config_finalize_list, f, f_list);

	/* Success!  */
	error = 0;

out:	KERNEL_UNLOCK_ONE(NULL);
	return error;
}

void
config_finalize(void)
{
	struct finalize_hook *f;
	struct pdevinit *pdev;
	extern struct pdevinit pdevinit[];
	int errcnt, rv;

	/*
	 * Now that device driver threads have been created, wait for
	 * them to finish any deferred autoconfiguration.
	 */
	mutex_enter(&config_misc_lock);
	while (!TAILQ_EMPTY(&config_pending)) {
		device_t dev;
		int error;

		error = cv_timedwait(&config_misc_cv, &config_misc_lock,
		    mstohz(1000));
		if (error == EWOULDBLOCK) {
			aprint_debug("waiting for devices:");
			TAILQ_FOREACH(dev, &config_pending, dv_pending_list)
				aprint_debug(" %s", device_xname(dev));
			aprint_debug("\n");
		}
	}
	mutex_exit(&config_misc_lock);

	KERNEL_LOCK(1, NULL);

	/* Attach pseudo-devices. */
	for (pdev = pdevinit; pdev->pdev_attach != NULL; pdev++)
		(*pdev->pdev_attach)(pdev->pdev_count);

	/* Run the hooks until none of them does any work. */
	do {
		rv = 0;
		TAILQ_FOREACH(f, &config_finalize_list, f_list)
			rv |= (*f->f_func)(f->f_dev);
	} while (rv != 0);

	config_finalize_done = 1;

	/* Now free all the hooks. */
	while ((f = TAILQ_FIRST(&config_finalize_list)) != NULL) {
		TAILQ_REMOVE(&config_finalize_list, f, f_list);
		kmem_free(f, sizeof(*f));
	}

	KERNEL_UNLOCK_ONE(NULL);

	errcnt = aprint_get_error_count();
	if ((boothowto & (AB_QUIET|AB_SILENT)) != 0 &&
	    (boothowto & AB_VERBOSE) == 0) {
		mutex_enter(&config_misc_lock);
		if (config_do_twiddle) {
			config_do_twiddle = 0;
			printf_nolog(" done.\n");
		}
		mutex_exit(&config_misc_lock);
	}
	if (errcnt != 0) {
		printf("WARNING: %d error%s while detecting hardware; "
		    "check system log.\n", errcnt,
		    errcnt == 1 ? "" : "s");
	}
}

void
config_twiddle_init(void)
{

	if ((boothowto & (AB_SILENT|AB_VERBOSE)) == AB_SILENT) {
		config_do_twiddle = 1;
	}
	callout_setfunc(&config_twiddle_ch, config_twiddle_fn, NULL);
}

void
config_twiddle_fn(void *cookie)
{

	mutex_enter(&config_misc_lock);
	if (config_do_twiddle) {
		twiddle();
		callout_schedule(&config_twiddle_ch, mstohz(100));
	}
	mutex_exit(&config_misc_lock);
}

static void
config_alldevs_enter(struct alldevs_foray *af)
{
	TAILQ_INIT(&af->af_garbage);
	mutex_enter(&alldevs_lock);
	config_collect_garbage(&af->af_garbage);
}

static void
config_alldevs_exit(struct alldevs_foray *af)
{
	mutex_exit(&alldevs_lock);
	config_dump_garbage(&af->af_garbage);
}

/*
 * device_lookup:
 *
 *	Look up a device instance for a given driver.
 *
 *	Caller is responsible for ensuring the device's state is
 *	stable, either by holding a reference already obtained with
 *	device_lookup_acquire or by otherwise ensuring the device is
 *	attached and can't be detached (e.g., holding an open device
 *	node and ensuring *_detach calls vdevgone).
 *
 *	XXX Find a way to assert this.
 *
 *	Safe for use up to and including interrupt context at IPL_VM.
 *	Never sleeps.
 */
device_t
device_lookup(cfdriver_t cd, int unit)
{
	device_t dv;

	mutex_enter(&alldevs_lock);
	if (unit < 0 || unit >= cd->cd_ndevs)
		dv = NULL;
	else if ((dv = cd->cd_devs[unit]) != NULL && dv->dv_del_gen != 0)
		dv = NULL;
	mutex_exit(&alldevs_lock);

	return dv;
}

/*
 * device_lookup_private:
 *
 *	Look up a softc instance for a given driver.
 */
void *
device_lookup_private(cfdriver_t cd, int unit)
{

	return device_private(device_lookup(cd, unit));
}

/*
 * device_lookup_acquire:
 *
 *	Look up a device instance for a given driver, and return a
 *	reference to it that must be released by device_release.
 *
 *	=> If the device is still attaching, blocks until *_attach has
 *	   returned.
 *
 *	=> If the device is detaching, blocks until *_detach has
 *	   returned.  May succeed or fail in that case, depending on
 *	   whether *_detach has backed out (EBUSY) or committed to
 *	   detaching.
 *
 *	May sleep.
 */
device_t
device_lookup_acquire(cfdriver_t cd, int unit)
{
	device_t dv;

	ASSERT_SLEEPABLE();

	/* XXX This should have a pserialized fast path -- TBD.  */
	mutex_enter(&config_misc_lock);
	mutex_enter(&alldevs_lock);
retry:	if (unit < 0 || unit >= cd->cd_ndevs ||
	    (dv = cd->cd_devs[unit]) == NULL ||
	    dv->dv_del_gen != 0 ||
	    dv->dv_detached) {
		dv = NULL;
	} else {
		/*
		 * Wait for the device to stabilize, if attaching or
		 * detaching.  Either way we must wait for *_attach or
		 * *_detach to complete, and either way we must retry:
		 * even if detaching, *_detach might fail (EBUSY) so
		 * the device may still be there.
		 */
		if ((dv->dv_attaching != NULL && dv->dv_attaching != curlwp) ||
		    dv->dv_detaching != NULL) {
			mutex_exit(&alldevs_lock);
			cv_wait(&config_misc_cv, &config_misc_lock);
			mutex_enter(&alldevs_lock);
			goto retry;
		}
		localcount_acquire(dv->dv_localcount);
	}
	mutex_exit(&alldevs_lock);
	mutex_exit(&config_misc_lock);

	return dv;
}

/*
 * device_release:
 *
 *	Release a reference to a device acquired with
 *	device_lookup_acquire.
 */
void
device_release(device_t dv)
{

	localcount_release(dv->dv_localcount,
	    &config_misc_cv, &config_misc_lock);
}

/*
 * device_find_by_xname:
 *
 *	Returns the device of the given name or NULL if it doesn't exist.
 */
device_t
device_find_by_xname(const char *name)
{
	device_t dv;
	deviter_t di;

	for (dv = deviter_first(&di, 0); dv != NULL; dv = deviter_next(&di)) {
		if (strcmp(device_xname(dv), name) == 0)
			break;
	}
	deviter_release(&di);

	return dv;
}

/*
 * device_find_by_driver_unit:
 *
 *	Returns the device of the given driver name and unit or
 *	NULL if it doesn't exist.
 */
device_t
device_find_by_driver_unit(const char *name, int unit)
{
	struct cfdriver *cd;

	if ((cd = config_cfdriver_lookup(name)) == NULL)
		return NULL;
	return device_lookup(cd, unit);
}

static bool
match_strcmp(const char * const s1, const char * const s2)
{
	return strcmp(s1, s2) == 0;
}

static bool
match_pmatch(const char * const s1, const char * const s2)
{
	return pmatch(s1, s2, NULL) == 2;
}

static bool
strarray_match_internal(const char ** const strings,
    unsigned int const nstrings, const char * const str,
    unsigned int * const indexp,
    bool (*match_fn)(const char *, const char *))
{
	unsigned int i;

	if (strings == NULL || nstrings == 0) {
		return false;
	}

	for (i = 0; i < nstrings; i++) {
		if ((*match_fn)(strings[i], str)) {
			*indexp = i;
			return true;
		}
	}

	return false;
}

static int
strarray_match(const char ** const strings, unsigned int const nstrings,
    const char * const str)
{
	unsigned int idx;

	if (strarray_match_internal(strings, nstrings, str, &idx,
				    match_strcmp)) {
		return (int)(nstrings - idx);
	}
	return 0;
}

static int
strarray_pmatch(const char ** const strings, unsigned int const nstrings,
    const char * const pattern)
{
	unsigned int idx;

	if (strarray_match_internal(strings, nstrings, pattern, &idx,
				    match_pmatch)) {
		return (int)(nstrings - idx);
	}
	return 0;
}

static int
device_compatible_match_strarray_internal(
    const char **device_compats, int ndevice_compats,
    const struct device_compatible_entry *driver_compats,
    const struct device_compatible_entry **matching_entryp,
    int (*match_fn)(const char **, unsigned int, const char *))
{
	const struct device_compatible_entry *dce = NULL;
	int rv;

	if (ndevice_compats == 0 || device_compats == NULL ||
	    driver_compats == NULL)
		return 0;

	for (dce = driver_compats; dce->compat != NULL; dce++) {
		rv = (*match_fn)(device_compats, ndevice_compats, dce->compat);
		if (rv != 0) {
			if (matching_entryp != NULL) {
				*matching_entryp = dce;
			}
			return rv;
		}
	}
	return 0;
}

/*
 * device_compatible_match:
 *
 *	Match a driver's "compatible" data against a device's
 *	"compatible" strings.  Returns resulted weighted by
 *	which device "compatible" string was matched.
 */
int
device_compatible_match(const char **device_compats, int ndevice_compats,
    const struct device_compatible_entry *driver_compats)
{
	return device_compatible_match_strarray_internal(device_compats,
	    ndevice_compats, driver_compats, NULL, strarray_match);
}

/*
 * device_compatible_pmatch:
 *
 *	Like device_compatible_match(), but uses pmatch(9) to compare
 *	the device "compatible" strings against patterns in the
 *	driver's "compatible" data.
 */
int
device_compatible_pmatch(const char **device_compats, int ndevice_compats,
    const struct device_compatible_entry *driver_compats)
{
	return device_compatible_match_strarray_internal(device_compats,
	    ndevice_compats, driver_compats, NULL, strarray_pmatch);
}

static int
device_compatible_match_strlist_internal(
    const char * const device_compats, size_t const device_compatsize,
    const struct device_compatible_entry *driver_compats,
    const struct device_compatible_entry **matching_entryp,
    int (*match_fn)(const char *, size_t, const char *))
{
	const struct device_compatible_entry *dce = NULL;
	int rv;

	if (device_compats == NULL || device_compatsize == 0 ||
	    driver_compats == NULL)
		return 0;

	for (dce = driver_compats; dce->compat != NULL; dce++) {
		rv = (*match_fn)(device_compats, device_compatsize,
		    dce->compat);
		if (rv != 0) {
			if (matching_entryp != NULL) {
				*matching_entryp = dce;
			}
			return rv;
		}
	}
	return 0;
}

/*
 * device_compatible_match_strlist:
 *
 *	Like device_compatible_match(), but take the device
 *	"compatible" strings as an OpenFirmware-style string
 *	list.
 */
int
device_compatible_match_strlist(
    const char * const device_compats, size_t const device_compatsize,
    const struct device_compatible_entry *driver_compats)
{
	return device_compatible_match_strlist_internal(device_compats,
	    device_compatsize, driver_compats, NULL, strlist_match);
}

/*
 * device_compatible_pmatch_strlist:
 *
 *	Like device_compatible_pmatch(), but take the device
 *	"compatible" strings as an OpenFirmware-style string
 *	list.
 */
int
device_compatible_pmatch_strlist(
    const char * const device_compats, size_t const device_compatsize,
    const struct device_compatible_entry *driver_compats)
{
	return device_compatible_match_strlist_internal(device_compats,
	    device_compatsize, driver_compats, NULL, strlist_pmatch);
}

static int
device_compatible_match_id_internal(
    uintptr_t const id, uintptr_t const mask, uintptr_t const sentinel_id,
    const struct device_compatible_entry *driver_compats,
    const struct device_compatible_entry **matching_entryp)
{
	const struct device_compatible_entry *dce = NULL;

	if (mask == 0)
		return 0;

	for (dce = driver_compats; dce->id != sentinel_id; dce++) {
		if ((id & mask) == dce->id) {
			if (matching_entryp != NULL) {
				*matching_entryp = dce;
			}
			return 1;
		}
	}
	return 0;
}

/*
 * device_compatible_match_id:
 *
 *	Like device_compatible_match(), but takes a single
 *	unsigned integer device ID.
 */
int
device_compatible_match_id(
    uintptr_t const id, uintptr_t const sentinel_id,
    const struct device_compatible_entry *driver_compats)
{
	return device_compatible_match_id_internal(id, (uintptr_t)-1,
	    sentinel_id, driver_compats, NULL);
}

/*
 * device_compatible_lookup:
 *
 *	Look up and return the device_compatible_entry, using the
 *	same matching criteria used by device_compatible_match().
 */
const struct device_compatible_entry *
device_compatible_lookup(const char **device_compats, int ndevice_compats,
			 const struct device_compatible_entry *driver_compats)
{
	const struct device_compatible_entry *dce;

	if (device_compatible_match_strarray_internal(device_compats,
	    ndevice_compats, driver_compats, &dce, strarray_match)) {
		return dce;
	}
	return NULL;
}

/*
 * device_compatible_plookup:
 *
 *	Look up and return the device_compatible_entry, using the
 *	same matching criteria used by device_compatible_pmatch().
 */
const struct device_compatible_entry *
device_compatible_plookup(const char **device_compats, int ndevice_compats,
			  const struct device_compatible_entry *driver_compats)
{
	const struct device_compatible_entry *dce;

	if (device_compatible_match_strarray_internal(device_compats,
	    ndevice_compats, driver_compats, &dce, strarray_pmatch)) {
		return dce;
	}
	return NULL;
}

/*
 * device_compatible_lookup_strlist:
 *
 *	Like device_compatible_lookup(), but take the device
 *	"compatible" strings as an OpenFirmware-style string
 *	list.
 */
const struct device_compatible_entry *
device_compatible_lookup_strlist(
    const char * const device_compats, size_t const device_compatsize,
    const struct device_compatible_entry *driver_compats)
{
	const struct device_compatible_entry *dce;

	if (device_compatible_match_strlist_internal(device_compats,
	    device_compatsize, driver_compats, &dce, strlist_match)) {
		return dce;
	}
	return NULL;
}

/*
 * device_compatible_plookup_strlist:
 *
 *	Like device_compatible_plookup(), but take the device
 *	"compatible" strings as an OpenFirmware-style string
 *	list.
 */
const struct device_compatible_entry *
device_compatible_plookup_strlist(
    const char * const device_compats, size_t const device_compatsize,
    const struct device_compatible_entry *driver_compats)
{
	const struct device_compatible_entry *dce;

	if (device_compatible_match_strlist_internal(device_compats,
	    device_compatsize, driver_compats, &dce, strlist_pmatch)) {
		return dce;
	}
	return NULL;
}

/*
 * device_compatible_lookup_id:
 *
 *	Like device_compatible_lookup(), but takes a single
 *	unsigned integer device ID.
 */
const struct device_compatible_entry *
device_compatible_lookup_id(
    uintptr_t const id, uintptr_t const sentinel_id,
    const struct device_compatible_entry *driver_compats)
{
	const struct device_compatible_entry *dce;

	if (device_compatible_match_id_internal(id, (uintptr_t)-1,
	    sentinel_id, driver_compats, &dce)) {
		return dce;
	}
	return NULL;
}

/*
 * Power management related functions.
 */

bool
device_pmf_is_registered(device_t dev)
{
	return (dev->dv_flags & DVF_POWER_HANDLERS) != 0;
}

bool
device_pmf_driver_suspend(device_t dev, const pmf_qual_t *qual)
{
	if ((dev->dv_flags & DVF_DRIVER_SUSPENDED) != 0)
		return true;
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) == 0)
		return false;
	if (pmf_qual_depth(qual) <= DEVACT_LEVEL_DRIVER &&
	    dev->dv_driver_suspend != NULL &&
	    !(*dev->dv_driver_suspend)(dev, qual))
		return false;

	dev->dv_flags |= DVF_DRIVER_SUSPENDED;
	return true;
}

bool
device_pmf_driver_resume(device_t dev, const pmf_qual_t *qual)
{
	if ((dev->dv_flags & DVF_DRIVER_SUSPENDED) == 0)
		return true;
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) != 0)
		return false;
	if (pmf_qual_depth(qual) <= DEVACT_LEVEL_DRIVER &&
	    dev->dv_driver_resume != NULL &&
	    !(*dev->dv_driver_resume)(dev, qual))
		return false;

	dev->dv_flags &= ~DVF_DRIVER_SUSPENDED;
	return true;
}

bool
device_pmf_driver_shutdown(device_t dev, int how)
{

	if (*dev->dv_driver_shutdown != NULL &&
	    !(*dev->dv_driver_shutdown)(dev, how))
		return false;
	return true;
}

bool
device_pmf_driver_register(device_t dev,
    bool (*suspend)(device_t, const pmf_qual_t *),
    bool (*resume)(device_t, const pmf_qual_t *),
    bool (*shutdown)(device_t, int))
{
	dev->dv_driver_suspend = suspend;
	dev->dv_driver_resume = resume;
	dev->dv_driver_shutdown = shutdown;
	dev->dv_flags |= DVF_POWER_HANDLERS;
	return true;
}

void
device_pmf_driver_deregister(device_t dev)
{
	device_lock_t dvl = device_getlock(dev);

	dev->dv_driver_suspend = NULL;
	dev->dv_driver_resume = NULL;

	mutex_enter(&dvl->dvl_mtx);
	dev->dv_flags &= ~DVF_POWER_HANDLERS;
	while (dvl->dvl_nlock > 0 || dvl->dvl_nwait > 0) {
		/* Wake a thread that waits for the lock.  That
		 * thread will fail to acquire the lock, and then
		 * it will wake the next thread that waits for the
		 * lock, or else it will wake us.
		 */
		cv_signal(&dvl->dvl_cv);
		pmflock_debug(dev, __func__, __LINE__);
		cv_wait(&dvl->dvl_cv, &dvl->dvl_mtx);
		pmflock_debug(dev, __func__, __LINE__);
	}
	mutex_exit(&dvl->dvl_mtx);
}

bool
device_pmf_driver_child_register(device_t dev)
{
	device_t parent = device_parent(dev);

	if (parent == NULL || parent->dv_driver_child_register == NULL)
		return true;
	return (*parent->dv_driver_child_register)(dev);
}

void
device_pmf_driver_set_child_register(device_t dev,
    bool (*child_register)(device_t))
{
	dev->dv_driver_child_register = child_register;
}

static void
pmflock_debug(device_t dev, const char *func, int line)
{
#ifdef PMFLOCK_DEBUG
	device_lock_t dvl = device_getlock(dev);
	const char *curlwp_name;

	if (curlwp->l_name != NULL)
		curlwp_name = curlwp->l_name;
	else
		curlwp_name = curlwp->l_proc->p_comm;

	aprint_debug_dev(dev,
	    "%s.%d, %s dvl_nlock %d dvl_nwait %d dv_flags %x\n", func, line,
	    curlwp_name, dvl->dvl_nlock, dvl->dvl_nwait, dev->dv_flags);
#endif	/* PMFLOCK_DEBUG */
}

static bool
device_pmf_lock1(device_t dev)
{
	device_lock_t dvl = device_getlock(dev);

	while (device_pmf_is_registered(dev) &&
	    dvl->dvl_nlock > 0 && dvl->dvl_holder != curlwp) {
		dvl->dvl_nwait++;
		pmflock_debug(dev, __func__, __LINE__);
		cv_wait(&dvl->dvl_cv, &dvl->dvl_mtx);
		pmflock_debug(dev, __func__, __LINE__);
		dvl->dvl_nwait--;
	}
	if (!device_pmf_is_registered(dev)) {
		pmflock_debug(dev, __func__, __LINE__);
		/* We could not acquire the lock, but some other thread may
		 * wait for it, also.  Wake that thread.
		 */
		cv_signal(&dvl->dvl_cv);
		return false;
	}
	dvl->dvl_nlock++;
	dvl->dvl_holder = curlwp;
	pmflock_debug(dev, __func__, __LINE__);
	return true;
}

bool
device_pmf_lock(device_t dev)
{
	bool rc;
	device_lock_t dvl = device_getlock(dev);

	mutex_enter(&dvl->dvl_mtx);
	rc = device_pmf_lock1(dev);
	mutex_exit(&dvl->dvl_mtx);

	return rc;
}

void
device_pmf_unlock(device_t dev)
{
	device_lock_t dvl = device_getlock(dev);

	KASSERT(dvl->dvl_nlock > 0);
	mutex_enter(&dvl->dvl_mtx);
	if (--dvl->dvl_nlock == 0)
		dvl->dvl_holder = NULL;
	cv_signal(&dvl->dvl_cv);
	pmflock_debug(dev, __func__, __LINE__);
	mutex_exit(&dvl->dvl_mtx);
}

device_lock_t
device_getlock(device_t dev)
{
	return &dev->dv_lock;
}

void *
device_pmf_bus_private(device_t dev)
{
	return dev->dv_bus_private;
}

bool
device_pmf_bus_suspend(device_t dev, const pmf_qual_t *qual)
{
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) != 0)
		return true;
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) == 0 ||
	    (dev->dv_flags & DVF_DRIVER_SUSPENDED) == 0)
		return false;
	if (pmf_qual_depth(qual) <= DEVACT_LEVEL_BUS &&
	    dev->dv_bus_suspend != NULL &&
	    !(*dev->dv_bus_suspend)(dev, qual))
		return false;

	dev->dv_flags |= DVF_BUS_SUSPENDED;
	return true;
}

bool
device_pmf_bus_resume(device_t dev, const pmf_qual_t *qual)
{
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) == 0)
		return true;
	if (pmf_qual_depth(qual) <= DEVACT_LEVEL_BUS &&
	    dev->dv_bus_resume != NULL &&
	    !(*dev->dv_bus_resume)(dev, qual))
		return false;

	dev->dv_flags &= ~DVF_BUS_SUSPENDED;
	return true;
}

bool
device_pmf_bus_shutdown(device_t dev, int how)
{

	if (*dev->dv_bus_shutdown != NULL &&
	    !(*dev->dv_bus_shutdown)(dev, how))
		return false;
	return true;
}

void
device_pmf_bus_register(device_t dev, void *priv,
    bool (*suspend)(device_t, const pmf_qual_t *),
    bool (*resume)(device_t, const pmf_qual_t *),
    bool (*shutdown)(device_t, int), void (*deregister)(device_t))
{
	dev->dv_bus_private = priv;
	dev->dv_bus_resume = resume;
	dev->dv_bus_suspend = suspend;
	dev->dv_bus_shutdown = shutdown;
	dev->dv_bus_deregister = deregister;
}

void
device_pmf_bus_deregister(device_t dev)
{
	if (dev->dv_bus_deregister == NULL)
		return;
	(*dev->dv_bus_deregister)(dev);
	dev->dv_bus_private = NULL;
	dev->dv_bus_suspend = NULL;
	dev->dv_bus_resume = NULL;
	dev->dv_bus_deregister = NULL;
}

void *
device_pmf_class_private(device_t dev)
{
	return dev->dv_class_private;
}

bool
device_pmf_class_suspend(device_t dev, const pmf_qual_t *qual)
{
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) != 0)
		return true;
	if (pmf_qual_depth(qual) <= DEVACT_LEVEL_CLASS &&
	    dev->dv_class_suspend != NULL &&
	    !(*dev->dv_class_suspend)(dev, qual))
		return false;

	dev->dv_flags |= DVF_CLASS_SUSPENDED;
	return true;
}

bool
device_pmf_class_resume(device_t dev, const pmf_qual_t *qual)
{
	if ((dev->dv_flags & DVF_CLASS_SUSPENDED) == 0)
		return true;
	if ((dev->dv_flags & DVF_BUS_SUSPENDED) != 0 ||
	    (dev->dv_flags & DVF_DRIVER_SUSPENDED) != 0)
		return false;
	if (pmf_qual_depth(qual) <= DEVACT_LEVEL_CLASS &&
	    dev->dv_class_resume != NULL &&
	    !(*dev->dv_class_resume)(dev, qual))
		return false;

	dev->dv_flags &= ~DVF_CLASS_SUSPENDED;
	return true;
}

void
device_pmf_class_register(device_t dev, void *priv,
    bool (*suspend)(device_t, const pmf_qual_t *),
    bool (*resume)(device_t, const pmf_qual_t *),
    void (*deregister)(device_t))
{
	dev->dv_class_private = priv;
	dev->dv_class_suspend = suspend;
	dev->dv_class_resume = resume;
	dev->dv_class_deregister = deregister;
}

void
device_pmf_class_deregister(device_t dev)
{
	if (dev->dv_class_deregister == NULL)
		return;
	(*dev->dv_class_deregister)(dev);
	dev->dv_class_private = NULL;
	dev->dv_class_suspend = NULL;
	dev->dv_class_resume = NULL;
	dev->dv_class_deregister = NULL;
}

bool
device_active(device_t dev, devactive_t type)
{
	size_t i;

	if (dev->dv_activity_count == 0)
		return false;

	for (i = 0; i < dev->dv_activity_count; ++i) {
		if (dev->dv_activity_handlers[i] == NULL)
			break;
		(*dev->dv_activity_handlers[i])(dev, type);
	}

	return true;
}

bool
device_active_register(device_t dev, void (*handler)(device_t, devactive_t))
{
	void (**new_handlers)(device_t, devactive_t);
	void (**old_handlers)(device_t, devactive_t);
	size_t i, old_size, new_size;
	int s;

	old_handlers = dev->dv_activity_handlers;
	old_size = dev->dv_activity_count;

	KASSERT(old_size == 0 || old_handlers != NULL);

	for (i = 0; i < old_size; ++i) {
		KASSERT(old_handlers[i] != handler);
		if (old_handlers[i] == NULL) {
			old_handlers[i] = handler;
			return true;
		}
	}

	new_size = old_size + 4;
	new_handlers = kmem_alloc(sizeof(void *) * new_size, KM_SLEEP);

	for (i = 0; i < old_size; ++i)
		new_handlers[i] = old_handlers[i];
	new_handlers[old_size] = handler;
	for (i = old_size+1; i < new_size; ++i)
		new_handlers[i] = NULL;

	s = splhigh();
	dev->dv_activity_count = new_size;
	dev->dv_activity_handlers = new_handlers;
	splx(s);

	if (old_size > 0)
		kmem_free(old_handlers, sizeof(void *) * old_size);

	return true;
}

void
device_active_deregister(device_t dev, void (*handler)(device_t, devactive_t))
{
	void (**old_handlers)(device_t, devactive_t);
	size_t i, old_size;
	int s;

	old_handlers = dev->dv_activity_handlers;
	old_size = dev->dv_activity_count;

	for (i = 0; i < old_size; ++i) {
		if (old_handlers[i] == handler)
			break;
		if (old_handlers[i] == NULL)
			return; /* XXX panic? */
	}

	if (i == old_size)
		return; /* XXX panic? */

	for (; i < old_size - 1; ++i) {
		if ((old_handlers[i] = old_handlers[i + 1]) != NULL)
			continue;

		if (i == 0) {
			s = splhigh();
			dev->dv_activity_count = 0;
			dev->dv_activity_handlers = NULL;
			splx(s);
			kmem_free(old_handlers, sizeof(void *) * old_size);
		}
		return;
	}
	old_handlers[i] = NULL;
}

/* Return true iff the device_t `dev' exists at generation `gen'. */
static bool
device_exists_at(device_t dv, devgen_t gen)
{
	return (dv->dv_del_gen == 0 || dv->dv_del_gen > gen) &&
	    dv->dv_add_gen <= gen;
}

static bool
deviter_visits(const deviter_t *di, device_t dv)
{
	return device_exists_at(dv, di->di_gen);
}

/*
 * Device Iteration
 *
 * deviter_t: a device iterator.  Holds state for a "walk" visiting
 *     each device_t's in the device tree.
 *
 * deviter_init(di, flags): initialize the device iterator `di'
 *     to "walk" the device tree.  deviter_next(di) will return
 *     the first device_t in the device tree, or NULL if there are
 *     no devices.
 *
 *     `flags' is one or more of DEVITER_F_RW, indicating that the
 *     caller intends to modify the device tree by calling
 *     config_detach(9) on devices in the order that the iterator
 *     returns them; DEVITER_F_ROOT_FIRST, asking for the devices
 *     nearest the "root" of the device tree to be returned, first;
 *     DEVITER_F_LEAVES_FIRST, asking for the devices furthest from
 *     the root of the device tree, first; and DEVITER_F_SHUTDOWN,
 *     indicating both that deviter_init() should not respect any
 *     locks on the device tree, and that deviter_next(di) may run
 *     in more than one LWP before the walk has finished.
 *
 *     Only one DEVITER_F_RW iterator may be in the device tree at
 *     once.
 *
 *     DEVITER_F_SHUTDOWN implies DEVITER_F_RW.
 *
 *     Results are undefined if the flags DEVITER_F_ROOT_FIRST and
 *     DEVITER_F_LEAVES_FIRST are used in combination.
 *
 * deviter_first(di, flags): initialize the device iterator `di'
 *     and return the first device_t in the device tree, or NULL
 *     if there are no devices.  The statement
 *
 *         dv = deviter_first(di);
 *
 *     is shorthand for
 *
 *         deviter_init(di);
 *         dv = deviter_next(di);
 *
 * deviter_next(di): return the next device_t in the device tree,
 *     or NULL if there are no more devices.  deviter_next(di)
 *     is undefined if `di' was not initialized with deviter_init() or
 *     deviter_first().
 *
 * deviter_release(di): stops iteration (subsequent calls to
 *     deviter_next() will return NULL), releases any locks and
 *     resources held by the device iterator.
 *
 * Device iteration does not return device_t's in any particular
 * order.  An iterator will never return the same device_t twice.
 * Device iteration is guaranteed to complete---i.e., if deviter_next(di)
 * is called repeatedly on the same `di', it will eventually return
 * NULL.  It is ok to attach/detach devices during device iteration.
 */
void
deviter_init(deviter_t *di, deviter_flags_t flags)
{
	device_t dv;

	memset(di, 0, sizeof(*di));

	if ((flags & DEVITER_F_SHUTDOWN) != 0)
		flags |= DEVITER_F_RW;

	mutex_enter(&alldevs_lock);
	if ((flags & DEVITER_F_RW) != 0)
		alldevs_nwrite++;
	else
		alldevs_nread++;
	di->di_gen = alldevs_gen++;
	di->di_flags = flags;

	switch (di->di_flags & (DEVITER_F_LEAVES_FIRST|DEVITER_F_ROOT_FIRST)) {
	case DEVITER_F_LEAVES_FIRST:
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
			if (!deviter_visits(di, dv))
				continue;
			di->di_curdepth = MAX(di->di_curdepth, dv->dv_depth);
		}
		break;
	case DEVITER_F_ROOT_FIRST:
		TAILQ_FOREACH(dv, &alldevs, dv_list) {
			if (!deviter_visits(di, dv))
				continue;
			di->di_maxdepth = MAX(di->di_maxdepth, dv->dv_depth);
		}
		break;
	default:
		break;
	}

	deviter_reinit(di);
	mutex_exit(&alldevs_lock);
}

static void
deviter_reinit(deviter_t *di)
{

	KASSERT(mutex_owned(&alldevs_lock));
	if ((di->di_flags & DEVITER_F_RW) != 0)
		di->di_prev = TAILQ_LAST(&alldevs, devicelist);
	else
		di->di_prev = TAILQ_FIRST(&alldevs);
}

device_t
deviter_first(deviter_t *di, deviter_flags_t flags)
{

	deviter_init(di, flags);
	return deviter_next(di);
}

static device_t
deviter_next2(deviter_t *di)
{
	device_t dv;

	KASSERT(mutex_owned(&alldevs_lock));

	dv = di->di_prev;

	if (dv == NULL)
		return NULL;

	if ((di->di_flags & DEVITER_F_RW) != 0)
		di->di_prev = TAILQ_PREV(dv, devicelist, dv_list);
	else
		di->di_prev = TAILQ_NEXT(dv, dv_list);

	return dv;
}

static device_t
deviter_next1(deviter_t *di)
{
	device_t dv;

	KASSERT(mutex_owned(&alldevs_lock));

	do {
		dv = deviter_next2(di);
	} while (dv != NULL && !deviter_visits(di, dv));

	return dv;
}

device_t
deviter_next(deviter_t *di)
{
	device_t dv = NULL;

	mutex_enter(&alldevs_lock);
	switch (di->di_flags & (DEVITER_F_LEAVES_FIRST|DEVITER_F_ROOT_FIRST)) {
	case 0:
		dv = deviter_next1(di);
		break;
	case DEVITER_F_LEAVES_FIRST:
		while (di->di_curdepth >= 0) {
			if ((dv = deviter_next1(di)) == NULL) {
				di->di_curdepth--;
				deviter_reinit(di);
			} else if (dv->dv_depth == di->di_curdepth)
				break;
		}
		break;
	case DEVITER_F_ROOT_FIRST:
		while (di->di_curdepth <= di->di_maxdepth) {
			if ((dv = deviter_next1(di)) == NULL) {
				di->di_curdepth++;
				deviter_reinit(di);
			} else if (dv->dv_depth == di->di_curdepth)
				break;
		}
		break;
	default:
		break;
	}
	mutex_exit(&alldevs_lock);

	return dv;
}

void
deviter_release(deviter_t *di)
{
	bool rw = (di->di_flags & DEVITER_F_RW) != 0;

	mutex_enter(&alldevs_lock);
	if (rw)
		--alldevs_nwrite;
	else
		--alldevs_nread;
	/* XXX wake a garbage-collection thread */
	mutex_exit(&alldevs_lock);
}

const char *
cfdata_ifattr(const struct cfdata *cf)
{
	return cf->cf_pspec->cfp_iattr;
}

bool
ifattr_match(const char *snull, const char *t)
{
	return (snull == NULL) || strcmp(snull, t) == 0;
}

void
null_childdetached(device_t self, device_t child)
{
	/* do nothing */
}

static void
sysctl_detach_setup(struct sysctllog **clog)
{

	sysctl_createv(clog, 0, NULL, NULL,
		CTLFLAG_PERMANENT | CTLFLAG_READWRITE,
		CTLTYPE_BOOL, "detachall",
		SYSCTL_DESCR("Detach all devices at shutdown"),
		NULL, 0, &detachall, 0,
		CTL_KERN, CTL_CREATE, CTL_EOL);
}
