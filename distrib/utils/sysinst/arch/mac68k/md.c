/*	$NetBSD: md.c,v 1.41 2004/03/26 20:02:22 dsl Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Piermont Information Systems Inc.
 * 4. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
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

/* md.c -- Machine specific code for mac68k */

#include <stdio.h>
#include <util.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int blk_size;

/*
 * Compare lexigraphically two strings
 */
int
stricmp(s1, s2)
	const char *s1;
	const char *s2;
{
	char c1, c2;

	while (1) {
	    c1 = tolower(*s1++);
	    c2 = tolower(*s2++);
	    if (c1 < c2) return -1;
	    if (c1 > c2) return 1;
	    if (c1 == 0) return 0;
	}
}

void
setpartition(part, in_use, slot)
	struct apple_part_map_entry *part;
	char in_use[];
	int slot;
{
	EBZB *bzb;

	bzb = (EBZB *)&part->pmBootArgs[0];
	in_use[slot] = 1;
	bzb->flags.used = 1;
	bzb->flags.part = 'a' + slot;
}

/*
 * Find an entry in a use array that is unused and return it or
 *  -1 if no entry is available
 */
int
getFreeLabelEntry(slots)
	char *slots;
{
	int i;

	for ( i = 0; i < MAXPARTITIONS; i++) {
		if (i != RAW_PART && slots[i] == 0)
			return i;
	}
	return -1;
}

/*
 * Figure out what type type of the given partition is and return it.
 */
int
whichType(part)
	struct apple_part_map_entry *part;
{
	MAP_TYPE *map_entry = (MAP_TYPE *)&map_types;
	EBZB *bzb;
	char partyp[32];
	int type, maxsiz, entry_type = MAP_OTHER;

	bzb = (EBZB *)&part->pmBootArgs[0];
	if (part->pmSig != APPLE_PART_MAP_ENTRY_MAGIC)
	    return 0;
	maxsiz = sizeof(part->pmPartType);
	if (maxsiz > sizeof(partyp))
	    maxsiz = sizeof(partyp);
	strncpy(partyp, part->pmPartType, maxsiz);
	partyp[maxsiz-1] = '\0';

	/*
	 * Find out how to treat the partition type under NetBSD
	 */
	while (map_entry->type != MAP_EOL) {
	    if (stricmp(map_entry->name, partyp) == 0) {
		entry_type = map_entry->type;
		break;
	    }
	    map_entry++;
	}

	/*
	 * Now classify the use for NetBSD
	 */
	if (entry_type == MAP_RESERVED)
		type = 0;
	else if (entry_type == MAP_NETBSD) {
	    if (bzb->magic != APPLE_BZB_MAGIC)
		type = 0;
	    else if (bzb->type == APPLE_BZB_TYPEFS) {
		if (bzb->flags.root)
		    type = ROOT_PART;
		else if (bzb->flags.usr)
		    type = UFS_PART;
		else
		    type = SCRATCH_PART;
	    } else if (bzb->type == APPLE_BZB_TYPESWAP)
		type = SWAP_PART;
	    else
		type = SCRATCH_PART;
	} else if (entry_type == MAP_MACOS)
	    type = HFS_PART;
	else
	    type = SCRATCH_PART;
	return type;
}

char *
getFstype(part, len_type, type)
	struct apple_part_map_entry *part;
	int len_type;
	char *type;
{
	*type = '\0';
	switch(whichType(part)) {
	    case ROOT_PART:
	    case UFS_PART:
		strncpy(type, "4.2BSD", len_type);
		break;
	    case SWAP_PART:
		strncpy(type, "swap", len_type);
		break;
	    case HFS_PART:
		strncpy(type, "HFS", len_type);
		break;
	    case SCRATCH_PART:
	    default:
		break;
	}
	return (type);
}

char *
getUse(part, len_use, use)
	struct apple_part_map_entry *part;
	int len_use;
	char *use;
{
	EBZB *bzb;
	char partyp[32];

	*use = '\0';
	bzb = (EBZB *)&part->pmBootArgs[0];
	switch(whichType(part)) {
	    case ROOT_PART:
		if (bzb->flags.usr)
		    strncpy(use, "Root&Usr", len_use);
		else
		    strncpy(use, "Root", len_use);
		break;
	    case UFS_PART:
		strncpy(use, "Usr", len_use);
		break;
	    case SWAP_PART:
		break;
	    case HFS_PART:
		strncpy(use, "MacOS", len_use);
		break;
	    case SCRATCH_PART:
		strncpy(partyp, part->pmPartType, sizeof(partyp));
		partyp[sizeof(partyp)-1] = '\0';
		if (stricmp("Apple_Free", partyp) == 0)
		    strncpy(use, "Free", len_use);
		else if (stricmp("Apple_Scratch", partyp) == 0)
		    strncpy(use, "Scratch", len_use);
		else if (stricmp("Apple_MFS", partyp) == 0)
		    strncpy(use, "MFS", len_use);
		else if (stricmp("Apple_PRODOS", partyp) == 0)
		    strncpy(use, "PRODOS", len_use);
		else
		    strncpy(use, "unknown", len_use);
	    default:
		break;
	}
	return(use);
}

