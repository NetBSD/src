/*	$NetBSD: perform.c,v 1.52 2000/06/16 23:49:17 sjg Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: perform.c,v 1.44 1997/10/13 15:03:46 jkh Exp";
#else
__RCSID("$NetBSD: perform.c,v 1.52 2000/06/16 23:49:17 sjg Exp $");
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

#include <err.h>
#include "lib.h"
#include "add.h"

#include <signal.h>
#include <string.h>
#include <sys/wait.h>

static char LogDir[FILENAME_MAX];
static int zapLogDir;		/* Should we delete LogDir? */

static package_t Plist;
static char *Home;

/*
 * Called to see if pkg is already installed as some other version, 
 * note found version in "note".
 */
static int
note_whats_installed(const char *found, char *note)
{
	(void) strcpy(note, found);
	return 0;
}

static int
sanity_check(char *pkg)
{
	int     code = 0;

	if (!fexists(CONTENTS_FNAME)) {
		warnx("package %s has no CONTENTS file!", pkg);
		code = 1;
	} else if (!fexists(COMMENT_FNAME)) {
		warnx("package %s has no COMMENT file!", pkg);
		code = 1;
	} else if (!fexists(DESC_FNAME)) {
		warnx("package %s has no DESC file!", pkg);
		code = 1;
	}
	return code;
}

/*
 * This is seriously ugly code following.  Written very fast!
 * [And subsequently made even worse..  Sigh!  This code was just born
 * to be hacked, I guess.. :) -jkh]
 */
