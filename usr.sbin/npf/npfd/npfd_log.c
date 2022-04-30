/*	$NetBSD: npfd_log.c,v 1.14 2022/04/30 13:20:09 christos Exp $	*/

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
__RCSID("$NetBSD: npfd_log.c,v 1.14 2022/04/30 13:20:09 christos Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <net/if.h>

#include <stdio.h>
#include <string.h>
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
	char *filter;
	int snaplen;
	pcap_t *pcap;
	pcap_dumper_t *dumper;
};

static void
npfd_log_setfilter(npfd_log_t *ctx)
{
	struct bpf_program bprog;

	if (ctx->filter == NULL)
		return;

	if (pcap_compile(ctx->pcap, &bprog, ctx->filter, 1, 0) == -1)
		errx(EXIT_FAILURE, "pcap_compile failed for `%s': %s",
		    ctx->filter, pcap_geterr(ctx->pcap));
	if (pcap_setfilter(ctx->pcap, &bprog) == -1)
		errx(EXIT_FAILURE, "pcap_setfilter failed: %s",
		    pcap_geterr(ctx->pcap));
	pcap_freecode(&bprog);
}

static FILE *
npfd_log_gethdr(npfd_log_t *ctx, struct pcap_file_header*hdr)
{
	FILE *fp = fopen(ctx->path, "r");

	hdr->magic = 0;
	if (fp == NULL)
		return NULL;

#define TCPDUMP_MAGIC 0xa1b2c3d4

	switch (fread(hdr, sizeof(*hdr), 1, fp)) {
	case 0:
		hdr->magic = 0;
		fclose(fp);
		return NULL;
	case 1:
		if (hdr->magic != TCPDUMP_MAGIC ||
		    hdr->version_major != PCAP_VERSION_MAJOR ||
		    hdr->version_minor != PCAP_VERSION_MINOR ||
		    hdr->sigfigs != (u_int)pcap_get_tstamp_precision(ctx->pcap))
			goto out;
		break;
	default:
		goto out;
	}

	return fp;
out:
	fclose(fp);
	hdr->magic = (uint32_t)-1;
	return NULL;
}

static int
npfd_log_getsnaplen(npfd_log_t *ctx)
{
	struct pcap_file_header hdr;
	FILE *fp = npfd_log_gethdr(ctx, &hdr);
	if (fp == NULL)
		return hdr.magic == (uint32_t)-1 ? -1 : 0;
	fclose(fp);
	return hdr.snaplen;
}

static int
npfd_log_validate(npfd_log_t *ctx)
{
	struct pcap_file_header hdr;
	FILE *fp = npfd_log_gethdr(ctx, &hdr);
	size_t o, no;

	if (fp == NULL) {
		if (hdr.magic == 0)
			return 0;
		goto rename;
	}

	struct stat st;
	if (fstat(fileno(fp), &st) == -1)
		goto rename;

	size_t count = 0;
	for (o = sizeof(hdr);; count++) {
		struct {
			uint32_t sec;
			uint32_t usec;
			uint32_t caplen;
			uint32_t len;
		} pkt;
		switch (fread(&pkt, sizeof(pkt), 1, fp)) {
		case 0:
			syslog(LOG_INFO, "%zu packets read from `%s'", count,
			    ctx->path);
			fclose(fp);
			return hdr.snaplen;
		case 1:
			no = o + sizeof(pkt) + pkt.caplen;
			if (pkt.caplen > hdr.snaplen)
				goto fix;
			if (no > (size_t)st.st_size)
				goto fix;
			if (fseeko(fp, pkt.caplen, SEEK_CUR) != 0)
				goto fix;
			o = no;
			break;
		default:
			goto fix;
		}
	}

fix:
	fclose(fp);
	no = st.st_size - o;
	syslog(LOG_INFO, "%zu packets read from `%s', %zu extra bytes",
	    count, ctx->path, no);
	if (no < 10240) {
		syslog(LOG_WARNING,
		    "Incomplete last packet in `%s', truncating",
		    ctx->path);
		if (truncate(ctx->path, (off_t)o) == -1) {
			syslog(LOG_ERR, "Cannot truncate `%s': %m", ctx->path);
			goto rename;
		}
	} else {
		syslog(LOG_ERR, "Corrupt file `%s'", ctx->path);
		goto rename;
	}
	fclose(fp);
	return hdr.snaplen;
rename:
	fclose(fp);
	char tmp[MAXPATHLEN];
	if (snprintf(tmp, sizeof(tmp), "%s.XXXXXX", ctx->path) > MAXPATHLEN)
		syslog(LOG_ERR, "Temp file truncated: `%s' does not fit",
		       ctx->path);
	int fd;
	if ((fd = mkstemp(tmp)) == -1) {
		syslog(LOG_ERR, "Can't make temp file `%s': %m", tmp);
		return -1;
	}
	close(fd);
	if (rename(ctx->path, tmp) == -1) {
		syslog(LOG_ERR, "Can't rename `%s' to `%s': %m",
		    ctx->path, tmp);
		return -1;
	}
	syslog(LOG_ERR, "Renamed to `%s'", tmp);
	return 0;
}
	

