/*	$NetBSD: cmd2.c,v 1.17 2003/08/07 11:14:36 agc Exp $	*/

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
static char sccsid[] = "@(#)cmd2.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: cmd2.c,v 1.17 2003/08/07 11:14:36 agc Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Mail -- a mail program
 *
 * More user commands.
 */
extern int wait_status;
static int igcomp(const void *, const void *);

/*
 * If any arguments were given, go to the next applicable argument
 * following dot, otherwise, go to the next applicable message.
 * If given as first command with no arguments, print first message.
 */
int
next(void *v)
{
	int *msgvec = v;
	struct message *mp;
	int *ip, *ip2;
	int list[2], mdot;

	if (*msgvec != 0) {

		/*
		 * If some messages were supplied, find the
		 * first applicable one following dot using
		 * wrap around.
		 */

		mdot = dot - &message[0] + 1;

		/*
		 * Find the first message in the supplied
		 * message list which follows dot.
		 */

		for (ip = msgvec; *ip != 0; ip++)
			if (*ip > mdot)
				break;
		if (*ip == 0)
			ip = msgvec;
		ip2 = ip;
		do {
			mp = &message[*ip2 - 1];
			if ((mp->m_flag & MDELETED) == 0) {
				dot = mp;
				goto hitit;
			}
			if (*ip2 != 0)
				ip2++;
			if (*ip2 == 0)
				ip2 = msgvec;
		} while (ip2 != ip);
		printf("No messages applicable\n");
		return(1);
	}

	/*
	 * If this is the first command, select message 1.
	 * Note that this must exist for us to get here at all.
	 */

	if (!sawcom)
		goto hitit;

	/*
	 * Just find the next good message after dot, no
	 * wraparound.
	 */

	for (mp = dot+1; mp < &message[msgCount]; mp++)
		if ((mp->m_flag & (MDELETED|MSAVED)) == 0)
			break;
	if (mp >= &message[msgCount]) {
		printf("At EOF\n");
		return(0);
	}
	dot = mp;
hitit:
	/*
	 * Print dot.
	 */

	list[0] = dot - &message[0] + 1;
	list[1] = 0;
	return(type(list));
}

/*
 * Save a message in a file.  Mark the message as saved
 * so we can discard when the user quits.
 */
int
save(void *v)
{
	char *str = v;

	return save1(str, 1, "save", saveignore);
}

/*
 * Save a message in a file.  Mark the message as saved
 * so we can discard when the user quits.  Save all fields
 * overriding saveignore and saveretain.
 */
int
Save(v)
	void *v;
{
	char *str = v;

	return save1(str, 1, "Save", NULL);
}

/*
 * Copy a message to a file without affected its saved-ness
 */
int
copycmd(void *v)
{
	char *str = v;

	return save1(str, 0, "copy", saveignore);
}

/*
 * Save/copy the indicated messages at the end of the passed file name.
 * If markmsg is true, mark the message "saved."
 */
int
save1(char str[], int markmsg, char *cmd, struct ignoretab *ignoretabs)
{
	int *ip;
	struct message *mp;
	char *fn, *disp;
	int f, *msgvec;
	FILE *obuf;

	msgvec = (int *) salloc((msgCount + 2) * sizeof *msgvec);
	if ((fn = snarf(str, &f)) == NULL)
		return(1);
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			printf("No messages to %s.\n", cmd);
			return(1);
		}
		msgvec[1] = 0;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return(1);
	if ((fn = expand(fn)) == NULL)
		return(1);
	printf("\"%s\" ", fn);
	fflush(stdout);
	if (access(fn, 0) >= 0)
		disp = "[Appended]";
	else
		disp = "[New file]";
	if ((obuf = Fopen(fn, "a")) == NULL) {
		warn(NULL);
		return(1);
	}
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		if (sendmessage(mp, obuf, ignoretabs, NULL) < 0) {
			warn("%s", fn);
			Fclose(obuf);
			return(1);
		}
		if (markmsg)
			mp->m_flag |= MSAVED;
	}
	fflush(obuf);
	if (ferror(obuf))
		warn("%s", fn);
	Fclose(obuf);
	printf("%s\n", disp);
	return(0);
}

/*
 * Write the indicated messages at the end of the passed
 * file name, minus header and trailing blank line.
 */
int
swrite(void *v)
{
	char *str = v;

	return save1(str, 1, "write", ignoreall);
}

/*
 * Snarf the file from the end of the command line and
 * return a pointer to it.  If there is no file attached,
 * just return NULL.  Put a null in front of the file
 * name so that the message list processing won't see it,
 * unless the file name is the only thing on the line, in
 * which case, return 0 in the reference flag variable.
 */

char *
snarf(char linebuf[], int *flag)
{
	char *cp;

	*flag = 1;
	cp = strlen(linebuf) + linebuf - 1;

	/*
	 * Strip away trailing blanks.
	 */

	while (cp > linebuf && isspace((unsigned char)*cp))
		cp--;
	*++cp = 0;

	/*
	 * Now search for the beginning of the file name.
	 */

	while (cp > linebuf && !isspace((unsigned char)*cp))
		cp--;
	if (*cp == '\0') {
		printf("No file specified.\n");
		return(NULL);
	}
	if (isspace((unsigned char)*cp))
		*cp++ = 0;
	else
		*flag = 0;
	return(cp);
}

