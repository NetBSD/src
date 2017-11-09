/*	$NetBSD: quit.c,v 1.29 2017/11/09 20:27:50 christos Exp $	*/

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
static char sccsid[] = "@(#)quit.c	8.2 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: quit.c,v 1.29 2017/11/09 20:27:50 christos Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include "thread.h"
#include "sig.h"

/*
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

/*
 * The "quit" command.
 */
/*ARGSUSED*/
PUBLIC int
quitcmd(void *v __unused)
{
	/*
	 * If we are sourcing, then return 1 so execute() can handle it.
	 * Otherwise, return -1 to abort command loop.
	 */
	if (sourcing)
		return 1;
	return -1;
}

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  On any error, just return -1.  Else return 0.
 * Incorporate the any new mail that we found.
 */
static int
writeback(FILE *res)
{
	struct message *mp;
	int p, c;
	FILE *obuf;

	p = 0;
	if ((obuf = Fopen(mailname, "ref+")) == NULL) {
		warn("%s", mailname);
		return -1;
	}
#ifndef APPEND
	if (res != NULL) {
		while ((c = getc(res)) != EOF)
			(void)putc(c, obuf);
		(void)fflush(obuf);
		if (ferror(obuf)) {
			warn("%s", mailname);
			(void)Fclose(obuf);
			return -1;
		}
	}
#endif
	for (mp = get_message(1); mp; mp = next_message(mp))
		if ((mp->m_flag & MPRESERVE) || (mp->m_flag & MTOUCH)==0) {
			p++;
			if (sendmessage(mp, obuf, NULL, NULL, NULL) < 0) {
				warn("%s", mailname);
				(void)Fclose(obuf);
				return -1;
			}
		}
#ifdef APPEND
	if (res != NULL)
		while ((c = getc(res)) != EOF)
			(void)putc(c, obuf);
#endif
	(void)fflush(obuf);
	if (!ferror(obuf))
		trunc(obuf);	/* XXX or should we truncate? */
	if (ferror(obuf)) {
		warn("%s", mailname);
		(void)Fclose(obuf);
		return -1;
	}
	if (res != NULL)
		(void)Fclose(res);
	(void)Fclose(obuf);
	alter(mailname);
	if (p == 1)
		(void)printf("Held 1 message in %s\n", mailname);
	else
		(void)printf("Held %d messages in %s\n", p, mailname);
	return 0;
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
static void
edstop(jmp_buf jmpbuf)
{
	int gotcha, c;
	struct message *mp;
	FILE *obuf;
	FILE *ibuf;
	FILE *readstat;
	struct stat statb;
	char tempname[PATHSIZE];
	int fd;

	sig_check();
	if (readonly)
		return;

	readstat = NULL;
	if (Tflag != NULL) {
		if ((readstat = Fopen(Tflag, "wef")) == NULL)
			Tflag = NULL;
	}
	for (mp = get_message(1), gotcha = 0; mp; mp = next_message(mp)) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MMODIFY|MDELETED|MSTATUS))
			gotcha++;
		if (Tflag != NULL && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			char *id;

			if ((id = hfield("article-id", mp)) != NULL)
				(void)fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NULL)
		(void)Fclose(readstat);
	if (!gotcha || Tflag != NULL)
		goto done;
	ibuf = NULL;
	if (stat(mailname, &statb) >= 0 && statb.st_size > mailsize) {
		(void)snprintf(tempname, sizeof(tempname),
		    "%s/mbox.XXXXXXXXXX", tmpdir);
		if ((fd = mkstemp(tempname)) == -1 ||
		    (obuf = Fdopen(fd, "wef")) == NULL) {
			warn("%s", tempname);
			if (fd != -1)
				(void)close(fd);
			sig_release();
			longjmp(jmpbuf, -11);
		}
		if ((ibuf = Fopen(mailname, "ref")) == NULL) {
			warn("%s", mailname);
			(void)Fclose(obuf);
			(void)rm(tempname);
			sig_release();
			longjmp(jmpbuf, -1);
		}
		(void)fseek(ibuf, (long)mailsize, 0);
		while ((c = getc(ibuf)) != EOF)
			(void)putc(c, obuf);
		(void)fflush(obuf);
		if (ferror(obuf)) {
			warn("%s", tempname);
			(void)Fclose(obuf);
			(void)Fclose(ibuf);
			(void)rm(tempname);
			sig_release();
			longjmp(jmpbuf, -1);
		}
		(void)Fclose(ibuf);
		(void)Fclose(obuf);
		if ((ibuf = Fopen(tempname, "ref")) == NULL) {
			warn("%s", tempname);
			(void)rm(tempname);
			sig_release();
			longjmp(jmpbuf, -1);
		}
		(void)rm(tempname);
	}
	(void)printf("\"%s\" ", mailname);
	(void)fflush(stdout);
	if ((obuf = Fopen(mailname, "ref+")) == NULL) {
		warn("%s", mailname);
		sig_release();
		longjmp(jmpbuf, -1);
	}
	trunc(obuf);
	c = 0;
	for (mp = get_message(1); mp; mp = next_message(mp)) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (sendmessage(mp, obuf, NULL, NULL, NULL) < 0) {
			warn("%s", mailname);
			sig_release();
			longjmp(jmpbuf, -1);
		}
	}
	gotcha = (c == 0 && ibuf == NULL);
	if (ibuf != NULL) {
		while ((c = getc(ibuf)) != EOF)
			(void)putc(c, obuf);
		(void)Fclose(ibuf);
	}
	(void)fflush(obuf);
	if (ferror(obuf)) {
		warn("%s", mailname);
		sig_release();
		longjmp(jmpbuf, -1);
	}
	(void)Fclose(obuf);
	if (gotcha) {
		(void)rm(mailname);
		(void)printf("removed\n");
	} else
		(void)printf("complete\n");
	(void)fflush(stdout);

