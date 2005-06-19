/* $NetBSD: device.h,v 1.74 2005/06/19 23:09:50 christos Exp $ */

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
 *	@(#)device.h	8.2 (Berkeley) 2/17/94
 */

#ifndef _SYS_DEVICE_H_
#define	_SYS_DEVICE_H_

#include <sys/properties.h>
#include <sys/queue.h>

/*
 * Minimal device structures.
 * Note that all ``system'' device types are listed here.
 */
enum devclass {
	DV_DULL,		/* generic, no special info */
	DV_CPU,			/* CPU (carries resource utilization) */
	DV_DISK,		/* disk drive (label, etc) */
	DV_IFNET,		/* network interface */
	DV_TAPE,		/* tape device */
	DV_TTY			/* serial line interface (?) */
};

/*
 * Actions for ca_activate.
 */
enum devact {
	DVACT_ACTIVATE,		/* activate the device */
	DVACT_DEACTIVATE	/* deactivate the device */
};

struct device {
	enum	devclass dv_class;	/* this device's classification */
	TAILQ_ENTRY(device) dv_list;	/* entry on list of all devices */
	struct	cfdata *dv_cfdata;	/* config data that found us
					   (NULL if pseudo-device) */
	struct	cfdriver *dv_cfdriver;	/* our cfdriver */
	struct	cfattach *dv_cfattach;	/* our cfattach */
	int	dv_unit;		/* device unit number */
	char	dv_xname[16];		/* external name (name + unit) */
	struct	device *dv_parent;	/* pointer to parent device
					   (NULL if pesudo- or root node) */
	int	dv_flags;		/* misc. flags; see below */
	int	*dv_locators;		/* our actual locators (optional) */
};

/* dv_flags */
#define	DVF_ACTIVE	0x0001		/* device is activated */

TAILQ_HEAD(devicelist, device);

/*
 * `event' counters (use zero or more per device instance, as needed)
 */

struct evcnt {
	u_int64_t	ev_count;	/* how many have occurred */
	TAILQ_ENTRY(evcnt) ev_list;	/* entry on list of all counters */
	unsigned char	ev_type;	/* counter type; see below */
	unsigned char	ev_grouplen;	/* 'group' len, excluding NUL */
	unsigned char	ev_namelen;	/* 'name' len, excluding NUL */
	char		ev_pad1;	/* reserved (for now); 0 */
	const struct evcnt *ev_parent;	/* parent, for hierarchical ctrs */
	const char	*ev_group;	/* name of group */
	const char	*ev_name;	/* name of specific event */
};
TAILQ_HEAD(evcntlist, evcnt);

/* maximum group/name lengths, including trailing NUL */
#define	EVCNT_STRING_MAX	256

/* ev_type values */
#define	EVCNT_TYPE_MISC		0	/* miscellaneous; catch all */
#define	EVCNT_TYPE_INTR		1	/* interrupt; count with vmstat -i */
#define	EVCNT_TYPE_TRAP		2	/* processor trap/execption */

/*
 * initializer for an event count structure.  the lengths are initted and
 * it is added to the evcnt list at attach time.
 */
#define	EVCNT_INITIALIZER(type, parent, group, name)			\
    {									\
	0,			/* ev_count */				\
	{ 0 },			/* ev_list */				\
	type,			/* ev_type */				\
	0,			/* ev_grouplen */			\
	0,			/* ev_namelen */			\
	0,			/* ev_pad1 */				\
	parent,			/* ev_parent */				\
	group,			/* ev_group */				\
	name,			/* ev_name */				\
    }

/*
 * Attach a static event counter.  This uses a link set to do the work.
 * NOTE: "ev" should not be a pointer to the object, but rather a direct
 * reference to the object itself.
 */
#define	EVCNT_ATTACH_STATIC(ev)		__link_set_add_data(evcnts, ev)
#define	EVCNT_ATTACH_STATIC2(ev, n)	__link_set_add_data2(evcnts, ev, n)

/*
 * Description of a configuration parent.  Each device attachment attaches
 * to an "interface attribute", which is given in this structure.  The parent
 * *must* carry this attribute.  Optionally, an individual device instance
 * may also specify a specific parent device instance.
 */
struct cfparent {
	const char *cfp_iattr;		/* interface attribute */
	const char *cfp_parent;		/* optional specific parent */
	int cfp_unit;			/* optional specific unit
					   (-1 to wildcard) */
};

/*
 * Configuration data (i.e., data placed in ioconf.c).
 */
struct cfdata {
	const char *cf_name;		/* driver name */
	const char *cf_atname;		/* attachment name */
	short	cf_unit;		/* unit number */
	short	cf_fstate;		/* finding state (below) */
	int	*cf_loc;		/* locators (machine dependent) */
	int	cf_flags;		/* flags from config */
	const struct cfparent *cf_pspec;/* parent specification */
	const char * const *cf_locnames;/* locator names (machine dependent) */
};
#define FSTATE_NOTFOUND		0	/* has not been found */
#define	FSTATE_FOUND		1	/* has been found */
#define	FSTATE_STAR		2	/* duplicable */
#define FSTATE_DSTAR		3	/* has not been found, and disabled */
#define FSTATE_DNOTFOUND	4	/* duplicate, and disabled */

/*
 * Multiple configuration data tables may be maintained.  This structure
 * provides the linkage.
 */
struct cftable {
	struct cfdata *ct_cfdata;	/* pointer to cfdata table */
	TAILQ_ENTRY(cftable) ct_list;	/* list linkage */
};
TAILQ_HEAD(cftablelist, cftable);

typedef int (*cfmatch_t)(struct device *, struct cfdata *, void *);

/*
 * XXX the "locdesc_t" is unnecessary; the len is known to "config" and
 * should be made available through cfdata->cf_pspec->cfp_iattr.
 * So just an "int *" should do it.
 */
typedef struct {
	int len;
	int locs[1];
} locdesc_t;
typedef int (*cfmatch_loc_t)(struct device *, struct cfdata *,
			     const locdesc_t *, void *);

/*
 * `configuration' attachment and driver (what the machine-independent
 * autoconf uses).  As devices are found, they are applied against all
 * the potential matches.  The one with the best match is taken, and a
 * device structure (plus any other data desired) is allocated.  Pointers
 * to these are placed into an array of pointers.  The array itself must
 * be dynamic since devices can be found long after the machine is up
 * and running.
 *
 * Devices can have multiple configuration attachments if they attach
 * to different attributes (busses, or whatever), to allow specification
 * of multiple match and attach functions.  There is only one configuration
 * driver per driver, so that things like unit numbers and the device
 * structure array will be shared.
 */
struct cfattach {
	const char *ca_name;		/* name of attachment */
	LIST_ENTRY(cfattach) ca_list;	/* link on cfdriver's list */
	size_t	  ca_devsize;		/* size of dev data (for malloc) */
	cfmatch_t ca_match;		/* returns a match level */
	void	(*ca_attach)(struct device *, struct device *, void *);
	int	(*ca_detach)(struct device *, int);
	int	(*ca_activate)(struct device *, enum devact);
	/* technically, the next 2 belong into "struct cfdriver" */
	int	(*ca_rescan)(struct device *, const char *,
			     const int *); /* scan for new children */
	void	(*ca_childdetached)(struct device *, struct device *);
};
LIST_HEAD(cfattachlist, cfattach);

#define	CFATTACH_DECL(name, ddsize, matfn, attfn, detfn, actfn)		\
struct cfattach __CONCAT(name,_ca) = {					\
	___STRING(name), { 0 }, ddsize, matfn, attfn, detfn, actfn, 0, 0 \
}

#define	CFATTACH_DECL2(name, ddsize, matfn, attfn, detfn, actfn, \
	rescanfn, chdetfn) \
