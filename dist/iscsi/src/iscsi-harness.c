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

#include <sys/types.h>
#include <sys/param.h>

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "scsi_cmd_codes.h"

#include "iscsi.h"
#include "initiator.h"
#include "tests.h"


int 
main(int argc, char **argv)
{
	struct sigaction	act;
	struct passwd		*pwp;
	char			hostname[1024];
	char		       *host;
	char		       *user;
	int             	tgtlo = 0;
	int             	tgthi = CONFIG_INITIATOR_NUM_TARGETS;
	int             	target = -1;
        int			digest_type;
        int			mutual_auth;
        int			auth_type;
	int             	lun = 0;
	int             	i, j;
	int             	iterations;

	/* Check args */

	iterations = 1;
	user = NULL;
	(void) gethostname(host = hostname, sizeof(hostname));
	digest_type = DigestNone;
	auth_type = AuthNone;
	mutual_auth = 0;

	while ((i = getopt(argc, argv, "a:d:h:l:n:t:u:V")) != -1) {
		switch(i) {
		case 'a':
			if (strcasecmp(optarg, "chap") == 0) {
				auth_type = AuthCHAP;
			} else if (strcasecmp(optarg, "kerberos") == 0) {
				auth_type = AuthKerberos;
			} else if (strcasecmp(optarg, "srp") == 0) {
				auth_type = AuthSRP;
			}
			break;
		case 'd':
			if (strcasecmp(optarg, "header") == 0) {
				digest_type = DigestHeader;
			} else if (strcasecmp(optarg, "data") == 0) {
				digest_type = DigestData;
			} else if (strcasecmp(optarg, "both") == 0 || strcasecmp(optarg, "all") == 0) {
				digest_type = (DigestHeader | DigestData);
			}
			break;
		case 'h':
			host = optarg;
			break;
		case 'l':
			lun = atoi(optarg);
			break;
		case 'n':
			iterations = atoi(optarg);
			break;
		case 't':
			target = atoi(optarg);
			break;
		case 'u':
			user = optarg;
			break;
		case 'V':
			(void) printf("\"%s\" %s\nPlease send all bug reports to %s\n", PACKAGE_NAME, PACKAGE_VERSION, PACKAGE_BUGREPORT);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
		default:
			(void) fprintf(stderr, "%s: unknown option `%c'", *argv, i);
		}
	}

	if (user == NULL) {
		if ((pwp = getpwuid(geteuid())) == NULL) {
			iscsi_trace_error(__FILE__, __LINE__, "can't find user information\n");
			exit(EXIT_FAILURE);
		}
		user = pwp->pw_name;
	}

	if (target != -1) {
		if (target >= CONFIG_INITIATOR_NUM_TARGETS) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator only configured with %d targets\n", CONFIG_INITIATOR_NUM_TARGETS);
			exit(EXIT_FAILURE);
		}
		tgtlo = target;
		tgthi = target + 1;
	}
	if (argc == 1) {
		(void) fprintf(stderr, "usage: %s [-V] [-a auth-type] [-d digest-type] [-h hostname] [-l lun] [-n iterations] [-t target] [-u user]\n", *argv);
		exit(EXIT_FAILURE);
	}
	for (j = 0; j < iterations; j++) {

		printf("<ITERATION %d>\n", j);

		/* Ignore sigpipe */

		act.sa_handler = SIG_IGN;
		sigaction(SIGPIPE, &act, NULL);

		/* Initialize Initiator */
		if (initiator_init(host, user, auth_type, mutual_auth, digest_type) == -1) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_init() failed\n");
			return -1;
		}
		/* Run tests for each target */

		for (i = tgtlo; i < tgthi; i++) {
			if (test_all(i, lun) != 0) {
				iscsi_trace_error(__FILE__, __LINE__, "test_all() failed\n");
				return -1;
			}
		}

		/* Shutdown Initiator */

		if (initiator_shutdown() == -1) {
			iscsi_trace_error(__FILE__, __LINE__, "initiator_shutdown() failed\n");
			return -1;
		}
	}

	printf("\n");
	printf("************************************\n");
	printf("* ALL TESTS COMPLETED SUCCESSFULLY *\n");
	printf("************************************\n");
	printf("\n");

	exit(EXIT_SUCCESS);
}
