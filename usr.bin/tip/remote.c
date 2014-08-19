/*	$NetBSD: remote.c,v 1.18.24.1 2014/08/20 00:05:04 tls Exp $	*/

/*
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
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
__COPYRIGHT("@(#) Copyright (c) 1992, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)remote.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: remote.c,v 1.18.24.1 2014/08/20 00:05:04 tls Exp $");
#endif /* not lint */

#include "pathnames.h"
#include "tip.h"

/*
 * Attributes to be gleened from remote host description
 *   data base.
 */
static char **caps[] = {
	&AT, &DV, &CM, &CU, &EL, &IE, &OE, &PN, &PR, &DI,
	&ES, &EX, &FO, &RC, &RE, &PA
};

static const char *capstrings[] = {
	"at", "dv", "cm", "cu", "el", "ie", "oe", "pn", "pr",
	"di", "es", "ex", "fo", "rc", "re", "pa", 0
};

static const char	*db_array[3] = { _PATH_REMOTE, 0, 0 };

#define cgetflag(f)	(cgetcap(bp, f, ':') != NULL)

static	void	getremcap(char *);

static char tiprecord[] = "tip.record";
static char wspace[] = "\t\n\b\f";

static void
getremcap(char *host)
{
	const char **p;
	char ***q;
	char *bp;
	char *rempath;
	int   status;

	rempath = getenv("REMOTE");
	if (rempath != NULL) {
		if (*rempath != '/')
			/* we have an entry */
			(void)cgetset(rempath);
		else {	/* we have a path */
			db_array[1] = rempath;
			db_array[2] = _PATH_REMOTE;
		}
	}
	if ((status = cgetent(&bp, db_array, host)) < 0) {
		if (DV ||
		    (host[0] == '/' && access(DV = host, R_OK | W_OK) == 0)) {
			CU = DV;
			HO = host;
			HW = 1;
			DU = 0;
			if (!BR)
				BR = DEFBR;
			FS = DEFFS;
			return;
		}
		switch(status) {
		case -1:
			warnx("unknown host %s", host);
			break;
		case -2:
			warnx("can't open host description file");
			break;
		case -3:
			warnx("possible reference loop in host "
			    "description file");

			break;
		}
		exit(3);
	}

	for (p = capstrings, q = caps; *p != NULL; p++, q++)
		if (**q == NULL)
			(void)cgetstr(bp, *p, *q);
	if (!BR && (cgetnum(bp, "br", &BR) == -1))
		BR = DEFBR;
	if (cgetnum(bp, "fs", &FS) == -1)
		FS = DEFFS;
	if (DU < 0)
		DU = 0;
	else
		DU = cgetflag("du");
	if (DV == NULL) {
		errx(3, "%s: missing device spec\n", host);
	}
	if (DU && CU == NULL)
		CU = DV;
	if (DU && PN == NULL) {
		errx(3, "%s: missing phone number\n", host);
	}

	HD = cgetflag("hd");

	/*
	 * This effectively eliminates the "hw" attribute
	 *   from the description file
	 */
	if (!HW)
		HW = (CU == NULL) || (DU && strcmp(DV, CU) == 0);
	HO = host;
	/*
	 * see if uppercase mode should be turned on initially
	 */
	if (cgetflag("ra"))
		setboolean(value(RAISE), 1);
	if (cgetflag("ec"))
		setboolean(value(ECHOCHECK), 1);
	if (cgetflag("be"))
		setboolean(value(BEAUTIFY), 1);
	if (cgetflag("nb"))
		setboolean(value(BEAUTIFY), 0);
	if (cgetflag("sc"))
		setboolean(value(SCRIPT), 1);
	if (cgetflag("tb"))
		setboolean(value(TABEXPAND), 1);
	if (cgetflag("vb"))
		setboolean(value(VERBOSE), 1);
	if (cgetflag("nv"))
		setboolean(value(VERBOSE), 0);
	if (cgetflag("ta"))
		setboolean(value(TAND), 1);
	if (cgetflag("nt"))
		setboolean(value(TAND), 0);
	if (cgetflag("rw"))
		setboolean(value(RAWFTP), 1);
	if (cgetflag("hd"))
		setboolean(value(HALFDUPLEX), 1);
	if (cgetflag("dc"))
		DC = 1;
	if (cgetflag("hf"))
		setboolean(value(HARDWAREFLOW), 1);
	if (RE == NULL)
		RE = tiprecord;
	if (EX == NULL)
		EX = wspace;
	if (ES != NULL)
		(void)vstring("es", ES);
	if (FO != NULL)
		(void)vstring("fo", FO);
	if (PR != NULL)
		(void)vstring("pr", PR);
	if (RC != NULL)
		(void)vstring("rc", RC);
	if (cgetnum(bp, "dl", &DL) == -1)
		DL = 0;
	if (cgetnum(bp, "cl", &CL) == -1)
		CL = 0;
	if (cgetnum(bp, "et", &ET) == -1)
		ET = 10;
}

char *
getremote(char *host)
{
	char *cp;
	static char *next;
	static int lookedup = 0;

	if (!lookedup) {
		if (host == NULL && (host = getenv("HOST")) == NULL) {
			errx(3, "no host specified");
		}
		getremcap(host);
		next = DV;
		lookedup++;
	}
	/*
	 * We return a new device each time we're called (to allow
	 *   a rotary action to be simulated)
	 */
	if (next == NULL)
		return (NULL);
	if ((cp = strchr(next, ',')) == NULL) {
		DV = next;
		next = NULL;
	} else {
		*cp++ = '\0';
		DV = next;
		next = cp;
	}
	return (DV);
}
