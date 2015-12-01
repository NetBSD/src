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
__FBSDID("$FreeBSD: src/sbin/gpt/recover.c,v 1.8 2005/08/31 01:47:19 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: recover.c,v 1.9 2015/12/01 16:32:19 christos Exp $");
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
#include "gpt_private.h"

static int recoverable;

static int cmd_recover(gpt_t, int, char *[]);

static const char *recoverhelp[] = {
    "",
};

struct gpt_cmd c_recover = {
	"recover",
	cmd_recover,
	recoverhelp, __arraycount(recoverhelp),
	0,
};

#define usage() gpt_usage(NULL, &c_recover)

static int
recover(gpt_t gpt)
{
	uint64_t last;
	struct gpt_hdr *hdr;

	if (map_find(gpt, MAP_TYPE_MBR) != NULL) {
		gpt_warnx(gpt, "Device contains an MBR");
		return -1;
	}

	gpt->gpt = map_find(gpt, MAP_TYPE_PRI_GPT_HDR);
	gpt->tpg = map_find(gpt, MAP_TYPE_SEC_GPT_HDR);
	gpt->tbl = map_find(gpt, MAP_TYPE_PRI_GPT_TBL);
	gpt->lbt = map_find(gpt, MAP_TYPE_SEC_GPT_TBL);

	if (gpt->gpt == NULL && gpt->tpg == NULL) {
		gpt_warnx(gpt, "No primary or secondary GPT headers, "
		    "can't recover");
		return -1;
	}
	if (gpt->tbl == NULL && gpt->lbt == NULL) {
		gpt_warnx(gpt, "No primary or secondary GPT tables, "
		    "can't recover");
		return -1;
	}

	last = gpt->mediasz / gpt->secsz - 1LL;

	if (gpt->gpt != NULL &&
	    ((struct gpt_hdr *)(gpt->gpt->map_data))->hdr_lba_alt != last) {
		gpt_warnx(gpt, "Media size has changed, please use "
		   "'gpt resizedisk'");
		return -1;
	}

	if (gpt->tbl != NULL && gpt->lbt == NULL) {
		gpt->lbt = map_add(gpt, last - gpt->tbl->map_size,
		    gpt->tbl->map_size, MAP_TYPE_SEC_GPT_TBL,
		    gpt->tbl->map_data);
		if (gpt->lbt == NULL) {
			gpt_warnx(gpt, "Adding secondary GPT table failed");
			return -1;
		}
		if (gpt_write(gpt, gpt->lbt) == -1) {
			gpt_warnx(gpt, "Writing secondary GPT table failed");
			return -1;
		}
		gpt_msg(gpt, "Recovered secondary GPT table from primary");
	} else if (gpt->tbl == NULL && gpt->lbt != NULL) {
		gpt->tbl = map_add(gpt, 2LL, gpt->lbt->map_size,
		    MAP_TYPE_PRI_GPT_TBL, gpt->lbt->map_data);
		if (gpt->tbl == NULL) {
			gpt_warnx(gpt, "Adding primary GPT table failed");
			return -1;
		}
		if (gpt_write(gpt, gpt->tbl) == -1) {
			gpt_warnx(gpt, "Writing primary GPT table failed");
			return -1;
		}
		gpt_msg(gpt, "Recovered primary GPT table from secondary");
	}

	if (gpt->gpt != NULL && gpt->tpg == NULL) {
		gpt->tpg = map_add(gpt, last, 1LL, MAP_TYPE_SEC_GPT_HDR,
		    calloc(1, gpt->secsz));
		if (gpt->tpg == NULL) {
			gpt_warnx(gpt, "Adding secondary GPT header failed");
			return -1;
		}
		memcpy(gpt->tpg->map_data, gpt->gpt->map_data, gpt->secsz);
		hdr = gpt->tpg->map_data;
		hdr->hdr_lba_self = htole64(gpt->tpg->map_start);
		hdr->hdr_lba_alt = htole64(gpt->gpt->map_start);
		hdr->hdr_lba_table = htole64(gpt->lbt->map_start);
		hdr->hdr_crc_self = 0;
		hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));
		if (gpt_write(gpt, gpt->tpg) == -1) {
			gpt_warnx(gpt, "Writing secondary GPT header failed");
			return -1;
		}
		gpt_msg(gpt, "Recovered secondary GPT header from primary");
	} else if (gpt->gpt == NULL && gpt->tpg != NULL) {
		gpt->gpt = map_add(gpt, 1LL, 1LL, MAP_TYPE_PRI_GPT_HDR,
		    calloc(1, gpt->secsz));
		if (gpt->gpt == NULL) {
			gpt_warnx(gpt, "Adding primary GPT header failed");
			return -1;
		}
		memcpy(gpt->gpt->map_data, gpt->tpg->map_data, gpt->secsz);
		hdr = gpt->gpt->map_data;
		hdr->hdr_lba_self = htole64(gpt->gpt->map_start);
		hdr->hdr_lba_alt = htole64(gpt->tpg->map_start);
		hdr->hdr_lba_table = htole64(gpt->tbl->map_start);
		hdr->hdr_crc_self = 0;
		hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));
		if (gpt_write(gpt, gpt->gpt) == -1) {
			gpt_warnx(gpt, "Writing primary GPT header failed");
			return -1;
		}
		gpt_msg(gpt, "Recovered primary GPT header from secondary");
	}
	return 0;
}

static int
cmd_recover(gpt_t gpt, int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "r")) != -1) {
		switch(ch) {
		case 'r':
			recoverable = 1;
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	return recover(gpt);
}
