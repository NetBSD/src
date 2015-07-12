/*	$NetBSD: gettext.c,v 1.3 2015/07/12 11:40:52 christos Exp $	*/

/*-
 * Copyright (c) 2015 William Orr <will@worrbase.com>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/cdefs.h>
__RCSID("$NetBSD: gettext.c,v 1.3 2015/07/12 11:40:52 christos Exp $");

#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <libintl.h>
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

static struct option longopts[] = {
	{ "help",       no_argument,        NULL,   'h' },
	{ "domain",	required_argument,  NULL,   'd' },
	{ NULL,         0,                  NULL,   '\0' },
};

static __dead void
usage(int exit_status)
{

	fprintf(stderr, "Usage: %s [-ehn] [[<textdomain>] <msgid>]\n",
	    getprogname());
	fprintf(stderr, "Usage: %s [-ehn] -d <textdomain> <msgid>\n",
	    getprogname());
	fprintf(stderr, "Usage: %s -s [<msgid>]...\n", getprogname());
	exit(exit_status);
}

static bool
expand(char *str)
{
	char *fp, *sp, ch, pl;
	bool nflag = false;

	for (fp = str, sp = str; *fp != 0;) {
		if (*fp == '\\') {
			switch (*++fp) {
			case 'a':
				*sp++ = '\a';
				fp++;
				break;
			case 'b':
				*sp++ = '\b';
				fp++;
				break;
			case 'c':
				nflag = true;
				fp++;
				break;
			case 'f':
				*sp++ = '\f';
				fp++;
				break;
			case 'n':
				*sp++ = '\n';
				fp++;
				break;
			case 'r':
				*sp++ = '\r';
				fp++;
				break;
			case 't':
				*sp++ = '\t';
				fp++;
				break;
			case 'v':
				*sp++ = '\v';
				fp++;
				break;
			case '\\':
				*sp++ = '\\';
				fp++;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				ch = *fp++ - '0';
				pl = 0;
				while (*fp >= '0' && *fp <= '7' && pl < 2) {
					ch *= 8;
					ch += *fp++ - '0';
					pl++;
				}

				*sp++ = ch;
				break;
			default:
				*sp++ = '\\';
				break;
			}
			continue;
		}
		*sp++ = *fp++;
	}

	*sp = '\0';
	return nflag;
}

int
main(int argc, char **argv)
{
	char *msgdomain = NULL;
	char *msgdomaindir = NULL;
	char *translation = NULL;
	char *s;
	bool eflag = false;
	bool sflag = false;
	bool nflag = false;
	int ch;

	setlocale(LC_ALL, "");
	setprogname(argv[0]);

	while ((ch = getopt_long(argc, argv, "d:eEhnsV", longopts, NULL)) != -1)
	{
		switch (ch) {
		case 'd':
			msgdomain = estrdup(optarg);
			break;
		case 'E':
			/* GNU gettext compat */
			break;
		case 'e':
			eflag = true;
			break;
		case 'V':
		case 'h':
			free(msgdomain);
			usage(EXIT_SUCCESS);
			/* NOTREACHED */
		case 'n':
			nflag = true;
			break;
		case 's':
			sflag = true;
			break;
		default:
			free(msgdomain);
			usage(EXIT_FAILURE);
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		free(msgdomain);
		warnx("missing msgid");
		usage(EXIT_FAILURE);
	}

	/* msgdomain can be passed as optional arg iff -s is not passed */
	if (!sflag) {
		if (argc == 2) {
			free(msgdomain);
			msgdomain = estrdup(argv[0]);

			argc -= 1;
			argv += 1;
		} else if (argc > 2) {
			warnx("too many arguments");
			usage(EXIT_FAILURE);
		}
	}

	/* msgdomain can be passed as env var */
	if (msgdomain == NULL) {
		if ((s = getenv("TEXTDOMAIN")) != NULL)
			msgdomain = estrdup(s);
	}

	if (msgdomain != NULL) {
		if ((s = getenv("TEXTDOMAINDIR")) != NULL)
			msgdomaindir = estrdup(s);
		if (msgdomaindir)
			bindtextdomain(msgdomain, msgdomaindir);
	}

	do {
		if (eflag)
			nflag |= expand(*argv);

		translation = dgettext(msgdomain, argv[0]);
		printf("%s", translation);

		argc--;
		argv++;
		if (argc)
			printf(" ");
	} while (sflag && argc != 0);

	if (sflag && !nflag)
		printf("\n");

	free(msgdomain);
	free(msgdomaindir);

	return EXIT_SUCCESS;
}
