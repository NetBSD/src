/*	$NetBSD: veriexecctl.c,v 1.5 2004/03/06 11:57:14 blymn Exp $	*/

/*-
 * Copyright (c) 1998-1999 Brett Lymn
 *                         (blymn@baea.com.au, brett_lymn@yahoo.com.au)
 * All rights reserved.
 *
 * This code has been donated to The NetBSD Foundation by the Author.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <err.h>

#define VERIEXEC_DEV "/dev/veriexec"

/* globals */
int fd;
extern FILE *yyin;
int yyparse(void);

int
main(int argc, char *argv[])
{
	if (argv[1] == NULL) {
		fprintf(stderr, "usage: veriexecctl signature_file\n");
		exit(1);
	}

	fd = open(VERIEXEC_DEV, O_WRONLY, 0);
	if (fd < 0) {
		err(EXIT_FAILURE, "Open of veriexec device %s failed",
		    VERIEXEC_DEV);
	}

	if ((yyin = fopen(argv[1], "r")) == NULL) {
		err(EXIT_FAILURE, "Opening signature file %s failed",
		    argv[1]);
	}

	yyparse();
        exit(0);
}
