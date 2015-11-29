/*-
 * Copyright (c) 2015 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by John Nemeth
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: header.c,v 1.3 2015/11/29 00:14:46 christos Exp $");
#endif

#include <sys/types.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"

const char headermsg[] = "header device ...";

__dead static void
usage_header(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), headermsg);
	exit(1);
}

static void
header(void)
{
	unsigned int revision;
	map_t *gpt;
	struct gpt_hdr *hdr;
	char buf[128];
#ifdef HN_AUTOSCALE
	char human_num[5];
#endif

#ifdef HN_AUTOSCALE
	if (humanize_number(human_num, 5, mediasz,
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("Media Size: %llu (%s)\n", (long long unsigned)mediasz,
		    human_num);
	else
#endif
		printf("Media Size: %llu\n", (long long unsigned)mediasz);

	printf("Sector Size: %u\n", secsz);

#ifdef HN_AUTOSCALE
	if (humanize_number(human_num, 5, mediasz / secsz,
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("Number of Sectors: %llu (%s)\n",
		    (long long unsigned)(mediasz / secsz), human_num);
	else
#endif
		printf("Number of Sectors: %llu\n",
		    (long long unsigned)(mediasz / secsz));

	printf("\nHeader Information:\n");

	gpt = map_find(MAP_TYPE_PRI_GPT_HDR);
	if (gpt == NULL) {
		printf("- GPT Header not found\n");
		return;
	}

	hdr = gpt->map_data;
	revision = le32toh(hdr->hdr_revision);
	printf("- GPT Header Revision: %u.%u\n", revision >> 16,
	     revision & 0xffff);
	printf("- First Data Sector: %llu\n",
	    (long long unsigned)hdr->hdr_lba_start);
#ifdef HN_AUTOSCALE
	if (humanize_number(human_num, 5, hdr->hdr_lba_end,
	    "", HN_AUTOSCALE, HN_NOSPACE|HN_B) < 0)
		human_num[0] = '\0';
	if (human_num[0] != '\0')
		printf("- Last Data Sector: %llu (%s)\n",
		    (long long unsigned)hdr->hdr_lba_end, human_num);
	else
#endif
	printf("- Last Data Sector: %llu\n",
	    (long long unsigned)hdr->hdr_lba_end);
	gpt_uuid_snprintf(buf, sizeof(buf), "%d", hdr->hdr_guid);
	printf("- Media GUID: %s\n", buf);
	printf("- Number of GPT Entries: %u\n", hdr->hdr_entries);
}

int
cmd_header(int argc, char *argv[])
{
	int fd;

	if (argc == optind)
		usage_header();

	while (optind < argc) {
		fd = gpt_open(argv[optind++], 0);
		if (fd == -1)
			continue;
		header();

		gpt_close(fd);
	}

	return (0);
}