char *
getName(part, len_name, name)
	struct apple_part_map_entry *part;
	int len_name;
	char *name;
{
	EBZB *bzb;
	int fd;
	off_t seek;
	char dev_name[100], macosblk[512];

	*name = '\0';
	bzb = (EBZB *)&part->pmBootArgs[0];
	switch(whichType(part)) {
	    case SCRATCH_PART:
	    case ROOT_PART:
	    case UFS_PART:
		strncpy(name, bzb->mount_point, len_name);
		break;
	    case SWAP_PART:
		break;
	    case HFS_PART:
		/*
		 * OK, this is stupid but it's damn nice to know!
		 */
		snprintf (dev_name, sizeof(dev_name), "/dev/r%sc", diskdev);
		/*
		 * Open the disk as a raw device
		 */
		if ((fd = open(dev_name, O_RDONLY, 0)) >= 0) {
		    seek = (off_t)part->pmPyPartStart + (off_t)2;
		    seek *= (off_t)blk_size;
		    lseek(fd, seek, SEEK_SET);
		    read(fd, &macosblk, sizeof(macosblk));
		    macosblk[37+32] = '\0';
		    strncpy(name, bzb->mount_point, len_name);
		    strncat(name, " (", len_name-strlen(name));
		    strncat(name, &macosblk[37], len_name-strlen(name));
		    strncat(name, ")", len_name-strlen(name));
		}
		break;
	    default:
		break;
	}
	return(name);
}

/*
 * Find the first occurance of a Standard Type partition and
 *  mark it for use along with the default mount slot.
 */
int
findStdType(num_parts, in_use, type, count, alt)
	int num_parts;
	char in_use[];
	int type;
	int *count;
	int alt;
{
	EBZB *bzb;
	int i;

	for (i = 0; i < num_parts; i++) {
		bzb = (EBZB *)&map.blk[i].pmBootArgs[0];
		if (whichType(&map.blk[i]) != type || bzb->flags.used)
			continue;
		if (type == ROOT_PART) {
			if (alt >= 0 && alt != bzb->cluster)
				continue;
			setpartition(&map.blk[i], in_use, 0);
			strcpy (bzb->mount_point, "/");
			*count += 1;
		} else if (type == UFS_PART) {
			if (alt >= 0 && alt != bzb->cluster)
				continue;
			setpartition(&map.blk[i], in_use, 6);
			if (bzb->mount_point[0] == '\0')
				strcpy (bzb->mount_point, "/usr");
			*count += 1;
		} else if (type == SWAP_PART) {
			setpartition(&map.blk[i], in_use, 1);
			*count += 1;
		}
		return 0;
	}
	return -1;
}

/*
 * Reset the flags and reserved fields in the selected partition.
 * This functions isn't called to process any of the reserved partitions
 * where the boot code for MacOS is stored, so (hopefully) we won't
 * do more damage that we're trying to avoid.  Eventually the NetBSD
 * Boot Code will need to go into a partition too, but that should go
 * into a reserved partition as well.  I'd suggest using a partition
 * named something like "NetBSD_Boot" with a pmPartName of "Macintosh".
 * The Apple Start Manager (in ROM) will then recognize the partition
 * as the one containing the system bootstrip for the volume.
 */
void
reset_part_flags (part)
	struct apple_part_map_entry *part;
{
	EBZB *bzb;

	/*
	 * Clear out the MacOS fields that might be used for booting just
	 *  in case we've clobbered the boot code.
	 */
	part->pmLgDataStart = 0;
	part->pmPartStatus = 0x77;  /* make sure the partition shows up */
	part->pmLgBootStart = 0;
	part->pmBootSize = 0;
	part->pmBootLoad = 0;
	part->pmBootLoad2 = 0;
	part->pmBootEntry = 0;
	part->pmBootEntry2 = 0;
	part->pmBootCksum = 0;

