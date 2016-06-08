/*	$NetBSD: mopd.c,v 1.15 2016/06/08 01:11:49 christos Exp $	*/

/*
 * Copyright (c) 1993-96 Mats O Jansson.  All rights reserved.
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
 */
#include "port.h"
#ifndef lint
__RCSID("$NetBSD: mopd.c,v 1.15 2016/06/08 01:11:49 christos Exp $");
#endif

/*
 * mopd - MOP Dump/Load Daemon
 *
 * Usage:	mopd -a [ -d -f -v ] [ -3 | -4 ]
 *		mopd [ -d -f -v ] [ -3 | -4 ] interface
 */

#include "os.h"
#include "cmp.h"
#include "common.h"
#include "device.h"
#include "dl.h"
#include "get.h"
#include "mopdef.h"
#include "pf.h"
#include "print.h"
#include "process.h"
#include "rc.h"

/*
 * The list of all interfaces that are being listened to. 
 * "selects" on the descriptors in this list.
 */
struct if_info *iflist;

__dead static void	Usage(void);
void	mopProcess(struct if_info *, u_char *);

int     AllFlag = 0;		/* listen on "all" interfaces */
int     DebugFlag = 0;		/* print debugging messages   */
int	ForegroundFlag = 0;	/* run in foreground          */
int	VersionFlag = 0;	/* print version              */
int	Not3Flag = 0;		/* Not MOP V3 messages.       */
int	Not4Flag = 0;		/* Not MOP V4 messages.       */
int	promisc = 1;		/* Need promisc mode    */
const char *MopdDir = MOP_FILE_PATH;  /* Path to mop directory  */

int
main(int argc, char  **argv)
{
	int	c, pid;

	extern char version[];

	while ((c = getopt(argc, argv, "34adfs:v")) != -1) {
		switch (c) {
			case '3':
				Not3Flag++;
				break;
			case '4':
				Not4Flag++;
				break;
			case 'a':
				AllFlag++;
				break;
			case 'd':
				DebugFlag++;
				break;
			case 'f':
				ForegroundFlag++;
				break;
			case 's':
				MopdDir = optarg;
				break;
			case 'v':
				VersionFlag++;
				break;
			default:
				Usage();
				/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (VersionFlag) {
		fprintf(stdout,"%s: version %s\n", getprogname(), version);
		exit(0);
	}

	if ((AllFlag && argc != 0) || (!AllFlag && argc == 0) ||
	    (Not3Flag && Not4Flag))
		Usage();

	/* All error reporting is done through syslogs. */
	openlog("mopd", LOG_PID, LOG_DAEMON);

	if ((!ForegroundFlag) && DebugFlag)
		fprintf(stdout,
		    "%s: not running as daemon, -d given.\n", getprogname());

	if ((!ForegroundFlag) && (!DebugFlag)) {
		pid = fork();
		if (pid > 0)
			/* Parent exits, leaving child in background. */
			exit(0);
		else
			if (pid == -1) {
				syslog(LOG_ERR, "cannot fork");
				exit(0);
			}

		/* Fade into the background */
		daemon(0, 0);
		pidfile(NULL);
	}

	syslog(LOG_INFO, "%s %s started.", getprogname(), version);

	if (AllFlag)
 		deviceInitAll();
	else {
		while (argc--)
			deviceInitOne(*argv++);
	}

	Loop();
	/* NOTREACHED */
	return (0);
}

static void
Usage(void)
{
	(void) fprintf(stderr, "usage: %s -a [ -d -f -v ] [ -3 | -4 ]\n",
	    getprogname());
	(void) fprintf(stderr, "       %s [ -d -f -v ] [ -3 | -4 ]\n",
	    getprogname());
	(void) fprintf(stderr, "           interface [...]\n");
	exit(1);
}

/*
 * Process incomming packages.
 */
void
mopProcess(struct if_info *ii, u_char *pkt)
{
	const u_char	*dst, *src;
	u_short  ptype;
	int	 idx, trans, len;

	/* We don't known with transport, Guess! */

	trans = mopGetTrans(pkt, 0);

	/* Ok, return if we don't wan't this message */

	if ((trans == TRANS_ETHER) && Not3Flag) return;
	if ((trans == TRANS_8023) && Not4Flag)	return;

	idx = 0;
	mopGetHeader(pkt, &idx, &dst, &src, &ptype, &len, trans);

	/*
	 * Ignore our own transmissions
	 *
	 */	
	if (mopCmpEAddr(ii->eaddr,src) == 0)
		return;

	switch(ptype) {
	case MOP_K_PROTO_DL:
		mopProcessDL(stdout, ii, pkt, &idx, dst, src, trans, len);
		break;
	case MOP_K_PROTO_RC:
		mopProcessRC(stdout, ii, pkt, &idx, dst, src, trans, len);
		break;
	default:
		break;
	}
}
