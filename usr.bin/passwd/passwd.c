/*	$NetBSD: passwd.c,v 1.14 1999/08/26 07:33:16 marc Exp $	*/

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
__RCSID("$NetBSD: passwd.c,v 1.14 1999/08/26 07:33:16 marc Exp $");
#endif
#endif /* not lint */

#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"
 
void	usage __P((void)); 

/*
 * Note on configuration:
 *      Generally one would not use both Kerberos and YP
 *      to maintain passwords.
 */

int use_kerberos;
int use_yp;
int yppwd;
int yflag;

extern	char *__progname;		/* from crt0.o */

int	main __P((int, char **));

#ifdef YP
extern int _yp_check __P((char **));	/* buried deep inside libc */
#endif

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	int ch;
	char *username;
#if defined(KERBEROS) || defined(KERBEROS5)
	char *iflag = 0, *rflag = 0, *uflag = 0;

	if (strcmp(__progname, "kpasswd") == 0)
		use_kerberos = 1;
	else
		use_kerberos = krb_check();
#endif
#ifdef	YP
	use_yp = _yp_check(NULL);
#endif

	if (strcmp(__progname, "yppasswd") == 0) {
#ifdef YP
		if (!use_yp)
			errx(1, "YP not in use.");
		use_kerberos = 0;
		yppwd = 1;
#else
		errx(1, "YP support not compiled in.");
#endif
	}

	while ((ch = getopt(argc, argv, "lkyi:r:u:")) != -1)
		switch (ch) {
		case 'l':		/* change local password file */
			if (yppwd)
				usage();
			use_kerberos = 0;
			use_yp = 0;
			break;
#ifdef KERBEROS
		case 'i':
			iflag = optarg;
			break;
		case 'r':
			rflag = optarg;
			break;
		case 'u':
			uflag = optarg;
			break;	
#endif
		case 'k':		/* change Kerberos password */
#if defined(KERBEROS) || defined(KERBEROS5)
			if (yppwd)
				usage();
			use_kerberos = 1;
			use_yp = 0;
			break;
#endif
#ifndef KERBEROS
		case 'i':
		case 'r':
		case 'u':
			errx(1, "Kerberos4 support not compiled in.");
#endif
		case 'y':		/* change YP password */
#ifdef	YP
			if (yppwd)
				usage();
			if (!use_yp)
				errx(1, "YP not in use.");
			use_kerberos = 0;
			yflag = 1;
			break;
#else
			errx(1, "YP support not compiled in.");
#endif
		default:
			usage();
		}

	argc -= optind;
	argv += optind;

	username = getlogin();
	if (username == NULL)
		errx(1, "who are you ??");
	
	switch(argc) {
	case 0:
		break;
	case 1:
#ifdef KERBEROS5
		if (use_kerberos && strcmp(argv[0], username)) {
			errx(1, "%s\n\t%s\n%s\n",
			     "to change another user's Kerberos password, do",
			     "\"kinit <user>; passwd; kdestroy\";",
			     "to change a user's local passwd, use\
			     \"passwd -l <user>\"");
		}
#endif
		username = argv[0];
		break;
	default:
		usage();
		exit(1);
	}

#if defined(KERBEROS) || defined(KERBEROS5)
	if (uflag && (iflag || rflag))
		errx(1, "-u cannot be used with -r or -i");

	if (use_kerberos)
		exit(kadm_passwd(username, iflag, rflag, uflag));
#else
#ifdef KERBEROS5
	if (use_kerberos)
		exit(krb_passwd());
#endif
#endif
#ifdef	YP
	if (use_yp)
		exit(yp_passwd(username));
#endif
	exit(local_passwd(username));
}

void
usage()
{

	if (yppwd)
		fprintf(stderr, "usage: %s user\n", __progname);
	else
		fprintf(stderr, "usage: %s [-l] [-k] [-y] [-i instance] [-r realm] [-u fullname] user\n", __progname);
	exit(1);
}
