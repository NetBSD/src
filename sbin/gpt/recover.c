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
__RCSID("$NetBSD: recover.c,v 1.12 2015/12/03 01:07:28 christos Exp $");
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
recover_gpt_hdr(gpt_t gpt, int type, off_t last)
{
	const char *name, *origname;
	void *p;
	map_t *dgpt, dtbl, sgpt, stbl;
	struct gpt_hdr *hdr;

	switch (type) {
	case MAP_TYPE_PRI_GPT_HDR:
		dgpt = &gpt->gpt;
		dtbl = gpt->tbl;
		sgpt = gpt->tpg;
		stbl = gpt->lbt;
		origname = "secondary";
		name = "primary";
		break;
	case MAP_TYPE_SEC_GPT_HDR:
		dgpt = &gpt->tpg;
		dtbl = gpt->lbt;
		sgpt = gpt->gpt;
		stbl = gpt->tbl;
		origname = "primary";
		name = "secondary";
		break;
	default:
		gpt_warn(gpt, "Bad table type %d", type);
		return -1;
	}

	if ((p = calloc(1, gpt->secsz)) == NULL) {
		gpt_warn(gpt, "Cannot allocate %s GPT header", name);
		return -1;
	}
	if ((*dgpt = map_add(gpt, last, 1LL, type, p)) == NULL) {
		gpt_warnx(gpt, "Cannot add %s GPT header", name);
		return -1;
	}
	memcpy((*dgpt)->map_data, sgpt->map_data, gpt->secsz);
	hdr = (*dgpt)->map_data;
	hdr->hdr_lba_self = htole64((*dgpt)->map_start);
	hdr->hdr_lba_alt = htole64(stbl->map_start);
	hdr->hdr_lba_table = htole64(dtbl->map_start);
	hdr->hdr_crc_self = 0;
	hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));
	if (gpt_write(gpt, *dgpt) == -1) {
		gpt_warnx(gpt, "Writing %s GPT header failed", name);
		return -1;
	}
	gpt_msg(gpt, "Recovered %s GPT header from %s", name, origname);
	return 0;
}

static int
recover_gpt_tbl(gpt_t gpt, int type, off_t start)
{
	const char *name, *origname;
	map_t *dtbl, stbl;

	switch (type) {
	case MAP_TYPE_PRI_GPT_TBL:
		dtbl = &gpt->tbl;
		stbl = gpt->lbt;
		origname = "secondary";
		name = "primary";
		break;
	case MAP_TYPE_SEC_GPT_TBL:
		dtbl = &gpt->lbt;
		stbl = gpt->tbl;
		origname = "primary";
		name = "secondary";
		break;
	default:
		gpt_warn(gpt, "Bad table type %d", type);
		return -1;
	}

	// XXX: non allocated memory
	*dtbl = map_add(gpt, start, stbl->map_size, type, stbl->map_data);
	if (*dtbl == NULL) {
		gpt_warnx(gpt, "Adding %s GPT table failed", name);
		return -1;
	}
	if (gpt_write(gpt, *dtbl) == -1) {
		gpt_warnx(gpt, "Writing %s GPT table failed", name);
		return -1;
	}
	gpt_msg(gpt, "Recovered %s GPT table from %s", name, origname);
	return 0;
}

static int
recover(gpt_t gpt, int recoverable)
{
	uint64_t last;

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
		   "'%s resizedisk'", getprogname());
		return -1;
	}

	if (gpt->tbl != NULL && gpt->lbt == NULL) {
		if (recover_gpt_tbl(gpt, MAP_TYPE_SEC_GPT_TBL,
		    last - gpt->tbl->map_size) == -1)
			return -1;
	} else if (gpt->tbl == NULL && gpt->lbt != NULL) {
		if (recover_gpt_tbl(gpt, MAP_TYPE_PRI_GPT_TBL, 2LL) == -1)
			return -1;
	}

	if (gpt->gpt != NULL && gpt->tpg == NULL) {
		if (recover_gpt_hdr(gpt, MAP_TYPE_SEC_GPT_HDR, last) == -1)
			return -1;
	} else if (gpt->gpt == NULL && gpt->tpg != NULL) {
		if (recover_gpt_hdr(gpt, MAP_TYPE_PRI_GPT_HDR, 1LL) == -1)
			return -1;
	}
	return 0;
}

static int
cmd_recover(gpt_t gpt, int argc, char *argv[])
{
	int ch;
	int recoverable = 0;

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

	return recover(gpt, recoverable);
}
