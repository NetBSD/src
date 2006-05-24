/*	$NetBSD: main.c,v 1.11 2006/05/24 23:16:50 christos Exp $	*/

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
 *	from: @(#)main.c	8.1 (Berkeley) 6/6/93
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

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
#include <sys/mman.h>
#include <paths.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "defs.h"
#include "sem.h"
#include <vis.h>

#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

int	vflag;				/* verbose output */
int	Pflag;				/* pack locators */

int	yyparse(void);

#ifndef MAKE_BOOTSTRAP
extern int yydebug;
#endif

static struct hashtab *mkopttab;
static struct nvlist **nextopt;
static struct nvlist **nextmkopt;
static struct nvlist **nextappmkopt;
static struct nvlist **nextfsopt;

static	void	usage(void);
static	void	dependopts(void);
static	void	do_depend(struct nvlist *);
static	void	stop(void);
static	int	do_option(struct hashtab *, struct nvlist ***,
		    const char *, const char *, const char *);
static	int	undo_option(struct hashtab *, struct nvlist **,
		    struct nvlist ***, const char *, const char *);
static	int	crosscheck(void);
static	int	badstar(void);
	int	main(int, char **);
static	int	mksymlinks(void);
static	int	mkident(void);
static	int	devbase_has_dead_instances(const char *, void *, void *);
static	int	devbase_has_any_instance(struct devbase *, int, int, int);
static	int	check_dead_devi(const char *, void *, void *);
static	void	kill_orphans(void);
static	void	do_kill_orphans(struct devbase *, struct attr *,
    struct devbase *, int);
static	int	kill_orphans_cb(const char *, void *, void *);
static	int	cfcrosscheck(struct config *, const char *, struct nvlist *);
static	const char *strtolower(const char *);
void	defopt(struct hashtab *ht, const char *fname,
	     struct nvlist *opts, struct nvlist *deps);

#define LOGCONFIG_LARGE "INCLUDE_CONFIG_FILE"
#define LOGCONFIG_SMALL "INCLUDE_JUST_CONFIG"

static	void	logconfig_start(void);
static	void	logconfig_end(void);
static	FILE	*cfg;
static	time_t	cfgtime;

static	int	is_elf(const char *);
static	int	extract_config(const char *, const char *, int);

int badfilename(const char *fname);

const char *progname;

int
main(int argc, char **argv)
{
	char *p, cname[20];
	const char *last_component;
	int pflag, xflag, ch, removeit;

	setprogname(argv[0]);

	pflag = 0;
	xflag = 0;
	while ((ch = getopt(argc, argv, "DPgpvb:s:x")) != -1) {
		switch (ch) {

#ifndef MAKE_BOOTSTRAP
		case 'D':
			yydebug = 1;
			break;
#endif

		case 'P':
			Pflag = 1;
			break;

		case 'g':
			/*
			 * In addition to DEBUG, you probably wanted to
			 * set "options KGDB" and maybe others.  We could
			 * do that for you, but you really should just
			 * put them in the config file.
			 */
			(void)fprintf(stderr,
		    "config: -g is obsolete (use makeoptions DEBUG=\"-g\")\n");
			usage();

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

		case 'x':
			xflag = 1;
			break;

		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;
	if (argc > 1) {
		usage();
	}

	if (xflag && (builddir != NULL || srcdir != NULL || Pflag || pflag ||
	    vflag)) {
		(void)fprintf(stderr, "config: -x must be used alone\n");
		exit(1);
	}

	if (xflag) {
#ifdef __NetBSD__
		conffile = (argc == 1) ? argv[0] : _PATH_UNIX;
#else
		if (argc == 0) {
			(void)fprintf(stderr, "config: no kernel supplied\n");
			exit(1);
		}
#endif
		if (!is_elf(conffile)) {
			(void)fprintf(stderr,
			    "config: %s: not a binary kernel\n",
			    conffile);
			exit(1);
		}
		if (!extract_config(conffile, "stdout", STDOUT_FILENO)) {
			(void)fprintf(stderr,
			    "config: %s does not contain embedded "
			    "configuration data\n", conffile);
			exit(2);
		}
		exit(0);
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
	devroottab = ht_new();
	devatab = ht_new();
	devitab = ht_new();
	deaddevitab = ht_new();
	selecttab = ht_new();
	needcnttab = ht_new();
	opttab = ht_new();
	mkopttab = ht_new();
	condmkopttab = ht_new();
	fsopttab = ht_new();
	deffstab = ht_new();
	defopttab = ht_new();
	defparamtab = ht_new();
	defflagtab = ht_new();
	optfiletab = ht_new();
	bdevmtab = ht_new();
	maxbdevm = 0;
	cdevmtab = ht_new();
	maxcdevm = 0;
	nextopt = &options;
	nextmkopt = &mkoptions;
	nextappmkopt = &appmkoptions;
	nextfsopt = &fsoptions;

	/*
	 * Handle profiling (must do this before we try to create any
	 * files).
	 */
	last_component = strrchr(conffile, '/');
	last_component = (last_component) ? last_component + 1 : conffile;
	if (pflag) {
		p = emalloc(strlen(last_component) + 17);
		(void)sprintf(p, "../compile/%s.PROF", last_component);
		(void)addmkoption(intern("PROF"), "-pg");
		(void)addoption(intern("GPROF"), NULL);
	} else {
		p = emalloc(strlen(last_component) + 13);
		(void)sprintf(p, "../compile/%s", last_component);
	}
	defbuilddir = (argc == 0) ? "." : p;

	removeit = 0;
	if (is_elf(conffile)) {
		const char *tmpdir;
		int cfd;

		if (builddir == NULL) {
			(void)fprintf(stderr,
			    "config: build directory must be specified with "
			    "binary kernels\n");
			exit(1);
		}

		/* Open temporary configuration file */
		tmpdir = getenv("TMPDIR");
		if (tmpdir == NULL)
			tmpdir = "/tmp";
		snprintf(cname, sizeof(cname), "%s/config.tmp.XXXXXX", tmpdir);
		cfd = mkstemp(cname);
		if (cfd == -1) {
			fprintf(stderr, "config: cannot create %s: %s", cname,
			    strerror(errno));
			exit(2);
		}

		printf("Using configuration data embedded in kernel...\n");
		if (!extract_config(conffile, cname, cfd)) {
			(void)fprintf(stderr,
			    "config: %s does not contain embedded "
			    "configuration data\n", conffile);
			exit(2);
		}

		removeit = 1;
		close(cfd);
		firstfile(cname);
	}

	/*
	 * Parse config file (including machine definitions).
	 */
	logconfig_start();
	if (yyparse())
		stop();
	logconfig_end();

	if (removeit)
		unlink(cname);

	/*
	 * Detect and properly ignore orphaned devices
	 */
	kill_orphans();

	/*
	 * Select devices and pseudo devices and their attributes
	 */
	if (fixdevis())
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
	 * Fix device-majors.
	 */
	if (fixdevsw())
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
	    mkioconf() || (do_devsw ? mkdevsw() : 0) || mkident())
		stop();
	(void)printf("Build directory is %s\n", builddir);
	(void)printf("Don't forget to run \"make depend\"\n");
	exit(0);
}

static void
usage(void)
{
	(void)fputs("usage: config [-Ppv] [-s srcdir] [-b builddir] "
		"[sysname]\n", stderr);
	(void)fputs("       config -x [kernel-file]\n", stderr);
	exit(1);
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
	struct attr *a;

	if (nv != NULL && (nv->nv_flags & NV_DEPENDED) == 0) {
		nv->nv_flags |= NV_DEPENDED;
		/*
		 * If the dependency is an attribute, then just add
		 * it to the selecttab.
		 */
		if ((a = ht_lookup(attrtab, nv->nv_name)) != NULL) {
			if (a->a_iattr)
				panic("do_depend(%s): dep `%s' is an iattr",
				    nv->nv_name, a->a_name);
			expandattr(a, selectattr);
		} else {
			if (ht_lookup(opttab, nv->nv_name) == NULL)
				addoption(nv->nv_name, NULL);
			if ((nextnv =
			     find_declared_option(nv->nv_name)) != NULL)
				do_depend(nextnv->nv_ptr);
		}
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
	struct nvlist *nv;

	snprintf(buf, sizeof(buf), "arch/%s/include", machine);
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
		snprintf(buf, sizeof(buf), "arch/%s/include", machinearch);
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

	for (nv = machinesubarches; nv != NULL; nv = nv->nv_next) {
		q = nv->nv_name;
		snprintf(buf, sizeof(buf), "arch/%s/include", q);
		p = sourcepath(buf);
		ret = unlink(q);
		if (ret && errno != ENOENT)
			(void)fprintf(stderr, "config: unlink(%s): %s\n",
			    q, strerror(errno));
		ret = symlink(p, q);
		if (ret)
			(void)fprintf(stderr, "config: symlink(%s -> %s): %s\n",
		    	q, p, strerror(errno));
		free(p);
	}

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
 * return the option's struct nvlist.
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
	struct nvlist *nv, *nextnv, *oldnv, *dep;
	struct attr *a;
	const char *name;
	char buf[500];

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
			(void) snprintf(buf, sizeof(buf), "opt_%s.h",
			    strtolower(nv->nv_name));
			name = intern(buf);
		} else {
			name = fname;
		}

		/* Use nv_ptr to link any other options that are implied. */
		nv->nv_ptr = deps;
		for (dep = deps; dep != NULL; dep = dep->nv_next) {
			/*
			 * If the dependency is an attribute, it must not
			 * be an interface attribute.  Otherwise, it must
			 * be a previously declared option.
			 */
			if ((a = ht_lookup(attrtab, dep->nv_name)) != NULL) {
				if (a->a_iattr)
					error("option `%s' dependency `%s' "
					    "is an interface attribute",
					    nv->nv_name, a->a_name);
			} else if (find_declared_option(dep->nv_name) == NULL) {
				error("option `%s' dependency `%s' "
				    "is an unknown option",
				    nv->nv_name, dep->nv_name);
			}
		}

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

	warn("The use of `defopt' is deprecated");
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
 * Define an option which must not have a value, and which
 * emits a "needs-flag" style output.
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
	n = strtolower(name);
	(void)ht_insert(selecttab, n, (void *)n);
}

void
deloption(const char *name)
{

	if (undo_option(opttab, &options, &nextopt, name, "options"))
		return;
	if (undo_option(selecttab, NULL, NULL, strtolower(name), "options"))
		return;
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
	n = strtolower(name);

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

void
delfsoption(const char *name)
{
	const char *n;

	n = strtolower(name);
	if (undo_option(fsopttab, &fsoptions, &nextfsopt, name, "file-system"))
		return;
	if (undo_option(fsopttab, NULL, NULL, n, "file-system"))
		return;
	if (undo_option(selecttab, NULL, NULL, n, "file-system"))
		return;
}

/*
 * Add a "make" option.
 */
void
addmkoption(const char *name, const char *value)
{

	(void)do_option(mkopttab, &nextmkopt, name, value, "makeoptions");
}

void
delmkoption(const char *name)
{

	(void)undo_option(mkopttab, &mkoptions, &nextmkopt, name,
	    "makeoptions");
}

/*
 * Add an appending "make" option.
 */
void
appendmkoption(const char *name, const char *value)
{
	struct nvlist *nv;

	nv = newnv(name, value, NULL, 0, NULL);
	*nextappmkopt = nv;
	nextappmkopt = &nv->nv_next;
}

/*
 * Add a conditional appending "make" option.
 */
void
appendcondmkoption(const char *selname, const char *name, const char *value)
{
	struct nvlist *nv, *lnv;
	const char *n;

	n = strtolower(selname);
	nv = newnv(name, value, NULL, 0, NULL);
	if (ht_insert(condmkopttab, n, nv) == 0)
		return;

	if ((lnv = ht_lookup(condmkopttab, n)) == NULL)
		panic("appendcondmkoption");
	for (; lnv->nv_next != NULL; lnv = lnv->nv_next)
		/* search for the last list element */;
	lnv->nv_next = nv;
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
	 * If a defopt'ed or defflag'ed option was enabled but without
	 * an explicit value (always the case for defflag), supply a
	 * default value of 1, as for non-defopt options (where cc
	 * treats -DBAR as -DBAR=1.) 
	 */
	if ((OPT_DEFOPT(name) || OPT_DEFFLAG(name)) && value == NULL)
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
 * Remove a name from a hash table,
 * and optionally, a name=value pair from an option list.
 */
static int
undo_option(struct hashtab *ht, struct nvlist **npp,
    struct nvlist ***next, const char *name, const char *type)
{
	struct nvlist *nv;
	
	if (ht_remove(ht, name)) {
		error("%s `%s' is not defined", type, name);
		return (1);
	}
	if (npp == NULL)
		return (0);

	for ( ; *npp != NULL; npp = &(*npp)->nv_next) {
		if ((*npp)->nv_name != name)
			continue;
		if (next != NULL && *next == &(*npp)->nv_next)
			*next = npp;
		nv = (*npp)->nv_next;
		nvfree(*npp);
		*npp = nv;
		return (0);
	}
	panic("%s `%s' is not defined in nvlist", type, name);
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

	for (i = deva->d_ihead; i != NULL; i = i->i_asame)
		if (i->i_active == DEVI_ACTIVE &&
		    (unit == WILD || unit == i->i_unit || i->i_unit == STAR))
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

	/*
	 * Pseudo-devices are a little special.  We consider them
	 * to have instances only if they are both:
	 *
	 *	1. Included in this kernel configuration.
	 *
	 *	2. Have one or more interface attributes.
	 */
	if (dev->d_ispseudo) {
		struct nvlist *nv;
		struct attr *a;

		if (ht_lookup(devitab, dev->d_name) == NULL)
			return (0);

		for (nv = dev->d_attrs; nv != NULL; nv = nv->nv_next) {
			a = nv->nv_ptr;
			if (a->a_iattr)
				return (1);
		}
		return (0);
	}

	for (da = dev->d_ahead; da != NULL; da = da->d_bsame)
		if (deva_has_instances(da, unit))
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
		TAILQ_FOREACH(pd, &allpseudo, i_next) {
			if (pd->i_base == dev && devunit < dev->d_umax &&
			    devunit >= 0)
				goto loop;
		}
		(void)fprintf(stderr,
		    "%s:%d: %s says %s on %s, but there's no %s\n",
		    conffile, cf->cf_lineno,
		    cf->cf_name, what, nv->nv_str, nv->nv_str);
		errs++;
 loop:
		;
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
	struct config *cf;
	int errs;

	errs = 0;
	if (TAILQ_EMPTY(&allcf)) {
		(void)fprintf(stderr, "%s has no configurations!\n",
		    conffile);
		errs++;
	}
	TAILQ_FOREACH(cf, &allcf, cf_next) {
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
	TAILQ_FOREACH(d, &allbases, d_next) {
		for (da = d->d_ahead; da != NULL; da = da->d_bsame)
			for (i = da->d_ihead; i != NULL; i = i->i_asame) {
				if (i->i_unit == STAR)
					goto aybabtu;
			}
		continue;
 aybabtu:
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
	struct stat st;

	/* srcdir must be specified if builddir is not specified or if
	 * no configuration filename was specified. */
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
}

/*
 * Write identifier from "ident" directive into file, for
 * newvers.sh to pick it up.
 */
int
mkident(void)
{
	FILE *fp;
	int error = 0;

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
		error = 1;
	(void)fclose(fp);

	return error;
}

void
logconfig_start(void)
{
	extern FILE *yyin;
	char line[1024];
	const char *tmpdir;
	struct stat st;
	int fd;

	if (yyin == NULL || fstat(fileno(yyin), &st) == -1)
		return;
	cfgtime = st.st_mtime;

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL)
		tmpdir = "/tmp";
	snprintf(line, sizeof(line), "%s/config.tmp.XXXXXX", tmpdir);
	if ((fd = mkstemp(line)) == -1 ||
	    (cfg = fdopen(fd, "r+")) == NULL) {
		if (fd != -1) {
			unlink(line);
			close(fd);
		}
		cfg = NULL;
		return;
	}
	unlink(line);

	(void)fprintf(cfg, "#include \"opt_config.h\"\n");
	(void)fprintf(cfg, "\n");
	(void)fprintf(cfg, "/*\n");
	(void)fprintf(cfg, " * Add either (or both) of\n");
	(void)fprintf(cfg, " *\n");
	(void)fprintf(cfg, " *\toptions %s\n", LOGCONFIG_LARGE);
	(void)fprintf(cfg, " *\toptions %s\n", LOGCONFIG_SMALL);
	(void)fprintf(cfg, " *\n");
	(void)fprintf(cfg,
	    " * to your kernel config file to embed it in the resulting\n");
	(void)fprintf(cfg,
	    " * kernel.  The latter option does not include files that are\n");
	(void)fprintf(cfg,
	    " * included (recursively) by your config file.  The embedded\n");
	(void)fprintf(cfg,
	    " * data be extracted by using the command:\n");
	(void)fprintf(cfg, " *\n");
	(void)fprintf(cfg,
	    " *\tstrings netbsd | sed -n 's/^_CFG_//p' | unvis\n");
	(void)fprintf(cfg, " */\n");
	(void)fprintf(cfg, "\n");
	(void)fprintf(cfg, "#ifdef CONFIG_FILE\n");
	(void)fprintf(cfg, "#if defined(%s) || defined(%s)\n\n",
	    LOGCONFIG_LARGE, LOGCONFIG_SMALL);
	(void)fprintf(cfg,
	    "static const char config[]\n#ifdef vax\n"
	    "\t__attribute__((__unused__))\n#else\n"
	    "\t__attribute__((__used__))\n#endif\n\t=\n\n");

	(void)fprintf(cfg, "#ifdef %s\n\n", LOGCONFIG_LARGE);
	(void)fprintf(cfg, "\"_CFG_### START CONFIG FILE \\\"%s\\\"\\n\"\n\n",
	    conffile);
	(void)fprintf(cfg, "#endif /* %s */\n\n", LOGCONFIG_LARGE);

	logconfig_include(yyin, NULL);

	(void)fprintf(cfg, "#ifdef %s\n\n", LOGCONFIG_LARGE);
	(void)fprintf(cfg, "\"_CFG_### END CONFIG FILE \\\"%s\\\"\\n\"\n",
	    conffile);

	rewind(yyin);
}

void
logconfig_include(FILE *cf, const char *filename)
{
	char line[1024], in[2048], *out;
	struct stat st;
	int missingeol;

	if (!cfg)
		return;

	missingeol = 0;
	if (fstat(fileno(cf), &st) == -1)
		return;
	if (cfgtime < st.st_mtime)
		cfgtime = st.st_mtime;

	if (filename)
		(void)fprintf(cfg,
		    "\"_CFG_### (included from \\\"%s\\\")\\n\"\n",
		    filename);
	while (fgets(line, sizeof(line), cf) != NULL) {
		missingeol = 1;
		(void)fprintf(cfg, "\"_CFG_");
		if (filename)
			(void)fprintf(cfg, "###> ");
		strvis(in, line, VIS_TAB);
		for (out = in; *out; out++)
			switch (*out) {
			case '\n':
				(void)fprintf(cfg, "\\n\"\n");
				missingeol = 0;
				break;
			case '"': case '\\':
				(void)fputc('\\', cfg);
				/* FALLTHROUGH */
			default:
				(void)fputc(*out, cfg);
				break;
			}
	}
	if (missingeol) {
		(void)fprintf(cfg, "\\n\"\n");
		(void)fprintf(stderr,
		    "config: %s: newline missing at EOF\n",
		    filename != NULL ? filename : conffile);
	}
	if (filename)
		(void)fprintf(cfg, "\"_CFG_### (end include \\\"%s\\\")\\n\"\n",
		    filename);

	rewind(cf);
}

void
logconfig_end(void)
{
	char line[1024];
	FILE *fp;
	struct stat st;

	if (!cfg)
		return;

	(void)fprintf(cfg, "#endif /* %s */\n", LOGCONFIG_LARGE);
	(void)fprintf(cfg, ";\n");
	(void)fprintf(cfg, "#endif /* %s || %s */\n",
	    LOGCONFIG_LARGE, LOGCONFIG_SMALL);
	(void)fprintf(cfg, "#endif /* CONFIG_FILE */\n");
	rewind(cfg);

	if (stat("config_file.h", &st) != -1) {
		if (cfgtime < st.st_mtime) {
			fclose(cfg);
			return;
		}
	}

	fp = fopen("config_file.h", "w");
	if(!fp) {
		(void)fprintf(stderr,
		    "config: cannot write to \"config_file.h\"\n");
		exit(1);
	}

	while (fgets(line, sizeof(line), cfg) != NULL)
		fputs(line, fp);
	fclose(fp);
	fclose(cfg);
}

static const char *
strtolower(const char *name)
{
	const char *n;
	char *p, low[500];
	unsigned char c;

	for (n = name, p = low; (c = *n) != '\0'; n++)
		*p++ = isupper(c) ? tolower(c) : c;
	*p = 0;
	return (intern(low));
}

static int
is_elf(const char *file)
{
	int kernel;
	char hdr[4];

	kernel = open(file, O_RDONLY);
	if (kernel == -1) {
		fprintf(stderr, "config: cannot open %s: %s\n", file,
		    strerror(errno));
		exit(2);
	}
	if (read(kernel, hdr, 4) == -1) {
		fprintf(stderr, "config: cannot read from %s: %s\n", file,
		    strerror(errno));
		exit(2);
	}
	close(kernel);

	return memcmp("\177ELF", hdr, 4) == 0 ? 1 : 0;
}

static int
extract_config(const char *kname, const char *cname, int cfd)
{
	char *ptr;
	int found, kfd, i;
	struct stat st;

	found = 0;

	/* mmap(2) binary kernel */
	kfd = open(conffile, O_RDONLY);
	if (kfd == -1) {
		fprintf(stderr, "config: cannot open %s: %s\n", kname,
		    strerror(errno));
		exit(2);
	}
	if ((fstat(kfd, &st) == -1)) {
		fprintf(stderr, "config: cannot stat %s: %s\n", kname,
		    strerror(errno));
		exit(2);
	}
	ptr = (char *)mmap(0, st.st_size, PROT_READ, MAP_FILE | MAP_SHARED,
	    kfd, 0);
	if (ptr == MAP_FAILED) {
		fprintf(stderr, "config: cannot mmap %s: %s\n", kname,
		    strerror(errno));
		exit(2);
	}

	/* Scan mmap(2)'ed region, extracting kernel configuration */
	for (i = 0; i < st.st_size; i++) {
		if ((*ptr == '_') && (st.st_size - i > 5) && memcmp(ptr,
		    "_CFG_", 5) == 0) {
			/* Line found */
			char *oldptr, line[LINE_MAX + 1], uline[LINE_MAX + 1];
			int j;

			found = 1;

			oldptr = (ptr += 5);
			while (*ptr != '\n' && *ptr != '\0') ptr++;
			if (ptr - oldptr > LINE_MAX) {
				fprintf(stderr, "config: line too long\n");
				exit(2);
			}
			i += ptr - oldptr + 5;
			memcpy(line, oldptr, (ptr - oldptr));
			line[ptr - oldptr] = '\0';
			j = strunvis(uline, line);
			if (j == -1) {
				fprintf(stderr, "config: unvis: invalid "
				    "encoded sequence\n");
				exit(2);
			}
			uline[j] = '\n';
			if (write(cfd, uline, j + 1) == -1) {
				fprintf(stderr, "config: cannot write to %s: "
				    "%s\n", cname, strerror(errno));
				exit(2);
			}
		} else ptr++;
	}

	close(kfd);

	return found;
}

struct dhdi_params {
	struct devbase *d;
	int unit;
	int level;
};

static int
devbase_has_dead_instances(const char *key, void *value, void *aux)
{
	struct devi *i;
	struct dhdi_params *dhdi = aux;

	for (i = value; i != NULL; i = i->i_alias)
		if (i->i_base == dhdi->d &&
		    (dhdi->unit == WILD || dhdi->unit == i->i_unit ||
		     i->i_unit == STAR) &&
		    i->i_level >= dhdi->level)
			return 1;
	return 0;
}

/*
 * This is almost the same as devbase_has_instances, except it
 * may have special considerations regarding ignored instances.
 */

static int
devbase_has_any_instance(struct devbase *dev, int unit, int state, int level)
{
	struct deva *da;
	struct devi *i;

	if (dev->d_ispseudo) {
		if (dev->d_ihead != NULL)
			return 1;
		else if (state != DEVI_IGNORED)
			return 0;
		if ((i = ht_lookup(deaddevitab, dev->d_name)) == NULL)
			return 0;
		return (i->i_level >= level);
	}

	for (da = dev->d_ahead; da != NULL; da = da->d_bsame)
		for (i = da->d_ihead; i != NULL; i = i->i_asame)
			if ((i->i_active == DEVI_ACTIVE ||
			     i->i_active == state) &&
			    (unit == WILD || unit == i->i_unit ||
			     i->i_unit == STAR))
				return 1;

	if (state == DEVI_IGNORED) {
		struct dhdi_params dhdi = { dev, unit, level };
		/* also check dead devices */
		return ht_enumerate(deaddevitab, devbase_has_dead_instances,
		    &dhdi);
	}
		
	return 0;
}

/*
 * check_dead_devi(), used with ht_enumerate, checks if any of the removed
 * device instances would have been a valid instance considering the devbase,
 * the parent device and the interface attribute.
 *
 * In other words, for a non-active device, it checks if children would be
 * actual orphans or the result of a negative statement in the config file.
 */

struct cdd_params {
	struct devbase *d;
	struct attr *at;
	struct devbase *parent;
};

static int
check_dead_devi(const char *key, void *value, void *aux)
{
	struct cdd_params *cdd = aux;
	struct devi *i = value;
	struct pspec *p;

	if (i->i_base != cdd->d)
		return 0;

	for (; i != NULL; i = i->i_alias) {
		p = i->i_pspec;
		if ((p == NULL && cdd->at == NULL) ||
		    (p != NULL && p->p_iattr == cdd->at &&
		     (p->p_atdev == NULL || p->p_atdev == cdd->parent))) {
			if (p != NULL &&
			    !devbase_has_any_instance(cdd->parent, p->p_atunit,
			    DEVI_IGNORED, i->i_level))
				return 0;
			else
				return 1;
		}
	}
	return 0;
}

static void
do_kill_orphans(struct devbase *d, struct attr *at, struct devbase *parent,
    int state)
{
	struct nvlist *nv, *nv1;
	struct attr *a;
	struct devi *i, *j = NULL;
	struct pspec *p;
	int active = 0;

	/*
	 * A pseudo-device will always attach at root, and if it has an
	 * instance (it cannot have more than one), it is enough to consider
	 * it active, as there is no real attachment.
	 *
	 * A pseudo device can never be marked DEVI_IGNORED.
	 */
	if (d->d_ispseudo) {
		if (d->d_ihead != NULL)
			d->d_ihead->i_active = active = DEVI_ACTIVE;
		else {
			if (ht_lookup(deaddevitab, d->d_name) != NULL)
				active = DEVI_IGNORED;
			else
				return;
		}
	} else {
		int seen = 0;

		for (i = d->d_ihead; i != NULL; i = i->i_bsame) {
			for (j = i; j != NULL; j = j->i_alias) {
				p = j->i_pspec;
				if ((p == NULL && at == NULL) ||
				    (p != NULL && p->p_iattr == at &&
				    (p->p_atdev == NULL ||
				    p->p_atdev == parent))) {
					if (p != NULL &&
					    !devbase_has_any_instance(parent,
					      p->p_atunit, state, j->i_level))
						continue;
					/*
					 * There are Fry-like devices which can
					 * be their own grand-parent (or even
					 * parent, like uhub).  We don't want
					 * to loop, so if we've already reached
					 * an instance for one reason or
					 * another, stop there.
					 */
					if (j->i_active == DEVI_ACTIVE ||
					    j->i_active == state) {
						/*
						 * Device has already been
						 * seen.  However it might
						 * have siblings who still
						 * have to be activated or
						 * orphaned.
						 */
						seen = 1;
						continue;
					}
					j->i_active = active = state;
					if (p != NULL)
						p->p_active = state;
				}
			}
		}
		/*
		 * If we've been there but have made no change, stop.
		 */
		if (seen && !active)
			return;
		if (!active) {
			struct cdd_params cdd = { d, at, parent };
			/* Look for a matching dead devi */
			if (ht_enumerate(deaddevitab, check_dead_devi, &cdd) &&
			    d != parent)
				/*
				 * That device had its instances removed.
				 * Continue the loop marking descendants
				 * with DEVI_IGNORED instead of DEVI_ACTIVE.
				 *
				 * There is one special case for devices that
				 * are their own parent:  if that instance is
				 * removed (e.g., no uhub* at uhub?), we don't
				 * have to continue looping.
				 */
				active = DEVI_IGNORED;
			else
				return;
		}
	}

	for (nv = d->d_attrs; nv != NULL; nv = nv->nv_next) {
		a = nv->nv_ptr;
		for (nv1 = a->a_devs; nv1 != NULL; nv1 = nv1->nv_next)
			do_kill_orphans(nv1->nv_ptr, a, d, active);
	}
}

static int
kill_orphans_cb(const char *key, void *value, void *aux)
{
	do_kill_orphans((struct devbase *)value, NULL, NULL, DEVI_ACTIVE);
	return 0;
}

static void
kill_orphans()
{
	ht_enumerate(devroottab, kill_orphans_cb, NULL);
}
