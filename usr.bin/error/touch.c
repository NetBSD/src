/*	$NetBSD: touch.c,v 1.31 2023/08/26 14:50:53 rillig Exp $	*/

/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)touch.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: touch.c,v 1.31 2023/08/26 14:50:53 rillig Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>
#include <stdarg.h>
#include <err.h>
#include "error.h"
#include "pathnames.h"

/*
 * Iterate through errors
 */
#define EITERATE(p, fv, i)	for (p = fv[i]; p < fv[i+1]; p++)
#define ECITERATE(ei, p, lb, errs, nerrs) \
	for (ei = lb; p = errs[ei],ei < nerrs; ei++)

#define FILEITERATE(fi, lb, num) \
	for (fi = lb; fi <= num; fi++)

static int touchstatus = Q_YES;

/*
 * codes for probethisfile to return
 */
#define F_NOTEXIST	1
#define F_NOTREAD	2
#define F_NOTWRITE	3
#define F_TOUCHIT	4

static int countfiles(Eptr *);
static bool nopertain(Eptr **);
static void hackfile(const char *, Eptr **, int, int);
static bool preview(int, Eptr **, int);
static int settotouch(const char *);
static void diverterrors(const char *, int, Eptr **, int, bool, int);
static bool oktotouch(const char *);
static void execvarg(int, int *, char ***);
static bool edit(const char *);
static void insert(int);
static void text(Eptr, bool);
static bool writetouched(bool);
static bool mustoverwrite(FILE *, FILE *);
static bool mustwrite(const char *, size_t, FILE *);
static void errorprint(FILE *, Eptr, bool);
static int probethisfile(const char *);

static const char *
makename(const char *name, size_t level)
{
	const char *p;

	if (level == 0)
		return name;

	if (*name == '/') {
		name++;
		if (level-- == 0)
			return name;
	}

	while (level-- != 0 && (p = strchr(name, '/')) != NULL)
		name = p + 1;

	return name;
}
void
findfiles(int my_nerrors, Eptr *my_errors, int *r_nfiles, Eptr ***r_files)
{
	int my_nfiles;
	Eptr **my_files;
	const char *name;
	int ei;
	int fi;
	Eptr errorp;

	my_nfiles = countfiles(my_errors);

	my_files = Calloc(my_nfiles + 3, sizeof (Eptr*));
	touchedfiles = Calloc(my_nfiles+3, sizeof(touchedfiles[0]));
	/*
	 * Now, partition off the error messages
	 * into those that are synchronization, discarded or
	 * not specific to any file, and those that were
	 * nulled or true errors.
	 */
	my_files[0] = &my_errors[0];
	ECITERATE(ei, errorp, 0, my_errors, my_nerrors) {
		if ( ! (NOTSORTABLE(errorp->error_e_class)))
			break;
	}
	/*
	 * Now, and partition off all error messages
	 * for a given file.
	 */
	my_files[1] = &my_errors[ei];
	touchedfiles[0] = touchedfiles[1] = false;
	name = "\1";
	fi = 1;
	ECITERATE(ei, errorp, ei, my_errors, my_nerrors) {
		const char *fname = makename(errorp->error_text[0], filelevel);
		if (errorp->error_e_class == C_NULLED
		    || errorp->error_e_class == C_TRUE) {
			if (strcmp(fname, name) != 0) {
				name = fname;
				touchedfiles[fi] = false;
				my_files[fi] = &my_errors[ei];
				fi++;
			}
		}
	}
	my_files[fi] = &my_errors[my_nerrors];
	*r_nfiles = my_nfiles;
	*r_files = my_files;
}

static int
countfiles(Eptr *errors)
{
	const char *name;
	int ei;
	Eptr errorp;
	int my_nfiles;

	my_nfiles = 0;
	name = "\1";
	ECITERATE(ei, errorp, 0, errors, nerrors) {
		if (SORTABLE(errorp->error_e_class)) {
			const char *fname = makename(errorp->error_text[0],
			    filelevel);
			if (strcmp(fname, name) != 0) {
				my_nfiles++;
				name = fname;
			}
		}
	}
	return my_nfiles;
}

