/*	$NetBSD: pt_file.c,v 1.12 2000/01/15 06:21:40 bgrayson Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *
 *	from: Id: pt_file.c,v 1.1 1992/05/25 21:43:09 jsp Exp
 *	@(#)pt_file.c	8.3 (Berkeley) 7/3/94
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pt_file.c,v 1.12 2000/01/15 06:21:40 bgrayson Exp $");
#endif /* not lint */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syslog.h>

#include "portald.h"

#ifdef DEBUG
#define DEBUG_SYSLOG	syslog
#else
/*
 * The "if (0) ..." will be optimized away by the compiler if
 * DEBUG is not defined.
 */
#define DEBUG_SYSLOG	if (0) syslog
#endif

int
lose_credentials(pcr)
	struct portal_cred	*pcr;
{
	/*
	 * If we are root, then switch into the caller's credentials.
	 * By the way, we _always_ log failures, to make
	 * sure questionable activity is noticed.
	 */
	if (getuid() == 0) {
		/* Set groups first ... */
		if (setgroups(pcr->pcr_ngroups, pcr->pcr_groups) < 0) {
			syslog(LOG_ERR,
			    "lose_credentials: setgroups() failed (%m)");
			return (errno);
		}
		/* ... then gid ... */
		if (setgid(pcr->pcr_gid) < 0) {
			syslog(LOG_ERR,
				"lose_credentials: setgid(%d) failed (%m)",
				pcr->pcr_gid);
		}
		/*
		 * ... and now do the setuid() where we lose all special
		 * powers (both real and effective userid).
		 */
		if (setuid(pcr->pcr_uid) < 0) {
			syslog(LOG_ERR,
				"lose_credentials: setuid(%d) failed (%m)",
				pcr->pcr_uid);
		}
		/* The credential change was successful! */
		DEBUG_SYSLOG(LOG_ERR, "Root-owned mount process lowered credentials -- returning successfully!\n");
		return 0;
	}
	DEBUG_SYSLOG(LOG_ERR, "Actual/effective/caller's creds are:");
	DEBUG_SYSLOG(LOG_ERR, "%d/%d %d/%d %d/%d", getuid(),
	    getgid(), geteuid(), getegid(), pcr->pcr_uid, pcr->pcr_gid);
	/*
	 * Else, fail if the uid is neither actual or effective
	 * uid of mount process...
	 */
	if ((getuid() != pcr->pcr_uid) && (geteuid() != pcr->pcr_uid)) {
		syslog(LOG_ERR, "lose_credentials: uid %d != uid %d, or euid %d != uid %d",
		    getuid(), pcr->pcr_uid, geteuid(), pcr->pcr_uid);
		return EPERM;
	}
	/*
	 * ... or the gid is neither the actual or effective
	 * gid of the mount process.
	 */
	if ((getgid() != pcr->pcr_gid) && (getegid() != pcr->pcr_gid)) {
		syslog(LOG_ERR, "lose_credentials: gid %d != gid %d, or egid %d != gid %d",
		    getgid(), pcr->pcr_gid, getegid(), pcr->pcr_gid);
		return EPERM;
	}
	/*
	 * If we make it here, we have a uid _and_ gid match! Allow the
	 * access.
	 */
	DEBUG_SYSLOG(LOG_ERR, "Returning successfully!\n");
	return 0;
}

int
portal_file(pcr, key, v, so, fdp)
	struct portal_cred *pcr;
	char   *key;
	char  **v;
	int     so;
	int    *fdp;
{
	int     fd;
	char    pbuf[MAXPATHLEN];
	int     error;
	int     origuid, origgid;

	origuid = getuid();
	origgid = getgid();
	pbuf[0] = '/';
	strcpy(pbuf + 1, key + (v[1] ? strlen(v[1]) : 0));
	DEBUG_SYSLOG(LOG_ERR, "path = %s, uid = %d, gid = %d",
	    pbuf, pcr->pcr_uid, pcr->pcr_gid);

	if ((error = lose_credentials(pcr)) != 0) {
		DEBUG_SYSLOG(LOG_ERR, "portal_file: Credential err %d", error);
		return error;
	}
	error = 0;
	/*
	 * Be careful -- only set error to errno if there is an error.
	 * errno could hold an old, uncaught value, from a routine
	 * called long before now.
	 */
	fd = open(pbuf, O_RDWR | O_CREAT, 0666);
	if (fd < 0) {
		error = errno;
		if (error == EACCES) {
			DEBUG_SYSLOG(LOG_ERR, "Error:  could not open '%s' "
			    "read/write with create flag.  "
			    "Trying read-only open...", pbuf);
			/* Try opening read-only. */
			fd = open(pbuf, O_RDONLY, 0);
			if (fd < 0) {
				error = errno;
				DEBUG_SYSLOG(LOG_ERR, "That failed too!  %m");
			} else {
				/* Clear the error indicator. */
				error = 0;
			}
		} else {
			DEBUG_SYSLOG(LOG_ERR, "Error:  could not open '%s': %m", pbuf);
		}

	}
	if (seteuid((uid_t) origuid) < 0) {	/* XXX - should reset gidset
						 * too */
		error = errno;
		syslog(LOG_ERR, "setcred: %m");
		if (fd >= 0) {
			(void) close(fd);
			fd = -1;
		}
	}
	if (error == 0)
		*fdp = fd;

	DEBUG_SYSLOG(LOG_ERR, "pt_file returns *fdp = %d, error = %d", *fdp,
	    error);
	return (error);
}
