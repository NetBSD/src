/*	$NetBSD: config.c,v 1.13 2000/05/28 19:30:19 dante Exp $	*/

/*
 * Copyright (c) 1989, 1993, 1995
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
static char sccsid[] = "@(#)config.c	8.8 (Berkeley) 1/31/95";
#else
__RCSID("$NetBSD: config.c,v 1.13 2000/05/28 19:30:19 dante Exp $");
#endif
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "pathnames.h"

struct _head head;

/*
 * config --
 *
 * Read the configuration file and build a doubly linked
 * list that looks like:
 *
 *	tag1 <-> record <-> record <-> record
 *	|
 *	tag2 <-> record <-> record <-> record
 */
void
config(fname)
	const char *fname;
{
	TAG *tp;
	FILE *cfp;
	size_t len;
	int lcnt;
	char *p, *t, type;

	if (fname == NULL)
		fname = _PATH_MANCONF;
	if ((cfp = fopen(fname, "r")) == NULL)
		err(1, "%s", fname);
	TAILQ_INIT(&head);
	for (lcnt = 1; (p = fgetln(cfp, &len)) != NULL; ++lcnt) {
		if (len == 1)			/* Skip empty lines. */
			continue;
		if (p[len - 1] != '\n') {	/* Skip corrupted lines. */
			warnx("%s: line %d corrupted", fname, lcnt);
			continue;
		}
		p[len - 1] = '\0';		/* Terminate the line. */

						/* Skip leading space. */
		for (; *p != '\0' && isspace((unsigned char)*p); ++p);
						/* Skip empty/comment lines. */
		if (*p == '\0' || *p == '#')
			continue;
						/* Find first token. */
		for (t = p; *t && !isspace((unsigned char)*t); ++t);
		if (*t == '\0')			/* Need more than one token.*/
			continue;
		*t = '\0';

		tp = getlist(p);
		if (tp == NULL)		/* Create a new tag. */
			tp = addlist(p);

		/*
		 * Attach new records. Check to see if it is a
		 * section record or not.
		 */

		if (*p == '_') {		/* not a section record */
			/*
			 * Special cases: _build and _crunch take the 
			 * rest of the line as a single entry.
			 */
			if (!strcmp(p, "_build") || !strcmp(p, "_crunch")) {
				/*
				 * The reason we're not just using
				 * strtok(3) for all of the parsing is
				 * so we don't get caught if a line
				 * has only a single token on it.
				 */
				while (*++t && isspace((unsigned char)*t));
				addentry(tp, t, 0);
			} else {
				for(++t; (p = strtok(t, " \t\n")) != NULL;
					t = NULL)
					addentry(tp, p, 0);
			}
				
		} else {			/* section record */

			/*
			 * section entries can either be all absolute
			 * paths or all relative paths, but not both.
			 */
			type = (TAILQ_FIRST(&tp->list) != NULL) ?
			  *(TAILQ_FIRST(&tp->list)->s) : 0;

			for (++t; (p = strtok(t, " \t\n")) != NULL; t = NULL) {

				/* ensure an assigned type */
				if (type == 0)
					type = *p;
				
				/* check for illegal mix */
				if (*p != type) {
	warnx("section %s: %s: invalid entry, does not match previous types",
	      tp->s, p);
	warnx("man.conf cannot mix absolute and relative paths in an entry");
					continue;
				}
				addentry(tp, p, 0);
			}
		}
	}

	fclose(cfp);
}

/*
 * addlist --
 *	Add a tag to the list.   caller should check for duplicate
 *	before calling (we don't).
 */
TAG *
addlist(name)
	const char *name;
{
	TAG *tp;

	if ((tp = malloc(sizeof(TAG))) == NULL ||
	    (tp->s = strdup(name)) == NULL)
		err(1, "malloc");
	TAILQ_INIT(&tp->list);
	TAILQ_INSERT_TAIL(&head, tp, q);
	return (tp);
}

/*
 * getlist --
 *	Return the linked list of entries for a tag if it exists.
 */
TAG *
getlist(name)
	const char *name;
{
	TAG *tp;

	for (tp = head.tqh_first; tp != NULL; tp = tp->q.tqe_next)
		if (!strcmp(name, tp->s))
			return (tp);
	return (NULL);
}

/*
 * addentry --
 *	add an entry to a list.
 */
void
addentry(tp, newent, head)
	TAG *tp;
	const char *newent;
	int head;
{
	ENTRY *ep;

	if ((ep = malloc(sizeof(*ep))) == NULL ||
	    (ep->s = strdup(newent)) == NULL)
		err(1, "malloc");
	if (head)
		TAILQ_INSERT_HEAD(&tp->list, ep, q);
	else
		TAILQ_INSERT_TAIL(&tp->list, ep, q);
}

/*
 * removelist() and renamelist() are used by catman(8)
 */
void
removelist(name)
	const char *name;
{
	TAG *tp;
	ENTRY *ep;

	tp = getlist(name);
	while ((ep = tp->list.tqh_first) != NULL) {
		free(ep->s);
		TAILQ_REMOVE(&tp->list, ep, q);
	}
	free(tp->s);
	TAILQ_REMOVE(&head, tp, q);

}

TAG *
renamelist(oldname, newname)
	const char *oldname;
	const char *newname;
{
	TAG *tp;

	if(!(tp = getlist(oldname)))
		return (NULL);
	free(tp->s);
	if(!(tp->s = strdup(newname)))
		err(1, "malloc");
	return (tp);
}

#ifdef MANDEBUG
void
debug(l)
	const char *l;
{
	TAG *tp;
	ENTRY *ep;

	(void)printf("%s ===============\n", l);
	for (tp = head.tqh_first; tp != NULL; tp = tp->q.tqe_next) {
		printf("%s\n", tp->s);
		for (ep = tp->list.tqh_first; ep != NULL; ep = ep->q.tqe_next)
			printf("\t%s\n", ep->s);
	}
}
#endif
