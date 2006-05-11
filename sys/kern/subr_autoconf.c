/* $NetBSD: subr_autoconf.c,v 1.108.4.2 2006/05/11 23:30:15 elad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: subr_autoconf.c,v 1.108.4.2 2006/05/11 23:30:15 elad Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/reboot.h>
#include <machine/limits.h>

#include "opt_userconf.h"
#ifdef USERCONF
#include <sys/userconf.h>
#endif

#ifdef __i386__
#include "opt_splash.h"
#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
#include <dev/splash/splash.h>
extern struct splash_progress *splash_progress_state;
#endif
#endif

/*
 * Autoconfiguration subroutines.
 */

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
struct cftablelist allcftables;
static struct cftable initcftable;

#define	ROOT ((device_t)NULL)

struct matchinfo {
	cfsubmatch_t fn;
	struct	device *parent;
	const int *locs;
	void	*aux;
	struct	cfdata *match;
	int	pri;
};

static char *number(char *, int);
static void mapply(struct matchinfo *, cfdata_t);

struct deferred_config {
	TAILQ_ENTRY(deferred_config) dc_queue;
	device_t dc_dev;
	void (*dc_func)(device_t);
};

TAILQ_HEAD(deferred_config_head, deferred_config);

struct deferred_config_head deferred_config_queue;
struct deferred_config_head interrupt_config_queue;

static void config_process_deferred(struct deferred_config_head *, device_t);

/* Hooks to finalize configuration once all real devices have been found. */
struct finalize_hook {
	TAILQ_ENTRY(finalize_hook) f_list;
	int (*f_func)(device_t);
	device_t f_dev;
};
static TAILQ_HEAD(, finalize_hook) config_finalize_list;
static int config_finalize_done;

/* list of all devices */
struct devicelist alldevs;

volatile int config_pending;		/* semaphore for mountroot */

#define	STREQ(s1, s2)			\
	(*(s1) == *(s2) && strcmp((s1), (s2)) == 0)

static int config_initialized;		/* config_init() has been called. */

static int config_do_twiddle;

/*
 * Initialize the autoconfiguration data structures.  Normally this
 * is done by configure(), but some platforms need to do this very
 * early (to e.g. initialize the console).
 */
void
config_init(void)
{
	const struct cfattachinit *cfai;
	int i, j;

	if (config_initialized)
		return;

	/* allcfdrivers is statically initialized. */
	for (i = 0; cfdriver_list_initial[i] != NULL; i++) {
		if (config_cfdriver_attach(cfdriver_list_initial[i]) != 0)
			panic("configure: duplicate `%s' drivers",
			    cfdriver_list_initial[i]->cd_name);
	}

	for (cfai = &cfattachinit[0]; cfai->cfai_name != NULL; cfai++) {
		for (j = 0; cfai->cfai_list[j] != NULL; j++) {
			if (config_cfattach_attach(cfai->cfai_name,
						   cfai->cfai_list[j]) != 0)
				panic("configure: duplicate `%s' attachment "
				    "of `%s' driver",
				    cfai->cfai_list[j]->ca_name,
				    cfai->cfai_name);
		}
	}

	TAILQ_INIT(&allcftables);
	initcftable.ct_cfdata = cfdata;
	TAILQ_INSERT_TAIL(&allcftables, &initcftable, ct_list);

	TAILQ_INIT(&deferred_config_queue);
	TAILQ_INIT(&interrupt_config_queue);
	TAILQ_INIT(&config_finalize_list);
	TAILQ_INIT(&alldevs);

	config_initialized = 1;
}

/*
 * Configure the system's hardware.
 */
