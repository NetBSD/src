/*	$NetBSD: names.c,v 1.33 2017/11/09 20:27:50 christos Exp $	*/

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
static char sccsid[] = "@(#)names.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: names.c,v 1.33 2017/11/09 20:27:50 christos Exp $");
#endif
#endif /* not lint */

/*
 * Mail -- a mail program
 *
 * Handle name lists.
 */

#include "rcv.h"
#include "extern.h"

/*
 * Allocate a single element of a name list,
 * initialize its name field to the passed
 * name and return it.
 */
PUBLIC struct name *
nalloc(char str[], int ntype)
{
	struct name *np;

	np = salloc(sizeof(*np));
	np->n_flink = NULL;
	np->n_blink = NULL;
	np->n_type = ntype;
	np->n_name = savestr(str);
	return np;
}

/*
 * Find the tail of a list and return it.
 */
static struct name *
tailof(struct name *name)
{
	struct name *np;

	np = name;
	if (np == NULL)
		return NULL;
	while (np->n_flink != NULL)
		np = np->n_flink;
	return np;
}

/*
 * Grab a single word (liberal word)
 * Throw away things between ()'s, and take anything between <>.
 */
static char *
yankword(char *ap, char wbuf[])
{
	char *cp, *cp2;

	cp = ap;
	for (;;) {
		if (*cp == '\0')
			return NULL;
		if (*cp == '(') {
			int nesting = 0;

			while (*cp != '\0') {
				switch (*cp++) {
				case '(':
					nesting++;
					break;
				case ')':
					--nesting;
					break;
				}
				if (nesting <= 0)
					break;
			}
		} else if (is_WSP(*cp) || *cp == ',')
			cp++;
		else
			break;
	}
	if (*cp ==  '<')
		for (cp2 = wbuf; *cp && (*cp2++ = *cp++) != '>'; /*EMPTY*/)
			continue;
	else
		for (cp2 = wbuf; *cp && !strchr(" \t,(", *cp); *cp2++ = *cp++)
			continue;
	*cp2 = '\0';
	return cp;
}

/*
 * Extract a list of names from a line,
 * and make a list of names from it.
 * Return the list or NULL if none found.
 */
PUBLIC struct name *
extract(char line[], int ntype)
{
	char *cp;
	struct name *begin, *np, *t;
	char nbuf[LINESIZE];

	if (line == NULL || *line == '\0')
		return NULL;
	begin = NULL;
	np = NULL;
	cp = line;
	while ((cp = yankword(cp, nbuf)) != NULL) {
		t = nalloc(nbuf, ntype);
		if (begin == NULL)
			begin = t;
		else
			np->n_flink = t;
		t->n_blink = np;
		np = t;
	}
	return begin;
}

/* XXX - is this really sufficient? */
static int need_quotes(char *str)
{
	return strpbrk(str, " \t") != NULL;
}

/*
 * Turn a list of names into a string of the same names.
 */
PUBLIC char *
detract(struct name *np, int ntype)
{
	size_t s;
	char *cp, *begin;
	struct name *p;
	int comma;
	int quote;

	quote = ntype & GSMOPTS;
	comma = ntype & GCOMMA;
	if (np == NULL)
		return NULL;
	ntype &= ~GCOMMA;
	s = 0;
	if (debug && comma)
		(void)fprintf(stderr, "detract asked to insert commas\n");
	for (p = np; p != NULL; p = p->n_flink) {
		if (ntype && (p->n_type & GMASK) != ntype)
			continue;
		s += strlen(p->n_name) + 1;
		if (comma)
			s++;
		if (quote && need_quotes(p->n_name))
			s += 2;
	}
	if (s == 0)
		return NULL;
	s += 2;
	begin = salloc(s);
	cp = begin;
	for (p = np; p != NULL; p = p->n_flink) {
		int do_quotes;
		if (ntype && (p->n_type & GMASK) != ntype)
			continue;
		do_quotes = (quote && need_quotes(p->n_name));
		if (do_quotes)
			*cp++ = '"';
		cp = copy(p->n_name, cp);
		if (comma && p->n_flink != NULL)
			*cp++ = ',';
		if (do_quotes)
			*cp++ = '"';
		*cp++ = ' ';
	}
	*--cp = 0;
	if (comma && *--cp == ',')
		*cp = 0;
	return begin;
}

