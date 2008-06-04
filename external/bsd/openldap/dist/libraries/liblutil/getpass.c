/* getpass.c -- get password from user */
/* $OpenLDAP: pkg/ldap/libraries/liblutil/getpass.c,v 1.17.2.3 2008/02/11 23:26:42 kurt Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2008 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1992, 1993  Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* This work was originally developed by the University of Michigan
 * and distributed as part of U-MICH LDAP.  It was adapted for use in
 * -llutil by Kurt D. Zeilenga.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/string.h>
#include <ac/termios.h>
#include <ac/time.h>
#include <ac/unistd.h>

#ifdef NEED_GETPASSPHRASE

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_CONIO_H
#include <conio.h>
#endif

#include <lber.h>
#include <ldap.h>

#include "ldap_defaults.h"

char *
lutil_getpass( const char *prompt )
{
#if !defined(HAVE_TERMIOS_H) && !defined(HAVE_SGTTY_H)
	static char buf[256];
	int i, c;

	if( prompt == NULL ) prompt = _("Password: ");

#ifdef DEBUG
	if (debug & D_TRACE)
		printf("->getpass(%s)\n", prompt);
#endif

	printf("%s", prompt);
	i = 0;
	while ( (c = getch()) != EOF && c != '\n' && c != '\r' )
		buf[i++] = c;
	if ( c == EOF )
		return( NULL );
	buf[i] = '\0';
	return (buf);
#else
	int no_pass = 0;
	char i, j, k;
	TERMIO_TYPE ttyb;
	TERMFLAG_TYPE flags;
	static char pbuf[513];
	register char *p;
	register int c;
	FILE *fi;
	RETSIGTYPE (*sig)( int sig );

	if( prompt == NULL ) prompt = _("Password: ");

#ifdef DEBUG
	if (debug & D_TRACE)
		printf("->getpass(%s)\n", prompt);
#endif
	/*
	 *  Stolen from the getpass() routine.  Can't use the plain
	 *  getpass() for two reasons.  One is that LDAP passwords
	 *  can be really, really long - much longer than 8 chars.
	 *  The second is that we like to make this client available
	 *  out of inetd via a Merit asynch port, and we need to be
	 *  able to do telnet control codes to turn on and off line
	 *  blanking.
	 */
	if ((fi = fdopen(open("/dev/tty", 2), "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	sig = SIGNAL (SIGINT, SIG_IGN);
	if (fi != stdin) {
		if (GETATTR(fileno(fi), &ttyb) < 0)
			perror("GETATTR");
	}
	flags = GETFLAGS( ttyb );
	SETFLAGS( ttyb, flags & ~ECHO );
	if (fi != stdin) {
		if (SETATTR(fileno(fi), &ttyb) < 0)
			perror("SETATTR");
	}

	/*  blank the line if through Merit */
	if (fi == stdin) {
		printf("%c%c%c", 255, 251, 1);
		fflush(stdout);
		(void) scanf("%c%c%c", &i, &j, &k);
		fflush(stdin);
	}

	/* fetch the password */
	fprintf(stdout, "%s", prompt); 
	fflush(stdout);
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (c == '\r')
			break;
		if (p < &pbuf[512])
			*p++ = c;
	}
	if (c == EOF)
		no_pass = 1;
	else {
		*p = '\0';
		if (*(p - 1) == '\r')
			*(p - 1) = '\0';
	}

	/*  unblank the line if through Merit */
	if (fi == stdin) {
		printf("%c%c%c", 255, 252, 1);
		fflush(stdout);
		(void) scanf("%c%c%c", &i, &j, &k);
		fflush(stdin);
		printf("\n"); fflush(stdout);
	}
	fprintf(stdout, "\n"); 
	fflush(stdout);

	/* tidy up */
	SETFLAGS( ttyb, flags );
	if (fi != stdin) {
		if (SETATTR(fileno(fi), &ttyb) < 0)
			perror("SETATTR");
	}
	(void) SIGNAL (SIGINT, sig);
	if (fi != stdin)
		(void) fclose(fi);
	else
		i = getchar();
	if (no_pass)
		return(NULL);
	return(pbuf);
#endif
}

#endif /* !NEED_GETPASSPHRASE */