void
configure(void)
{
	int errcnt;

	/* Initialize data structures. */
	config_init();

#ifdef USERCONF
	if (boothowto & RB_USERCONF)
		user_config();
#endif

	if ((boothowto & (AB_SILENT|AB_VERBOSE)) == AB_SILENT) {
		config_do_twiddle = 1;
		printf_nolog("Detecting hardware...");
	}

	/*
	 * Do the machine-dependent portion of autoconfiguration.  This
	 * sets the configuration machinery here in motion by "finding"
	 * the root bus.  When this function returns, we expect interrupts
	 * to be enabled.
	 */
	cpu_configure();

	/*
	 * Now that we've found all the hardware, start the real time
	 * and statistics clocks.
	 */
	initclocks();

	cold = 0;	/* clocks are running, we're warm now! */

	/*
	 * Now callback to finish configuration for devices which want
	 * to do this once interrupts are enabled.
	 */
	config_process_deferred(&interrupt_config_queue, NULL);

	errcnt = aprint_get_error_count();
	if ((boothowto & (AB_QUIET|AB_SILENT)) != 0 &&
	    (boothowto & AB_VERBOSE) == 0) {
		if (config_do_twiddle) {
			config_do_twiddle = 0;
			printf_nolog("done.\n");
		}
		if (errcnt != 0) {
			printf("WARNING: %d error%s while detecting hardware; "
			    "check system log.\n", errcnt,
			    errcnt == 1 ? "" : "s");
		}
	}
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
			return (EEXIST);
	}

	LIST_INIT(&cd->cd_attach);
	LIST_INSERT_HEAD(&allcfdrivers, cd, cd_list);

	return (0);
}

/*
 * Remove a cfdriver from the system.
 */
int
config_cfdriver_detach(struct cfdriver *cd)
{
	int i;

	/* Make sure there are no active instances. */
	for (i = 0; i < cd->cd_ndevs; i++) {
		if (cd->cd_devs[i] != NULL)
			return (EBUSY);
	}

	/* ...and no attachments loaded. */
	if (LIST_EMPTY(&cd->cd_attach) == 0)
		return (EBUSY);

	LIST_REMOVE(cd, cd_list);

	KASSERT(cd->cd_devs == NULL);

	return (0);
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
			return (cd);
	}

	return (NULL);
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
		return (ESRCH);

	/* Make sure this attachment isn't already on this driver. */
	LIST_FOREACH(lca, &cd->cd_attach, ca_list) {
		if (STREQ(lca->ca_name, ca->ca_name))
			return (EEXIST);
	}

	LIST_INSERT_HEAD(&cd->cd_attach, ca, ca_list);

	return (0);
}

/*
 * Remove a cfattach from the specified driver.
 */
int
config_cfattach_detach(const char *driver, struct cfattach *ca)
{
	struct cfdriver *cd;
	device_t dev;
	int i;

	cd = config_cfdriver_lookup(driver);
	if (cd == NULL)
		return (ESRCH);

	/* Make sure there are no active instances. */
	for (i = 0; i < cd->cd_ndevs; i++) {
		if ((dev = cd->cd_devs[i]) == NULL)
			continue;
		if (dev->dv_cfattach == ca)
			return (EBUSY);
	}

	LIST_REMOVE(ca, ca_list);

	return (0);
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
			return (ca);
	}

	return (NULL);
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
		return (NULL);

	return (config_cfattach_lookup_cd(cd, atname));
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

	ci = cfiattr_lookup(cf->cf_pspec->cfp_iattr, parent->dv_cfdriver);
	KASSERT(ci);
	nlocs = ci->ci_loclen;
	for (i = 0; i < nlocs; i++) {
		cl = &ci->ci_locdesc[i];
		/* !cld_defaultstr means no default value */
		if ((!(cl->cld_defaultstr)
		     || (cf->cf_loc[i] != cl->cld_default))
		    && cf->cf_loc[i] != locs[i])
			return (0);
	}

	return (config_match(parent, cf, aux));
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
		return (0);

	for (cpp = cd->cd_attrs; *cpp; cpp++) {
		if (STREQ((*cpp)->ci_name, ia)) {
			/* Match. */
			return (*cpp);
		}
	}
	return (0);
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
		return (cfdriver_get_iattr(cd, name));

	LIST_FOREACH(d, &allcfdrivers, cd_list) {
		ia = cfdriver_get_iattr(d, name);
		if (ia)
			return (ia);
	}
	return (0);
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
		return (0);

	pcd = parent->dv_cfdriver;
	KASSERT(pcd != NULL);

	/*
	 * First, ensure this parent has the correct interface
	 * attribute.
	 */
	if (!cfdriver_get_iattr(pcd, cfp->cfp_iattr))
		return (0);

	/*
	 * If no specific parent device instance was specified (i.e.
	 * we're attaching to the attribute only), we're done!
	 */
	if (cfp->cfp_parent == NULL)
		return (1);

	/*
	 * Check the parent device's name.
	 */
	if (STREQ(pcd->cd_name, cfp->cfp_parent) == 0)
		return (0);	/* not the same parent */

	/*
	 * Make sure the unit number matches.
	 */
	if (cfp->cfp_unit == DVUNIT_ANY ||	/* wildcard */
	    cfp->cfp_unit == parent->dv_unit)
		return (1);

	/* Unit numbers don't match. */
	return (0);
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

	/*
	 * "alldevs" is likely longer than an LKM's cfdata, so make it
	 * the outer loop.
	 */
	TAILQ_FOREACH(d, &alldevs, dv_list) {

		if (!(d->dv_cfattach->ca_rescan))
			continue;

		for (cf1 = cf; cf1->cf_name; cf1++) {

			if (!cfparent_match(d, cf1->cf_pspec))
				continue;

			(*d->dv_cfattach->ca_rescan)(d,
				cf1->cf_pspec->cfp_iattr, cf1->cf_loc);
		}
	}
}

