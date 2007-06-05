/*	$NetBSD: cmd3.c,v 1.34 2007/06/05 17:50:22 christos Exp $	*/

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
static char sccsid[] = "@(#)cmd3.c	8.2 (Berkeley) 4/20/95";
#else
__RCSID("$NetBSD: cmd3.c,v 1.34 2007/06/05 17:50:22 christos Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include <util.h>
#include "extern.h"
#include "mime.h"
#include "thread.h"

/*
 * Mail -- a mail program
 *
 * Still more user commands.
 */


/*
 * Do a dictionary order comparison of the arguments from
 * qsort.
 */
static int
diction(const void *a, const void *b)
{
	return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/*
 * Sort the passed string vector into ascending dictionary
 * order.
 */
PUBLIC void
sort(const char **list)
{
	const char **ap;

	for (ap = list; *ap != NULL; ap++)
		continue;
	if (ap-list < 2)
		return;
	qsort(list, (size_t)(ap-list), sizeof(*list), diction);
}

/*
 * Expand the shell escape by expanding unescaped !'s into the
 * last issued command where possible.
 */
static int
bangexp(char *str)
{
	static char lastbang[128];
	char bangbuf[LINESIZE];
	char *cp, *cp2;
	int n;
	int changed = 0;

	cp = str;
	cp2 = bangbuf;
	n = sizeof(bangbuf);	/* bytes left in bangbuf */
	while (*cp) {
		if (*cp == '!') {
			if (n < (int)strlen(lastbang)) {
overf:
				(void)printf("Command buffer overflow\n");
				return -1;
			}
			changed++;
			(void)strcpy(cp2, lastbang);
			cp2 += strlen(lastbang);
			n -= strlen(lastbang);
			cp++;
			continue;
		}
		if (*cp == '\\' && cp[1] == '!') {
			if (--n <= 1)
				goto overf;
			*cp2++ = '!';
			cp += 2;
			changed++;
		}
		if (--n <= 1)
			goto overf;
		*cp2++ = *cp++;
	}
	*cp2 = 0;
	if (changed) {
		(void)printf("!%s\n", bangbuf);
		(void)fflush(stdout);
	}
	(void)strcpy(str, bangbuf);
	(void)strlcpy(lastbang, bangbuf, sizeof(lastbang));
	return 0;
}

/*
 * Process a shell escape by saving signals, ignoring signals,
 * and forking a sh -c
 */
PUBLIC int
shell(void *v)
{
	char *str = v;
	sig_t sigint = signal(SIGINT, SIG_IGN);
	const char *shellcmd;
	char cmd[LINESIZE];

	(void)strcpy(cmd, str);
	if (bangexp(cmd) < 0)
		return 1;
	if ((shellcmd = value(ENAME_SHELL)) == NULL)
		shellcmd = _PATH_CSHELL;
	(void)run_command(shellcmd, 0, 0, 1, "-c", cmd, NULL);
	(void)signal(SIGINT, sigint);
	(void)printf("!\n");
	return 0;
}

/*
 * Fork an interactive shell.
 */
/*ARGSUSED*/
PUBLIC int
dosh(void *v __unused)
{
	sig_t sigint = signal(SIGINT, SIG_IGN);
	const char *shellcmd;

	if ((shellcmd = value(ENAME_SHELL)) == NULL)
		shellcmd = _PATH_CSHELL;
	(void)run_command(shellcmd, 0, 0, 1, NULL);
	(void)signal(SIGINT, sigint);
	(void)putchar('\n');
	return 0;
}

/*
 * Print out a nice help message from some file or another.
 */

/*ARGSUSED*/
PUBLIC int
help(void *v __unused)
{
	cathelp(_PATH_HELP);
	return 0;
}

/*
 * Change user's working directory.
 */
PUBLIC int
schdir(void *v)
{
	char **arglist = v;
	const char *cp;

	if (*arglist == NULL)
		cp = homedir;
	else
		if ((cp = expand(*arglist)) == NULL)
			return 1;
	if (chdir(cp) < 0) {
		warn("%s", cp);
		return 1;
	}
	return 0;
}

/*
 * Return the smopts field if "ReplyAsRecipient" is defined.
 */
static struct name *
set_smopts(struct message *mp)
{
	char *cp;
	struct name *np = NULL;
	char *reply_as_recipient = value(ENAME_REPLYASRECIPIENT);

	if (reply_as_recipient &&
	    (cp = skin(hfield("to", mp))) != NULL &&
	    extract(cp, GTO)->n_flink == NULL) {  /* check for one recipient */
		char *p, *q;
		size_t len = strlen(cp);
		/*
		 * XXX - perhaps we always want to ignore
		 *       "undisclosed-recipients:;" ?
		 */
		for (p = q = reply_as_recipient; *p; p = q) {
			while (*q != '\0' && *q != ',' && !isblank((unsigned char)*q))
				q++;
			if (p + len == q && strncasecmp(cp, p, len) == 0)
				return np;
			while (*q == ',' || isblank((unsigned char)*q))
				q++;
		}
		np = extract(__UNCONST("-f"), GSMOPTS);
		np = cat(np, extract(cp, GSMOPTS));
	}

	return np;
}

/*
 * Modify the subject we are replying to to begin with Re: if
 * it does not already.
 */
static char *
reedit(char *subj)
{
	char *newsubj;

	if (subj == NULL)
		return NULL;
	if ((subj[0] == 'r' || subj[0] == 'R') &&
	    (subj[1] == 'e' || subj[1] == 'E') &&
	    subj[2] == ':')
		return subj;
	newsubj = salloc(strlen(subj) + 5);
	(void)strcpy(newsubj, "Re: ");
	(void)strcpy(newsubj + 4, subj);
	return newsubj;
}

/*
 * Set the "In-Reply-To" and "References" header fields appropriately.
 * Used in replies.
 */
static void
set_ident_fields(struct header *hp, struct message *mp)
{
	char *in_reply_to;
	char *references;

	in_reply_to = hfield("message-id", mp);
	hp->h_in_reply_to = in_reply_to;

	references = hfield("references", mp);
	hp->h_references = extract(references, GMISC);
	hp->h_references = cat(hp->h_references, extract(in_reply_to, GMISC));
}

/*
 * Reply to a list of messages.  Extract each name from the
 * message header and send them off to mail1()
 */
static int
_respond(int *msgvec)
{
	struct message *mp;
	char *cp, *rcv, *replyto;
	char **ap;
	struct name *np;
	struct header head;

	/* ensure that all header fields are initially NULL */
	(void)memset(&head, 0, sizeof(head));

	if (msgvec[1] != 0) {
		(void)printf("Sorry, can't reply to multiple messages at once\n");
		return 1;
	}
	mp = get_message(msgvec[0]);
	touch(mp);
	dot = mp;
	if ((rcv = skin(hfield("from", mp))) == NULL)
		rcv = skin(nameof(mp, 1));
	if ((replyto = skin(hfield("reply-to", mp))) != NULL)
		np = extract(replyto, GTO);
	else if ((cp = skin(hfield("to", mp))) != NULL)
		np = extract(cp, GTO);
	else
		np = NULL;
	np = elide(np);
	/*
	 * Delete my name from the reply list,
	 * and with it, all my alternate names.
	 */
	np = delname(np, myname);
	if (altnames)
		for (ap = altnames; *ap; ap++)
			np = delname(np, *ap);
	if (np != NULL && replyto == NULL)
		np = cat(np, extract(rcv, GTO));
	else if (np == NULL) {
		if (replyto != NULL)
			(void)printf("Empty reply-to field -- replying to author\n");
		np = extract(rcv, GTO);
	}
	head.h_to = np;
	if ((head.h_subject = hfield("subject", mp)) == NULL)
		head.h_subject = hfield("subj", mp);
	head.h_subject = reedit(head.h_subject);
	if (replyto == NULL && (cp = skin(hfield("cc", mp))) != NULL) {
		np = elide(extract(cp, GCC));
		np = delname(np, myname);
		if (altnames != 0)
			for (ap = altnames; *ap; ap++)
				np = delname(np, *ap);
		head.h_cc = np;
	} else
		head.h_cc = NULL;
	head.h_bcc = NULL;
	head.h_smopts = set_smopts(mp);
#ifdef MIME_SUPPORT
	head.h_attach = NULL;
#endif
	set_ident_fields(&head, mp);
	mail1(&head, 1);
	return 0;
}

/*
 * Reply to a series of messages by simply mailing to the senders
 * and not messing around with the To: and Cc: lists as in normal
 * reply.
 */
static int
_Respond(int msgvec[])
{
	struct header head;
	struct message *mp;
	int *ap;
	char *cp;

	/* ensure that all header fields are initially NULL */
	(void)memset(&head, 0, sizeof(head));

	head.h_to = NULL;
	for (ap = msgvec; *ap != 0; ap++) {
		mp = get_message(*ap);
		touch(mp);
		dot = mp;
		if ((cp = skin(hfield("from", mp))) == NULL)
			cp = skin(nameof(mp, 2));
		head.h_to = cat(head.h_to, extract(cp, GTO));
	}
	if (head.h_to == NULL)
		return 0;
	mp = get_message(msgvec[0]);
	if ((head.h_subject = hfield("subject", mp)) == NULL)
		head.h_subject = hfield("subj", mp);
	head.h_subject = reedit(head.h_subject);
	head.h_cc = NULL;
	head.h_bcc = NULL;
	head.h_smopts = set_smopts(mp);
#ifdef MIME_SUPPORT
	head.h_attach = NULL;
#endif
	set_ident_fields(&head, mp);
	mail1(&head, 1);
	return 0;
}

PUBLIC int
respond(void *v)
{
	int *msgvec = v;
	if (value(ENAME_REPLYALL) == NULL)
		return _respond(msgvec);
	else
		return _Respond(msgvec);
}

PUBLIC int
Respond(void *v)
{
	int *msgvec = v;
	if (value(ENAME_REPLYALL) == NULL)
		return _Respond(msgvec);
	else
		return _respond(msgvec);
}

/*
 * Preserve the named messages, so that they will be sent
 * back to the system mailbox.
 */
PUBLIC int
preserve(void *v)
{
	int *msgvec = v;
	int *ip;

	if (edit) {
		(void)printf("Cannot \"preserve\" in edit mode\n");
		return 1;
	}
	for (ip = msgvec; *ip != 0; ip++)
		dot = set_m_flag(*ip, ~(MBOX | MPRESERVE), MPRESERVE);

	return 0;
}

/*
 * Mark all given messages as unread, preserving the new status.
 */
PUBLIC int
unread(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++)
		dot = set_m_flag(*ip, ~(MREAD | MTOUCH | MSTATUS), MSTATUS);

	return 0;
}

/*
 * Mark all given messages as read.
 */
PUBLIC int
markread(void *v)
{
	int *msgvec = v;
	int *ip;

	for (ip = msgvec; *ip != 0; ip++)
		dot = set_m_flag(*ip,
		    ~(MNEW | MTOUCH | MREAD | MSTATUS), MREAD | MSTATUS);

	return 0;
}

/*
 * Print the size of each message.
 */
PUBLIC int
messize(void *v)
{
	int *msgvec = v;
	struct message *mp;
	int *ip, mesg;

	for (ip = msgvec; *ip != 0; ip++) {
		mesg = *ip;
		mp = get_message(mesg);
		(void)printf("%d: %ld/%llu\n", mesg, mp->m_blines,
		    (unsigned long long)mp->m_size);
	}
	return 0;
}

/*
 * Quit quickly.  If we are sourcing, just pop the input level
 * by returning an error.
 */
/*ARGSUSED*/
PUBLIC int
rexit(void *v __unused)
{
	if (sourcing)
		return 1;
	exit(0);
	/*NOTREACHED*/
}

/*
 * Set or display a variable value.  Syntax is similar to that
 * of csh.
 */
PUBLIC int
set(void *v)
{
	const char **arglist = v;
	struct var *vp;
	const char *cp;
	char varbuf[LINESIZE];
	const char **ap, **p;
	int errs, h, s;
	size_t l;

	if (*arglist == NULL) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				s++;
		ap = salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				*p++ = vp->v_name;
		*p = NULL;
		sort(ap);
		for (p = ap; *p != NULL; p++)
			(void)printf("%s\t%s\n", *p, value(*p));
		return 0;
	}
	errs = 0;
	for (ap = arglist; *ap != NULL; ap++) {
		cp = *ap;
		while (*cp != '=' && *cp != '\0')
			++cp;
		l = cp - *ap;
		if (l >= sizeof varbuf)
			l = sizeof(varbuf) - 1;
		(void)strncpy(varbuf, *ap, l);
		varbuf[l] = '\0';
		if (*cp == '\0')
			cp = "";
		else
			cp++;
		if (equal(varbuf, "")) {
			(void)printf("Non-null variable name required\n");
			errs++;
			continue;
		}
		assign(varbuf, cp);
	}
	return errs;
}

