/*	$NetBSD: utmpx.c,v 1.3 2002/03/05 16:16:02 christos Exp $	 */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: utmpx.c,v 1.3 2002/03/05 16:16:02 christos Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>
#include <vis.h>
#include <utmpx.h>
#include <unistd.h>
#include <fcntl.h>

static FILE *fp;
static struct utmpx ut;
static char utfile[MAXPATHLEN] = _PATH_UTMPX;

static struct utmpx *utmp_update(const struct utmpx *);

static const char vers[] = "utmpx-1.00";

void
setutxent()
{
	(void)memset(&ut, 0, sizeof(ut));
	if (fp == NULL)
		return;
	(void)fseeko(fp, (off_t)sizeof(ut), SEEK_SET);
}


void
endutxent()
{
	(void)memset(&ut, 0, sizeof(ut));
	if (fp != NULL)
		(void)fclose(fp);
}


struct utmpx *
getutxent()
{
	if (fp == NULL) {
		struct stat st;

		if ((fp = fopen(utfile, "r+")) == NULL)
			if ((fp = fopen(utfile, "r")) == NULL)
				goto fail;

		/* get file size in order to check if new file */
		if (fstat(fileno(fp), &st) == -1)
			goto failclose;

		if (st.st_size == 0) {
			/* new file, add signature record */
			(void)memset(&ut, 0, sizeof(ut));
			ut.ut_type = SIGNATURE;
			(void)memcpy(ut.ut_user, vers, sizeof(vers));
			if (fwrite(&ut, sizeof(ut), 1, fp) != sizeof(ut))
				goto failclose;
		} else {
			/* old file, read signature record */
			if (fread(&ut, sizeof(ut), 1, fp) != sizeof(ut))
				goto failclose;
			if (memcmp(ut.ut_user, vers, sizeof(vers)) != 0 ||
			    ut.ut_type != SIGNATURE)
				goto failclose;
		}
	}

	if (fread(&ut, sizeof(ut), 1, fp) != sizeof(ut))
		goto fail;

	return &ut;
failclose:
	(void)fclose(fp);
fail:
	(void)memset(&ut, 0, sizeof(ut));
	return NULL;
}


struct utmpx *
getutxid(const struct utmpx *utx)
{
	if (utx->ut_type == EMPTY)
		return NULL;

	do {
		if (ut.ut_type == EMPTY)
			continue;
		switch (utx->ut_type) {
		case EMPTY:
			return NULL;
		case RUN_LVL:
		case BOOT_TIME:
		case OLD_TIME:
		case NEW_TIME:
			if (ut.ut_type == utx->ut_type)
				return &ut;
			break;
		case INIT_PROCESS:
		case LOGIN_PROCESS:
		case USER_PROCESS:
		case DEAD_PROCESS:
			switch (ut.ut_type) {
			case INIT_PROCESS:
			case LOGIN_PROCESS:
			case USER_PROCESS:
			case DEAD_PROCESS:
				if (memcmp(ut.ut_id, utx->ut_id,
				    sizeof(ut.ut_id)) == 0)
					return &ut;
				break;
			default:
				break;
			}
			break;
		default:
			return NULL;
		}
	}
	while (getutxent() != NULL);
	return NULL;
}


struct utmpx *
getutxline(const struct utmpx *utx)
{
	do {
		switch (ut.ut_type) {
		case EMPTY:
			break;
		case LOGIN_PROCESS:
		case USER_PROCESS:
			if (strncmp(ut.ut_line, utx->ut_line,
			    sizeof(ut.ut_line)) == 0)
				return &ut;
			break;
		default:
			break;
		}
	}
	while (getutxent() != NULL);
	return NULL;
}


struct utmpx *
pututxline(const struct utmpx *utx)
{
	struct utmpx temp, *u = NULL;
	int gotlock = 0;

	if (strcmp(_PATH_UTMPX, utfile) == 0 && geteuid() != 0)
		return utmp_update(utx);

	if (utx == NULL)
		return NULL;

	(void)memcpy(&temp, utx, sizeof(temp));

	if (fp == NULL) {
		(void)getutxent();
		if (fp == NULL)
			return NULL;
	}

	if (getutxid(&temp) == NULL) {
		setutxent();
		if (getutxid(&temp) == NULL) {
			if (lockf(fileno(fp), F_LOCK, (off_t)0) == -1)
				return NULL;
			gotlock++;
			if (fseeko(fp, (off_t)0, SEEK_END) == -1)
				goto fail;
		}
	}

	if (!gotlock) {
		/* we are not appending */
		if (fseeko(fp, -(off_t)sizeof(ut), SEEK_CUR) == -1)
			return NULL;
	}

	if (fwrite(&temp, sizeof (temp), 1, fp) != 1)
		goto fail;

	if (fflush(fp) == -1)
		goto fail;

	u = memcpy(&ut, &temp, sizeof(ut));
fail:
	if (gotlock) {
		if (lockf(fileno(fp), F_ULOCK, (off_t)0) == -1)
			return NULL;
	}
	return u;
}


static struct utmpx *
utmp_update(const struct utmpx *utx)
{
	char buf[sizeof(*utx) * 4 + 1];
	pid_t pid;
	int status;

	(void)strvisx(buf, (const char *)(const void *)utx, sizeof(*utx),
	    VIS_WHITE);
	switch (pid = fork()) {
	case 0:
		(void)execl(_PATH_UTMP_UPDATE,
		    strrchr(_PATH_UTMP_UPDATE, '/') + 1, buf);
		exit(1);
		/*NOTREACHED*/
	case -1:
		return NULL;
	default:
		if (waitpid(pid, &status, 0) == -1)
			return NULL;
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
			return memcpy(&ut, utx, sizeof(ut));
		return NULL;
	}

}

void
updwtmpx(const char *file, const struct utmpx *utx)
{
	int fd = open(file, O_WRONLY | O_APPEND);
	if (fd == -1) {
		if ((fd = open(file, O_CREAT | O_WRONLY, 0644)) == -1)
			return;
		(void)memset(&ut, 0, sizeof(ut));
		ut.ut_type = SIGNATURE;
		(void)memcpy(ut.ut_user, vers, sizeof(vers));
		(void)write(fd, &ut, sizeof(ut));
	}
	(void)write(fd, utx, sizeof(*utx));
	(void)close(fd);
}


/*
 * The following are extensions and not part of the X/Open spec
 */
int
utmpxname(const char *fname)
{
	size_t len = strlen(fname);

	if (len >= sizeof(utfile))
		return 0;

	/* must end in x! */
	if (fname[len - 1] != 'x')
		return 0;

	(void)strcpy(utfile, fname);
	endutxent();
	return 1;
}
