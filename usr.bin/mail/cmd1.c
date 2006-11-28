/*	$NetBSD: cmd1.c,v 1.28 2006/11/28 18:45:32 christos Exp $	*/

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
static char sccsid[] = "@(#)cmd1.c	8.2 (Berkeley) 4/20/95";
#else
__RCSID("$NetBSD: cmd1.c,v 1.28 2006/11/28 18:45:32 christos Exp $");
#endif
#endif /* not lint */

#include <assert.h>

#include "rcv.h"
#include "extern.h"
#include "format.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif
#include "thread.h"


/*
 * Mail -- a mail program
 *
 * User commands.
 */

static int screen;

/*
 * Compute screen size.
 */
static int
screensize(void)
{
	int s;
	char *cp;

	if ((cp = value(ENAME_SCREEN)) != NULL && (s = atoi(cp)) > 0)
		return s;
	return screenheight - 4;
}

/*
 * Print out the header of a specific message.
 * This is a slight improvement to the standard one.
 */
PUBLIC void
printhead(int mesg)
{
	const char *fmtstr;
	char *msgline;

	fmtstr = value(ENAME_HEADER_FORMAT);
	if (fmtstr == NULL)
		fmtstr = DEFAULT_HEADER_FORMAT;
	msgline = smsgprintf(fmtstr, get_message(mesg));
	if (screenwidth > 0)
		msgline[screenwidth] = '\0';
	(void)printf("%s\n", msgline);
}

/*
 * Print the current active headings.
 * Don't change dot if invoker didn't give an argument.
 */
PUBLIC int
headers(void *v)
{
	int *msgvec = v;
	int n, flag;
	struct message *mp;
	int size;

	size = screensize();
	n = msgvec[0];
	if (n != 0)
		screen = (n - 1)/size;
	if (screen < 0)
		screen = 0;

	if ((mp = get_message(screen * size + 1)) == NULL) {
		int msgCount;
		msgCount = get_msgCount();
		if (screen * size + 1 > msgCount)
			mp = get_message(msgCount - size + 1);
		if (mp == NULL)
			mp = get_message(1);
	}
	flag = 0;
	if (dot != get_message(n))
		dot = mp;
	for (/*EMPTY*/; mp; mp = next_message(mp)) {
		if (mp->m_flag & MDELETED)
			continue;
		if (flag++ >= size)
			break;
		printhead(get_msgnum(mp));
	}
	if (flag == 0) {
		(void)printf("No more mail.\n");
		return 1;
	}
	return 0;
}

/*
 * Scroll to the next/previous screen
 */
PUBLIC int
scroll(void *v)
{
	char *arg = v;
	int s, size;
	int cur[1];

	cur[0] = 0;
	size = screensize();
	s = screen;
	switch (*arg) {
	case 0:
	case '+':
		s++;
		if (s * size >= get_msgCount()) {
			(void)printf("On last screenful of messages\n");
			return 0;
		}
		screen = s;
		break;

	case '-':
		if (--s < 0) {
			(void)printf("On first screenful of messages\n");
			return 0;
		}
		screen = s;
		break;

	default:
		(void)printf("Unrecognized scrolling command \"%s\"\n", arg);
		return 1;
	}
	return headers(cur);
}

/*
 * Print out the headlines for each message
 * in the passed message list.
 */
PUBLIC int
from(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++)
		printhead(*ip);
	if (--ip >= msgvec)
		dot = get_message(*ip);
	return 0;
}

/*
 * Print out the value of dot.
 */
/*ARGSUSED*/
PUBLIC int
pdot(void *v)
{
	int *msgvec;

	msgvec = v;
	dot = get_message(msgvec[0]);

	(void)printf("%d\n", get_msgnum(dot));
	return 0;
}

/*
 * Print out all the possible commands.
 */
/*ARGSUSED*/
PUBLIC int
pcmdlist(void *v __unused)
{
	const struct cmd *cp;
	int cc;

	(void)printf("Commands are:\n");
	for (cc = 0, cp = cmdtab; cp->c_name != NULL; cp++) {
		cc += strlen(cp->c_name) + 2;
		if (cc > 72) {
			(void)printf("\n");
			cc = strlen(cp->c_name) + 2;
		}
		if ((cp + 1)->c_name != NULL)
			(void)printf("%s, ", cp->c_name);
		else
			(void)printf("%s\n", cp->c_name);
	}
	return 0;
}


