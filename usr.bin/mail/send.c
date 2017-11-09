/*	$NetBSD: send.c,v 1.39 2017/11/09 20:27:50 christos Exp $	*/

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
static char sccsid[] = "@(#)send.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: send.c,v 1.39 2017/11/09 20:27:50 christos Exp $");
#endif
#endif /* not lint */

#include <assert.h>

#include "rcv.h"
#include "extern.h"
#ifdef MIME_SUPPORT
#include "mime.h"
#endif
#include "sig.h"

/*
 * Mail -- a mail program
 *
 * Mail to others.
 */

/*
 * compare twwo name structures.  Support for get_smopts.
 */
static int
namecmp(struct name *np1, struct name *np2)
{
	for (/*EMPTY*/; np1 && np2; np1 = np1->n_flink, np2 = np2->n_flink) {
		if (strcmp(np1->n_name, np2->n_name) != 0)
			return 1;
	}
	return np1 || np2;
}

/*
 * Get the sendmail options from the smopts list.
 */
static struct name *
get_smopts(struct name *to)
{
	struct smopts_s *sp;
	struct name *smargs, *np;
	smargs = NULL;
	for (np = to; np; np = np->n_flink) {
		if ((sp = findsmopts(np->n_name, 0)) == NULL)
			continue;
		if (smargs == NULL)
			smargs = sp->s_smopts;
		else if (namecmp(smargs, sp->s_smopts) != 0)
			return NULL;
	}
	if (smargs &&
	    smargs->n_flink == NULL &&
	    (smargs->n_name == NULL || smargs->n_name[0] == '\0'))
		return NULL;
	return smargs;
}

/*
 * Output a reasonable looking status field.
 */
static void
statusput(struct message *mp, FILE *obuf, const char *prefix)
{
	char statout[3];
	char *cp = statout;

	if (mp->m_flag & MREAD)
		*cp++ = 'R';
	if ((mp->m_flag & MNEW) == 0)
		*cp++ = 'O';
	*cp = 0;
	if (statout[0])
		(void)fprintf(obuf, "%sStatus: %s\n",
			prefix == NULL ? "" : prefix, statout);
}

/*
 * Send message described by the passed pointer to the
 * passed output buffer.  Return -1 on error.
 * Adjust the status: field if need be.
 * If doign is given, suppress ignored header fields.
 * prefix is a string to prepend to each output line.
 */
