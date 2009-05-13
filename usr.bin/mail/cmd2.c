/*	$NetBSD: cmd2.c,v 1.23.14.1 2009/05/13 19:19:56 jym Exp $	*/

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
__RCSID("$NetBSD: cmd2.c,v 1.23.14.1 2009/05/13 19:19:56 jym Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include <util.h>
#include "extern.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif
#include "thread.h"

/*
 * Mail -- a mail program
 *
 * More user commands.
 */

/*
 * If any arguments were given, go to the next applicable argument
 * following dot, otherwise, go to the next applicable message.
 * If given as first command with no arguments, print first message.
 */
PUBLIC int
next(void *v)
{
	int *msgvec;
	struct message *mp;
	int *ip, *ip2;
	int list[2], mdot;

	msgvec = v;
	if (*msgvec != 0) {

		/*
		 * If some messages were supplied, find the
		 * first applicable one following dot using
		 * wrap around.
		 */
		mdot = get_msgnum(dot);

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
			mp = get_message(*ip2);
			if ((mp->m_flag & MDELETED) == 0) {
				dot = mp;
				goto hitit;
			}
			if (*ip2 != 0)
				ip2++;
			if (*ip2 == 0)
				ip2 = msgvec;
		} while (ip2 != ip);
		(void)printf("No messages applicable\n");
		return 1;
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

	for (mp = next_message(dot); mp; mp = next_message(mp))
		if ((mp->m_flag & (MDELETED|MSAVED)) == 0)
			break;

	if (mp == NULL) {
		(void)printf("At EOF\n");
		return 0;
	}
	dot = mp;
hitit:
	/*
	 * Print dot.
	 */

	list[0] = get_msgnum(dot);
	list[1] = 0;
	return type(list);
}

/*
 * Snarf the file from the end of the command line and
 * return a pointer to it.  If there is no file attached,
 * just return NULL.  Put a null in front of the file
 * name so that the message list processing won't see it,
 * unless the file name is the only thing on the line, in
 * which case, return 0 in the reference flag variable.
 */
static char *
snarf(char linebuf[], int *flag, const char *string)
{
	char *cp;

	*flag = 1;
	cp = strlen(linebuf) + linebuf - 1;

	/*
	 * Strip away trailing blanks.
	 */
	while (cp >= linebuf && isspace((unsigned char)*cp))
		cp--;
	*++cp = '\0';

	/*
	 * Now search for the beginning of the file name.
	 */
	while (cp > linebuf && !isspace((unsigned char)*cp))
		cp--;
	if (*cp == '\0') {
		(void)printf("No %s specified.\n", string);
		return NULL;
	}
	if (isspace((unsigned char)*cp))
		*cp++ = '\0';
	else
		*flag = 0;
	return cp;
}

struct save1_core_args_s {
	FILE *obuf;
	struct ignoretab *igtab;
	int markmsg;
};
static int
save1_core(struct message *mp, void *v)
{
	struct save1_core_args_s *args;

	args = v;
	touch(mp);

	if (sendmessage(mp, args->obuf, args->igtab, NULL, NULL) < 0)
		return -1;

	if (args->markmsg)
		mp->m_flag |= MSAVED;

	return 0;
}

/*
 * Save/copy the indicated messages at the end of the passed file name.
 * If markmsg is true, mark the message "saved."
 */
static int
save1(char str[], int markmsg, const char *cmd, struct ignoretab *igtab)
{
	int *ip;
	const char *fn;
	const char *disp;
	int f, *msgvec;
	int msgCount;
	FILE *obuf;

	msgCount = get_msgCount();
	msgvec = salloc((msgCount + 2) * sizeof(*msgvec));
	if ((fn = snarf(str, &f, "file")) == NULL)
		return 1;
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			(void)printf("No messages to %s.\n", cmd);
			return 1;
		}
		msgvec[1] = 0;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return 1;
	if ((fn = expand(fn)) == NULL)
		return 1;
	(void)printf("\"%s\" ", fn);
	(void)fflush(stdout);
	if (access(fn, 0) >= 0)
		disp = "[Appended]";
	else
		disp = "[New file]";
	if ((obuf = Fopen(fn, "a")) == NULL) {
		warn(NULL);
		return 1;
	}
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		struct save1_core_args_s args;
		struct message *mp;

		args.obuf = obuf;
		args.igtab = igtab;
		args.markmsg = markmsg;
		mp = get_message(*ip);
		if (thread_recursion(mp, save1_core, &args)) {
			warn("%s", fn);
			(void)Fclose(obuf);
			return 1;
		}
	}
	(void)fflush(obuf);
	if (ferror(obuf))
		warn("%s", fn);
	(void)Fclose(obuf);
	(void)printf("%s\n", disp);
	return 0;
}

/*
 * Save a message in a file.  Mark the message as saved
 * so we can discard when the user quits.
 */
PUBLIC int
save(void *v)
{
	char *str;

	str = v;
	return save1(str, 1, "save", saveignore);
}