	/*
	 * Clear out all the NetBSD fields too.  We only clear out the ones
	 *  that should get reset during our processing.
	 */
	bzb = (EBZB *)&part->pmBootArgs[0];
	bzb->magic = 0;
	bzb->cluster = 0;
	bzb->inode = 0;
	bzb->type = 0;
	bzb->flags.root = 0;
	bzb->flags.usr = 0;
	bzb->flags.crit = 0;
	bzb->flags.slice = 0;
	bzb->flags.used = 0;
	return;
}

/*
 * sortmerge:
 *  1) Moves all the Partition Map entries to the front of the Map.
 *     This is required because some disk formatters leave holes.
 *  2) Sorts all entries by ascending start block number.
 *     Needed so the NetBSD algorithm for finding partitions works
 *     consistently from a user perspective.
 *  3) Collapse multiple adjected "free" entries into a single entry.
 *  4) Identify the NetBSD mount_points.
 */
void
sortmerge(void)
{
    struct apple_part_map_entry tmp_blk;
    char in_use[MAXPARTITIONS];
    int i, j;
    EBZB *bzb;

    /*
     * Step 1, squeeze out the holes that some disk formatters leave in
     *  the Map.  Also convert any "old" Map entries to the new entry
     *  type. Also clear out our used flag which is used to indicte
     *  we've mapped the partition.
     */
    map.in_use_cnt = 0;
    for (i=0;i<map.size-1;i++) {
	if (map.blk[i].pmSig == 0x5453)
	    map.blk[i].pmSig = APPLE_PART_MAP_ENTRY_MAGIC;
	if (map.blk[i].pmSig != APPLE_PART_MAP_ENTRY_MAGIC) {
	    for (j=i+1;j<map.size;j++) {
		if (map.blk[j].pmSig == 0x5453)
		    map.blk[j].pmSig = APPLE_PART_MAP_ENTRY_MAGIC;
		if (map.blk[j].pmSig == APPLE_PART_MAP_ENTRY_MAGIC) {
		    memcpy (&map.blk[i], &map.blk[j], sizeof(map.blk[i]));
		    map.blk[j].pmSig = 0;
		    break;
		}
	    }
	} else {
	    map.in_use_cnt += 1;
	    bzb = (EBZB *)&map.blk[i].pmBootArgs[0];
	    bzb->flags.used = 0;
	    bzb->flags.part = 0;
	}
    }

    /*
     * Step 2, sort by ascending starting block number.  Since
     *         we've already removed the holes we only need to
     *         deal with the in_use count of blocks.
     */
    for (i=0;i<map.in_use_cnt-1;i++) {
	for (j=i+1;j<map.in_use_cnt;j++) {
	    if (map.blk[i].pmPyPartStart > map.blk[j].pmPyPartStart) {
		memcpy (&tmp_blk, &map.blk[i], sizeof(tmp_blk));
		memcpy (&map.blk[i], &map.blk[j], sizeof(map.blk[i]));
		memcpy (&map.blk[j], &tmp_blk, sizeof(map.blk[j]));
	    }
	}
    }

    /*
     * Step 3, merge adjacent free space
     */
    for (i=0;i<map.in_use_cnt-1;i++) {
        if (stricmp("Apple_Free", map.blk[i].pmPartType) == 0 &&
	    stricmp("Apple_Free", map.blk[i+1].pmPartType) == 0) {
	    map.blk[i].pmPartBlkCnt += map.blk[i+1].pmPartBlkCnt;
	    map.blk[i].pmDataCnt += map.blk[i+1].pmDataCnt;
	    map.blk[i+1].pmSig = 0;
	    for (j=i+1;j<map.in_use_cnt-1;j++) {
		memcpy (&map.blk[j], &map.blk[j+1], sizeof(map.blk[j]));
		map.blk[j+1].pmSig = 0;
	    }
	    map.in_use_cnt -= 1;
	}
    }

    /*
     * Step 4, try to identify the mount points for the partitions
     *         and adjust the pmMapBlkCnt in each Map entry.  Set
     *         up the display array for the non-reserved partitions,
     *         and count the number of NetBSD usable partitions
     */
    map.hfs_cnt = 0;
    map.root_cnt = 0;
    map.swap_cnt = 0;
    map.usr_cnt = 0;
    map.usable_cnt = 0;
    /*
     * Clear out record of partition slots already in use
     */
    memset(&in_use, 0, sizeof(in_use));
    for (i=0,j=0;i<map.in_use_cnt;i++) {
        map.blk[i].pmSig = APPLE_PART_MAP_ENTRY_MAGIC;
        map.blk[i].pmMapBlkCnt = map.in_use_cnt;
        if (whichType(&map.blk[i]) && (j < MAXPARTITIONS)) {
		map.mblk[j++] = i;
		map.usable_cnt += 1;
	}
    }
    /*
     * Fill in standard partitions.  Look for a Cluster "0" first and use
     *  it, otherwise take any Cluster value.
     */
    if (findStdType(map.in_use_cnt, in_use, ROOT_PART, &map.root_cnt, 0))
	findStdType(map.in_use_cnt, in_use, ROOT_PART, &map.root_cnt, -1);
    if (findStdType(map.in_use_cnt, in_use, UFS_PART, &map.usr_cnt, 0))
	findStdType(map.in_use_cnt, in_use, UFS_PART, &map.usr_cnt, -1);
    if (findStdType(map.in_use_cnt, in_use, SWAP_PART, &map.swap_cnt, 0))
	findStdType(map.in_use_cnt, in_use, SWAP_PART, &map.swap_cnt, -1);

#if 0
	md_debug_dump("After marking Standard Types");
#endif
    /*
     * Now fill in the remaining partitions counting them by type and
     *  assigning them the slot the where the kernel should map them.
     * This will be where they are displayed in the Edit Map.
     */
    for (i=0; i < map.in_use_cnt; i++) {
	bzb = (EBZB *)&map.blk[i].pmBootArgs[0];
	if (bzb->flags.used == 0) {
	    if ((j = getFreeLabelEntry(in_use)) < 0)
		break;
	    switch (whichType(&map.blk[i])) {
		case ROOT_PART:
		    map.root_cnt += 1;
		    setpartition(&map.blk[i], in_use, j);
		    break;
		case UFS_PART:
		    map.usr_cnt += 1;
		    setpartition(&map.blk[i], in_use, j);
		    break;
		case SWAP_PART:
		    map.swap_cnt += 1;
		    setpartition(&map.blk[i], in_use, j);
		    break;
		case HFS_PART:
		    map.hfs_cnt += 1;
		    setpartition(&map.blk[i], in_use, j);
		    break;
		case SCRATCH_PART:
		    setpartition(&map.blk[i], in_use, j);
		default:
		    break;
	    }
	}
    }
#if 0
	md_debug_dump("After sort merge");
#endif
    return;
}