/*
 * Attach a supplemental config data table and rescan potential
 * parent devices if required.
 */
int
config_cfdata_attach(cfdata_t cf, int scannow)
{
	struct cftable *ct;

	ct = malloc(sizeof(struct cftable), M_DEVBUF, M_WAITOK);
	ct->ct_cfdata = cf;
	TAILQ_INSERT_TAIL(&allcftables, ct, ct_list);

	if (scannow)
		rescan_with_cfdata(cf);

	return (0);
}

/*
 * Helper for config_cfdata_detach: check whether a device is
 * found through any attachment in the config data table.
 */
static int
dev_in_cfdata(const struct device *d, const struct cfdata *cf)
{
	const struct cfdata *cf1;

	for (cf1 = cf; cf1->cf_name; cf1++)
		if (d->dv_cfdata == cf1)
			return (1);

	return (0);
}

/*
 * Detach a supplemental config data table. Detach all devices found
 * through that table (and thus keeping references to it) before.
 */
int
config_cfdata_detach(cfdata_t cf)
{
	device_t d;
	int error;
	struct cftable *ct;

again:
	TAILQ_FOREACH(d, &alldevs, dv_list) {
		if (dev_in_cfdata(d, cf)) {
			error = config_detach(d, 0);
			if (error) {
				aprint_error("%s: unable to detach instance\n",
					d->dv_xname);
				return (error);
			}
			goto again;
		}
	}

	TAILQ_FOREACH(ct, &allcftables, ct_list) {
		if (ct->ct_cfdata == cf) {
			TAILQ_REMOVE(&allcftables, ct, ct_list);
			free(ct, M_DEVBUF);
			return (0);
		}
	}

	/* not found -- shouldn't happen */
	return (EINVAL);
}

/*
 * Invoke the "match" routine for a cfdata entry on behalf of
 * an external caller, usually a "submatch" routine.
 */
int
config_match(device_t parent, cfdata_t cf, void *aux)
{
	struct cfattach *ca;

	ca = config_cfattach_lookup(cf->cf_name, cf->cf_atname);
	if (ca == NULL) {
		/* No attachment for this entry, oh well. */
		return (0);
	}

	return ((*ca->ca_match)(parent, cf, aux));
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
cfdata_t
config_search_loc(cfsubmatch_t fn, device_t parent,
		  const char *ifattr, const int *locs, void *aux)
{
	struct cftable *ct;
	cfdata_t cf;
	struct matchinfo m;

	KASSERT(config_initialized);
	KASSERT(!ifattr || cfdriver_get_iattr(parent->dv_cfdriver, ifattr));

	m.fn = fn;
	m.parent = parent;
	m.locs = locs;
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
			if (ifattr && !STREQ(ifattr, cf->cf_pspec->cfp_iattr))
				continue;

			if (cfparent_match(parent, cf->cf_pspec))
				mapply(&m, cf);
		}
	}
	return (m.match);
}

cfdata_t
config_search_ia(cfsubmatch_t fn, device_t parent, const char *ifattr,
    void *aux)
{

	return (config_search_loc(fn, parent, ifattr, NULL, aux));
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
	return (m.match);
}

static const char * const msgs[3] = { "", " not configured\n", " unsupported\n" };

