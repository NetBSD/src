/*-
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: src/sbin/gpt/add.c,v 1.14 2006/06/22 22:05:28 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: add.c,v 1.33 2015/12/01 19:25:24 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"
#include "gpt_private.h"

static gpt_uuid_t type;
static off_t alignment, block, sectors, size;
static unsigned int entry;
static uint8_t *name;
static int cmd_add(gpt_t, int, char *[]);

static const char *addhelp[] = {
    "[-a alignment] [-b blocknr] [-i index] [-l label]",
    "[-s size] [-t type]",
};

struct gpt_cmd c_add = {
	"add",
	cmd_add,
	addhelp, __arraycount(addhelp),
	0,
};

#define usage() gpt_usage(NULL, &c_add)

static int
add(gpt_t gpt)
{
	map_t map;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent, e;
	unsigned int i;
	off_t alignsecs;
	char buf[128];
	
	if ((hdr = gpt_hdr(gpt)) == NULL)
		return -1;

	ent = NULL;

	if (entry > le32toh(hdr->hdr_entries)) {
		gpt_warnx(gpt, "index %u out of range (%u max)",
		    entry, le32toh(hdr->hdr_entries));
		return -1;
	}

	if (entry > 0) {
		i = entry - 1;
		ent = gpt_ent_primary(gpt, i);
		if (!gpt_uuid_is_nil(ent->ent_type)) {
			gpt_warnx(gpt, "Entry at index %u is not free", entry);
			return -1;
		}
	} else {
		/* Find empty slot in GPT table. */
		for (i = 0; i < le32toh(hdr->hdr_entries); i++) {
			ent = gpt_ent_primary(gpt, i);
			if (gpt_uuid_is_nil(ent->ent_type))
				break;
		}
		if (i == le32toh(hdr->hdr_entries)) {
			gpt_warnx(gpt, "No available table entries");
			return -1;
		}
	}

	if (alignment > 0) {
		alignsecs = alignment / gpt->secsz;
		map = map_alloc(gpt, block, sectors, alignsecs);
		if (map == NULL) {
			gpt_warnx(gpt, "Not enough space available on "
			      "device for an aligned partition");
			return -1;
		}
	} else {
		map = map_alloc(gpt, block, sectors, 0);
		if (map == NULL) {
			gpt_warnx(gpt, "Not enough space available on device");
			return -1;
		}
	}

	memset(&e, 0, sizeof(e));
	gpt_uuid_copy(e.ent_type, type);
	e.ent_lba_start = htole64(map->map_start);
	e.ent_lba_end = htole64(map->map_start + map->map_size - 1LL);
	if (name != NULL)
		utf8_to_utf16(name, e.ent_name, 36);

	memcpy(ent, &e, sizeof(e));
	gpt_write_primary(gpt);

	ent = gpt_ent_backup(gpt, i);
	memcpy(ent, &e, sizeof(e));
	gpt_write_backup(gpt);

	gpt_uuid_snprintf(buf, sizeof(buf), "%d", type);
	gpt_msg(gpt, "Partition %d added: %s %" PRIu64 " %" PRIu64, i + 1,
	    buf, map->map_start, map->map_size);
	return 0;
}

static int
cmd_add(gpt_t gpt, int argc, char *argv[])
{
	int ch;
	int64_t human_num;

	while ((ch = getopt(argc, argv, GPT_AIS "bl:t:")) != -1) {
		switch(ch) {
		case 'b':
			if (block > 0)
				return usage();
			if (dehumanize_number(optarg, &human_num) < 0)
				return usage();
			block = human_num;
			if (block < 1)
				return usage();
			break;
		case 'l':
			if (name != NULL)
				return usage();
			name = (uint8_t *)strdup(optarg);
			break;
		case 't':
			if (!gpt_uuid_is_nil(type))
				return usage();
			if (gpt_uuid_parse(optarg, type) != 0)
				return usage();
			break;
		default:
			if (gpt_add_ais(gpt, &alignment, &entry, &size, ch)
			    == -1)
				return usage();
			break;
		}
	}

	if (argc != optind)
		return usage();

	/* Create NetBSD FFS partitions by default. */
	if (gpt_uuid_is_nil(type)) {
		gpt_uuid_create(GPT_TYPE_NETBSD_FFS, type, NULL, 0);
	}

	if (optind != argc)
		return usage();

	if ((sectors = gpt_check_ais(gpt, alignment, entry, size)) == -1)
		return -1;

	return add(gpt);
}