static int
pkg_do(char *pkg)
{
	char    pkg_fullname[FILENAME_MAX];
	char    playpen[FILENAME_MAX];
	char    extract_contents[FILENAME_MAX];
	char   *where_to, *tmp, *extract;
	char   *dbdir;
	FILE   *cfile;
	int     code;
	plist_t *p;
	struct stat sb;
	int     inPlace;

	code = 0;
	zapLogDir = 0;
	LogDir[0] = '\0';
	strcpy(playpen, FirstPen);
	inPlace = 0;
	dbdir = (tmp = getenv(PKG_DBDIR)) ? tmp : DEF_LOG_DIR;

	/* make sure dbdir actually exists! */
	if (!(isdir(dbdir) || islinktodir(dbdir))) {
		if (vsystem("/bin/mkdir -p -m 755 %s", dbdir)) {
			errx(1, "Database-dir %s cannot be generated, aborting.",
			    dbdir);
		}
	}
	
	/* Are we coming in for a second pass, everything already extracted?
	 * (Slave mode) */
	if (!pkg) {
		fgets(playpen, FILENAME_MAX, stdin);
		playpen[strlen(playpen) - 1] = '\0';	/* remove newline! */
		if (chdir(playpen) == FAIL) {
			warnx("add in SLAVE mode can't chdir to %s", playpen);
			return 1;
		}
		read_plist(&Plist, stdin);
		where_to = playpen;
	}
	/* Nope - do it now */
	else {
		/*
		 * Is it an ftp://foo.bar.baz/file.tgz or http://foo.bar.baz/file.tgz
		 * specification?
		 */
		if (IS_URL(pkg)) {
			char buf[FILENAME_MAX];
			char *tmppkg = pkg;

			if (ispkgpattern(pkg)) {
				/* Handle wildcard depends */

				char *s;
				s=fileFindByPath(NULL, pkg);
				if (s == NULL) {
					warnx("no pkg found for '%s', sorry.", pkg);
					return 1;
				}
				strcpy(buf, s);
				tmppkg = buf;
			}
			
			if (!(Home = fileGetURL(NULL, tmppkg))) {
				warnx("unable to fetch `%s' by URL", tmppkg);
				if (ispkgpattern(pkg))
					return 1;

				if (strstr(pkg, ".tgz") != NULL) {
					/* There already is a ".tgz" - give up 
					 * (We don't want to pretend to be exceedingly
					 *  clever - the user should give something sane!)
					 */
					return 1;
			}
			
				
				/* Second chance - maybe just a package name was given,
				 * without even a wildcard as a version. Tack on
				 * the same pattern as we do for local packages: "-[0-9]*",
				 * plus a ".tgz" as we're talking binary pkgs here.
				 * Then retry.
				 */
				{
					char *s;
					char buf2[FILENAME_MAX];
					
					snprintf(buf2, sizeof(buf2), "%s-[0-9]*.tgz", tmppkg);
					s=fileFindByPath(NULL, buf2);
					if (s == NULL) {
						warnx("no pkg found for '%s' on 2nd try, sorry.", buf2);
						return 1;
					}
					strcpy(buf, s);
					tmppkg = buf;
					if (!(Home = fileGetURL(NULL, tmppkg))) {
						warnx("unable to fetch `%s' by URL", tmppkg);
				return 1;
			}
				}
			}
			where_to = Home;
			strcpy(pkg_fullname, tmppkg);
			cfile = fopen(CONTENTS_FNAME, "r");
			if (!cfile) {
				warnx("unable to open table of contents file `%s' - not a package?",
				      CONTENTS_FNAME);
				goto bomb;
			}
			read_plist(&Plist, cfile);
			fclose(cfile);
		} else { /* no URL */
			strcpy(pkg_fullname, pkg);	/* copy for sanity's sake, could remove pkg_fullname */
			if (strcmp(pkg, "-")) {
			        /* not stdin */
				if (!ispkgpattern(pkg_fullname)
				    && stat(pkg_fullname, &sb) == FAIL) {
					warnx("can't stat package file '%s'", pkg_fullname);
					goto bomb;
				}
				(void) snprintf(extract_contents, sizeof(extract_contents), "--fast-read %s", CONTENTS_FNAME);
				extract = extract_contents;
			} else {
			        /* some values for stdin */
				extract = NULL;
				sb.st_size = 100000;	/* Make up a plausible average size */
			}
			Home = make_playpen(playpen, sizeof(playpen), sb.st_size * 4);
			if (!Home)
				warnx("unable to make playpen for %ld bytes",
				      (long) (sb.st_size * 4));
			where_to = Home;
			if (unpack(pkg_fullname, extract)) {
				warnx("unable to extract table of contents file from `%s' - not a package?",
				      pkg_fullname);
				goto bomb;
			}
			cfile = fopen(CONTENTS_FNAME, "r");
			if (!cfile) {
				warnx("unable to open table of contents file `%s' - not a package?",
				      CONTENTS_FNAME);
				goto bomb;
			}
			read_plist(&Plist, cfile);
			fclose(cfile);

			/* Extract directly rather than moving?  Oh goodie! */
			if (find_plist_option(&Plist, "extract-in-place")) {
				if (Verbose)
					printf("Doing in-place extraction for %s\n", pkg_fullname);
				p = find_plist(&Plist, PLIST_CWD);
				if (p) {
					if (!(isdir(p->name) || islinktodir(p->name)) && !Fake) {
						if (Verbose)
							printf("Desired prefix of %s does not exist, creating.\n", p->name);
						(void) vsystem("mkdir -p %s", p->name);
					}
					if (chdir(p->name) == -1) {
						warn("unable to change directory to `%s'", p->name);
						goto bomb;
					}
					where_to = p->name;
					inPlace = 1;
				} else {
					warnx(
					    "no prefix specified in `%s' - this is a bad package!",
					    pkg_fullname);
					goto bomb;
				}
			}

			/*
		         * Apply a crude heuristic to see how much space the package will
		         * take up once it's unpacked.  I've noticed that most packages
		         * compress an average of 75%, so multiply by 4 for good measure.
		         */

			if (!inPlace && min_free(playpen) < sb.st_size * 4) {
				warnx("projected size of %ld exceeds available free space.\n"
				    "Please set your PKG_TMPDIR variable to point to a location with more\n"
				    "free space and try again", (long) (sb.st_size * 4));
				warnx("not extracting %s\ninto %s, sorry!",
				    pkg_fullname, where_to);
				goto bomb;
			}

			/* If this is a direct extract and we didn't want it, stop now */
			if (inPlace && Fake)
				goto success;

			/* Finally unpack the whole mess */
			if (unpack(pkg_fullname, NULL)) {
				warnx("unable to extract `%s'!", pkg_fullname);
				goto bomb;
			}
		}

		/* Check for sanity */
		if (sanity_check(pkg))
			goto bomb;

		/* If we're running in MASTER mode, just output the plist and return */
		if (AddMode == MASTER) {
			printf("%s\n", where_playpen());
			write_plist(&Plist, stdout);
			return 0;
		}
	}

	/*
         * If we have a prefix, delete the first one we see and add this
         * one in place of it.
         */
	if (Prefix) {
		delete_plist(&Plist, FALSE, PLIST_CWD, NULL);
		add_plist_top(&Plist, PLIST_CWD, Prefix);
	}

	setenv(PKG_PREFIX_VNAME, (p = find_plist(&Plist, PLIST_CWD)) ? p->name : ".", 1);
	/* Protect against old packages with bogus @name fields */
	PkgName = (p = find_plist(&Plist, PLIST_NAME)) ? p->name : "anonymous";

	/* See if this package (exact version) is already registered */
	(void) snprintf(LogDir, sizeof(LogDir), "%s/%s", dbdir, PkgName);
	if ((isdir(LogDir) || islinktodir(LogDir)) && !Force) {
		warnx("package `%s' already recorded as installed", PkgName);
		code = 1;
		goto success;	/* close enough for government work */
	}

	/* See if some other version of us is already installed */
	{
		char   *s;

		if ((s = strrchr(PkgName, '-')) != NULL) {
			char    buf[FILENAME_MAX];
			char    installed[FILENAME_MAX];

			(void) snprintf(buf, sizeof(buf), "%.*s[0-9]*",
				(int)(s - PkgName) + 1, PkgName);
			if (findmatchingname(dbdir, buf, note_whats_installed, installed) > 0) {
				warnx("other version '%s' already installed", installed);
				code = 1;
				goto success;	/* close enough for government work */
			}
		}
	}

	/* See if there are conflicting packages installed */
	for (p = Plist.head; p; p = p->next) {
		char    installed[FILENAME_MAX];

		if (p->type != PLIST_PKGCFL)
			continue;
		if (Verbose)
			printf("Package `%s' conflicts with `%s'.\n", PkgName, p->name);

		/* was: */
		/* if (!vsystem("/usr/sbin/pkg_info -qe '%s'", p->name)) { */
		if (findmatchingname(dbdir, p->name, note_whats_installed, installed) > 0) {
			warnx("Conflicting package `%s'installed, please use\n"
			      "\t\"pkg_delete %s\" first to remove it!\n", installed, installed);
			++code;
		}
	}

	/* Quick pre-check if any conflicting dependencies are installed
	 * (e.g. version X is installed, but version Y is required)
	 */
	for (p = Plist.head; p; p = p->next) {
		char installed[FILENAME_MAX];
		
		if (p->type != PLIST_PKGDEP)
			continue;
		if (Verbose)
			printf("Depends pre-scan: `%s' required.\n", p->name);
		/* if (vsystem("/usr/sbin/pkg_info -qe '%s'", p->name)) { */
		if (findmatchingname(dbdir, p->name, note_whats_installed, installed) <= 0) {
			/* 
			 * required pkg not found. look if it's available with a more liberal
			 * pattern. If so, this will lead to problems later (check on "some
			 * other version of us is already installed" will fail, see above),
			 * and we better stop right now.
			 */
			char *s;
			char *fmt = NULL;
			int skip = -1;

			/* doing this right required to parse the full version(s),
			 * do a 99% solution here for now */
			if ((s = strpbrk(p->name, "<>")) != NULL) {
				fmt = "%.*s-[0-9]*";
				skip = 0;
			} else if ((s = strrchr(p->name, '-')) != NULL) {
				fmt = "%.*s[0-9]*";
				skip = 1;
			}
			
			if (fmt != NULL) {
				char    buf[FILENAME_MAX];
		
				(void) snprintf(buf, sizeof(buf), fmt,
					(int)(s - p->name) + skip, p->name);
				if (findmatchingname(dbdir, buf, note_whats_installed, installed) > 0) {
					warnx("pkg `%s' required, but `%s' found installed.",
					      p->name, installed);
					if (Force) {
						warnx("Proceeding anyways.");
					} else {	
						warnx("Please resolve this conflict!");
						code = 1;
						goto success; /* close enough */
					}
				}
			}
		}
	}
	

	/* Now check the packing list for dependencies */
	for (p = Plist.head; p; p = p->next) {
		char    installed[FILENAME_MAX];

		if (p->type != PLIST_PKGDEP)
			continue;
		if (Verbose)
			printf("Package `%s' depends on `%s'.\n", PkgName, p->name);
		/* if (vsystem("/usr/sbin/pkg_info -qe '%s'", p->name)) { */
		if (findmatchingname(dbdir, p->name, note_whats_installed, installed) != 1) {
			/* required pkg not found - need to pull in */

			if (!Fake) {
				if (!IS_URL(pkg) && !getenv("PKG_ADD_BASE")) {
					/* install depending pkg from local disk */

					char    path[FILENAME_MAX], *cp = NULL;

					(void) snprintf(path, sizeof(path), "%s/%s.tgz", Home, p->name);
					if (fexists(path))
						cp = path;
					else
						cp = fileFindByPath(pkg, p->name); /* files & wildcards */
					if (cp) {
						if (Verbose)
							printf("Loading it from %s.\n", cp);
						if (vsystem("%s/pkg_add %s%s%s %s%s",
							    BINDIR,
							    Force ? "-f " : "",
							    Prefix ? "-p " : "",
							    Prefix ? Prefix : "",
							    Verbose ? "-v " : "",
							    cp)) {
							warnx("autoload of dependency `%s' failed%s",
							    cp, Force ? " (proceeding anyway)" : "!");
							if (!Force)
								++code;
						}
					} else {
						warnx("<%s> (1) add of dependency `%s' failed%s",
						    pkg, p->name, Force ? " (proceeding anyway)" : "!");
						if (!Force)
							++code;
					} /* cp */
				} else {
					/* pkg is url -> install depending pkg via FTP */

					char   *saved_Current;	/* allocated/set by save_dirs(), */
					char   *saved_Previous;	/* freed by restore_dirs() */
					char   *cp, *new_pkg, *new_name;

					new_pkg = pkg;
					new_name = p->name;

					if (ispkgpattern(p->name)) {
						/* Handle wildcard depends here */

						char *s;
						s=fileFindByPath(pkg, p->name);
						if (Verbose) {
							printf("HF: pkg='%s'\n", pkg);
							printf("HF: s='%s'\n", s);
						}

						/* adjust new_pkg and new_name */
						new_pkg = NULL;
						new_name = s;
					}

					/* makeplaypen() and leave_playpen() clobber Current and
					 * Previous, save them! */
						save_dirs(&saved_Current, &saved_Previous);

					if ((cp = fileGetURL(new_pkg, new_name)) != NULL) {
							if (Verbose)
							printf("Finished loading %s over FTP.\n", new_name);
							if (!fexists(CONTENTS_FNAME)) {
								warnx("autoloaded package %s has no %s file?",
								    p->name, CONTENTS_FNAME);
								if (!Force)
									++code;
						} else {
							if (vsystem("(pwd; cat %s) | pkg_add %s%s%s %s-S",
									CONTENTS_FNAME,
									Force ? "-f " : "",
									Prefix ? "-p " : "",
									Prefix ? Prefix : "",
								Verbose ? "-v " : "")) {
								warnx("<%s> (2) add of dependency `%s' failed%s",
								      pkg, p->name, Force ? " (proceeding anyway)" : "!");
								if (!Force)
									++code;
							} else if (Verbose) {
								printf("\t`%s' loaded successfully as `%s'.\n", p->name, new_name);
							}
						}
							/* Nuke the temporary playpen */
							leave_playpen(cp);

					} else {
						if (Verbose)
							warnx("HF: fileGetURL('%s', '%s') failed", new_pkg, new_name);
						if (!Force)
							code++;
					}
					
					restore_dirs(saved_Current, saved_Previous);
				}
			} else {
			        /* fake install (???) */
				if (Verbose)
					printf("Package dependency %s for %s not installed%s\n", p->name, pkg,
					    Force ? " (proceeding anyway)" : "!");
			}
		} else if (Verbose) {
			printf(" - %s already installed.\n", installed);
		}
	}

	if (code != 0)
		goto bomb;

	/* Look for the requirements file */
	if (fexists(REQUIRE_FNAME)) {
		vsystem("%s +x %s", CHMOD, REQUIRE_FNAME);	/* be sure */
		if (Verbose)
			printf("Running requirements file first for %s.\n", PkgName);
		if (!Fake && vsystem("./%s %s INSTALL", REQUIRE_FNAME, PkgName)) {
			warnx("package %s fails requirements %s", pkg_fullname,
			    Force ? "installing anyway" : "- not installed");
			if (!Force) {
				code = 1;
				goto success;	/* close enough for government work */
			}
		}
	}
	
	/* If we're really installing, and have an installation file, run it */
	if (!NoInstall && fexists(INSTALL_FNAME)) {
		vsystem("%s +x %s", CHMOD, INSTALL_FNAME);	/* make sure */
		if (Verbose)
			printf("Running install with PRE-INSTALL for %s.\n", PkgName);
		if (!Fake && vsystem("./%s %s PRE-INSTALL", INSTALL_FNAME, PkgName)) {
			warnx("install script returned error status");
			code = 1;
			goto success;	/* nothing to uninstall yet */
		}
	}

	/* Now finally extract the entire show if we're not going direct */
	if (!inPlace && !Fake)
	    if (!extract_plist(".", &Plist)) {
		code = 1;
		goto fail;
	    }

	if (!Fake && fexists(MTREE_FNAME)) {
		if (Verbose)
			printf("Running mtree for %s.\n", PkgName);
		p = find_plist(&Plist, PLIST_CWD);
		if (Verbose)
			printf("mtree -U -f %s -d -e -p %s\n", MTREE_FNAME, p ? p->name : "/");
		if (!Fake) {
			if (vsystem("%s/mtree -U -f %s -d -e -p %s", BINDIR, MTREE_FNAME, p ? p->name : "/"))
				warnx("mtree returned a non-zero status - continuing");
		}
		unlink(MTREE_FNAME); /* remove this line to tar up pkg later  - HF */
	}

	/* Run the installation script one last time? */
	if (!NoInstall && fexists(INSTALL_FNAME)) {
		if (Verbose)
			printf("Running install with POST-INSTALL for %s.\n", PkgName);
		if (!Fake && vsystem("./%s %s POST-INSTALL", INSTALL_FNAME, PkgName)) {
			warnx("install script returned error status");
			code = 1;
			goto fail;
		}
	}

	/* Time to record the deed? */
	if (!NoRecord && !Fake) {
		char    contents[FILENAME_MAX];
		FILE   *cfile;

		umask(022);
		if (getuid() != 0)
			warnx("not running as root - trying to record install anyway");
		if (!PkgName) {
			warnx("no package name! can't record package, sorry");
			code = 1;
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
			code = 1;
			goto success;	/* close enough for government work */
		}
		/* Make sure pkg_info can read the entry */
		vsystem("%s a+rx %s", CHMOD, LogDir);
		if (fexists(DEINSTALL_FNAME))
			move_file(".", DEINSTALL_FNAME, LogDir);
		if (fexists(REQUIRE_FNAME))
			move_file(".", REQUIRE_FNAME, LogDir);
		if (fexists(SIZE_PKG_FNAME))
			move_file(".", SIZE_PKG_FNAME, LogDir);
		if (fexists(SIZE_ALL_FNAME))
			move_file(".", SIZE_ALL_FNAME, LogDir);
		(void) snprintf(contents, sizeof(contents), "%s/%s", LogDir, CONTENTS_FNAME);
		cfile = fopen(contents, "w");
		if (!cfile) {
			warnx("can't open new contents file '%s'! can't register pkg",
			    contents);
			goto success;	/* can't log, but still keep pkg */
		}
		write_plist(&Plist, cfile);
		fclose(cfile);
		move_file(".", DESC_FNAME, LogDir);
		move_file(".", COMMENT_FNAME, LogDir);
		if (fexists(BUILD_VERSION_FNAME))
			move_file(".", BUILD_VERSION_FNAME, LogDir);
		if (fexists(BUILD_INFO_FNAME))
			move_file(".", BUILD_INFO_FNAME, LogDir);
		if (fexists(DISPLAY_FNAME))
			move_file(".", DISPLAY_FNAME, LogDir);

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
				s = findbestmatchingname(dirname_of(contents),
				    basename_of(contents));
				if (s != NULL) {
					char   *t;
					t = strrchr(contents, '/');
					strcpy(t + 1, s);
				} else {
					errx(1, "Where did our dependency go?!");
					/* this shouldn't happen... X-) */
				}
			}
			strcat(contents, "/");
			strcat(contents, REQUIRED_BY_FNAME);

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

	goto success;

bomb:
	code = 1;
	goto success;

fail:
	/* Nuke the whole (installed) show, XXX but don't clean directories */
	if (!Fake)
		delete_package(FALSE, FALSE, &Plist);

success:
	/* delete the packing list contents */
	free_plist(&Plist);
	leave_playpen(Home);
	return code;
}