struct cfattach __CONCAT(name,_ca) = {					\
	___STRING(name), { 0 }, ddsize, matfn, attfn, detfn, actfn, \
		rescanfn, chdetfn \
}

/* Flags given to config_detach(), and the ca_detach function. */
#define	DETACH_FORCE	0x01		/* force detachment; hardware gone */
#define	DETACH_QUIET	0x02		/* don't print a notice */

struct cfdriver {
	LIST_ENTRY(cfdriver) cd_list;	/* link on allcfdrivers */
	struct cfattachlist cd_attach;	/* list of all attachments */
	void	**cd_devs;		/* devices found */
	const char *cd_name;		/* device name */
	enum	devclass cd_class;	/* device classification */
	int	cd_ndevs;		/* size of cd_devs array */
	const char * const *cd_attrs;	/* attributes for this device */
};
LIST_HEAD(cfdriverlist, cfdriver);

#define	CFDRIVER_DECL(name, class, attrs)				\
struct cfdriver __CONCAT(name,_cd) = {					\
	{ 0 }, { 0 }, NULL, ___STRING(name), class, 0, attrs		\
}

/*
 * The cfattachinit is a data structure used to associate a list of
 * cfattach's with cfdrivers as found in the static kernel configuration.
 */
struct cfattachinit {
	const char *cfai_name;		 /* driver name */
	struct cfattach * const *cfai_list;/* list of attachments */
};
/*
 * the same, but with a non-constant list so it can be modified
 * for LKM bookkeeping
 */
