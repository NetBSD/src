/*-
 * Copyright (c) 2009,2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Alistair Crooks (agc@NetBSD.org)
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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "hkpd.h"

#define HAVE_DAEMON	1

/* set the home directory value to "home/subdir" */
static int
set_homedir(netpgp_t *netpgp, char *home, const char *subdir, const int quiet)
{
	struct stat	st;
	char		d[MAXPATHLEN];

	if (home == NULL) {
		if (!quiet) {
			(void) fprintf(stderr, "NULL HOME directory\n");
		}
		return 0;
	}
	(void) snprintf(d, sizeof(d), "%s%s", home, (subdir) ? subdir : "");
	if (stat(d, &st) == 0) {
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			netpgp_setvar(netpgp, "homedir", d);
			return 1;
		}
		(void) fprintf(stderr, "netpgp: homedir \"%s\" is not a dir\n",
					d);
		return 0;
	}
	if (!quiet) {
		(void) fprintf(stderr,
			"netpgp: warning homedir \"%s\" not found\n", d);
	}
	return 1;
}

int
main(int argc, char **argv)
{
	netpgp_t	 netpgp;
	char		*host;
	int		 daemonise;
	int		 port;
	int		 sock;
	int		 i;

	(void) memset(&netpgp, 0x0, sizeof(netpgp));
	/* set some defaults */
	set_homedir(&netpgp, getenv("HOME"), "/.gnupg", 1);
	port = 11371;
	host = strdup("localhost");
	daemonise = 1;
	while ((i = getopt(argc, argv, "DH:Vh:p:v:")) != -1) {
		switch(i) {
		case 'D':
			daemonise = 0;
			break;
		case 'H':
			set_homedir(&netpgp, optarg, NULL, 0);
			break;
		case 'V':
			printf("%s: Version %d\n", *argv, HKPD_VERSION);
			exit(EXIT_SUCCESS);
		case 'h':
			(void) free(host);
			host = strdup(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'v':
			netpgp_set_debug(optarg);
			break;
		default:
			break;
		}
	}
#ifdef HAVE_DAEMON
	/* if we are supposed to be a daemon, detach from controlling tty */
	if (daemonise && daemon(0, 0) < 0) {
		(void) fprintf(stderr, "daemon() failed\n");
		exit(EXIT_FAILURE);
	}
#endif 
	if (!netpgp_init(&netpgp)) {
		(void) fprintf(stderr, "can't initialise\n");
		exit(EXIT_FAILURE);
	}
	if ((sock = hkpd_sock_get()) < 0) {
		(void) fprintf(stderr,"hkpd: can't get a socket\n");
		exit(EXIT_FAILURE);
	}
	if (hkpd_sock_bind(sock, host, port) < 0) {
		(void) fprintf(stderr,"hkpd: can't connect socket\n");
		exit(EXIT_FAILURE);
	}
	hkpd(&netpgp, sock);
}
