/*	$NetBSD: biosboot.c,v 1.20 2015/12/02 12:24:02 christos Exp $ */

/*
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to the NetBSD Foundation
 * by Mike M. Volokhov. Development of this software was supported by the
 * Google Summer of Code program.
 * The GSoC project was mentored by Allen Briggs and Joerg Sonnenberger.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: biosboot.c,v 1.20 2015/12/02 12:24:02 christos Exp $");
#endif

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifdef DIOCGWEDGEINFO
#include <sys/disk.h>
#endif
#include <sys/param.h>
#include <sys/bootblock.h>

#include <err.h>
#include <fcntl.h>
#include <paths.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "map.h"
#include "gpt.h"
#include "gpt_private.h"

#define DEFAULT_BOOTDIR		"/usr/mdec"
#define DEFAULT_BOOTCODE	"gptmbr.bin"

static daddr_t start;
static uint64_t size;

static char *bootpath;
static unsigned int entry;
static uint8_t *label;

static int cmd_biosboot(gpt_t, int, char *[]);

static const char *biosboothelp[] = {
    "[-c bootcode] [-i index] [-L label]",
#if notyet
    "[-a alignment] [-b blocknr] [-i index] [-l label]",
    "[-s size] [-t type]",
#endif
};

struct gpt_cmd c_biosboot = {
	"biosboot",
	cmd_biosboot,
	biosboothelp, __arraycount(biosboothelp),
	0,
};

#define usage() gpt_usage(NULL, &c_biosboot)

static struct mbr*
read_boot(gpt_t gpt)
{
	int bfd, ret = 0;
	struct mbr *buf;
	struct stat st;

	buf = NULL;
	bfd = -1;

	if (bootpath == NULL)
		bootpath = strdup(DEFAULT_BOOTDIR "/" DEFAULT_BOOTCODE);
	else if (*bootpath == '/')
		bootpath = strdup(bootpath);
	else {
		if (asprintf(&bootpath, "%s/%s", DEFAULT_BOOTDIR, bootpath) < 0)
			bootpath = NULL;
	}

	if (bootpath == NULL) {
		gpt_warn(gpt, "Can't allocate memory for bootpath");
		goto fail;
	}

	if ((buf = malloc((size_t)gpt->secsz)) == NULL) {
		gpt_warn(gpt, "Can't allocate memory for sector");
		goto fail;
	}


	if ((bfd = open(bootpath, O_RDONLY)) < 0 || fstat(bfd, &st) == -1) {
		gpt_warn(gpt, "Can't open `%s'", bootpath);
		goto fail;
	}

	if (st.st_size != MBR_DSN_OFFSET) {
		gpt_warnx(gpt, "The bootcode in `%s' does not match the"
		    " expected size %u", bootpath, MBR_DSN_OFFSET);
		goto fail;
	}

	if (read(bfd, buf, st.st_size) != st.st_size) {
		gpt_warn(gpt, "Error reading from `%s'", bootpath);
		goto fail;
	}

	ret++;

    fail:
	if (bfd != -1)
		close(bfd);
	if (ret == 0) {
		free(buf);
		buf = NULL;
	}
	return buf;
}

static int
set_bootable(gpt_t gpt, map_t map, map_t tbl, unsigned int i)
{
	unsigned int j;
	struct gpt_hdr *hdr = map->map_data;
	struct gpt_ent *ent;
	unsigned int ne = le32toh(hdr->hdr_entries);

	for (j = 0; j < ne; j++) {
		ent = gpt_ent(map, tbl, j);
		ent->ent_attr &= ~GPT_ENT_ATTR_LEGACY_BIOS_BOOTABLE;
	}

	ent = gpt_ent(map, tbl, i);
	ent->ent_attr |= GPT_ENT_ATTR_LEGACY_BIOS_BOOTABLE;

	return gpt_write_crc(gpt, map, tbl);
}

static int
biosboot(gpt_t gpt)
{
	map_t mbrmap, m;
	struct mbr *mbr, *bootcode;
	unsigned int i;
	struct gpt_ent *ent;
	uint8_t utfbuf[__arraycount(ent->ent_name) * 3 + 1];

	/*
	 * Parse and validate partition maps
	 */
	if (gpt_hdr(gpt) == NULL)
		return -1;

	mbrmap = map_find(gpt, MAP_TYPE_PMBR);
	if (mbrmap == NULL || mbrmap->map_start != 0) {
		gpt_warnx(gpt, "No valid Protective MBR found");
		return -1;
	}

	mbr = mbrmap->map_data;

	/*
	 * Update the boot code
	 */
	if ((bootcode = read_boot(gpt)) == NULL) {
		gpt_warnx(gpt, "Error reading bootcode");
		return -1;
	}
	(void)memcpy(&mbr->mbr_code, &bootcode->mbr_code,
		sizeof(mbr->mbr_code));
	free(bootcode);

	/*
	 * Walk through the GPT and see where we can boot from
	 */
	for (m = map_first(gpt); m != NULL; m = m->map_next) {
		if (m->map_type != MAP_TYPE_GPT_PART || m->map_index < 1)
			continue;

		ent = m->map_data;

		/* first, prefer user selection */
		if (entry > 0 && m->map_index == entry)
			break;

		if (label != NULL) {
			utf16_to_utf8(ent->ent_name, utfbuf, sizeof(utfbuf));
			if (strcmp((char *)label, (char *)utfbuf) == 0)
				break;
		}

		/* next, partition as could be specified by wedge */
		if (entry < 1 && label == NULL && size > 0 &&
		    m->map_start == start && m->map_size == (off_t)size)
			break;
	}

	if (m == NULL) {
		gpt_warnx(gpt, "No bootable partition");
		return -1;
	}

	i = m->map_index - 1;


	if (set_bootable(gpt, gpt->gpt, gpt->tbl, i) == -1)
		return -1;

	if (set_bootable(gpt, gpt->tpg, gpt->lbt, i) == -1)
		return -1;

	if (gpt_write(gpt, mbrmap) == -1) {
		gpt_warnx(gpt, "Cannot update Protective MBR");
		return -1;
	}

	gpt_msg(gpt, "Partition %d marked as bootable", i + 1);
	return 0;
}

