/*	$NetBSD: config.h,v 1.54 2001/06/08 12:47:06 fredette Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)config.h	8.1 (Berkeley) 6/6/93
 */

/*
 * config.h:  Global definitions for "config"
 */

#include <sys/types.h>
#include <sys/param.h>

#if !defined(MAKE_BOOTSTRAP) && defined(BSD)
#include <sys/cdefs.h>
#include <paths.h>
#endif

#include <stdlib.h>
#include <unistd.h>

/* These are really for MAKE_BOOTSTRAP but harmless. */
#ifndef __dead
#define __dead
#endif
#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

#ifdef	MAKE_BOOTSTRAP
#undef	dev_t
#undef	NODEV
#undef	major
#undef	minor
#undef	makedev
#define	dev_t		int		/* XXX: assumes int is 32 bits */
#define	NODEV		((dev_t)-1)
#define major(x)        ((int)((((x) & 0x000fff00) >>  8)))
#define minor(x)        ((int)((((x) & 0xfff00000) >> 12) | \
			       (((x) & 0x000000ff) >>  0)))
#define makedev(x,y)    ((dev_t)((((x) <<  8) & 0x000fff00) | \
                                 (((y) << 12) & 0xfff00000) | \
                                 (((y) <<  0) & 0x000000ff)))
#define __attribute__(x)

extern const char *progname;
#define	setprogname(s)	((void)(progname = (s)))
#define	getprogname()	(progname)
#endif	/* MAKE_BOOTSTRAP */

#define ARRCHR '#'

/*
 * Name/value lists.  Values can be strings or pointers and/or can carry
 * integers.  The names can be NULL, resulting in simple value lists.
 */
struct nvlist {
	struct	nvlist *nv_next;
	const char *nv_name;
	union {
		const char *un_str;
		void *un_ptr;
	} nv_un;
#define	nv_str	nv_un.un_str
#define	nv_ptr	nv_un.un_ptr
	int	nv_int;
	int	nv_ifunit;		/* XXX XXX XXX */
	int	nv_flags;
#define	NV_DEPENDED	1
};

/*
 * Kernel configurations.
 */
struct config {
	struct	config *cf_next;	/* linked list */
	const char *cf_name;		/* "vmunix" */
	int	cf_lineno;		/* source line */
	const char *cf_fstype;		/* file system type */
	struct	nvlist *cf_root;	/* "root on ra0a" */
	struct	nvlist *cf_swap;	/* "swap on ra0b and ra1b" */
	struct	nvlist *cf_dump;	/* "dumps on ra0b" */
};

/*
 * Attributes.  These come in three flavors: "plain", "device class,"
 * and "interface".  Plain attributes (e.g., "ether") simply serve
 * to pull in files.  Device class attributes are like plain
 * attributes, but additionally specify a device class (e.g., the
 * "disk" device class attribute specifies that devices with the
 * attribute belong to the "DV_DISK" class) and are mutually exclusive.
 * Interface attributes (e.g., "scsi") carry three lists: locators,
 * child devices, and references.  The locators are those things
 * that must be specified in order to configure a device instance
 * using this attribute (e.g., "tg0 at scsi0").  The a_devs field
 * lists child devices that can connect here (e.g., "tg"s), while
 * the a_refs are parents that carry the attribute (e.g., actual
 * SCSI host adapter drivers such as the SPARC "esp").
 */
struct attr {
	const char *a_name;		/* name of this attribute */
	int	a_iattr;		/* true => allows children */
	const char *a_devclass;		/* device class described */
	struct	nvlist *a_locs;		/* locators required */
	int	a_loclen;		/* length of above list */
	struct	nvlist *a_devs;		/* children */
	struct	nvlist *a_refs;		/* parents */
};

/*
 * The "base" part (struct devbase) of a device ("uba", "sd"; but not
 * "uba2" or "sd0").  It may be found "at" one or more attributes,
 * including "at root" (this is represented by a NULL attribute), as
 * specified by the device attachments (struct deva).
 *
 * Each device may also export attributes.  If any provide an output
 * interface (e.g., "esp" provides "scsi"), other devices (e.g.,
 * "tg"s) can be found at instances of this one (e.g., "esp"s).
 * Such a connection must provide locators as specified by that
 * interface attribute (e.g., "target").  The base device can
 * export both output (aka `interface') attributes, as well as
 * import input (`plain') attributes.  Device attachments may
 * only import input attributes; it makes no sense to have a
 * specific attachment export a new interface to other devices.
 *
 * Each base carries a list of instances (via d_ihead).  Note that this
 * list "skips over" aliases; those must be found through the instances
 * themselves.  Each base also carries a list of possible attachments,
 * each of which specify a set of devices that the device can attach
 * to, as well as the device instances that are actually using that
 * attachment.
 */
