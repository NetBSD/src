/*	$NetBSD: acu.c,v 1.9 2004/04/23 22:11:44 christos Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
#if 0
static char sccsid[] = "@(#)acu.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: acu.c,v 1.9 2004/04/23 22:11:44 christos Exp $");
#endif /* not lint */

#include "tip.h"

static acu_t *acu = NULL;
static int conflag;
static jmp_buf jmpbuf;

static void	acuabort __P((int));
static acu_t   *acutype __P((char *));

/*
 * Establish connection for tip
 *
 * If DU is true, we should dial an ACU whose type is AT.
 * The phone numbers are in PN, and the call unit is in CU.
 *
 * If the PN is an '@', then we consult the PHONES file for
 *   the phone numbers.  This file is /etc/phones, unless overriden
 *   by an exported shell variable.
 *
 * The data base files must be in the format:
 *	host-name[ \t]*phone-number
 *   with the possibility of multiple phone numbers
 *   for a single host acting as a rotary (in the order
 *   found in the file).
 */
const char *
connect()
{
	char *cp = PN;
	char *phnum, string[256];
	FILE *fd;
	int tried = 0;

#if __GNUC__		/* XXX pacify gcc */
	(void)&cp;
	(void)&tried;
#endif

	if (!DU) {		/* regular connect message */
		if (CM != NULL)
			xpwrite(FD, CM, strlen(CM));
		logent(value(HOST), "", DV, "call completed");
		return (NULL);
	}
	/*
	 * @ =>'s use data base in PHONES environment variable
	 *        otherwise, use /etc/phones
	 */
	signal(SIGINT, acuabort);
	signal(SIGQUIT, acuabort);
	if (setjmp(jmpbuf)) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		printf("\ncall aborted\n");
		logent(value(HOST), "", "", "call aborted");
		if (acu != NULL) {
			setboolean(value(VERBOSE), FALSE);
			if (conflag)
				disconnect(NULL);
			else
				(*acu->acu_abort)();
		}
		return ("interrupt");
	}
	if ((acu = acutype(AT)) == NULL)
		return ("unknown ACU type");
	if (*cp != '@') {
		while (*cp) {
			for (phnum = cp; *cp && *cp != ','; cp++)
				;
			if (*cp)
				*cp++ = '\0';
			
			if ((conflag = (*acu->acu_dialer)(phnum, CU)) != 0) {
				if (CM != NULL)
					xpwrite(FD, CM, strlen(CM));
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NULL);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
	} else {
		if ((fd = fopen(PH, "r")) == NULL) {
			printf("%s: ", PH);
			return ("can't open phone number file");
		}
		while (fgets(string, sizeof(string), fd) != NULL) {
			for (cp = string; !any(*cp, " \t\n"); cp++)
				;
			if (*cp == '\n') {
				fclose(fd);
				return ("unrecognizable host name");
			}
			*cp++ = '\0';
			if (strcmp(string, value(HOST)))
				continue;
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\n') {
				fclose(fd);
				return ("missing phone number");
			}
			for (phnum = cp; *cp && *cp != ',' && *cp != '\n'; cp++)
				;
			if (*cp)
				*cp++ = '\0';
			
			if ((conflag = (*acu->acu_dialer)(phnum, CU)) != 0) {
				fclose(fd);
				if (CM != NULL)
					xpwrite(FD, CM, strlen(CM));
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NULL);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
		fclose(fd);
	}
	if (!tried)
		logent(value(HOST), "", acu->acu_name, "missing phone number");
	else
		(*acu->acu_abort)();
	return (tried ? "call failed" : "missing phone number");
}

void
disconnect(reason)
	const char *reason;
{

	if (!conflag) {
		logent(value(HOST), "", DV, "call terminated");
		return;
	}
	if (reason == NULL) {
		logent(value(HOST), "", acu->acu_name, "call terminated");
		if (boolean(value(VERBOSE)))
			printf("\r\ndisconnecting...");
	} else 
		logent(value(HOST), "", acu->acu_name, reason);
	(*acu->acu_disconnect)();
}

static void
acuabort(s)
	int s;
{

	signal(s, SIG_IGN);
	longjmp(jmpbuf, 1);
}

static acu_t *
acutype(s)
	char *s;
{
	acu_t *p;

	for (p = acutable; p->acu_name != '\0'; p++)
		if (!strcmp(s, p->acu_name))
			return (p);
	return (NULL);
}