/*
 * Delete messages.
 */
int
delete(void *v)
{
	int *msgvec = v;
	delm(msgvec);
	return 0;
}

/*
 * Delete messages, then type the new dot.
 */
int
deltype(void *v)
{
	int *msgvec = v;
	int list[2];
	int lastdot;

	lastdot = dot - &message[0] + 1;
	if (delm(msgvec) >= 0) {
		list[0] = dot - &message[0] + 1;
		if (list[0] > lastdot) {
			touch(dot);
			list[1] = 0;
			return(type(list));
		}
		printf("At EOF\n");
	} else
		printf("No more messages\n");
	return(0);
}

/*
 * Delete the indicated messages.
 * Set dot to some nice place afterwards.
 * Internal interface.
 */
int
delm(int *msgvec)
{
	struct message *mp;
	int *ip;
	int last;

	last = 0;
	for (ip = msgvec; *ip != 0; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		mp->m_flag |= MDELETED|MTOUCH;
		mp->m_flag &= ~(MPRESERVE|MSAVED|MBOX);
		last = *ip;
	}
	if (last != 0) {
		dot = &message[last-1];
		last = first(0, MDELETED);
		if (last != 0) {
			dot = &message[last-1];
			return(0);
		}
		else {
			dot = &message[0];
			return(-1);
		}
	}

	/*
	 * Following can't happen -- it keeps lint happy
	 */

	return(-1);
}

/*
 * Undelete the indicated messages.
 */
int
undeletecmd(void *v)
{
	int *msgvec = v;
	struct message *mp;
	int *ip;

	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		mp = &message[*ip - 1];
		touch(mp);
		dot = mp;
		mp->m_flag &= ~MDELETED;
	}
	return 0;
}

/*
 * Interactively dump core on "core"
 */
int
core(void *v)
{
	int pid;

	switch (pid = vfork()) {
	case -1:
		warn("fork");
		return(1);
	case 0:
		abort();
		_exit(1);
	}
	printf("Okie dokie");
	fflush(stdout);
	wait_child(pid);
	if (WCOREDUMP(wait_status))
		printf(" -- Core dumped.\n");
	else
		printf(" -- Can't dump core.\n");
	return 0;
}

/*
 * Clobber as many bytes of stack as the user requests.
 */
int
clobber(void *v)
{
	char **argv = v;
	int times;

	if (argv[0] == 0)
		times = 1;
	else
		times = (atoi(argv[0]) + 511) / 512;
	clob1(times);
	return 0;
}

/*
 * Clobber the stack.
 */
void
clob1(int n)
{
	char buf[512];
	char *cp;

	if (n <= 0)
		return;
	for (cp = buf; cp < &buf[512]; *cp++ = 0xFF)
		;
	clob1(n - 1);
}

/*
 * Add the given header fields to the retained list.
 * If no arguments, print the current list of retained fields.
 */
int
retfield(void *v)
{
	char **list = v;

	return ignore1(list, ignore + 1, "retained");
}

/*
 * Add the given header fields to the ignored list.
 * If no arguments, print the current list of ignored fields.
 */
int
igfield(void *v)
{
	char **list = v;

	return ignore1(list, ignore, "ignored");
}

int
saveretfield(void *v)
{
	char **list = v;

	return ignore1(list, saveignore + 1, "retained");
}

int
saveigfield(void *v)
{
	char **list = v;

	return ignore1(list, saveignore, "ignored");
}

int
ignore1(char *list[], struct ignoretab *tab, char *which)
{
	char field[LINESIZE];
	int h;
	struct ignore *igp;
	char **ap;

	if (*list == NULL)
		return igshow(tab, which);
	for (ap = list; *ap != 0; ap++) {
		istrcpy(field, *ap);
		if (member(field, tab))
			continue;
		h = hash(field);
		igp = (struct ignore *) calloc(1, sizeof (struct ignore));
		igp->i_field = calloc((unsigned) strlen(field) + 1,
			sizeof (char));
		strcpy(igp->i_field, field);
		igp->i_link = tab->i_head[h];
		tab->i_head[h] = igp;
		tab->i_count++;
	}
	return 0;
}

/*
 * Print out all currently retained fields.
 */
int
igshow(struct ignoretab *tab, char *which)
{
	int h;
	struct ignore *igp;
	char **ap, **ring;

	if (tab->i_count == 0) {
		printf("No fields currently being %s.\n", which);
		return 0;
	}
	ring = (char **) salloc((tab->i_count + 1) * sizeof (char *));
	ap = ring;
	for (h = 0; h < HSHSIZE; h++)
		for (igp = tab->i_head[h]; igp != 0; igp = igp->i_link)
			*ap++ = igp->i_field;
	*ap = 0;
	qsort(ring, tab->i_count, sizeof (char *), igcomp);
	for (ap = ring; *ap != 0; ap++)
		printf("%s\n", *ap);
	return 0;
}

/*
 * Compare two names for sorting ignored field list.
 */
static int
igcomp(const void *l, const void *r)
{
	return (strcmp(*(char **)l, *(char **)r));
}
