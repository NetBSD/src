/* $NetBSD: checkrc.c,v 1.1 2014/07/26 19:30:44 dholland Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jeffrey C. Rizzo
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

/* checkrc.c -- Create a script on the target to check the state of
 * its rc.conf variables. */

#include <curses.h>
#include <err.h>
#include <stdio.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

#define RC_CHECK_SCRIPT "/tmp/checkrc.sh"

static int create_script(const char *, int);
static int check(const char *, int);

char *rcconf = NULL;

enum {
	CHECK_CONF,
	CHECK_DEFAULT
};

static int
create_script(const char *varname, int filetocheck)
{
	FILE	*fp;
	
	if ((fp = fopen(target_expand(RC_CHECK_SCRIPT), "w")) == NULL) {
		if (logfp)
			fprintf(logfp,"Could not open %s for writing",
			    target_expand(RC_CHECK_SCRIPT));
		warn("Could not open %s for writing",
		    target_expand(RC_CHECK_SCRIPT));
		return 1;
	}

	if (filetocheck == CHECK_DEFAULT)
		fprintf(fp, "#!/bin/sh\n. /etc/defaults/rc.conf\n"
		    ". /etc/rc.subr\n");
	else
		fprintf(fp, "#!/bin/sh\n. /etc/rc.conf\n. /etc/rc.subr\n");
	fprintf(fp, "if checkyesno %s\nthen\necho YES\nelse\necho NO\nfi\n",
	    varname);

	fclose(fp);
	return 0;
}

static int
check(const char *varname, int filetocheck)
{
	char *buf;

	create_script(varname, filetocheck);

	if (target_already_root())
		collect(T_OUTPUT, &buf, "/bin/sh %s 2>&1", RC_CHECK_SCRIPT);
	else
		collect(T_OUTPUT, &buf, "chroot %s /bin/sh %s 2>&1",
				target_prefix(), RC_CHECK_SCRIPT);


	unlink(target_expand(RC_CHECK_SCRIPT));

	if (logfp) {
		fprintf(logfp,"var %s is %s\n", varname, buf);
		fflush(logfp);
	}

	if (strncmp(buf, "YES", strlen("YES")) == 0)
		return 1;
	else
		return 0;
}

int
check_rcvar(const char *varname)
{
	return check(varname, CHECK_CONF);
}

int
check_rcdefault(const char *varname)
{
	return check(varname, CHECK_DEFAULT);
}
