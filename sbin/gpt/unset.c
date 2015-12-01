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
__RCSID("$NetBSD: unset.c,v 1.8 2015/12/01 16:32:19 christos Exp $");
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

static unsigned int entry;
static uint64_t attributes;

static int cmd_unset(gpt_t, int, char *[]);

static const char *unsethelp[] = {
    "-a attribute -i index",
};

struct gpt_cmd c_unset = {
	"unset",
	cmd_unset,
	unsethelp, __arraycount(unsethelp),
	0,
};

#define usage() gpt_usage(NULL, &c_unset)

static int
unset(gpt_t gpt)
{
	struct gpt_hdr *hdr;
	struct gpt_ent *ent;
	unsigned int i;
	

	if ((hdr = gpt_hdr(gpt)) == NULL)
		return -1;

	if (entry > le32toh(hdr->hdr_entries)) {
		gpt_warnx(gpt, "Index %u out of range (%u max)",
		    entry, le32toh(hdr->hdr_entries));
		return -1;
	}

	i = entry - 1;
	ent = gpt_ent_primary(gpt, i);
	if (gpt_uuid_is_nil(ent->ent_type)) {
		gpt_warnx(gpt, "Entry at index %u is unused", entry);
		return -1;
	}

	ent->ent_attr &= ~attributes;

	if (gpt_write_primary(gpt) == -1)
		return -1;

	ent = gpt_ent_backup(gpt, i);
	ent->ent_attr &= ~attributes;

	if (gpt_write_backup(gpt) == -1)
		return -1;
	gpt_msg(gpt, "Partition %d attributes updated", entry);
	return 0;
}

static int
cmd_unset(gpt_t gpt, int argc, char *argv[])
{
	char *p;
	int ch;

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
				return usage();
			break;
		case 'i':
			if (entry > 0)
				return usage();
			entry = strtoul(optarg, &p, 10);
			if (*p != 0 || entry < 1)
				return usage();
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	if (entry == 0 || attributes == 0)
		return usage();

	return unset(gpt);
}
