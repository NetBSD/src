/*	$NetBSD: perform.c,v 1.1.1.4.2.2 2008/01/09 01:26:17 matt Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.44 1997/10/13 15:03:46 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.1.1.4.2.2 2008/01/09 01:26:17 matt Exp $");
#endif
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * This is the main body of the add module.
 *
 */

#if HAVE_ASSERT_H
#include <assert.h>
#endif
#if HAVE_ERR_H
#include <err.h>
#endif
#if HAVE_ERRNO_H
#include <errno.h>
#endif
#include "defs.h"
#include "lib.h"
#include "add.h"
#include "verify.h"

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

static char LogDir[MaxPathSize];
static int zapLogDir;		/* Should we delete LogDir? */

static package_t Plist;
static char *Home;

static lfile_head_t files;

/* used in build information */
enum {
	Good,
	Missing,
	Warning,
	Fatal
};

static void
normalise_platform(struct utsname *host_name)
{
#ifdef NUMERIC_VERSION_ONLY
	size_t span;

	span = strspn(host_name->release, "0123456789.");
	host_name->release[span] = '\0';
#endif
}

/* Read package build information */
static int
read_buildinfo(char **buildinfo)
{
	char   *key;
	char   *line;
	size_t	len;
	FILE   *fp;

	if ((fp = fopen(BUILD_INFO_FNAME, "r")) == NULL) {
		warnx("unable to open %s file.", BUILD_INFO_FNAME);
		return 0;
	}

	while ((line = fgetln(fp, &len)) != NULL) {
		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		if ((key = strsep(&line, "=")) == NULL)
			continue;

		/*
		 * pkgsrc used to create the BUILDINFO file using
		 * "key= value", so skip the space if it's there.
		 */
		if (line == NULL)
			continue;
		if (line[0] == ' ')
			line += sizeof(char);

		/*
		 * we only care about opsys, arch, version, and
		 * dependency recommendations
		 */
		if (line[0] != '\0') {
			if (strcmp(key, "OPSYS") == 0)
			    buildinfo[BI_OPSYS] = strdup(line);
			else if (strcmp(key, "OS_VERSION") == 0)
			    buildinfo[BI_OS_VERSION] = strdup(line);
			else if (strcmp(key, "MACHINE_ARCH") == 0)
			    buildinfo[BI_MACHINE_ARCH] = strdup(line);
			else if (strcmp(key, "IGNORE_RECOMMENDED") == 0)
			    buildinfo[BI_IGNORE_RECOMMENDED] = strdup(line);
			else if (strcmp(key, "USE_ABI_DEPENDS") == 0)
			    buildinfo[BI_USE_ABI_DEPENDS] = strdup(line);
		}
	}
	(void) fclose(fp);
	if (buildinfo[BI_OPSYS] == NULL ||
	    buildinfo[BI_OS_VERSION] == NULL ||
	    buildinfo[BI_MACHINE_ARCH] == NULL) {
		warnx("couldn't extract build information from package.");
		return 0;
	}
	return 1;
}

static int
sanity_check(const char *pkg)
{
	int     errc = 0;

	if (!fexists(CONTENTS_FNAME)) {
		warnx("package %s has no CONTENTS file!", pkg);
		errc = 1;
	} else if (!fexists(COMMENT_FNAME)) {
		warnx("package %s has no COMMENT file!", pkg);
		errc = 1;
	} else if (!fexists(DESC_FNAME)) {
		warnx("package %s has no DESC file!", pkg);
		errc = 1;
	}
	return errc;
}

/* install a pre-requisite package. Returns 1 if it installed it */
static int
installprereq(const char *name, int *errc, int doupdate)
{
	int ret;
	ret = 0;

	if (Verbose)
		printf("Loading it from %s.\n", name);
	path_setenv("PKG_PATH");

	if (fexec_skipempty(BINDIR "/pkg_add", "-K", _pkgdb_getPKGDB_DIR(),
			    "-s", get_verification(),
	            doupdate > 1 ? "-uu" : (doupdate ? "-u" : ""),
	            Fake ? "-n" : "",
			    NoView ? "-L" : "",
			    View ? "-w" : "", View ? View : "",
			    Viewbase ? "-W" : "", Viewbase ? Viewbase : "",
			    Force ? "-f" : "",
			    Prefix ? "-p" : "", Prefix ? Prefix : "",
			    Verbose ? "-v" : "",
			    "-A", name, NULL)) {
		warnx("autoload of dependency `%s' failed%s",
			name, Force ? " (proceeding anyway)" : "!");
		if (!Force)
			++(*errc);
	} else {
		ret = 1;
	}

	return ret;
}

