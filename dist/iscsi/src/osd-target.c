/*
 * IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING. By downloading, copying, installing or
 * using the software you agree to this license. If you do not agree to this license, do not download, install,
 * copy or use the software.
 *
 * Intel License Agreement
 *
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *
 * -Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *  following disclaimer.
 *
 * -Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
 *  following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * -The name of Intel Corporation may not be used to endorse or promote products derived from this software
 *  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#define EXTERN

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
  
#include <stdio.h>
#include <stdlib.h>
    
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <unistd.h>

#include "iscsi.h"
#include "util.h"
#include "target.h"
#include "device.h"

#include "conffile.h"
#include "storage.h"

/*
 * Globals
 */

static int      g_main_pid;
static globals_t	g;

/*
 * Control-C handler
 */

/* ARGSUSED0 */
static void 
handler(int s)
{
	if (ISCSI_GETPID != g_main_pid)
		return;
	if (target_shutdown(&g) != 0) {
		TRACE_ERROR("target_shutdown() failed\n");
		return;
	}
	return;
}

int 
main(int argc, char **argv)
{
	char            TargetName[1024];
	int             i;

	(void) memset(&g, 0x0, sizeof(g));

	/* set defaults */
	(void) strlcpy(TargetName, DEFAULT_TARGET_NAME, sizeof(TargetName));
	g.port = ISCSI_PORT;

	while ((i = getopt(argc, argv, "c:l:d:" "p:t:v:")) != -1) {
		switch (i) {
		case 'c':
			device_set_var("capacity", optarg);
			break;
		case 'd':
			device_set_var("directory", optarg);
			break;
		case 'l':
			device_set_var("luns", optarg);
			break;
		case 'p':
			g.port = (uint16_t) atoi(optarg);
			break;
		case 't':
			(void) strlcpy(TargetName, optarg, sizeof(TargetName));
			break;
		case 'v':
			if (strcmp(optarg, "net") == 0) {
				set_debug("net");
			} else if (strcmp(optarg, "iscsi") == 0) {
				set_debug("iscsi");
			} else if (strcmp(optarg, "scsi") == 0) {
				set_debug("scsi");
			} else if (strcmp(optarg, "osd") == 0) {
				set_debug("osd");
			} else if (strcmp(optarg, "all") == 0) {
				set_debug("all");
			}
			break;
		}
	}

	(void) signal(SIGPIPE, SIG_IGN);

	(void) signal(SIGINT, handler);
	g_main_pid = ISCSI_GETPID;

	/* Initialize target */
	if (optind == argc) {
		if (target_init(&g, TargetName, NULL) != 0) {
			TRACE_ERROR("target_init() failed\n");
			exit(EXIT_FAILURE);
		}
	} else {
		for (i = optind ; i < argc ; i++) {
			if (target_init(&g, TargetName, argv[i]) != 0) {
				TRACE_ERROR("target_init() failed\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	/* Wait for connections */
	if (target_listen(&g) != 0) {
		TRACE_ERROR("target_listen() failed\n");
	}
	return EXIT_SUCCESS;
}