/*
 * Save a message in a file.  Mark the message as saved
 * so we can discard when the user quits.  Save all fields
 * overriding saveignore and saveretain.
 */
PUBLIC int
Save(void *v)
{
	char *str;

	str = v;
	return save1(str, 1, "Save", NULL);
}

/*
 * Copy a message to a file without affected its saved-ness
 */
PUBLIC int
copycmd(void *v)
{
	char *str;

	str = v;
	return save1(str, 0, "copy", saveignore);
}

/*
 * Write the indicated messages at the end of the passed
 * file name, minus header and trailing blank line.
 */
PUBLIC int
swrite(void *v)
{
	char *str;

	str = v;
	return save1(str, 1, "write", ignoreall);
}

/*
 * Delete the indicated messages.
 * Set dot to some nice place afterwards.
 * Internal interface.
 */
static int
delm(int *msgvec)
{
	struct message *mp;
	int *ip;
	int last;

	last = 0;
	for (ip = msgvec; *ip != 0; ip++) {
		mp = set_m_flag(*ip,
		    ~(MPRESERVE|MSAVED|MBOX|MDELETED|MTOUCH), MDELETED|MTOUCH);
		touch(mp);
		last = *ip;
	}
	if (last != 0) {
		dot = get_message(last);
		last = first(0, MDELETED);
		if (last != 0) {
			dot = get_message(last);
			return 0;
		}
		else {
			dot = get_message(1);
			return -1;
		}
	}

	/*
	 * Following can't happen -- it keeps lint happy
	 */
	return -1;
}

/*
 * Delete messages.
 */
PUBLIC int
delete(void *v)
{
	int *msgvec;

	msgvec = v;
	(void)delm(msgvec);
	return 0;
}

/*
 * Delete messages, then type the new dot.
 */
PUBLIC int
deltype(void *v)
{
	int *msgvec;
	int list[2];
	int lastdot;

	msgvec = v;
	lastdot = get_msgnum(dot);
	if (delm(msgvec) >= 0) {
		list[0] = get_msgnum(dot);
		if (list[0] > lastdot) {
			touch(dot);
			list[1] = 0;
			return type(list);
		}
		(void)printf("At EOF\n");
	} else
		(void)printf("No more messages\n");
	return 0;
}

/*
 * Undelete the indicated messages.
 */
PUBLIC int
undeletecmd(void *v)
{
	int msgCount;
	int *msgvec;
	int *ip;

	msgvec = v;
	msgCount = get_msgCount();
	for (ip = msgvec; *ip && ip-msgvec < msgCount; ip++) {
		dot = set_m_flag(*ip, ~MDELETED, 0);
		touch(dot);
		dot->m_flag &= ~MDELETED;
	}
	return 0;
}

/*************************************************************************/

/*
 * Interactively dump core on "core"
 */
/*ARGSUSED*/
PUBLIC int
core(void *v __unused)
{
	int pid;

	switch (pid = vfork()) {
	case -1:
		warn("fork");
		return 1;
	case 0:
		abort();
		_exit(1);
	}
	(void)printf("Okie dokie");
	(void)fflush(stdout);
	(void)wait_child(pid);
	if (WCOREDUMP(wait_status))
		(void)printf(" -- Core dumped.\n");
	else
		(void)printf(" -- Can't dump core.\n");
	return 0;
}

/*
 * Clobber the stack.
 */
static void
clob1(int n)
{
	char buf[512];
	char *cp;

	if (n <= 0)
		return;
	for (cp = buf; cp < &buf[512]; *cp++ = (char)0xFF)
		continue;
	clob1(n - 1);
}

/*
 * Clobber as many bytes of stack as the user requests.
 */
PUBLIC int
clobber(void *v)
{
	char **argv;
	int times;

	argv = v;
	if (argv[0] == 0)
		times = 1;
	else
		times = (atoi(argv[0]) + 511) / 512;
	clob1(times);
	return 0;
}

/*
 * Compare two names for sorting ignored field list.
 */
static int
igcomp(const void *l, const void *r)
{
	return strcmp(*(const char *const *)l, *(const char *const *)r);
}

/*
 * Print out all currently retained fields.
 */
static int
igshow(struct ignoretab *tab, const char *which)
{
	int h;
	struct ignore *igp;
	char **ap, **ring;

	if (tab->i_count == 0) {
		(void)printf("No fields currently being %s.\n", which);
		return 0;
	}
	ring = salloc((tab->i_count + 1) * sizeof(char *));
	ap = ring;
	for (h = 0; h < HSHSIZE; h++)
		for (igp = tab->i_head[h]; igp != 0; igp = igp->i_link)
			*ap++ = igp->i_field;
	*ap = 0;
	qsort(ring, tab->i_count, sizeof(char *), igcomp);
	for (ap = ring; *ap != 0; ap++)
		(void)printf("%s\n", *ap);
	return 0;
}

/*
 * core ignore routine.
 */
