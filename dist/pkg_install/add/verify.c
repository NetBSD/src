/* $NetBSD: verify.c,v 1.1.1.1 2007/07/16 13:01:46 joerg Exp $ */

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
#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif

#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1999 \
	        The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: verify.c,v 1.1.1.1 2007/07/16 13:01:46 joerg Exp $");
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "verify.h"
#include "lib.h"

enum {
	MaxExtensions = 10
};

/* this struct defines a verification type */ 
typedef struct ver_t {
	const char     *name;				/* name of type */
	const char     *command1;			/* command to execute to verify */
	const char     *command2;			/* command to execute to verify */
	const char     *extensions[MaxExtensions];	/* signature file extensions */
} ver_t;

static char	*verification_type;	/* the verification type which has been selected */

/* called when gpg verification type is selected */
static int
do_verify(const char *pkgname, const char *cmd1, const char *cmd2, const char *const *extensions)
{
	struct stat	st;
	const char    *const *ep;
	char		buf[BUFSIZ];
	char		f[MaxPathSize];
	int		ret;
	int		i;

	if (cmd1 == NULL) {
		return 1;
	}
	for (i = 0, ep = extensions ; i < MaxExtensions && *ep ; ep++, i++) {
		(void) snprintf(f, sizeof(f), "%s%s", pkgname, *ep);
		if (stat(f, &st) == 0) {
			(void) fprintf(stderr, "pkg_add: Using signature file: %s\n", f);
			ret = (cmd2 == NULL) ? fexec(cmd1, f, NULL) : fexec(cmd1, cmd2, f, NULL);
			if (ret != 0) {
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
static const ver_t	vertab[] = {
	{ "none",	NULL,	NULL,		{ NULL } },
	{ "gpg",	"gpg", "--verify",	{ ".sig", ".asc", NULL } },
	{ "pgp5",	"pgpv", NULL,		{ ".sig", ".asc", ".pgp", NULL } },
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
	const ver_t *vp;

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
	const ver_t *vp;

	if (verification_type == NULL) {
		return do_verify(pkg, NULL, NULL, NULL);
	}
	for (vp = vertab ; vp->name ; vp++) {
		if (strcasecmp(verification_type, vp->name) == 0) {
			return do_verify(pkg, vp->command1, vp->command2, vp->extensions);
		}
	}
	(void) fprintf(stderr, "Can't find `%s' verification details\n", verification_type);
	return 0;
}
