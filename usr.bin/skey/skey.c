/*	$NetBSD: skey.c,v 1.9.4.1 2000/07/17 19:55:53 mjl Exp $	*/

/*
 * S/KEY v1.1b (skey.c)
 *
 * Authors:
 *          Neil M. Haller <nmh@thumper.bellcore.com>
 *          Philip R. Karn <karn@chicago.qualcomm.com>
 *          John S. Walden <jsw@thumper.bellcore.com>
 *          Scott Chasin <chasin@crimelab.com>
 *
 *
 * Stand-alone program for computing responses to S/Key challenges.
 * Takes the iteration count and seed as command line args, prompts
 * for the user's key, and produces both word and hex format responses.
 *
 * Usage example:
 *	>skey 88 ka9q2
 *	Enter password:
 *	OMEN US HORN OMIT BACK AHOY
 *	>
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: skey.c,v 1.9.4.1 2000/07/17 19:55:53 mjl Exp $");
#endif

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <skey.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	main __P((int, char **));
void    usage __P((char *));

int
main(int	argc, char **argv)
{
	int     n, cnt, i, pass = 0, hexmode = 0;
	char    passwd[SKEY_MAX_PW_LEN+1], key[SKEY_BINKEY_SIZE];
	char	buf[33], *seed, *slash, *t;

	cnt = 1;

	while ((i = getopt(argc, argv, "n:p:t:x")) != -1) {
		switch (i) {
		case 'n':
			cnt = atoi(optarg);
			break;
		case 'p':
			if (strlcpy(passwd, argv[++i], sizeof(passwd)) >=
							sizeof(passwd))
				errx(1, "Password too long");
			pass = 1;
			break;
		case 't':
			if (skey_set_algorithm(optarg) == NULL) {
				errx(1, "Unknown hash algorithm %s", optarg);
                        }
			break;
		case 'x':
			hexmode = 1;
			break;
		default:
			usage(argv[0]);
			break;		
		}
	}

	/* could be in the form <number>/<seed> */
	if (argc <= optind + 1) {
		/* look for / in it */
		if (argc <= optind)
			usage(argv[0]);
		slash = strchr(argv[optind], '/');
		if (slash == NULL)
			usage(argv[0]);
		*slash++ = '\0';
		seed = slash;

		if ((n = atoi(argv[optind])) < 0) {
			fprintf(stderr, "%s not positive\n", argv[optind]);
			usage(argv[0]);
                } else if (n > SKEY_MAX_SEQ) {
			warnx("%d is larger than max (%d)", n, SKEY_MAX_SEQ);
			usage(argv[0]);
		}
	} else {

		if ((n = atoi(argv[optind])) < 0) {
			fprintf(stderr, "%s not positive\n", argv[optind]);
			usage(argv[0]);
		} else if (n > SKEY_MAX_SEQ) {
			warnx("%d is larger than max (%d)", n, SKEY_MAX_SEQ);
			usage(argv[0]);
		}
		seed = argv[++optind];
	}

	for(t = seed; *t; t++) {
		if(!isalnum(*t))
			errx(1, "seed must be alphanumeric");
	}

	if(!*seed || strlen(seed) > SKEY_MAX_SEED_LEN)
		errx(1, "seed must be between 1 and %d long", SKEY_MAX_SEED_LEN);

	/* Get user's secret password */
	if (!pass) {
		(void)fputs("Reminder - Do not use this program while "
			    "logged in via telnet or rlogin.\n", stderr);
		fprintf(stderr, "Enter secret password: ");
		readpass(passwd, sizeof(passwd));
		if (passwd[0] == '\0') 
			exit(1);
	}

	if(strlen(passwd) < SKEY_MIN_PW_LEN)
		errx(1, "password must be at least %d long", SKEY_MIN_PW_LEN);

	/* Crunch seed and password into starting key */
	if (keycrunch(key, seed, passwd) != 0)
		errx(1, "key crunch failed");

	if (cnt == 1) {
		while (n-- != 0)
			f(key);
		(void)puts(hexmode ? put8(buf, key) : btoe(buf, key));
	} else {
		for (i = 0; i <= n - cnt; i++)
			f(key);
		for (; i <= n; i++) {
			(void)printf("%3d: %-29s", i, btoe(buf, key));
			if (hexmode)
				(void)printf("\t%s", put8(buf, key));
			puts("");
			f(key);
		}
	}
	exit(0);
}

void
usage(char *s)
{

	fprintf(stderr,
	    "Usage: %s [-n count] [-p password ] [-t hash] [-x] sequence#"
	    	"[/] key\n", s);
	exit(1);
}