/*
 * Determine if the passed address is a local "send to file" address.
 * If any of the network metacharacters precedes any slashes, it can't
 * be a filename.  We cheat with .'s to allow path names like ./...
 */
static int
isfileaddr(char *name)
{
	char *cp;

	if (*name == '+')
		return 1;
	for (cp = name; *cp; cp++) {
		if (*cp == '!' || *cp == '%' || *cp == '@')
			return 0;
		if (*cp == '/')
			return 1;
	}
	return 0;
}

/*
 * For each recipient in the passed name list with a /
 * in the name, append the message to the end of the named file
 * and remove him from the recipient list.
 *
 * Recipients whose name begins with | are piped through the given
 * program and removed.
 */
PUBLIC struct name *
outof(struct name *names, FILE *fo, struct header *hp)
{
	int c, fd;
	struct name *np, *begin;
	time_t now;
	char *date;
	const char *fname;
	FILE *fout, *fin;
	int ispipe;
	char tempname[PATHSIZE];

	if (value("expandaddr") == NULL)
		return names;

	begin = names;
	np = names;
	(void)time(&now);
	date = ctime(&now);
	while (np != NULL) {
		if (!isfileaddr(np->n_name) && np->n_name[0] != '|') {
			np = np->n_flink;
			continue;
		}
		ispipe = np->n_name[0] == '|';
		if (ispipe)
			fname = np->n_name+1;
		else {
			fname = expand(np->n_name);
			if (fname == NULL) {
				warnx("Filename expansion of %s failed",
				    np->n_name);
				senderr++;
				goto cant;
			}
		}


		/*
		 * See if we have copied the complete message out yet.
		 * If not, do so.
		 */

		if (image < 0) {
			(void)snprintf(tempname, sizeof(tempname),
			    "%s/mail.ReXXXXXXXXXXXX", tmpdir);
			if ((fd = mkstemp(tempname)) == -1 ||
			    (fout = Fdopen(fd, "aef")) == NULL) {
				if (fd != -1)
					(void)close(fd);
				warn("%s", tempname);
				senderr++;
				goto cant;
			}
			image = open(tempname, O_RDWR | O_CLOEXEC);
			(void)unlink(tempname);
			if (image < 0) {
				warn("%s", tempname);
				senderr++;
				(void)Fclose(fout);
				goto cant;
			}
			(void)fprintf(fout, "From %s %s", myname, date);
#ifdef MIME_SUPPORT
			(void)puthead(hp, fout, GTO|GSUBJECT|GCC|GMISC|GMIME|GNL);
#else
			(void)puthead(hp, fout, GTO|GSUBJECT|GCC|GMISC|GNL);
#endif
			while ((c = getc(fo)) != EOF)
				(void)putc(c, fout);
			rewind(fo);
			(void)putc('\n', fout);
			(void)fflush(fout);
			if (ferror(fout)) {
				warn("%s", tempname);
				senderr++;
				(void)Fclose(fout);
				goto cant;
			}
			(void)Fclose(fout);
		}

		/*
		 * Now either copy "image" to the desired file
		 * or give it as the standard input to the desired
		 * program as appropriate.
		 */

		if (ispipe) {
			int pid;
			const char *shellcmd;
			sigset_t nset;

			/*
			 * XXX
			 * We can't really reuse the same image file,
			 * because multiple piped recipients will
			 * share the same lseek location and trample
			 * on one another.
			 */
			if ((shellcmd = value(ENAME_SHELL)) == NULL)
				shellcmd = _PATH_CSHELL;
			(void)sigemptyset(&nset);
			(void)sigaddset(&nset, SIGHUP);
			(void)sigaddset(&nset, SIGINT);
			(void)sigaddset(&nset, SIGQUIT);
			pid = start_command(shellcmd, &nset,
				image, -1, "-c", fname, NULL);
			if (pid < 0) {
				senderr++;
				goto cant;
			}
			free_child(pid);
		} else {
			int f;
			if ((fout = Fopen(fname, "aef")) == NULL) {
				warn("%s", fname);
				senderr++;
				goto cant;
			}
			if ((f = dup(image)) < 0) {
				warn("dup");
				fin = NULL;
			} else
				fin = Fdopen(f, "ref");
			if (fin == NULL) {
				(void)fprintf(stderr, "Can't reopen image\n");
				(void)Fclose(fout);
				senderr++;
				goto cant;
			}
			rewind(fin);
			while ((c = getc(fin)) != EOF)
				(void)putc(c, fout);
			if (ferror(fout)) {
				warn("%s", fname);
				senderr++;
				(void)Fclose(fout);
				(void)Fclose(fin);
				goto cant;
			}
			(void)Fclose(fout);
			(void)Fclose(fin);
		}
cant:
		/*
		 * In days of old we removed the entry from the
		 * the list; now for sake of header expansion
		 * we leave it in and mark it as deleted.
		 */
		np->n_type |= GDEL;
		np = np->n_flink;
	}
	if (image >= 0) {
		(void)close(image);
		image = -1;
	}
	return begin;
}

