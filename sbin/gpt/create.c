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
__FBSDID("$FreeBSD: src/sbin/gpt/create.c,v 1.11 2005/08/31 01:47:19 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: create.c,v 1.14 2015/12/01 09:05:33 christos Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/bootblock.h>

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"
#include "gpt_private.h"

static int force;
static u_int parts;
static int primary_only;

const char createmsg[] = "create [-fP] [-p <partitions>]";

static int
usage_create(void)
{

	fprintf(stderr,
	    "usage: %s %s\n", getprogname(), createmsg);
	return -1;
}

static int
create(gpt_t gpt)
{
	off_t blocks, last;
	map_t map;
	struct mbr *mbr;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;
	void *p;

	last = gpt->mediasz / gpt->secsz - 1LL;

	if (map_find(gpt, MAP_TYPE_PRI_GPT_HDR) != NULL ||
	    map_find(gpt, MAP_TYPE_SEC_GPT_HDR) != NULL) {
		gpt_warnx(gpt, "Device already contains a GPT");
		return -1;
	}
	map = map_find(gpt, MAP_TYPE_MBR);
	if (map != NULL) {
		if (!force) {
			gpt_warnx(gpt, "Device contains a MBR");
			return -1;
		}
		/* Nuke the MBR in our internal map. */
		map->map_type = MAP_TYPE_UNUSED;
	}

	/*
	 * Create PMBR.
	 */
	if (map_find(gpt, MAP_TYPE_PMBR) == NULL) {
		if (map_free(gpt, 0LL, 1LL) == 0) {
			gpt_warnx(gpt, "No room for the PMBR");
			return -1;
		}
		mbr = gpt_read(gpt, 0LL, 1);
		if (mbr == NULL) {
			gpt_warnx(gpt, "Error reading MBR");
			return -1;
		}
		memset(mbr, 0, sizeof(*mbr));
		mbr->mbr_sig = htole16(MBR_SIG);
		gpt_create_pmbr_part(mbr->mbr_part, last);

		map = map_add(gpt, 0LL, 1LL, MAP_TYPE_PMBR, mbr);
		gpt_write(gpt, map);
	}

	/* Get the amount of free space after the MBR */
	blocks = map_free(gpt, 1LL, 0LL);
	if (blocks == 0LL) {
		gpt_warnx(gpt, "No room for the GPT header");
		return -1;
	}

	/* Don't create more than parts entries. */
	if ((uint64_t)(blocks - 1) * gpt->secsz >
	    parts * sizeof(struct gpt_ent)) {
		blocks = (parts * sizeof(struct gpt_ent)) / gpt->secsz;
		if ((parts * sizeof(struct gpt_ent)) % gpt->secsz)
			blocks++;
		blocks++;		/* Don't forget the header itself */
	}

	/* Never cross the median of the device. */
	if ((blocks + 1LL) > ((last + 1LL) >> 1))
		blocks = ((last + 1LL) >> 1) - 1LL;

	/*
	 * Get the amount of free space at the end of the device and
	 * calculate the size for the GPT structures.
	 */
	map = map_last(gpt);
	if (map->map_type != MAP_TYPE_UNUSED) {
		gpt_warnx(gpt, "No room for the backup header");
		return -1;
	}

	if (map->map_size < blocks)
		blocks = map->map_size;
	if (blocks == 1LL) {
		gpt_warnx(gpt, "No room for the GPT table");
		return -1;
	}

	if ((p = calloc(1, gpt->secsz)) == NULL) {
		gpt_warnx(gpt, "Can't allocate the GPT");
		return -1;
	}
	if ((gpt->gpt = map_add(gpt, 1LL, 1LL,
	    MAP_TYPE_PRI_GPT_HDR, p)) == NULL) {
		free(p);
		gpt_warnx(gpt, "Can't add the GPT");
		return -1;
	}

	blocks--;		/* Number of blocks in the GPT table. */
	if ((p = calloc(blocks, gpt->secsz)) == NULL) {
		gpt_warnx(gpt, "Can't allocate the GPT table");
		return -1;
	}
	if ((gpt->tbl = map_add(gpt, 2LL, blocks,
	    MAP_TYPE_PRI_GPT_TBL, p)) == NULL) {
		free(p);
		gpt_warnx(gpt, "Can't add the GPT table");
		return -1;
	}

	hdr = gpt->gpt->map_data;
	memcpy(hdr->hdr_sig, GPT_HDR_SIG, sizeof(hdr->hdr_sig));
	hdr->hdr_revision = htole32(GPT_HDR_REVISION);
	hdr->hdr_size = htole32(GPT_HDR_SIZE);
	hdr->hdr_lba_self = htole64(gpt->gpt->map_start);
	hdr->hdr_lba_alt = htole64(last);
	hdr->hdr_lba_start = htole64(gpt->tbl->map_start + blocks);
	hdr->hdr_lba_end = htole64(last - blocks - 1LL);
	gpt_uuid_generate(hdr->hdr_guid);
	hdr->hdr_lba_table = htole64(gpt->tbl->map_start);
	hdr->hdr_entries = htole32((blocks * gpt->secsz) /
	    sizeof(struct gpt_ent));
	if (le32toh(hdr->hdr_entries) > parts)
		hdr->hdr_entries = htole32(parts);
	hdr->hdr_entsz = htole32(sizeof(struct gpt_ent));

	ent = gpt->tbl->map_data;
	for (i = 0; i < le32toh(hdr->hdr_entries); i++) {
		gpt_uuid_generate(ent[i].ent_guid);
	}

	if (gpt_write_primary(gpt) == -1)
		return -1;

	/*
	 * Create backup GPT if the user didn't suppress it.
	 */
	if (!primary_only) {
		// XXX: error checks 
		gpt->tpg = map_add(gpt, last, 1LL, MAP_TYPE_SEC_GPT_HDR,
		    calloc(1, gpt->secsz));
		gpt->lbt = map_add(gpt, last - blocks, blocks,
		    MAP_TYPE_SEC_GPT_TBL, gpt->tbl->map_data);
		memcpy(gpt->tpg->map_data, gpt->gpt->map_data, gpt->secsz);
		hdr = gpt->tpg->map_data;
		hdr->hdr_lba_self = htole64(gpt->tpg->map_start);
		hdr->hdr_lba_alt = htole64(gpt->gpt->map_start);
		hdr->hdr_lba_table = htole64(gpt->lbt->map_start);
		if (gpt_write_backup(gpt) == -1)
			return -1;
	}
	return 0;
}

int
cmd_create(gpt_t gpt, int argc, char *argv[])
{
	int ch;

	parts = 128;

	while ((ch = getopt(argc, argv, "fPp:")) != -1) {
		switch(ch) {
		case 'f':
			force = 1;
			break;
		case 'P':
			primary_only = 1;
			break;
		case 'p':
			parts = atoi(optarg);
			break;
		default:
			return usage_create();
		}
	}

	if (argc != optind)
		return usage_create();

	return create(gpt);
}