void
disp_selected_part(sel)
	int sel;
{
	int i,j;
	char fstyp[16], use[16], name[32];
	EBZB *bzb;

	msg_table_add(MSG_part_header);
	for (i=0;i<map.usable_cnt;i++) {
	    if (i == sel) msg_standout();
	    j = map.mblk[i];
	    getFstype(&map.blk[j], sizeof(fstyp), fstyp);
	    getUse(&map.blk[j], sizeof(use), use);
	    getName(&map.blk[j], sizeof(name), name);
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    msg_table_add(MSG_part_row, diskdev,
		bzb->flags.part, map.blk[j].pmPyPartStart,
                 map.blk[j].pmPartBlkCnt, fstyp, use, name);
	    if (i == sel) msg_standend();
	}
	return;
}

/*
 * check for any anomalies on the requested setup
 */
int
check_for_errors()
{
    int i, j;
    int errs = 0;

    errs = (!map.root_cnt) || (map.root_cnt > 1) || (!map.swap_cnt) ||
	   (map.swap_cnt > 1);

    for (i=0;i<map.usable_cnt;i++) {
	j = map.mblk[i];
	if (map.blk[j].pmPyPartStart > dlsize)
		errs++;
	if ((map.blk[j].pmPyPartStart + map.blk[j].pmPartBlkCnt) > dlsize + 1)
		errs++;
    } 
    return(errs);
}

/*
 * check for and report anomalies on the requested setup
 */