PUBLIC int
sendmessage(struct message *mp, FILE *obuf, struct ignoretab *doign,
    const char *prefix, struct mime_info *mip)
{
	off_t len;
	FILE *ibuf;
	char line[LINESIZE];
	int isheadflag, infld, ignoring, dostat, firstline;
	char *cp;
	size_t prefixlen;
	size_t linelen;
	int rval;

	ignoring = 0;
	prefixlen = 0;
	rval = -1;

	/*
	 * Compute the prefix string, without trailing whitespace
	 */
	if (prefix != NULL) {
		const char *dp, *dp2 = NULL;
		for (dp = prefix; *dp; dp++)
			if (!is_WSP(*dp))
				dp2 = dp;
		prefixlen = dp2 == 0 ? 0 : dp2 - prefix + 1;
	}
	ibuf = setinput(mp);
	len = mp->m_size;
	isheadflag = 1;
	dostat = doign == 0 || !isign("status", doign);
	infld = 0;
	firstline = 1;

	sig_check();
	/*
	 * Process headers first
	 */
#ifdef MIME_SUPPORT
	if (mip)
		obuf = mime_decode_header(mip);
#endif
	while (len > 0 && isheadflag) {
		sig_check();
		if (fgets(line, (int)sizeof(line), ibuf) == NULL)
			break;
		len -= linelen = strlen(line);
		if (firstline) {
			/*
			 * First line is the From line, so no headers
			 * there to worry about
			 */
			firstline = 0;
			ignoring = doign == ignoreall || doign == bouncetab;
#ifdef MIME_SUPPORT
			/* XXX - ignore multipart boundary lines! */
			if (line[0] == '-' && line[1] == '-')
				ignoring = 1;
#endif
		} else if (line[0] == '\n') {
			/*
			 * If line is blank, we've reached end of
			 * headers, so force out status: field
			 * and note that we are no longer in header
			 * fields
			 */
			if (dostat) {
				statusput(mp, obuf, prefix);
				dostat = 0;
			}
			isheadflag = 0;
			ignoring = doign == ignoreall;
		} else if (infld && is_WSP(line[0])) {
			/*
			 * If this line is a continuation (via space or tab)
			 * of a previous header field, just echo it
			 * (unless the field should be ignored).
			 * In other words, nothing to do.
			 */
		} else {
			/*
			 * Pick up the header field if we have one.
			 */
			char *save_cp;
			for (cp = line; *cp && *cp != ':' && !is_WSP(*cp); cp++)
				continue;
			save_cp = cp;
			cp = skip_WSP(cp);
			if (*cp++ != ':') {
				/*
				 * Not a header line, force out status:
				 * This happens in uucp style mail where
				 * there are no headers at all.
				 */
				if (dostat) {
					statusput(mp, obuf, prefix);
					dostat = 0;
				}
				if (doign != ignoreall)
					/* add blank line */
					(void)putc('\n', obuf);
				isheadflag = 0;
				ignoring = 0;
			} else {
				/*
				 * If it is an ignored field and
				 * we care about such things, skip it.
				 */
				int save_c;
				save_c = *save_cp;
				*save_cp = 0;	/* temporarily null terminate */
				if (doign && isign(line, doign))
					ignoring = 1;
				else if ((line[0] == 's' || line[0] == 'S') &&
					 strcasecmp(line, "status") == 0) {
					/*
					 * If the field is "status," go compute
					 * and print the real Status: field
					 */
					if (dostat) {
						statusput(mp, obuf, prefix);
						dostat = 0;
					}
					ignoring = 1;
				} else {
					ignoring = 0;
				}
				infld = 1;
				*save_cp = save_c;	/* restore the line */
			}
		}
		if (!ignoring) {
			/*
			 * Strip trailing whitespace from prefix
			 * if line is blank.
			 */
			if (prefix != NULL) {
				if (linelen > 1)
					(void)fputs(prefix, obuf);
				else
					(void)fwrite(prefix, sizeof(*prefix),
							prefixlen, obuf);
			}
			(void)fwrite(line, sizeof(*line), linelen, obuf);
			if (ferror(obuf))
				goto out;
		}
	}
	sig_check();

	/*
	 * Copy out message body
	 */
#ifdef MIME_SUPPORT
	if (mip) {
		obuf = mime_decode_body(mip);
		sig_check();
		if (obuf == NULL) { /* XXX - early out */
			rval = 0;
			goto out;
		}
	}
#endif
	if (doign == ignoreall)
		len--;		/* skip final blank line */

	linelen = 0;		/* needed for in case len == 0 */
	if (prefix != NULL) {
		while (len > 0) {
			sig_check();
			if (fgets(line, (int)sizeof(line), ibuf) == NULL) {
				linelen = 0;
				break;
			}
			len -= linelen = strlen(line);
			/*
			 * Strip trailing whitespace from prefix
			 * if line is blank.
			 */
			if (linelen > 1)
				(void)fputs(prefix, obuf);
			else
				(void)fwrite(prefix, sizeof(*prefix),
						prefixlen, obuf);
			(void)fwrite(line, sizeof(*line), linelen, obuf);
			if (ferror(obuf))
				goto out;
		}
	}
	else {
		while (len > 0) {
			sig_check();
			linelen = (size_t)len < sizeof(line) ? (size_t)len : sizeof(line);
			if ((linelen = fread(line, sizeof(*line), linelen, ibuf)) == 0) {
				break;
			}
			len -= linelen;
			if (fwrite(line, sizeof(*line), linelen, obuf) != linelen)
				goto out;
		}
	}
	sig_check();
	if (doign == ignoreall && linelen > 0 && line[linelen - 1] != '\n') {
		int c;

		/* no final blank line */
		if ((c = getc(ibuf)) != EOF && putc(c, obuf) == EOF)
			goto out;
	}
	rval = 0;
 out:
	sig_check();
	return rval;
}

/*
 * Fix the header by glopping all of the expanded names from
 * the distribution list into the appropriate fields.
 */
static void
fixhead(struct header *hp, struct name *tolist)
{
	struct name *np;

	hp->h_to = NULL;
	hp->h_cc = NULL;
	hp->h_bcc = NULL;
	for (np = tolist; np != NULL; np = np->n_flink) {
		if (np->n_type & GDEL)
			continue;	/* Don't copy deleted addresses to the header */
		if ((np->n_type & GMASK) == GTO)
			hp->h_to =
				cat(hp->h_to, nalloc(np->n_name, np->n_type));
		else if ((np->n_type & GMASK) == GCC)
			hp->h_cc =
				cat(hp->h_cc, nalloc(np->n_name, np->n_type));
		else if ((np->n_type & GMASK) == GBCC)
			hp->h_bcc =
				cat(hp->h_bcc, nalloc(np->n_name, np->n_type));
	}
}

