/*-
 * Copyright (c) 2004 Marcel Moolenaar
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
__FBSDID("$FreeBSD: src/sbin/gpt/remove.c,v 1.10 2006/10/04 18:20:25 marcel Exp $");
#endif
#ifdef __RCSID
__RCSID("$NetBSD: type.c,v 1.5 2014/09/30 18:00:00 christos Exp $");
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

static int all;
static gpt_uuid_t type, newtype;
static off_t block, size;
static unsigned int entry;
static uint8_t *label;

const char typemsg1[] = "type -a -T newtype device ...";
const char typemsg2[] = "type [-b blocknr] [-i index] [-L label] "
	"[-s sectors] [-t type]";
const char typemsg3[] = "     -T newtype device ...";

__dead static void
usage_type(void)
{

	fprintf(stderr,
            "usage: %s %s\n"
            "       %s %s\n"
            "       %*s %s\n", getprogname(), typemsg1,
            getprogname(), typemsg2, (int)strlen(getprogname()), "", typemsg3);
	exit(1);
}

static void
chtype(int fd)
{
	map_t *gpt, *tpg;
	map_t *tbl, *lbt;
	map_t *m;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;

	gpt = map_find(MAP_TYPE_PRI_GPT_HDR);
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

	/* Change type of all matching entries in the map. */
	for (m = map_first(); m != NULL; m = m->map_next) {
		if (m->map_type != MAP_TYPE_GPT_PART || m->map_index < 1)
			continue;
		if (entry > 0 && entry != m->map_index)
			continue;
		if (block > 0 && block != m->map_start)
			continue;
		if (size > 0 && size != m->map_size)
			continue;

		i = m->map_index - 1;

		hdr = gpt->map_data;
		ent = (void*)((char*)tbl->map_data + i *
		    le32toh(hdr->hdr_entsz));

		if (label != NULL)
			if (strcmp((char *)label,
			    (char *)utf16_to_utf8(ent->ent_name)) != 0)
				continue;

		if (!gpt_uuid_is_nil(ent->ent_type) &&
		    !gpt_uuid_equal(type, ent->ent_type))
			continue;

		/* Change the primary entry. */
		gpt_uuid_copy(ent->ent_type, newtype);

		hdr->hdr_crc_table = htole32(crc32(tbl->map_data,
		    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
		hdr->hdr_crc_self = 0;
		hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

		gpt_write(fd, gpt);
		gpt_write(fd, tbl);

		hdr = tpg->map_data;
		ent = (void*)((char*)lbt->map_data + i *
		    le32toh(hdr->hdr_entsz));

		/* Change the secondary entry. */
		gpt_uuid_copy(ent->ent_type, newtype);

		hdr->hdr_crc_table = htole32(crc32(lbt->map_data,
		    le32toh(hdr->hdr_entries) * le32toh(hdr->hdr_entsz)));
		hdr->hdr_crc_self = 0;
		hdr->hdr_crc_self = htole32(crc32(hdr, le32toh(hdr->hdr_size)));

		gpt_write(fd, lbt);
		gpt_write(fd, tpg);
		printf("partition %d type changed\n", m->map_index);
	}
}

int
cmd_type(int argc, char *argv[])
{
	char *p;
	int ch, fd;
	int64_t human_num;

	/* Get the type options */
	while ((ch = getopt(argc, argv, "ab:i:L:s:t:T:")) != -1) {
		switch(ch) {
		case 'a':
			if (all > 0)
				usage_type();
			all = 1;
			break;
		case 'b':
			if (block > 0)
				usage_type();
			if (dehumanize_number(optarg, &human_num) < 0)
				usage_type();
			block = human_num;
			if (block < 1)
				usage_type();
			break;
		case 'i':
			if (entry > 0)
				usage_type();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				usage_type();
			break;
                case 'L':
                        if (label != NULL)
                                usage_type();
                        label = (uint8_t *)strdup(optarg);
                        break;
		case 's':
			if (size > 0)
				usage_type();
			size = strtoll(optarg, &p, 10);
			if (*p != 0 || size < 1)
				usage_type();
			break;
		case 't':
			if (!gpt_uuid_is_nil(type))
				usage_type();
			if (gpt_uuid_parse(optarg, type) != 0)
				usage_type();
			break;
		case 'T':
			if (!gpt_uuid_is_nil(newtype))
				usage_type();
			if (gpt_uuid_parse(optarg, newtype) != 0)
				usage_type();
			break;
		default:
			usage_type();
		}
	}

	if (!all ^
	    (block > 0 || entry > 0 || label != NULL || size > 0 ||
	    !gpt_uuid_is_nil(type)))
		usage_type();
	if (gpt_uuid_is_nil(newtype))
		usage_type();

	if (argc == optind)
		usage_type();

	while (optind < argc) {
		fd = gpt_open(argv[optind++]);
		if (fd == -1) {
			warn("unable to open device '%s'", device_name);
			continue;
		}

		chtype(fd);

		gpt_close(fd);
	}

	return (0);
}
