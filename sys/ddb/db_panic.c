/*-
 * Copyright (c) 2000, 2002, 2006, 2007, 2009, 2013 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: db_panic.c,v 1.2.4.3 2014/08/20 00:03:35 tls Exp $");

#include <sys/types.h>
#include <sys/cpu.h>

#include <ddb/ddbvar.h>
#include <ddb/ddb.h>

/*
 * db_panic: Called by panic().  May print a stack trace; may enter the
 * kernel debugger; may just return so that panic() will continue to
 * halt or reboot the system.
 */
void db_panic(void)
{

	if (db_onpanic == 1)
		Debugger();
	else if (db_onpanic >= 0) {
		static int intrace = 0;

		if (intrace == 0) {
			intrace = 1;
			printf("cpu%u: Begin traceback...\n",
			    cpu_index(curcpu()));
			db_stack_trace_print(
			    (db_expr_t)(intptr_t)__builtin_frame_address(0),
			    true, 65535, "", printf);
			printf("cpu%u: End traceback...\n",
			    cpu_index(curcpu()));
			intrace = 0;
		} else
			printf("Faulted in mid-traceback; aborting...\n");
		if (db_onpanic == 2)
			Debugger();
	}
	return;
}