const char *class_table[] = {
	/*C_UNKNOWN	0	*/	"Unknown",
	/*C_IGNORE	1	*/	"ignore",
	/*C_SYNC	2	*/	"synchronization",
	/*C_DISCARD	3	*/	"discarded",
	/*C_NONSPEC	4	*/	"non specific",
	/*C_THISFILE	5	*/	"specific to this file",
	/*C_NULLED	6	*/	"nulled",
	/*C_TRUE	7	*/	"true",
	/*C_DUPL	8	*/	"duplicated"
};

int class_count[C_LAST - C_FIRST] = {0};

void
filenames(int my_nfiles, Eptr **my_files)
{
	int fi;
	const char *sep = " ";
	bool someerrors;

	/*
	 * first, simply dump out errors that
	 * don't pertain to any file
	 */
	someerrors = nopertain(my_files);

	if (my_nfiles > 0) {
		someerrors = true;
		if (terse)
			fprintf(stdout, "%d file%s", my_nfiles, plural(my_nfiles));
		else
			fprintf(stdout, "%d file%s contain%s errors",
				my_nfiles, plural(my_nfiles), verbform(my_nfiles));
		if (!terse) {
			FILEITERATE(fi, 1, my_nfiles) {
				const char *fname = makename(
				    (*my_files[fi])->error_text[0], filelevel);
				fprintf(stdout, "%s\"%s\" (%d)",
					sep, fname,
					(int)(my_files[fi+1] - my_files[fi]));
				sep = ", ";
			}
		}
		fprintf(stdout, "\n");
	}
	if (!someerrors)
		fprintf(stdout, "No errors.\n");
}

/*
 * Dump out errors that don't pertain to any file
 */
static bool
nopertain(Eptr **my_files)
{
	int type;
	bool someerrors = false;
	Eptr *erpp;
	Eptr errorp;

	if (my_files[1] - my_files[0] <= 0)
		return false;
	for (type = C_UNKNOWN; NOTSORTABLE(type); type++) {
		if (class_count[type] <= 0)
			continue;
		if (type > C_SYNC)
			someerrors = true;
		if (terse) {
			fprintf(stdout, "\t%d %s errors NOT PRINTED\n",
				class_count[type], class_table[type]);
		} else {
			fprintf(stdout, "\n\t%d %s errors follow\n",
				class_count[type], class_table[type]);
			EITERATE(erpp, my_files, 0) {
				errorp = *erpp;
				if (errorp->error_e_class == type) {
					errorprint(stdout, errorp, true);
				}
			}
		}
	}
	return someerrors;
}


bool
touchfiles(int my_nfiles, Eptr **my_files, int *r_edargc, char ***r_edargv)
{
	const char *name;
	Eptr errorp;
	int fi;
	Eptr *erpp;
	int ntrueerrors;
	bool scribbled;
	int n_pissed_on;	/* # of file touched*/
	int spread;

	FILEITERATE(fi, 1, my_nfiles) {
		name = makename((*my_files[fi])->error_text[0], filelevel);
		spread = (int)(my_files[fi+1] - my_files[fi]);

		fprintf(stdout, terse
			? "\"%s\" has %d error%s, "
			: "\nFile \"%s\" has %d error%s.\n"
			, name ,spread ,plural(spread));
		/*
		 * First, iterate through all error messages in this file
		 * to see how many of the error messages really will
		 * get inserted into the file.
		 */
		ntrueerrors = 0;
		EITERATE(erpp, my_files, fi) {
			errorp = *erpp;
			if (errorp->error_e_class == C_TRUE)
				ntrueerrors++;
		}
		fprintf(stdout, terse
		  ? "insert %d\n"
		  : "\t%d of these errors can be inserted into the file.\n",
			ntrueerrors);

		hackfile(name, my_files, fi, ntrueerrors);
	}
	scribbled = false;
	n_pissed_on = 0;
	FILEITERATE(fi, 1, my_nfiles) {
		scribbled |= touchedfiles[fi];
		n_pissed_on++;
	}
	if (scribbled) {
		/*
		 * Construct an execv argument
		 */
		execvarg(n_pissed_on, r_edargc, r_edargv);
		return true;
	} else {
		if (!terse)
			fprintf(stdout, "You didn't touch any files.\n");
		return false;
	}
}

