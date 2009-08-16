/*	$NetBSD: ldd_aout.c,v 1.4 2009/08/16 18:43:08 martin Exp $	*/

/*-
 * Copyright (c) 1998, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
#ifndef lint
__RCSID("$NetBSD: ldd_aout.c,v 1.4 2009/08/16 18:43:08 martin Exp $");
#endif /* not lint */

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/exec_aout.h>

#include <a.out.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* from <compat/netbsd32/netbsd32_exec.h> */
/*
 * Header prepended to each a.out file.
 * only manipulate the a_midmag field via the
 * N_SETMAGIC/N_GET{MAGIC,MID,FLAG} macros below.
 */
typedef unsigned int netbsd32_u_long;
struct netbsd32_exec {
	netbsd32_u_long a_midmag;       /* htonl(flags<<26 | mid<<16 | magic) */
	netbsd32_u_long a_text;         /* text segment size */
	netbsd32_u_long a_data;         /* initialized data size */
	netbsd32_u_long a_bss;          /* uninitialized data size */
	netbsd32_u_long a_syms;         /* symbol table size */
	netbsd32_u_long a_entry;        /* entry point */
	netbsd32_u_long a_trsize;       /* text relocation size */
	netbsd32_u_long a_drsize;       /* data relocation size */
};


/*
 * XXX put this here, rather than a separate ldd_aout.h
#include "ldd.h"
 */
int aout_ldd(int, char *, char *, char *);

int
aout_ldd(int fd, char *file, char *fmt1, char *fmt2)
{
	struct netbsd32_exec hdr;
	int status, rval;

	if (lseek(fd, 0, SEEK_SET) < 0 ||
	    read(fd, &hdr, sizeof hdr) != sizeof hdr
#if 1 /* Compatibility */
	    || hdr.a_entry < N_PAGSIZ(hdr)
#endif
	    || (N_GETFLAG(hdr) & EX_DPMASK) != EX_DYNAMIC)
		/* calling function prints warning */
		return -1;

	setenv("LD_TRACE_LOADED_OBJECTS", "", 1);
	if (fmt1)
		setenv("LD_TRACE_LOADED_OBJECTS_FMT1", fmt1, 1);
	if (fmt2)
		setenv("LD_TRACE_LOADED_OBJECTS_FMT2", fmt2, 1);

	setenv("LD_TRACE_LOADED_OBJECTS_PROGNAME", file, 1);
	if (fmt1 == NULL && fmt2 == NULL)
		printf("%s:\n", file);
	fflush(stdout);

	rval = 0;
	switch (fork()) {
	case -1:
		err(1, "fork");
		break;
	default:
		if (wait(&status) <= 0) {
			warn("wait");
			rval |= 1;
		} else if (WIFSIGNALED(status)) {
			fprintf(stderr, "%s: signal %d\n",
					file, WTERMSIG(status));
			rval |= 1;
		} else if (WIFEXITED(status) && WEXITSTATUS(status)) {
			fprintf(stderr, "%s: exit status %d\n",
					file, WEXITSTATUS(status));
			rval |= 1;
		}
		break;
	case 0:
		rval |= execl(file, file, NULL) != 0;
		perror(file);
		_exit(1);
	}
	return rval;
}
