/*	$NetBSD: inp.c,v 1.15 2003/07/12 13:47:43 itojun Exp $	*/

/*
 * Copyright (c) 1988, Larry Wall
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following condition
 * is met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this condition and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
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
__RCSID("$NetBSD: inp.c,v 1.15 2003/07/12 13:47:43 itojun Exp $");
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

static void plan_a(char *);
static bool rev_in_string(char *);

/* Input-file-with-indexable-lines abstract type. */

static size_t i_size;			/* Size of the input file */
static char *i_womp;			/* Plan a buffer for entire file */
static char **i_ptr;			/* Pointers to lines in i_womp */

/*
 * New patch -- prepare to edit another file.
 */
void
re_input(void)
{
	i_size = 0;

	if (i_ptr != NULL)
		free(i_ptr);
	if (i_womp != NULL)
		free(i_womp);
	i_womp = NULL;
	i_ptr = NULL;
}

/*
 * Construct the line index, somehow or other.
 */
void
scan_input(char *filename)
{
	plan_a(filename);
	if (verbose)
		say("Patching file %s using Plan A...\n", filename);
}

/*
 * Try keeping everything in memory.
 */
static void
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
		const char *cs = NULL;
		char *filebase;
		size_t pathlen;

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

	i_womp = xmalloc(i_size + 2);
	if ((ifd = open(filename, 0)) < 0)
		pfatal("can't open file %s", filename);
	if (read(ifd, i_womp, i_size) != i_size)
		pfatal("read error");
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
	i_ptr = xmalloc((iline + 2) * sizeof(char *));
    
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
}

/*
 * Fetch a line from the input file, \n terminated, not necessarily \0.
 */
const char *
ifetch(LINENUM line)
{
	if (line < 1 || line > input_lines)
		return "";

	return i_ptr[line];
}

/*
 * True if the string argument contains the revision number we want.
 */
static bool
rev_in_string(char *string)
{
	char *s;
	size_t patlen;

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