/*
 * Unset a bunch of variable values.
 */
PUBLIC int
unset(void *v)
{
	char **arglist = v;
	struct var *vp, *vp2;
	int errs, h;
	char **ap;

	errs = 0;
	for (ap = arglist; *ap != NULL; ap++) {
		if ((vp2 = lookup(*ap)) == NULL) {
			if (getenv(*ap)) {
				(void)unsetenv(*ap);
			} else if (!sourcing) {
				(void)printf("\"%s\": undefined variable\n", *ap);
				errs++;
			}
			continue;
		}
		h = hash(*ap);
		if (vp2 == variables[h]) {
			variables[h] = variables[h]->v_link;
			v_free(vp2->v_name);
                        v_free(vp2->v_value);
			free(vp2);
			continue;
		}
		for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link)
			continue;
		vp->v_link = vp2->v_link;
                v_free(vp2->v_name);
                v_free(vp2->v_value);
		free(vp2);
	}
	return errs;
}

/*
 * Show a variable value.
 */
PUBLIC int
show(void *v)
{
	const char **arglist = v;
	struct var *vp;
	const char **ap, **p;
	int h, s;

	if (*arglist == NULL) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				s++;
		ap = salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (vp = variables[h]; vp != NULL; vp = vp->v_link)
				*p++ = vp->v_name;
		*p = NULL;
		sort(ap);
		for (p = ap; *p != NULL; p++)
			(void)printf("%s=%s\n", *p, value(*p));
		return 0;
	}

	for (ap = arglist; *ap != NULL; ap++) {
		char *val = value(*ap);
		(void)printf("%s=%s\n", *ap, val ? val : "<null>");
	}
	return 0;
}