/*
 * The given `aux' argument describes a device that has been found
 * on the given parent, but not necessarily configured.  Locate the
 * configuration data for that device (using the submatch function
 * provided, or using candidates' cd_match configuration driver
 * functions) and attach it, and return true.  If the device was
 * not configured, call the given `print' function and return 0.
 */
device_t
config_found_sm_loc(device_t parent,
		const char *ifattr, const int *locs, void *aux,
		cfprint_t print, cfsubmatch_t submatch)
{
	cfdata_t cf;

#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
	if (splash_progress_state)
		splash_progress_update(splash_progress_state);
#endif

	if ((cf = config_search_loc(submatch, parent, ifattr, locs, aux)))
		return(config_attach_loc(parent, cf, locs, aux, print));
	if (print) {
		if (config_do_twiddle)
			twiddle();
		aprint_normal("%s", msgs[(*print)(aux, parent->dv_xname)]);
	}

#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
	if (splash_progress_state)
		splash_progress_update(splash_progress_state);
#endif

	return (NULL);
}

device_t
config_found_ia(device_t parent, const char *ifattr, void *aux,
    cfprint_t print)
{

	return (config_found_sm_loc(parent, ifattr, NULL, aux, print, NULL));
}

device_t
config_found(device_t parent, void *aux, cfprint_t print)
{

	return (config_found_sm_loc(parent, NULL, NULL, aux, print, NULL));
}

/*
 * As above, but for root devices.
 */
device_t
config_rootfound(const char *rootname, void *aux)
{
	cfdata_t cf;

	if ((cf = config_rootsearch((cfsubmatch_t)NULL, rootname, aux)) != NULL)
		return (config_attach(ROOT, cf, aux, (cfprint_t)NULL));
	aprint_error("root device %s not configured\n", rootname);
	return (NULL);
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
	return (ep);
}

/*
 * Expand the size of the cd_devs array if necessary.
 */
