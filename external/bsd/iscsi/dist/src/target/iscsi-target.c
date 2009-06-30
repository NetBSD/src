/* $NetBSD: iscsi-target.c,v 1.2 2009/06/30 02:44:53 agc Exp $ */

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@netbsd.org)
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
#include "config.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
 
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
    
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "iscsi.h"


int
main(int argc, char **argv)
{
	iscsi_target_t	tgt;
	char		buf[32];
	int		detach_me_harder;
	int		i;

	detach_me_harder = 1;
	iscsi_target_set_defaults(&tgt);
	while ((i = getopt(argc, argv, "46b:Df:p:s:t:Vv:")) != -1) {
		switch (i) {
		case '4':
		case '6':
			(void) snprintf(buf, sizeof(buf), "%d", i);
			iscsi_target_setvar(&tgt, "address family", buf);
			break;
		case 'b':
			iscsi_target_setvar(&tgt, "blocklen", optarg);
			break;
		case 'D':
			detach_me_harder = 0;
			break;
		case 'f':
			iscsi_target_setvar(&tgt, "configfile", optarg);
			break;
		case 'p':
			iscsi_target_setvar(&tgt, "target port", optarg);
			break;
		case 's':
			if ((i = atoi(optarg)) > 0 && i < 128) {
				iscsi_target_setvar(&tgt, "max sessions", optarg);
			}
			break;
		case 't':
			iscsi_target_setvar(&tgt, "iqn", optarg);
			break;
		case 'V':
			(void) printf("\"%s\" %s\nPlease send all bug reports to %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
		case 'v':
			if (strcmp(optarg, "net") == 0) {
				iscsi_target_setvar(&tgt, "debug", "net");
			} else if (strcmp(optarg, "iscsi") == 0) {
				iscsi_target_setvar(&tgt, "debug", "iscsi");
			} else if (strcmp(optarg, "scsi") == 0) {
				iscsi_target_setvar(&tgt, "debug", "scsi");
			} else if (strcmp(optarg, "all") == 0) {
				iscsi_target_setvar(&tgt, "debug", "all");
			}
			break;
		}
	}

	(void) signal(SIGPIPE, SIG_IGN);

	/* Initialize target */
	if (iscsi_target_start(&tgt) != 0) {
		(void) fprintf(stderr, "iscsi_target_start() failed\n");
		exit(EXIT_FAILURE);
	}

#ifdef HAVE_DAEMON
	/* if we are supposed to be a daemon, detach from controlling tty */
	if (detach_me_harder && daemon(0, 0) < 0) {
		(void) fprintf(stderr, "daemon() failed\n");
		exit(EXIT_FAILURE);
	}
#endif

	/* write pid to a file */
	iscsi_target_write_pidfile(NULL);
	
	/* Wait for connections */
	if (iscsi_target_listen(&tgt) != 0) {
		(void) fprintf(stderr, "iscsi_target_listen() failed\n");
	}

	return EXIT_SUCCESS;
}