void
report_errors()
{
    int i, j;
    int errs = 0;
    EBZB *bzb;

    if (!map.root_cnt) {
	msg_display_add(MSG_disksetup_no_root);
	errs++;
    }
    if (map.root_cnt > 1) {
	msg_display_add(MSG_disksetup_multiple_roots);
	errs++;
    }
    if (!map.swap_cnt) {
	msg_display_add(MSG_disksetup_no_swap);
	errs++;
    }
    if (map.swap_cnt > 1) {
	msg_display_add(MSG_disksetup_multiple_swaps);
	errs++;
    }
    for (i=0;i<map.usable_cnt;i++) {
	j = map.mblk[i];
	if (map.blk[j].pmPyPartStart > dlsize) {
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    msg_display_add(MSG_disksetup_part_beginning,
		diskdev, bzb->flags.part);
	    errs++;
	}
	if ((map.blk[j].pmPyPartStart + map.blk[j].pmPartBlkCnt) > dlsize) {
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    msg_display_add(MSG_disksetup_part_size,
		diskdev, bzb->flags.part);
	    errs++;
	}
    }
    if (!errs)
	msg_display_add(MSG_disksetup_noerrors);
    return;
}

int
edit_diskmap(void)
{
    int i;

	/* Ask full/part */
	msg_display (MSG_fullpart, diskdev);
	process_menu (MENU_fullpart, NULL);

	map.selected = 0;
	sortmerge();

	/* If blowing away the whole disk, let user know if there
	 *  are any active disk partitions */
	if (usefull) {
	    if (map.usable_cnt > (map.root_cnt+map.swap_cnt+map.usr_cnt)) {
		msg_display (MSG_ovrwrite);
		process_menu (MENU_noyes, NULL);
		if (!yesno) {
			endwin();
			return (0);
		}
	    }
	    /*
	     * mark all non-reserved partitions as "free"
	     *  then sort and merge the map into something sensible
	     */
	    for (i=0;i<map.size;i++)
		if (whichType(&map.blk[i]))
		    strcpy (map.blk[i].pmPartType, "Apple_Free");
	    sortmerge();
	}
	process_menu (MENU_editparttable, NULL);
	return (1);
}

int
md_get_info()
{
	struct disklabel disklabel;
	int fd, i;
	char dev_name[100];
	struct apple_part_map_entry block;

	snprintf(dev_name, sizeof(dev_name), "/dev/r%s%c",
		diskdev, 'a' + getrawpartition());

	/*
	 * Open the disk as a raw device
	 */
	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf (stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	/*
	 * Try to get the default disklabel info for the device
	 */
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf (stderr, "Can't read disklabel on %s\n", dev_name);
		close(fd);
		exit(1);
	}
	/*
	 * Get the disk parameters from the disk driver.  It should have
	 *  obained them by querying the disk itself.
	 */
	blk_size = disklabel.d_secsize;
	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	/*
	 * Just in case, initialize the structures we'll need if we
	 *  need to completely initialize the disk.
	 */
	dlsize = disklabel.d_secperunit;
	for (i=0;i<NEW_MAP_SIZE;i++) {
	   if (i > 0)
		new_map[i].pmPyPartStart = new_map[i-1].pmPyPartStart +
			new_map[i-1].pmPartBlkCnt;
	   new_map[i].pmDataCnt = new_map[i].pmPartBlkCnt;
	   if (new_map[i].pmPartBlkCnt == 0) {
		new_map[i].pmPartBlkCnt = dlsize;
		new_map[i].pmDataCnt = dlsize;
		break;
	   }
	   dlsize -= new_map[i].pmPartBlkCnt;
	}
	dlsize = disklabel.d_secperunit;
#if 0
	msg_display(MSG_dldebug, blk_size, dlcyl, dlhead, dlsec, dlsize);
	process_menu(MENU_ok, NULL);
#endif
	map.size = 0;
	/*
	 * Verify the disk has been initialized for MacOS use by checking
	 *  to see if the disk have a Boot Block
	 */
	if (lseek(fd, (off_t)0 * blk_size, SEEK_SET) < 0 ||
	    read(fd,  &block, sizeof(block)) < sizeof(block) ||
	    block.pmSig != 0x4552) {
             process_menu(MENU_nodiskmap, NULL);
        }
	else {
	   /*
	    * Scan for the Partition Map entry that describes the Partition
	    *  Map itself.  We need to know the number of blocks allocated
	    *  to it and the number currently in use.
	    */
	   for (i=0;i<MAXMAXPARTITIONS;i++) {
		lseek(fd, (off_t)(i+1) * blk_size, SEEK_SET);
		read(fd, &block, sizeof(block));
		if (stricmp("Apple_partition_map", block.pmPartType) == 0) {
		    map.size = block.pmPartBlkCnt;
		    map.in_use_cnt = block.pmMapBlkCnt;
		    map.blk = (struct apple_part_map_entry *)malloc(map.size * blk_size);
		    break;
	        }
            }
	    lseek(fd, (off_t)1 * blk_size, SEEK_SET);
	    read(fd, map.blk, map.size * blk_size);
	}
	close(fd);
	/*
	 * Setup the disktype so /etc/disktab gets proper info
	 */
	if (strncmp(diskdev, "sd", 2) == 0) {
		disktype = "SCSI";
		doessf = "sf:";
	} else
		disktype = "IDE";

	return edit_diskmap();
}

