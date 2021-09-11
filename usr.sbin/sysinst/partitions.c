/*	$NetBSD: partitions.c,v 1.13 2021/09/11 20:28:06 andvar Exp $	*/

/*
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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

extern const struct disk_partitioning_scheme disklabel_parts;

/*
 * Generic reader - query a disk device and read all partitions from it.
 * disk_size is in units of physical sector size, which is passe as
 * bytes_per_sec.
 */
struct disk_partitions *
partitions_read_disk(const char *dev, daddr_t disk_size, size_t bytes_per_sec,
    bool no_mbr)
{
	const struct disk_partitioning_scheme **ps;
#ifdef HAVE_MBR
	bool mbr_done = false, disklabel_done = false;
#endif

	if (!available_part_schemes)
		return NULL;

	for (ps = available_part_schemes; *ps; ps++) {
#ifdef HAVE_MBR
		if (!no_mbr && (*ps) == &disklabel_parts && !mbr_done)
			continue;
		if (no_mbr && (*ps)->name == MSG_parttype_mbr)
			continue;
		if ((*ps)->name == MSG_parttype_mbr)
			mbr_done = true;
		if ((*ps)->read_from_disk == disklabel_parts.read_from_disk)
			disklabel_done = true;
#endif
		struct disk_partitions *parts =
		    (*ps)->read_from_disk(dev, 0, disk_size, bytes_per_sec,
		        *ps);
		if (parts)
			return parts;
	}
#ifdef HAVE_MBR
	if (!disklabel_done)
		return disklabel_parts.read_from_disk(dev, 0, disk_size,
		    bytes_per_sec, &disklabel_parts);
#endif
	return NULL;
}

bool
generic_adapt_foreign_part_info(const struct disk_partitions *myself,
    struct disk_part_info *dest,
    const struct disk_partitioning_scheme *src_scheme,
    const struct disk_part_info *src)
{
	*dest = *src;
	if (myself->pscheme == src_scheme)
		return true;	/* no conversion needed */

	if (src->nat_type == NULL)
		return false;

	/* slightly simplistic, enhance when needed */
	dest->nat_type = myself->pscheme->get_fs_part_type(
	    dest->nat_type ? dest->nat_type->generic_ptype : PT_root,
	    dest->fs_type,
	    dest->fs_sub_type);
	if (dest->nat_type == NULL)
		dest->nat_type = myself->pscheme->get_generic_part_type(
		    src->nat_type->generic_ptype);
	if (dest->nat_type == NULL)
		dest->nat_type = myself->pscheme->create_unknown_part_type();
	if (dest->nat_type == NULL)
		dest->nat_type = myself->pscheme->get_generic_part_type(
		    PT_unknown);

	return true;
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

#if RAW_PART == 3
static struct disk_partitioning_scheme only_disklabel_parts;

/*
 * If not overridden by MD code, we can not boot from plain
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
#if RAW_PART != 3	/* only available as primary on some architectures */
		{ NULL, &disklabel_parts },
#endif
#ifdef HAVE_GPT
		{ gpt_parts_check, &gpt_parts },
#endif
#ifdef HAVE_MBR
		{ NULL, &mbr_parts },
#endif
#if RAW_PART == 3	/* "whole disk NetBSD" disklabel variant */
		{ NULL, &only_disklabel_parts },
#endif
	};

	size_t i, avail;
	const struct disk_partitioning_scheme **out;
	bool *is_available;
	static const size_t all_cnt = __arraycount(all_descs);

	check_available_binaries();

#if RAW_PART == 3
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

/*
 * Final cleanup
 */
void
partitions_cleanup(void)
{
	for (size_t i = 0; i < num_available_part_schemes; i++)
		if (available_part_schemes[i]->cleanup != NULL)
			available_part_schemes[i]->cleanup();
	free(available_part_schemes);
}
