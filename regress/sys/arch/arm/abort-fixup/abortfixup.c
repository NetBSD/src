/* $NetBSD: abortfixup.c,v 1.3 2002/03/17 12:25:11 bjh21 Exp $ */

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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
 *
 *
 * Trying out if an unhandled instruction pattern in the late abort fixup
 * generates a panic or sysfaults like it ought to do.
 */

#include <sys/types.h>

__RCSID("$NetBSD: abortfixup.c,v 1.3 2002/03/17 12:25:11 bjh21 Exp $");

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

void
sighandler(int sig)
{

	/* Catching SIGSEGV means the test passed. */
	exit(0);
}

int
main(void)
{

	if (signal(SIGSEGV, sighandler) == SIG_ERR)
		err(1, "signal");

	printf("ARM 6/7 abort fixup panic regression test\n");
	printf("    It should not panic!\n");

	/*
 	 * issue an instruction that for certain generates a page
	 * fault but _can't_ be fixed up by late abort fixup
	 * routines due to its structure.
	 */

	__asm __volatile ("
		mvn r0, #0
		mov r1, r0
		str r1, [r0], r1, ror #10
	");

	/* Should not be reached if OK */
	printf("!!! Regression test FAILED - no SEGV recieved\n");
	return 1;
};

