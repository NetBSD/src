/*	$KAME: safefile.c,v 1.5 2001/03/05 19:54:06 thorpej Exp $	*/

/*
 * Copyright (C) 2000 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: safefile.c,v 1.2 2003/07/12 09:37:12 itojun Exp $");

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "plog.h"
#include "debug.h"
#include "misc.h"
#include "safefile.h"

int
safefile(path, secret)
	const char *path;
	int secret;
{
	struct stat s;
	uid_t me;

	/* no setuid */
	if (getuid() != geteuid()) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "setuid'ed execution not allowed\n");
		return -1;
	}

	if (stat(path, &s) != 0)
		return -1;

	/* the file must be owned by the running uid */
	me = getuid();
	if (s.st_uid != me) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "%s has invalid owner uid\n", path);
		return -1;
	}

	switch (s.st_mode & S_IFMT) {
	case S_IFREG:
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
		    "%s is an invalid file type 0x%x\n", path,
		    (s.st_mode & S_IFMT));
		return -1;
	}

	/* secret file should not be read by others */
	if (secret) {
		if ((s.st_mode & S_IRWXG) != 0 || (s.st_mode & S_IRWXO) != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "%s has weak file permission\n", path);
			return -1;
		}
	}

	return 0;
}