/*
 * Put add users to a group.
 */
PUBLIC int
group(void *v)
{
	const char **argv = v;
	struct grouphead *gh;
	struct group *gp;
	int h;
	int s;
	const char *gname;
	const char **ap, **p;

	if (*argv == NULL) {
		for (h = 0, s = 1; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NULL; gh = gh->g_link)
				s++;
		ap = salloc(s * sizeof *ap);
		for (h = 0, p = ap; h < HSHSIZE; h++)
			for (gh = groups[h]; gh != NULL; gh = gh->g_link)
				*p++ = gh->g_name;
		*p = NULL;
		sort(ap);
		for (p = ap; *p != NULL; p++)
			printgroup(*p);
		return 0;
	}
	if (argv[1] == NULL) {
		printgroup(*argv);
		return 0;
	}
	gname = *argv;
	h = hash(gname);
	if ((gh = findgroup(gname)) == NULL) {
		gh = (struct grouphead *) ecalloc(1, sizeof *gh);
		gh->g_name = vcopy(gname);
		gh->g_list = NULL;
		gh->g_link = groups[h];
		groups[h] = gh;
	}

	/*
	 * Insert names from the command list into the group.
	 * Who cares if there are duplicates?  They get tossed
	 * later anyway.
	 */

	for (ap = argv + 1; *ap != NULL; ap++) {
		gp = (struct group *) ecalloc(1, sizeof *gp);
		gp->ge_name = vcopy(*ap);
		gp->ge_link = gh->g_list;
		gh->g_list = gp;
	}
	return 0;
}