PUBLIC char *
sget_msgnum(struct message *mp, struct message *parent)
{
	char *p;
	if (parent == NULL || parent == mp) {
		(void)sasprintf(&p, "%d", mp->m_index);
		return p;
	}
	p = sget_msgnum(mp->m_plink, parent);

	(void)sasprintf(&p, "%s.%d", p, mp->m_index);
	return p;
}

PUBLIC void
show_msgnum(FILE *obuf, struct message *mp, struct message *parent)
{
	if (value(ENAME_QUIET) == NULL)
		(void)fprintf(obuf, "Message %s:\n", sget_msgnum(mp, parent));
}

struct type1_core_args_s {
	FILE *obuf;
	struct message *parent;
	struct ignoretab *igtab;
	struct mime_info **mip;
};
static int
type1_core(struct message *mp, void *v)
{
	struct type1_core_args_s *args;

	args = v;
	touch(mp);
	show_msgnum(args->obuf, mp, args->parent);
#ifdef MIME_SUPPORT
	if (args->mip == NULL)
		(void)mime_sendmessage(mp, args->obuf, args->igtab, NULL, NULL);
	else {
		*args->mip = mime_decode_open(mp);
		(void)mime_sendmessage(mp, args->obuf, args->igtab, NULL, *args->mip);
		mime_decode_close(*args->mip);
	}
#else
	(void)sendmessage(mp, args->obuf, args->igtab, NULL, NULL);
#endif
	return 0;
}

/*
 * Respond to a broken pipe signal --
 * probably caused by quitting more.
 */
static jmp_buf	pipestop;

/*ARGSUSED*/
static void
brokpipe(int signo __unused)
{
	longjmp(pipestop, 1);
}

/*
 * Type out the messages requested.
 */
#ifndef MIME_SUPPORT
# define type1(a,b,c)		legacy_type1(a,b)
#endif
static int
type1(int *msgvec, int doign, int mime_decode)
{
	int recursive;
	int *ip;
	int msgCount;
	/*
	 * Some volatile variables so longjmp will get the current not
	 * starting values.  Note it is the variable that is volatile,
	 * not what it is pointing at!
	 */
	FILE *volatile obuf;		/* avoid longjmp clobbering? */
#ifdef MIME_SUPPORT
	sig_t volatile oldsigpipe;	/* XXX - is volatile needed? */
	struct mime_info *volatile mip; /* avoid longjmp clobbering - needed */
#endif

	if ((obuf = last_registered_file(0)) == NULL)
		obuf = stdout;

#ifdef MIME_SUPPORT
	mip = NULL;

	oldsigpipe = signal(SIGPIPE, SIG_IGN);
	
	if (setjmp(pipestop))
		goto close_pipe;

	(void)signal(SIGPIPE, brokpipe);
#endif
	msgCount = get_msgCount();

	recursive = do_recursion();
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		struct type1_core_args_s args;
		struct message *mp;

		mp = get_message(*ip);
		dot = mp;
		args.obuf = obuf;
		args.parent = recursive ? mp : NULL;
		args.igtab = doign ? ignore : 0;
#ifdef MIME_SUPPORT
		args.mip = mime_decode ? __UNVOLATILE(&mip) : NULL;
#else
		args.mip = NULL;
#endif
		(void)thread_recursion(mp, type1_core, &args);
	}
#ifdef MIME_SUPPORT
close_pipe:
	if (mip != NULL) {
		/*
		 * Ignore SIGPIPE so it can't cause a duplicate close.
		 */
		(void)signal(SIGPIPE, SIG_IGN);
		mime_decode_close(mip);
		(void)signal(SIGPIPE, oldsigpipe);
	}
#endif
	return 0;
}

#ifdef MIME_SUPPORT
static int
de_mime(void)
{
	return value(ENAME_MIME_DECODE_MSG) != NULL;
}

/*
 * Identical to type(), but with opposite mime behavior.
 */
PUBLIC int
view(void *v)
{
	int *msgvec = v;
	return type1(msgvec, 1, !de_mime());
}

/*
 * Identical to Type(), but with opposite mime behavior.
 */
PUBLIC int
View(void *v)
{
	int *msgvec = v;

	return type1(msgvec, 0, !de_mime());
}
#endif /* MIME_SUPPORT */

/*
 * Type out messages, honor ignored fields.
 */
PUBLIC int
type(void *v)
{
	int *msgvec = v;

	return type1(msgvec, 1, de_mime());
}

/*
 * Type out messages, even printing ignored fields.
 */