void
config_makeroom(int n, struct cfdriver *cd)
{
	int old, new;
	void **nsp;

	if (n < cd->cd_ndevs)
		return;

	/*
	 * Need to expand the array.
	 */
	old = cd->cd_ndevs;
	if (old == 0)
		new = MINALLOCSIZE / sizeof(void *);
	else
		new = old * 2;
	while (new <= n)
		new *= 2;
	cd->cd_ndevs = new;
	nsp = malloc(new * sizeof(void *), M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (nsp == NULL)
		panic("config_attach: %sing dev array",
		    old != 0 ? "expand" : "creat");
	memset(nsp + old, 0, (new - old) * sizeof(void *));
	if (old != 0) {
		memcpy(nsp, cd->cd_devs, old * sizeof(void *));
		free(cd->cd_devs, M_DEVBUF);
	}
	cd->cd_devs = nsp;
}

/*
 * Attach a found device.  Allocates memory for device variables.
 */
device_t
config_attach_loc(device_t parent, cfdata_t cf,
	const int *locs, void *aux, cfprint_t print)
{
	device_t dev;
	struct cftable *ct;
	struct cfdriver *cd;
	struct cfattach *ca;
	size_t lname, lunit;
	const char *xunit;
	int myunit;
	char num[10];
	const struct cfiattrdata *ia;

#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
	if (splash_progress_state)
		splash_progress_update(splash_progress_state);
#endif

	cd = config_cfdriver_lookup(cf->cf_name);
	KASSERT(cd != NULL);

	ca = config_cfattach_lookup_cd(cd, cf->cf_atname);
	KASSERT(ca != NULL);

	if (ca->ca_devsize < sizeof(struct device))
		panic("config_attach");

#ifndef __BROKEN_CONFIG_UNIT_USAGE
	if (cf->cf_fstate == FSTATE_STAR) {
		for (myunit = cf->cf_unit; myunit < cd->cd_ndevs; myunit++)
			if (cd->cd_devs[myunit] == NULL)
				break;
		/*
		 * myunit is now the unit of the first NULL device pointer,
		 * or max(cd->cd_ndevs,cf->cf_unit).
		 */
	} else {
		myunit = cf->cf_unit;
		KASSERT(cf->cf_fstate == FSTATE_NOTFOUND);
		cf->cf_fstate = FSTATE_FOUND;
	}
#else
	myunit = cf->cf_unit;
	if (cf->cf_fstate == FSTATE_STAR)
		cf->cf_unit++;
	else {
		KASSERT(cf->cf_fstate == FSTATE_NOTFOUND);
		cf->cf_fstate = FSTATE_FOUND;
	}
#endif /* ! __BROKEN_CONFIG_UNIT_USAGE */

	/* compute length of name and decimal expansion of unit number */
	lname = strlen(cd->cd_name);
	xunit = number(&num[sizeof(num)], myunit);
	lunit = &num[sizeof(num)] - xunit;
	if (lname + lunit > sizeof(dev->dv_xname))
		panic("config_attach: device name too long");

	/* get memory for all device vars */
	dev = (device_t)malloc(ca->ca_devsize, M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (!dev)
	    panic("config_attach: memory allocation for device softc failed");
	memset(dev, 0, ca->ca_devsize);
	TAILQ_INSERT_TAIL(&alldevs, dev, dv_list);	/* link up */
	dev->dv_class = cd->cd_class;
	dev->dv_cfdata = cf;
	dev->dv_cfdriver = cd;
	dev->dv_cfattach = ca;
	dev->dv_unit = myunit;
	memcpy(dev->dv_xname, cd->cd_name, lname);
	memcpy(dev->dv_xname + lname, xunit, lunit);
	dev->dv_parent = parent;
	dev->dv_flags = DVF_ACTIVE;	/* always initially active */
	if (locs) {
		KASSERT(parent); /* no locators at root */
		ia = cfiattr_lookup(cf->cf_pspec->cfp_iattr,
				    parent->dv_cfdriver);
		dev->dv_locators = malloc(ia->ci_loclen * sizeof(int),
					  M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
		memcpy(dev->dv_locators, locs, ia->ci_loclen * sizeof(int));
	}
	dev->dv_properties = prop_dictionary_create();
	KASSERT(dev->dv_properties != NULL);

	if (config_do_twiddle)
		twiddle();
	else
		aprint_naive("Found ");
	/*
	 * We want the next two printfs for normal, verbose, and quiet,
	 * but not silent (in which case, we're twiddling, instead).
	 */
	if (parent == ROOT) {
		aprint_naive("%s (root)", dev->dv_xname);
		aprint_normal("%s (root)", dev->dv_xname);
	} else {
		aprint_naive("%s at %s", dev->dv_xname, parent->dv_xname);
		aprint_normal("%s at %s", dev->dv_xname, parent->dv_xname);
		if (print)
			(void) (*print)(aux, NULL);
	}

	/* put this device in the devices array */
	config_makeroom(dev->dv_unit, cd);
	if (cd->cd_devs[dev->dv_unit])
		panic("config_attach: duplicate %s", dev->dv_xname);
	cd->cd_devs[dev->dv_unit] = dev;

	/*
	 * Before attaching, clobber any unfound devices that are
	 * otherwise identical.
	 */
	TAILQ_FOREACH(ct, &allcftables, ct_list) {
		for (cf = ct->ct_cfdata; cf->cf_name; cf++) {
			if (STREQ(cf->cf_name, cd->cd_name) &&
			    cf->cf_unit == dev->dv_unit) {
				if (cf->cf_fstate == FSTATE_NOTFOUND)
					cf->cf_fstate = FSTATE_FOUND;
#ifdef __BROKEN_CONFIG_UNIT_USAGE
				/*
				 * Bump the unit number on all starred cfdata
				 * entries for this device.
				 */
				if (cf->cf_fstate == FSTATE_STAR)
					cf->cf_unit++;
#endif /* __BROKEN_CONFIG_UNIT_USAGE */
			}
		}
	}
#ifdef __HAVE_DEVICE_REGISTER
	device_register(dev, aux);
#endif
#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
	if (splash_progress_state)
		splash_progress_update(splash_progress_state);
#endif
	(*ca->ca_attach)(parent, dev, aux);
#if defined(SPLASHSCREEN) && defined(SPLASHSCREEN_PROGRESS)
	if (splash_progress_state)
		splash_progress_update(splash_progress_state);
#endif
	config_process_deferred(&deferred_config_queue, dev);
	return (dev);
}

device_t
config_attach(device_t parent, cfdata_t cf, void *aux, cfprint_t print)
{

	return (config_attach_loc(parent, cf, NULL, aux, print));
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
	struct cfdriver *cd;
	struct cfattach *ca;
	size_t lname, lunit;
	const char *xunit;
	int myunit;
	char num[10];

	cd = config_cfdriver_lookup(cf->cf_name);
	if (cd == NULL)
		return (NULL);

	ca = config_cfattach_lookup_cd(cd, cf->cf_atname);
	if (ca == NULL)
		return (NULL);

	if (ca->ca_devsize < sizeof(struct device))
		panic("config_attach_pseudo");

	/*
	 * We just ignore cf_fstate, instead doing everything with
	 * cf_unit.
	 *
	 * XXX Should we change this and use FSTATE_NOTFOUND and
	 * XXX FSTATE_STAR?
	 */

	if (cf->cf_unit == DVUNIT_ANY) {
		for (myunit = 0; myunit < cd->cd_ndevs; myunit++)
			if (cd->cd_devs[myunit] == NULL)
				break;
		/*
		 * myunit is now the unit of the first NULL device pointer.
		 */
	} else {
		myunit = cf->cf_unit;
		if (myunit < cd->cd_ndevs && cd->cd_devs[myunit] != NULL)
			return (NULL);
	}

	/* compute length of name and decimal expansion of unit number */
	lname = strlen(cd->cd_name);
	xunit = number(&num[sizeof(num)], myunit);
	lunit = &num[sizeof(num)] - xunit;
	if (lname + lunit > sizeof(dev->dv_xname))
		panic("config_attach_pseudo: device name too long");

	/* get memory for all device vars */
	dev = (device_t)malloc(ca->ca_devsize, M_DEVBUF,
	    cold ? M_NOWAIT : M_WAITOK);
	if (!dev)
		panic("config_attach_pseudo: memory allocation for device "
		    "softc failed");
	memset(dev, 0, ca->ca_devsize);
	TAILQ_INSERT_TAIL(&alldevs, dev, dv_list);	/* link up */
	dev->dv_class = cd->cd_class;
	dev->dv_cfdata = cf;
	dev->dv_cfdriver = cd;
	dev->dv_cfattach = ca;
	dev->dv_unit = myunit;
	memcpy(dev->dv_xname, cd->cd_name, lname);
	memcpy(dev->dv_xname + lname, xunit, lunit);
	dev->dv_parent = ROOT;
	dev->dv_flags = DVF_ACTIVE;	/* always initially active */
	dev->dv_properties = prop_dictionary_create();
	KASSERT(dev->dv_properties != NULL);

	/* put this device in the devices array */
	config_makeroom(dev->dv_unit, cd);
	if (cd->cd_devs[dev->dv_unit])
		panic("config_attach_pseudo: duplicate %s", dev->dv_xname);
	cd->cd_devs[dev->dv_unit] = dev;

#if 0	/* XXXJRT not yet */
#ifdef __HAVE_DEVICE_REGISTER
	device_register(dev, NULL);	/* like a root node */
#endif
#endif
	(*ca->ca_attach)(ROOT, dev, NULL);
	config_process_deferred(&deferred_config_queue, dev);
	return (dev);
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
	struct cftable *ct;
	cfdata_t cf;
	const struct cfattach *ca;
	struct cfdriver *cd;
#ifdef DIAGNOSTIC
	device_t d;
#endif
	int rv = 0, i;

#ifdef DIAGNOSTIC
	if (dev->dv_cfdata != NULL &&
	    dev->dv_cfdata->cf_fstate != FSTATE_FOUND &&
	    dev->dv_cfdata->cf_fstate != FSTATE_STAR)
		panic("config_detach: bad device fstate");
#endif
	cd = dev->dv_cfdriver;
	KASSERT(cd != NULL);

	ca = dev->dv_cfattach;
	KASSERT(ca != NULL);

	/*
	 * Ensure the device is deactivated.  If the device doesn't
	 * have an activation entry point, we allow DVF_ACTIVE to
	 * remain set.  Otherwise, if DVF_ACTIVE is still set, the
	 * device is busy, and the detach fails.
	 */
	if (ca->ca_activate != NULL)
		rv = config_deactivate(dev);

	/*
	 * Try to detach the device.  If that's not possible, then
	 * we either panic() (for the forced but failed case), or
	 * return an error.
	 */
	if (rv == 0) {
		if (ca->ca_detach != NULL)
			rv = (*ca->ca_detach)(dev, flags);
		else
			rv = EOPNOTSUPP;
	}
	if (rv != 0) {
		if ((flags & DETACH_FORCE) == 0)
			return (rv);
		else
			panic("config_detach: forced detach of %s failed (%d)",
			    dev->dv_xname, rv);
	}

	/*
	 * The device has now been successfully detached.
	 */

#ifdef DIAGNOSTIC
	/*
	 * Sanity: If you're successfully detached, you should have no
	 * children.  (Note that because children must be attached
	 * after parents, we only need to search the latter part of
	 * the list.)
	 */
	for (d = TAILQ_NEXT(dev, dv_list); d != NULL;
	    d = TAILQ_NEXT(d, dv_list)) {
		if (d->dv_parent == dev) {
			printf("config_detach: detached device %s"
			    " has children %s\n", dev->dv_xname, d->dv_xname);
			panic("config_detach");
		}
	}
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
#ifdef __BROKEN_CONFIG_UNIT_USAGE
				/*
				 * Note that we can only re-use a starred
				 * unit number if the unit being detached
				 * had the last assigned unit number.
				 */
				if (cf->cf_fstate == FSTATE_STAR &&
				    cf->cf_unit == dev->dv_unit + 1)
					cf->cf_unit--;
#endif /* __BROKEN_CONFIG_UNIT_USAGE */
			}
		}
	}

	/*
	 * Unlink from device list.
	 */
	TAILQ_REMOVE(&alldevs, dev, dv_list);

	/*
	 * Remove from cfdriver's array, tell the world (unless it was
	 * a pseudo-device), and free softc.
	 */
	cd->cd_devs[dev->dv_unit] = NULL;
	if (dev->dv_cfdata != NULL && (flags & DETACH_QUIET) == 0)
		aprint_normal("%s detached\n", dev->dv_xname);
	if (dev->dv_locators)
		free(dev->dv_locators, M_DEVBUF);
	KASSERT(dev->dv_properties != NULL);
	prop_object_release(dev->dv_properties);
	free(dev, M_DEVBUF);

	/*
	 * If the device now has no units in use, deallocate its softc array.
	 */
	for (i = 0; i < cd->cd_ndevs; i++)
		if (cd->cd_devs[i] != NULL)
			break;
	if (i == cd->cd_ndevs) {		/* nothing found; deallocate */
		free(cd->cd_devs, M_DEVBUF);
		cd->cd_devs = NULL;
		cd->cd_ndevs = 0;
	}

	/*
	 * Return success.
	 */
	return (0);
}