struct devbase {
	const char *d_name;		/* e.g., "sd" */
	struct	devbase *d_next;	/* linked list */
	int	d_isdef;		/* set once properly defined */
	int	d_ispseudo;		/* is a pseudo-device */
	int	d_major;		/* used for "root on sd0", e.g. */
	struct	nvlist *d_attrs;	/* attributes, if any */
	int	d_umax;			/* highest unit number + 1 */
	struct	devi *d_ihead;		/* first instance, if any */
	struct	devi **d_ipp;		/* used for tacking on more instances */
	struct	deva *d_ahead;		/* first attachment, if any */
	struct	deva **d_app;		/* used for tacking on attachments */
	struct	attr *d_classattr;	/* device class attribute (if any) */
};

struct deva {
	const char *d_name;		/* name of attachment, e.g. "com_isa" */
	struct	deva *d_next;		/* linked list */
	struct	deva *d_bsame;		/* list on same base */
	int	d_isdef;		/* set once properly defined */
	struct	devbase *d_devbase;	/* the base device */
	struct	nvlist *d_atlist;	/* e.g., "at tg" (attr list) */
	struct	nvlist *d_attrs;	/* attributes, if any */
	struct	devi *d_ihead;		/* first instance, if any */
	struct	devi **d_ipp;		/* used for tacking on more instances */
};

/*
 * An "instance" of a device.  The same instance may be listed more
 * than once, e.g., "xx0 at isa? port FOO" + "xx0 at isa? port BAR".
 *
 * After everything has been read in and verified, the devi's are
 * "packed" to collect all the information needed to generate ioconf.c.
 * In particular, we try to collapse multiple aliases into a single entry.
 * We then assign each "primary" (non-collapsed) instance a cfdata index.
 * Note that there may still be aliases among these.
 */
struct devi {
	/* created while parsing config file */
	const char *i_name;	/* e.g., "sd0" */
	int	i_unit;		/* unit from name, e.g., 0 */
	struct	devbase *i_base;/* e.g., pointer to "sd" base */
	struct	devi *i_next;	/* list of all instances */
	struct	devi *i_bsame;	/* list on same base */
	struct	devi *i_asame;	/* list on same base attachment */
	struct	devi *i_alias;	/* other aliases of this instance */
	const char *i_at;	/* where this is "at" (NULL if at root) */
	struct	attr *i_atattr;	/* attr that allowed attach */
	struct	devbase *i_atdev;/* if "at <devname><unit>", else NULL */
	struct	deva *i_atdeva;
	const char **i_locs;	/* locators (as given by i_atattr) */
	int	i_atunit;	/* unit from "at" */
	int	i_cfflags;	/* flags from config line */
	int	i_lineno;	/* line # in config, for later errors */

	/* created during packing or ioconf.c generation */
/* 		i_loclen	   via i_atattr->a_loclen */
	short	i_collapsed;	/* set => this alias no longer needed */
	short	i_cfindex;	/* our index in cfdata */
	short	i_pvlen;	/* number of parents */
	short	i_pvoff;	/* offset in parents.vec */
	short	i_locoff;	/* offset in locators.vec */
	struct	devi **i_parents;/* the parents themselves */

};
/* special units */
#define	STAR	(-1)		/* unit number for, e.g., "sd*" */
#define	WILD	(-2)		/* unit number for, e.g., "sd?" */

/*
 * Files.  Each file is either standard (always included) or optional,
 * depending on whether it has names on which to *be* optional.  The
 * options field (fi_optx) is actually an expression tree, with nodes
 * for OR, AND, and NOT, as well as atoms (words) representing some   
 * particular option.  The node type is stored in the nv_int field.
 * Subexpressions appear in the `next' field; for the binary operators
 * AND and OR, the left subexpression is first stored in the nv_ptr field.
 * 
 * For any file marked as needs-count or needs-flag, fixfiles() will
 * build fi_optf, a `flat list' of the options with nv_int fields that
 * contain counts or `need' flags; this is used in mkheaders().
 */