/*
 * Delete the named group alias. Return zero if the group was
 * successfully deleted, or -1 if there was no such group.
 */
static int
delgroup(const char *name)
{
	struct grouphead *gh, *p;
	struct group *g;
	int h;

	h = hash(name);
	for (gh = groups[h], p = NULL; gh != NULL; p = gh, gh = gh->g_link)
		if (strcmp(gh->g_name, name) == 0) {
			if (p == NULL)
				groups[h] = gh->g_link;
			else
				p->g_link = gh->g_link;
			while (gh->g_list != NULL) {
				g = gh->g_list;
				gh->g_list = g->ge_link;
				free(g->ge_name);
				free(g);
			}
			free(gh->g_name);
			free(gh);
			return 0;
		}
	return -1;
}

/*
 * The unalias command takes a list of alises
 * and discards the remembered groups of users.
 */
PUBLIC int
unalias(void *v)
{
	char **ap;

	for (ap = v; *ap != NULL; ap++)
		(void)delgroup(*ap);
	return 0;
}

/*
 * The do nothing command for comments.
 */
/*ARGSUSED*/
PUBLIC int
null(void *v __unused)
{
	return 0;
}

/*
 * Change to another file.  With no argument, print information about
 * the current file.
 */
PUBLIC int
file(void *v)
{
	char **argv = v;

	if (argv[0] == NULL) {
		(void)newfileinfo(0);
		return 0;
	}
	if (setfile(*argv) < 0)
		return 1;
	announce();

	return 0;
}

