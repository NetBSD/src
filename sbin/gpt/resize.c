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
__RCSID("$NetBSD: resize.c,v 1.16 2015/12/01 16:32:19 christos Exp $");
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

static off_t alignment, sectors, size;
static unsigned int entry;

static int cmd_resize(gpt_t, int, char *[]);

static const char *resizehelp[] = {
    "-i index [-a alignment] [-s size]",
};

struct gpt_cmd c_resize = {
	"resize",
	cmd_resize,
	resizehelp, __arraycount(resizehelp),
	0,
};

#define usage() gpt_usage(NULL, &c_resize)

static int
resize(gpt_t gpt)
{
	map_t map;
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;
	off_t alignsecs, newsize;
	

	if ((hdr = gpt_hdr(gpt)) == NULL)
		return -1;

	ent = NULL;

	i = entry - 1;
	ent = gpt_ent_primary(gpt, i);
	if (gpt_uuid_is_nil(ent->ent_type)) {
		gpt_warnx(gpt, "Entry at index %u is unused", entry);
		return -1;
	}

	alignsecs = alignment / gpt->secsz;

	for (map = map_first(gpt); map != NULL; map = map->map_next) {
		if (entry == map->map_index)
			break;
	}
	if (map == NULL) {
		gpt_warnx(gpt, "Could not find map entry corresponding "
		    "to index");
		return -1;
	}

	if (sectors > 0 && sectors == map->map_size)
		if (alignment == 0 ||
		    (alignment > 0 && sectors % alignsecs == 0)) {
			/* nothing to do */
			gpt_warnx(gpt, "partition does not need resizing");
			return 0;
		}

	newsize = map_resize(gpt, map, sectors, alignsecs);
	if (newsize == 0 && alignment > 0) {
		gpt_warnx(gpt, "Could not resize partition with alignment "
		      "constraint");
		return -1;
	} else if (newsize == 0) {
		gpt_warnx(gpt, "Could not resize partition");
		return -1;
	}

	ent->ent_lba_end = htole64(map->map_start + newsize - 1LL);

	if (gpt_write_primary(gpt) == -1)
		return -1;

	ent = gpt_ent(gpt->gpt, gpt->lbt, i);
	ent->ent_lba_end = htole64(map->map_start + newsize - 1LL);

	if (gpt_write_backup(gpt) == -1)
		return -1;

	gpt_msg(gpt, "Partition %d resized: %" PRIu64 " %" PRIu64 "\n", entry,
	    map->map_start, newsize);

	return 0;
}

static int
cmd_resize(gpt_t gpt, int argc, char *argv[])
{
	char *p;
	int ch;
	int64_t human_num;

	while ((ch = getopt(argc, argv, "a:i:s:")) != -1) {
		switch(ch) {
		case 'a':
			if (alignment > 0)
				return usage();
			if (dehumanize_number(optarg, &human_num) < 0)
				return usage();
			alignment = human_num;
			if (alignment < 1)
				return usage();
			break;
		case 'i':
			if (entry > 0)
				return usage();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				return usage();
			break;
		case 's':
			if (sectors > 0 || size > 0)
				return usage();
			sectors = strtoll(optarg, &p, 10);
			if (sectors < 1)
				return usage();
			if (*p == '\0')
				break;
			if (*p == 's' || *p == 'S') {
				if (*(p + 1) == '\0')
					break;
				else
					return usage();
			}
			if (*p == 'b' || *p == 'B') {
				if (*(p + 1) == '\0') {
					size = sectors;
					sectors = 0;
					break;
				} else
					return usage();
			}
			if (dehumanize_number(optarg, &human_num) < 0)
				return usage();
			size = human_num;
			sectors = 0;
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	if (entry == 0)
		return usage();

	if ((sectors = gpt_check(gpt, alignment, size)) == -1)
		return -1;

	return resize(gpt);
}