static int
pkg_do_installed(int *replacing, char replace_via[MaxPathSize], char replace_to[MaxPathSize],
    Boolean is_depoted_pkg, const char *dbdir)
{
	char    replace_from[MaxPathSize];
	char   *s;
	char    buf[MaxPathSize];
	char *best_installed;

	const size_t replace_via_size = MaxPathSize;
	const size_t replace_to_size = MaxPathSize;

	if ((s = strrchr(PkgName, '-')) == NULL) {
		warnx("Package name %s does not contain a version, bailing out", PkgName);
		return -1;
	}
	
	/*
	 * See if the pkg is already installed. If so, we might want to
	 * upgrade/replace it. Otherwise, just return and let pkg_do work.
	 */
	(void) snprintf(buf, sizeof(buf), "%.*s[0-9]*",
		(int)(s - PkgName) + 1, PkgName);
	best_installed = find_best_matching_installed_pkg(buf);
	if (best_installed == NULL)
		return 0;

	if (!Replace || Fake) {
		if (is_depoted_pkg) {
			free(best_installed);
			return 0;
		} else {
			warnx("other version '%s' already installed", best_installed);
			free(best_installed);
			return 1;	/* close enough for government work */
		}
	}

	/* XXX Should list the steps in Fake mode */
	snprintf(replace_from, sizeof(replace_from), "%s/%s/" REQUIRED_BY_FNAME,
		 dbdir, best_installed);
	snprintf(replace_via, replace_via_size, "%s/.%s." REQUIRED_BY_FNAME,
		 dbdir, best_installed);
	snprintf(replace_to, replace_to_size, "%s/%s/" REQUIRED_BY_FNAME,
		 dbdir, PkgName);

	if (Verbose)
		printf("Upgrading %s to %s.\n", best_installed, PkgName);

	if (fexists(replace_from)) {  /* Are there any dependencies? */
	  	/*
		 * Upgrade step 1/4: Check if the new version is ok with all pkgs
		 * (from +REQUIRED_BY) that require this pkg
		 */
		FILE *rb;                     /* +REQUIRED_BY file */
		char pkg2chk[MaxPathSize];

		rb = fopen(replace_from, "r");
		if (! rb) {
			warnx("Cannot open '%s' for reading%s", replace_from,
			      Force ? " (proceeding anyways)" : "");
			if (Force)
				goto ignore_replace_depends_check;
			else
				return -1;
		}
		while (fgets(pkg2chk, sizeof(pkg2chk), rb)) {
			package_t depPlist;
			FILE *depf;
			plist_t *depp;
			char depC[MaxPathSize];
							
			depPlist.head = depPlist.tail = NULL;

			s = strrchr(pkg2chk, '\n');
			if (s)
				*s = '\0'; /* strip trailing '\n' */
							
			/* 
			 * step into pkg2chk, read it's +CONTENTS file and see if
			 * all @pkgdep lines agree with PkgName (using pkg_match()) 
			 */
			snprintf(depC, sizeof(depC), "%s/%s/%s", dbdir, pkg2chk, CONTENTS_FNAME);
			depf = fopen(depC , "r");
			if (depf == NULL) {
				warnx("Cannot check depends in '%s'%s", depC, 
				      Force ? " (proceeding anyways)" : "!" );
				if (Force)
					goto ignore_replace_depends_check;
				else
					return -1;
			}
			read_plist(&depPlist, depf);
			fclose(depf);
							
			for (depp = depPlist.head; depp; depp = depp->next) {
				char base_new[MaxPathSize];
				char base_exist[MaxPathSize];
				char *s2;
							
				if (depp->type != PLIST_PKGDEP)
					continue;

				/*
				 *  Prepare basename (no versions) of both pkgs,
				 *  to see if we want to compare against that
				 *  one at all. 
				 */
				strlcpy(base_new, PkgName, sizeof(base_new));
				s2 = strpbrk(base_new, "<>[]?*{"); /* } */
				if (s2)
					*s2 = '\0';
				else {
					s2 = strrchr(base_new, '-');
					if (s2)
						*s2 = '\0';
				}
				strlcpy(base_exist, depp->name, sizeof(base_exist));
				s2 = strpbrk(base_exist, "<>[]?*{"); /* } */
				if (s2)
					*s2 = '\0';
				else {
					s2 = strrchr(base_exist, '-');
					if (s2)
						*s2 = '\0';
				}
				if (strcmp(base_new, base_exist) == 0) {
					/* Same pkg, so do the interesting compare */
					if (pkg_match(depp->name, PkgName)) {
						if (Verbose)
							printf("@pkgdep check: %s is ok for %s (in %s pkg)\n",
							       PkgName, depp->name, pkg2chk);
					} else {
						printf("Package %s requires %s, \n\tCannot replace with %s%s\n",
						       pkg2chk, depp->name, PkgName,
						       Force? " (proceeding anyways)" : "!");
						if (! Force)
							return -1;
					}
				}
			}
		}
		fclose(rb);

ignore_replace_depends_check:
		/*
		 * Upgrade step 2/4: Do the actual update by moving aside
		 * the +REQUIRED_BY file, deinstalling the old pkg, adding
		 * the new one and moving the +REQUIRED_BY file back
		 * into place (finished in step 3/4)
		 */
		if (Verbose)
			printf("mv %s %s\n", replace_from, replace_via);						
		if (rename(replace_from, replace_via) != 0)
			err(EXIT_FAILURE, "renaming \"%s\" to \"%s\" failed", replace_from, replace_via);

		*replacing = 1;
	}

	if (Verbose) {
		printf("%s/pkg_delete -K %s '%s'\n",
			BINDIR, dbdir, best_installed);
	}
	fexec(BINDIR "/pkg_delete", "-K", dbdir, best_installed, NULL);

	free(best_installed);

	return 0;
}

