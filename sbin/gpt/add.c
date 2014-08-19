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

#include <sys/cdefs.h>
#ifdef __FBSDID
__FBSDID("$FreeBSD: src/sbin/gpt/add.c,v 1.14 2006/06/22 22:05:28 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: add.c,v 1.11.8.2 2014/08/20 00:02:25 tls Exp $");
#endif

#include <sys/types.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include "map.h"
#include "gpt.h"

static uuid_t type;
static off_t alignment, block, sectors, size;
static unsigned int entry;
static uint8_t *name;

const char addmsg1[] = "add [-a alignment] [-b blocknr] [-i index] [-l label]";
const char addmsg2[] = "    [-s size] [-t type] device ...";

__dead static void
usage_add(void)
{

	fprintf(stderr,
	    "usage: %s %s\n"
	    "       %*s %s\n", getprogname(), addmsg1,
	    (int)strlen(getprogname()), "", addmsg2);
	exit(1);
}

static void
add(int fd)
{
	uuid_t uuid;
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	map_t *map;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;
	off_t alignsecs;
	

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

	if (entry > 0) {
		i = entry - 1;
		ent = (void*)((char*)tbl->map_data + i *
		    le32toh(hdr->hdr_entsz));
		le_uuid_dec(ent->ent_type, &uuid);
		if (!uuid_is_nil(&uuid, NULL)) {
			warnx("%s: error: entry at index %u is not free",
			    device_name, entry);
			return;
		}
	} else {
		/* Find empty slot in GPT table. */
		for (i = 0; i < le32toh(hdr->hdr_entries); i++) {
			ent = (void*)((char*)tbl->map_data + i *
			    le32toh(hdr->hdr_entsz));
			le_uuid_dec(ent->ent_type, &uuid);
			if (uuid_is_nil(&uuid, NULL))
				break;
		}
		if (i == le32toh(hdr->hdr_entries)) {
			warnx("%s: error: no available table entries",
			    device_name);
			return;
		}
	}

	if (alignment > 0) {
		alignsecs = alignment / secsz;
		map = map_alloc(block, sectors, alignsecs);
		if (map == NULL) {
			warnx("%s: error: not enough space available on "
			      "device for an aligned partition", device_name);
			return;
		}
	} else {
		map = map_alloc(block, sectors, 0);
		if (map == NULL) {
			warnx("%s: error: not enough space available on "
			      "device", device_name);
			return;
		}
	}

	le_uuid_enc(ent->ent_type, &type);
	ent->ent_lba_start = htole64(map->map_start);
	ent->ent_lba_end = htole64(map->map_start + map->map_size - 1LL);
	if (name != NULL)
		utf8_to_utf16(name, ent->ent_name, 36);

	hdr->hdr_crc_table = htole32(crc32(tbl->map_data,
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

	gpt_write(fd, gpt);
	gpt_write(fd, tbl);

	hdr = tpg->map_data;
	ent = (void*)((char*)lbt->map_data + i * le32toh(hdr->hdr_entsz));

	le_uuid_enc(ent->ent_type, &type);
	ent->ent_lba_start = htole64(map->map_start);
	ent->ent_lba_end = htole64(map->map_start + map->map_size - 1LL);
	if (name != NULL)
		utf8_to_utf16(name, ent->ent_name, 36);

	hdr->hdr_crc_table = htole32(crc32(lbt->map_data,
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

	gpt_write(fd, lbt);
	gpt_write(fd, tpg);

	printf("Partition %d added, use:\n", i + 1);
	printf("\tdkctl %s addwedge <wedgename> %" PRIu64 " %" PRIu64
	    " <type>\n", device_arg, map->map_start, map->map_size);
	printf("to create a wedge for it\n");
}

int
cmd_add(int argc, char *argv[])
{
	char *p;
	int ch, fd;
	int64_t human_num;

	while ((ch = getopt(argc, argv, "a:b:i:l:s:t:")) != -1) {
		switch(ch) {
		case 'a':
			if (alignment > 0)
				usage_add();
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_add();
			alignment = human_num;
			if (alignment < 1)
				usage_add();
			break;
		case 'b':
			if (block > 0)
				usage_add();
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_add();
			block = human_num;
			if (block < 1)
				usage_add();
			break;
		case 'i':
			if (entry > 0)
				usage_add();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				usage_add();
			break;
		case 'l':
			if (name != NULL)
				usage_add();
			name = (uint8_t *)strdup(optarg);
			break;
		case 's':
			if (sectors > 0 || size > 0)
				usage_add();
			sectors = strtoll(optarg, &p, 10);
			if (sectors < 1)
				usage_add();
			if (*p == '\0')
				break;
			if (*p == 's' || *p == 'S') {
				if (*(p + 1) == '\0')
					break;
				else
					usage_add();
			}
			if (*p == 'b' || *p == 'B') {
				if (*(p + 1) == '\0') {
					size = sectors;
					sectors = 0;
					break;
				} else
					usage_add();
			}
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_add();
			size = human_num;
			sectors = 0;
			break;
		case 't':
			if (!uuid_is_nil(&type, NULL))
				usage_add();
			if (parse_uuid(optarg, &type) != 0)
				usage_add();
			break;
		default:
			usage_add();
		}
	}

	if (argc == optind)
		usage_add();

	/* Create NetBSD FFS partitions by default. */
	if (uuid_is_nil(&type, NULL)) {
		static const uuid_t nb_ffs = GPT_ENT_TYPE_NETBSD_FFS;
		type = nb_ffs;
	}

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}

		if (alignment % secsz != 0) {
			warnx("Alignment must be a multiple of sector size;");
			warnx("the sector size for %s is %d bytes.",
			    device_name, secsz);
			continue;
		}

		if (size % secsz != 0) {
			warnx("Size in bytes must be a multiple of sector "
			      "size;");
			warnx("the sector size for %s is %d bytes.",
			    device_name, secsz);
			continue;
		}
		if (size > 0)
			sectors = size / secsz;

		add(fd);

		gpt_close(fd);
	}

	return (0);
}
