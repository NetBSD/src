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
__RCSID("$NetBSD: resizedisk.c,v 1.6 2014/10/01 03:52:42 jnemeth Exp $");
#endif

#include <sys/bootblock.h>
#include <sys/types.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"

static uint64_t sector, size;

const char resizediskmsg[] = "resizedisk [-s size] device ...";

__dead static void
usage_resizedisk(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), resizediskmsg);
	exit(1);
}

/*
 * relocate the secondary GPT based on the following criteria:
 * - size not specified
 *   - disk has not changed size, do nothing
 *   - disk has grown, relocate secondary
 *   - disk has shrunk, create new secondary
 * - size specified
 *   - size is larger then disk or same as current location, do nothing
 *   - relocate or create new secondary
 * - when shrinking, verify that table fits
 */
static void
resizedisk(int fd)
{
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	map_t *mbrmap;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	struct mbr *mbr;
	uint64_t last, oldloc, newloc, lastdata, gpt_size;
	int i;
	
	last = mediasz / secsz - 1;
	lastdata = 0;
	newloc = 0;

	if (sector > last) {
		warnx("%s: specified size is larger then the disk",
		    device_name);
		return;
	}

        mbrmap = map_find(MAP_TYPE_PMBR);
        if (mbrmap == NULL || mbrmap->map_start != 0) {
                warnx("%s: error: no valid Protective MBR found", device_name);
                return;
        }
        mbr = mbrmap->map_data;

	gpt = map_find(MAP_TYPE_PRI_GPT_HDR);
	ent = NULL;
	if (gpt == NULL) {
		warnx("%s: error: no primary GPT header; run create or recover",
		    device_name);
		return;
	}
	hdr = gpt->map_data;
	oldloc = le64toh(hdr->hdr_lba_alt);

	tpg = map_find(MAP_TYPE_SEC_GPT_HDR);
	if (tpg == NULL)
		if (gpt_gpt(fd, oldloc, 1))
			tpg = map_find(MAP_TYPE_SEC_GPT_HDR);

	tbl = map_find(MAP_TYPE_PRI_GPT_TBL);
	lbt = map_find(MAP_TYPE_SEC_GPT_TBL);
	if (tbl == NULL) {
		warnx("%s: error: run recover -- trust me", device_name);
		return;
	}

	gpt_size = tbl->map_size;
	if (sector == oldloc) {
		warnx("%s: device is already the specified size", device_name);
		return;
	}
	if (sector == 0 && last == oldloc) {
		warnx("%s: device hasn't changed size", device_name);
		return;
	}

	for (ent = tbl->map_data; ent <
	    (struct gpt_ent *)((char *)tbl->map_data +
	    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)); ent++) {
		if (!gpt_uuid_is_nil(ent->ent_type) &&
		    (le64toh(ent->ent_lba_end) > lastdata)) {
			lastdata = le64toh(ent->ent_lba_end);
		}
	}
	if (sector - gpt_size <= lastdata) {
		warnx("%s: not enough space at %" PRIu64
		    " for secondary GPT table", device_name, sector);
		return;
	}
	if (last - gpt_size <= lastdata) {
		warnx("%s: not enough space for new secondary GPT table",
		    device_name);
		return;
	}

	if (sector > oldloc)
		newloc = sector;
	if (sector > 0 && sector < oldloc && last >= oldloc)
		newloc = sector;
	if (sector == 0 && last > oldloc)
		newloc = last;
	if (newloc > 0) {
		if (tpg == NULL) {
			warnx("%s: error: no secondary GPT header; run recover",
			    device_name);
			return;
		}
		if (lbt == NULL) {
			warnx("%s: error: run recover -- trust me",
			    device_name);
			return;
		}
		tpg->map_start = newloc;
		lbt->map_start = newloc - gpt_size;
	} else {
		if (sector > 0)
			newloc = sector;
		else
			newloc = last;
		tpg = map_add(newloc, 1LL, MAP_TYPE_SEC_GPT_HDR,
		    calloc(1, secsz));
		lbt = map_add(newloc - gpt_size, gpt_size, MAP_TYPE_SEC_GPT_TBL,
		    tbl->map_data);
		memcpy(tpg->map_data, gpt->map_data, secsz);
	}

	hdr = gpt->map_data;
	hdr->hdr_lba_alt = tpg->map_start;
	hdr->hdr_crc_self = 0;
	hdr->hdr_lba_end = htole64(lbt->map_start - 1);
	hdr->hdr_crc_self =
	    htole32(crc32(gpt->map_data, GPT_HDR_SIZE));
	gpt_write(fd, gpt);

	hdr = tpg->map_data;
	hdr->hdr_lba_self = htole64(tpg->map_start);
	hdr->hdr_lba_alt = htole64(gpt->map_start);
	hdr->hdr_lba_end = htole64(lbt->map_start - 1);
	hdr->hdr_lba_table = htole64(lbt->map_start);
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self =
	    htole32(crc32(tpg->map_data, GPT_HDR_SIZE));
	gpt_write(fd, lbt);
	gpt_write(fd, tpg);

	for (i = 0; i < 4; i++)
		if (mbr->mbr_part[0].part_typ == MBR_PTYPE_PMBR)
			break;
	if (i == 4) {
		warnx("%s: no valid PMBR partition found", device_name);
		return;
	}
	if (last > 0xffffffff) {
		mbr->mbr_part[0].part_size_lo = htole16(0xffff);
		mbr->mbr_part[0].part_size_hi = htole16(0xffff);
	} else {
		mbr->mbr_part[0].part_size_lo = htole16(last);
		mbr->mbr_part[0].part_size_hi = htole16(last >> 16);
	}
	gpt_write(fd, mbrmap);

	return;
}

int
cmd_resizedisk(int argc, char *argv[])
{
	char *p;
	int ch, fd;
	int64_t human_num;

	while ((ch = getopt(argc, argv, "s:")) != -1) {
		switch(ch) {
		case 's':
			if (sector > 0 || size > 0)
				usage_resizedisk();
			sector = strtoll(optarg, &p, 10);
			if (sector < 1)
				usage_resizedisk();
			if (*p == '\0')
				break;
			if (*p == 's' || *p == 'S') {
				if (*(p + 1) == '\0')
					break;
				else
					usage_resizedisk();
			}
			if (*p == 'b' || *p == 'B') {
				if (*(p + 1) == '\0') {
					size = sector;
					sector = 0;
					break;
				} else
					usage_resizedisk();
			}
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_resizedisk();
			size = human_num;
			sector = 0;
			break;
		default:
			usage_resizedisk();
		}
	}

	if (argc == optind)
		usage_resizedisk();

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
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
			sector = size / secsz - 1;

		resizedisk(fd);

		gpt_close(fd);
	}

	return 0;
}
