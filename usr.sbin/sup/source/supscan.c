/*	$NetBSD: supscan.c,v 1.19.2.1 2012/05/23 10:08:30 yamt Exp $	*/

/*
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * supscan -- SUP Scan File Builder
 *
 * Usage: supscan [ -v ] collection [ basedir ]
 *	  supscan [ -v ] -f dirfile
 *	  supscan [ -v ] -s
 *	-f	"file" -- use dirfile instead of system coll.dir
 *	-s	"system" -- perform scan for system supfile
 *	-v	"verbose" -- print messages as you go
 *	collection	-- name of the desired collection if not -s
 *	basedir		-- name of the base directory, if not
 *			   the default or recorded in coll.dir
 *	dirfile		-- name of replacement for system coll.dir.
 *
 **********************************************************************
 * HISTORY
 * Revision 1.14  92/08/11  12:08:30  mrt
 * 	Picked up Brad's deliniting and variable argument changes
 * 	[92/08/10            mrt]
 *
 * Revision 1.13  92/02/08  18:04:44  dlc
 * 	Once again revised localhost().  Do not use gethostbyname() at
 * 	all, but assume that the host names in the coll.host file are at
 * 	least a prefix of the fully qualified name.  Modcoll (and related
 * 	scripts) will maintain this fact.
 * 	[92/02/08            dlc]
 *
 * Revision 1.12  91/08/17  23:35:31  dlc
 * 	Changes to localhost() function:
 * 		- Use host name in kernel for local host name; assume it is
 * 		  fully qualified.
 * 		- If gethostbyname() of host to see if we are the repository
 * 		  fails, with TRY_AGAIN or NO_RECOVERY, then use the "host"
 * 		  parameter.  Print a diagnostic in this case.
 * 	[91/08/17            dlc]
 *
 * Revision 1.11  90/04/04  10:53:01  dlc
 * 	Changed localhost to retry getting the local host name 4 times with
 * 	30 second sleep intervals before aborting; after 4 tries, things are
 * 	probably too messed up for the supscan to do anything useful
 * 	[90/04/04            dlc]
 *
 * Revision 1.10  89/08/03  19:49:33  mja
 * 	Updated to use v*printf() in place of _doprnt().
 * 	[89/04/19            mja]
 *
 * Revision 1.9  89/06/18  14:41:37  gm0w
 * 	Fixed up some notify messages of errors to use "SUP:" prefix.
 * 	[89/06/18            gm0w]
 *
 * 13-May-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed goaway to longjmp back to top-level to scan next
 *	collection. [V7.6]
 *
 * 19-Feb-88  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added -f <filename> switch to scan all (or part) of the
 *	collections in a file of collection/base-directory pairs.
 *	[V7.5]
 *
 * 27-Dec-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed nameserver support (which means to use a new
 *	datafile).
 *
 * 09-Sep-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Use case-insensitive hostname comparison.
 *
 * 28-Jun-87  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code for "release" support. [V6.4]
 *
 * 05-Jan-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed collection setup errors to be non-fatal. [V5.3]
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Moved most of the scanning code to scan.c. [V4.2]
 *
 * 02-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added "-s" option.
 *
 * 22-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Merged 4.1 and 4.2 versions together.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for 4.2 BSD.
 *
 **********************************************************************
 */

#include <netdb.h>
#include <setjmp.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/param.h>
#include "supcdefs.h"
#include "supextern.h"
#include "libc.h"
#include "c.h"

#define PGMVERSION 6

/*******************************************
 ***    D A T A   S T R U C T U R E S    ***
 *******************************************/

struct scan_collstruct {	/* one per collection to be upgraded */
	char *Cname;		/* collection name */
	char *Cbase;		/* local base directory */
	char *Cprefix;		/* local collection pathname prefix */
	struct scan_collstruct *Cnext;	/* next collection */
};
typedef struct scan_collstruct SCAN_COLLECTION;

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

int trace;			/* -v flag */
int quiet;			/* -q flag */

SCAN_COLLECTION *firstC;	/* collection list pointer */
char *collname;			/* collection name */
char *basedir;			/* base directory name */
char *prefix;			/* collection pathname prefix */
time_t lasttime = 0;		/* time of last upgrade */
time_t scantime;		/* time of this scan */
int newonly = FALSE;		/* new files only */
jmp_buf sjbuf;			/* jump location for errors */

