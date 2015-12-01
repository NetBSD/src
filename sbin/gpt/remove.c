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
__RCSID("$NetBSD: remove.c,v 1.18 2015/12/01 09:05:33 christos Exp $");
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

static int all;
static gpt_uuid_t type;
static off_t block, size;
static unsigned int entry;
static uint8_t *label;

const char removemsg1[] = "remove -a";
const char removemsg2[] = "remove [-b blocknr] [-i index] [-L label] "
	"[-s sectors] [-t type]";

static int
usage_remove(void)
{

	fprintf(stderr,
	    "usage: %s %s\n"
	    "       %s %s\n",
	    getprogname(), removemsg1, getprogname(), removemsg2);
	return -1;
}

static int
rem(gpt_t gpt)
{
	map_t m;
	struct gpt_ent *ent;
	unsigned int i;

	if (gpt_hdr(gpt) == NULL)
		return -1;

	/* Remove all matching entries in the map. */
	for (m = map_first(gpt); m != NULL; m = m->map_next) {
		if (m->map_type != MAP_TYPE_GPT_PART || m->map_index < 1)
			continue;
		if (entry > 0 && entry != m->map_index)
			continue;
		if (block > 0 && block != m->map_start)
			continue;
		if (size > 0 && size != m->map_size)
			continue;

		i = m->map_index - 1;

		ent = gpt_ent_primary(gpt, i);

		if (label != NULL)
			if (strcmp((char *)label,
			    (char *)utf16_to_utf8(ent->ent_name)) != 0)
				continue;

		if (!gpt_uuid_is_nil(type) &&
		    !gpt_uuid_equal(type, ent->ent_type))
			continue;

		/* Remove the primary entry by clearing the partition type. */
		gpt_uuid_copy(ent->ent_type, gpt_uuid_nil);

		if (gpt_write_primary(gpt) == -1)
			return -1;

		ent = gpt_ent_backup(gpt, i);

		/* Remove the secondary entry. */
		gpt_uuid_copy(ent->ent_type, gpt_uuid_nil);

		if (gpt_write_backup(gpt) == -1)
			return -1;
		gpt_msg(gpt, "partition %d removed", m->map_index);
	}
	return 0;
}

int
cmd_remove(gpt_t gpt, int argc, char *argv[])
{
	char *p;
	int ch;
	int64_t human_num;

	/* Get the remove options */
	while ((ch = getopt(argc, argv, "ab:i:L:s:t:")) != -1) {
		switch(ch) {
		case 'a':
			if (all > 0)
				return usage_remove();
			all = 1;
			break;
		case 'b':
			if (block > 0)
				return usage_remove();
			if (dehumanize_number(optarg, &human_num) < 0)
				return usage_remove();
			block = human_num;
			if (block < 1)
				return usage_remove();
			break;
		case 'i':
			if (entry > 0)
				return usage_remove();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				return usage_remove();
			break;
		case 'L':
			if (label != NULL)
				return usage_remove();
			label = (uint8_t *)strdup(optarg);
			break;
		case 's':
			if (size > 0)
				usage_remove();
			size = strtoll(optarg, &p, 10);
			if (*p != 0 || size < 1)
				return usage_remove();
			break;
		case 't':
			if (!gpt_uuid_is_nil(type))
				return usage_remove();
			if (gpt_uuid_parse(optarg, type) != 0)
				return usage_remove();
			break;
		default:
			return usage_remove();
		}
	}

	if (!all ^
	    (block > 0 || entry > 0 || label != NULL || size > 0 ||
	    !gpt_uuid_is_nil(type)))
		return usage_remove();

	if (argc != optind)
		return usage_remove();

	return rem(gpt);
}
