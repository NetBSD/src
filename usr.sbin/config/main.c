/*	$NetBSD: main.c,v 1.52 2000/12/12 17:49:20 lukem Exp $	*/

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
 *	from: @(#)main.c	8.1 (Berkeley) 6/6/93
 */

#ifndef MAKE_BOOTSTRAP
#include <sys/cdefs.h>
#define	COPYRIGHT(x)	__COPYRIGHT(x)
#else
#define	COPYRIGHT(x)	static const char copyright[] = x
#endif

#ifndef lint
COPYRIGHT("@(#) Copyright (c) 1992, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "sem.h"

int	vflag;				/* verbose output */

int	yyparse(void);

#ifndef MAKE_BOOTSTRAP
extern int yydebug;
#endif

static struct hashtab *mkopttab;
static struct nvlist **nextopt;
static struct nvlist **nextmkopt;
static struct nvlist **nextfsopt;

static	void	dependopts(void);
static	void	do_depend(struct nvlist *);
static	void	stop(void);
static	int	do_option(struct hashtab *, struct nvlist ***,
		    const char *, const char *, const char *);
static	int	crosscheck(void);
static	int	badstar(void);
	int	main(int, char **);
static	int	mksymlinks(void);
static	int	mkident(void);
static	int	hasparent(struct devi *);
static	int	cfcrosscheck(struct config *, const char *, struct nvlist *);
void	defopt(struct hashtab *ht, const char *fname,
	     struct nvlist *opts, struct nvlist *deps);

int badfilename(const char *fname);

#ifdef	MAKE_BOOTSTRAP
char *__progname;
#endif

int
main(int argc, char **argv)
{
	char *p;
	const char *last_component;
	int pflag, ch;

#ifdef	MAKE_BOOTSTRAP
	__progname = argv[0];
#endif

	pflag = 0;
	while ((ch = getopt(argc, argv, "Dgpvb:s:")) != -1) {
		switch (ch) {

#ifndef MAKE_BOOTSTRAP
		case 'D':
			yydebug = 1;
			break;
#endif

		case 'g':
			/*
			 * In addition to DEBUG, you probably wanted to
			 * set "options KGDB" and maybe others.  We could
			 * do that for you, but you really should just
			 * put them in the config file.
			 */
			(void)fputs(
			    "-g is obsolete (use makeoptions DEBUG=\"-g\")\n",
			    stderr);
			goto usage;

		case 'p':
			/*
			 * Essentially the same as makeoptions PROF="-pg",
			 * but also changes the path from ../../compile/FOO
			 * to ../../compile/FOO.PROF; i.e., compile a
			 * profiling kernel based on a typical "regular"
			 * kernel.
			 *
			 * Note that if you always want profiling, you
			 * can (and should) use a "makeoptions" line.
			 */
			pflag = 1;
			break;

		case 'v':
			vflag = 1;
			break;

		case 'b':
			builddir = optarg;
			break;

		case 's':
			srcdir = optarg;
			break;

		case '?':
		default:
			goto usage;
		}
	}

	argc -= optind;
	argv += optind;
	if (argc > 1) {
usage:
		(void)fputs("usage: config [-pv] [-s srcdir] [-b builddir] sysname\n", stderr);
		exit(1);
	}
	conffile = (argc == 1) ? argv[0] : "CONFIG";
	if (firstfile(conffile)) {
		(void)fprintf(stderr, "config: cannot read %s: %s\n",
		    conffile, strerror(errno));
		exit(2);
	}

	/*
	 * Init variables.
	 */
	minmaxusers = 1;
	maxmaxusers = 10000;
	initintern();
	initfiles();
	initsem();
	ident = NULL;
	devbasetab = ht_new();
	devatab = ht_new();
	selecttab = ht_new();
	needcnttab = ht_new();
	opttab = ht_new();
	mkopttab = ht_new();
	fsopttab = ht_new();
	deffstab = ht_new();
	defopttab = ht_new();
	defparamtab = ht_new();
	defflagtab = ht_new();
	optfiletab = ht_new();
	nextopt = &options;
	nextmkopt = &mkoptions;
	nextfsopt = &fsoptions;

	/*
	 * Handle profiling (must do this before we try to create any
	 * files).
	 */
	last_component = strrchr(conffile, '/');
	last_component = (last_component) ? last_component + 1 : conffile;
	if (pflag) {
		p  = emalloc(strlen(last_component) + 17);
		(void)sprintf(p, "../compile/%s.PROF", last_component);
		(void)addmkoption(intern("PROF"), "-pg");
		(void)addoption(intern("GPROF"), NULL);
	} else {
		p = emalloc(strlen(last_component) + 13);
		(void)sprintf(p, "../compile/%s", last_component);
	}
	defbuilddir = (argc == 0) ? "." : p;

	/*
	 * Parse config file (including machine definitions).
	 */
	if (yyparse())
		stop();

	/*
	 * Deal with option dependencies.
	 */
	dependopts();

	/*
	 * Fix (as in `set firmly in place') files.
	 */
	if (fixfiles())
		stop();

	/*
	 * Fix objects and libraries.
	 */
	if (fixobjects())
		stop();

	/*
	 * Perform cross-checking.
	 */
	if (maxusers == 0) {
		if (defmaxusers) {
			(void)printf("maxusers not specified; %d assumed\n",
			    defmaxusers);
			maxusers = defmaxusers;
		} else {
			(void)fprintf(stderr,
			    "config: need \"maxusers\" line\n");
			errors++;
		}
	}
	if (fsoptions == NULL) {
		(void)fprintf(stderr,
		    "config: need at least one \"file-system\" line\n");
		errors++;
	}
	if (crosscheck() || errors)
		stop();

	/*
	 * Squeeze things down and finish cross-checks (STAR checks must
	 * run after packing).
	 */
	pack();
	if (badstar())
		stop();

	/*
	 * Ready to go.  Build all the various files.
	 */
	if (mksymlinks() || mkmakefile() || mkheaders() || mkswap() ||
	    mkioconf() || mkident())
		stop();
	(void)printf("Don't forget to run \"make depend\"\n");
	exit(0);
}

/*
 * Set any options that are implied by other options.
 */
static void
dependopts(void)
{
	struct nvlist *nv, *opt;

	for (nv = options; nv != NULL; nv = nv->nv_next) {
		if ((opt = find_declared_option(nv->nv_name)) != NULL) {
			for (opt = opt->nv_ptr; opt != NULL;
			    opt = opt->nv_next) {
				do_depend(opt);
			}
		}
	}
}

static void
do_depend(struct nvlist *nv)
{
	struct nvlist *nextnv;

	if (nv != NULL && !(nv->nv_flags & NV_DEPENDED)) {
		nv->nv_flags |= NV_DEPENDED;
		if (ht_lookup(opttab, nv->nv_name) == NULL)
			addoption(nv->nv_name, NULL);
		if ((nextnv = find_declared_option(nv->nv_name)) != NULL)
			do_depend(nextnv->nv_ptr);
	}
}

/*
 * Make a symlink for "machine" so that "#include <machine/foo.h>" works,
 * and for the machine's CPU architecture, so that works as well.
 */
static int
mksymlinks(void)
{
	int ret;
	char *p, buf[MAXPATHLEN];
	const char *q;

	sprintf(buf, "arch/%s/include", machine);
	p = sourcepath(buf);
	ret = unlink("machine");
	if (ret && errno != ENOENT)
		(void)fprintf(stderr, "config: unlink(machine): %s\n",
		    strerror(errno));
	ret = symlink(p, "machine");
	if (ret)
		(void)fprintf(stderr, "config: symlink(machine -> %s): %s\n",
		    p, strerror(errno));
	free(p);

	if (machinearch != NULL) {
		sprintf(buf, "arch/%s/include", machinearch);
		p = sourcepath(buf);
		q = machinearch;
	} else {
		p = estrdup("machine");
		q = machine;
	}
	ret = unlink(q);
	if (ret && errno != ENOENT)
		(void)fprintf(stderr, "config: unlink(%s): %s\n",
		    q, strerror(errno));
	ret = symlink(p, q);
	if (ret)
		(void)fprintf(stderr, "config: symlink(%s -> %s): %s\n",
		    q, p, strerror(errno));
	free(p);

	return (ret);
}

static __dead void
stop(void)
{
	(void)fprintf(stderr, "*** Stop.\n");
	exit(1);
}

/*
 * Define one or more file systems.  If file system options file name is
 * specified, a preprocessor #define for that file system will be placed
 * in that file.  In this case, only one file system may be specified.
 * Otherwise, no preprocessor #defines will be generated.
 */
void
deffilesystem(const char *fname, struct nvlist *fses)
{
	struct nvlist *nv;

	/*
	 * Mark these options as ones to skip when creating the Makefile.
	 */
	for (nv = fses; nv != NULL; nv = nv->nv_next) {
		if (ht_insert(defopttab, nv->nv_name, nv)) {
			error("file system or option `%s' already defined",
			    nv->nv_name);
			return;
		}

		/*
		 * Also mark it as a valid file system, which may be
		 * used in "file-system" directives in the config
		 * file.
		 */
		if (ht_insert(deffstab, nv->nv_name, nv))
			panic("file system `%s' already in table?!",
			    nv->nv_name);

		if (fname != NULL) {
			/*
			 * Only one file system allowed in this case.
			 */
			if (nv->nv_next != NULL) {
				error("only one file system per option "
				    "file may be specified");
				return;
			}

			if (ht_insert(optfiletab, fname, nv)) {
				error("option file `%s' already exists",
				    fname);
				return;
			}
		}
	}
}

/*
 * Sanity check a file name.
 */
int
badfilename(const char *fname)
{
	const char *n;

	/*
	 * We're putting multiple options into one file.  Sanity
	 * check the file name.
	 */
	if (strchr(fname, '/') != NULL) {
		error("option file name contains a `/'");
		return 1;
	}
	if ((n = strrchr(fname, '.')) == NULL || strcmp(n, ".h") != 0) {
		error("option file name does not end in `.h'");
		return 1;
	}
	return 0;
}


/*
 * Search for a defined option (defopt, filesystem, etc), and if found,
 * return the  option's struct nvlist.
 */
struct nvlist *
find_declared_option(const char *name)
{
	struct nvlist *option = NULL;

	if ((option = ht_lookup(defopttab, name)) != NULL ||
	    (option = ht_lookup(defparamtab, name)) != NULL ||
	    (option = ht_lookup(defflagtab, name)) != NULL ||
	    (option = ht_lookup(fsopttab, name)) != NULL) {
		return (option);
	}

	return (NULL);
}


/*
 * Define one or more standard options.  If an option file name is specified,
 * place all options in one file with the specified name.  Otherwise, create
 * an option file for each option.
 * record the option information in the specified table.
 */
void
defopt(struct hashtab *ht, const char *fname, struct nvlist *opts,
       struct nvlist *deps)
{
	struct nvlist *nv, *nextnv, *oldnv;
	const char *name, *n;
	char *p, c;
	char low[500];

	if (fname != NULL && badfilename(fname)) {
		return;
	}

	/*
	 * Mark these options as ones to skip when creating the Makefile.
	 */
	for (nv = opts; nv != NULL; nv = nextnv) {
		nextnv = nv->nv_next;

		/* An option name can be declared at most once. */
		if (DEFINED_OPTION(nv->nv_name)) {
			error("file system or option `%s' already defined",
			    nv->nv_name);
			return;
		}

		if (ht_insert(ht, nv->nv_name, nv)) {
			error("file system or option `%s' already defined",
			    nv->nv_name);
			return;
		}

		if (fname == NULL) {
			/*
			 * Each option will be going into its own file.
			 * Convert the option name to lower case.  This
			 * lower case name will be used as the option
			 * file name.
			 */
			(void) strcpy(low, "opt_");
			p = low + strlen(low);
			for (n = nv->nv_name; (c = *n) != '\0'; n++)
				*p++ = isupper(c) ? tolower(c) : c;
			*p = '\0';
			strcat(low, ".h");

			name = intern(low);
		} else {
			name = fname;
		}

		/* Use nv_ptr to link any other options that are implied. */
		nv->nv_ptr = deps;

		/*
		 * Remove this option from the parameter list before adding
		 * it to the list associated with this option file.
		 */
		nv->nv_next = NULL;

		/*
		 * Add this option file if we haven't seen it yet.
		 * Otherwise, append to the list of options already
		 * associated with this file.
		 */
		if ((oldnv = ht_lookup(optfiletab, name)) == NULL) {
			(void)ht_insert(optfiletab, name, nv);
		} else {
			while (oldnv->nv_next != NULL)
				oldnv = oldnv->nv_next;
			oldnv->nv_next = nv;
		}
	}
}

/*
 * Define one or more standard options.  If an option file name is specified,
 * place all options in one file with the specified name.  Otherwise, create
 * an option file for each option.
 */
void
defoption(const char *fname, struct nvlist *opts, struct nvlist *deps)
{

	defopt(defopttab, fname, opts, deps);
}


/*
 * Define an option for which a value is required. 
 */
void
defparam(const char *fname, struct nvlist *opts, struct nvlist *deps)
{

	defopt(defparamtab, fname, opts, deps);
}

/*
 * Define an option which must have a value, and which
 * emits  a "needs-flag" style output.
 */
void
defflag(const char *fname, struct nvlist *opts, struct nvlist *deps)
{

	defopt(defflagtab, fname, opts, deps);
}


/*
 * Add an option from "options FOO".  Note that this selects things that
 * are "optional foo".
 */
void
addoption(const char *name, const char *value)
{
	const char *n;
	char *p, c;
	char low[500];
	int is_fs, is_param, is_flag, is_opt, is_undecl;

	/* 
	 * Figure out how this option was declared (if at all.)
	 * XXX should use "params" and "flags" in config.
	 * XXX crying out for a type field in a unified hashtab.
	 */
	is_fs = OPT_FSOPT(name);
	is_param = OPT_DEFPARAM(name);
	is_opt = OPT_DEFOPT(name);
	is_flag =  OPT_DEFFLAG(name);
	is_undecl = !DEFINED_OPTION(name);

	/* Make sure this is not a defined file system. */
	if (is_fs) {
		error("`%s' is a defined file system", name);
		return;
	}
	/* A defparam must have a value */
	if (is_param && value == NULL) {
		error("option `%s' must have a value", name);
		return;
	}
	/* A defflag must not have a value */
	if (is_flag && value != NULL) {
		error("option `%s' must not have a value", name);
		return;
	}

	if (is_undecl && vflag) {
		warn("undeclared option `%s' added to IDENT", name);
	}

	if (do_option(opttab, &nextopt, name, value, "options"))
		return;

	/* make lowercase, then add to select table */
	for (n = name, p = low; (c = *n) != '\0'; n++)
		*p++ = isupper(c) ? tolower(c) : c;
	*p = 0;
	n = intern(low);
	(void)ht_insert(selecttab, n, (void *)n);
}

/*
 * Add a file system option.  This routine simply inserts the name into
 * a list of valid file systems, which is used to validate the root
 * file system type.  The name is then treated like a standard option.
 */
void
addfsoption(const char *name)
{
	const char *n; 
	char *p, c;
	char low[500];

	/* Make sure this is a defined file system. */
	if (!OPT_FSOPT(name)) {
		error("`%s' is not a defined file system", name);
		return;
	}

	/*
	 * Convert to lower case.  This will be used in the select
	 * table, to verify root file systems, and when the initial
	 * VFS list is created.
	 */
	for (n = name, p = low; (c = *n) != '\0'; n++)
		*p++ = isupper(c) ? tolower(c) : c;
	*p = 0;
	n = intern(low);

	if (do_option(fsopttab, &nextfsopt, name, n, "file-system"))
		return;

	/*
	 * Add a lower-case version to the table for root file system
	 * verification.
	 */
	if (ht_insert(fsopttab, n, (void *)n))
		panic("addfsoption: already in table");

	/* Add to select table. */
	(void)ht_insert(selecttab, n, (void *)n);
}

/*
 * Add a "make" option.
 */
void
addmkoption(const char *name, const char *value)
{

	(void)do_option(mkopttab, &nextmkopt, name, value, "mkoptions");
}

/*
 * Add a name=value pair to an option list.  The value may be NULL.
 */
static int
do_option(struct hashtab *ht, struct nvlist ***nppp, const char *name,
	  const char *value, const char *type)
{
	struct nvlist *nv;

	/*
	 * If a defopt'ed option was enabled but without an explicit
	 * value, supply a default value of 1, as for non-defopt
	 * options (where cc treats -DBAR as -DBAR=1.) 
	 */
	if (OPT_DEFOPT(name) && value == NULL)
		value = "1";

	/* assume it will work */
	nv = newnv(name, value, NULL, 0, NULL);
	if (ht_insert(ht, name, nv) == 0) {
		**nppp = nv;
		*nppp = &nv->nv_next;
		return (0);
	}

	/* oops, already got that option */
	nvfree(nv);
	if ((nv = ht_lookup(ht, name)) == NULL)
		panic("do_option");
	if (nv->nv_str != NULL && !OPT_FSOPT(name))
		error("already have %s `%s=%s'", type, name, nv->nv_str);
	else
		error("already have %s `%s'", type, name);
	return (1);
}

/*
 * Return true if there is at least one instance of the given unit
 * on the given device attachment (or any units, if unit == WILD).
 */
int
deva_has_instances(struct deva *deva, int unit)
{
	struct devi *i;

	if (unit == WILD)
		return (deva->d_ihead != NULL);
	for (i = deva->d_ihead; i != NULL; i = i->i_asame)
		if (unit == i->i_unit)
			return (1);
	return (0);
}

/*
 * Return true if there is at least one instance of the given unit
 * on the given base (or any units, if unit == WILD).
 */
int
devbase_has_instances(struct devbase *dev, int unit)
{
	struct deva *da;

	for (da = dev->d_ahead; da != NULL; da = da->d_bsame)
		if (deva_has_instances(da, unit))
			return (1);
	return (0);
}

static int
hasparent(struct devi *i)
{
	struct nvlist *nv;
	int atunit = i->i_atunit;

	/*
	 * We determine whether or not a device has a parent in in one
	 * of two ways:
	 *	(1) If a parent device was named in the config file,
	 *	    i.e. cases (2) and (3) in sem.c:adddev(), then
	 *	    we search its devbase for a matching unit number.
	 *	(2) If the device was attach to an attribute, then we
	 *	    search all attributes the device can be attached to
	 *	    for parents (with appropriate unit numebrs) that
	 *	    may be able to attach the device.
	 */

	/*
	 * Case (1): A parent was named.  Either it's configured, or not.
	 */
	if (i->i_atdev != NULL)
		return (devbase_has_instances(i->i_atdev, atunit));

	/*
	 * Case (2): No parent was named.  Look for devs that provide the attr.
	 */
	if (i->i_atattr != NULL)
		for (nv = i->i_atattr->a_refs; nv != NULL; nv = nv->nv_next)
			if (devbase_has_instances(nv->nv_ptr, atunit))
				return (1);
	return (0);
}

static int
cfcrosscheck(struct config *cf, const char *what, struct nvlist *nv)
{
	struct devbase *dev;
	struct devi *pd;
	int errs, devunit;

	if (maxpartitions <= 0)
		panic("cfcrosscheck");

	for (errs = 0; nv != NULL; nv = nv->nv_next) {
		if (nv->nv_name == NULL)
			continue;
		dev = ht_lookup(devbasetab, nv->nv_name);
		if (dev == NULL)
			panic("cfcrosscheck(%s)", nv->nv_name);
		if (has_attr(dev->d_attrs, s_ifnet))
			devunit = nv->nv_ifunit;	/* XXX XXX XXX */
		else
			devunit = minor(nv->nv_int) / maxpartitions;
		if (devbase_has_instances(dev, devunit))
			continue;
		if (devbase_has_instances(dev, STAR) &&
		    devunit >= dev->d_umax)
			continue;
		for (pd = allpseudo; pd != NULL; pd = pd->i_next)
			if (pd->i_base == dev && devunit < dev->d_umax &&
			    devunit >= 0)
				goto loop;
		(void)fprintf(stderr,
		    "%s:%d: %s says %s on %s, but there's no %s\n",
		    conffile, cf->cf_lineno,
		    cf->cf_name, what, nv->nv_str, nv->nv_str);
		errs++;
loop:
	}
	return (errs);
}

/*
 * Cross-check the configuration: make sure that each target device
 * or attribute (`at foo[0*?]') names at least one real device.  Also
 * see that the root and dump devices for all configurations are there.
 */
int
crosscheck(void)
{
	struct devi *i;
	struct config *cf;
	int errs;

	errs = 0;
	for (i = alldevi; i != NULL; i = i->i_next) {
		if (i->i_at == NULL || hasparent(i))
			continue;
		xerror(conffile, i->i_lineno,
		    "%s at %s is orphaned", i->i_name, i->i_at);
		(void)fprintf(stderr, " (%s %s declared)\n",
		    i->i_atunit == WILD ? "nothing matching" : "no",
		    i->i_at);
		errs++;
	}
	if (allcf == NULL) {
		(void)fprintf(stderr, "%s has no configurations!\n",
		    conffile);
		errs++;
	}
	for (cf = allcf; cf != NULL; cf = cf->cf_next) {
		if (cf->cf_root != NULL) {	/* i.e., not root on ? */
			errs += cfcrosscheck(cf, "root", cf->cf_root);
			errs += cfcrosscheck(cf, "dumps", cf->cf_dump);
		}
	}
	return (errs);
}

/*
 * Check to see if there is a *'d unit with a needs-count file.
 */
int
badstar(void)
{
	struct devbase *d;
	struct deva *da;
	struct devi *i;
	int errs, n;

	errs = 0;
	for (d = allbases; d != NULL; d = d->d_next) {
		for (da = d->d_ahead; da != NULL; da = da->d_bsame)
			for (i = da->d_ihead; i != NULL; i = i->i_asame) {
				if (i->i_unit == STAR)
					goto foundstar;
			}
		continue;
	foundstar:
		if (ht_lookup(needcnttab, d->d_name)) {
			(void)fprintf(stderr,
		    "config: %s's cannot be *'d until its driver is fixed\n",
			    d->d_name);
			errs++;
			continue;
		}
		for (n = 0; i != NULL; i = i->i_alias)
			if (!i->i_collapsed)
				n++;
		if (n < 1)
			panic("badstar() n<1");
	}
	return (errs);
}

/*
 * Verify/create builddir if necessary, change to it, and verify srcdir.
 * This will be called when we see the first include.
 */
void
setupdirs(void)
{
#define KERNNAMEOPTPREFIX	"_KERNEL_"
	char		*p, buf[sizeof(KERNNAMEOPTPREFIX) + PATH_MAX + 1];
	const char	*name;
	struct stat	 st;

			/*
			 * srcdir must be specified if builddir is not
			 * specified or if no configuration filename was
			 * specified.
			 */
	if ((builddir || strcmp(defbuilddir, ".") == 0) && !srcdir) {
		error("source directory must be specified");
		exit(1);
	}

	if (srcdir == NULL)
		srcdir = "../../../..";
	if (builddir == NULL)
		builddir = defbuilddir;

	if (stat(builddir, &st) != 0) {
		if (mkdir(builddir, 0777)) {
			(void)fprintf(stderr, "config: cannot create %s: %s\n",
			    builddir, strerror(errno));
			exit(2);
		}
	} else if (!S_ISDIR(st.st_mode)) {
		(void)fprintf(stderr, "config: %s is not a directory\n",
			      builddir);
		exit(2);
	}
	if (chdir(builddir) != 0) {
		(void)fprintf(stderr, "config: cannot change to %s\n",
			      builddir);
		exit(2);
	}
	if (stat(srcdir, &st) != 0 || !S_ISDIR(st.st_mode)) {
		(void)fprintf(stderr, "config: %s is not a directory\n",
			      srcdir);
		exit(2);
	}

			/*
			 * add option `KERNEL_foo', where `foo' is the
			 * basename of either the build dir (if set)
			 * or the basename of the config file
			 */
	name = (builddir) ? builddir : conffile;
	if ((p = strrchr(name, '/')) != NULL)
		name = p + 1;
	snprintf(buf, sizeof(buf), "%s%s", KERNNAMEOPTPREFIX, name);
	for (p = buf; *p; p++) {		/* prevent pain... */
		if (! isalnum(*p))
			*p = '_';
	}
	(void)addoption(intern(buf), NULL);
}

/*
 * Write identifier from "ident" directive into file, for
 * newvers.sh to pick it up.
 */
int
mkident(void)
{
	FILE *fp;

	(void)unlink("ident");

	if (ident == NULL)
		return (0);
	
	if ((fp = fopen("ident", "w")) == NULL) {
		(void)fprintf(stderr, "config: cannot write ident: %s\n",
		    strerror(errno));
		return (1);
	}
	if (vflag)
		(void)printf("using ident '%s'\n", ident);
	if (fprintf(fp, "%s\n", ident) < 0)
		return (1);
	(void)fclose(fp);

	return (0);
}
