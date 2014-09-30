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
__RCSID("$NetBSD: set.c,v 1.5 2014/09/30 18:00:00 christos Exp $");
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

static unsigned int entry;
static uint64_t attributes;

const char setmsg[] = "set -a attribute -i index device ...";

__dead static void
usage_set(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), setmsg);
	exit(1);
}

static void
set(int fd)
{
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;
	

	gpt = map_find(MAP_TYPE_PRI_GPT_HDR);
	ent = NULL;
	if (gpt == NULL) {
		warnx("%s: error: no primary GPT header; run create or recover",
		    device_name);
		return;
	}

	tpg = map_find(MAP_TYPE_SEC_GPT_HDR);
	if (tpg == NULL) {
		warnx("%s: error: no secondary GPT header; run recover",
		    device_name);
		return;
	}

	tbl = map_find(MAP_TYPE_PRI_GPT_TBL);
	lbt = map_find(MAP_TYPE_SEC_GPT_TBL);
	if (tbl == NULL || lbt == NULL) {
		warnx("%s: error: run recover -- trust me", device_name);
		return;
	}

	hdr = gpt->map_data;
	if (entry > le32toh(hdr->hdr_entries)) {
		warnx("%s: error: index %u out of range (%u max)", device_name,
		    entry, le32toh(hdr->hdr_entries));
		return;
	}

	i = entry - 1;
	ent = (void*)((char*)tbl->map_data + i *
	    le32toh(hdr->hdr_entsz));
	if (gpt_uuid_is_nil(ent->ent_type)) {
		warnx("%s: error: entry at index %u is unused",
		    device_name, entry);
		return;
	}

	ent->ent_attr |= attributes;

	hdr->hdr_crc_table = htole32(crc32(tbl->map_data,
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

	gpt_write(fd, gpt);
	gpt_write(fd, tbl);

	hdr = tpg->map_data;
	ent = (void*)((char*)lbt->map_data + i * le32toh(hdr->hdr_entsz));
	ent->ent_attr |= attributes;

	hdr->hdr_crc_table = htole32(crc32(lbt->map_data,
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

	gpt_write(fd, lbt);
	gpt_write(fd, tpg);

	printf("Partition %d attributes updated\n", entry);
}

int
cmd_set(int argc, char *argv[])
{
	char *p;
	int ch, fd;

	while ((ch = getopt(argc, argv, "a:i:")) != -1) {
		switch(ch) {
		case 'a':
			if (strcmp(optarg, "biosboot") == 0)
				attributes |= GPT_ENT_ATTR_LEGACY_BIOS_BOOTABLE;
			else if (strcmp(optarg, "bootme") == 0)
				attributes |= GPT_ENT_ATTR_BOOTME;
			else if (strcmp(optarg, "bootonce") == 0)
				attributes |= GPT_ENT_ATTR_BOOTONCE;
			else if (strcmp(optarg, "bootfailed") == 0)
				attributes |= GPT_ENT_ATTR_BOOTFAILED;
			else
				usage_set();
			break;
		case 'i':
			if (entry > 0)
				usage_set();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				usage_set();
			break;
		default:
			usage_set();
		}
	}

	if (argc == optind)
		usage_set();

	if (entry == 0 || attributes == 0)
		usage_set();

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}

		set(fd);

		gpt_close(fd);
	}

	return 0;
}
