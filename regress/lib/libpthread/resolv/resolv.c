/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#include <pthread.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stringlist.h>

#define NTHREADS	10
#define NHOSTS		100
#define WS		" \t\n\r"

static StringList *hosts = NULL;

static void usage(void)  __attribute__((__noreturn__));
static void load(const char *);
static void resolvone(int);
static void *resolvloop(void *);
static void run(int *);

static void
usage(void)
{
	(void)fprintf(stderr,
	    "Usage: %s [-h <nhosts>] [-n <nthreads>] <file> ...\n",
	    getprogname());
	exit(1);
}

static void
load(const char *fname)
{
	FILE *fp;
	size_t len;
	char *line;

	if ((fp = fopen(fname, "r")) == NULL)
		err(1, "Cannot open `%s'", fname);
	while ((line = fgetln(fp, &len)) != NULL) {
		char c = line[len];
		char *ptr;
		line[len] = '\0';
		for (ptr = strtok(line, WS); ptr; ptr = strtok(NULL, WS))
			sl_add(hosts, strdup(ptr));
		line[len] = c;
	}

	(void)fclose(fp);
}

static void
resolvone(int n)
{
	char buf[1024];
	pthread_t self = pthread_self();
	size_t i = (random() & 0x0fffffff) % hosts->sl_cur;
	char *host = hosts->sl_str[i];
	struct addrinfo *res;
	int error, len;
	len = snprintf(buf, sizeof(buf), "%p: %d resolving %s %d\n", self, n,
	    host, (int)i);
	(void)write(STDOUT_FILENO, buf, len);
	error = getaddrinfo(host, NULL, NULL, &res);
	len = snprintf(buf, sizeof(buf), "%p: host %s %s\n",
	    self, host, error ? "not found" : "ok");
	(void)write(STDOUT_FILENO, buf, len);
	if (error == 0)
		freeaddrinfo(res);
}

static void *
resolvloop(void *p)
{
	int nhosts = *(int *)p;
	printf("nhosts = %d\n", nhosts);
	while (nhosts--)
		resolvone(nhosts);
	return NULL;
}

static void
run(int *nhosts)
{
	pthread_t self = pthread_self();
	if (pthread_create(&self, NULL, resolvloop, nhosts) != 0)
		err(1, "pthread_create");
}

int
main(int argc, char *argv[])
{
	int nthreads = NTHREADS;
	int nhosts = NHOSTS;
	int i, c;
	hosts = sl_init();

	srandom(1234);

	while ((c = getopt(argc, argv, "h:t:")) != -1)
		switch (c) {
		case 'h':
			nhosts = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		default:
			usage();
		}
	for (i = optind; i < argc; i++)
		load(argv[i]);

	if (hosts->sl_cur == 0)
		usage();
	for (i = 0; i < nthreads; i++)
		run(&nhosts);
	sleep(100000);
	return 0;
}