static void
hackfile(const char *name, Eptr **my_files, int ix, int my_nerrors)
{
	bool previewed;
	int errordest;	/* where errors go */

	if (!oktotouch(name)) {
		previewed = false;
		errordest = TOSTDOUT;
	} else {
		previewed = preview(my_nerrors, my_files, ix);
		errordest = settotouch(name);
	}

	if (errordest != TOSTDOUT)
		touchedfiles[ix] = true;

	if (previewed && errordest == TOSTDOUT)
		return;

	diverterrors(name, errordest, my_files, ix, previewed, my_nerrors);

	if (errordest == TOTHEFILE) {
		/*
		 * overwrite the original file
		 */
		writetouched(true);
	}
}

static bool
preview(int my_nerrors, Eptr **my_files, int ix)
{
	bool back;
	Eptr *erpp;

	if (my_nerrors <= 0)
		return false;
	back = false;
	if (query) {
		int answer = inquire(terse
		    ? "Preview? "
		    : "Do you want to preview the errors first? ");
		if (answer == Q_YES || answer == Q_yes) {
			back = true;
			EITERATE(erpp, my_files, ix) {
				errorprint(stdout, *erpp, true);
			}
			if (!terse)
				fprintf(stdout, "\n");
		}
	}
	return back;
}

static int
settotouch(const char *name)
{
	int dest = TOSTDOUT;

	if (query) {
		int reply;
		if (terse)
			reply = inquire("Touch? ");
		else
			reply = inquire("Do you want to touch file \"%s\"? ",
			    name);
		switch (reply) {
		case Q_NO:
		case Q_no:
		case Q_error:
			touchstatus = Q_NO;
			return dest;
		default:
			touchstatus = Q_YES;
			break;
		}
	}

	switch (probethisfile(name)) {
	case F_NOTREAD:
		dest = TOSTDOUT;
		fprintf(stdout, terse
			? "\"%s\" unreadable\n"
			: "File \"%s\" is unreadable\n",
			name);
		break;
	case F_NOTWRITE:
		dest = TOSTDOUT;
		fprintf(stdout, terse
			? "\"%s\" unwritable\n"
			: "File \"%s\" is unwritable\n",
			name);
		break;
	case F_NOTEXIST:
		dest = TOSTDOUT;
		fprintf(stdout, terse
			? "\"%s\" not found\n"
			: "Can't find file \"%s\" to insert error messages into.\n",
			name);
		break;
	default:
		dest = edit(name) ? TOSTDOUT : TOTHEFILE;
		break;
	}
	return dest;
}

static void
diverterrors(const char *name, int dest, Eptr **my_files, int ix,
	     bool previewed, int nterrors)
{
	int my_nerrors;
	Eptr *erpp;
	Eptr errorp;

	my_nerrors = (int)(my_files[ix+1] - my_files[ix]);

	if (my_nerrors != nterrors && !previewed) {
		if (terse)
			printf("Uninserted errors\n");
		else
			printf(">>Uninserted errors for file \"%s\" follow.\n",
			    name);
	}

	EITERATE(erpp, my_files, ix) {
		errorp = *erpp;
		if (errorp->error_e_class != C_TRUE) {
			if (previewed || touchstatus == Q_NO)
				continue;
			errorprint(stdout, errorp, true);
			continue;
		}
		switch (dest) {
		case TOSTDOUT:
			if (previewed || touchstatus == Q_NO)
				continue;
			errorprint(stdout,errorp, true);
			break;
		case TOTHEFILE:
			insert(errorp->error_line);
			text(errorp, false);
			break;
		}
	}
}

static bool
oktotouch(const char *filename)
{
	const char *src;
	const char *pat;
	const char *osrc;

	pat = suffixlist;
	if (pat == 0)
		return false;
	if (*pat == '*')
		return true;
	while (*pat++ != '.')
		continue;
	--pat;		/* point to the period */

	for (src = &filename[strlen(filename)], --src;
	     src > filename && *src != '.'; --src)
		continue;
	if (*src != '.')
		return false;

	for (src++, pat++, osrc = src;
	    *src != '\0' && *pat != '\0'; src = osrc, pat++) {
		for (;   *src != '\0'		/* not at end of the source */
		      && *pat != '\0'		/* not off end of pattern */
		      && *pat != '.'		/* not off end of sub pattern */
		      && *pat != '*'		/* not wild card */
		      && *src == *pat;		/* and equal... */
		      src++, pat++)
			continue;
		if (*src == 0 && (*pat == 0 || *pat == '.' || *pat == '*'))
			return true;
		if (*src != 0 && *pat == '*')
			return true;
		while (*pat != '\0' && *pat != '.')
			pat++;
		if (*pat == '\0')
			return false;
	}
	return false;
}

