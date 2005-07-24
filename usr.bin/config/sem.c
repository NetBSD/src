/*	$NetBSD: sem.c,v 1.2 2005/07/24 21:31:02 cube Exp $	*/

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
 *	from: @(#)sem.c	8.1 (Berkeley) 6/6/93
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/param.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "sem.h"

/*
 * config semantics.
 */

#define	NAMESIZE	100	/* local name buffers */

const char *s_ifnet;		/* magic attribute */
const char *s_qmark;
const char *s_none;

static struct hashtab *cfhashtab;	/* for config lookup */
struct hashtab *devitab;		/* etc */

static struct attr errattr;
static struct devbase errdev;
static struct deva errdeva;

static int has_errobj(struct nvlist *, void *);
static struct nvlist *addtoattr(struct nvlist *, struct devbase *);
static int resolve(struct nvlist **, const char *, const char *,
		   struct nvlist *, int);
static struct pspec *getpspec(struct attr *, struct devbase *, int);
static struct devi *newdevi(const char *, int, struct devbase *d);
static struct devi *getdevi(const char *);
static const char *concat(const char *, int);
static char *extend(char *, const char *);
static int split(const char *, size_t, char *, size_t, int *);
static void selectbase(struct devbase *, struct deva *);
static int onlist(struct nvlist *, void *);
static const char **fixloc(const char *, struct attr *, struct nvlist *);
static const char *makedevstr(int, int);
static const char *major2name(int);
static int dev2major(struct devbase *);

extern const char *yyfile;

void
initsem(void)
{

	attrtab = ht_new();
	errattr.a_name = "<internal>";

	TAILQ_INIT(&allbases);

	TAILQ_INIT(&alldevas);

	TAILQ_INIT(&allpspecs);

	cfhashtab = ht_new();
	TAILQ_INIT(&allcf);

	TAILQ_INIT(&alldevi);
	errdev.d_name = "<internal>";

	TAILQ_INIT(&allpseudo);

	TAILQ_INIT(&alldevms);

	s_ifnet = intern("ifnet");
	s_qmark = intern("?");
	s_none = intern("none");
}

/* Name of include file just ended (set in scan.l) */
extern const char *lastfile;

void
enddefs(void)
{
	struct devbase *dev;

	TAILQ_FOREACH(dev, &allbases, d_next) {
		if (!dev->d_isdef) {
			(void)fprintf(stderr,
			    "%s: device `%s' used but not defined\n",
			    lastfile, dev->d_name);
			errors++;
			continue;
		}
	}
	if (errors) {
		(void)fprintf(stderr, "*** Stop.\n");
		exit(1);
	}
}

void
setdefmaxusers(int min, int def, int max)
{

	if (min < 1 || min > def || def > max)
		error("maxusers must have 1 <= min (%d) <= default (%d) <= max (%d)", min, def, max);
	else {
		minmaxusers = min;
		defmaxusers = def;
		maxmaxusers = max;
	}
}

void
setmaxusers(int n)
{

	if (maxusers != 0) {
		error("duplicate maxusers parameter");
		return;
	}
	maxusers = n;
	if (n < minmaxusers) {
		error("warning: minimum of %d maxusers assumed", minmaxusers);
		errors--;	/* take it away */
		maxusers = minmaxusers;
	} else if (n > maxmaxusers) {
		error("warning: maxusers (%d) > %d", n, maxmaxusers);
		errors--;
	}
}

void
setident(const char *i)
{

	ident = intern(i);
}

/*
 * Define an attribute, optionally with an interface (a locator list)
 * and a set of attribute-dependencies.
 *
 * Attribute dependencies MAY NOT be interface attributes.
 *
 * Since an empty locator list is logically different from "no interface",
 * all locator lists include a dummy head node, which we discard here.
 */
int
defattr(const char *name, struct nvlist *locs, struct nvlist *deps,
    int devclass)
{
	struct attr *a, *dep;
	struct nvlist *nv;
	int len;

	if (locs != NULL && devclass)
		panic("defattr(%s): locators and devclass", name);

	if (deps != NULL && devclass)
		panic("defattr(%s): dependencies and devclass", name);

	/*
	 * If this attribute depends on any others, make sure none of
	 * the dependencies are interface attributes.
	 */
	for (nv = deps; nv != NULL; nv = nv->nv_next) {
		dep = nv->nv_ptr;
		if (dep->a_iattr) {
			error("`%s' dependency `%s' is an interface attribute",
			    name, dep->a_name);
			return (1);
		}
	}

	a = ecalloc(1, sizeof *a);
	if (ht_insert(attrtab, name, a)) {
		free(a);
		error("attribute `%s' already defined", name);
		nvfreel(locs);
		return (1);
	}

	a->a_name = name;
	if (locs != NULL) {
		a->a_iattr = 1;
		a->a_locs = locs->nv_next;
		nvfree(locs);
	} else {
		a->a_iattr = 0;
		a->a_locs = NULL;
	}
	if (devclass) {
		size_t l = strlen(name) + 4;
		char *classenum = alloca(l), *cp;
		int errored = 0;

		strlcpy(classenum, "DV_", l);
		strlcat(classenum, name, l);
		for (cp = classenum + 3; *cp; cp++) {
			if (!errored &&
			    (!isalnum((unsigned char)*cp) ||
			      (isalpha((unsigned char)*cp) && !islower((unsigned char)*cp)))) {
				error("device class names must be lower-case alphanumeric characters");
				errored = 1;
			}
			*cp = toupper((unsigned char)*cp);
		}
		a->a_devclass = intern(classenum);
	} else
		a->a_devclass = NULL;
	len = 0;
	for (nv = a->a_locs; nv != NULL; nv = nv->nv_next)
		len++;
	a->a_loclen = len;
	a->a_devs = NULL;
	a->a_refs = NULL;
	a->a_deps = deps;
	a->a_expanding = 0;

	/* Expand the attribute to check for cycles in the graph. */
	expandattr(a, NULL);

	return (0);
}

/*
 * Return true if the given `error object' is embedded in the given
 * pointer list.
 */
static int
has_errobj(struct nvlist *nv, void *obj)
{

	for (; nv != NULL; nv = nv->nv_next)
		if (nv->nv_ptr == obj)
			return (1);
	return (0);
}

/*
 * Return true if the given attribute is embedded in the given
 * pointer list.
 */
int
has_attr(struct nvlist *nv, const char *attr)
{
	struct attr *a;

	if ((a = getattr(attr)) == NULL)
		return (0);

	for (; nv != NULL; nv = nv->nv_next)
		if (nv->nv_ptr == a)
			return (1);
	return (0);
}

/*
 * Add a device base to a list in an attribute (actually, to any list).
 * Note that this does not check for duplicates, and does reverse the
 * list order, but no one cares anyway.
 */
static struct nvlist *
addtoattr(struct nvlist *l, struct devbase *dev)
{
	struct nvlist *n;

	n = newnv(NULL, NULL, dev, 0, l);
	return (n);
}

/*
 * Define a device.  This may (or may not) also define an interface
 * attribute and/or refer to existing attributes.
 */
void
defdev(struct devbase *dev, struct nvlist *loclist, struct nvlist *attrs,
       int ispseudo)
{
	struct nvlist *nv;
	struct attr *a;

	if (dev == &errdev)
		goto bad;
	if (dev->d_isdef) {
		error("redefinition of `%s'", dev->d_name);
		goto bad;
	}

	dev->d_isdef = 1;
	if (has_errobj(attrs, &errattr))
		goto bad;

	/*
	 * Handle implicit attribute definition from locator list.  Do
	 * this before scanning the `at' list so that we can have, e.g.:
	 *	device foo at other, foo { slot = -1 }
	 * (where you can plug in a foo-bus extender to a foo-bus).
	 */
	if (loclist != NULL) {
		nv = loclist;
		loclist = NULL;	/* defattr disposes of them for us */
		if (defattr(dev->d_name, nv, NULL, 0))
			goto bad;
		attrs = newnv(dev->d_name, NULL, getattr(dev->d_name), 0,
		    attrs);
	}

	/* Committed!  Set up fields. */
	dev->d_ispseudo = ispseudo;
	dev->d_attrs = attrs;
	dev->d_classattr = NULL;		/* for now */

	/*
	 * For each interface attribute this device refers to, add this
	 * device to its reference list.  This makes, e.g., finding all
	 * "scsi"s easier.
	 *
	 * While looking through the attributes, set up the device
	 * class if any are devclass attributes (and error out if the
	 * device has two classes).
	 */
	for (nv = attrs; nv != NULL; nv = nv->nv_next) {
		a = nv->nv_ptr;
		if (a->a_iattr)
			a->a_refs = addtoattr(a->a_refs, dev);
		if (a->a_devclass != NULL) {
			if (dev->d_classattr != NULL) {
				error("device `%s' has multiple classes (`%s' and `%s')",
				    dev->d_name, dev->d_classattr->a_name,
				    a->a_name);
			}
			dev->d_classattr = a;
		}
	}
	return;
 bad:
	nvfreel(loclist);
	nvfreel(attrs);
}

/*
 * Look up a devbase.  Also makes sure it is a reasonable name,
 * i.e., does not end in a digit or contain special characters.
 */
struct devbase *
getdevbase(const char *name)
{
	u_char *p;
	struct devbase *dev;

	p = (u_char *)name;
	if (!isalpha(*p))
		goto badname;
	while (*++p) {
		if (!isalnum(*p) && *p != '_')
			goto badname;
	}
	if (isdigit(*--p)) {
 badname:
		error("bad device base name `%s'", name);
		return (&errdev);
	}
	dev = ht_lookup(devbasetab, name);
	if (dev == NULL) {
		dev = ecalloc(1, sizeof *dev);
		dev->d_name = name;
		dev->d_isdef = 0;
		dev->d_major = NODEV;
		dev->d_attrs = NULL;
		dev->d_ihead = NULL;
		dev->d_ipp = &dev->d_ihead;
		dev->d_ahead = NULL;
		dev->d_app = &dev->d_ahead;
		dev->d_umax = 0;
		TAILQ_INSERT_TAIL(&allbases, dev, d_next);
		if (ht_insert(devbasetab, name, dev))
			panic("getdevbase(%s)", name);
	}
	return (dev);
}

/*
 * Define some of a device's allowable parent attachments.
 * There may be a list of (plain) attributes.
 */
void
defdevattach(struct deva *deva, struct devbase *dev, struct nvlist *atlist,
	     struct nvlist *attrs)
{
	struct nvlist *nv;
	struct attr *a;
	struct deva *da;

	if (dev == &errdev)
		goto bad;
	if (deva == NULL)
		deva = getdevattach(dev->d_name);
	if (deva == &errdeva)
		goto bad;
	if (!dev->d_isdef) {
		error("attaching undefined device `%s'", dev->d_name);
		goto bad;
	}
	if (deva->d_isdef) {
		error("redefinition of `%s'", deva->d_name);
		goto bad;
	}
	if (dev->d_ispseudo) {
		error("pseudo-devices can't attach");
		goto bad;
	}

	deva->d_isdef = 1;
	if (has_errobj(attrs, &errattr))
		goto bad;
	for (nv = attrs; nv != NULL; nv = nv->nv_next) {
		a = nv->nv_ptr;
		if (a == &errattr)
			continue;		/* already complained */
		if (a->a_iattr || a->a_devclass != NULL)
			error("`%s' is not a plain attribute", a->a_name);
	}

	/* Committed!  Set up fields. */
	deva->d_attrs = attrs;
	deva->d_atlist = atlist;
	deva->d_devbase = dev;

	/*
	 * Turn the `at' list into interface attributes (map each
	 * nv_name to an attribute, or to NULL for root), and add
	 * this device to those attributes, so that children can
	 * be listed at this particular device if they are supported
	 * by that attribute.
	 */
	for (nv = atlist; nv != NULL; nv = nv->nv_next) {
		if (nv->nv_name == NULL)
			nv->nv_ptr = a = NULL;	/* at root */
		else
			nv->nv_ptr = a = getattr(nv->nv_name);
		if (a == &errattr)
			continue;		/* already complained */

		/*
		 * Make sure that an attachment spec doesn't
		 * already say how to attach to this attribute.
		 */
		for (da = dev->d_ahead; da != NULL; da = da->d_bsame)
			if (onlist(da->d_atlist, a))
				error("attach at `%s' already done by `%s'",
				     a ? a->a_name : "root", da->d_name);

		if (a == NULL)
			continue;		/* at root; don't add */
		if (!a->a_iattr)
			error("%s cannot be at plain attribute `%s'",
			    dev->d_name, a->a_name);
		else
			a->a_devs = addtoattr(a->a_devs, dev);
	}

	/* attach to parent */
	*dev->d_app = deva;
	dev->d_app = &deva->d_bsame;
	return;
 bad:
	nvfreel(atlist);
	nvfreel(attrs);
}

/*
 * Look up a device attachment.  Also makes sure it is a reasonable
 * name, i.e., does not contain digits or special characters.
 */
struct deva *
getdevattach(const char *name)
{
	u_char *p;
	struct deva *deva;

	p = (u_char *)name;
	if (!isalpha(*p))
		goto badname;
	while (*++p) {
		if (!isalnum(*p) && *p != '_')
			goto badname;
	}
	if (isdigit(*--p)) {
 badname:
		error("bad device attachment name `%s'", name);
		return (&errdeva);
	}
	deva = ht_lookup(devatab, name);
	if (deva == NULL) {
		deva = ecalloc(1, sizeof *deva);
		deva->d_name = name;
		deva->d_bsame = NULL;
		deva->d_isdef = 0;
		deva->d_devbase = NULL;
		deva->d_atlist = NULL;
		deva->d_attrs = NULL;
		deva->d_ihead = NULL;
		deva->d_ipp = &deva->d_ihead;
		TAILQ_INSERT_TAIL(&alldevas, deva, d_next);
		if (ht_insert(devatab, name, deva))
			panic("getdeva(%s)", name);
	}
	return (deva);
}

/*
 * Look up an attribute.
 */
struct attr *
getattr(const char *name)
{
	struct attr *a;

	if ((a = ht_lookup(attrtab, name)) == NULL) {
		error("undefined attribute `%s'", name);
		a = &errattr;
	}
	return (a);
}

/*
 * Recursively expand an attribute and its dependencies, checking for
 * cycles, and invoking a callback for each attribute found.
 */
void
expandattr(struct attr *a, void (*callback)(struct attr *))
{
	struct nvlist *nv;
	struct attr *dep;

	if (a->a_expanding) {
		error("circular dependency on attribute `%s'", a->a_name);
		return;
	}

	a->a_expanding = 1;

	/* First expand all of this attribute's dependencies. */
	for (nv = a->a_deps; nv != NULL; nv = nv->nv_next) {
		dep = nv->nv_ptr;
		expandattr(dep, callback);
	}

	/* ...and now invoke the callback for ourself. */
	if (callback != NULL)
		(*callback)(a);

	a->a_expanding = 0;
}

/*
 * Set the major device number for a device, so that it can be used
 * as a root/dumps "on" device in a configuration.
 */
void
setmajor(struct devbase *d, int n)
{

	if (d != &errdev && d->d_major != NODEV)
		error("device `%s' is already major %d",
		    d->d_name, d->d_major);
	else
		d->d_major = n;
}

const char *
major2name(int maj)
{
	struct devbase *dev;
	struct devm *dm;

	if (!do_devsw) {
		TAILQ_FOREACH(dev, &allbases, d_next) {
			if (dev->d_major == maj)
				return (dev->d_name);
		}
	} else {
		TAILQ_FOREACH(dm, &alldevms, dm_next) {
			if (dm->dm_bmajor == maj)
				return (dm->dm_name);
		}
	}
	return (NULL);
}

int
dev2major(struct devbase *dev)
{
	struct devm *dm;

	if (!do_devsw)
		return (dev->d_major);

	TAILQ_FOREACH(dm, &alldevms, dm_next) {
		if (strcmp(dm->dm_name, dev->d_name) == 0)
			return (dm->dm_bmajor);
	}
	return (NODEV);
}

/*
 * Make a string description of the device at maj/min.
 */
static const char *
makedevstr(int maj, int min)
{
	const char *devname;
	char buf[32];

	devname = major2name(maj);
	if (devname == NULL)
		(void)snprintf(buf, sizeof(buf), "<%d/%d>", maj, min);
	else
		(void)snprintf(buf, sizeof(buf), "%s%d%c", devname,
		    min / maxpartitions, (min % maxpartitions) + 'a');

	return (intern(buf));
}

/*
 * Map things like "ra0b" => makedev(major("ra"), 0*maxpartitions + 'b'-'a').
 * Handle the case where the device number is given but there is no
 * corresponding name, and map NULL to the default.
 */
static int
resolve(struct nvlist **nvp, const char *name, const char *what,
	struct nvlist *dflt, int part)
{
	struct nvlist *nv;
	struct devbase *dev;
	const char *cp;
	int maj, min, i, l;
	int unit;
	char buf[NAMESIZE];

	if ((u_int)(part -= 'a') >= maxpartitions)
		panic("resolve");
	if ((nv = *nvp) == NULL) {
		dev_t	d = NODEV;
		/*
		 * Apply default.  Easiest to do this by number.
		 * Make sure to retain NODEVness, if this is dflt's disposition.
		 */
		if (dflt->nv_int != NODEV) {
			maj = major(dflt->nv_int);
			min = ((minor(dflt->nv_int) / maxpartitions) *
			    maxpartitions) + part;
			d = makedev(maj, min);
			cp = makedevstr(maj, min);
		} else
			cp = NULL;
		*nvp = nv = newnv(NULL, cp, NULL, d, NULL);
	}
	if (nv->nv_int != NODEV) {
		/*
		 * By the numbers.  Find the appropriate major number
		 * to make a name.
		 */
		maj = major(nv->nv_int);
		min = minor(nv->nv_int);
		nv->nv_str = makedevstr(maj, min);
		return (0);
	}

	if (nv->nv_str == NULL || nv->nv_str == s_qmark)
		/*
		 * Wildcarded or unspecified; leave it as NODEV.
		 */
		return (0);

	/*
	 * The normal case: things like "ra2b".  Check for partition
	 * suffix, remove it if there, and split into name ("ra") and
	 * unit (2).
	 */
	l = i = strlen(nv->nv_str);
	cp = &nv->nv_str[l];
	if (l > 1 && *--cp >= 'a' && *cp < 'a' + maxpartitions &&
	    isdigit((unsigned char)cp[-1])) {
		l--;
		part = *cp - 'a';
	}
	cp = nv->nv_str;
	if (split(cp, l, buf, sizeof buf, &unit)) {
		error("%s: invalid %s device name `%s'", name, what, cp);
		return (1);
	}
	dev = ht_lookup(devbasetab, intern(buf));
	if (dev == NULL) {
		error("%s: device `%s' does not exist", name, buf);
		return (1);
	}

	/*
	 * Check for the magic network interface attribute, and
	 * don't bother making a device number.
	 */
	if (has_attr(dev->d_attrs, s_ifnet)) {
		nv->nv_int = NODEV;
		nv->nv_ifunit = unit;	/* XXX XXX XXX */
	} else {
		maj = dev2major(dev);
		if (maj == NODEV) {
			error("%s: can't make %s device from `%s'",
			    name, what, nv->nv_str);
			return (1);
		}
		nv->nv_int = makedev(maj, unit * maxpartitions + part);
	}

	nv->nv_name = dev->d_name;
	return (0);
}

/*
 * Add a completed configuration to the list.
 */
void
addconf(struct config *cf0)
{
	struct config *cf;
	struct nvlist *nv;
	const char *name;

	name = cf0->cf_name;
	cf = ecalloc(1, sizeof *cf);
	if (ht_insert(cfhashtab, name, cf)) {
		error("configuration `%s' already defined", name);
		free(cf);
		goto bad;
	}
	*cf = *cf0;

	/*
	 * Resolve the root device.
	 */
	if (cf->cf_root->nv_str != s_qmark) {
		nv = cf->cf_root;
		if (nv == NULL) {
			error("%s: no root device specified", name);
			goto bad;
		}
		if (resolve(&cf->cf_root, name, "root", nv, 'a'))
			goto bad;
	}

	/*
	 * Resolve the dump device.
	 */
	if (cf->cf_dump == NULL || cf->cf_dump->nv_str == s_qmark) {
		/*
		 * Wildcarded dump device is equivalent to unspecified.
		 */
		cf->cf_dump = NULL;
	} else if (cf->cf_dump->nv_str == s_none) {
		/*
		 * Operator has requested that no dump device should be
		 * configured; do nothing.
		 */
	} else {
		if (resolve(&cf->cf_dump, name, "dumps", cf->cf_dump, 'b'))
			goto bad;
	}

	/* Wildcarded fstype is `unspecified'. */
	if (cf->cf_fstype == s_qmark)
		cf->cf_fstype = NULL;

	TAILQ_INSERT_TAIL(&allcf, cf, cf_next);
	return;
 bad:
	nvfreel(cf0->cf_root);
	nvfreel(cf0->cf_dump);
}

void
setconf(struct nvlist **npp, const char *what, struct nvlist *v)
{

	if (*npp != NULL) {
		error("duplicate %s specification", what);
		nvfreel(v);
	} else
		*npp = v;
}

void
setfstype(const char **fstp, const char *v)
{

	if (*fstp != NULL) {
		error("multiple fstype specifications");
		return;
	}

	if (v != s_qmark && OPT_FSOPT(v)) {
		error("\"%s\" is not a configured file system", v);
		return;
	}

	*fstp = v;
}

static struct devi *
newdevi(const char *name, int unit, struct devbase *d)
{
	struct devi *i;

	i = ecalloc(1, sizeof *i);
	i->i_name = name;
	i->i_unit = unit;
	i->i_base = d;
	i->i_bsame = NULL;
	i->i_asame = NULL;
	i->i_alias = NULL;
	i->i_at = NULL;
	i->i_pspec = NULL;
	i->i_atdeva = NULL;
	i->i_locs = NULL;
	i->i_cfflags = 0;
	i->i_lineno = currentline();
	if (unit >= d->d_umax)
		d->d_umax = unit + 1;
	return (i);
}

/*
 * Add the named device as attaching to the named attribute (or perhaps
 * another device instead) plus unit number.
 */
void
adddev(const char *name, const char *at, struct nvlist *loclist, int flags)
{
	struct devi *i;		/* the new instance */
	struct pspec *p;	/* and its pspec */
	struct attr *attr;	/* attribute that allows attach */
	struct devbase *ib;	/* i->i_base */
	struct devbase *ab;	/* not NULL => at another dev */
	struct nvlist *nv;
	struct deva *iba;	/* devbase attachment used */
	const char *cp;
	int atunit;
	char atbuf[NAMESIZE];
	int hit;

	ab = NULL;
	iba = NULL;
	if (at == NULL) {
		/* "at root" */
		p = NULL;
		if ((i = getdevi(name)) == NULL)
			goto bad;
		/*
		 * Must warn about i_unit > 0 later, after taking care of
		 * the STAR cases (we could do non-star's here but why
		 * bother?).  Make sure this device can be at root.
		 */
		ib = i->i_base;
		hit = 0;
		for (iba = ib->d_ahead; iba != NULL; iba = iba->d_bsame)
			if (onlist(iba->d_atlist, NULL)) {
				hit = 1;
				break;
			}
		if (!hit) {
			error("`%s' cannot attach to the root", ib->d_name);
			goto bad;
		}
		attr = &errattr;	/* a convenient "empty" attr */
	} else {
		if (split(at, strlen(at), atbuf, sizeof atbuf, &atunit)) {
			error("invalid attachment name `%s'", at);
			/* (void)getdevi(name); -- ??? */
			goto bad;
		}
		if ((i = getdevi(name)) == NULL)
			goto bad;
		ib = i->i_base;

		/*
		 * Devices can attach to two types of things: Attributes,
		 * and other devices (which have the appropriate attributes
		 * to allow attachment).
		 *
		 * (1) If we're attached to an attribute, then we don't need
		 *     look at the parent base device to see what attributes
		 *     it has, and make sure that we can attach to them.    
		 *
		 * (2) If we're attached to a real device (i.e. named in
		 *     the config file), we want to remember that so that
		 *     at cross-check time, if the device we're attached to
		 *     is missing but other devices which also provide the
		 *     attribute are present, we don't get a false "OK."
		 *
		 * (3) If the thing we're attached to is an attribute
		 *     but is actually named in the config file, we still
		 *     have to remember its devbase.
		 */
		cp = intern(atbuf);

		/* Figure out parent's devbase, to satisfy case (3). */
		ab = ht_lookup(devbasetab, cp);

		/* Find out if it's an attribute. */
		attr = ht_lookup(attrtab, cp);

		/* Make sure we're _really_ attached to the attr.  Case (1). */
		if (attr != NULL && onlist(attr->a_devs, ib))
			goto findattachment;

		/*
		 * Else a real device, and not just an attribute.  Case (2).
		 *
		 * Have to work a bit harder to see whether we have
		 * something like "tg0 at esp0" (where esp is merely
		 * not an attribute) or "tg0 at nonesuch0" (where
		 * nonesuch is not even a device).
		 */
		if (ab == NULL) {
			error("%s at %s: `%s' unknown",
			    name, at, atbuf);
			goto bad;
		}

		/*
		 * See if the named parent carries an attribute
		 * that allows it to supervise device ib.
		 */
		for (nv = ab->d_attrs; nv != NULL; nv = nv->nv_next) {
			attr = nv->nv_ptr;
			if (onlist(attr->a_devs, ib))
				goto findattachment;
		}
		error("`%s' cannot attach to `%s'", ib->d_name, atbuf);
		goto bad;

 findattachment:
		/*
		 * Find the parent spec.  If a matching one has not yet been
		 * created, create one.
		 */
		p = getpspec(attr, ab, atunit);
		p->p_devs = newnv(NULL, NULL, i, 0, p->p_devs);

		/* find out which attachment it uses */
		hit = 0;
		for (iba = ib->d_ahead; iba != NULL; iba = iba->d_bsame)
			if (onlist(iba->d_atlist, attr)) {
				hit = 1;
				break;
			}
		if (!hit)
			panic("adddev: can't figure out attachment");
	}
	if ((i->i_locs = fixloc(name, attr, loclist)) == NULL)
		goto bad;
	i->i_at = at;
	i->i_pspec = p;
	i->i_atdeva = iba;
	i->i_cfflags = flags;

	*iba->d_ipp = i;
	iba->d_ipp = &i->i_asame;

	/* all done, fall into ... */
 bad:
	nvfreel(loclist);
	return;
}

void
deldev(const char *name, const char *at)
{
	struct devi *firsti, *i, *match, *previ, **ppi;
	struct deva *iba;
	struct devbase *d;
	int unit;
	char base[NAMESIZE];

	if (split(name, strlen(name), base, sizeof base, &unit)) {
		error("invalid device name `%s'", name);
		return;
	}
	d = ht_lookup(devbasetab, intern(base));
	if (d == NULL) {
		error("%s: unknown device `%s'", name, base);
		return;
	}
	if (d->d_ispseudo) {
		error("%s: %s is a pseudo-device", name, base);
		return;
	}
	if ((firsti = ht_lookup(devitab, name)) == NULL) {
		error("`%s' not defined", name);
		return;
	}
	match = NULL;
	previ = NULL;
	if (at == NULL)
		/* 'at root' */
		match = firsti;
	else {
		if (strcmp(at, firsti->i_at) == 0) {
			match = firsti;
		} else {
			for (i = firsti; i != NULL; i = i->i_alias) {
				if (strcmp(at, i->i_at) == 0) {
					match = i;
					break;
				}
				previ = i;
			}
		}
		if (match == NULL) {
			error("`%s' at `%s' not found", name, at);
			return;
		}
	}

	i = match;
	/*
	 * We have the device instance, i.
	 * We have to:
	 *   - delete the alias
	 *
	 *      If the devi was an alias of an already listed devi, all is
	 *      good we don't have to do more.
	 *      If it was the first alias, we have to replace i's entry in
	 *      d's list by its first alias.
	 *      If it was the only entry, we must remove i's entry from d's
	 *      list.
	 */
	if (previ != NULL)
		previ->i_alias = i->i_alias;
	else {
		if (i->i_alias == NULL)
			/* No alias, must unlink the entry from devitab */
			ht_remove(devitab, i->i_name);
		else
			/* Or have the first alias replace i in d's list */
			i->i_alias->i_bsame = i->i_bsame;
		/*
		 *   - remove/replace the instance from the devbase's list
		 *
		 * A double-linked list would make this much easier.  Oh, well,
		 * what is done is done.
		 */
		ppi = &d->d_ihead;
		for (ppi = &d->d_ihead;
		    *ppi != NULL && *ppi != i && (*ppi)->i_bsame != i;
		    ppi = &(*ppi)->i_bsame);
		if (*ppi == NULL)
			panic("deldev: dev (%s) doesn't list the devi (%s at %s)",
			    d->d_name, i->i_name, i->i_at);
		if (*ppi == i)
			*ppi = i->i_bsame;
		else
			(*ppi)->i_bsame = i->i_bsame;
		if (d->d_ipp == &i->i_bsame) {
			if (d->d_ihead == i)
				d->d_ipp = &d->d_ihead;
			else
				d->d_ipp = &(*ppi)->i_bsame;
		}
	}
	/*
	 *   - delete the attachment instance
	 */
	iba = i->i_atdeva;
	ppi = &iba->d_ihead;
	for (ppi = &iba->d_ihead;
	    *ppi != NULL && *ppi != i && (*ppi)->i_asame != i;
	    ppi = &(*ppi)->i_asame);
	if (*ppi == NULL)
		panic("deldev: deva (%s) doesn't list the devi (%s)",
		    iba->d_name, i->i_name);
	if (*ppi == i)
		*ppi = i->i_asame;
	else
		(*ppi)->i_asame = i->i_asame;
	if (iba->d_ipp == &i->i_asame) {
		if (iba->d_ihead == i)
			iba->d_ipp = &iba->d_ihead;
		else
			iba->d_ipp = &(*ppi)->i_asame;
	}
	/*
	 *   - delete the pspec
	 */
	if (i->i_pspec) {
		struct pspec *p = i->i_pspec;
		struct nvlist *nv, *onv;

		/* Double-linked nvlist anyone? */
		for (nv = p->p_devs; nv->nv_ptr != NULL; nv = nv->nv_next) {
			if (nv->nv_next && nv->nv_next->nv_ptr == i) {
				onv = nv->nv_next;
				nv->nv_next = onv->nv_next;
				nvfree(onv);
				break;
			} if (nv->nv_ptr = i) {
				/* nv is p->p_devs in that case */
				p->p_devs = nv->nv_next;
				nvfree(nv);
				break;
			}
		}
		if (p->p_devs == NULL)
			TAILQ_REMOVE(&allpspecs, p, p_list);
	}
	/*
	 *   - delete the alldevi entry
	 */
	TAILQ_REMOVE(&alldevi, i, i_next);
	ndevi--;
	/*
	 *   - reconstuct d->d_umax
	 */
	d->d_umax = 0;
	for (i = d->d_ihead; i != NULL; i = i->i_bsame)
		if (i->i_unit >= d->d_umax)
			d->d_umax = i->i_unit + 1;
}

void
addpseudo(const char *name, int number)
{
	struct devbase *d;
	struct devi *i;

	d = ht_lookup(devbasetab, name);
	if (d == NULL) {
		error("undefined pseudo-device %s", name);
		return;
	}
	if (!d->d_ispseudo) {
		error("%s is a real device, not a pseudo-device", name);
		return;
	}
	if (ht_lookup(devitab, name) != NULL) {
		error("`%s' already defined", name);
		return;
	}
	i = newdevi(name, number - 1, d);	/* foo 16 => "foo0..foo15" */
	if (ht_insert(devitab, name, i))
		panic("addpseudo(%s)", name);
	TAILQ_INSERT_TAIL(&allpseudo, i, i_next);
}

void
delpseudo(const char *name)
{
	struct devbase *d;
	struct devi *i;

	d = ht_lookup(devbasetab, name);
	if (d == NULL) {
		error("undefined pseudo-device %s", name);
		return;
	}
	if (!d->d_ispseudo) {
		error("%s is a real device, not a pseudo-device", name);
		return;
	}
	if ((i = ht_lookup(devitab, name)) == NULL) {
		error("`%s' not defined", name);
		return;
	}
	d->d_umax = 0;		/* clear neads-count entries */
	TAILQ_REMOVE(&allpseudo, i, i_next);
	if (ht_remove(devitab, name))
		panic("delpseudo(%s) - can't remove from devitab", name);
}

void
adddevm(const char *name, int cmajor, int bmajor, struct nvlist *options)
{
	struct devm *dm;

	if (cmajor < -1 || cmajor >= 4096) {
		error("character major %d is invalid", cmajor);
		nvfreel(options);
		return;
	}

	if (bmajor < -1 || bmajor >= 4096) {
		error("block major %d is invalid", bmajor);
		nvfreel(options);
		return;
	}
	if (cmajor == -1 && bmajor == -1) {
		error("both character/block majors are not specified");
		nvfreel(options);
		return;
	}

	dm = ecalloc(1, sizeof(*dm));
	dm->dm_srcfile = yyfile;
	dm->dm_srcline = currentline();
	dm->dm_name = name;
	dm->dm_cmajor = cmajor;
	dm->dm_bmajor = bmajor;
	dm->dm_opts = options;

	TAILQ_INSERT_TAIL(&alldevms, dm, dm_next);

	maxcdevm = MAX(maxcdevm, dm->dm_cmajor);
	maxbdevm = MAX(maxbdevm, dm->dm_bmajor);
}

void
fixdevis(void)
{
	struct devi *i;

	TAILQ_FOREACH(i, &alldevi, i_next)
		selectbase(i->i_base, i->i_atdeva);

	TAILQ_FOREACH(i, &allpseudo, i_next)
		selectbase(i->i_base, NULL);
}

/*
 * Look up a parent spec, creating a new one if it does not exist.
 */
static struct pspec *
getpspec(struct attr *attr, struct devbase *ab, int atunit)
{
	struct pspec *p;

	TAILQ_FOREACH(p, &allpspecs, p_list) {
		if (p->p_iattr == attr &&
		    p->p_atdev == ab &&
		    p->p_atunit == atunit)
			return (p);
	}

	p = ecalloc(1, sizeof(*p));

	p->p_iattr = attr;
	p->p_atdev = ab;
	p->p_atunit = atunit;
	p->p_inst = npspecs++;

	TAILQ_INSERT_TAIL(&allpspecs, p, p_list);

	return (p);
}

/*
 * Define a new instance of a specific device.
 */
static struct devi *
getdevi(const char *name)
{
	struct devi *i, *firsti;
	struct devbase *d;
	int unit;
	char base[NAMESIZE];

	if (split(name, strlen(name), base, sizeof base, &unit)) {
		error("invalid device name `%s'", name);
		return (NULL);
	}
	d = ht_lookup(devbasetab, intern(base));
	if (d == NULL) {
		error("%s: unknown device `%s'", name, base);
		return (NULL);
	}
	if (d->d_ispseudo) {
		error("%s: %s is a pseudo-device", name, base);
		return (NULL);
	}
	firsti = ht_lookup(devitab, name);
	i = newdevi(name, unit, d);
	if (firsti == NULL) {
		if (ht_insert(devitab, name, i))
			panic("getdevi(%s)", name);
		*d->d_ipp = i;
		d->d_ipp = &i->i_bsame;
	} else {
		while (firsti->i_alias)
			firsti = firsti->i_alias;
		firsti->i_alias = i;
	}
	TAILQ_INSERT_TAIL(&alldevi, i, i_next);
	ndevi++;
	return (i);
}

static const char *
concat(const char *name, int c)
{
	int len;
	char buf[NAMESIZE];

	len = strlen(name);
	if (len + 2 > sizeof(buf)) {
		error("device name `%s%c' too long", name, c);
		len = sizeof(buf) - 2;
	}
	memmove(buf, name, len);
	buf[len] = c;
	buf[len + 1] = 0;
	return (intern(buf));
}

const char *
starref(const char *name)
{

	return (concat(name, '*'));
}

const char *
wildref(const char *name)
{

	return (concat(name, '?'));
}

/*
 * Split a name like "foo0" into base name (foo) and unit number (0).
 * Return 0 on success.  To make this useful for names like "foo0a",
 * the length of the "foo0" part is one of the arguments.
 */
static int
split(const char *name, size_t nlen, char *base, size_t bsize, int *aunit)
{
	const char *cp;
	int c, l;

	l = nlen;
	if (l < 2 || l >= bsize || isdigit((unsigned char)*name))
		return (1);
	c = (u_char)name[--l];
	if (!isdigit(c)) {
		if (c == '*')
			*aunit = STAR;
		else if (c == '?')
			*aunit = WILD;
		else
			return (1);
	} else {
		cp = &name[l];
		while (isdigit((unsigned char)cp[-1]))
			l--, cp--;
		*aunit = atoi(cp);
	}
	memmove(base, name, l);
	base[l] = 0;
	return (0);
}

void
selectattr(struct attr *a)
{

	(void)ht_insert(selecttab, a->a_name, (char *)a->a_name);
}

/*
 * We have an instance of the base foo, so select it and all its
 * attributes for "optional foo".
 */
static void
selectbase(struct devbase *d, struct deva *da)
{
	struct attr *a;
	struct nvlist *nv;

	(void)ht_insert(selecttab, d->d_name, (char *)d->d_name);
	for (nv = d->d_attrs; nv != NULL; nv = nv->nv_next) {
		a = nv->nv_ptr;
		expandattr(a, selectattr);
	}
	if (da != NULL) {
		(void)ht_insert(selecttab, da->d_name, (char *)da->d_name);
		for (nv = da->d_attrs; nv != NULL; nv = nv->nv_next) {
			a = nv->nv_ptr;
			expandattr(a, selectattr);
		}
	}
}

/*
 * Is the given pointer on the given list of pointers?
 */
static int
onlist(struct nvlist *nv, void *ptr)
{
	for (; nv != NULL; nv = nv->nv_next)
		if (nv->nv_ptr == ptr)
			return (1);
	return (0);
}

static char *
extend(char *p, const char *name)
{
	int l;

	l = strlen(name);
	memmove(p, name, l);
	p += l;
	*p++ = ',';
	*p++ = ' ';
	return (p);
}

/*
 * Check that we got all required locators, and default any that are
 * given as "?" and have defaults.  Return 0 on success.
 */
static const char **
fixloc(const char *name, struct attr *attr, struct nvlist *got)
{
	struct nvlist *m, *n;
	int ord;
	const char **lp;
	int nmissing, nextra, nnodefault;
	char *mp, *ep, *ndp;
	char missing[1000], extra[1000], nodefault[1000];
	static const char *nullvec[1];

	/*
	 * Look for all required locators, and number the given ones
	 * according to the required order.  While we are numbering,
	 * set default values for defaulted locators.
	 */
	if (attr->a_loclen == 0)	/* e.g., "at root" */
		lp = nullvec;
	else
		lp = emalloc((attr->a_loclen + 1) * sizeof(const char *));
	for (n = got; n != NULL; n = n->nv_next)
		n->nv_int = -1;
	nmissing = 0;
	mp = missing;
	/* yes, this is O(mn), but m and n should be small */
	for (ord = 0, m = attr->a_locs; m != NULL; m = m->nv_next, ord++) {
		for (n = got; n != NULL; n = n->nv_next) {
			if (n->nv_name == m->nv_name) {
				n->nv_int = ord;
				break;
			}
		}
		if (n == NULL && m->nv_int == 0) {
			nmissing++;
			mp = extend(mp, m->nv_name);
		}
		lp[ord] = m->nv_str;
	}
	if (ord != attr->a_loclen)
		panic("fixloc");
	lp[ord] = NULL;
	nextra = 0;
	ep = extra;
	nnodefault = 0;
	ndp = nodefault;
	for (n = got; n != NULL; n = n->nv_next) {
		if (n->nv_int >= 0) {
			if (n->nv_str != NULL)
				lp[n->nv_int] = n->nv_str;
			else if (lp[n->nv_int] == NULL) {
				nnodefault++;
				ndp = extend(ndp, n->nv_name);
			}
		} else {
			nextra++;
			ep = extend(ep, n->nv_name);
		}
	}
	if (nextra) {
		ep[-2] = 0;	/* kill ", " */
		error("%s: extraneous locator%s: %s",
		    name, nextra > 1 ? "s" : "", extra);
	}
	if (nmissing) {
		mp[-2] = 0;
		error("%s: must specify %s", name, missing);
	}
	if (nnodefault) {
		ndp[-2] = 0;
		error("%s: cannot wildcard %s", name, nodefault);
	}
	if (nmissing || nnodefault) {
		free(lp);
		lp = NULL;
	}
	return (lp);
}