/*
 * Format the given header line to not exceed 72 characters.
 */
static void
fmt(const char *str, struct name *np, FILE *fo, size_t comma)
{
	size_t col;
	size_t len;

	comma = comma ? 1 : 0;
	col = strlen(str);
	if (col)
		(void)fputs(str, fo);
	for (/*EMPTY*/; np != NULL; np = np->n_flink) {
		if (np->n_flink == NULL)
			comma = 0;
		len = strlen(np->n_name);
		col++;		/* for the space */
		if (col + len + comma > 72 && col > 4) {
			(void)fputs("\n    ", fo);
			col = 4;
		} else
			(void)putc(' ', fo);
		(void)fputs(np->n_name, fo);
		if (comma)
			(void)putc(',', fo);
		col += len + comma;
	}
	(void)putc('\n', fo);
}

/*
 * Formatting for extra_header lines: try to wrap them before 72
 * characters.
 *
 * XXX - should this allow for quoting?
 */
static void
fmt2(const char *str, FILE *fo)
{
	const char *p;
	size_t col;

	col = 0;
	p = strchr(str, ':');
	assert(p != NULL);	/* this is a coding error */

	while (str <= p) {	/* output the header name */
		(void)putc(*str, fo);
		str++;
		col++;
	}
	assert(is_WSP(*str));	/* this is a coding error */

	(void)putc(' ', fo);	/* output a single space after the ':' */
	str = skip_WSP(str);

	while (*str != '\0') {
		if (p <= str) {
			p = strpbrk(str, " \t");
			if (p == NULL)
				p = str + strlen(str);
			if (col + (p - str) > 72 && col > 4) {
				(void)fputs("\n    ", fo);
				col = 4;
				str = skip_WSP(str);
				continue;
			}
		}
		(void)putc(*str, fo);
		str++;
		col++;
	}
	(void)putc('\n', fo);
}

/*
 * Dump the to, subject, cc header on the
 * passed file buffer.
 */
PUBLIC int
puthead(struct header *hp, FILE *fo, int w)
{
	struct name *np;
	int gotcha;

	gotcha = 0;
	if (hp->h_to != NULL && w & GTO)
		fmt("To:", hp->h_to, fo, w & GCOMMA), gotcha++;
	if (hp->h_subject != NULL && w & GSUBJECT)
		(void)fprintf(fo, "Subject: %s\n", hp->h_subject), gotcha++;
	if (hp->h_cc != NULL && w & GCC)
		fmt("Cc:", hp->h_cc, fo, w & GCOMMA), gotcha++;
	if (hp->h_bcc != NULL && w & GBCC)
		fmt("Bcc:", hp->h_bcc, fo, w & GCOMMA), gotcha++;
	if (hp->h_in_reply_to != NULL && w & GMISC)
		(void)fprintf(fo, "In-Reply-To: %s\n", hp->h_in_reply_to), gotcha++;
	if (hp->h_references != NULL && w & GMISC)
		fmt("References:", hp->h_references, fo, w & GCOMMA), gotcha++;
	if (hp->h_extra != NULL && w & GMISC) {
		for (np = hp->h_extra; np != NULL; np = np->n_flink)
			fmt2(np->n_name, fo);
		gotcha++;
	}
	if (hp->h_smopts != NULL && w & GSMOPTS)
		(void)fprintf(fo, "(sendmail options: %s)\n", detract(hp->h_smopts, GSMOPTS)), gotcha++;
#ifdef MIME_SUPPORT
	if (w & GMIME && (hp->h_attach || value(ENAME_MIME_ENCODE_MSG)))
		mime_putheader(fo, hp), gotcha++;
#endif
	if (gotcha && w & GNL)
		(void)putc('\n', fo);
	return 0;
}

/*
 * Prepend a header in front of the collected stuff
 * and return the new file.
 */
