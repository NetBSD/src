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
__RCSID("$NetBSD: resize.c,v 1.10 2014/09/30 02:12:55 christos Exp $");
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

static off_t alignment, sectors, size;
static unsigned int entry;

const char resizemsg[] = "resize -i index [-a alignment] [-s size] device ...";

__dead static void
usage_resize(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), resizemsg);
	exit(1);
}

static void
resize(int fd)
{
	uuid_t uuid;
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	map_t *map;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;
	off_t alignsecs, newsize;
	

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
	uuid_dec_le(ent->ent_type, &uuid);
	if (uuid_is_nil(&uuid, NULL)) {
		warnx("%s: error: entry at index %u is unused",
		    device_name, entry);
		return;
	}

	alignsecs = alignment / secsz;

	for (map = map_first(); map != NULL; map = map->map_next) {
		if (entry == map->map_index)
			break;
	}
	if (map == NULL) {
		warnx("%s: error: could not find map entry corresponding "
		      "to index", device_name);
		return;
	}

	if (sectors > 0 && sectors == map->map_size)
		if (alignment == 0 ||
		    (alignment > 0 && sectors % alignsecs == 0)) {
			/* nothing to do */
			warnx("%s: partition does not need resizing",
			    device_name);
			return;
		}

	newsize = map_resize(map, sectors, alignsecs);
	if (newsize == 0 && alignment > 0) {
		warnx("%s: could not resize partition with alignment "
		      "constraint", device_name);
		return;
	} else if (newsize == 0) {
		warnx("%s: could not resize partition", device_name);
		return;
	}

	ent->ent_lba_end = htole64(map->map_start + newsize - 1LL);

	hdr->hdr_crc_table = htole32(crc32(tbl->map_data,
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

	gpt_write(fd, gpt);
	gpt_write(fd, tbl);

	hdr = tpg->map_data;
	ent = (void*)((char*)lbt->map_data + i * le32toh(hdr->hdr_entsz));

	ent->ent_lba_end = htole64(map->map_start + newsize - 1LL);

	hdr->hdr_crc_table = htole32(crc32(lbt->map_data,
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

	gpt_write(fd, lbt);
	gpt_write(fd, tpg);

	printf("Partition %d resized, use:\n", entry);
	printf("\tdkctl %s addwedge <wedgename> %" PRIu64 " %" PRIu64
	    " <type>\n", device_arg, map->map_start, newsize);
	printf("to create a wedge for it\n");
}

int
cmd_resize(int argc, char *argv[])
{
	char *p;
	int ch, fd;
	int64_t human_num;

	while ((ch = getopt(argc, argv, "a:i:s:")) != -1) {
		switch(ch) {
		case 'a':
			if (alignment > 0)
				usage_resize();
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_resize();
			alignment = human_num;
			if (alignment < 1)
				usage_resize();
			break;
		case 'i':
			if (entry > 0)
				usage_resize();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				usage_resize();
			break;
		case 's':
			if (sectors > 0 || size > 0)
				usage_resize();
			sectors = strtoll(optarg, &p, 10);
			if (sectors < 1)
				usage_resize();
			if (*p == '\0')
				break;
			if (*p == 's' || *p == 'S') {
				if (*(p + 1) == '\0')
					break;
				else
					usage_resize();
			}
			if (*p == 'b' || *p == 'B') {
				if (*(p + 1) == '\0') {
					size = sectors;
					sectors = 0;
					break;
				} else
					usage_resize();
			}
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_resize();
			size = human_num;
			sectors = 0;
			break;
		default:
			usage_resize();
		}
	}

	if (argc == optind)
		usage_resize();

	if (entry == 0)
		usage_resize();

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

		resize(fd);

		gpt_close(fd);
	}

	return 0;
}
