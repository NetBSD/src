/*	$NetBSD: skeyinit.c,v 1.28.10.1 2009/05/13 19:20:05 jym Exp $	*/

/* S/KEY v1.1b (skeyinit.c)
 *
 * Authors:
 *          Neil M. Haller <nmh@thumper.bellcore.com>
 *          Philip R. Karn <karn@chicago.qualcomm.com>
 *          John S. Walden <jsw@thumper.bellcore.com>
 *          Scott Chasin <chasin@crimelab.com>
 *
 * Modifications:
 *          Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * S/KEY initialization and seed update
 */

#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: skeyinit.c,v 1.28.10.1 2009/05/13 19:20:05 jym Exp $");
#endif

#include <sys/param.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <skey.h>

#ifndef SKEY_NAMELEN
#define SKEY_NAMELEN    4
#endif

int main(int argc, char **argv)
{
	int     rval, nn, i;
	size_t	l;
	int n = 0, defaultsetup = 1, zerokey = 0, hexmode = 0;
	int	argpass = 0, argkey = 0;
	time_t  now;
	char	hostname[MAXHOSTNAMELEN + 1];
	char    seed[SKEY_MAX_PW_LEN+2], key[SKEY_BINKEY_SIZE], defaultseed[SKEY_MAX_SEED_LEN+1];
	char    passwd[SKEY_MAX_PW_LEN+2], passwd2[SKEY_MAX_PW_LEN+2], tbuf[27], buf[80];
	char    lastc, me[LOGIN_NAME_MAX+1], *p, *pw, *ht = NULL;
	const	char *salt;
	struct	skey skey;
	struct	passwd *pp;
	struct	tm *tm;
	int c;

	pw = NULL;	/* XXX gcc -Wuninitialized [sh3] */

	/*
	 * Make sure using stdin/stdout/stderr is safe
	 * after opening any file.
	 */
	i = open(_PATH_DEVNULL, O_RDWR);
	while (i >= 0 && i < 2)
		i = dup(i);
	if (i > 2)
		close(i);

	if (geteuid() != 0)
		errx(1, "must be setuid root.");

	if (gethostname(hostname, sizeof(hostname)) < 0)
		err(1, "gethostname");

	/*
	 * Copy the hostname into the default seed, eliminating any
	 * non alpha-numeric characters.
	 */
	for (i = 0, l = 0; l < sizeof(defaultseed); i++) {
		if (hostname[i] == '\0') {
			defaultseed[l] = hostname[i];
			break;
		}
		if (isalnum((unsigned char)hostname[i]))
			defaultseed[l++] = hostname[i];
	}

	defaultseed[SKEY_NAMELEN] = '\0';
	(void)time(&now);
	(void)snprintf(tbuf, sizeof(tbuf), "%05ld", (long) (now % 100000));
	(void)strlcat(defaultseed, tbuf, sizeof(defaultseed));

	if ((pp = getpwuid(getuid())) == NULL)
		err(1, "no user with uid %ld", (u_long)getuid());
	(void)strlcpy(me, pp->pw_name, sizeof(me));

	if ((pp = getpwnam(me)) == NULL)
		err(1, "Who are you?");
	salt = pp->pw_passwd;

	while((c = getopt(argc, argv, "k:n:p:t:sxz")) != -1) {
		switch(c) {
			case 'k':
				argkey = 1;
				if (strlen(optarg) > SKEY_MAX_PW_LEN)
					errx(1, "key too long");
				strlcpy(passwd, optarg, sizeof(passwd));
				strlcpy(passwd2, optarg, sizeof(passwd));
				break;
			case 'n':
				n = atoi(optarg);
				if(n < 1 || n > SKEY_MAX_SEQ)
					errx(1, "count must be between 1 and %d", SKEY_MAX_SEQ);
				break;
			case 'p':
				if (strlen(optarg) >= _PASSWORD_LEN)
					errx(1, "password too long");
				if ((pw = malloc(_PASSWORD_LEN + 1)) == NULL)
					err(1, "no memory for password");
				strlcpy(pw, optarg, _PASSWORD_LEN + 1);
				break;
			case 't':
				if(skey_set_algorithm(optarg) == NULL)
					errx(1, "Unknown hash algorithm %s", optarg);
				ht = optarg;
				break;
			case 's':
				defaultsetup = 0;
				break;
			case 'x':
				hexmode = 1;
				break;
			case 'z':
				zerokey = 1;
				break;
			default:
				errx(1, "usage: %s skeyinit [-sxz] [-k passphrase] "
				    "[-n count] [-p password] [-t hash] [user]",
				    argv[0]);
		}
	}

	if(argc > optind) {
		pp = getpwnam(argv[optind]);
		if (pp == NULL)
			errx(1, "User %s unknown", argv[optind]);
		}

	if (strcmp(pp->pw_name, me) != 0) {
		if (getuid() != 0) {
			/* Only root can change other's passwds */
			errx(1, "Permission denied.");
		}
	}

	if (getuid() != 0) {
		if (!argpass)
			pw = getpass("Password:");
		p = crypt(pw, salt);

		if (strcmp(p, pp->pw_passwd)) {
			errx(1, "Password incorrect.");
		}
	}

	rval = skeylookup(&skey, pp->pw_name);
	switch (rval) {
	case -1:
		err(1, "cannot open database");
	case 0:
		/* comment out user if asked to */
		if (zerokey)
			exit(skeyzero(&skey, pp->pw_name));

		printf("[Updating %s]\n", pp->pw_name);
		printf("Old key: [%s] %s\n", skey_get_algorithm(), skey.seed);

		/*
		 * lets be nice if they have a skey.seed that
		 * ends in 0-8 just add one
		 */
		l = strlen(skey.seed);
		if (l > 0) {
			lastc = skey.seed[l - 1];
			if (isdigit((unsigned char)lastc) && lastc != '9') {
				(void)strlcpy(defaultseed, skey.seed,
				    sizeof(defaultseed));
				defaultseed[l - 1] = lastc + 1;
			}
			if (isdigit((unsigned char)lastc) && lastc == '9' &&
			    l < 16) {
				(void)strlcpy(defaultseed, skey.seed,
				    sizeof(defaultseed));
				defaultseed[l - 1] = '0';
				defaultseed[l] = '0';
				defaultseed[l + 1] = '\0';
			}
		}
		break;
	case 1:
		if (zerokey)
			errx(1, "You have no entry to zero.");
		printf("[Adding %s]\n", pp->pw_name);
		break;
	}
	
	if (n==0)
		n = 100;

	/* Set hash type if asked to */
	if (ht) {
		/* Need to zero out old key when changing algorithm */
		if (strcmp(ht, skey_get_algorithm()) && skey_set_algorithm(ht))
			zerokey = 1;
	}

	if (!defaultsetup) {
		printf("You need the 6 english words generated from the \"skey\" command.\n");
		for (i = 0;; i++) {
			if (i >= 2)
				exit(1);
			printf("Enter sequence count from 1 to %d: ", SKEY_MAX_SEQ);
			fgets(buf, sizeof(buf), stdin);
			n = atoi(buf);
			if (n > 0 && n < SKEY_MAX_SEQ)
				break;	/* Valid range */
			printf("\nError: Count must be between 0 and %d\n", SKEY_MAX_SEQ);
		}
	
		for (i = 0;; i++) {
			if (i >= 2)
				exit(1);

			printf("Enter new seed [default %s]: ", defaultseed);
			fflush(stdout);
			fgets(seed, sizeof(seed), stdin);
			rip(seed);
			for (p = seed; *p; p++) {
				if (isalpha((unsigned char)*p)) {
					*p = tolower((unsigned char)*p);
				} else if (!isdigit((unsigned char)*p)) {
					(void)puts("Error: seed may only contain alphanumeric characters");
					break;
				}
			}
			if (*p == '\0')
				break;  /* Valid seed */
		}
		if (strlen(seed) > SKEY_MAX_SEED_LEN) {
			printf("Notice: Seed truncated to %d characters.\n", SKEY_MAX_SEED_LEN);
			seed[SKEY_MAX_SEED_LEN] = '\0';
		}
		if (seed[0] == '\0')
			(void)strlcpy(seed, defaultseed, sizeof(seed));

		for (i = 0;; i++) {
			if (i >= 2)
				exit(1);

			printf("otp-%s %d %s\ns/key access password: ",
				skey_get_algorithm(), n, seed);
			fgets(buf, sizeof(buf), stdin);
			rip(buf);
			backspace(buf);

			if (buf[0] == '?') {
				puts("Enter 6 English words from secure S/Key calculation.");
				continue;
			} else if (buf[0] == '\0') {
				exit(1);
			}
			if (etob(key, buf) == 1 || atob8(key, buf) == 0)
				break;	/* Valid format */
			(void)puts("Invalid format - try again with 6 English words.");
		}
	} else {
	/* Get user's secret password */
	puts("Reminder - Only use this method if you are directly connected\n"
	      "           or have an encrypted channel. If you are using telnet\n"
	      "           or rlogin, exit with no password and use skeyinit -s.\n");

	for (i = 0;; i++) {
			if (i >= 2)
				exit(1);

			if (!argkey) {
				printf("Enter secret password: ");
				readpass(passwd, sizeof(passwd));
				if (passwd[0] == '\0')
					exit(1);
			}

			if (strlen(passwd) < SKEY_MIN_PW_LEN) {
				(void)fprintf(stderr,
				    "Your password must be at least %d characters long.\n", SKEY_MIN_PW_LEN);
				continue;
			} else if (strcmp(passwd, pp->pw_name) == 0) {
				(void)fputs("Your password may not be the same as your user name.\n", stderr);
				continue;
			}
#if 0			 
			else if (strspn(passwd, "abcdefghijklmnopqrstuvwxyz") == strlen(passwd)) {
				(void)fputs("Your password must contain more than just lower case letters.\n"
					    "Whitespace, numbers, and puctuation are suggested.\n", stderr);
				continue;
			}
#endif

			if (!argkey) {
				printf("Again secret password: ");
				readpass(passwd2, sizeof(passwd));
				if (passwd2[0] == '\0')
					exit(1);
			}

			if (strcmp(passwd, passwd2) == 0)
				break;

			puts("Passwords do not match.");
		}

		/* Crunch seed and password into starting key */
		(void)strlcpy(seed, defaultseed, sizeof(seed));
		if (keycrunch(key, seed, passwd) != 0)
			err(2, "key crunch failed");
		nn = n;
		while (nn-- != 0)
			f(key);
	}
	(void)time(&now);
	tm = localtime(&now);
	(void)strftime(tbuf, sizeof(tbuf), " %b %d,%Y %T", tm);

	if ((skey.val = (char *)malloc(16 + 1)) == NULL)
		err(1, "Can't allocate memory");

	/* Zero out old key if necessary (entry would change size) */
	if (zerokey) {
		(void)skeyzero(&skey, pp->pw_name);
		/* Re-open keys file and seek to the end */
		if (skeylookup(&skey, pp->pw_name) == -1)
			err(1, "cannot open database");
	}

	btoa8(skey.val, key);

	/*
	 * Obtain an exclusive lock on the key file so we don't
	 * clobber someone authenticating themselves at the same time.
	 */
	for (i = 0; i < 300; i++) {
		if ((rval = flock(fileno(skey.keyfile), LOCK_EX|LOCK_NB)) == 0
		    || errno != EWOULDBLOCK)
			break;
		usleep(100000);			/* Sleep for 0.1 seconds */
	}
	if (rval == -1)	{			/* Can't get exclusive lock */
		errno = EAGAIN;
		err(1, "cannot open database");
	}

	/* Don't save algorithm type for md4 (keep record length same) */
	if (strcmp(skey_get_algorithm(), "md4") == 0)
		(void)fprintf(skey.keyfile, "%s %04d %-16s %s %-21s\n",
		    pp->pw_name, n, seed, skey.val, tbuf);
	else
		(void)fprintf(skey.keyfile, "%s %s %04d %-16s %s %-21s\n",
		    pp->pw_name, skey_get_algorithm(), n, seed, skey.val, tbuf);

	(void)fclose(skey.keyfile);

	(void)printf("\nID %s skey is otp-%s %d %s\n", pp->pw_name,
		     skey_get_algorithm(), n, seed);
	(void)printf("Next login password: %s\n\n",
		     hexmode ? put8(buf, key) : btoe(buf, key));

	return(0);
}
