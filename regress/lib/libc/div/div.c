/*	$NetBSD: div.c,v 1.2 2002/02/21 07:38:15 itojun Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>

#define	NUM	1999236
#define	DENOM	1000000
#define	QUOT	1
#define	REM	999236

int
main(void)
{
	div_t d;
	ldiv_t ld;
	lldiv_t lld;
	int error = 0;

	d = div(NUM, DENOM);
	ld = ldiv(NUM, DENOM);
	lld = lldiv(NUM, DENOM);

	if (d.quot != QUOT || d.rem != REM) {
		fprintf(stderr, "div returned (%d, %d), expected (%d, %d)\n",
		    d.quot, d.rem, QUOT, REM);
		error++;
	}
	if (ld.quot != QUOT || ld.rem != REM) {
		fprintf(stderr, "ldiv returned (%ld, %ld), expected (%d, %d)\n",
		    ld.quot, ld.rem, QUOT, REM);
		error++;
	}
	if (lld.quot != QUOT || lld.rem != REM) {
		fprintf(stderr, "lldiv returned (%lld, %lld), expected (%d, %d)\n",
		    lld.quot, lld.rem, QUOT, REM);
		error++;
	}
	if (error > 0)
		fprintf(stderr, "div: got %d errors\n", error);
	exit(error);
}