/*
 * Expand file names like echo
 */
PUBLIC int
echo(void *v)
{
	char **argv = v;
	char **ap;
	const char *cp;

	for (ap = argv; *ap != NULL; ap++) {
		cp = *ap;
		if ((cp = expand(cp)) != NULL) {
			if (ap != argv)
				(void)putchar(' ');
			(void)printf("%s", cp);
		}
	}
	(void)putchar('\n');
	return 0;
}

/*
 * Routines to push and pop the condition code to support nested
 * if/else/endif statements.
 */
static void
push_cond(int c_cond)
{
	struct cond_stack_s *csp;
	csp = emalloc(sizeof(*csp));
	csp->c_cond = c_cond;
	csp->c_next = cond_stack;
	cond_stack = csp;
}

static int
pop_cond(void)
{
	int c_cond;
	struct cond_stack_s *csp;

	if ((csp = cond_stack) == NULL)
		return -1;

	c_cond = csp->c_cond;
	cond_stack = csp->c_next;
	free(csp);
	return c_cond;	
}

/*
 * Conditional commands.  These allow one to parameterize one's
 * .mailrc and do some things if sending, others if receiving.
 */
static int
if_push(void)
{
	push_cond(cond);
	cond &= ~CELSE;
	if ((cond & (CIF | CSKIP)) == (CIF | CSKIP)) {
		cond |= CIGN;
		return 1;
	}
	return 0;
}

PUBLIC int
ifcmd(void *v)
{
	char **argv = v;
	char *keyword = argv[0];
	static const struct modetbl_s {
		const char *m_name;
		enum mailmode_e m_mode;
	} modetbl[] = {
		{ "receiving",		mm_receiving },
		{ "sending",		mm_sending },
		{ "headersonly",	mm_hdrsonly },
		{ NULL,			0 },
	};
	const struct modetbl_s *mtp;

	if (if_push())
		return 0;

	cond = CIF;
	for (mtp = modetbl; mtp->m_name; mtp++)
		if (strcasecmp(keyword, mtp->m_name) == 0)
			break;

	if (mtp->m_name == NULL) {
		cond = CNONE;
		(void)printf("Unrecognized if-keyword: \"%s\"\n", keyword);
		return 1;
	}
	if (mtp->m_mode != mailmode)
		cond |= CSKIP;

	return 0;
}

PUBLIC int
ifdefcmd(void *v)
{
	char **argv = v;

	if (if_push())
		return 0;

	cond = CIF;
	if (value(argv[0]) == NULL)
		cond |= CSKIP;

	return 0;
}

PUBLIC int
ifndefcmd(void *v)
{
	int rval;
	rval = ifdefcmd(v);
	cond ^= CSKIP;
	return rval;
}

/*
 * Implement 'else'.  This is pretty simple -- we just
 * flip over the conditional flag.
 */
/*ARGSUSED*/
PUBLIC int
elsecmd(void *v __unused)
{
	if (cond_stack == NULL || (cond & (CIF | CELSE)) != CIF) {
		(void)printf("\"else\" without matching \"if\"\n");
		cond = CNONE;
		return 1;
	}
	if ((cond & CIGN) == 0) {
		cond ^= CSKIP;
		cond |= CELSE;
	}
	return 0;
}

/*
 * End of if statement.  Just set cond back to anything.
 */
/*ARGSUSED*/
PUBLIC int
endifcmd(void *v __unused)
{
	if (cond_stack == NULL || (cond & CIF) != CIF) {
		(void)printf("\"endif\" without matching \"if\"\n");
		cond = CNONE;
		return 1;
	}
	cond = pop_cond();
	return 0;
}

/*
 * Set the list of alternate names.
 */
PUBLIC int
alternates(void *v)
{
	char **namelist = v;
	int c;
	char **ap, **ap2, *cp;

	c = argcount(namelist) + 1;
	if (c == 1) {
		if (altnames == 0)
			return 0;
		for (ap = altnames; *ap; ap++)
			(void)printf("%s ", *ap);
		(void)printf("\n");
		return 0;
	}
	if (altnames != 0)
		free(altnames);
	altnames = (char **) ecalloc((unsigned) c, sizeof (char *));
	for (ap = namelist, ap2 = altnames; *ap; ap++, ap2++) {
		cp = ecalloc((unsigned) strlen(*ap) + 1, sizeof (char));
		(void)strcpy(cp, *ap);
		*ap2 = cp;
	}
	*ap2 = 0;
	return 0;
}