static FILE *
infix(struct header *hp, FILE *fi)
{
	FILE *nfo, *nfi;
	int c, fd;
	char tempname[PATHSIZE];

	(void)snprintf(tempname, sizeof(tempname),
	    "%s/mail.RsXXXXXXXXXX", tmpdir);
	if ((fd = mkstemp(tempname)) == -1 ||
	    (nfo = Fdopen(fd, "wef")) == NULL) {
		if (fd != -1)
			(void)close(fd);
		warn("%s", tempname);
		return fi;
	}
	if ((nfi = Fopen(tempname, "ref")) == NULL) {
		warn("%s", tempname);
		(void)Fclose(nfo);
		(void)rm(tempname);
		return fi;
	}
	(void)rm(tempname);
	(void)puthead(hp, nfo,
	    GTO | GSUBJECT | GCC | GBCC | GMISC | GNL | GCOMMA
#ifdef MIME_SUPPORT
	    | GMIME
#endif
		);

	c = getc(fi);
	while (c != EOF) {
		(void)putc(c, nfo);
		c = getc(fi);
	}
	if (ferror(fi)) {
		warn("read");
		rewind(fi);
		return fi;
	}
	(void)fflush(nfo);
	if (ferror(nfo)) {
		warn("%s", tempname);
		(void)Fclose(nfo);
		(void)Fclose(nfi);
		rewind(fi);
		return fi;
	}
	(void)Fclose(nfo);
	(void)Fclose(fi);
	rewind(nfi);
	return nfi;
}

/*
 * Save the outgoing mail on the passed file.
 *
 * Take care not to save a valid headline or the mbox file will be
 * broken.  Prefix valid headlines with '>'.
 *
 * Note: most servers prefix any line starting with "From " in the
 * body of the message.  We are more restrictive to avoid header/body
 * issues.
 */
/*ARGSUSED*/
static int
savemail(const char name[], FILE *fi)
{
	FILE *fo;
	time_t now;
	mode_t m;
	char *line;
	size_t linelen;
	int afterblank;

	m = umask(077);
	fo = Fopen(name, "aef");
	(void)umask(m);
	if (fo == NULL) {
		warn("%s", name);
		return -1;
	}
	(void)time(&now);
	(void)fprintf(fo, "From %s %s", myname, ctime(&now));
	afterblank = 0;
	while ((line = fgetln(fi, &linelen)) != NULL) {
		char c, *cp;
		cp = line + linelen - 1;
		if (afterblank &&
		    linelen > sizeof("From . Aaa Aaa O0 00:00 0000") &&
		    line[0] == 'F' &&
		    line[1] == 'r' &&
		    line[2] == 'o' &&
		    line[3] == 'm' &&
		    line[4] == ' ' &&
		    *cp == '\n') {
			c = *cp;
			*cp = '\0';
			if (ishead(line))
				(void)fputc('>', fo);
			*cp = c;
		}
		(void)fwrite(line, 1, linelen, fo);
		afterblank = linelen == 1 && line[0] == '\n';
	}
	(void)putc('\n', fo);
	(void)fflush(fo);
	if (ferror(fo))
		warn("%s", name);
	(void)Fclose(fo);
	rewind(fi);
	return 0;
}

/*
 * Mail a message that is already prepared in a file.
 */
PUBLIC void
mail2(FILE *mtf, const char **namelist)
{
	int pid;
	const char *cp;

	if (debug) {
		const char **t;

		(void)printf("Sendmail arguments:");
		for (t = namelist; *t != NULL; t++)
			(void)printf(" \"%s\"", *t);
		(void)printf("\n");
		return;
	}
	if ((cp = value(ENAME_RECORD)) != NULL)
		(void)savemail(expand(cp), mtf);
	/*
	 * Fork, set up the temporary mail file as standard
	 * input for "mail", and exec with the user list we generated
	 * far above.
	 */
	pid = fork();
	switch (pid) {
	case -1:
		warn("fork");
		savedeadletter(mtf);
		return;
	case 0:
	{
		sigset_t nset;
		(void)sigemptyset(&nset);
		(void)sigaddset(&nset, SIGHUP);
		(void)sigaddset(&nset, SIGINT);
		(void)sigaddset(&nset, SIGQUIT);
		(void)sigaddset(&nset, SIGTSTP);
		(void)sigaddset(&nset, SIGTTIN);
		(void)sigaddset(&nset, SIGTTOU);
		prepare_child(&nset, fileno(mtf), -1);
		if ((cp = value(ENAME_SENDMAIL)) != NULL)
			cp = expand(cp);
		else
			cp = _PATH_SENDMAIL;
		(void)execv(cp, (char *const *)__UNCONST(namelist));
		warn("%s", cp);
		_exit(1);
		break;		/* Appease GCC */
	}
	default:
		if (value(ENAME_VERBOSE) != NULL)
			(void)wait_child(pid);
		else
			free_child(pid);
		break;
	}
}