npfd_log_t *
npfd_log_create(const char *filename, const char *ifname, const char *filter,
    int snaplen)
{
	npfd_log_t *ctx;

	if ((ctx = calloc(1, sizeof(*ctx))) == NULL)
		err(EXIT_FAILURE, "malloc failed");

	snprintf(ctx->ifname, sizeof(ctx->ifname), "%s", ifname);
	if (filename == NULL)
		snprintf(ctx->path, sizeof(ctx->path), NPFD_LOG_PATH "/%s.pcap",
		    ctx->ifname);
	else
		snprintf(ctx->path, sizeof(ctx->path), "%s", filename);

	if (filter != NULL) {
		ctx->filter = strdup(filter);
		if (ctx->filter == NULL)
			err(EXIT_FAILURE, "malloc failed");
	}
	ctx->snaplen = snaplen;

	/* Open a live capture handle in non-blocking mode.  */
	npfd_log_pcap_reopen(ctx);

	/* Open the log file */
	npfd_log_file_reopen(ctx, false);
	return ctx;
}


bool
npfd_log_pcap_reopen(npfd_log_t *ctx)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	int snaplen = ctx->snaplen;
	int rc;

	if (ctx->pcap != NULL)
		pcap_close(ctx->pcap);
	else
		syslog(LOG_INFO, "reopening pcap socket");

	ctx->pcap = pcap_create(ctx->ifname, errbuf);
	if (ctx->pcap == NULL)
		errx(EXIT_FAILURE, "pcap_create failed: %s", errbuf);

	if (pcap_setnonblock(ctx->pcap, 1, errbuf) == -1)
		errx(EXIT_FAILURE, "pcap_setnonblock failed: %s", errbuf);

	int sl = npfd_log_getsnaplen(ctx);
	if (sl == -1)
		errx(EXIT_FAILURE, "corrupt log file `%s'", ctx->path);

	if (sl != 0 && sl != snaplen) {
		warnx("Overriding snaplen from %d to %d from `%s'", snaplen,
		    sl, ctx->path);
		snaplen = sl;
	}

	if (pcap_set_snaplen(ctx->pcap, snaplen) == -1)
		errx(EXIT_FAILURE, "pcap_set_snaplen failed: %s",
		    pcap_geterr(ctx->pcap));

	if (pcap_set_timeout(ctx->pcap, 1000) == -1)
		errx(EXIT_FAILURE, "pcap_set_timeout failed: %s",
		    pcap_geterr(ctx->pcap));

	if ((rc = pcap_activate(ctx->pcap)) != 0) {
		const char *msg = pcap_geterr(ctx->pcap);
		if (rc > 0) {
			warnx("pcap_activate warning: %s", msg);
		} else {
			errx(EXIT_FAILURE, "pcap_activate failed: %s", msg);
		}
	}

	npfd_log_setfilter(ctx);
	return true;
}

bool
npfd_log_file_reopen(npfd_log_t *ctx, bool die)
{
	mode_t omask = umask(077);

	if (ctx->dumper)
		pcap_dump_close(ctx->dumper);
	/*
	 * Open a log file to write for a given interface and dump there.
	 */
	switch (npfd_log_validate(ctx)) {
	case -1:
		syslog(LOG_ERR, "Giving up");
		exit(EXIT_FAILURE);
		/*NOTREACHED*/
	case 0:
		ctx->dumper = pcap_dump_open(ctx->pcap, ctx->path);
		break;
	default:
		ctx->dumper = pcap_dump_open_append(ctx->pcap, ctx->path);
		break;
	}
	(void)umask(omask);

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


int
npfd_log(npfd_log_t *ctx)
{
	pcap_dumper_t *dumper = ctx->dumper;

	return pcap_dispatch(ctx->pcap, PCAP_NPACKETS, pcap_dump,
	    (void *)dumper);
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
