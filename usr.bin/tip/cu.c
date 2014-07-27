/*	$NetBSD: cu.c,v 1.22 2014/07/27 04:32:23 dholland Exp $	*/

/*
 * Copyright (c) 1983, 1993
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
#include <getopt.h>

#ifndef lint
#if 0
static char sccsid[] = "@(#)cu.c	8.1 (Berkeley) 6/6/93";
#endif
__RCSID("$NetBSD: cu.c,v 1.22 2014/07/27 04:32:23 dholland Exp $");
#endif /* not lint */

#include "tip.h"

__dead static void cuhelp(void);
__dead static void cuusage(void);

/*
 * Botch the interface to look like cu's
 */
void
cumain(int argc, char *argv[])
{
	int c, i, phonearg = 0;
	int parity = 0;		/* 0 is no parity */
	int flow = -1;		/* -1 is "tandem" ^S/^Q */
	static int helpme = 0, nostop = 0;
	char useresc = '~';
	static char sbuf[12];
	int cmdlineBR;
	extern char *optarg;
	extern int optind;

	static struct option longopts[] = {
		{ "help",	no_argument,		&helpme,	1 },
		{ "escape",	required_argument,	NULL,		'E' },
		{ "flow",	required_argument,	NULL,		'F' },
		{ "parity",	required_argument,	NULL,		'P' },
		{ "phone", 	required_argument,	NULL,		'c' },
		{ "port",	required_argument,	NULL,		'a' },
		{ "line",	required_argument,	NULL,		'l' },
		{ "speed",	required_argument,	NULL,		's' },
		{ "halfduplex",	no_argument,		NULL,		'h' },
		{ "nostop",	no_argument,		&nostop,	1  },
		{ NULL,		0,			NULL,		0 }
	};


	if (argc < 2)
		cuusage();

	CU = NULL;
	DV = NULL;
	BR = DEFBR;
	cmdlineBR = 0;

	while((c = getopt_long(argc, argv,
	    "E:F:P:a:p:c:l:s:hefot0123456789", longopts, NULL)) != -1) {

		if (helpme == 1) cuhelp();

		switch(c) {

		case 'E':
			if(strlen(optarg) > 1)
				errx(3, "only one escape character allowed");
			useresc = optarg[0];
			break;
		case 'F':
			if (strncmp(optarg, "hard", sizeof("hard") - 1 ) == 0)
				flow = 1;
			else
				if (strncmp(optarg, "soft",
				    sizeof("soft") - 1 ) == 0)
					flow = -1;
				else
					if(strcmp(optarg, "none") != 0)
						errx(3, "bad flow setting");
					else
						flow = 0;
			break;
		case 'P':
			if(strcmp(optarg, "even") == 0)
				parity = -1;
			else
				if(strcmp(optarg, "odd") == 0)
					parity = 1;
				else
					if(strcmp(optarg, "none") != 0)
						errx(3, "bad parity setting");
					else
						parity = 0;
			break;
		case 'a':
		case 'p':
			CU = optarg;
			break;
		case 'c':
			phonearg = 1;
			PN = optarg;
			break;
		case 'l':
			if (DV != NULL)
				errx(3,"more than one line specified");
			if(strchr(optarg, '/'))
				DV = optarg;
			else
				(void)asprintf(&DV, "/dev/%s", optarg);
			break;
		case 's':
			BR = atoi(optarg);
			break;
		case 'h':
			HD = TRUE;
			break;
		case 'e':
			if (parity != 0)
				errx(3, "more than one parity specified");
			parity = -1; /* even */
			break;
			/* Compatibility with Taylor cu */
		case 'f':
			flow = 0;
			break;
		case 'o':
			if (parity != 0)
				errx(3, "more than one parity specified");
			parity = 1; /* odd */
			break;
		case 't':
			HW = 1, DU = -1, DC = 1;
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			cmdlineBR = cmdlineBR * 10 + (c - '0');
			BR = cmdlineBR;
			break;
		default:
			if (nostop == 0)
				cuusage();
			break;
		}
	}

	argc -= optind;
	argv += optind;

	switch (argc) {
	case 1:
		if (phonearg)
			errx(3, "more than one phone number specified");
		else
			/* Compatibility with Taylor cu */
			if(!strcmp(argv[0], "dir")) {
				HW = 1; DU = -1; DC = 1;
			} else
				PN = argv[0];
		break;
	case 0:
		/*
		 * No system or number to call.  We're "direct", so use
		 * the tty as local.
		 */
		HW = 1; DU = -1; DC = 1;
		break;
	default:
		cuusage();
		break;
	}

	(void)signal(SIGINT, cleanup);
	(void)signal(SIGQUIT, cleanup);
	(void)signal(SIGHUP, cleanup);
	(void)signal(SIGTERM, cleanup);
	/* (void)signal(SIGCHLD, SIG_DFL) */	/* XXX seems wrong */

	/*
	 * The "cu" host name is used to define the
	 * attributes of the generic dialer.
	 */
	(void)snprintf(sbuf, sizeof sbuf, "cu%d", (int)BR);
	if ((i = hunt(sbuf)) == 0) {
		errx(3,"all ports busy");
	}
	if (i == -1) {
		errx(3, "link down");
	}
	setbuf(stdout, NULL);
	vinit();
	switch (parity) {
	case -1:
		setparity("even");
		break;
	case 1:
		setparity("odd");
		break;
	case 0:
		setparity("none");
		break;
	default:
		setparity("none");
		break;
	}

	switch (flow) {
	case -1:
		if(nostop) {
			setboolean(value(TAND), FALSE);
			setboolean(value(HARDWAREFLOW), FALSE);
		}
		else {
			setboolean(value(TAND), TRUE);
			setboolean(value(HARDWAREFLOW), FALSE);
		}
		break;
	case 1:
		setboolean(value(TAND), FALSE);
		setboolean(value(HARDWAREFLOW), TRUE);
		break;
	case 0:
	default:
		setboolean(value(TAND), FALSE);
		setboolean(value(HARDWAREFLOW), FALSE);
		break;
	}
	setcharacter(value(ESCAPE), useresc);
	setboolean(value(VERBOSE), FALSE);
	if (HD)
		setboolean(value(LECHO), TRUE);
	if (HW) {
		if (ttysetup((speed_t)BR) != 0) {
			errx(3, "unsupported speed %ld", BR);
		}
	}
	if (tip_connect()) {
		errx(1, "Connect failed");
	}
	if (!HW) {
		if (ttysetup((speed_t)BR) != 0) {
			errx(3, "unsupported speed %ld", BR);
		}
	}
}

static void
cuusage(void)
{
	(void)fprintf(stderr, "Usage: cu [options] [phone-number|\"dir\"]\n"
	    "Use cu --help for help\n");
	exit(8);
}

static void
cuhelp(void)
{
	(void)fprintf(stderr,
	    "BSD tip/cu\n"
	    "Usage: cu [options] [phone-number|\"dir\"]\n"
	    " -E,--escape char: Use this escape character\n"
	    " -F,--flow {hard,soft,none}: Use RTS/CTS, ^S/^Q, no flow control\n"
	    " -f: Use no flow control\n"
	    " --nostop: Do not use software flow control\n"
	    " -a, -p,--port port: Use this port as ACU/Dialer\n"
	    " -c,--phone number: Call this number\n"
	    " -h,--halfduplex: Echo characters locally (use \"half duplex\")\n"
	    " -e: Use even parity\n"
	    " -o: Use odd parity\n"
	    " -P,--parity {even,odd,none}: use even, odd, no parity\n"
	    " -l,--line line: Use this device (ttyXX)\n"
	    " -s,--speed,--baud speed,-#: Use this speed\n"
	    " -t: Connect via hard-wired connection\n");
	exit(0);
}