int
md_pre_disklabel()
{
    int fd;
    char dev_name[100];
    struct disklabel lp;
    Block0 new_block0 = {APPLE_DRVR_MAP_MAGIC, 512, 0};

    /*
     * Danger Will Robinson!  We're about to turn that nice MacOS disk
     *  into an expensive doorstop...
     */
    printf ("%s", msg_string (MSG_dodiskmap));

    snprintf (dev_name, sizeof(dev_name), "/dev/r%sc", diskdev);
    /*
     * Open the disk as a raw device
     */
    if ((fd = open(dev_name, O_WRONLY, 0)) < 0) {
	endwin();
	fprintf(stderr, "Can't open %s to rewrite the Disk Map\n", dev_name);
	exit (1);
    }
    /*
     *  First check the pmSigPad field of the first block in the incore
     *  Partition Map.  It should be zero, but if it's 0xa5a5 that means
     *  we need to write out Block0 too.
     */
    if (map.blk[0].pmSigPad == 0xa5a5) {
	if (lseek (fd, (off_t)0 * blk_size, SEEK_SET) < 0) {
	    endwin();
	    fprintf (stderr, "Can't position to write Block0\n");
	    close (fd);
	    exit (1);
	}
	new_block0.sbBlkCount = dlsize;		/* Set disk size */
	if (write (fd, &new_block0, blk_size) != blk_size) {
	    endwin();
	    fprintf (stderr, "I/O error writing Block0\n");
	    close (fd);
	    exit (1);
	}
	map.blk[0].pmSigPad = 0;
    }
    if (lseek (fd, (off_t)1 * blk_size, SEEK_SET) < 0) {
	endwin();
	fprintf (stderr, "Can't position disk to rewrite Disk Map\n");
	close (fd);
	exit (1);
    }
    if (write (fd, map.blk, map.size * blk_size) != (map.size * blk_size)) {
	endwin();
	fprintf(stderr, "I/O error writing Disk Map\n");
	close (fd);
	exit (1);
    }
    fsync(fd);
    /*
     * Well, if we get here the dirty deed has been done.
     *
     * Now we need to force the incore disk table to get updated. This
     * should be done by disklabel -- which is normally called right after
     * we return -- but may be commented out for the mac68k port. We'll
     * instead update the incore table by forcing a dummy write here. This
     * relies on a change in the mac68k-specific writedisklabel() routine.
     * If that change doesn't exist nothing bad happens here. If disklabel
     * properly updates the ondisk and incore labels everything still
     * works. Only if we fail here and if disklabel fails are we in
     * in a state where we've updated the disk but not the incore and
     * a reboot is necessary.
     *
     * First, we grab a copy of the incore label as it existed before
     * we did anything to it. Then we invoke the "write label" ioctl to
     * rewrite it to disk. As a result, the ondisk partition map is
     * re-read and the incore label is reconstructed from it. If
     * disklabel() is then called to update again, either that fails
     * because the mac68k port doesn't support native disklabels, or it
     * succeeds and writes out a new ondisk copy.
     */
    ioctl(fd, DIOCGDINFO, &lp);    /* Get the current disk label */
    ioctl(fd, DIOCWDINFO, &lp);    /* Write it out again */

    close (fd);
    return 0;
}