/*
 * Construct an execv argument
 * We need 1 argument for the editor's name
 * We need 1 argument for the initial search string
 * We need n_pissed_on arguments for the file names
 * We need 1 argument that is a null for execv.
 * The caller fills in the editor's name.
 * We fill in the initial search string.
 * We fill in the arguments, and the null.
 */
static void
execvarg(int n_pissed_on, int *r_argc, char ***r_argv)
{
	Eptr p;
	const char *sep, *name;
	int fi;

	sep = NULL;
	(*r_argv) = Calloc(n_pissed_on + 3, sizeof(char *));
	(*r_argc) =  n_pissed_on + 2;
	(*r_argv)[1] = Strdup("+1;/###/"); /* XXX leaked */
	n_pissed_on = 2;
	if (!terse) {
		fprintf(stdout, "You touched file(s):");
		sep = " ";
	}
	FILEITERATE(fi, 1, nfiles) {
		if (!touchedfiles[fi])
			continue;
		p = *(files[fi]);
		name = makename(p->error_text[0], filelevel);
		if (!terse) {
			fprintf(stdout,"%s\"%s\"", sep, name);
			sep = ", ";
		}
		(*r_argv)[n_pissed_on++] = __UNCONST(name);
	}
	if (!terse)
		fprintf(stdout, "\n");
	(*r_argv)[n_pissed_on] = 0;
}

static FILE *o_touchedfile;	/* the old file */
static FILE *n_touchedfile;	/* the new file */
static const char *o_name;
static char n_name[MAXPATHLEN];
static int o_lineno;
static int n_lineno;
static bool tempfileopen = false;

/*
 * open the file; guaranteed to be both readable and writable
 * Well, if it isn't, then return TRUE if something failed
 */
static bool
edit(const char *name)
{
	int fd;
	const char *tmpdir;

	o_name = name;
	if ((o_touchedfile = fopen(name, "r")) == NULL) {
		warn("Can't open file `%s' to touch (read)", name);
		return true;
	}
	if ((tmpdir = getenv("TMPDIR")) == NULL)
		tmpdir = _PATH_TMP;
	(void)snprintf(n_name, sizeof (n_name), "%s/%s", tmpdir, TMPFILE);
	fd = -1;
	if ((fd = mkstemp(n_name)) == -1 ||
	    (n_touchedfile = fdopen(fd, "w")) == NULL) {
		warn("Can't open file `%s' to touch (write)", name);
		if (fd != -1)
			close(fd);
		return true;
	}
	tempfileopen = true;
	n_lineno = 0;
	o_lineno = 0;
	return false;
}

/*
 * Position to the line (before, after) the line given by place
 */
static char edbuf[BUFSIZ];

static void
insert(int place)
{
	--place;	/* always insert messages before the offending line */
	for (; o_lineno < place; o_lineno++, n_lineno++) {
		if (fgets(edbuf, BUFSIZ, o_touchedfile) == NULL)
			return;
		fputs(edbuf, n_touchedfile);
	}
}

static void
text(Eptr p, bool use_all)
{
	int offset = use_all ? 0 : 2;

	fputs(lang_table[p->error_language].lang_incomment, n_touchedfile);
	fprintf(n_touchedfile, "%d [%s] ",
		p->error_line,
		lang_table[p->error_language].lang_name);
	wordvprint(n_touchedfile, p->error_lgtext-offset, p->error_text+offset);
	fputs(lang_table[p->error_language].lang_outcomment, n_touchedfile);
	n_lineno++;
}

/*
 * write the touched file to its temporary copy,
 * then bring the temporary in over the local file
 */
