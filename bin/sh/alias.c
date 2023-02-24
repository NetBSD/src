/*	$NetBSD: alias.c,v 1.22 2023/02/24 19:04:54 kre Exp $	*/

/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Kenneth Almquist.
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
static char sccsid[] = "@(#)alias.c	8.3 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: alias.c,v 1.22 2023/02/24 19:04:54 kre Exp $");
#endif
#endif /* not lint */

#include <stdlib.h>
#include "shell.h"
#include "input.h"
#include "output.h"
#include "error.h"
#include "memalloc.h"
#include "mystring.h"
#include "alias.h"
#include "options.h"	/* XXX for argptr (should remove?) */
#include "builtins.h"
#include "var.h"

#define ATABSIZE 39

struct alias *atab[ATABSIZE];

STATIC void setalias(char *, char *);
STATIC int by_name(const void *, const void *);
STATIC void list_aliases(void);
STATIC int unalias(char *);
STATIC struct alias **freealias(struct alias **, int);
STATIC struct alias **hashalias(const char *);
STATIC int countaliases(void);

STATIC
void
setalias(char *name, char *val)
{
	struct alias *ap, **app;

	(void) unalias(name);	/* old one (if any) is now gone */
	app = hashalias(name);

	INTOFF;
	ap = ckmalloc(sizeof (struct alias));
	ap->name = savestr(name);
	ap->flag = 0;
	ap->val = savestr(val);
	ap->next = *app;
	*app = ap;
	INTON;
}

STATIC struct alias **
freealias(struct alias **app, int force)
{
	struct alias *ap = *app;

	if (ap == NULL)
		return app;

	/*
	 * if the alias is currently in use (i.e. its
	 * buffer is being used by the input routine) we
	 * just null out the name instead of discarding it.
	 * If we encounter it later, when it is idle,
	 * we will finish freeing it then.
	 *
	 * Unless we want to simply free everything (INIT)
	 */
	if (ap->flag & ALIASINUSE && !force) {
		*ap->name = '\0';
		return &ap->next;
	}

	INTOFF;
	*app = ap->next;
	ckfree(ap->name);
	ckfree(ap->val);
	ckfree(ap);
	INTON;

	return app;
}

STATIC int
unalias(char *name)
{
	struct alias *ap, **app;

	app = hashalias(name);
	while ((ap = *app) != NULL) {
		if (equal(name, ap->name)) {
			(void) freealias(app, 0);
			return 0;
		}
		app = &ap->next;
	}

	return 1;
}

#ifdef mkinit
MKINIT void rmaliases(int);

SHELLPROC {
	rmaliases(1);
}
#endif

void
rmaliases(int force)
{
	struct alias **app;
	int i;

	INTOFF;
	for (i = 0; i < ATABSIZE; i++) {
		app = &atab[i];
		while (*app)
			app = freealias(app, force);
	}
	INTON;
}

struct alias *
lookupalias(const char *name, int check)
{
	struct alias *ap = *hashalias(name);

	while (ap != NULL) {
		if (equal(name, ap->name)) {
			if (check && (ap->flag & ALIASINUSE))
				return NULL;
			return ap;
		}
		ap = ap->next;
	}

	return NULL;
}

const char *
alias_text(void *dummy __unused, const char *name)
{
	struct alias *ap;

	ap = lookupalias(name, 0);
	if (ap == NULL)
		return NULL;
	return ap->val;
}

STATIC int
by_name(const void *a, const void *b)
{

	return strcmp(
		(*(const struct alias * const *)a)->name,
		(*(const struct alias * const *)b)->name);
}

STATIC void
list_aliases(void)
{
	int i, j, n;
	const struct alias **aliases;
	const struct alias *ap;

	INTOFF;
	n = countaliases();
	aliases = stalloc(n * (int)(sizeof aliases[0]));

	j = 0;
	for (i = 0; i < ATABSIZE; i++)
		for (ap = atab[i]; ap != NULL; ap = ap->next)
			if (ap->name[0] != '\0')
				aliases[j++] = ap;
	if (j != n)
		error("Alias count botch");
	INTON;

	qsort(aliases, n, sizeof aliases[0], by_name);

	for (i = 0; i < n; i++) {
		out1fmt("alias %s=", aliases[i]->name);
		print_quoted(aliases[i]->val);
		out1c('\n');
	}

	stunalloc(aliases);
}

/*
 * Count how many aliases are defined (skipping any
 * that have been deleted, but don't know it yet).
 * Use this opportunity to clean up any of those
 * zombies that are no longer needed.
 */
STATIC int
countaliases(void)
{
	struct alias *ap, **app;
	size_t n;
	int i;

	n = 0;
	for (i = 0; i < ATABSIZE; i++)
		for (app = &atab[i]; (ap = *app) != NULL;) {
			if (ap->name[0] != '\0')
				n++;
			else {
				app = freealias(app, 0);
				continue;
			}
			app = &ap->next;
		}

	return n;
}

int
aliascmd(int argc, char **argv)	/* ARGSUSED */
{
	char *n, *v;
	int ret = 0;
	struct alias *ap;

	(void) nextopt(NULL);	/* consume possible "--" */

	if (*argptr == NULL) {
		list_aliases();
		return 0;
	}

	while ((n = *argptr++) != NULL) {
		if ((v = strchr(n+1, '=')) == NULL) { /* n+1: funny ksh stuff */
			if ((ap = lookupalias(n, 0)) == NULL) {
				outfmt(out2, "alias: %s not found\n", n);
				ret = 1;
			} else {
				out1fmt("alias %s=", n);
				print_quoted(ap->val);
				out1c('\n');
			}
		} else {
			*v++ = '\0';
			setalias(n, v);
		}
	}

	return ret;
}

int
unaliascmd(int argc, char **argv)
{
	int i;

	while ((i = nextopt("a")) != '\0') {
		if (i == 'a') {
			rmaliases(0);
			return 0;
		}
	}

	(void)countaliases();	/* delete any dead ones */
	for (i = 0; *argptr; argptr++)
		i |= unalias(*argptr);

	return i;
}

STATIC struct alias **
hashalias(const char *p)
{
	unsigned int hashval;

	hashval = *(const unsigned char *)p << 4;
	while (*p)
		hashval += *p++;
	return &atab[hashval % ATABSIZE];
}