/*
 * Install a single package
 * Returns 0 if everything is ok, >0 else
 */
static int
pkg_do(const char *pkg, lpkg_head_t *pkgs)
{
	char    playpen[MaxPathSize];
	char    replace_via[MaxPathSize];
	char    replace_to[MaxPathSize];
	char   *buildinfo[BI_ENUM_COUNT];
	int	replacing = 0;
	char   dbdir[MaxPathSize];
	const char *tmppkg;
	FILE   *cfile;
	int     errc, err_prescan;
	plist_t *p;
	struct stat sb;
	struct utsname host_uname;
	uint64_t needed;
	Boolean	is_depoted_pkg = FALSE;
	lfile_t	*lfp;
	int	result;

	errc = 0;
	zapLogDir = 0;
	LogDir[0] = '\0';
	strlcpy(playpen, FirstPen, sizeof(playpen));
	memset(buildinfo, '\0', sizeof(buildinfo));

	umask(DEF_UMASK);

	tmppkg = fileFindByPath(pkg);
	if (tmppkg == NULL) {
		warnx("no pkg found for '%s', sorry.", pkg);
		return 1;
	}

	pkg = tmppkg;

	if (IS_URL(pkg)) {
		Home = fileGetURL(pkg);
		if (Home == NULL) {
			warnx("unable to fetch `%s' by URL", pkg);
		}

		/* make sure the pkg is verified */
		if (!verify(pkg)) {
			warnx("Package %s will not be extracted", pkg);
			goto bomb;
		}
	} else { /* local */
		if (!IS_STDIN(pkg)) {
		        /* not stdin */
			if (!ispkgpattern(pkg)) {
				if (stat(pkg, &sb) == FAIL) {
					warnx("can't stat package file '%s'", pkg);
					goto bomb;
				}
				/* make sure the pkg is verified */
				if (!verify(pkg)) {
					warnx("Package %s will not be extracted", pkg);
					goto bomb;
				}
			}
			LFILE_ADD(&files, lfp, CONTENTS_FNAME);
		} else {
		        /* some values for stdin */
			sb.st_size = 100000;	/* Make up a plausible average size */
		}
		Home = make_playpen(playpen, sizeof(playpen), sb.st_size * 4);
		if (!Home)
			warnx("unable to make playpen for %ld bytes",
			      (long) (sb.st_size * 4));
		result = unpack(pkg, &files);
		while ((lfp = TAILQ_FIRST(&files)) != NULL) {
			TAILQ_REMOVE(&files, lfp, lf_link);
			free(lfp);
		}
		if (result) {
			warnx("unable to extract table of contents file from `%s' - not a package?",
			      pkg);
			goto bomb;
		}
	}

	cfile = fopen(CONTENTS_FNAME, "r");
	if (!cfile) {
		warnx("unable to open table of contents file `%s' - not a package?",
		      CONTENTS_FNAME);
		goto bomb;
	}
	read_plist(&Plist, cfile);
	fclose(cfile);

	if (!IS_URL(pkg)) {
		/*
		 * Apply a crude heuristic to see how much space the package will
		 * take up once it's unpacked.  I've noticed that most packages
		 * compress an average of 75%, so multiply by 4 for good measure.
		 */

		needed = 4 * (uint64_t) sb.st_size;
		if (min_free(playpen) < needed) {
			warnx("projected size of %" MY_PRIu64 " bytes exceeds available free space\n"
			    "in %s. Please set your PKG_TMPDIR variable to point\n"
			    "to a location with more free space and try again.",
				needed, playpen);
			goto bomb;
		}

		/* Finally unpack the whole mess */
		if (unpack(pkg, NULL)) {
			warnx("unable to extract `%s'!", pkg);
			goto bomb;
		}
	}

	/* Check for sanity */
	if (sanity_check(pkg))
		goto bomb;

	/* Read the OS, version and architecture from BUILD_INFO file */
	if (!read_buildinfo(buildinfo)) {
		warn("can't read build information from %s", BUILD_INFO_FNAME);
		if (!Force) {
			warnx("aborting.");
			goto bomb;
		}
	}

	if (uname(&host_uname) < 0) {
		warnx("uname() failed.");
		if (!Force) {
			warnx("aborting.");
			goto bomb;
		}
	} else {
		int	status = Good;

		normalise_platform(&host_uname);

		/* check that we have read some values from buildinfo */
		if (buildinfo[BI_OPSYS] == NULL) {
			warnx("Missing operating system value from build information");
			status = Missing;
		}
		if (buildinfo[BI_MACHINE_ARCH] == NULL) {
			warnx("Missing machine architecture value from build information");
			status = Missing;
		}
		if (buildinfo[BI_OS_VERSION] == NULL) {
			warnx("Missing operating system version value from build information");
			status = Missing;
		}

		if (status == Good) {
			const char *effective_arch;

			if (OverrideMachine != NULL)
				effective_arch = OverrideMachine;
			else
				effective_arch = MACHINE_ARCH;

			/* If either the OS or arch are different, bomb */
			if (strcmp(OPSYS_NAME, buildinfo[BI_OPSYS]) != 0)
				status = Fatal;
			if (strcmp(effective_arch, buildinfo[BI_MACHINE_ARCH]) != 0)
				status = Fatal;

			/* If OS and arch are the same, warn if version differs */
			if (status == Good &&
			    strcmp(host_uname.release, buildinfo[BI_OS_VERSION]) != 0)
				status = Warning;

			if (status != Good) {
				warnx("Warning: package `%s' was built for a different version of the OS:", pkg);
				warnx("%s/%s %s (pkg) vs. %s/%s %s (this host)",
				    buildinfo[BI_OPSYS],
				    buildinfo[BI_MACHINE_ARCH],
				    buildinfo[BI_OS_VERSION],
				    OPSYS_NAME,
				    effective_arch,
				    host_uname.release);
			}
		}

		if (!Force && status == Fatal) {
			warnx("aborting.");
			goto bomb;
		}
	}

	/* Check if USE_ABI_DEPENDS or IGNORE_RECOMMENDED was set
	 * when this package was built. IGNORE_RECOMMENDED is historical. */

	if ((buildinfo[BI_USE_ABI_DEPENDS] != NULL &&
	    strcasecmp(buildinfo[BI_USE_ABI_DEPENDS], "YES") != 0) ||
	    (buildinfo[BI_IGNORE_RECOMMENDED] != NULL &&
	    strcasecmp(buildinfo[BI_IGNORE_RECOMMENDED], "NO") != 0)) {
		warnx("%s was built", pkg);
		warnx("\tto ignore recommended ABI dependencies, this may cause problems!\n");
	}

	/*
         * If we have a prefix, delete the first one we see and add this
         * one in place of it.
         */
	if (Prefix) {
		delete_plist(&Plist, FALSE, PLIST_CWD, NULL);
		add_plist_top(&Plist, PLIST_CWD, Prefix);
	}

	/* Protect against old packages with bogus @name fields */
	p = find_plist(&Plist, PLIST_NAME);
	if (p->name == NULL) {
		warnx("PLIST contains no @name field");
		goto bomb;
	}
	PkgName = p->name;

	if (fexists(VIEWS_FNAME))
		is_depoted_pkg = TRUE;
	
	/*
	 * Depoted packages' dbdir is the same as DEPOTBASE.  Non-depoted
	 * packages' dbdir comes from the command-line or the environment.
	 */
	if (is_depoted_pkg) {
		p = find_plist(&Plist, PLIST_CWD);
		if (p == NULL) {
			warn("no @cwd in +CONTENTS file?! aborting.");
			goto bomb;
		}
		(void) strlcpy(dbdir, dirname_of(p->name), sizeof(dbdir));
		(void) strlcpy(LogDir, p->name, sizeof(LogDir));
	} else {
		(void) strlcpy(dbdir, _pkgdb_getPKGDB_DIR(), sizeof(dbdir));
		(void) snprintf(LogDir, sizeof(LogDir), "%s/%s", dbdir, PkgName);
	}

	/* Set environment variables expected by the +INSTALL script. */
	setenv(PKG_PREFIX_VNAME, (p = find_plist(&Plist, PLIST_CWD)) ? p->name : ".", 1);
	setenv(PKG_METADATA_DIR_VNAME, LogDir, 1);
	setenv(PKG_REFCOUNT_DBDIR_VNAME, pkgdb_refcount_dir(), 1);
		
	/* make sure dbdir actually exists! */
	if (!(isdir(dbdir) || islinktodir(dbdir))) {
		if (fexec("mkdir", "-p", dbdir, NULL)) {
			errx(EXIT_FAILURE,
			    "Database-dir %s cannot be generated, aborting.",
			    dbdir);
		}
	}

	/* See if this package (exact version) is already registered */
	if (isdir(LogDir) && !Force) {
		if (!Automatic && is_automatic_installed(PkgName)) {
			if (mark_as_automatic_installed(PkgName, 0) == 0)
				warnx("package `%s' was already installed as "
				      "dependency, now marked as installed "
				      "manually", PkgName);
		} else {
			warnx("package `%s' already recorded as installed",
			      PkgName);
		}
		goto success;	/* close enough for government work */
	}

	/* See if some other version of us is already installed */
	switch (pkg_do_installed(&replacing, replace_via, replace_to, is_depoted_pkg, dbdir)) {
	case 0:
		break;
	case 1:
		errc = 1;
		goto success;
	case -1:
		goto bomb;
	}

	/* See if there are conflicting packages installed */
	for (p = Plist.head; p; p = p->next) {
		char *best_installed;

		if (p->type != PLIST_PKGCFL)
			continue;
		if (Verbose)
			printf("Package `%s' conflicts with `%s'.\n", PkgName, p->name);
		best_installed = find_best_matching_installed_pkg(p->name);
		if (best_installed) {
			warnx("Package `%s' conflicts with `%s', and `%s' is installed.",
			      PkgName, p->name, best_installed);
			free(best_installed);
			++errc;
		}
	}

	/* See if any of the installed packages conflicts with this one. */
	{
		char *inst_pkgname, *inst_pattern;

		if (some_installed_package_conflicts_with(PkgName, &inst_pkgname, &inst_pattern)) {
			warnx("Installed package `%s' conflicts with `%s' when trying to install `%s'.",
				inst_pkgname, inst_pattern, PkgName);
			free(inst_pkgname);
			free(inst_pattern);
			errc++;
		}
	}

	/* Quick pre-check if any conflicting dependencies are installed
	 * (e.g. version X is installed, but version Y is required)
	 */
	err_prescan=0;
	for (p = Plist.head; p; p = p->next) {
		char *best_installed;
		
		if (p->type != PLIST_PKGDEP)
			continue;
		if (Verbose)
			printf("Depends pre-scan: `%s' required.\n", p->name);
		best_installed = find_best_matching_installed_pkg(p->name);
		if (best_installed == NULL) {
			/* 
			 * required pkg not found. look if it's available with a more liberal
			 * pattern. If so, this will lead to problems later (check on "some
			 * other version of us is already installed" will fail, see above),
			 * and we better stop right now.
			 */
			char *s;
			int skip = -1;

			/* doing this right required to parse the full version(s),
			 * do a 99% solution here for now */
			if (strchr(p->name, '{'))
				continue;	/* would remove trailing '}' else */

			if ((s = strpbrk(p->name, "<>")) != NULL) {
				skip = 0;
			} else if (((s = strstr(p->name, "-[0-9]*")) != NULL) &&
				    (*(s + sizeof("-[0-9]*") - 1) == '\0')) {
				/* -[0-9]* already present so no need to */
				/* add it a second time */
				skip = -1;
			} else if ((s = strrchr(p->name, '-')) != NULL) {
				skip = 1;
			}
			
			if (skip >= 0) {
				char    buf[MaxPathSize];
		
				(void) snprintf(buf, sizeof(buf),
				    skip ? "%.*s[0-9]*" : "%.*s-[0-9]*",
				    (int)(s - p->name) + skip, p->name);
				best_installed = find_best_matching_installed_pkg(buf);
				if (best_installed) {
					int done = 0;

					if (Replace > 1)
					{
						int errc0 = 0;
						char tmp[MaxPathSize];

						warnx("Attempting to update `%s' using binary package\n", p->name);
						/* Yes, append .tgz after the version so the */
						/* pattern can match a filename. */
						snprintf(tmp, sizeof(tmp), "%s.tgz", p->name);
						done = installprereq(tmp, &errc0, 2);
					}
					else if (Replace)
					{
						warnx("To perform necessary upgrades on required packages specify -u twice.\n");
					}

					if (!done)
					{
						warnx("pkg `%s' required, but `%s' found installed.",
							  p->name, best_installed);
						if (Force) {
							warnx("Proceeding anyway.");
						} else {
							err_prescan++;
						}
					}
					free(best_installed);
				}
			}
		} else {
			free(best_installed);
		}
	}
	if (err_prescan > 0) {
		warnx("Please resolve this conflict!");
		errc += err_prescan;
		goto success; /* close enough */
	}	
	

	/* Now check the packing list for dependencies */
	for (p = Plist.head; p; p = p->next) {
		char *best_installed;

		if (p->type != PLIST_PKGDEP)
			continue;
		if (Verbose)
			printf("Package `%s' depends on `%s'.\n", PkgName, p->name);

		best_installed = find_best_matching_installed_pkg(p->name);

		if (best_installed == NULL) {
			/* required pkg not found - need to pull in */

			if (Fake) {
			        /* fake install (???) */
				if (Verbose)
					printf("Package dependency %s for %s not installed%s\n", p->name, pkg,
					    Force ? " (proceeding anyway)" : "!");
			} else {
				int done = 0;
				int errc0 = 0;

				done = installprereq(p->name, &errc0, (Replace > 1) ? 2 : 0);
				if (!done && !Force) {
					errc += errc0;
				}
			}
		} else {
			if (Verbose)
				printf(" - %s already installed.\n", best_installed);
			free(best_installed);
		}
	}

	if (errc != 0)
		goto bomb;

	/* If we're really installing, and have an installation file, run it */
	if (!NoInstall && fexists(INSTALL_FNAME)) {
		(void) fexec(CHMOD_CMD, "+x", INSTALL_FNAME, NULL);	/* make sure */
		if (Verbose)
			printf("Running install with PRE-INSTALL for %s.\n", PkgName);
		if (!Fake && fexec("./" INSTALL_FNAME, PkgName, "PRE-INSTALL", NULL)) {
			warnx("install script returned error status");
			errc = 1;
			goto success;	/* nothing to uninstall yet */
		}
	}

	/*
	 * Now finally extract the entire show if we're not going direct.
	 * We need to reset the package dbdir so that extract_plist()
	 * updates the correct pkgdb.byfile.db database.
	 */
	if (!Fake) {
		_pkgdb_setPKGDB_DIR(dbdir);
		if (!extract_plist(".", &Plist)) {
			errc = 1;
			goto fail;
		}
	}

	if (!Fake && fexists(MTREE_FNAME)) {
		warnx("Mtree file ignored for package %s", PkgName);
	}

	/* Run the installation script one last time? */
	if (!NoInstall && fexists(INSTALL_FNAME)) {
		if (Verbose)
			printf("Running install with POST-INSTALL for %s.\n", PkgName);
		if (!Fake && fexec("./" INSTALL_FNAME, PkgName, "POST-INSTALL", NULL)) {
			warnx("install script returned error status");
			errc = 1;
			goto fail;
		}
	}

	/* Time to record the deed? */
	if (!NoRecord && !Fake) {
		char    contents[MaxPathSize];

		if (!PkgName) {
			warnx("no package name! can't record package, sorry");
			errc = 1;
			goto success;	/* well, partial anyway */
		}
		(void) snprintf(LogDir, sizeof(LogDir), "%s/%s", dbdir, PkgName);
		zapLogDir = 1; /* LogDir contains something valid now */
		if (Verbose)
			printf("Attempting to record package into %s.\n", LogDir);
		if (make_hierarchy(LogDir)) {
			warnx("can't record package into '%s', you're on your own!",
			    LogDir);
			memset(LogDir, 0, sizeof(LogDir));
			errc = 1;
			goto success;	/* close enough for government work */
		}
		/* Make sure pkg_info can read the entry */
		(void) fexec(CHMOD_CMD, "a+rx", LogDir, NULL);

		/* Move all of the +-files into place */
		move_files(".", "+*", LogDir);

		/* Generate the +CONTENTS file in-place from the Plist */
		(void) snprintf(contents, sizeof(contents), "%s/%s", LogDir, CONTENTS_FNAME);
		cfile = fopen(contents, "w");
		if (!cfile) {
			warnx("can't open new contents file '%s'! can't register pkg",
			    contents);
			goto success;	/* can't log, but still keep pkg */
		}
		write_plist(&Plist, cfile, NULL);
		fclose(cfile);

		/* register dependencies */
		/* we could save some cycles here if we remembered what we
		 * installed above (in case we got a wildcard dependency)  */
		/* XXX remembering in p->name would NOT be good! */
		for (p = Plist.head; p; p = p->next) {
			if (p->type != PLIST_PKGDEP)
				continue;
			if (Verbose)
				printf("Attempting to record dependency on package `%s'\n", p->name);
			(void) snprintf(contents, sizeof(contents), "%s/%s", dbdir,
			    basename_of(p->name));
			if (ispkgpattern(p->name)) {
				char   *s;

				s = find_best_matching_installed_pkg(p->name);

				if (s == NULL)
					errx(EXIT_FAILURE, "Where did our dependency go?!");

				(void) snprintf(contents, sizeof(contents), "%s/%s", dbdir, s);
				free(s);
			}
			strlcat(contents, "/", sizeof(contents));
			strlcat(contents, REQUIRED_BY_FNAME, sizeof(contents));

			cfile = fopen(contents, "a");
			if (!cfile)
				warnx("can't open dependency file '%s'!\n"
				    "dependency registration is incomplete", contents);
			else {
				fprintf(cfile, "%s\n", PkgName);
				if (fclose(cfile) == EOF)
					warnx("cannot properly close file %s", contents);
			}
		}
		if (Automatic)
			mark_as_automatic_installed(PkgName, 1);
		if (Verbose)
			printf("Package %s registered in %s\n", PkgName, LogDir);
	}

	if ((p = find_plist(&Plist, PLIST_DISPLAY)) != NULL) {
		FILE   *fp;
		char    buf[BUFSIZ];

		(void) snprintf(buf, sizeof(buf), "%s/%s", LogDir, p->name);
		fp = fopen(buf, "r");
		if (fp) {
			putc('\n', stdout);
			while (fgets(buf, sizeof(buf), fp))
				fputs(buf, stdout);
			putc('\n', stdout);
			(void) fclose(fp);
		} else
			warnx("cannot open %s as display file", buf);
	}

	/* Add the package to a default view. */
	if (!Fake && !NoView && is_depoted_pkg) {
		if (Verbose) {
			printf("%s/pkg_view -d %s %s%s %s%s %sadd %s\n",
				BINDIR, dbdir,
				View ? "-w " : "", View ? View : "",
				Viewbase ? "-W " : "", Viewbase ? Viewbase : "",
				Verbose ? "-v " : "", PkgName);
		}

		fexec_skipempty(BINDIR "/pkg_view", "-d", dbdir,
				View ? "-w " : "", View ? View : "",
				Viewbase ? "-W " : "", Viewbase ? Viewbase : "",
				Verbose ? "-v " : "", "add", PkgName, NULL);
	}

	goto success;

bomb:
	errc = 1;
	goto success;

fail:
	/* Nuke the whole (installed) show, XXX but don't clean directories */
	if (!Fake)
		delete_package(FALSE, FALSE, &Plist, FALSE);

success:
	/* delete the packing list contents */
	free_plist(&Plist);
	leave_playpen(Home);

	if (replacing) {
		/*
		 * Upgrade step 3/4: move back +REQUIRED_BY file
		 * (see also step 2/4)
		 */
		if (rename(replace_via, replace_to) != 0)
			err(EXIT_FAILURE, "renaming \"%s\" to \"%s\" failed", replace_via, replace_to);
		
		/*
		 * Upgrade step 4/4: Fix pkgs that depend on us to
		 * depend on the new version instead of the old
		 * one by fixing @pkgdep lines in +CONTENTS files.
		 */
		/* TODO */
	}

	return errc;
}

