/* $NetBSD: device.h,v 1.87.2.2 2006/05/24 10:59:21 yamt Exp $ */

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

#include <sys/evcnt.h>
#include <sys/queue.h>

#include <prop/proplib.h>

/*
 * Minimal device structures.
 * Note that all ``system'' device types are listed here.
 */
typedef enum devclass {
	DV_DULL,		/* generic, no special info */
	DV_CPU,			/* CPU (carries resource utilization) */
	DV_DISK,		/* disk drive (label, etc) */
	DV_IFNET,		/* network interface */
	DV_TAPE,		/* tape device */
	DV_TTY			/* serial line interface (?) */
} devclass_t;

/*
 * Actions for ca_activate.
 */
typedef enum devact {
	DVACT_ACTIVATE,		/* activate the device */
	DVACT_DEACTIVATE	/* deactivate the device */
} devact_t;

typedef struct cfdata *cfdata_t;
typedef struct cfdriver *cfdriver_t;
typedef struct cfattach *cfattach_t;
typedef struct device *device_t;

struct device {
	devclass_t	dv_class;	/* this device's classification */
	TAILQ_ENTRY(device) dv_list;	/* entry on list of all devices */
	cfdata_t	dv_cfdata;	/* config data that found us
					   (NULL if pseudo-device) */
	cfdriver_t	dv_cfdriver;	/* our cfdriver */
	cfattach_t	dv_cfattach;	/* our cfattach */
	int		dv_unit;	/* device unit number */
	char		dv_xname[16];	/* external name (name + unit) */
	device_t	dv_parent;	/* pointer to parent device
					   (NULL if pesudo- or root node) */
	int		dv_flags;	/* misc. flags; see below */
	int		*dv_locators;	/* our actual locators (optional) */
	prop_dictionary_t dv_properties;/* properties dictionary */
};

/* dv_flags */
#define	DVF_ACTIVE	0x0001		/* device is activated */

TAILQ_HEAD(devicelist, device);

/*
 * Description of a locator, as part of interface attribute definitions.
 */
struct cflocdesc {
	const char *cld_name;
	const char *cld_defaultstr; /* NULL if no default */
	int cld_default;
};

/*
 * Description of an interface attribute, provided by potential
 * parent device drivers, referred to by child device configuration data.
 */
struct cfiattrdata {
	const char *ci_name;
	int ci_loclen;
	const struct cflocdesc ci_locdesc[
#if defined(__GNUC__) && __GNUC__ <= 2
		0
#endif
	];
};

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
	cfdata_t	ct_cfdata;	/* pointer to cfdata table */
	TAILQ_ENTRY(cftable) ct_list;	/* list linkage */
};
TAILQ_HEAD(cftablelist, cftable);

typedef int (*cfsubmatch_t)(device_t, cfdata_t, const int *, void *);

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
	int	(*ca_match)(device_t, cfdata_t, void *);
	void	(*ca_attach)(device_t, device_t, void *);
	int	(*ca_detach)(device_t, int);
	int	(*ca_activate)(device_t, devact_t);
	/* technically, the next 2 belong into "struct cfdriver" */
	int	(*ca_rescan)(device_t, const char *,
			     const int *); /* scan for new children */
	void	(*ca_childdetached)(device_t, device_t);
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
	const struct cfiattrdata * const *cd_attrs; /* attributes provided */
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
extern struct cftablelist allcftables;	/* list of all cfdata tables */
extern device_t booted_device;		/* the device we booted from */
extern device_t booted_wedge;		/* the wedge on that device */
extern int booted_partition;		/* or the partition on that device */

extern volatile int config_pending; 	/* semaphore for mountroot */

void	config_init(void);
void	configure(void);

int	config_cfdriver_attach(struct cfdriver *);
int	config_cfdriver_detach(struct cfdriver *);

int	config_cfattach_attach(const char *, struct cfattach *);
int	config_cfattach_detach(const char *, struct cfattach *);

int	config_cfdata_attach(cfdata_t, int);
int	config_cfdata_detach(cfdata_t);

struct cfdriver *config_cfdriver_lookup(const char *);
struct cfattach *config_cfattach_lookup(const char *, const char *);
const struct cfiattrdata *cfiattr_lookup(const char *, const struct cfdriver *);

int	config_stdsubmatch(device_t, cfdata_t, const int *, void *);
cfdata_t config_search_loc(cfsubmatch_t, device_t,
				 const char *, const int *, void *);
cfdata_t config_search_ia(cfsubmatch_t, device_t,
				 const char *, void *);
cfdata_t config_rootsearch(cfsubmatch_t, const char *, void *);
device_t config_found_sm_loc(device_t, const char *, const int *,
			     void *, cfprint_t, cfsubmatch_t);
device_t config_found_ia(device_t, const char *, void *, cfprint_t);
device_t config_found(device_t, void *, cfprint_t);
device_t config_rootfound(const char *, void *);
device_t config_attach_loc(device_t, cfdata_t, const int *, void *, cfprint_t);
device_t config_attach(device_t, cfdata_t, void *, cfprint_t);
int	config_match(device_t, cfdata_t, void *);

device_t config_attach_pseudo(cfdata_t);

void	config_makeroom(int n, struct cfdriver *cd);
int	config_detach(device_t, int);
int	config_activate(device_t);
int	config_deactivate(device_t);
void	config_defer(device_t, void (*)(device_t));
void	config_interrupts(device_t, void (*)(device_t));
void	config_pending_incr(void);
void	config_pending_decr(void);

int	config_finalize_register(device_t, int (*)(device_t));
void	config_finalize(void);

void		*device_lookup(cfdriver_t, int);
#ifdef __HAVE_DEVICE_REGISTER
void		device_register(device_t, void *);
#endif

devclass_t	device_class(device_t);
cfdata_t	device_cfdata(device_t);
cfdriver_t	device_cfdriver(device_t);
cfattach_t	device_cfattach(device_t);
int		device_unit(device_t);
const char	*device_xname(device_t);
device_t	device_parent(device_t);
boolean_t	device_is_active(device_t);
int		device_locator(device_t, u_int);
void		*device_private(device_t);
prop_dictionary_t device_properties(device_t);

boolean_t	device_is_a(device_t, const char *);

#endif /* _KERNEL */

#endif /* !_SYS_DEVICE_H_ */