static int
ignore1(char *list[], struct ignoretab *tab, const char *which)
{
	char **ap;

	if (*list == NULL)
		return igshow(tab, which);

	for (ap = list; *ap != 0; ap++)
		add_ignore(*ap, tab);

	return 0;
}

/*
 * Add the given header fields to the retained list.
 * If no arguments, print the current list of retained fields.
 */
PUBLIC int
retfield(void *v)
{
	char **list;

	list = v;
	return ignore1(list, ignore + 1, "retained");
}

/*
 * Add the given header fields to the ignored list.
 * If no arguments, print the current list of ignored fields.
 */
PUBLIC int
igfield(void *v)
{
	char **list;

	list = v;
	return ignore1(list, ignore, "ignored");
}

/*
 * Add the given header fields to the save retained list.
 * If no arguments, print the current list of save retained fields.
 */
PUBLIC int
saveretfield(void *v)
{
	char **list;

	list = v;
	return ignore1(list, saveignore + 1, "retained");
}

/*
 * Add the given header fields to the save ignored list.
 * If no arguments, print the current list of save ignored fields.
 */
PUBLIC int
saveigfield(void *v)
{
	char **list;

	list = v;
	return ignore1(list, saveignore, "ignored");
}

#ifdef MIME_SUPPORT

static char*
check_dirname(char *filename)
{
	struct stat sb;
	char *fname;
	char canon_buf[MAXPATHLEN];
	char *canon_name;

	canon_name = canon_buf;
	fname = filename;
	if (fname[0] == '~' && fname[1] == '/') {
		if (homedir && homedir[0] != '~')
			(void)easprintf(&fname, "%s/%s",
			    homedir, fname + 2);
	}
	if (realpath(fname, canon_name) == NULL) {
		warn("realpath: %s", filename);
		canon_name = NULL;
		goto done;
	}
	if (stat(canon_name, &sb) == -1) {
		warn("stat: %s", canon_name);
		canon_name = NULL;
		goto done;
	}
	if (!S_ISDIR(sb.st_mode)) {
		warnx("stat: %s is not a directory", canon_name);
		canon_name = NULL;
		goto done;
	}
	if (access(canon_name, W_OK|X_OK) == -1) {
		warnx("access: %s is not writable", canon_name);
		canon_name = NULL;
		goto done;
	}
 done:
	if (fname != filename)
		free(fname);

	return canon_name ? savestr(canon_name) : NULL;
}

struct detach1_core_args_s {
	struct message *parent;
	struct ignoretab *igtab;
	const char *dstdir;
};
static int
detach1_core(struct message *mp, void *v)
{
	struct mime_info *mip;
	struct detach1_core_args_s *args;

	args = v;
	touch(mp);
	show_msgnum(stdout, mp, args->parent);
	mip = mime_decode_open(mp);
	mime_detach_msgnum(mip, sget_msgnum(mp, args->parent));
	(void)mime_sendmessage(mp, NULL, args->igtab, args->dstdir, mip);
	mime_decode_close(mip);
	return 0;
}

/*
 * detach attachments.
 */
static int
detach1(void *v, int do_unnamed)
{
	int recursive;
	int f;
	int msgCount;
	int *msgvec;
	int *ip;
	char *str;
	char *dstdir;

	str = v;

	/*
	 * Get the destination directory.
	 */
	if ((dstdir = snarf(str, &f, "directory")) == NULL &&
	    (dstdir = value(ENAME_MIME_DETACH_DIR)) == NULL &&
	    (dstdir = origdir) == NULL)
		return 1;

	if ((dstdir = check_dirname(dstdir)) == NULL)
		return 1;

	/*
	 * Setup the message list.
	 */
	msgCount = get_msgCount();
	msgvec = salloc((msgCount + 2) * sizeof(*msgvec));
	if (!f) {
		*msgvec = first(0, MMNORM);
		if (*msgvec == 0) {
			(void)printf("No messages to detach.\n");
			return 1;
		}
		msgvec[1] = 0;
	}
	if (f && getmsglist(str, msgvec, 0) < 0)
		return 1;

	if (mime_detach_control() != 0)
		return 1;

	/*
	 * do 'dot' if nothing else was selected.
	 */
	if (msgvec[0] == 0 && dot != NULL) {
		msgvec[0] = get_msgnum(dot);
		msgvec[1] = 0;
	}
	recursive = do_recursion();
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		struct detach1_core_args_s args;
		struct message *mp;

		mp = get_message(*ip);
		dot = mp;
		args.parent = recursive ? mp : NULL;
		args.igtab = do_unnamed ? detachall : ignoreall;
		args.dstdir = dstdir;
		(void)thread_recursion(mp, detach1_core, &args);
	}
	return 0;
}

/*
 * detach named attachments.
 */
PUBLIC int
detach(void *v)
{

	return detach1(v, 0);
}

/*
 * detach all attachments.
 */
PUBLIC int
Detach(void *v)
{

	return detach1(v, 1);
}
#endif /* MIME_SUPPORT */
