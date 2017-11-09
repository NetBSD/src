/*	$NetBSD: edit.c,v 1.29 2017/11/09 20:27:50 christos Exp $	*/

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
static char sccsid[] = "@(#)edit.c	8.1 (Berkeley) 6/6/93";
#else
__RCSID("$NetBSD: edit.c,v 1.29 2017/11/09 20:27:50 christos Exp $");
#endif
#endif /* not lint */

#include "rcv.h"
#include "extern.h"
#include "thread.h"
#include "sig.h"

/*
 * Mail -- a mail program
 *
 * Perform message editing functions.
 */

/*
 * Run an editor on the file at "fpp" of "size" bytes,
 * and return a new file pointer.
 * Signals must be handled by the caller.
 * "Editortype" is 'e' for _PATH_EX, 'v' for _PATH_VI.
 */
PUBLIC FILE *
run_editor(FILE *fp, off_t size, int editortype, int readonlyflag)
{
	FILE *nf = NULL;
	int t;
	time_t modtime;
	const char *editcmd;
	char tempname[PATHSIZE];
	struct stat statb;

	(void)snprintf(tempname, sizeof(tempname),
	    "%s/mail.ReXXXXXXXXXX", tmpdir);
	if ((t = mkstemp(tempname)) == -1) {
		warn("%s", tempname);
		goto out;
	}
	if (readonlyflag && fchmod(t, 0400) == -1) {
		(void)close(t);
		warn("%s", tempname);
		(void)unlink(tempname);
		goto out;
	}
	if ((nf = Fdopen(t, "wef")) == NULL) {
		(void)close(t);
		warn("%s", tempname);
		(void)unlink(tempname);
		goto out;
	}
	if (size >= 0)
		while (--size >= 0 && (t = getc(fp)) != EOF)
			(void)putc(t, nf);
	else
		while ((t = getc(fp)) != EOF)
			(void)putc(t, nf);
	(void)fflush(nf);
	if (ferror(nf)) {
		(void)Fclose(nf);
		warn("%s", tempname);
		(void)unlink(tempname);
		nf = NULL;
		goto out;
	}
	if (fstat(fileno(nf), &statb) < 0)
		modtime = 0;
	else
		modtime = statb.st_mtime;
	if (Fclose(nf) < 0) {
		warn("%s", tempname);
		(void)unlink(tempname);
		nf = NULL;
		goto out;
	}
	nf = NULL;
	editcmd = value(editortype == 'e' ? ENAME_EDITOR : ENAME_VISUAL);
	if (editcmd == NULL)
		editcmd = editortype == 'e' ? _PATH_EX : _PATH_VI;
	if (run_command(editcmd, NULL, 0, -1, tempname, NULL) < 0) {
		(void)unlink(tempname);
		goto out;
	}
	/*
	 * If in read only mode or file unchanged, just remove the editor
	 * temporary and return.
	 */
	if (readonlyflag) {
		(void)unlink(tempname);
		goto out;
	}
	if (stat(tempname, &statb) < 0) {
		warn("%s", tempname);
		goto out;
	}
	if (modtime == statb.st_mtime) {
		(void)unlink(tempname);
		goto out;
	}
	/*
	 * Now switch to new file.
	 */
	if ((nf = Fopen(tempname, "aef+")) == NULL) {
		warn("%s", tempname);
		(void)unlink(tempname);
		goto out;
	}
	(void)unlink(tempname);
out:
	return nf;
}

/*
 * Edit a message by writing the message into a funnily-named file
 * (which should not exist) and forking an editor on it.
 * We get the editor from the stuff above.
 */
static int
edit1(int *msgvec, int editortype)
{
	int c;
	int i;
	int msgCount;
	FILE *fp;
	struct message *mp;
	off_t size;

	/*
	 * Deal with each message to be edited . . .
	 */
	msgCount = get_msgCount();
	for (i = 0; i < msgCount && msgvec[i]; i++) {
		sigset_t oset;
		struct sigaction osa;

		if (i > 0) {
			char buf[100];
			char *p;

			(void)printf("Edit message %d [ynq]? ", msgvec[i]);
			if (fgets(buf, (int)sizeof(buf), stdin) == NULL)
				break;
			p = skip_WSP(buf);
			if (*p == 'q')
				break;
			if (*p == 'n')
				continue;
		}
		dot = mp = get_message(msgvec[i]);
		touch(mp);
		sig_check();
		(void)sig_ignore(SIGINT, &osa, &oset);
		fp = run_editor(setinput(mp), mp->m_size, editortype,
				readonly);
		if (fp != NULL) {
			(void)fseek(otf, 0L, 2);
			size = ftell(otf);
			mp->m_block = blockof(size);
			mp->m_offset = blkoffsetof(size);
			mp->m_size = fsize(fp);
			mp->m_lines = 0;
			mp->m_blines = 0;
			mp->m_flag |= MMODIFY;
			rewind(fp);
			while ((c = getc(fp)) != EOF) {
				/*
				 * XXX. if you edit a message, we treat
				 * header lines as body lines in the recount.
				 * This is because the original message copy
				 * and the edit reread use different code to
				 * count the lines, and this one here is
				 * simple-minded.
				 */
				if (c == '\n') {
					mp->m_lines++;
					mp->m_blines++;
				}
				if (putc(c, otf) == EOF)
					break;
			}
			if (ferror(otf))
				warn("/tmp");
			(void)Fclose(fp);
		}
		(void)sig_restore(SIGINT, &osa, &oset);
		sig_check();
	}
	return 0;
}

/*
 * Edit a message list.
 */
PUBLIC int
editor(void *v)
{
	int *msgvec = v;

	return edit1(msgvec, 'e');
}

/*
 * Invoke the visual editor on a message list.
 */
PUBLIC int
visual(void *v)
{
	int *msgvec = v;

	return edit1(msgvec, 'v');
}
