/*	$NetBSD: npfd_log.c,v 1.4 2016/12/30 19:55:46 christos Exp $	*/

/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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
__RCSID("$NetBSD: npfd_log.c,v 1.4 2016/12/30 19:55:46 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <net/if.h>

#include <stdio.h>
#include <err.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>

#include <pcap/pcap.h>
#include "npfd.h"

struct npfd_log {
	char ifname[IFNAMSIZ];
	char path[MAXPATHLEN];
	pcap_t *pcap;
	pcap_dumper_t *dumper;
};

static void
npfd_log_setfilter(npfd_log_t *ctx, const char *filter)
{
	struct bpf_program bprog;

	if (pcap_compile(ctx->pcap, &bprog, filter, 1, 0) == -1)
		errx(EXIT_FAILURE, "pcap_compile failed for `%s': %s", filter,
		    pcap_geterr(ctx->pcap));
	if (pcap_setfilter(ctx->pcap, &bprog) == -1)
		errx(EXIT_FAILURE, "pcap_setfilter failed: %s",
		    pcap_geterr(ctx->pcap));
	pcap_freecode(&bprog);
}

npfd_log_t *
npfd_log_create(const char *ifname, const char *filter, int snaplen)
{
	npfd_log_t *ctx;
	char errbuf[PCAP_ERRBUF_SIZE];

	if ((ctx = calloc(1, sizeof(*ctx))) == NULL)
		err(EXIT_FAILURE, "malloc failed");

	/*
	 * Open a live capture handle in non-blocking mode.
	 */
	snprintf(ctx->ifname, sizeof(ctx->ifname), "%s", ifname);
	ctx->pcap = pcap_create(ctx->ifname, errbuf);
	if (ctx->pcap == NULL)
		errx(EXIT_FAILURE, "pcap_create failed: %s", errbuf);

	if (pcap_setnonblock(ctx->pcap, 1, errbuf) == -1)
		errx(EXIT_FAILURE, "pcap_setnonblock failed: %s", errbuf);

	if (pcap_set_snaplen(ctx->pcap, snaplen) == -1)
		errx(EXIT_FAILURE, "pcap_set_snaplen failed: %s",
		    pcap_geterr(ctx->pcap));

	if (pcap_activate(ctx->pcap) == -1)
		errx(EXIT_FAILURE, "pcap_activate failed: %s",
		    pcap_geterr(ctx->pcap));

	if (filter)
		npfd_log_setfilter(ctx, filter);

	snprintf(ctx->path, sizeof(ctx->path), NPFD_LOG_PATH "/%s.pcap",
	    ctx->ifname);

	npfd_log_reopen(ctx, true);
	return ctx;
}

bool
npfd_log_reopen(npfd_log_t *ctx, bool die)
{
	if (ctx->dumper)
		pcap_dump_close(ctx->dumper);
	/*
	 * Open a log file to write for a given interface and dump there.
	 */
	if (access(ctx->path, F_OK) == 0)
		ctx->dumper = pcap_dump_open_append(ctx->pcap, ctx->path);
	else
		ctx->dumper = pcap_dump_open(ctx->pcap, ctx->path);
	if (ctx->dumper == NULL) {
		if (die)
			errx(EXIT_FAILURE, "pcap_dump_open failed for `%s': %s",
			    ctx->path, pcap_geterr(ctx->pcap));
		syslog(LOG_ERR, "pcap_dump_open failed for `%s': %s",
		    ctx->path, pcap_geterr(ctx->pcap));
		return false;
	}
	return true;
}

void
npfd_log_destroy(npfd_log_t *ctx)
{
	if (ctx->dumper)
		pcap_dump_close(ctx->dumper);
	if (ctx->pcap)
		pcap_close(ctx->pcap);
	free(ctx);
}

int
npfd_log_getsock(npfd_log_t *ctx)
{
	return pcap_get_selectable_fd(ctx->pcap);
}

void
npfd_log_flush(npfd_log_t *ctx)
{
	if (!ctx->dumper)
		return;
	if (pcap_dump_flush(ctx->dumper) == -1)
		syslog(LOG_ERR, "pcap_dump_flush failed for `%s': %m",
		    ctx->path);
}


void
npfd_log(npfd_log_t *ctx)
{
	pcap_dumper_t *dumper = ctx->dumper;

	pcap_dispatch(ctx->pcap, PCAP_NPACKETS, pcap_dump, (uint8_t *)dumper);
}

void
npfd_log_stats(npfd_log_t *ctx)
{
	pcap_t *pcap = ctx->pcap;
	struct pcap_stat ps;

	if (pcap_stats(pcap, &ps) == -1) {
		syslog(LOG_ERR, "pcap_stats failed: %s", pcap_geterr(pcap));
		return;
	}
	syslog(LOG_INFO, "packet statistics: %s: %u received, %u dropped",
	    ctx->ifname, ps.ps_recv, ps.ps_drop);
}
