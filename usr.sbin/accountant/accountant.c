/*
 * Copyright (c) 1993 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
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

static char *rcsid = "$Id: accountant.c,v 1.3 1993/05/03 04:08:17 cgd Exp $";

#include <sys/types.h>
#include <sys/file.h>
#include <sys/acct.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <syslog.h>

#include "pathnames.h"

char *acctfile = _PATH_ACCTLOG;
char *acctdev = _PATH_ACCTDEV;
int devfd = -1;
int filefd = -1;
int min_on = 2;
int max_off = 4;
int sleeptime = 15;
int debug;

#define dprintf		if (debug) printf
#define dperror		if (debug) perror

void usage(void)
{
	fprintf(stderr, "usage: accountant [-V] [-d] [-s] [-f accounting log] [-F accounting device]\n");
	fprintf(stderr, "                  [-m min_on pct] [-M max_off pct] [-t sleep time]\n");
	exit(1);
}

void reinitfiles(int num)
{
	if (devfd != -1) close(devfd);
	if ((devfd = open(acctdev, O_RDONLY, 0)) < 0) {
		perror(acctdev);
		exit(1);
	}
	if (filefd != -1) close(filefd);
	if ((filefd = open(acctfile, O_WRONLY|O_APPEND|O_CREAT, 0666)) < 0) {
		perror(acctfile);
		exit(1);
	}
}

void checkspace()
{
	struct statfs fs;
	int freespace = 1;
	int slept = 0;

	do {
		if (fstatfs(filefd, &fs) < 0) {
			perror("checkspace");
			freespace = 0;
		} else {
			if (fs.f_bavail < fs.f_blocks / (100 / min_on)) {
				freespace = 0;
			} else if (fs.f_bavail > fs.f_blocks / (100 / max_off)) {
				freespace = 1;
			}
		}
		if (!freespace) {
			if (!slept) syslog(LOG_NOTICE, "Accounting suspended");
			dprintf("going to sleep...\n");
			sleep(sleeptime);
			slept = 1;
		}
	} while (!freespace);

	if (slept) {
		syslog(LOG_NOTICE, "Accounting resumed");
		dprintf("awake and happy...\n");
	}
}

main(argc, argv)
	int argc;
	char **argv;
{
	int ch;
	int dosync = 0;
	extern char *optarg;
	extern int optind;
	FILE *pidf;

	while ((ch = getopt(argc, argv, "Vdsf:F:m:M:t:?")) != EOF) {
		switch ((char) ch) {
		case 'V':	/* version */
			fprintf(stderr, "version: %s\n", rcsid);
			exit(0);
			break;
		case 'd':	/* debug it */
			debug = 1;
			break;
		case 's':	/* force syncs after writes */
			dosync = 1;
			break;
		case 'f':	/* set log file */
			acctfile = optarg;
			break;
		case 'F':	/* set device */
			acctdev = optarg;
			break;
		case 'm':	/* set min free */
			min_on = atoi(optarg);
			if (min_on < 0 || min_on > 100)
				usage();
			break;
		case 'M':	/* set max free */
			max_off = atoi(optarg);
			if (max_off < 0 || max_off > 100)
				usage();
			break;
		case 't':	/* set sleep time */
			sleeptime = atoi(optarg);
			if (sleeptime < 1)
				usage();
			break;
		case '?':
		default:
			usage();
		}
	}

	if (argc -= optind)
		usage();

	openlog("accountant", LOG_PERROR, LOG_DAEMON);

	if (!debug)
		daemon(0, 0);

	dprintf("%s: in debugging mode\n", argv[0]);

	signal(SIGHUP, reinitfiles);
	reinitfiles(0);
	pidf = fopen(_PATH_ACCOUTANT_PID, "w");
	if (pidf != NULL) {
		fprintf(pidf, "%d\n", getpid());
		fclose(pidf);
	}

	for (;;) {
		struct acct abuf;
		int rv;

		rv = read(devfd, &abuf, sizeof(struct acct));
		if (rv == sizeof(struct acct)) {
			checkspace();
			dprintf("accounting record copied for: %s\n",
				abuf.ac_comm);
			rv = write(filefd, &abuf, sizeof(struct acct));
			if (rv != sizeof(struct acct)) {
				if (rv < 0) {
					dperror("write");
				} else {
					dprintf("weird write result: %d\n", rv);
				}
			}
			if (dosync)
				fsync(filefd);
		} else {
			if (rv < 0) {
				dperror("read");
			} else {
				dprintf("weird read result: %d\n", rv);
			}
		}
	}

	return 0;
}