struct files {
	struct	files *fi_next;	/* linked list */
	const char *fi_srcfile;	/* the name of the "files" file that got us */
	u_short	fi_srcline;	/* and the line number */
	u_char	fi_flags;	/* as below */
	char	fi_lastc;	/* last char from path */
	const char *fi_path;	/* full file path */
	const char *fi_tail;	/* name, i.e., strrchr(fi_path, '/') + 1 */
	const char *fi_base;	/* tail minus ".c" (or whatever) */
	const char *fi_prefix;	/* any file prefix */
	struct  nvlist *fi_optx;/* options expression */
	struct  nvlist *fi_optf;/* flattened version of above, if needed */
	const char *fi_mkrule;	/* special make rule, if any */
};
/* flags */
#define	FI_SEL		0x01	/* selected */
#define	FI_NEEDSCOUNT	0x02	/* needs-count */
#define	FI_NEEDSFLAG	0x04	/* needs-flag */
#define	FI_HIDDEN	0x08	/* obscured by other(s), base names overlap */

/*
 * Objects and libraries.  This allows precompiled object and library
 * files (e.g. binary-only device drivers) to be linked in.
 */
struct objects {
	struct	objects *oi_next;	/* linked list */
	const char *oi_srcfile;	/* the name of the "objects" file that got us */
	u_short	oi_srcline;	/* and the line number */
	u_char	oi_flags;	/* as below */
	char	oi_lastc;	/* last char from path */
	const char *oi_path;	/* full object path */
	const char *oi_prefix;	/* any file prefix */
	struct  nvlist *oi_optx;/* options expression */
	struct  nvlist *oi_optf;/* flattened version of above, if needed */
};
/* flags */
#define	OI_SEL		0x01	/* selected */
#define	OI_NEEDSFLAG	0x02	/* needs-flag */

#define	FX_ATOM		0	/* atom (in nv_name) */
#define	FX_NOT		1	/* NOT expr (subexpression in nv_next) */
#define	FX_AND		2	/* AND expr (lhs in nv_ptr, rhs in nv_next) */
#define	FX_OR		3	/* OR expr (lhs in nv_ptr, rhs in nv_next) */

/*
 * File/object prefixes.  These are arranged in a stack, and affect
 * the behavior of the source path.
 */
struct prefix {
	struct prefix *pf_next;	/* next prefix in stack */
	const char *pf_prefix;	/* the actual prefix */
};

/*
 * Hash tables look up name=value pairs.  The pointer value of the name
 * is assumed to be constant forever; this can be arranged by interning
 * the name.  (This is fairly convenient since our lexer does this for
 * all identifier-like strings---it has to save them anyway, lest yacc's
 * look-ahead wipe out the current one.)
 */
struct hashtab;

const char *conffile;		/* source file, e.g., "GENERIC.sparc" */
const char *machine;		/* machine type, e.g., "sparc" or "sun3" */
const char *machinearch;	/* machine arch, e.g., "sparc" or "m68k" */
struct	nvlist *machinesubarches;
				/* machine subarches, e.g., "sun68k" or "hpc" */
const char *srcdir;		/* path to source directory (rel. to build) */
const char *builddir;		/* path to build directory */
const char *defbuilddir;	/* default build directory */
const char *ident;		/* kernel "ident"ification string */
int	errors;			/* counts calls to error() */
int	minmaxusers;		/* minimum "maxusers" parameter */
int	defmaxusers;		/* default "maxusers" parameter */
int	maxmaxusers;		/* default "maxusers" parameter */
int	maxusers;		/* configuration's "maxusers" parameter */
int	maxpartitions;		/* configuration's "maxpartitions" parameter */
struct	nvlist *options;	/* options */
struct	nvlist *fsoptions;	/* filesystems */
struct	nvlist *mkoptions;	/* makeoptions */
struct	hashtab *devbasetab;	/* devbase lookup */
struct	hashtab *devatab;	/* devbase attachment lookup */
struct	hashtab *selecttab;	/* selects things that are "optional foo" */
struct	hashtab *needcnttab;	/* retains names marked "needs-count" */
struct	hashtab *opttab;	/* table of configured options */
struct	hashtab *fsopttab;	/* table of configured file systems */
struct	hashtab *defopttab;	/* options that have been "defopt"'d */
struct	hashtab *defflagtab;	/* options that have been "defflag"'d */
struct	hashtab *defparamtab;	/* options that have been "defparam"'d */
struct	hashtab *deffstab;	/* defined file systems */
struct	hashtab *optfiletab;	/* "defopt"'d option .h files */
struct	hashtab *attrtab;	/* attributes (locators, etc.) */

