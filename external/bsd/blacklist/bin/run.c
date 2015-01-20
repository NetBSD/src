/*	$NetBSD: run.c,v 1.1 2015/01/20 00:19:21 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__RCSID("$NetBSD: run.c,v 1.1 2015/01/20 00:19:21 christos Exp $");

#include <stdio.h>
#include <util.h>
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <inttypes.h>
#include <syslog.h>
#include <string.h>
#include <netinet/in.h>

#include "run.h"
#include "internal.h"

extern char **environ;

static char *
run(const char *cmd, ...)
{
	const char *argv[20];
	size_t i;
	va_list ap;
	FILE *fp;
	char buf[BUFSIZ], *res;

	argv[0] = "control";
	argv[1] = cmd;
	argv[2] = rulename;
	va_start(ap, cmd);
	for (i = 3; i < __arraycount(argv) &&
	    (argv[i] = va_arg(ap, char *)) != NULL; i++)
		continue;
	va_end(ap);
		
	if (debug) {
		printf("run %s [", controlprog);
		for (i = 0; argv[i]; i++)
			printf(" %s", argv[i]);
		printf("]\n");
	}

	fp = popenve(controlprog, __UNCONST(argv), environ, "r");
	if (fp == NULL) {
		(*lfun)(LOG_ERR, "popen %s failed (%m)", controlprog);
		return NULL;
	}
	if (fgets(buf, sizeof(buf), fp) != NULL)
		res = strdup(buf);
	else
		res = NULL;
	pclose(fp);
	if (debug)
		printf("%s returns %s\n", cmd, res);
	return res;
}

void
run_flush(void)
{
	free(run("flush", NULL));
}

int
run_add(int proto, in_port_t port, const struct sockaddr_storage *ss)
{
	const char *prname;
	char poname[64], adname[128], *rv;
	int id, e;

	switch (proto) {
	case IPPROTO_TCP:
		prname = "tcp";
		break;
	case IPPROTO_UDP:
		prname = "udp";
		break;
	default:
		(*lfun)(LOG_ERR, "%s: bad protocol %d", __func__, proto);
		return -1;
	}

	snprintf(poname, sizeof(poname), "%d", port);
	sockaddr_snprintf(adname, sizeof(adname), "%a", (const void *)ss);

	rv = run("add", prname, adname, poname, NULL);
	if (rv == NULL)
		return -1;
	id = (int)strtoi(rv, NULL, 0, 0, INT_MAX, &e);
	if (e) {
		(*lfun)(LOG_ERR, "%s: bad number %s (%m)", __func__, rv);
		id = -1;
	}
	free(rv);
	return id;
}

void
run_rem(int id)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d", id);
	free(run("rem", buf, NULL));
}
