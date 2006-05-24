/*	$NetBSD: quit.c,v 1.23 2006/05/24 15:53:21 christos Exp $	*/

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
__RCSID("$NetBSD: quit.c,v 1.23 2006/05/24 15:53:21 christos Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"

/*
 * Rcv -- receive mail rationally.
 *
 * Termination processing.
 */

/*
 * The "quit" command.
 */
int
/*ARGSUSED*/
quitcmd(void *v)
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
 * Save all of the undetermined messages at the top of "mbox"
 * Save all untouched messages back in the system mailbox.
 * Remove the system mailbox, if none saved there.
 */
void
quit(void)
{
	int mcount, p, modify, autohold, anystat, holdbit, nohold;
	FILE *ibuf = NULL, *obuf, *fbuf, *rbuf, *readstat = NULL, *abuf;
	struct message *mp;
	int c, fd;
	struct stat minfo;
	const char *mbox;
	char tempname[PATHSIZE];

#ifdef __GNUC__
	obuf = NULL;		/* XXX gcc -Wuninitialized */
#endif

	/*
	 * If we are read only, we can't do anything,
	 * so just return quickly.
	 */
	if (readonly)
		return;
	/*
	 * If editing (not reading system mail box), then do the work
	 * in edstop()
	 */
	if (edit) {
		edstop();
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

	fbuf = Fopen(mailname, "r");
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
		    (rbuf = Fdopen(fd, "w")) == NULL) {
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
		if ((rbuf = Fopen(tempname, "r")) == NULL)
			goto newmail;
		(void)rm(tempname);
	}

	/*
	 * Adjust the message flags in each message.
	 */

	anystat = 0;
	autohold = value("hold") != NULL;
	holdbit = autohold ? MPRESERVE : MBOX;
	nohold = MBOX|MSAVED|MDELETED|MPRESERVE;
	if (value("keepsave") != NULL)
		nohold &= ~MSAVED;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
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
		if ((readstat = Fopen(Tflag, "w")) == NULL)
			Tflag = NULL;
	}
	for (c = 0, p = 0, mp = &message[0]; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MBOX)
			c++;
		if (mp->m_flag & MPRESERVE)
			p++;
		if (mp->m_flag & MODIFY)
			modify++;
		if (Tflag != NULL && (mp->m_flag & (MREAD|MDELETED)) != 0) {
			char *id;

			if ((id = hfield("article-id", mp)) != NULL)
				(void)fprintf(readstat, "%s\n", id);
		}
	}
	if (Tflag != NULL)
		(void)Fclose(readstat);
	if (p == msgCount && !modify && !anystat) {
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
	if (value("append") == NULL) {
		(void)snprintf(tempname, sizeof(tempname),
		    "%s/mail.RmXXXXXXXXXX", tmpdir);
		if ((fd = mkstemp(tempname)) == -1 ||
		    (obuf = Fdopen(fd, "w")) == NULL) {
			warn("%s", tempname);
			if (fd != -1)
				(void)close(fd);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		if ((ibuf = Fopen(tempname, "r")) == NULL) {
			warn("%s", tempname);
			(void)rm(tempname);
			(void)Fclose(obuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)rm(tempname);
		if ((abuf = Fopen(mbox, "r")) != NULL) {
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
		if ((obuf = Fopen(mbox, "r+")) == NULL) {
			warn("%s", mbox);
			(void)Fclose(ibuf);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
	}
	else {
		if ((obuf = Fopen(mbox, "a")) == NULL) {
			warn("%s", mbox);
			(void)Fclose(fbuf);
			dot_unlock(mailname);
			return;
		}
		(void)fchmod(fileno(obuf), 0600);
	}
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if (mp->m_flag & MBOX)
			if (sendmessage(mp, obuf, saveignore, NULL) < 0) {
				warn("%s", mbox);
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

	if (value("append") == NULL) {
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
		abuf = Fopen(mailname, "r+");
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

/*
 * Preserve all the appropriate messages back in the system
 * mailbox, and print a nice message indicated how many were
 * saved.  On any error, just return -1.  Else return 0.
 * Incorporate the any new mail that we found.
 */
int
writeback(FILE *res)
{
	struct message *mp;
	int p, c;
	FILE *obuf;

	p = 0;
	if ((obuf = Fopen(mailname, "r+")) == NULL) {
		warn("%s", mailname);
		return(-1);
	}
#ifndef APPEND
	if (res != NULL) {
		while ((c = getc(res)) != EOF)
			(void)putc(c, obuf);
		(void)fflush(obuf);
		if (ferror(obuf)) {
			warn("%s", mailname);
			(void)Fclose(obuf);
			return(-1);
		}
	}
#endif
	for (mp = &message[0]; mp < &message[msgCount]; mp++)
		if ((mp->m_flag&MPRESERVE)||(mp->m_flag&MTOUCH)==0) {
			p++;
			if (sendmessage(mp, obuf, NULL, NULL) < 0) {
				warn("%s", mailname);
				(void)Fclose(obuf);
				return(-1);
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
		return(-1);
	}
	if (res != NULL)
		(void)Fclose(res);
	(void)Fclose(obuf);
	alter(mailname);
	if (p == 1)
		(void)printf("Held 1 message in %s\n", mailname);
	else
		(void)printf("Held %d messages in %s\n", p, mailname);
	return(0);
}

/*
 * Terminate an editing session by attempting to write out the user's
 * file from the temporary.  Save any new stuff appended to the file.
 */
void
edstop(void)
{
	int gotcha, c;
	struct message *mp;
	FILE *obuf, *ibuf, *readstat = NULL;
	struct stat statb;
	char tempname[PATHSIZE];
	int fd;

	if (readonly)
		return;
	holdsigs();
	if (Tflag != NULL) {
		if ((readstat = Fopen(Tflag, "w")) == NULL)
			Tflag = NULL;
	}
	for (mp = &message[0], gotcha = 0; mp < &message[msgCount]; mp++) {
		if (mp->m_flag & MNEW) {
			mp->m_flag &= ~MNEW;
			mp->m_flag |= MSTATUS;
		}
		if (mp->m_flag & (MODIFY|MDELETED|MSTATUS))
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
		    (obuf = Fdopen(fd, "w")) == NULL) {
			warn("%s", tempname);
			if (fd != -1)
				(void)close(fd);
			relsesigs();
			reset(0);
		}
		if ((ibuf = Fopen(mailname, "r")) == NULL) {
			warn("%s", mailname);
			(void)Fclose(obuf);
			(void)rm(tempname);
			relsesigs();
			reset(0);
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
			relsesigs();
			reset(0);
		}
		(void)Fclose(ibuf);
		(void)Fclose(obuf);
		if ((ibuf = Fopen(tempname, "r")) == NULL) {
			warn("%s", tempname);
			(void)rm(tempname);
			relsesigs();
			reset(0);
		}
		(void)rm(tempname);
	}
	(void)printf("\"%s\" ", mailname);
	(void)fflush(stdout);
	if ((obuf = Fopen(mailname, "r+")) == NULL) {
		warn("%s", mailname);
		relsesigs();
		reset(0);
	}
	trunc(obuf);
	c = 0;
	for (mp = &message[0]; mp < &message[msgCount]; mp++) {
		if ((mp->m_flag & MDELETED) != 0)
			continue;
		c++;
		if (sendmessage(mp, obuf, NULL, NULL) < 0) {
			warn("%s", mailname);
			relsesigs();
			reset(0);
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
		relsesigs();
		reset(0);
	}
	(void)Fclose(obuf);
	if (gotcha) {
		(void)rm(mailname);
		(void)printf("removed\n");
	} else
		(void)printf("complete\n");
	(void)fflush(stdout);

done:
	relsesigs();
}
