/*	$NetBSD: partitions.c,v 1.1.2.2 2019/10/28 02:53:17 msaitoh Exp $	*/

/*
 * Copyright 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "defs.h"
#include "mbr.h"
#include <assert.h>

/*
 * A list of partitioning schemes, so we can iterate over everything
 * supported (e.g. when partitioning a new disk). NULL terminated.
 */
const struct disk_partitioning_scheme **available_part_schemes;
/*
 * The number of valid entries on above list
 */
size_t num_available_part_schemes;

/*
 * Generic reader - query a disk device and read all partitions from it
 */
struct disk_partitions *
partitions_read_disk(const char *dev, daddr_t disk_size, bool no_mbr)
{
	const struct disk_partitioning_scheme **ps;

	if (!available_part_schemes)
		return NULL;

	for (ps = available_part_schemes; *ps; ps++) {
#ifdef HAVE_MBR
		if (no_mbr && (*ps)->name == MSG_parttype_mbr)
			continue;
#endif
		struct disk_partitions *parts =
		    (*ps)->read_from_disk(dev, 0, disk_size, *ps);
		if (parts)
			return parts;
	}
	return NULL;
}

/*************** global init ****************************************/
/*
 * Helper structure to fill our global list of available partitioning
 * schemes.
 */
struct part_scheme_desc {
	bool (*is_available)(void);
	const struct disk_partitioning_scheme *ps;
};

#ifdef HAVE_GPT
bool gpt_parts_check(void);
extern const struct disk_partitioning_scheme gpt_parts;
#endif
#ifdef HAVE_MBR
extern const struct disk_partitioning_scheme mbr_parts;
#endif

extern const struct disk_partitioning_scheme disklabel_parts;
#if RAW_PART != 2
static struct disk_partitioning_scheme only_disklabel_parts;

/*
 * If not overriden by MD code, we can not boot from plain
 * disklabel disks (w/o MBR).
 */
static bool have_only_disklabel_boot_support(const char *disk)
{
#ifdef HAVE_PLAIN_DISKLABEL_BOOT
	return HAVE_PLAIN_DISKLABEL_BOOT(disk);
#else
	return false;
#endif
}
#endif

/*
 * One time initialization
 */
void
partitions_init(void)
{
	/*
	 * List of partitioning schemes.
	 * Order is important, the selection menu is created from start
	 * to end. Keep good defaults early. Most architectures will
	 * only offer very few entries.
	 */
static const struct part_scheme_desc all_descs[] = {
#if RAW_PART == 2	/* only available as primary on some architectures */
		{ NULL, &disklabel_parts },
#endif
#ifdef HAVE_GPT
		{ gpt_parts_check, &gpt_parts },
#endif
#ifdef HAVE_MBR
		{ NULL, &mbr_parts },
#endif
#if RAW_PART != 2	/* "whole disk NetBSD" disklabel variant */
		{ NULL, &only_disklabel_parts },
#endif
	};

	size_t i, avail;
	const struct disk_partitioning_scheme **out;
	bool *is_available;
	static const size_t all_cnt = __arraycount(all_descs);

	check_available_binaries();

#if RAW_PART != 2
	/* generate a variant of disklabel w/o parent scheme */
	only_disklabel_parts = disklabel_parts;
	only_disklabel_parts.name = MSG_parttype_only_disklabel;
	only_disklabel_parts.have_boot_support =
	    have_only_disklabel_boot_support;
#endif


	is_available = malloc(all_cnt);

	for (avail = i = 0; i < all_cnt; i++) {
		is_available[i] = all_descs[i].is_available == NULL
				|| all_descs[i].is_available();
		if (is_available[i])
			avail++;
	}

	if (avail == 0)
		return;

	num_available_part_schemes = avail;
	available_part_schemes = malloc(sizeof(*available_part_schemes)
	    * (avail+1));
	if (available_part_schemes == NULL)
		return;

	for (out = available_part_schemes, i = 0; i < all_cnt; i++) {
		if (!is_available[i])
			continue;
		*out++ = all_descs[i].ps;
	}
	*out = NULL;

	free(is_available);
}
