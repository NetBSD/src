/*	$NetBSD: passwd.c,v 1.22 2001/03/28 03:17:42 simonb Exp $	*/

/*
 * Copyright (c) 1988, 1993, 1994
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
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "from: @(#)passwd.c    8.3 (Berkeley) 4/2/94";
#else
__RCSID("$NetBSD: passwd.c,v 1.22 2001/03/28 03:17:42 simonb Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>

#include "extern.h"

static struct pw_module_s {
	const char *argv0;
	const char *args;
	const char *usage;
	int (*pw_init) __P((const char *));
	int (*pw_arg) __P((char, const char *));
	int (*pw_arg_end) __P((void));
	void (*pw_end) __P((void));

	int (*pw_chpw) __P((const char*));
	int invalid;
#define	INIT_INVALID 1
#define ARG_INVALID 2
	int use_class;
} pw_modules[] = {
#ifdef KERBEROS5
	{ NULL, "5ku:", "[-5] [-k] [-u principal]",
	    krb5_init, krb5_arg, krb5_arg_end, krb5_end, krb5_chpw, 0, 0 },
	{ "kpasswd", "5ku:", "[-5] [-k] [-u principal]",
	    krb5_init, krb5_arg, krb5_arg_end, krb5_end, krb5_chpw, 0, 0 },
#endif
#ifdef KERBEROS
	{ NULL, "4ku:i:r:", "[-4] [-k] [-u user] [-i instance] [-r realm]",
	    krb4_init, krb4_arg, krb4_arg_end, krb4_end, krb4_chpw, 0, 0 },
	{ "kpasswd", "4ku:i:r:", "[-4] [-k] [-u user] [-i instance] [-r realm]",
	    krb4_init, krb4_arg, krb4_arg_end, krb4_end, krb4_chpw, 0, 0 },
#endif
#ifdef YP
	{ NULL, "y", "[-y]",
	    yp_init, yp_arg, yp_arg_end, yp_end, yp_chpw, 0, 0 },
	{ "yppasswd", "", "[-y]",
	    yp_init, yp_arg, yp_arg_end, yp_end, yp_chpw, 0, 0 },
#endif
	/* local */
	{ NULL, "l", "[-l]",
	    local_init, local_arg, local_arg_end, local_end, local_chpw, 0, 0 },

	/* terminator */
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};
 
void	usage __P((void)); 

int	main __P((int, char **));

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	char *username;
	char optstring[64];  /* if we ever get more than 64 args, shoot me. */
	const char *curopt, *optopt;
	int i, j;
	int valid;
	int use_always;

	/* allow passwd modules to do argv[0] specific processing */
	use_always = 0;
	valid = 0;
	for (i = 0; pw_modules[i].pw_init != NULL; i++) {
		pw_modules[i].invalid = 0;
		if (pw_modules[i].argv0) {
			/*
			 * If we have a module that matches this progname, be
			 * sure that no modules but those that match this
			 * progname can be used.  If we have a module that
			 * matches against a particular progname, but does NOT
			 * match this one, don't use that module.
			 */
			if ((strcmp(getprogname(), pw_modules[i].argv0) == 0) &&
			    use_always == 0) {
				for (j = 0; j < i; j++) {
					pw_modules[j].invalid |= INIT_INVALID;
					(*pw_modules[j].pw_end)();
				}
				use_always = 1;
			} else if (use_always == 0)
				pw_modules[i].invalid |= INIT_INVALID;
		} else if (use_always)
			pw_modules[i].invalid |= INIT_INVALID;

		if (pw_modules[i].invalid)
			continue;

		pw_modules[i].invalid |=
		    (*pw_modules[i].pw_init)(getprogname()) ?
		    /* zero on success, non-zero on error */
		    INIT_INVALID : 0;

		if (! pw_modules[i].invalid)
			valid = 1;
	}

	if (valid == 0)
		errx(1, "Can't change password.");

	/* Build the option string from the individual modules' option
	 * strings.  Note that two modules can share a single option
	 * letter. */
	optstring[0] = '\0';
	j = 0;
	for (i = 0; pw_modules[i].pw_init != NULL; i++) {
		if (pw_modules[i].invalid)
			continue;

		curopt = pw_modules[i].args;
		while (*curopt != '\0') {
			if ((optopt = strchr(optstring, *curopt)) == NULL) {
				optstring[j++] = *curopt;
				if (curopt[1] == ':') {
					curopt++;
					optstring[j++] = *curopt;
				}
				optstring[j] = '\0';
			} else if ((optopt[1] == ':' && curopt[1] != ':') ||
			    (optopt[1] != ':' && curopt[1] == ':')) {
				errx(1, "NetBSD ERROR!  Different password "
				    "modules have two different ideas about "
				    "%c argument format.", curopt[0]);
			}
			curopt++;
		}
	}

	while ((ch = getopt(argc, argv, optstring)) != -1)
	{
		valid = 0;
		for (i = 0; pw_modules[i].pw_init != NULL; i++) {
			if (pw_modules[i].invalid)
				continue;
			if ((optopt = strchr(pw_modules[i].args, ch)) != NULL) {
				j = (optopt[1] == ':') ?
				    ! (*pw_modules[i].pw_arg)(ch, optarg) :
				    ! (*pw_modules[i].pw_arg)(ch, NULL);
				if (j != 0)
					pw_modules[i].invalid |= ARG_INVALID;
				if (pw_modules[i].invalid)
					(*pw_modules[i].pw_end)();
			} else {
				/* arg doesn't match this module */
				pw_modules[i].invalid |= ARG_INVALID;
				(*pw_modules[i].pw_end)();
			}
			if (! pw_modules[i].invalid)
				valid = 1;
		}
		if (! valid) {
			usage();
			exit(1);
		}
	}

	/* select which module to use to actually change the password. */
	use_always = 0;
	valid = 0;
	for (i = 0; pw_modules[i].pw_init != NULL; i++)
		if (! pw_modules[i].invalid) {
			pw_modules[i].use_class = (*pw_modules[i].pw_arg_end)();
			if (pw_modules[i].use_class != PW_DONT_USE)
				valid = 1;
			if (pw_modules[i].use_class == PW_USE_FORCE)
				use_always = 1;
		}


	if (! valid)
		/* hang the DJ */
		errx(1, "No valid password module specified.");

	argc -= optind;
	argv += optind;

	username = getlogin();
	if (username == NULL)
		errx(1, "who are you ??");
	
	switch(argc) {
	case 0:
		break;
	case 1:
		username = argv[0];
		break;
	default:
		usage();
		exit(1);
	}

	/* allow for fallback to other chpw() methods. */
	for (i = 0; pw_modules[i].pw_init != NULL; i++) {
		if (pw_modules[i].invalid)
			continue;
		if ((use_always && pw_modules[i].use_class == PW_USE_FORCE) ||
		    (!use_always && pw_modules[i].use_class == PW_USE)) {
			valid = (*pw_modules[i].pw_chpw)(username);
			(*pw_modules[i].pw_end)();
			if (valid >= 0)
				exit(valid);
			/* return value < 0 indicates continuation. */
		}
	}
	exit(1);
}

void
usage()
{
	int i;

	fprintf(stderr, "usage:\n");
	for (i = 0; pw_modules[i].pw_init != NULL; i++)
		if (! (pw_modules[i].invalid & INIT_INVALID))
			fprintf(stderr, "\t%s %s [user]\n", getprogname(),
			    pw_modules[i].usage);
	exit(1);
}