TREELIST *listTL;		/* list of all files specified by <coll>.list */
TREE *listT;			/* final list of files in collection */
TREE *refuseT = NULL;		/* list of all files specified by <coll>.list */


void usage(void);
int init(int, char **);
static SCAN_COLLECTION *getscancoll(char *, char *, char *);
int localhost(char *);
int main(int, char **);

/*************************************
 ***    M A I N   R O U T I N E    ***
 *************************************/

int
main(int argc, char **argv)
{
	SCAN_COLLECTION * volatile c;	/* Avoid longjmp clobbering */
	volatile int errs;
#ifdef RLIMIT_DATA
	struct rlimit dlim;

	if (getrlimit(RLIMIT_DATA, &dlim) == -1)
		goaway("Error getting resource limit (%s)",
		    strerror(errno));
	if (dlim.rlim_cur != dlim.rlim_max) {
		dlim.rlim_cur = dlim.rlim_max;
		if (setrlimit(RLIMIT_DATA, &dlim) == -1)
			goaway("Error setting resource limit (%s)",
			    strerror(errno));
	}
#endif

	errs = init(argc, argv);	/* process arguments */
	if (errs) {
		fprintf(stderr, "supscan: %d collections had errors", errs);
		return 1;
	}
	for (c = firstC; c; c = c->Cnext) {
		collname = c->Cname;
		basedir = c->Cbase;
		prefix = c->Cprefix;
		(void) chdir(basedir);
		scantime = time(NULL);
		if (!quiet)
			printf("SUP Scan for %s starting at %s", collname,
			    ctime(&scantime));
		(void) fflush(stdout);
		if (!setjmp(sjbuf)) {
			makescanlists();	/* record names in scan files */
			scantime = time(NULL);
			if (!quiet)
				printf("SUP Scan for %s completed at %s",
				    collname, ctime(&scantime));
		} else {
			fprintf(stderr,
			    "SUP: Scan for %s aborted at %s", collname,
			    ctime(&scantime));
			errs++;
		}
		if (!quiet)
			(void) fflush(stdout);
	}
	while ((c = firstC) != NULL) {
		firstC = firstC->Cnext;
		free(c->Cname);
		free(c->Cbase);
		if (c->Cprefix)
			free(c->Cprefix);
		free(c);
	}
	return errs ? 1 : 0;
}
/*****************************************
 ***    I N I T I A L I Z A T I O N    ***
 *****************************************/

void
usage(void)
{
	fprintf(stderr, "Usage: supscan [ -vq ] collection [ basedir ]\n");
	fprintf(stderr, "       supscan [ -vq ] -f dirfile\n");
	fprintf(stderr, "       supscan [ -vq ] -s\n");
	exit(1);
}

int
init(int argc, char **argv)
{
	char buf[STRINGLENGTH], fbuf[STRINGLENGTH], *p, *q;
	FILE *f;
	SCAN_COLLECTION **c;
	int fflag, sflag;
	char *filename = NULL;
	int errs = 0;

	quiet = FALSE;
	trace = FALSE;
	fflag = FALSE;
	sflag = FALSE;
	while (argc > 1 && argv[1][0] == '-') {
		switch (argv[1][1]) {
		case 'f':
			fflag = TRUE;
			if (argc == 2)
				usage();
			--argc;
			argv++;
			filename = argv[1];
			break;
		case 'q':
			quiet = TRUE;
			break;
		case 'v':
			trace = TRUE;
			break;
		case 's':
			sflag = TRUE;
			break;
		default:
			fprintf(stderr, "supscan: Invalid flag %s ignored\n",
			    argv[1]);
			(void) fflush(stderr);
		}
		--argc;
		argv++;
	}
	if (!fflag) {
		(void) sprintf(fbuf, FILEDIRS, DEFDIR);
		filename = fbuf;
	}
	if (sflag) {
		if (argc != 1)
			usage();
		firstC = NULL;
		c = &firstC;
		(void) sprintf(buf, FILEHOSTS, DEFDIR);
		if ((f = fopen(buf, "r")) == NULL)
			quit(1, "supscan: Unable to open %s\n", buf);
		while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
			q = strchr(p, '\n');
			if (q)
				*q = 0;
			if (strchr("#;:", *p))
				continue;
			collname = nxtarg(&p, " \t=");
			p = skipover(p, " \t=");
			if (!localhost(p))
				continue;
			*c = getscancoll(filename, estrdup(collname), NULL);
			if (*c)
				c = &((*c)->Cnext);
			else
				errs++;
		}
		(void) fclose(f);
		return errs;
	}
	if (argc < 2 && fflag) {
		firstC = NULL;
		c = &firstC;
		if ((f = fopen(filename, "r")) == NULL)
			quit(1, "supscan: Unable to open %s\n", filename);
		while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
			q = strchr(p, '\n');
			if (q)
				*q = 0;
			if (strchr("#;:", *p))
				continue;
			q = nxtarg(&p, " \t=");
			p = skipover(p, " \t=");
			*c = getscancoll(filename, estrdup(q), estrdup(p));
			if (*c)
				c = &((*c)->Cnext);
			else
				errs++;
		}
		(void) fclose(f);
		return errs;
	}
	if (argc < 2 || argc > 3)
		usage();
	firstC = getscancoll(filename, estrdup(argv[1]),
	    argc > 2 ? estrdup(argv[2]) : NULL);
	if (firstC == NULL)
		errs++;
	return errs;
}