int
config_activate(device_t dev)
{
	const struct cfattach *ca = dev->dv_cfattach;
	int rv = 0, oflags = dev->dv_flags;

	if (ca->ca_activate == NULL)
		return (EOPNOTSUPP);

	if ((dev->dv_flags & DVF_ACTIVE) == 0) {
		dev->dv_flags |= DVF_ACTIVE;
		rv = (*ca->ca_activate)(dev, DVACT_ACTIVATE);
		if (rv)
			dev->dv_flags = oflags;
	}
	return (rv);
}

int
config_deactivate(device_t dev)
{
	const struct cfattach *ca = dev->dv_cfattach;
	int rv = 0, oflags = dev->dv_flags;

	if (ca->ca_activate == NULL)
		return (EOPNOTSUPP);

	if (dev->dv_flags & DVF_ACTIVE) {
		dev->dv_flags &= ~DVF_ACTIVE;
		rv = (*ca->ca_activate)(dev, DVACT_DEACTIVATE);
		if (rv)
			dev->dv_flags = oflags;
	}
	return (rv);
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

#ifdef DIAGNOSTIC
	for (dc = TAILQ_FIRST(&deferred_config_queue); dc != NULL;
	     dc = TAILQ_NEXT(dc, dc_queue)) {
		if (dc->dc_dev == dev)
			panic("config_defer: deferred twice");
	}
#endif

	dc = malloc(sizeof(*dc), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (dc == NULL)
		panic("config_defer: unable to allocate callback");

	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&deferred_config_queue, dc, dc_queue);
	config_pending_incr();
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

#ifdef DIAGNOSTIC
	for (dc = TAILQ_FIRST(&interrupt_config_queue); dc != NULL;
	     dc = TAILQ_NEXT(dc, dc_queue)) {
		if (dc->dc_dev == dev)
			panic("config_interrupts: deferred twice");
	}
#endif

	dc = malloc(sizeof(*dc), M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (dc == NULL)
		panic("config_interrupts: unable to allocate callback");

	dc->dc_dev = dev;
	dc->dc_func = func;
	TAILQ_INSERT_TAIL(&interrupt_config_queue, dc, dc_queue);
	config_pending_incr();
}

/*
 * Process a deferred configuration queue.
 */
static void
config_process_deferred(struct deferred_config_head *queue,
    device_t parent)
{
	struct deferred_config *dc, *ndc;

	for (dc = TAILQ_FIRST(queue); dc != NULL; dc = ndc) {
		ndc = TAILQ_NEXT(dc, dc_queue);
		if (parent == NULL || dc->dc_dev->dv_parent == parent) {
			TAILQ_REMOVE(queue, dc, dc_queue);
			(*dc->dc_func)(dc->dc_dev);
			free(dc, M_DEVBUF);
			config_pending_decr();
		}
	}
}

/*
 * Manipulate the config_pending semaphore.
 */
void
config_pending_incr(void)
{

	config_pending++;
}

void
config_pending_decr(void)
{

#ifdef DIAGNOSTIC
	if (config_pending == 0)
		panic("config_pending_decr: config_pending == 0");
#endif
	config_pending--;
	if (config_pending == 0)
		wakeup(&config_pending);
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

	/*
	 * If finalization has already been done, invoke the
	 * callback function now.
	 */
	if (config_finalize_done) {
		while ((*fn)(dev) != 0)
			/* loop */ ;
	}

	/* Ensure this isn't already on the list. */
	TAILQ_FOREACH(f, &config_finalize_list, f_list) {
		if (f->f_func == fn && f->f_dev == dev)
			return (EEXIST);
	}

	f = malloc(sizeof(*f), M_TEMP, M_WAITOK);
	f->f_func = fn;
	f->f_dev = dev;
	TAILQ_INSERT_TAIL(&config_finalize_list, f, f_list);

	return (0);
}

void
config_finalize(void)
{
	struct finalize_hook *f;
	int rv;

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
		free(f, M_TEMP);
	}
}

