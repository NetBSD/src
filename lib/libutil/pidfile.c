/*	$NetBSD: pidfile.c,v 1.1 1999/06/06 01:50:00 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__RCSID("$NetBSD: pidfile.c,v 1.1 1999/06/06 01:50:00 thorpej Exp $");
#endif

#include <sys/param.h>
#include <paths.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <util.h>

static char pidfile_path_store[MAXPATHLEN];
static char *pidfile_path;

static void pidfile_cleanup __P((void));

extern const char *__progname;		/* from crt0.o */

void
pidfile(basename)
	const char *basename;
{
	FILE *f;

	if (pidfile_path != NULL)
		return;

	if (basename == NULL)
		basename = __progname;

	/* _PATH_VARRUN includes trailing / */
	(void) snprintf(pidfile_path_store, sizeof(pidfile_path_store),
	    "%s%s.pid", _PATH_VARRUN, basename);

	if ((f = fopen(pidfile_path_store, "w")) == NULL)
		return;

	(void) fprintf(f, "%d\n", getpid());
	(void) fclose(f);

	pidfile_path = pidfile_path_store;

	(void) atexit(pidfile_cleanup);
}

static void
pidfile_cleanup()
{

	if (pidfile_path != NULL)
		(void) unlink(pidfile_path);
}