done:
	sig_release();
	sig_check();
}

/*
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */
PUBLIC void
quit(jmp_buf jmpbuf)
{
	int mcount, p, modify, autohold, anystat, holdbit, nohold;
	_Bool append;
	FILE *ibuf, *obuf, *fbuf, *rbuf, *readstat, *abuf;
	struct message *mp;
	int c, fd;
	struct stat minfo;
	const char *mbox;
	char tempname[PATHSIZE];

#ifdef __GNUC__		/* XXX gcc -Wuninitialized */
	ibuf = NULL;
	readstat = NULL;
#endif
	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */
	if (readonly)
		return;

#ifdef THREAD_SUPPORT
	(void)showtagscmd(NULL);	/* make sure we see tagged messages */
	(void)unthreadcmd(NULL);
#endif
	/*
	 * If editing (not reading system mail box), then do the work
	 * in edstop()
	 */
	if (edit) {
		edstop(jmpbuf);
		return;
	}

	/*
	 * See if there any messages to save in mbox.  If no, we
	 * can save copying mbox to /tmp and back.
	 *
	 * Check also to see if any files need to be preserved.
	 * Delete all untouched messages to keep them out of mbox.
	 * If all the messages are to be preserved, just exit with
	 * a message.
	 */

	fbuf = Fopen(mailname, "ref");
	if (fbuf == NULL)
		goto newmail;
	if (flock(fileno(fbuf), LOCK_EX) == -1) {
nolock:
		warn("Unable to lock mailbox");
		(void)Fclose(fbuf);
		return;
	}
	if (dot_lock(mailname, 1, stdout, ".") == -1)
		goto nolock;
	rbuf = NULL;
	if (fstat(fileno(fbuf), &minfo) >= 0 && minfo.st_size > mailsize) {
		(void)printf("New mail has arrived.\n");
		(void)snprintf(tempname, sizeof(tempname),
		    "%s/mail.RqXXXXXXXXXX", tmpdir);
		if ((fd = mkstemp(tempname)) == -1 ||
		    (rbuf = Fdopen(fd, "wef")) == NULL) {
		    	if (fd != -1)
				(void)close(fd);
			goto newmail;
		}
#ifdef APPEND
		(void)fseek(fbuf, (long)mailsize, 0);
		while ((c = getc(fbuf)) != EOF)
			(void)putc(c, rbuf);
#else
		p = minfo.st_size - mailsize;
		while (p-- > 0) {
			c = getc(fbuf);
			if (c == EOF)
				goto newmail;
			(void)putc(c, rbuf);
		}
#endif
		(void)fflush(rbuf);
		if (ferror(rbuf)) {
			warn("%s", tempname);
			(void)Fclose(rbuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)Fclose(rbuf);
		if ((rbuf = Fopen(tempname, "ref")) == NULL)
			goto newmail;
		(void)rm(tempname);
	}

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value(ENAME_HOLD) != NULL;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value(ENAME_KEEPSAVE) != NULL)
		nohold &= ~MSAVED;
	for (mp = get_message(1); mp; mp = next_message(mp)) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & MSTATUS)
			anystat++;
		if ((mp->m_flag & MTOUCH) == 0)
			mp->m_flag |= MPRESERVE;
		if ((mp->m_flag & nohold) == 0)
			mp->m_flag |= holdbit;
	}
	modify = 0;
	if (Tflag != NULL) {
		if ((readstat = Fopen(Tflag, "wef")) == NULL)
			Tflag = NULL;
	}
	for (c = 0, p = 0, mp = get_message(1); mp; mp = next_message(mp)) {
		if (mp->m_flag & MBOX)
			c++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MMODIFY)
			modify++;
		if (Tflag != NULL && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			char *id;

			if ((id = hfield("article-id", mp)) != NULL)
				(void)fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NULL)
		(void)Fclose(readstat);
	if (p == get_msgCount() && !modify && !anystat) {
		(void)printf("Held %d message%s in %s\n",
			p, p == 1 ? "" : "s", mailname);
		(void)Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}
	if (c == 0) {
		if (p != 0) {
			(void)writeback(rbuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		goto cream;
	}

	/*
	 * Create another temporary file and copy user's mbox file
	 * darin.  If there is no mbox, copy nothing.
	 * If he has specified "append" don't copy his mailbox,
	 * just copy saveable entries at the end.
	 */

	mbox = expand("&");
	mcount = c;
	append = value(ENAME_APPEND) != NULL;
	if (!append) {
		(void)snprintf(tempname, sizeof(tempname),
		    "%s/mail.RmXXXXXXXXXX", tmpdir);
		if ((fd = mkstemp(tempname)) == -1 ||
		    (obuf = Fdopen(fd, "wef")) == NULL) {
			warn("%s", tempname);
			if (fd != -1)
				(void)close(fd);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		if ((ibuf = Fopen(tempname, "ref")) == NULL) {
			warn("%s", tempname);
			(void)rm(tempname);
			(void)Fclose(obuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)rm(tempname);
		if ((abuf = Fopen(mbox, "ref")) != NULL) {
			while ((c = getc(abuf)) != EOF)
				(void)putc(c, obuf);
			(void)Fclose(abuf);
		}
		if (ferror(obuf)) {
			warn("%s", tempname);
			(void)Fclose(ibuf);
			(void)Fclose(obuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)Fclose(obuf);
		if ((fd = creat(mbox, 0600)) != -1)
			(void)close(fd);
		if ((obuf = Fopen(mbox, "ref+")) == NULL) {
			warn("%s", mbox);
			(void)Fclose(ibuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
	}
	else {
		if ((obuf = Fopen(mbox, "aef")) == NULL) {
			warn("%s", mbox);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)fchmod(fileno(obuf), 0600);
	}
	for (mp = get_message(1); mp; mp = next_message(mp))
		if (mp->m_flag & MBOX)
			if (sendmessage(mp, obuf, saveignore, NULL, NULL) < 0) {
				warn("%s", mbox);
				if (!append)
					(void)Fclose(ibuf);
				(void)Fclose(obuf);
				(void)Fclose(fbuf);
				dot_unlock(mailname);
				return;
			}

	/*
	 * Copy the user's old mbox contents back
	 * to the end of the stuff we just saved.
	 * If we are appending, this is unnecessary.
	 */

	if (!append) {
		rewind(ibuf);
		c = getc(ibuf);
		while (c != EOF) {
			(void)putc(c, obuf);
			if (ferror(obuf))
				break;
			c = getc(ibuf);
		}
		(void)Fclose(ibuf);
	}
	(void)fflush(obuf);
	if (!ferror(obuf))
		trunc(obuf);	/* XXX or should we truncate? */
	if (ferror(obuf)) {
		warn("%s", mbox);
		(void)Fclose(obuf);
		(void)Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}
	(void)Fclose(obuf);
	if (mcount == 1)
		(void)printf("Saved 1 message in mbox\n");
	else
		(void)printf("Saved %d messages in mbox\n", mcount);

	/*
	 * Now we are ready to copy back preserved files to
	 * the system mailbox, if any were requested.
	 */

	if (p != 0) {
		(void)writeback(rbuf);
		(void)Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}

	/*
	 * Finally, remove his /var/mail file.
	 * If new mail has arrived, copy it back.
	 */

cream:
	if (rbuf != NULL) {
		abuf = Fopen(mailname, "ref+");
		if (abuf == NULL)
			goto newmail;
		while ((c = getc(rbuf)) != EOF)
			(void)putc(c, abuf);
		(void)fflush(abuf);
		if (ferror(abuf)) {
			warn("%s", mailname);
			(void)Fclose(abuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)Fclose(rbuf);
		trunc(abuf);
		(void)Fclose(abuf);
		alter(mailname);
		(void)Fclose(fbuf);
		dot_unlock(mailname);
		return;
	}
	demail();
	(void)Fclose(fbuf);
	dot_unlock(mailname);
	return;

newmail:
	(void)printf("Thou hast new mail.\n");
	if (fbuf != NULL) {
		(void)Fclose(fbuf);
		dot_unlock(mailname);
	}
}