struct cfattachlkminit {
	const char *cfai_name;		/* driver name */
	struct cfattach **cfai_list;	/* list of attachments */
};

/*
 * Configuration printing functions, and their return codes.  The second
 * argument is NULL if the device was configured; otherwise it is the name
 * of the parent device.  The return value is ignored if the device was
 * configured, so most functions can return UNCONF unconditionally.
 */
typedef int (*cfprint_t)(void *, const char *);		/* XXX const char * */
#define	QUIET	0		/* print nothing */
#define	UNCONF	1		/* print " not configured\n" */
#define	UNSUPP	2		/* print " not supported\n" */

/*
 * Pseudo-device attach information (function + number of pseudo-devs).
 */
struct pdevinit {
	void	(*pdev_attach)(int);
	int	pdev_count;
};

/* This allows us to wildcard a device unit. */
#define	DVUNIT_ANY	-1

#ifdef _KERNEL

extern struct cfdriverlist allcfdrivers;/* list of all cfdrivers */
extern struct devicelist alldevs;	/* list of all devices */
extern struct evcntlist allevents;	/* list of all event counters */
extern struct cftablelist allcftables;	/* list of all cfdata tables */
extern struct device *booted_device;	/* the device we booted from */
extern struct device *booted_wedge;	/* the wedge on that device */
extern int booted_partition;		/* or the partition on that device */

extern __volatile int config_pending; 	/* semaphore for mountroot */

extern propdb_t dev_propdb;		/* device properties database */

void	config_init(void);
void	configure(void);

int	config_cfdriver_attach(struct cfdriver *);
int	config_cfdriver_detach(struct cfdriver *);

int	config_cfattach_attach(const char *, struct cfattach *);
int	config_cfattach_detach(const char *, struct cfattach *);

int	config_cfdata_attach(struct cfdata *, int);
int	config_cfdata_detach(struct cfdata *);

struct cfdriver *config_cfdriver_lookup(const char *);
struct cfattach *config_cfattach_lookup(const char *, const char *);

struct cfdata *config_search(cfmatch_t, struct device *, void *);
struct cfdata *config_search_loc(cfmatch_loc_t, struct device *,
				 const char *, const locdesc_t *, void *);
#define config_search_ia(sm, d, ia, a) \
	config_search_loc((sm), (d), (ia), NULL, (a))
struct cfdata *config_rootsearch(cfmatch_t, const char *, void *);
struct device *config_found_sm(struct device *, void *, cfprint_t, cfmatch_t);
struct device *config_found_sm_loc(struct device *,
				   const char *, const locdesc_t *, void *,
				   cfprint_t, cfmatch_loc_t);
#define config_found_ia(d, ia, a, p) \
	config_found_sm_loc((d), (ia), NULL, (a), (p), NULL)
#define config_found(d, a, p) \
	config_found_sm_loc((d), NULL, NULL, (a), (p), NULL)
struct device *config_rootfound(const char *, void *);
struct device *config_attach_loc(struct device *, struct cfdata *,
    const locdesc_t *, void *, cfprint_t);
#define config_attach(p, cf, aux, pr) \
	config_attach_loc((p), (cf), 0, (aux), (pr))
int config_match(struct device *, struct cfdata *, void *);

struct device *config_attach_pseudo(struct cfdata *);

void config_makeroom(int n, struct cfdriver *cd);
int config_detach(struct device *, int);
int config_activate(struct device *);
int config_deactivate(struct device *);
void config_defer(struct device *, void (*)(struct device *));
void config_interrupts(struct device *, void (*)(struct device *));
void config_pending_incr(void);
void config_pending_decr(void);

int config_finalize_register(struct device *, int (*)(struct device *));
void config_finalize(void);

#ifdef __HAVE_DEVICE_REGISTER
void device_register(struct device *, void *);
#endif

void	evcnt_init(void);
void	evcnt_attach_static(struct evcnt *);
void	evcnt_attach_dynamic(struct evcnt *, int, const struct evcnt *,
	    const char *, const char *);
void	evcnt_detach(struct evcnt *);

/* convenience definitions */
#define	device_lookup(cfd, unit)					\
	(((unit) < (cfd)->cd_ndevs) ? (cfd)->cd_devs[(unit)] : NULL)

#ifdef DDB
void event_print(int, void (*)(const char *, ...));
#endif
#endif /* _KERNEL */

#endif /* !_SYS_DEVICE_H_ */
