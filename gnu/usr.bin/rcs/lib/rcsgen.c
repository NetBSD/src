/*
 *                     RCS revision generation
 */

/* Copyright (C) 1982, 1988, 1989 Walter Tichy
   Copyright 1990, 1991 by Paul Eggert
   Distributed under license by the Free Software Foundation, Inc.

This file is part of RCS.

RCS is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

RCS is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RCS; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

Report problems and direct all questions to:

    rcs-bugs@cs.purdue.edu

*/

#include "rcsbase.h"

libId(genId, "$Id: rcsgen.c,v 1.2 1993/08/02 17:47:23 mycroft Exp $")

int interactiveflag;  /* Should we act as if stdin is a tty?  */
struct buf curlogbuf;  /* buffer for current log message */

enum stringwork { enter, copy, edit, expand, edit_expand };
static void scandeltatext P((struct hshentry*,enum stringwork,int));




	char const *
buildrevision(deltas, target, outfile, expandflag)
	struct hshentries const *deltas;
	struct hshentry *target;
	FILE *outfile;
	int expandflag;
/* Function: Generates the revision given by target
 * by retrieving all deltas given by parameter deltas and combining them.
 * If outfile is set, the revision is output to it,
 * otherwise written into a temporary file.
 * Temporary files are allocated by maketemp().
 * if expandflag is set, keyword expansion is performed.
 * Return nil if outfile is set, the name of the temporary file otherwise.
 *
 * Algorithm: Copy initial revision unchanged.  Then edit all revisions but
 * the last one into it, alternating input and output files (resultfile and
 * editfile). The last revision is then edited in, performing simultaneous
 * keyword substitution (this saves one extra pass).
 * All this simplifies if only one revision needs to be generated,
 * or no keyword expansion is necessary, or if output goes to stdout.
 */
{
	if (deltas->first == target) {
                /* only latest revision to generate */
		openfcopy(outfile);
		scandeltatext(target, expandflag?expand:copy, true);
		if (outfile)
			return 0;
		else {
			Ozclose(&fcopy);
                        return(resultfile);
		}
        } else {
                /* several revisions to generate */
		/* Get initial revision without keyword expansion.  */
		scandeltatext(deltas->first, enter, false);
		while ((deltas=deltas->rest)->rest) {
                        /* do all deltas except last one */
			scandeltatext(deltas->first, edit, false);
                }
		if (expandflag || outfile) {
                        /* first, get to beginning of file*/
			finishedit((struct hshentry *)nil, outfile, false);
                }
		scandeltatext(deltas->first, expandflag?edit_expand:edit, true);
		finishedit(
			expandflag ? deltas->first : (struct hshentry*)nil,
			outfile, true
		);
		if (outfile)
			return 0;
		Ozclose(&fcopy);
		return resultfile;
        }
}



	static void
scandeltatext(delta, func, needlog)
	struct hshentry * delta;
	enum stringwork func;
	int needlog;
/* Function: Scans delta text nodes up to and including the one given
 * by delta. For the one given by delta, the log message is saved into
 * delta->log if needlog is set; func specifies how to handle the text.
 * Assumes the initial lexeme must be read in first.
 * Does not advance nexttok after it is finished.
 */
{
	struct hshentry const *nextdelta;
	struct cbuf cb;

        for (;;) {
		if (eoflex())
		    fatserror("can't find delta for revision %s", delta->num);
                nextlex();
                if (!(nextdelta=getnum())) {
		    fatserror("delta number corrupted");
                }
		getkeystring(Klog);
		if (needlog && delta==nextdelta) {
			cb = savestring(&curlogbuf);
			delta->log = cleanlogmsg(curlogbuf.string, cb.size);
                } else {readstring();
                }
                nextlex();
		while (nexttok==ID && strcmp(NextString,Ktext)!=0)
			ignorephrase();
		getkeystring(Ktext);

		if (delta==nextdelta)
			break;
		readstring(); /* skip over it */

	}
	switch (func) {
		case enter: enterstring(); break;
		case copy: copystring(); break;
		case expand: xpandstring(delta); break;
		case edit: editstring((struct hshentry *)nil); break;
		case edit_expand: editstring(delta); break;
	}
}

	struct cbuf
cleanlogmsg(m, s)
	char *m;
	size_t s;
{
	register char *t = m;
	register char const *f = t;
	struct cbuf r;
	while (s) {
	    --s;
	    if ((*t++ = *f++) == '\n')
		while (m < --t)
		    if (t[-1]!=' ' && t[-1]!='\t') {
			*t++ = '\n';
			break;
		    }
	}
	while (m < t  &&  (t[-1]==' ' || t[-1]=='\t' || t[-1]=='\n'))
	    --t;
	r.string = m;
	r.size = t - m;
	return r;
}