/*
 * device_lookup:
 *
 *	Look up a device instance for a given driver.
 */
void *
device_lookup(cfdriver_t cd, int unit)
{

	if (unit < 0 || unit >= cd->cd_ndevs)
		return (NULL);
	
	return (cd->cd_devs[unit]);
}

/*
 * Accessor functions for the device_t type.
 */
devclass_t
device_class(device_t dev)
{

	return (dev->dv_class);
}

cfdata_t
device_cfdata(device_t dev)
{

	return (dev->dv_cfdata);
}

cfdriver_t
device_cfdriver(device_t dev)
{

	return (dev->dv_cfdriver);
}

cfattach_t
device_cfattach(device_t dev)
{

	return (dev->dv_cfattach);
}

int
device_unit(device_t dev)
{

	return (dev->dv_unit);
}

const char *
device_xname(device_t dev)
{

	return (dev->dv_xname);
}

device_t
device_parent(device_t dev)
{

	return (dev->dv_parent);
}

boolean_t
device_is_active(device_t dev)
{

	return ((dev->dv_flags & DVF_ACTIVE) != 0);
}

int
device_locator(device_t dev, u_int locnum)
{

	KASSERT(dev->dv_locators != NULL);
	return (dev->dv_locators[locnum]);
}

void *
device_private(device_t dev)
{

	/*
	 * For now, at least, "struct device" is the first thing in
	 * the driver's private data.  So, we just return ourselves.
	 */
	return (dev);
}

prop_dictionary_t
device_properties(device_t dev)
{

	return (dev->dv_properties);
}

/*
 * device_is_a:
 *
 *	Returns true if the device is an instance of the specified
 *	driver.
 */
boolean_t
device_is_a(device_t dev, const char *dname)
{

	return (strcmp(dev->dv_cfdriver->cd_name, dname) == 0);
}
