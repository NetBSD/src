/*	$NetBSD: inp.c,v 1.9 2002/03/11 23:29:02 kristerw Exp $	*/
#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: inp.c,v 1.9 2002/03/11 23:29:02 kristerw Exp $");
#endif /* not lint */

#include "EXTERN.h"
#include "backupfile.h"
#include "common.h"
#include "util.h"
#include "pch.h"
#include "INTERN.h"
#include "inp.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

/* Input-file-with-indexable-lines abstract type. */

static long i_size;			/* Size of the input file */
static char *i_womp;			/* Plan a buffer for entire file */
static char **i_ptr;			/* Pointers to lines in i_womp */

static int tifd = -1;			/* Plan b virtual string array */
static char *tibuf[2];			/* Plan b buffers */
static LINENUM tiline[2] = {-1, -1};	/* 1st line in each buffer */
static LINENUM lines_per_buf;		/* How many lines per buffer */
static int tireclen;			/* Length of records in tmp file */

/*
 * New patch--prepare to edit another file.
 */
void
re_input(void)
{
	if (using_plan_a) {
		i_size = 0;

		if (i_ptr != NULL)
			free(i_ptr);
		if (i_womp != NULL)
			free(i_womp);
		i_womp = NULL;
		i_ptr = NULL;
	} else {
		using_plan_a = TRUE;	/* maybe the next one is smaller */
		Close(tifd);
		tifd = -1;
		free(tibuf[0]);
		free(tibuf[1]);
		tibuf[0] = tibuf[1] = NULL;
		tiline[0] = tiline[1] = -1;
		tireclen = 0;
	}
}

/*
 * Constuct the line index, somehow or other.
 */
void
scan_input(char *filename)
{
	if (!plan_a(filename))
		plan_b(filename);
	if (verbose) {
		say("Patching file %s using Plan %s...\n", filename,
		    (using_plan_a ? "A" : "B") );
	}
}

/*
 * Try keeping everything in memory.
 */
bool
plan_a(char *filename)
{
	int ifd, statfailed;
	char *s;
	LINENUM iline;
	char lbuf[MAXLINELEN];

	statfailed = stat(filename, &filestat);
	if (statfailed && ok_to_create_file) {
		if (verbose)
			say("(Creating file %s...)\n",filename);
		makedirs(filename, TRUE);
		close(creat(filename, 0666));
		statfailed = stat(filename, &filestat);
	}
	/*
	 * For nonexistent or read-only files, look for RCS or SCCS
	 * versions.
	 */
	if (statfailed ||
	    /* No one can write to it. */
	    (filestat.st_mode & 0222) == 0 ||
	    /* I can't write to it. */
	    ((filestat.st_mode & 0022) == 0 && filestat.st_uid != myuid)) {
		struct stat cstat;
		char *cs = NULL;
		char *filebase;
		int pathlen;

		filebase = basename(filename);
		pathlen = filebase - filename;

		/*
		 * Put any leading path into `s'.
		 * Leave room in lbuf for the diff command.
		 */
		s = lbuf + 20;
		strncpy(s, filename, pathlen);

#define try(f, a1, a2) (Sprintf(s + pathlen, f, a1, a2), stat(s, &cstat) == 0)
#define try1(f, a1) (Sprintf(s + pathlen, f, a1), stat(s, &cstat) == 0)
		if (try("RCS/%s%s", filebase, RCSSUFFIX) ||
		    try1("RCS/%s"  , filebase) ||
		    try("%s%s", filebase, RCSSUFFIX)) {
			Sprintf(buf, CHECKOUT, filename);
			Sprintf(lbuf, RCSDIFF, filename);
			cs = "RCS";
		} else if (try("SCCS/%s%s", SCCSPREFIX, filebase) ||
			   try("%s%s", SCCSPREFIX, filebase)) {
			Sprintf(buf, GET, s);
			Sprintf(lbuf, SCCSDIFF, s, filename);
			cs = "SCCS";
		} else if (statfailed)
			fatal("can't find %s\n", filename);
		/*
		 * else we can't write to it but it's not under a version
		 * control system, so just proceed.
		 */
		if (cs) {
			if (!statfailed) {
				if ((filestat.st_mode & 0222) != 0)
					/* The owner can write to it.  */
					fatal(
"file %s seems to be locked by somebody else under %s\n",
					      filename, cs);
				/*
				 * It might be checked out unlocked.  See if
				 * it's safe to check out the default version
				 * locked.
				 */
				if (verbose)
					say(
"Comparing file %s to default %s version...\n",
					    filename, cs);
				if (system(lbuf))
					fatal(
"can't check out file %s: differs from default %s version\n",
					      filename, cs);
			}
			if (verbose)
				say("Checking out file %s from %s...\n",
				    filename, cs);
			if (system(buf) || stat(filename, &filestat))
				fatal("can't check out file %s from %s\n",
				      filename, cs);
		}
	}
	if (old_file_is_dev_null &&
	    ok_to_create_file &&
	    (filestat.st_size != 0)) {
		fatal(
"patch creates new file but existing file %s not empty\n",
		      filename);
	}

	filemode = filestat.st_mode;
	if (!S_ISREG(filemode))
		fatal("%s is not a normal file--can't patch\n", filename);
	i_size = filestat.st_size;
	if (out_of_mem) {
		set_hunkmax();	/* make sure dynamic arrays are allocated */
		out_of_mem = FALSE;
		return FALSE;	/* force plan b because plan a bombed */
    }

	i_womp = malloc(i_size + 2);
	if (i_womp == NULL)
		return FALSE;
	if ((ifd = open(filename, 0)) < 0)
		pfatal("can't open file %s", filename);
	if (read(ifd, i_womp, i_size) != i_size) {
		/*
		 * This probably means i_size > 15 or 16 bits worth at this
		 *  point it doesn't matter if i_womp was undersized.
		 */
		Close(ifd);
		free(i_womp);
		return FALSE;
	}
	Close(ifd);
	if (i_size && i_womp[i_size - 1] != '\n')
		i_womp[i_size++] = '\n';
	i_womp[i_size] = '\0';

	/*
	 * Count the lines in the buffer so we know how many pointers we
	 * need.
	 */
	iline = 0;
	for (s = i_womp; *s; s++) {
		if (*s == '\n')
			iline++;
	}
	i_ptr = malloc((iline + 2) * sizeof(char *));
	if (i_ptr == NULL) {	/* shucks, it was a near thing */
		free(i_womp);
		return FALSE;
	}
    
	/* Now scan the buffer and build pointer array. */
	iline = 1;
	i_ptr[iline] = i_womp;
	for (s = i_womp; *s; s++) {
		if (*s == '\n') {
			/* These are NOT null terminated. */
			i_ptr[++iline] = s + 1;
		}
	}
	input_lines = iline - 1;

	/* Now check for revision, if any. */
	if (revision != NULL) { 
		if (!rev_in_string(i_womp)) {
			if (force) {
				if (verbose)
					say(
"Warning: this file doesn't appear to be the %s version--patching anyway.\n",
			revision);
			} else if (batch) {
				fatal(
"this file doesn't appear to be the %s version--aborting.\n", revision);
			} else {
				ask(
"This file doesn't appear to be the %s version--patch anyway? [n] ",
		    revision);
				if (*buf != 'y')
					fatal("aborted\n");
			}
		} else if (verbose)
			say("Good.  This file appears to be the %s version.\n",
			    revision);
	}

    return TRUE;		/* Plan a will work. */
}