static SCAN_COLLECTION *
getscancoll(char *filename, char *collname, char *basedir)
{
	char buf[STRINGLENGTH], *p, *q;
	FILE *f;
	SCAN_COLLECTION *c;

	if (basedir == NULL) {
		if ((f = fopen(filename, "r")) != NULL) {
			while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
				q = strchr(p, '\n');
				if (q)
					*q = 0;
				if (strchr("#;:", *p))
					continue;
				q = nxtarg(&p, " \t=");
				if (strcmp(q, collname) == 0) {
					p = skipover(p, " \t=");
					basedir = estrdup(p);
					break;
				}
			}
			(void) fclose(f);
		}
		if (basedir == NULL) {
			(void) sprintf(buf, FILEBASEDEFAULT, collname);
			basedir = estrdup(buf);
		}
	}
	if (chdir(basedir) < 0) {
		fprintf(stderr, "supscan: Can't chdir to base directory %s "
		    "for %s (%s)\n", basedir, collname, strerror(errno));
		return (NULL);
	}
	prefix = NULL;
	(void) sprintf(buf, FILEPREFIX, collname);
	if ((f = fopen(buf, "r")) != NULL) {
		while ((p = fgets(buf, STRINGLENGTH, f)) != NULL) {
			q = strchr(p, '\n');
			if (q)
				*q = 0;
			if (strchr("#;:", *p))
				continue;
			prefix = estrdup(p);
			if (chdir(prefix) < 0) {
				fprintf(stderr, "supscan: can't chdir to %s "
				    " from base directory %s for %s (%s)\n",
				    prefix, basedir, collname, strerror(errno));
				fclose(f);
				free(prefix);
				return (NULL);
			}
			break;
		}
		(void) fclose(f);
	}
	if ((c = malloc(sizeof(*c))) == NULL)
		quit(1, "supscan: can't malloc collection structure\n");
	c->Cname = collname;
	c->Cbase = basedir;
	c->Cprefix = prefix;
	c->Cnext = NULL;
	return (c);
}

void
goaway(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void) putc('\n', stderr);
	(void) fflush(stderr);
	longjmp(sjbuf, TRUE);
}

int 
localhost(char *host)
{
	static char myhost[MAXHOSTNAMELEN + 1];
	static unsigned int myhostlen;
	unsigned int hostlen;

	if (*myhost == '\0') {
		/*
		 * We assume that the host name in the kernel is the
		 * fully qualified form.
		 */
		if (gethostname(myhost, sizeof(myhost)) < 0) {
			quit(1, "supscan: can't get kernel host name\n");
		}
		myhost[sizeof(myhost) - 1] = '\0';
		myhostlen = strlen(myhost);
	}
	/*
	 * Here, we assume that the 'host' parameter from the
	 * coll.host file is at least a prefix of the fully qualified
	 * host name of some machine.  This will be true when modcoll(8)
	 * (and related scripts) maintain the relevant files, but if
	 * a person makes a manual change, problems could result.  In
	 * particular, if a nicname, such as "Y" for "GANDALF.CS.CMU.EDU"
	 * is present in the coll.host file, things will not work as
	 * expected.
	 */

	hostlen = strlen(host);

	return (strncasecmp(myhost, host,
		hostlen < myhostlen ? hostlen : myhostlen) == 0);
}
