/*	$NetBSD: util.c,v 1.4 1999/02/05 21:40:49 lukem Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Luke Mewburn.
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
#ifndef lint
__RCSID("$NetBSD: util.c,v 1.4 1999/02/05 21:40:49 lukem Exp $");
#endif /* not lint */

#include <sys/param.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#include "extern.h"

/*
 * logcmd --
 *	based on the arguments, syslog a message:
 *	 if bytes != -1		"<command> <file1> = <bytes> bytes"
 *	 else if file2 != NULL	"<command> <file1> <file2>"
 *	 else			"<command> <file1>"
 */

void
logcmd(command, bytes, file1, file2)
	const char	*command;
	off_t		 bytes;
	const char	*file1;
	const char	*file2;
{
	char buf[MAXPATHLEN + 50], realfile[MAXPATHLEN + 1];
	const char *p;

	if (logging <=1)
		return;

	if ((p = realpath(file1, realfile)) == NULL) {
		syslog(LOG_WARNING, "realpath failed: %s", realfile);
		p = file1;
	}
	snprintf(buf, sizeof(buf), "%s %s", command, p);

	if (bytes != (off_t)-1) {
		syslog(LOG_INFO, "%s = %qd bytes", buf, (long long) bytes);
	} else if (file2 != NULL) {
		if ((p = realpath(file2, realfile)) == NULL) {
			syslog(LOG_WARNING, "realpath failed: %s", realfile);
			p = file2;
		}
		syslog(LOG_INFO, "%s %s", buf, p);
	} else {
		syslog(LOG_INFO, "%s", buf);
	}
}

char *
xstrdup(s)
	const char *s;
{
	char *new = strdup(s);

	if (new == NULL)
		fatal("Local resource failure: malloc");
		/* NOTREACHED */
	return (new);
}