/*
 * Keep (virtually) nothing in memory.
 */
void
plan_b(char *filename)
{
	FILE *ifp;
	int i = 0;
	int maxlen = 1;
	bool found_revision = (revision == NULL);

	using_plan_a = FALSE;
	if ((ifp = fopen(filename, "r")) == NULL)
		pfatal("can't open file %s", filename);
	if ((tifd = creat(TMPINNAME, 0666)) < 0)
		pfatal("can't open file %s", TMPINNAME);
	while (fgets(buf, sizeof buf, ifp) != NULL) {
		if (revision != NULL && !found_revision && rev_in_string(buf))
			found_revision = TRUE;
		if ((i = strlen(buf)) > maxlen)
			maxlen = i;		/* Find longest line. */
	}
	if (revision != NULL) {
		if (!found_revision) {
			if (force) {
				if (verbose)
					say(
"Warning: this file doesn't appear to be the %s version--patching anyway.\n",
					    revision);
			} else if (batch) {
				fatal(
"this file doesn't appear to be the %s version--aborting.\n", revision);
			} else {
				ask(
"This file doesn't appear to be the %s version--patch anyway? [n] ",
				    revision);
				if (*buf != 'y')
					fatal("aborted\n");
			}
		} else if (verbose)
			say("Good.  This file appears to be the %s version.\n",
			    revision);
	}
	Fseek(ifp, 0L, 0);		/* Rewind file. */
	lines_per_buf = BUFFERSIZE / maxlen;
	tireclen = maxlen;
	tibuf[0] = malloc(BUFFERSIZE + 1);
	tibuf[1] = malloc(BUFFERSIZE + 1);
	if (tibuf[1] == NULL)
		fatal("out of memory\n");
	for (i = 1; ; i++) {
		if (! (i % lines_per_buf))	/* New block. */
			if (write(tifd, tibuf[0], BUFFERSIZE) < BUFFERSIZE)
				pfatal("can't write temp file");
		if (fgets(tibuf[0] + maxlen * (i % lines_per_buf),
			  maxlen + 1, ifp) == NULL) {
			input_lines = i - 1;
			if (i % lines_per_buf)
				if (write(tifd, tibuf[0], BUFFERSIZE)
				    < BUFFERSIZE)
					pfatal("can't write temp file");
			break;
		}
	}
	Fclose(ifp);
	Close(tifd);
	if ((tifd = open(TMPINNAME, 0)) < 0) {
		pfatal("can't reopen file %s", TMPINNAME);
	}
}

/*
 * Fetch a line from the input file, \n terminated, not necessarily \0.
 */
char *
ifetch(LINENUM line, int whichbuf)
{
	if (line < 1 || line > input_lines)
		return "";
	if (using_plan_a)
		return i_ptr[line];
	else {
		LINENUM offline = line % lines_per_buf;
		LINENUM baseline = line - offline;

		if (tiline[0] == baseline)
			whichbuf = 0;
		else if (tiline[1] == baseline)
			whichbuf = 1;
		else {
			tiline[whichbuf] = baseline;
			Lseek(tifd, baseline / lines_per_buf * BUFFERSIZE, 0);
			if (read(tifd, tibuf[whichbuf], BUFFERSIZE) < 0)
				pfatal("error reading tmp file %s", TMPINNAME);
		}
		return tibuf[whichbuf] + (tireclen * offline);
	}
}

/*
 * True if the string argument contains the revision number we want.
 */
bool
rev_in_string(char *string)
{
	char *s;
	int patlen;

	if (revision == NULL)
		return TRUE;
	patlen = strlen(revision);
	if (strnEQ(string,revision,patlen) &&
	    isspace((unsigned char)string[patlen]))
		return TRUE;
	for (s = string; *s; s++) {
		if (isspace((unsigned char)*s) &&
		    strnEQ(s + 1, revision, patlen) && 
		    isspace((unsigned char)s[patlen + 1] )) {
			return TRUE;
		}
	}
    return FALSE;
}
