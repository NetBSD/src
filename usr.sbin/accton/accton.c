/*
 * Copyright (c) 1993 Christopher G. Demetriou
 * Copyright (c) 1988 Regents of the University of California.
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

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1988 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/* from static char sccsid[] = "@(#)accton.c	4.3 (Berkeley) 6/1/90"; */
static char rcsid[] = "$Id: accton.c,v 1.2 1993/05/03 01:53:23 cgd Exp $";
#endif /* not lint */

#include <stdio.h>
#include <string.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int turnon;
	char *tail;

	if ((tail = strrchr(argv[0], '/')) == NULL) {
		tail = argv[0];
	} else {
		tail++;
	}
	if (!strcmp(tail, "accton")) {
		turnon = 1;
	} else if (!strcmp(tail, "acctoff")) {
		turnon = 0;
	} else {
		turnon = -1;
	}
	if (argc != 1 || turnon == -1) {
		fputs("usage: accton\n", stderr);
		fputs("or:    acctoff\n", stderr);
		exit(1);
	}
	if (acct(turnon) < 0) {
		perror("accton");
		exit(1);
	}
	exit(0);
}