void
cleanup(int signo)
{
	static int alreadyCleaning;
	void    (*oldint) (int);
	void    (*oldhup) (int);
	oldint = signal(SIGINT, SIG_IGN);
	oldhup = signal(SIGHUP, SIG_IGN);

	if (!alreadyCleaning) {
		alreadyCleaning = 1;
		if (signo)
			printf("Signal %d received, cleaning up.\n", signo);
		if (!Fake && zapLogDir && LogDir[0])
			vsystem("%s -rf %s", REMOVE_CMD, LogDir);
		leave_playpen(Home);
		if (signo)
			exit(1);
	}
	signal(SIGINT, oldint);
	signal(SIGHUP, oldhup);
}

int
pkg_perform(lpkg_head_t *pkgs)
{
	int     err_cnt = 0;
	lpkg_t *lpp;

	signal(SIGINT, cleanup);
	signal(SIGHUP, cleanup);

	if (AddMode == SLAVE)
		err_cnt = pkg_do(NULL);
	else {
		while ((lpp = TAILQ_FIRST(pkgs))) {
			err_cnt += pkg_do(lpp->lp_name);
			TAILQ_REMOVE(pkgs, lpp, lp_link);
			free_lpkg(lpp);
		}
	}
	
	ftp_stop();
	
	return err_cnt;
}