PUBLIC int
Type(void *v)
{
	int *msgvec = v;

	return type1(msgvec, 0, de_mime());
}

/*
 * Pipe the current message buffer to a command.
 */
PUBLIC int
pipecmd(void *v)
{
	char *cmd = v;
	FILE *volatile obuf;	/* void longjmp clobbering - we want
				   the current value not start value */
	if (dot == NULL) {
		warn("pipcmd: no current message");
		return 1;
	}

	obuf = stdout;
	if (setjmp(pipestop))
		goto close_pipe;

	obuf = Popen(cmd, "w");
	if (obuf == NULL) {
		warn("pipecmd: Popen failed: %s", cmd);
		return 1;
	} else
		(void)signal(SIGPIPE, brokpipe);

	(void)sendmessage(dot, obuf, ignoreall, NULL, NULL);
 close_pipe:
	if (obuf != stdout) {
		/*
		 * Ignore SIGPIPE so it can't cause a duplicate close.
		 */
		(void)signal(SIGPIPE, SIG_IGN);
		(void)Pclose(obuf);
		(void)signal(SIGPIPE, SIG_DFL);
	}
	return 0;
}


struct top_core_args_s {
	int lineb;
	int topl;
	struct message *parent;
};
static int
top_core(struct message *mp, void *v)
{
	char buffer[LINESIZE];
	struct top_core_args_s *args;
	FILE *ibuf;
	int lines;
	int c;

	args = v;
	touch(mp);
	if (!args->lineb)
		(void)printf("\n");
	show_msgnum(stdout, mp, args->parent);
	ibuf = setinput(mp);
	c = mp->m_lines;
	for (lines = 0; lines < c && lines <= args->topl; lines++) {
		if (mail_readline(ibuf, buffer, sizeof(buffer)) < 0)
			break;
		(void)puts(buffer);
		args->lineb = blankline(buffer);
	}
	return 0;
}

/*
 * Print the top so many lines of each desired message.
 * The number of lines is taken from the variable "toplines"
 * and defaults to 5.
 */
PUBLIC int
top(void *v)
{
	struct top_core_args_s args;
	int recursive;
	int msgCount;
	int *msgvec = v;
	int *ip;
	int topl;
	char *valtop;

	topl = 5;
	valtop = value(ENAME_TOPLINES);
	if (valtop != NULL) {
		topl = atoi(valtop);
		if (topl < 0 || topl > 10000)
			topl = 5;
	}
	args.topl = topl;
	args.lineb = 1;
	recursive = do_recursion();
	msgCount = get_msgCount();
	for (ip = msgvec; *ip && ip - msgvec < msgCount; ip++) {
		struct message *mp;

		mp = get_message(*ip);
		dot = mp;
		args.parent = recursive ? mp : NULL;
		(void)thread_recursion(mp, top_core, &args);
	}
	return 0;
}

/*
 * Touch all the given messages so that they will
 * get mboxed.
 */
PUBLIC int
stouch(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++)
		dot = set_m_flag(*ip, ~(MPRESERVE | MTOUCH), MTOUCH);

	return 0;
}

/*
 * Make sure all passed messages get mboxed.
 */
PUBLIC int
mboxit(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++)
		dot = set_m_flag(*ip,
		    ~(MPRESERVE | MTOUCH | MBOX), MTOUCH | MBOX);

	return 0;
}

/*
 * List the folders the user currently has.
 */
/*ARGSUSED*/
PUBLIC int
folders(void *v __unused)
{
	char dirname[PATHSIZE];
	const char *cmd;

	if (getfold(dirname, sizeof(dirname)) < 0) {
		(void)printf("No value set for \"folder\"\n");
		return 1;
	}
	if ((cmd = value(ENAME_LISTER)) == NULL)
		cmd = "ls";
	(void)run_command(cmd, 0, -1, -1, dirname, NULL);
	return 0;
}

/*
 * Update the mail file with any new messages that have
 * come in since we started reading mail.
 */
/*ARGSUSED*/
PUBLIC int
inc(void *v __unused)
{
	int nmsg, mdot;

	nmsg = incfile();

	if (nmsg == 0) {
		(void)printf("No new mail.\n");
	} else if (nmsg > 0) {
		struct message *mp;
		mdot = newfileinfo(get_abs_msgCount() - nmsg);
		if ((mp = get_message(mdot)) != NULL)
			dot = mp;
	} else {
		(void)printf("\"inc\" command failed...\n");
	}

	return 0;
}
