/*	$NetBSD: termcap.c,v 1.22 1999/09/20 04:48:05 lukem Exp $	*/

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
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static char sccsid[] = "@(#)termcap.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: termcap.c,v 1.22 1999/09/20 04:48:05 lukem Exp $");
#endif
#endif /* not lint */

#define	PBUFSIZ		512	/* max length of filename path */
#define	PVECSIZ		32	/* max number of names in path */

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <errno.h>
#include "pathnames.h"

/* internal definition of tinfo structure - just a pointer to the malloc'ed
 * buffer for now.
 */
struct tinfo
{
	char *info;
};

/*
 * termcap - routines for dealing with the terminal capability data base
 *
 * BUG:		Should use a "last" pointer in tbuf, so that searching
 *		for capabilities alphabetically would not be a n**2/2
 *		process when large numbers of capabilities are given.
 * Note:	If we add a last pointer now we will screw up the
 *		tc capability. We really should compile termcap.
 *
 * Essentially all the work here is scanning and decoding escapes
 * in string capabilities.  We don't use stdio because the editor
 * doesn't, and because living w/o it is not hard.
 */

static	char *tbuf;	/* termcap buffer */
static  struct tinfo *fbuf;     /* untruncated termcap buffer */

/*
 * Get an extended entry for the terminal name.  This differs from
 * tgetent only in a) the buffer is malloc'ed for the caller and
 * b) the termcap entry is not truncated to 1023 characters.
 */

int
t_getent(bp, name)
	struct tinfo **bp;
	const char *name;
{
	char  *p;
	char  *cp;
	char **fname;
	char  *home;
	int    i;
	char   pathbuf[PBUFSIZ];	/* holds raw path of filenames */
	char  *pathvec[PVECSIZ];	/* to point to names in pathbuf */
	char  *termpath;

	_DIAGASSERT(bp != NULL);
	_DIAGASSERT(name != NULL);

	if ((*bp = malloc(sizeof(struct tinfo))) == NULL) return 0;
	
	fname = pathvec;
	p = pathbuf;
	cp = getenv("TERMCAP");
	/*
	 * TERMCAP can have one of two things in it. It can be the
	 * name of a file to use instead of
	 * /usr/share/misc/termcap. In this case it better start with
	 * a "/". Or it can be an entry to use so we don't have to
	 * read the file. In this case cgetset() withh crunch out the
	 * newlines.  If TERMCAP does not hold a file name then a path
	 * of names is searched instead.  The path is found in the
	 * TERMPATH variable, or becomes _PATH_DEF ("$HOME/.termcap
	 * /usr/share/misc/termcap") if no TERMPATH exists.
	 */
	if (!cp || *cp != '/') {	/* no TERMCAP or it holds an entry */
		if ((termpath = getenv("TERMPATH")) != NULL)
			strncpy(pathbuf, termpath, PBUFSIZ);
		else {
			if ((home = getenv("HOME")) != NULL) {
				/* set up default */
				p += strlen(home);	/* path, looking in */
				(void)strncpy(pathbuf, home,
				    sizeof(pathbuf) - 1);	/* $HOME first */
				*p++ = '/';
			}	/* if no $HOME look in current directory */
			strncpy(p, _PATH_DEF, PBUFSIZ - (size_t)(p - pathbuf));
		}
	}
	else				/* user-defined name in TERMCAP */
		strncpy(pathbuf, cp, PBUFSIZ);	/* still can be tokenized */

	*fname++ = pathbuf;	/* tokenize path into vector of names */
	while (*++p)
		if (*p == ' ' || *p == ':') {
			*p = '\0';
			while (*++p)
				if (*p != ' ' && *p != ':')
					break;
			if (*p == '\0')
				break;
			*fname++ = p;
			if (fname >= pathvec + PVECSIZ) {
				fname--;
				break;
			}
		}
	*fname = (char *) 0;			/* mark end of vector */
	if (cp && *cp && *cp != '/')
		if (cgetset(cp) < 0)
			return (-2);

	/*
	 * XXX potential security hole here in a set-id program if the
	 * user had setup name to be built from a path they can not
	 * normally read.
	 */
 	(*bp)->info = NULL;
 	i = cgetent(&((*bp)->info), pathvec, name);      

	/* no tc reference loop return code in libterm XXX */
	if (i == -3)
		return (-1);
	return (i + 1);
}

/*
 * Get an entry for terminal name in buffer bp from the termcap file.
 */
int
tgetent(bp, name)
	char *bp;
	const char *name;
{
	int i, plen, elen, c;
        char *ptrbuf = NULL;
	
	i = t_getent(&fbuf, name);
	
	if (i == 1) {
		  /* stash the full buffer pointer as the ZZ capability
		     in the termcap buffer passed.
		  */
                plen = asprintf(&ptrbuf, ":ZZ=%p", fbuf->info);
		strncpy(bp, fbuf->info, 1024);
		bp[1023] = '\0';
                elen = strlen(bp);
		  /* backup over the entry if the addition of the full
		     buffer pointer will overflow the buffer passed.  We
		     want to truncate the termcap entry on a capability
		     boundary.
		  */
                if ((elen + plen) > 1023) {
			bp[1023 - plen] = '\0';
			for (c = (elen - plen); c > 0; c--) {
				if (bp[c] == ':') {
					bp[c] = '\0';
					break;
				}
			}
		}
		
		strcat(bp, ptrbuf);
                tbuf = bp;
	}

	return i;
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
int

t_getnum(info, id)
	struct tinfo *info;
	const char *id;
{
	long num;

	_DIAGASSERT(id != NULL);

	if (cgetnum(info->info, id, &num) == 0)
		return (int)(num);
	else
		return (-1);
}

int
tgetnum(id)
	const char *id;
{
	return t_getnum(fbuf, id);
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
int t_getflag(info, id)
	struct tinfo *info;
	const char *id;
{
	return (cgetcap(info->info, id, ':') != NULL);
}

int
tgetflag(id)
	const char *id;
{

	_DIAGASSERT(id != NULL);

	return t_getflag(fbuf, id);
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * limit is the number of characters allowed to be put into
 * area, this is updated.
 */
char *
t_getstr(info, id, area, limit)
	struct tinfo *info;
	const char *id;
	char **area;
	size_t *limit;
{
	char ids[3];
	char *s;
	int i;

	_DIAGASSERT(id != NULL);
	_DIAGASSERT(area != NULL);

	/*
	 * XXX
	 * This is for all the boneheaded programs that relied on tgetstr
	 * to look only at the first 2 characters of the string passed...
	 */
	*ids = *id;
	ids[1] = id[1];
	ids[2] = '\0';

	if ((i = cgetstr(info->info, ids, &s)) < 0) {
		errno = ENOENT;
		return NULL;
	}
	
	if (area != NULL) {
		  /* check if there is room for the new entry to be put into area */
		if (limit != NULL && (*limit < i)) {
			errno = E2BIG;
			return NULL;
		}
  	
		strcpy(*area, s);
		*area += i + 1;
		if (limit != NULL) *limit -= i;
  	
		return (s);
	} else {
		*limit = i;
		return NULL;
	}
}
 
/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
tgetstr(id, area)
	const char *id;
	char **area;
{
	struct tinfo dummy;

	if ((id[0] == 'Z') && (id[1] == 'Z')) {
		dummy.info = tbuf;
		return t_getstr(&dummy, id, area, NULL);
	}
	else
		return t_getstr(fbuf, id, area, NULL);
}

/*
 * Free the buffer allocated by t_getent
 *
 */
void
t_freent(info)
	struct tinfo *info;
{
	free(info->info);
	free(info);
}