int
md_post_disklabel(void)
{
    struct disklabel updated_label;
    int fd, i, no_match;
    char dev_name[100], buf[80];
    const char *fst[] = {"free", "swap", " v6 ", " v7 ", "sysv", "v71k", 
			" v8 ", "ffs ", "dos ", "lfs ", "othr", "hpfs",
			"9660", "boot", "ados", "hfs ", "fcor", "ex2f",
			"ntfs", "raid", "ccd "};
      
    snprintf(dev_name, sizeof(dev_name), "/dev/r%sc", diskdev);
    /*
     * Open the disk as a raw device
     */
    if ((fd = open(dev_name, O_RDONLY, 0)) < 0)
       return 0;
    /*
     * Get the "new" label to see if we were successful.  If we aren't
     *  we'll return an error to keep from destroying the user's disk.
     */
    ioctl(fd, DIOCGDINFO, &updated_label);
    close(fd);
    /*
     * Make sure the in-core label matches the on-disk one
     */
    no_match = 0;
    for (i=0;i<MAXPARTITIONS;i++) {
        if (i > updated_label.d_npartitions)
           break;
        if (bsdlabel[i].pi_size != updated_label.d_partitions[i].p_size)
           no_match = 1;
        if (bsdlabel[i].pi_size) {
           if (bsdlabel[i].pi_offset != updated_label.d_partitions[i].p_offset)
               no_match = 1;
           if (bsdlabel[i].pi_fstype != updated_label.d_partitions[i].p_fstype)
               no_match = 1;
        }
        if (no_match)
           break;
    }
    /*
     * If the labels don't match, tell the user why
     */
    if (no_match) {
       msg_clear();
       msg_display(MSG_label_error);
       msg_table_add(MSG_dump_line,
           " in-core: offset      size type on-disk: offset      size type");
       for (i=0;i<MAXPARTITIONS;i++) {
           sprintf(buf, " %c:%13.8x%10.8x%5s%16.8x%10.8x%5s", i+'a',
              bsdlabel[i].pi_offset, bsdlabel[i].pi_size,
              fst[bsdlabel[i].pi_fstype],
              updated_label.d_partitions[i].p_offset,
              updated_label.d_partitions[i].p_size,
              fst[updated_label.d_partitions[i].p_fstype]);
           msg_table_add(MSG_dump_line, buf);
       }
       process_menu(MENU_ok2, NULL);
    }
    return no_match;
}

int
md_debug_dump(title)
	char *title;
{
	char buf[96], type;
	char fstyp[16], use[16], name[64];
	int i, j;
	EBZB *bzb;

	msg_clear();
	sprintf(buf, "Apple Disk Partition Map: %s", title);
	msg_table_add(MSG_dump_line, buf);
	msg_table_add(MSG_dump_line,
           "slot      base   fstype        use name");
	for (i=0;i<map.in_use_cnt;i++) {
	   j = whichType(&map.blk[i]);
	   getFstype(&map.blk[i], sizeof(fstyp), fstyp);
	   getUse(&map.blk[i], sizeof(use), use);
	   getName(&map.blk[i], sizeof(name), name);
	   bzb = (EBZB *) &map.blk[i].pmBootArgs[0];
	   type = bzb->flags.part;
	   if (type < 'a' || type > 'h') type = '?';
	   if (j == 0) strcpy (name, "reserved for Apple");
           sprintf(buf, " %02d:%c %08x %8s %10s %s", i+1,  type,
		map.blk[i].pmPyPartStart, fstyp, use, name);
           msg_table_add(MSG_dump_line, buf);
	}
	process_menu(MENU_okabort, NULL);
	msg_clear();
	return(yesno);
}

int
md_post_newfs(void)
{
	return 0;
}

int
md_copy_filesystem(void)
{
	if (target_already_root()) {
		return 1;
	}

	return 0;
}



