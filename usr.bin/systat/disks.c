/*	$NetBSD: disks.c,v 1.16.30.1 2009/05/13 19:20:07 jym Exp $	*/

/*-
 * Copyright (c) 1980, 1992, 1993
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
static char sccsid[] = "@(#)disks.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: disks.c,v 1.16.30.1 2009/05/13 19:20:07 jym Exp $");
#endif /* not lint */

#include <ctype.h>
#include <string.h>

#include "systat.h"
#include "extern.h"
#include "drvstats.h"

static void drvselect(char *args, int truefalse, int selections[]);

void
disks_add(char *args)
{

	if (args)
		drvselect(args, 1, drv_select);
}

void
disks_remove(char *args)
{

	if (args)
		drvselect(args, 0, drv_select);
}

void
disks_drives(char *args)
{
	size_t i;

	if (args) {
		for (i = 0; i < ndrive; i++)
			drv_select[i] = 0;
		disks_add(args);
	} else {
		move(CMDLINE, 0);
		clrtoeol();
		for (i = 0; i < ndrive; i++)
			printw("%s ", dr_name[i]);
	}
}

static void
drvselect(char *args, int truefalse, int selections[])
{
	char *cp;
	size_t i;

	cp = strchr(args, '\n');
	if (cp)
		*cp = '\0';
	for (;;) {
		for (cp = args; *cp && isspace((unsigned char)*cp); cp++)
			;
		args = cp;
		for (; *cp && !isspace((unsigned char)*cp); cp++)
			;
		if (*cp)
			*cp++ = '\0';
		if (cp - args == 0)
			break;
		for (i = 0; i < ndrive; i++)
			if (strcmp(args, dr_name[i]) == 0) {
				selections[i] = truefalse;
				break;
			}
		if (i >= ndrive)
			error("%s: unknown drive", args);
		args = cp;
	}
	labels();
	display(0);
}
