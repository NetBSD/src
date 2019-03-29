/*	$NetBSD: itime.c,v 1.20.20.1 2019/03/29 19:43:28 martin Exp $	*/

/*-
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
static char sccsid[] = "@(#)itime.c	8.1 (Berkeley) 6/5/93";
#else
__RCSID("$NetBSD: itime.c,v 1.20.20.1 2019/03/29 19:43:28 martin Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dump.h"

struct dumptime {
	struct	dumpdates dt_value;
	SLIST_ENTRY(dumptime) dt_list;
};
SLIST_HEAD(dthead, dumptime) dthead = SLIST_HEAD_INITIALIZER(dthead);
struct	dumpdates **ddatev = 0;
int	nddates = 0;

static	void dumprecout(FILE *, struct dumpdates *);
static	int getrecord(FILE *, struct dumpdates *);
static	int makedumpdate(struct dumpdates *, const char *);
static	void readdumptimes(FILE *);

void
initdumptimes(void)
{
	FILE *df;

	if ((df = fopen(dumpdates, "r")) == NULL) {
		if (errno != ENOENT) {
			msg("WARNING: cannot read %s: %s\n", dumpdates,
			    strerror(errno));
			return;
		}
		/*
		 * Dumpdates does not exist, make an empty one.
		 */
		msg("WARNING: no file `%s', making an empty one\n", dumpdates);
		if ((df = fopen(dumpdates, "w")) == NULL) {
			msg("WARNING: cannot create %s: %s\n", dumpdates,
			    strerror(errno));
			return;
		}
		(void) fclose(df);
		if ((df = fopen(dumpdates, "r")) == NULL) {
			quite(errno, "cannot read %s even after creating it",
			    dumpdates);
			/* NOTREACHED */
		}
	}
	(void) flock(fileno(df), LOCK_SH);
	readdumptimes(df);
	(void) fclose(df);
}

static void
readdumptimes(FILE *df)
{
	int i;
	struct	dumptime *dtwalk;

	for (;;) {
		dtwalk = (struct dumptime *)xcalloc(1, sizeof(struct dumptime));
		if (getrecord(df, &(dtwalk->dt_value)) < 0) {
			free(dtwalk);
			break;
		}
		nddates++;
		SLIST_INSERT_HEAD(&dthead, dtwalk, dt_list);
	}

	/*
	 *	arrayify the list, leaving enough room for the additional
	 *	record that we may have to add to the ddate structure
	 */
	ddatev = (struct dumpdates **)
		xcalloc((unsigned) (nddates + 1), sizeof(struct dumpdates *));
	dtwalk = SLIST_FIRST(&dthead);
	for (i = nddates - 1; i >= 0; i--, dtwalk = SLIST_NEXT(dtwalk, dt_list))
		ddatev[i] = &dtwalk->dt_value;
}

void
getdumptime(void)
{
	struct dumpdates *ddp;
	int i;
	const char *fname;

	fname = dumpdev ? dumpdev : disk;
#ifdef FDEBUG
	msg("Looking for name %s in dumpdates = %s for level = %c\n",
		fname, dumpdates, level);
#endif
	spcl.c_ddate = 0;
	lastlevel = '0';

	initdumptimes();
	/*
	 *	Go find the entry with the same name for a lower increment
	 *	and older date.  If we are doing a true incremental, then
	 *	we can use any level as a ref point.
	 */
	ITITERATE(i, ddp) {
		if (strncmp(fname, ddp->dd_name, sizeof (ddp->dd_name)) != 0)
			continue;
		/* trueinc: ostensibly could omit the second clause
		 * since if trueinc is set, we don't care about the level
		 * at all.
		 */
		/* if ((!trueinc && (ddp->dd_level >= level)) ||
		    (trueinc && (ddp->dd_level > level))) */
		if (!trueinc && (ddp->dd_level >= level))
			continue;
		if (ddp->dd_ddate <= iswap32(spcl.c_ddate))
			continue;
		spcl.c_ddate = iswap32(ddp->dd_ddate);
		lastlevel = ddp->dd_level;
	}
}

void
putdumptime(void)
{
	FILE *df;
	struct dumpdates *dtwalk, *dtfound;
	int i;
	int fd;
	const char *fname;

	if (uflag == 0 && dumpdev == NULL)
		return;
	if ((df = fopen(dumpdates, "r+")) == NULL)
		quite(errno, "cannot rewrite %s", dumpdates);
	fd = fileno(df);
	(void) flock(fd, LOCK_EX);
	fname = dumpdev ? dumpdev : disk;
	free((char *)ddatev);
	ddatev = 0;
	nddates = 0;
	readdumptimes(df);
	if (fseek(df, 0L, 0) < 0)
		quite(errno, "can't fseek %s", dumpdates);
	spcl.c_ddate = 0;
	ITITERATE(i, dtwalk) {
		if (strncmp(fname, dtwalk->dd_name,
				sizeof (dtwalk->dd_name)) != 0)
			continue;
		if (dtwalk->dd_level != level)
			continue;
		goto found;
	}
	/*
	 *	construct the new upper bound;
	 *	Enough room has been allocated.
	 */
	dtwalk = ddatev[nddates] =
		(struct dumpdates *)xcalloc(1, sizeof (struct dumpdates));
	nddates += 1;
  found:
	(void) strlcpy(dtwalk->dd_name, fname, sizeof(dtwalk->dd_name));
	dtwalk->dd_level = level;
	dtwalk->dd_ddate = iswap32(spcl.c_date);
	dtfound = dtwalk;

	ITITERATE(i, dtwalk) {
		dumprecout(df, dtwalk);
	}
	if (fflush(df))
		quite(errno, "can't flush %s", dumpdates);
	if (ftruncate(fd, ftell(df)))
		quite(errno, "can't ftruncate %s", dumpdates);
	(void) fclose(df);
	msg("level %c dump on %s", level,
		spcl.c_date == 0 ? "the epoch\n" : ctime(&dtfound->dd_ddate));
}

static void
dumprecout(FILE *file, struct dumpdates *what)
{

	if (fprintf(file, DUMPOUTFMT,
		    what->dd_name,
		    what->dd_level,
		    ctime(&what->dd_ddate)) < 0)
		quite(errno, "can't write %s", dumpdates);
}

int	recno;

static int
getrecord(FILE *df, struct dumpdates *ddatep)
{
	char tbuf[BUFSIZ];

	recno = 0;
	if ( (fgets(tbuf, sizeof (tbuf), df)) != tbuf)
		return(-1);
	recno++;
	if (makedumpdate(ddatep, tbuf) < 0)
		msg("Unknown intermediate format in %s, line %d\n",
			dumpdates, recno);

#ifdef FDEBUG
	msg("getrecord: %s %c %s", ddatep->dd_name, ddatep->dd_level,
	    ddatep->dd_ddate == 0 ? "the epoch\n" : ctime(&ddatep->dd_ddate));
#endif
	return(0);
}

static int
makedumpdate(struct dumpdates *ddp, const char *tbuf)
{
	char un_buf[128];

	(void) sscanf(tbuf, DUMPINFMT, ddp->dd_name, &ddp->dd_level, un_buf);
	ddp->dd_ddate = unctime(un_buf);
	if (ddp->dd_ddate < 0)
		return(-1);
	return(0);
}
