/*	$NetBSD: npfd_log.c,v 1.1 2016/12/27 22:20:00 rmind Exp $	*/

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
__RCSID("$NetBSD: npfd_log.c,v 1.1 2016/12/27 22:20:00 rmind Exp $");

#include <stdio.h>
#include <inttypes.h>
#include <limits.h>

#include <pcap/pcap.h>

struct npfd_log {
	pcap_t *	pcap;
	pcap_dumper_t *	dumper;
};

npfd_log_t *
npfd_log_create(unsigned if_idx)
{
	npfd_log_t *ctx;
	char errbuf[PCAP_ERRBUF_SIZE];
	char ifname[IFNAMSIZ], path[PATH_MAX];
	FILE *fp;

	if ((ctx = calloc(1, sizeof(npfd_log_t))) == NULL) {
		syslog(LOG_ERR, "malloc failed: %m");
		return NULL;
	}

	/*
	 * Open a live capture handle in non-blocking mode.
	 */
	snprintf(ifname, sizeof(ifname), NPFD_NPFLOG "%u", if_idx);
	pcap = pcap_create(ifname, errbuf);
	if ((ctx->pcap = pcap) == NULL) {
		syslog(LOG_ERR, "pcap_create failed: %s", errbuf);
		goto err;
	}
	if (pcap_setnonblock(pcap, 1, errbuf) == -1) {
		syslog(LOG_ERR, "pcap_setnonblock failed: %s", errbuf);
		goto err;
	}
	pcap_set_snaplen(pcap, snaplen);

	/*
	 * Open a log file to write for a given interface and dump there.
	 */
	snprintf(path, sizeof(path), "%s/%s%s", NPFD_LOG_PATH, ifname, ".pcap");
	if ((fp = fopen(path, "w")) == NULL) {
		syslog(LOG_ERR, "open failed: %m");
		goto err;
	}
	if ((ctx->dumper = pcap_dump_fopen(pcap, fp)) == NULL) {
		syslog(LOG_ERR, "pcap_dump_fopen failed: %s", errbuf);
		goto err;
	}
	return ctx;
err:
	if (!ctx->dumper && fp) {
		fclose(fp);
	}
	npfd_log_destroy(ctx);
	return NULL;
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

	if (pcap_stats(pcap, &ps) == -1)
		syslog(LOG_ERR, "pcap_stats failed: %s", pcap_geterr(pcap));
		return;
	}
	syslog(LOG_NOTICE, "packet statistics: %u received, %u dropped",
	    ps.ps_recv, ps.ps_drop);
}