/*
 * Put another node onto a list of names and return
 * the list.
 */
static struct name *
put(struct name *list, struct name *node)
{
	node->n_flink = list;
	node->n_blink = NULL;
	if (list != NULL)
		list->n_blink = node;
	return node;
}

/*
 * Recursively expand a group name.  We limit the expansion to some
 * fixed level to keep things from going haywire.
 * Direct recursion is not expanded for convenience.
 */
PUBLIC struct name *
gexpand(struct name *nlist, struct grouphead *gh, int metoo, int ntype)
{
	struct group *gp;
	struct grouphead *ngh;
	struct name *np;
	static int depth;
	char *cp;

	if (depth > MAXEXP) {
		(void)printf("Expanding alias to depth larger than %d\n", MAXEXP);
		return nlist;
	}
	depth++;
	for (gp = gh->g_list; gp != NULL; gp = gp->ge_link) {
		cp = gp->ge_name;
		if (*cp == '\\')
			goto quote;
		if (strcmp(cp, gh->g_name) == 0)
			goto quote;
		if ((ngh = findgroup(cp)) != NULL) {
			nlist = gexpand(nlist, ngh, metoo, ntype);
			continue;
		}
quote:
		np = nalloc(cp, ntype);
		/*
		 * At this point should allow to expand
		 * to self if only person in group
		 */
		if (gp == gh->g_list && gp->ge_link == NULL)
			goto skip;
		if (!metoo && strcmp(cp, myname) == 0)
			np->n_type |= GDEL;
skip:
		nlist = put(nlist, np);
	}
	depth--;
	return nlist;
}

/*
 * Map all of the aliased users in the invoker's mailrc
 * file and insert them into the list.
 * Changed after all these months of service to recursively
 * expand names (2/14/80).
 */

PUBLIC struct name *
usermap(struct name *names)
{
	struct name *new, *np, *cp;
	struct grouphead *gh;
	int metoo;

	new = NULL;
	np = names;
	metoo = (value(ENAME_METOO) != NULL);
	while (np != NULL) {
		if (np->n_name[0] == '\\') {
			cp = np->n_flink;
			new = put(new, np);
			np = cp;
			continue;
		}
		gh = findgroup(np->n_name);
		cp = np->n_flink;
		if (gh != NULL)
			new = gexpand(new, gh, metoo, np->n_type);
		else
			new = put(new, np);
		np = cp;
	}
	return new;
}

/*
 * Concatenate the two passed name lists, return the result.
 */
PUBLIC struct name *
cat(struct name *n1, struct name *n2)
{
	struct name *tail;

	if (n1 == NULL)
		return n2;
	if (n2 == NULL)
		return n1;
	tail = tailof(n1);
	tail->n_flink = n2;
	n2->n_blink = tail;
	return n1;
}

/*
 * Determine the number of undeleted elements in
 * a name list and return it.
 */
PUBLIC int
count(struct name *np)
{
	int c;

	for (c = 0; np != NULL; np = np->n_flink)
		if ((np->n_type & GDEL) == 0)
			c++;
	return c;
}

/*
 * Unpack the name list onto a vector of strings.
 * Return an error if the name list won't fit.
 */