struct	devbase *allbases;	/* list of all devbase structures */
struct	deva *alldevas;		/* list of all devbase attachment structures */
struct	config *allcf;		/* list of configured kernels */
struct	devi *alldevi;		/* list of all instances */
struct	devi *allpseudo;	/* list of all pseudo-devices */
int	ndevi;			/* number of devi's (before packing) */
int	npseudo;		/* number of pseudo's */

struct	files *allfiles;	/* list of all kernel source files */
struct	objects *allobjects;	/* list of all kernel object and library
				   files */
struct prefix *prefixes;	/* prefix stack */
struct prefix *allprefixes;	/* all prefixes used (after popped) */

struct	devi **packed;		/* arrayified table for packed devi's */
int	npacked;		/* size of packed table, <= ndevi */

struct {			/* pv[] table for config */
	short	*vec;
	int	used;
} parents;
struct {			/* loc[] table for config */
	const char **vec;
	int	used;
} locators;

/* files.c */
void	initfiles(void);
void	checkfiles(void);
int	fixfiles(void);		/* finalize */
int	fixobjects(void);
void	addfile(const char *, struct nvlist *, int, const char *);
void	addobject(const char *, struct nvlist *, int);

/* hash.c */
struct	hashtab *ht_new(void);
int	ht_insrep(struct hashtab *, const char *, void *, int);
#define	ht_insert(ht, nam, val) ht_insrep(ht, nam, val, 0)
#define	ht_replace(ht, nam, val) ht_insrep(ht, nam, val, 1)
void	*ht_lookup(struct hashtab *, const char *);
void	initintern(void);
const char *intern(const char *);
typedef int (*ht_callback)(const char *, void *, void *);
int	ht_enumerate(struct hashtab *, ht_callback, void *);

/* main.c */
void	addoption(const char *name, const char *value);
void	addfsoption(const char *name);
void	addmkoption(const char *name, const char *value);
void	deffilesystem(const char *fname, struct nvlist *fses);
void	defoption(const char *fname, struct nvlist *opts,
	    struct nvlist *deps);
void	defflag(const char *fname, struct nvlist *opts,
	    struct nvlist *deps);
void	defparam(const char *fname, struct nvlist *opts,
	    struct nvlist *deps);
int	devbase_has_instances(struct devbase *, int);
struct nvlist * find_declared_option(const char *name);
int	deva_has_instances(struct deva *, int);
void	setupdirs(void);

/* tests on option types */
#define OPT_FSOPT(n)	(ht_lookup(deffstab, (n)) != NULL)
#define OPT_DEFOPT(n)	(ht_lookup(defopttab, (n)) != NULL)
#define OPT_DEFFLAG(n)	(ht_lookup(defflagtab, (n)) != NULL)
#define OPT_DEFPARAM(n)	(ht_lookup(defparamtab, (n)) != NULL)
#define DEFINED_OPTION(n) (find_declared_option((n)) != NULL)


/* mkheaders.c */
int	mkheaders(void);

/* mkioconf.c */
int	mkioconf(void);

/* mkmakefile.c */
int	mkmakefile(void);

/* mkswap.c */
int	mkswap(void);

/* pack.c */
void	pack(void);

/* scan.l */
int	currentline(void);
int	firstfile(const char *);
int	include(const char *, int, int);

/* sem.c, other than for yacc actions */
void	initsem(void);

/* util.c */
void	*emalloc(size_t);
void	*erealloc(void *, size_t);
char	*estrdup(const char *);
void	prefix_push(const char *);
void	prefix_pop(void);
char	*sourcepath(const char *);
void	warn(const char *, ...)				/* immediate warns */
     __attribute__((__format__(__printf__, 1, 2)));	
void	error(const char *, ...)			/* immediate errs */
     __attribute__((__format__(__printf__, 1, 2)));
void	xerror(const char *, int, const char *, ...)	/* delayed errs */
     __attribute__((__format__(__printf__, 3, 4)));
__dead void panic(const char *, ...)
     __attribute__((__format__(__printf__, 1, 2)));
struct nvlist *newnv(const char *, const char *, void *, int, struct nvlist *);
void	nvfree(struct nvlist *);
void	nvfreel(struct nvlist *);

/* liby */
void	yyerror(const char *);
int	yylex(void);