static bool
writetouched(bool overwrite)
{
	size_t nread;
	FILE *localfile;
	FILE *temp;
	bool botch;
	bool oktorm;

	botch = false;
	oktorm = true;
	while ((nread = fread(edbuf, 1, sizeof(edbuf), o_touchedfile)) != 0) {
		if (nread != fwrite(edbuf, 1, nread, n_touchedfile)) {
			/*
			 * Catastrophe in temporary area: file system full?
			 */
			botch = true;
			warn("write failure: No errors inserted in `%s'",
			    o_name);
		}
	}
	fclose(n_touchedfile);
	fclose(o_touchedfile);

	/*
	 * Now, copy the temp file back over the original
	 * file, thus preserving links, etc
	 */
	if (!botch && overwrite) {
		localfile = NULL;
		temp = NULL;
		if ((localfile = fopen(o_name, "w")) == NULL) {
			warn("Can't open file `%s' to overwrite", o_name);
			botch = true;
		}
		if ((temp = fopen(n_name, "r")) == NULL) {
			warn("Can't open file `%s' to read", n_name);
			botch = true;
		}
		if (!botch)
			oktorm = mustoverwrite(localfile, temp);
		if (localfile != NULL)
			fclose(localfile);
		if (temp != NULL)
			fclose(temp);
	}
	if (!oktorm)
		errx(1, "Catastrophe: A copy of `%s': was saved in `%s'",
		    o_name, n_name);
	/*
	 * Kiss the temp file good bye
	 */
	unlink(n_name);
	tempfileopen = false;
	return true;
}

/*
 * return whether the tmpfile can be removed after writing it out
 */
static bool
mustoverwrite(FILE *preciousfile, FILE *temp)
{
	size_t nread;

	while ((nread = fread(edbuf, 1, sizeof(edbuf), temp)) != 0) {
		if (!mustwrite(edbuf, nread, preciousfile))
			return false;
	}
	return true;
}

/*
 * return false on catastrophe
 */
static bool
mustwrite(const char *base, size_t n, FILE *preciousfile)
{
	size_t nwrote;

	if (n == 0)
		return true;
	nwrote = fwrite(base, 1, n, preciousfile);
	if (nwrote == n)
		return true;
	warn("write failed");
	switch (inquire(terse
	    ? "Botch overwriting: retry? "
	    : "Botch overwriting the source file: retry? ")) {
	case Q_YES:
	case Q_yes:
		mustwrite(base + nwrote, n - nwrote, preciousfile);
		return true;
	case Q_NO:
	case Q_no:
		switch (inquire("Are you sure? ")) {
		case Q_error:
		case Q_YES:
		case Q_yes:
			return false;
		case Q_NO:
		case Q_no:
			mustwrite(base + nwrote, n - nwrote, preciousfile);
			return true;
		default:
			abort();
		}
		/* FALLTHROUGH */
	case Q_error:
	default:
		return false;
	}
}

void
onintr(int sig)
{
	switch (inquire(terse
	    ? "\nContinue? "
	    : "\nInterrupt: Do you want to continue? ")) {
	case Q_YES:
	case Q_yes:
		signal(sig, onintr);
		return;
	case Q_error:
	default:
		if (tempfileopen) {
			/*
			 * Don't overwrite the original file!
			 */
			writetouched(false);
		}
		(void)raise_default_signal(sig);
		_exit(127);
	}
	/*NOTREACHED*/
}

static void
errorprint(FILE *place, Eptr errorp, bool print_all)
{
	int offset = print_all ? 0 : 2;

	if (errorp->error_e_class == C_IGNORE)
		return;
	fprintf(place, "[%s] ", lang_table[errorp->error_language].lang_name);
	wordvprint(place,errorp->error_lgtext-offset,errorp->error_text+offset);
	putc('\n', place);
}

int
inquire(const char *fmt, ...)
{
	va_list ap;
	char buffer[128];

	if (queryfile == NULL)
		return Q_error;
	for (;;) {
		fflush(stdout);
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
		fflush(stderr);
		if (fgets(buffer, 127, queryfile) == NULL)
			return Q_error;
		switch (buffer[0]) {
		case 'Y': return Q_YES;
		case 'y': return Q_yes;
		case 'N': return Q_NO;
		case 'n': return Q_no;
		default: fprintf(stderr, "Yes or No only!\n");
		}
	}
}

static int
probethisfile(const char *name)
{
	struct stat statbuf;

	if (stat(name, &statbuf) < 0)
		return F_NOTEXIST;
	if ((statbuf.st_mode & S_IREAD) == 0)
		return F_NOTREAD;
	if ((statbuf.st_mode & S_IWRITE) == 0)
		return F_NOTWRITE;
	return F_TOUCHIT;
}