int
md_make_bsd_partitions(void)
{
	FILE *f;
	int i, j, pl;
	EBZB *bzb;

	/*
	 * Scan for any problems and report them before continuing.
	 *  The user can abort installation and we'll take them back
	 *  to the main menu; continue ignoring the warnings, or
	 *  ask to reedit the Disk Partition Map.
	 */
	while (1) {
	    if (check_for_errors()) {
	        process_menu (MENU_sanity, NULL);
	        if (yesno < 0)
		    return (0);
	        else if (yesno)
		    break;
	        edit_diskmap();
	    } else
		break;
	}

	/* Build standard partitions */
	memset(&bsdlabel, 0, sizeof bsdlabel);

	/*
	 * The mac68k port has a predefined partition for "c" which
	 *  is the size of the disk, everything else is unused.
	 */
	bsdlabel[RAW_PART].pi_size = dlsize;
	/*
	 * Now, scan through the Disk Partition Map and transfer the
	 *  information into the incore disklabel.
	 */
	for (i=0;i<map.usable_cnt;i++) {
	    j = map.mblk[i];
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    if (bzb->flags.part) {
		pl = bzb->flags.part - 'a';
		switch (whichType(&map.blk[j])) {
		    case HFS_PART:
			bsdlabel[pl].pi_fstype = FS_HFS; 
			strcpy (bsdlabel[pl].pi_mount, bzb->mount_point);
			break;
		    case ROOT_PART:
		    case UFS_PART:
			bsdlabel[pl].pi_fstype = FS_BSDFFS;
			strcpy (bsdlabel[pl].pi_mount, bzb->mount_point);
			break;
		    case SWAP_PART:
			bsdlabel[pl].pi_fstype = FS_SWAP;
			break;
		    case SCRATCH_PART:
			bsdlabel[pl].pi_fstype = FS_OTHER;
			strcpy (bsdlabel[pl].pi_mount, bzb->mount_point);
		    default:
			break;
		}
	        if (bsdlabel[pl].pi_fstype != FS_UNUSED) {
		    bsdlabel[pl].pi_size = map.blk[j].pmPartBlkCnt;
		    bsdlabel[pl].pi_offset = map.blk[j].pmPyPartStart;
		    if (bsdlabel[pl].pi_fstype != FS_SWAP) {
		        bsdlabel[pl].pi_frag = 8;
		        bsdlabel[pl].pi_fsize = 1024;
		    }
		}
	    }
	}

	/* Disk name  - don't bother asking, just use the physical name*/
	strcpy (bsddiskname, diskdev);

	/* Create the disktab.preinstall */
	run_program(0, "cp /etc/disktab.preinstall /etc/disktab");
#ifdef DEBUG
	f = fopen ("/tmp/disktab", "a");
#else
	f = fopen ("/etc/disktab", "a");
#endif
	if (f == NULL) {
		endwin();
		(void) fprintf (stderr, "Could not open /etc/disktab");
		exit (1);
	}
	(void)fprintf (f, "%s|NetBSD installation generated:\\\n", bsddiskname);
	(void)fprintf (f, "\t:dt=%s:ty=winchester:\\\n", disktype);
	(void)fprintf (f, "\t:nc#%d:nt#%d:ns#%d:\\\n", dlcyl, dlhead, dlsec);
	(void)fprintf (f, "\t:sc#%d:su#%d:\\\n", dlhead*dlsec, dlsize);
	(void)fprintf (f, "\t:se#%d:%s\\\n", blk_size, doessf);
	for (i=0; i<8; i++) {
		if (bsdlabel[i].pi_fstype == FS_HFS)
		    (void)fprintf (f, "\t:p%c#%d:o%c#%d:t%c=macos:",
			       'a'+i, bsdlabel[i].pi_size,
			       'a'+i, bsdlabel[i].pi_offset,
			       'a'+i);
		else
		    (void)fprintf (f, "\t:p%c#%d:o%c#%d:t%c=%s:",
			       'a'+i, bsdlabel[i].pi_size,
			       'a'+i, bsdlabel[i].pi_offset,
			       'a'+i, fstypenames[bsdlabel[i].pi_fstype]);
		if (bsdlabel[i].pi_fstype == FS_BSDFFS)
			(void)fprintf (f, "b%c#%d:f%c#%d",
			   'a'+i, bsdlabel[i].pi_fsize * bsdlabel[i].pi_frag,
			   'a'+i, bsdlabel[i].pi_fsize);
		if (i < 7)
			(void)fprintf (f, "\\\n");
		else
			(void)fprintf (f, "\n");
	}
	fclose (f);

	/* Everything looks OK. */
	return (1);
}


/* Upgrade support */
int
md_update (void)
{
	endwin();
	move_aout_libs();
	md_copy_filesystem ();
	md_post_newfs();
	wrefresh(curscr);
	wmove(stdscr, 0, 0);
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}


void
md_cleanup_install(void)
{

	enable_rc_conf();

	run_program(0, "rm -f %s", target_expand("/sysinst"));
	run_program(0, "rm -f %s", target_expand("/.termcap"));
	run_program(0, "rm -f %s", target_expand("/.profile"));
}

int
md_pre_update()
{
	return 0;
}

void
md_init()
{
       struct utsname instsys;

	/*
	 * Get the name of the Install Kernel we are running under and
	 * enable the installation of the corresponding GENERIC kernel.
	 *
	 * Note:  In md.h the two kernels are disabled.  If they are
	 *        enabled there the logic here needs to be switched.
	 */
        uname(&instsys);
        if (strstr(instsys.version, "(INSTALLSBC)"))
		/*
		 * Running the SBC Installation Kernel, so enable GENERICSBC
		 */
		sets_selected = (sets_selected & ~SET_KERNEL) | SET_KERNEL_2;
        else
		/*
		 * Running the GENERIC Installation Kernel, so enable GENERIC
		 */
		sets_selected = (sets_selected & ~SET_KERNEL) | SET_KERNEL_1;
}

void
md_set_sizemultname()
{

	set_sizemultname_meg();
}
