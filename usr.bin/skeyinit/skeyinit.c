/*	$NetBSD: skeyinit.c,v 1.12 1998/12/19 22:18:00 christos Exp $	*/

/* S/KEY v1.1b (skeyinit.c)
 *
 * Authors:
 *          Neil M. Haller <nmh@thumper.bellcore.com>
 *          Philip R. Karn <karn@chicago.qualcomm.com>
 *          John S. Walden <jsw@thumper.bellcore.com>
 *          Scott Chasin <chasin@crimelab.com>
 *
 * S/KEY initialization and seed update
 */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <pwd.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>

#include "skey.h"

#define NAMELEN 2

int main __P((int, char **));

int
main(argc, argv)
	int     argc;
	char   *argv[];
{
	int     rval, n, nn, i, defaultsetup, l;
	time_t  now;
	char	hostname[MAXHOSTNAMELEN + 1];
	char    seed[18], tmp[80], key[8], defaultseed[17];
	char    passwd[256], passwd2[256], tbuf[27], buf[60];
	char    lastc, me[80], *p, *pw;
	const	char *salt;
	struct	skey skey;
	struct	passwd *pp;
	struct	tm *tm;

	time(&now);
	tm = localtime(&now);
	strftime(tbuf, sizeof(tbuf), "%M%j", tm);
	tbuf[sizeof(tbuf) - 1] = '\0';

	if (gethostname(hostname, sizeof(hostname)) < 0)
		err(1, "gethostname");
	hostname[sizeof(hostname) - 1] = '\0';
	strncpy(defaultseed, hostname, sizeof(defaultseed)- 1);
	defaultseed[4] = '\0';
	strncat(defaultseed, tbuf, sizeof(defaultseed) - 5);

	if ((pp = getpwuid(getuid())) == NULL)
		err(1, "no user with uid %ld", (u_long)getuid());
	(void)strncpy(me, pp->pw_name, sizeof(me) - 1);

	if ((pp = getpwnam(me)) == NULL)
		err(1, "Who are you?");

	defaultsetup = 1;
	if (argc > 1) {
		if (strcmp("-s", argv[1]) == 0)
			defaultsetup = 0;
		else
			pp = getpwnam(argv[1]);

		if (argc > 2)
			pp = getpwnam(argv[2]);
	}
	if (pp == NULL) {
		err(1, "User unknown");
	}
	if (strcmp(pp->pw_name, me) != 0) {
		if (getuid() != 0) {
			/* Only root can change other's passwds */
			printf("Permission denied.\n");
			exit(1);
		}
	}
	salt = pp->pw_passwd;

	setpriority(PRIO_PROCESS, 0, -4);

	if (getuid() != 0) {

		pw = getpass("Password:");
		p = crypt(pw, salt);

		setpriority(PRIO_PROCESS, 0, 0);

		if (pp && strcmp(p, pp->pw_passwd)) {
			printf("Password incorrect.\n");
			exit(1);
		}
	}
	rval = skeylookup(&skey, pp->pw_name);
	switch (rval) {
	case -1:
		err(1, "cannot open database");
	case 0:
		printf("[Updating %s]\n", pp->pw_name);
		printf("Old key: %s\n", skey.seed);

		/*
		 * lets be nice if they have a skey.seed that
		 * ends in 0-8 just add one
		 */
		l = strlen(skey.seed);
		if (l > 0) {
			lastc = skey.seed[l - 1];
			if (isdigit((unsigned char)lastc) && lastc != '9') {
				(void)strncpy(defaultseed, skey.seed,
				    sizeof(defaultseed) - 1);
				defaultseed[l - 1] = lastc + 1;
			}
			if (isdigit((unsigned char)lastc) && lastc == '9' &&
			    l < 16) {
				(void)strncpy(defaultseed, skey.seed,
				    sizeof(defaultseed) - 1);
				defaultseed[l - 1] = '0';
				defaultseed[l] = '0';
				defaultseed[l + 1] = '\0';
			}
		}
		break;
	case 1:
		printf("[Adding %s]\n", pp->pw_name);
		break;
	}
	n = 99;

	if (!defaultsetup) {
		printf("You need the 6 english words generated from the \"key\" command.\n");
		for (i = 0;; i++) {
			if (i >= 2)
				exit(1);
			printf("Enter sequence count from 1 to 10000: ");
			fgets(tmp, sizeof(tmp), stdin);
			n = atoi(tmp);
			if (n > 0 && n < 10000)
				break;	/* Valid range */
			printf("\n Error: Count must be > 0 and < 10000\n");
		}
	}
	if (!defaultsetup) {
		printf("Enter new key [default %s]: ", defaultseed);
		fflush(stdout);
		fgets(seed, sizeof(seed), stdin);
		rip(seed);
		if (strlen(seed) > 16) {
			printf("Notice: Seed truncated to 16 characters.\n");
			seed[16] = '\0';
		}
		if (seed[0] == '\0')
			(void)strncpy(seed, defaultseed, sizeof(seed) - 1);

		for (i = 0;; i++) {
			if (i >= 2)
				exit(1);

			printf("s/key %d %s\ns/key access password: ", n, seed);
			fgets(tmp, sizeof(tmp), stdin);
			rip(tmp);
			backspace(tmp);

			if (tmp[0] == '?') {
				printf("Enter 6 English words from secure S/Key calculation.\n");
				continue;
			}
			if (tmp[0] == '\0') {
				exit(1);
			}
			if (etob(key, tmp) == 1 || atob8(key, tmp) == 0)
				break;	/* Valid format */
			printf("Invalid format - try again with 6 English words.\n");
		}
	} else {
		/* Get user's secret password */
		for (i = 0;; i++) {
			if (i >= 2)
				exit(1);

			printf("Enter secret password: ");
			readpass(passwd, sizeof(passwd));
			if (passwd[0] == '\0')
				exit(1);

			printf("Again secret password: ");
			readpass(passwd2, sizeof(passwd));
			if (passwd2[0] == '\0')
				exit(1);

			if (strlen(passwd) < 4 && strlen(passwd2) < 4)
				err(1, "Your password must be longer");
			if (strcmp(passwd, passwd2) == 0)
				break;

			printf("Passwords do not match.\n");
		}
		(void)strncpy(seed, defaultseed, sizeof(seed) - 1);

		/* Crunch seed and password into starting key */
		if (keycrunch(key, seed, passwd) != 0)
			err(2, "key crunch failed");
		nn = n;
		while (nn-- != 0)
			f(key);
	}
	time(&now);
	tm = localtime(&now);
	strftime(tbuf, sizeof(tbuf), " %b %d,%Y %T", tm);

	skey.val = (char *)malloc(16 + 1);

	btoa8(skey.val, key);

	fprintf(skey.keyfile, "%s %04d %-16s %s %-21s\n", pp->pw_name, n,
	    seed, skey.val, tbuf);
	fclose(skey.keyfile);
	printf("ID %s s/key is %d %s\n", pp->pw_name, n, seed);
	printf("Next login password: %s\n", btoe(buf, key));
#ifdef HEXIN
	printf("%s\n", put8(buf, key));
#endif

	exit(1);
}