PUBLIC const char **
unpack(struct name *smopts, struct name *np)
{
	const char **ap, **begin;
	struct name *n;
	int t, extra, metoo, verbose;

	n = np;
	if ((t = count(n)) == 0)
		errx(EXIT_FAILURE, "No names to unpack");
	/*
	 * Compute the number of extra arguments we will need.
	 * We need at least two extra -- one for "mail" and one for
	 * the terminating 0 pointer.  Additional spots may be needed
	 * to pass along -f to the host mailer.
	 */
	extra = 3 + count(smopts);
	extra++;
	metoo = value(ENAME_METOO) != NULL;
	if (metoo)
		extra++;
	verbose = value(ENAME_VERBOSE) != NULL;
	if (verbose)
		extra++;
	begin = salloc((t + extra) * sizeof(*begin));
	ap = begin;
	*ap++ = "sendmail";
	*ap++ = "-i";
	if (metoo)
		*ap++ = "-m";
	if (verbose)
		*ap++ = "-v";
	for (/*EMPTY*/; smopts != NULL; smopts = smopts->n_flink)
		if ((smopts->n_type & GDEL) == 0)
			*ap++ = smopts->n_name;
	*ap++ = "--";
	for (/*EMPTY*/; n != NULL; n = n->n_flink)
		if ((n->n_type & GDEL) == 0)
			*ap++ = n->n_name;
	*ap = NULL;
	return begin;
}

/*
 * Remove all of the duplicates from the passed name list by
 * insertion sorting them, then checking for dups.
 * Return the head of the new list.
 */
PUBLIC struct name *
elide(struct name *names)
{
	struct name *np, *t, *new;
	struct name *x;

	if (names == NULL)
		return NULL;
	new = names;
	np = names;
	np = np->n_flink;
	if (np != NULL)
		np->n_blink = NULL;
	new->n_flink = NULL;
	while (np != NULL) {
		t = new;
		while (strcasecmp(t->n_name, np->n_name) < 0) {
			if (t->n_flink == NULL)
				break;
			t = t->n_flink;
		}

		/*
		 * If we ran out of t's, put the new entry after
		 * the current value of t.
		 */

		if (strcasecmp(t->n_name, np->n_name) < 0) {
			t->n_flink = np;
			np->n_blink = t;
			t = np;
			np = np->n_flink;
			t->n_flink = NULL;
			continue;
		}

		/*
		 * Otherwise, put the new entry in front of the
		 * current t.  If at the front of the list,
		 * the new guy becomes the new head of the list.
		 */

		if (t == new) {
			t = np;
			np = np->n_flink;
			t->n_flink = new;
			new->n_blink = t;
			t->n_blink = NULL;
			new = t;
			continue;
		}

		/*
		 * The normal case -- we are inserting into the
		 * middle of the list.
		 */

		x = np;
		np = np->n_flink;
		x->n_flink = t;
		x->n_blink = t->n_blink;
		t->n_blink->n_flink = x;
		t->n_blink = x;
	}

	/*
	 * Now the list headed up by new is sorted.
	 * Go through it and remove duplicates.
	 */

	np = new;
	while (np != NULL) {
		t = np;
		while (t->n_flink != NULL &&
		       strcasecmp(np->n_name, t->n_flink->n_name) == 0)
			t = t->n_flink;
		if (t == np || t == NULL) {
			np = np->n_flink;
			continue;
		}

		/*
		 * Now t points to the last entry with the same name
		 * as np.  Make np point beyond t.
		 */

		np->n_flink = t->n_flink;
		if (t->n_flink != NULL)
			t->n_flink->n_blink = np;
		np = np->n_flink;
	}
	return new;
}

/*
 * Delete the given name from a namelist.
 */
PUBLIC struct name *
delname(struct name *np, char name[])
{
	struct name *p;

	for (p = np; p != NULL; p = p->n_flink)
		if (strcasecmp(p->n_name, name) == 0) {
			if (p->n_blink == NULL) {
				if (p->n_flink != NULL)
					p->n_flink->n_blink = NULL;
				np = p->n_flink;
				continue;
			}
			if (p->n_flink == NULL) {
				if (p->n_blink != NULL)
					p->n_blink->n_flink = NULL;
				continue;
			}
			p->n_blink->n_flink = p->n_flink;
			p->n_flink->n_blink = p->n_blink;
		}
	return np;
}

/*
 * Pretty print a name list
 * Uncomment it if you need it.
 */
#if 0
PUBLIC void
prettyprint(name)
	struct name *name;
{
	struct name *np;

	np = name;
	while (np != NULL) {
		(void)fprintf(stderr, "%s(%d) ", np->n_name, np->n_type);
		np = np->n_flink;
	}
	(void)fprintf(stderr, "\n");
}
#endif
