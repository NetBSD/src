/*	$NetBSD: run.c,v 1.9 2015/01/22 15:29:27 christos Exp $	*/

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/cdefs.h>
__RCSID("$NetBSD: run.c,v 1.9 2015/01/22 15:29:27 christos Exp $");

#include <stdio.h>
#ifdef HAVE_UTIL_H
#include <util.h>
#endif
#include <stdarg.h>
#include <limits.h>
#include <stdlib.h>
#include <inttypes.h>
#include <syslog.h>
#include <string.h>
#include <netinet/in.h>

#include "run.h"
#include "conf.h"
#include "internal.h"

extern char **environ;

static char *
run(const char *cmd, const char *name, ...)
{
	const char *argv[20];
	size_t i;
	va_list ap;
	FILE *fp;
	char buf[BUFSIZ], *res;

	argv[0] = "control";
	argv[1] = cmd;
	argv[2] = name;
	va_start(ap, name);
	for (i = 3; i < __arraycount(argv) &&
	    (argv[i] = va_arg(ap, char *)) != NULL; i++)
		continue;
	va_end(ap);
		
	if (debug) {
		(*lfun)(LOG_DEBUG, "run %s [", controlprog);
		for (i = 0; argv[i]; i++)
			(*lfun)(LOG_DEBUG, " %s", argv[i]);
		(*lfun)(LOG_DEBUG, "]\n");
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
		(*lfun)(LOG_DEBUG, "%s returns %s\n", cmd, res);
	return res;
}

void
run_flush(const struct conf *c)
{
	free(run("flush", c->c_name, NULL));
}

int
run_change(const char *how, const struct conf *c, 
    const struct sockaddr_storage *ss, char *id, size_t len)
{
	const char *prname;
	char poname[64], adname[128], *rv;
	size_t off;

	switch (c->c_proto) {
	case IPPROTO_TCP:
		prname = "tcp";
		break;
	case IPPROTO_UDP:
		prname = "udp";
		break;
	default:
		(*lfun)(LOG_ERR, "%s: bad protocol %d", __func__, c->c_proto);
		return -1;
	}

	snprintf(poname, sizeof(poname), "%d", c->c_port);
	sockaddr_snprintf(adname, sizeof(adname), "%a", (const void *)ss);

	rv = run(how, c->c_name, prname, adname, poname, id, NULL);
	if (rv == NULL)
		return -1;
	if (len != 0) {
		rv[strcspn(rv, "\n")] = '\0';
		off = strncmp(rv, "OK ", 3) == 0 ? 3 : 0;
		strlcpy(id, rv + off, len);
	}
	free(rv);
	return 0;
}
