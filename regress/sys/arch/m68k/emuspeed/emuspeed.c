/*	$NetBSD: emuspeed.c,v 1.5 2002/02/21 07:38:19 itojun Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "speed.h"

#define PRECISION 500

const struct test {
	char *name; 
	void (*func)__P((int));
	char *comment;
} testlist[] = {
	{"Illegal", illegal, "(test: unimplemented)"},
	{"mulsl Da,Db", mul32sreg, "(test: should be native)"},
	{"mulsl sp@(8),Da", mul32smem, "(test: should be native)\n"},
	
	{"mulsl Dn,Da:Db", mul64sreg, "emulated on 68060"},
	{"mulul Dn,Da:Db", mul64ureg, "\t\""},
	{"mulsl sp@(8),Da:Db", mul64smem, "\t\""},
	{"mulul sp@(8),Da:Db", mul64umem, "\t\"\n"},

	{"divsl Da:Db,Dn", div64sreg, "\t\""},
	{"divul Da:Db,Dn", div64ureg, "\t\""},
	{"divsl Da:Db,sp@(8)", div64smem, "\t\""},
	{"divul Da:Db,sp@(8)", div64umem, "\t\"\n"},

	{NULL, NULL, NULL}
};

jmp_buf jbuf;
void illhand (int);

int
main(argc, argv)
	int argc;
	char *argv[];
{
	const struct test *t;
	clock_t start, stop;
	int count;


	if (signal(SIGILL, &illhand))
		warn("%s: can't install illegal instruction handler.",
		    argv[0]);

	printf("Speed of instructions which are emulated on some cpus:\n\n");
	(void)sleep(1);
	for (t=testlist; t->name; t++) {
		printf("%-20s", t->name);
		fflush(stdout);

		if (setjmp(jbuf)) {
			printf("%12s    %s\n", "[unimplemented]", t->comment);
			continue;
		}
			
		count = 1000;
		do {
			count *= 2;
			start = clock();
			t->func(count);
			stop = clock();
		} while ((stop - start) < PRECISION);
		printf("%10d/s    %s\n",
		    CLOCKS_PER_SEC*(count /(stop - start)),
		    t->comment);
	}
	exit (0);
}

void
illhand(int i)
{
	longjmp(jbuf, 1);
}