static struct name *
ncopy(struct name *np)
{
	struct name *rv;
	struct name *lp;
	struct name *tp;

	lp = NULL; /* XXX gcc -Wuninitialized sh3 */
	rv = NULL;
	for (/*EMPTY*/; np; np = np->n_flink) {
		tp = nalloc(np->n_name, np->n_type);
		if (rv == NULL)
			rv = tp;
		else {
			lp->n_flink = tp;
			tp->n_blink = lp;
		}
		lp = tp;
	}
	return rv;
}

/*
 * Mail a message on standard input to the people indicated
 * in the passed header.  (Internal interface).
 */
PUBLIC void
mail1(struct header *hp, int printheaders)
{
	const char **namelist;
	struct name *to;
	FILE *mtf;

	/*
	 * Collect user's mail from standard input.
	 * Get the result as mtf.
	 */
	if ((mtf = collect(hp, printheaders)) == NULL)
		return;

	/*
	 * Grab any extra header lines.  Do this after collect() so
	 * that we can add header lines while collecting.
	 */
	hp->h_extra = ncopy(extra_headers);

	if (value(ENAME_INTERACTIVE) != NULL) {
		if (value(ENAME_ASKCC) != NULL || value(ENAME_ASKBCC) != NULL) {
			if (value(ENAME_ASKCC) != NULL)
				(void)grabh(hp, GCC);
			if (value(ENAME_ASKBCC) != NULL)
				(void)grabh(hp, GBCC);
		} else {
			(void)printf("EOT\n");
			(void)fflush(stdout);
		}
	}
	if (fsize(mtf) == 0) {
		if (value(ENAME_DONTSENDEMPTY) != NULL)
			goto out;
		if (hp->h_subject == NULL)
			(void)printf("No message, no subject; hope that's ok\n");
		else
			(void)printf("Null message body; hope that's ok\n");
	}
	/*
	 * Now, take the user names from the combined
	 * to and cc lists and do all the alias
	 * processing.
	 */
	senderr = 0;
	to = usermap(cat(hp->h_bcc, cat(hp->h_to, hp->h_cc)));
	if (to == NULL) {
		(void)printf("No recipients specified\n");
		senderr++;
	}
#ifdef MIME_SUPPORT
	/*
	 * If there are attachments, repackage the mail as a
	 * multi-part MIME message.
	 */
	if (hp->h_attach || value(ENAME_MIME_ENCODE_MSG))
		mtf = mime_encode(mtf, hp);
#endif
	/*
	 * Look through the recipient list for names with /'s
	 * in them which we write to as files directly.
	 */
	to = outof(to, mtf, hp);
	if (senderr)
		savedeadletter(mtf);
	to = elide(to);
	if (count(to) == 0)
		goto out;
	fixhead(hp, to);
	if ((mtf = infix(hp, mtf)) == NULL) {
		(void)fprintf(stderr, ". . . message lost, sorry.\n");
		return;
	}
	if (hp->h_smopts == NULL) {
		hp->h_smopts = get_smopts(to);
		if (hp->h_smopts != NULL &&
		    hp->h_smopts->n_name[0] != '\0' &&
		    value(ENAME_SMOPTS_VERIFY) != NULL)
			if (grabh(hp, GSMOPTS)) {
				(void)printf("mail aborted!\n");
				savedeadletter(mtf);
				goto out;
			}
	}
	namelist = unpack(hp->h_smopts, to);
	mail2(mtf, namelist);
 out:
	(void)Fclose(mtf);
}

/*
 * Interface between the argument list and the mail1 routine
 * which does all the dirty work.
 */
PUBLIC int
mail(struct name *to, struct name *cc, struct name *bcc,
     struct name *smopts, char *subject, struct attachment *attach)
{
	struct header head;

	/* ensure that all header fields are initially NULL */
	(void)memset(&head, 0, sizeof(head));

	head.h_to = to;
	head.h_subject = subject;
	head.h_cc = cc;
	head.h_bcc = bcc;
	head.h_smopts = smopts;
#ifdef MIME_SUPPORT
	head.h_attach = attach;
#endif
	mail1(&head, 0);
	return 0;
}

/*
 * Send mail to a bunch of user names.  The interface is through
 * the mail1 routine above.
 */
PUBLIC int
sendmail(void *v)
{
	char *str = v;
	struct header head;

	/* ensure that all header fields are initially NULL */
	(void)memset(&head, 0, sizeof(head));

	head.h_to = extract(str, GTO);

	mail1(&head, 0);
	return 0;
}
