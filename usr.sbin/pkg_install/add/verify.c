/* $NetBSD: verify.c,v 1.1 2001/09/25 10:28:16 agc Exp $ */

/*
 * Copyright (c) 2001 Alistair G. Crooks.  All rights reserved.
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
 *	This product includes software developed by Alistair G. Crooks.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1999 \
	        The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: verify.c,v 1.1 2001/09/25 10:28:16 agc Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "verify.h"
#include "lib.h"

enum {
	MaxExtensions = 10
};

/* this struct defines a verification type */ 
typedef struct ver_t {
	const char     *name;				/* name of type */
	const char     *command;			/* command to execute to verify */
	const char     *extensions[MaxExtensions];	/* signature file extensions */
} ver_t;

static char	*verification_type;	/* the verification type which has been selected */

/* called when gpg verification type is selected */
static int
do_verify(const char *pkgname, const char *cmd, const char **extensions)
{
	struct stat	st;
	const char    **ep;
	char		buf[BUFSIZ];
	char		f[FILENAME_MAX];
	int		i;

	if (cmd == NULL) {
		return 1;
	}
	for (i = 0, ep = extensions ; i < MaxExtensions && *ep ; ep++, i++) {
		(void) snprintf(f, sizeof(f), "%s%s", pkgname, *ep);
		if (stat(f, &st) == 0) {
			if (vsystem(cmd, f) != 0) {
				(void) fprintf(stderr, "*** WARNING ***: `%s' has a bad signature\n", f);
				return 0;
			}
			(void) fprintf(stderr, "Proceed with addition of %s: [y/n]? ", pkgname);
			if (fgets(buf, sizeof(buf), stdin) == NULL) {
				(void) fprintf(stderr, "Exiting now...");
				exit(EXIT_FAILURE);
			}
			switch(buf[0]) {
			case 'Y':
			case 'y':
			case '1':
				return 1;
			}
			(void) fprintf(stderr, "Package `%s' will not be added\n", pkgname);
			return 0;
		}
	}
	(void) fprintf(stderr, "No valid signature file found for `%s'\n", pkgname);
	return 0;
}

/* table holding possible verifications which can be made */
static ver_t	vertab[] = {
	{ "none",	NULL,			{ NULL } },
	{ "gpg",	"gpg --verify %s",	{ ".sig", ".asc", NULL } },
	{ "pgp5",	"pgpv %s",		{ ".sig", ".asc", ".pgp", NULL } },
	{ NULL }
};

/* set the verification type - usually called during command line processing */
void
set_verification(const char *type)
{
	if (verification_type) {
		(void) free(verification_type);
	}
	verification_type = strdup(type);
}

/* return the type of verification that is being used */
char *
get_verification(void)
{
	ver_t  *vp;

	if (verification_type != NULL) {
		for (vp = vertab ; vp->name ; vp++) {
			if (strcasecmp(verification_type, vp->name) == 0) {
				return verification_type;
			}
		}
	}
	return "none";
}

/* verify the digital signature (if any) on a package */
int
verify(const char *pkg)
{
	ver_t   *vp;

	if (verification_type == NULL) {
		return do_verify(pkg, NULL, NULL);
	}
	for (vp = vertab ; vp->name ; vp++) {
		if (strcasecmp(verification_type, vp->name) == 0) {
			return do_verify(pkg, vp->command, vp->extensions);
		}
	}
	(void) fprintf(stderr, "Can't find `%s' verification details\n", verification_type);
	return 0;
}