static int
cmd_biosboot(gpt_t gpt, int argc, char *argv[])
{
#ifdef DIOCGWEDGEINFO
	struct dkwedge_info dkw;
#endif
	char *dev;
	int ch;
	gpt_t ngpt = gpt;

	while ((ch = getopt(argc, argv, "c:i:L:")) != -1) {
		switch(ch) {
		case 'c':
			if (gpt_name_get(gpt, &bootpath) == -1)
				return usage();
			break;
		case 'i':
			if (gpt_entry_get(&entry) == -1)
				return usage();
			break;
		case 'L':
			if (gpt_name_get(gpt, &label) == -1)
				return usage();
			break;
		default:
			return usage();
		}
	}

	if (argc != optind)
		return usage();

	start = 0;
	size = 0;

#ifdef DIOCGWEDGEINFO
	if ((gpt->sb.st_mode & S_IFMT) != S_IFREG &&
	    ioctl(gpt->fd, DIOCGWEDGEINFO, &dkw) != -1) {
		if (entry > 0)
			/* wedges and indexes are mutually exclusive */
			return usage();
		dev = dkw.dkw_parent;
		start = dkw.dkw_offset;
		size = dkw.dkw_size;
		ngpt = gpt_open(dev, gpt->flags, gpt->verbose,
		    gpt->mediasz, gpt->secsz);
		if (ngpt == NULL)
			return -1;
	}
#endif
	biosboot(ngpt);
	if (ngpt != gpt)
		gpt_close(ngpt);

	return 0;
}