void
cleanup(int signo)
{
	static int alreadyCleaning;
	void   (*oldint) (int);
	void   (*oldhup) (int);
	int    saved_errno;

	saved_errno = errno;
	oldint = signal(SIGINT, SIG_IGN);
	oldhup = signal(SIGHUP, SIG_IGN);

	if (!alreadyCleaning) {
		alreadyCleaning = 1;
		if (signo)
			printf("Signal %d received, cleaning up.\n", signo);
		if (!Fake && zapLogDir && LogDir[0])
			(void) fexec(REMOVE_CMD, "-fr", LogDir, NULL);
		leave_playpen(Home);
		if (signo)
			exit(1);
	}
	signal(SIGINT, oldint);
	signal(SIGHUP, oldhup);
	errno = saved_errno;
}

int
pkg_perform(lpkg_head_t *pkgs)
{
	int     err_cnt = 0;
	lpkg_t *lpp;

	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	TAILQ_INIT(&files);

	while ((lpp = TAILQ_FIRST(pkgs)) != NULL) {
		path_prepend_from_pkgname(lpp->lp_name);
		err_cnt += pkg_do(lpp->lp_name, pkgs);
		path_prepend_clear();
		TAILQ_REMOVE(pkgs, lpp, lp_link);
		free_lpkg(lpp);
	}
	
	ftp_stop();
	
	return err_cnt;
}