int ttystdin()
{
	static int initialized;
	if (!initialized) {
		if (!interactiveflag)
			interactiveflag = isatty(STDIN_FILENO);
		initialized = true;
	}
	return interactiveflag;
}

	int
getcstdin()
{
	register FILE *in;
	register int c;

	in = stdin;
	if (feof(in) && ttystdin())
		clearerr(in);
	c = getc(in);
	if (c < 0) {
		testIerror(in);
		if (feof(in) && ttystdin())
			afputc('\n',stderr);
	}
	return c;
}

#if has_prototypes
	int
yesorno(int default_answer, char const *question, ...)
#else
		/*VARARGS2*/ int
	yesorno(default_answer, question, va_alist)
		int default_answer; char const *question; va_dcl
#endif
{
	va_list args;
	register int c, r;
	if (!quietflag && ttystdin()) {
		oflush();
		vararg_start(args, question);
		fvfprintf(stderr, question, args);
		va_end(args);
		eflush();
		r = c = getcstdin();
		while (c!='\n' && !feof(stdin))
			c = getcstdin();
		if (r=='y' || r=='Y')
			return true;
		if (r=='n' || r=='N')
			return false;
	}
	return default_answer;
}


	void
putdesc(textflag, textfile)
	int textflag;
	char *textfile;
/* Function: puts the descriptive text into file frewrite.
 * if finptr && !textflag, the text is copied from the old description.
 * Otherwise, if the textfile!=nil, the text is read from that
 * file, or from stdin, if textfile==nil.
 * A textfile with a leading '-' is treated as a string, not a file name.
 * If finptr, the old descriptive text is discarded.
 * Always clears foutptr.
 */
{
	static struct buf desc;
	static struct cbuf desclean;

	register FILE *txt;
	register int c;
	register FILE * frew;
	register char *p;
	register size_t s;
	char const *plim;

	frew = frewrite;
	if (finptr && !textflag) {
                /* copy old description */
		aprintf(frew, "\n\n%s%c", Kdesc, nextc);
		foutptr = frewrite;
		getdesc(false);
		foutptr = 0;
        } else {
		foutptr = 0;
                /* get new description */
		if (finptr) {
                        /*skip old description*/
			getdesc(false);
                }
		aprintf(frew,"\n\n%s\n%c",Kdesc,SDELIM);
		if (!textfile)
			desclean = getsstdin(
				"t-", "description",
				"NOTE: This is NOT the log message!\n", &desc
			);
		else if (!desclean.string) {
			if (*textfile == '-') {
				p = textfile + 1;
				s = strlen(p);
			} else {
				if (!(txt = fopen(textfile, "r")))
					efaterror(textfile);
				bufalloc(&desc, 1);
				p = desc.string;
				plim = p + desc.size;
				for (;;) {
					if ((c=getc(txt)) < 0) {
						testIerror(txt);
						if (feof(txt))
							break;
					}
					if (plim <= p)
					    p = bufenlarge(&desc, &plim);
					*p++ = c;
				}
				if (fclose(txt) != 0)
					Ierror();
				s = p - desc.string;
				p = desc.string;
			}
			desclean = cleanlogmsg(p, s);
		}
		putstring(frew, false, desclean, true);
		aputc('\n', frew);
        }
}

	struct cbuf
getsstdin(option, name, note, buf)
	char const *option, *name, *note;
	struct buf *buf;
{
	register int c;
	register char *p;
	register size_t i;
	register int tty = ttystdin();

	if (tty)
	    aprintf(stderr,
		"enter %s, terminated with single '.' or end of file:\n%s>> ",
		name, note
	    );
	else if (feof(stdin))
	    faterror("can't reread redirected stdin for %s; use -%s<%s>",
		name, option, name
	    );
	
	for (
	   i = 0,  p = 0;
	   c = getcstdin(),  !feof(stdin);
	   bufrealloc(buf, i+1),  p = buf->string,  p[i++] = c
	)
		if (c == '\n')
			if (i  &&  p[i-1]=='.'  &&  (i==1 || p[i-2]=='\n')) {
				/* Remove trailing '.'.  */
				--i;
				break;
			} else if (tty)
				aputs(">> ", stderr);
	return cleanlogmsg(p, i);
}
