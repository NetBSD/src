/*	$NetBSD: md.c,v 1.8 1999/06/22 00:57:09 cgd Exp $ */

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
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

MAP map = {0, 0, 0, 0, 0, 0, 0, 0, {0}};

/* prototypes */

/*
 * Define the known partition type table that we use to
 *  classify what we find on disk.  The ParType fields here could use
 *  the predefined strings that exist in the NetBSD/mac68k disklabel.h
 *  file, but there they are all in uppercase.  Apple really defined
 *  these to be in mixed case, something not all 3rd party disk formatters
 *  honored.  Just to be a purist we'll use the original Apple definitions
 *  here, not that it makes a lot of difference since we always do a
 *  case insensitive string match anyway.
 *
 * Note also that the "Apple_Unix_SVR2" type is initially classified as
 *  an unknown BSD type partition. This will change to a "4.2BSD" type
 *  classification if the appropiate flags are found in the BootArgs
 *  block.  Not all 3rd party disk formatters properly set these flags
 *  so this could be an important tip-off that the disk is not properly
 *  configured for use by NetBSD.
 */
PTYPES ptypes[] = {
	{ TYP_RSRVD, "Apple_partition_map", "reserved"},
	{ TYP_RSRVD, "Apple_Driver", "reserved"},
	{ TYP_RSRVD, "Apple_Driver43", "reserved"},
	{ TYP_RSRVD, "Apple_Driver_ATA", "reserved"},
	{ TYP_RSRVD, "Apple_Patches", "reserved"},
	{ TYP_AVAIL, "Apple_MFS", "MFS"},
	{ TYP_HFS,   "Apple_HFS", "HFS"},
	{ TYP_BSD,   "Apple_Unix_SVR2", "?BSD?"},
	{ TYP_AVAIL, "Apple_PRODOS", "PRODOS"},
	{ TYP_AVAIL, "Apple_Free", "free"},
	{ TYP_AVAIL, "Apple_Scratch", "scratch"},
	{ TYP_AVAIL,  NULL, NULL}};

/*
 * Order of the BSD disk parts as they are assigned in the mac68k port
 */
char bsd_order[] = {'g', 'd', 'e', 'f', 'h'};
char macos_order[] = {'a', 'd', 'e', 'f', 'g', 'h'};

/*
 * Define a default Disk Partition Map that can be used for an uninitialized
 *  disk.
 */
Block0 new_block0 = {0x4552, 512, 0};
 
/*
 * Compare lexigraphically two strings up to a max length
 */
int
strnicmp(s1, s2, n)
	const char *s1;
	const char *s2;
	int n;
{
    int i;
    char c1, c2;
    for (i=0; i<n; i++) {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (!c1) return 0;
    }
    return 0;
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
	int part;
{
    EBZB *bzb;

    /*
     * Clear out the MacOS fields that might be used for booting just
     *  in case we've clobbered the boot code.
     */
    map.blk[part].pmLgDataStart = 0;
    map.blk[part].pmPartStatus = 0x7f;  /* make sure the partition shows up */
    map.blk[part].pmLgBootStart = 0;
    map.blk[part].pmBootSize = 0;
    map.blk[part].pmBootLoad = 0;
    map.blk[part].pmBootLoad2 = 0;
    map.blk[part].pmBootEntry = 0;
    map.blk[part].pmBootEntry2 = 0;
    map.blk[part].pmBootCksum = 0;

    /*
     * Clear out all the NetBSD fields too.  We only clear out the ones
     *  that should get reset during our processing.
     */
    bzb = (EBZB *)&map.blk[part].pmBootArgs[0];
    bzb->magic = 0;
    bzb->cluster = 0;
    bzb->inode = 0;
    bzb->type = 0;
    bzb->flags.root = 0;
    bzb->flags.usr = 0;
    bzb->flags.crit = 0;
    bzb->flags.slice = 0;
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
    struct part_map_entry tmp_blk;
    char fstyp[16], use[16], name[64];
    int i, j, k;
    EBZB *bzb;

    /*
     * Step 1, squeeze out the holes
     */
    for (i=0;i<map.size-1;i++) {
	if (map.blk[i].pmSig != PART_ENTRY_MAGIC && map.blk[i].pmSig != 0x5453) {
	    for (j=i+1;j<map.size;j++) {
		if (map.blk[j].pmSig == PART_ENTRY_MAGIC || map.blk[j].pmSig == 0x5453) {
		    memcpy (&map.blk[i], &map.blk[j], sizeof(struct part_map_entry));
		    map.blk[j].pmSig = 0;
		    break;
		}
	    }
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
		memcpy (&tmp_blk, &map.blk[i],  sizeof(struct part_map_entry));
		memcpy (&map.blk[i], &map.blk[j], sizeof(struct part_map_entry));
		memcpy (&map.blk[j], &tmp_blk, sizeof(struct part_map_entry));
	    }
	}
    }

    /*
     * Step 3, merge adjacent free space
     */
    for (i=0;i<map.in_use_cnt-1;i++) {
        if (strnicmp("Apple_Free", map.blk[i].pmPartType,
		sizeof(map.blk[i].pmPartType)) == 0 &&
	    strnicmp("Apple_Free", map.blk[i+1].pmPartType,
		sizeof(map.blk[i+1].pmPartType)) == 0) {
	    map.blk[i].pmPartBlkCnt += map.blk[i+1].pmPartBlkCnt;
	    map.blk[i].pmDataCnt += map.blk[i+1].pmDataCnt;
	    map.blk[i+1].pmSig = 0;
	    for (j=i+1;j<map.in_use_cnt-1;j++) {
		memcpy (&map.blk[j], &map.blk[j+1], sizeof(struct part_map_entry));
		map.blk[j+1].pmSig = 0;
	    }
	    map.in_use_cnt -= 1;
	}
    }

    /*
     * Step 4, try to identify the mount points for the partitions
     *         Also, force use of PART_ENTRY_MAGIC in place of the old
     *         0x5453 pmSig value, and adjust the pmMapBlkCnt in
     *         each Map entry.  Set up the display array for the
     *         non-reserved partitions, and count the number of
     *         NetBSD usable partitions
     */
    map.hfs_cnt = 0;
    map.root_cnt = 0;
    map.swap_cnt = 0;
    map.usr_cnt = 0;
    map.usable_cnt = 0;
    for (i=0,j=0;i<map.in_use_cnt;i++) {
        map.blk[i].pmSig = PART_ENTRY_MAGIC;
        map.blk[i].pmMapBlkCnt = map.in_use_cnt;
        if ((part_type(i, fstyp, use, name) != TYP_RSRVD) &&
	    (j < MAXPARTITIONS)) {
		map.mblk[j++] = i;
		map.usable_cnt += 1;
	}
	if (strnicmp("Apple_Unix_SVR2", map.blk[i].pmPartType,
		sizeof(map.blk[i].pmPartType)) == 0) {
	   bzb = (EBZB *)&map.blk[i].pmBootArgs[0];
	   bzb->flags.part = '?';
	   if (bzb->magic == BZB_MAGIC) {
		switch (bzb->type) {
		    case BZB_TYPEFS:
			if (bzb->flags.root && !map.root_cnt) {
			    map.root_cnt = 1;
			    bzb->flags.part = 'a';
                            strcpy (bzb->mount_point, "/");
			}
			else if (bzb->flags.root || bzb->flags.usr)
			    bzb->flags.part = bsd_order[map.usr_cnt++];
			break;
		    case BZB_TYPESWAP:
			if (!map.swap_cnt)
			    bzb->flags.part = 'b';
			map.swap_cnt += 1;
			break;
		    default:
			break;
		}
	   }
	}
    } 

    /*
     * Map any remaining slots and partitions to any other non-reserved
     *  partition type on the disk. This picks up the MacOS HFS parts.
     */
    if (map.root_cnt+map.usr_cnt+map.swap_cnt < MAXPARTITIONS) {
	for (i=0,k=map.root_cnt;i<map.in_use_cnt;i++) {
	    j = part_type(i, fstyp, use, name);
	    if (j == TYP_AVAIL || j == TYP_HFS) {
		bzb = (EBZB *)&map.blk[i].pmBootArgs[0];
		bzb->flags.part = macos_order[k++];
                if (j == TYP_HFS)
                   map.hfs_cnt += 1;
	    }
	}
    }
    return;
}

/*
 * Identify the Partition Map entry by Filesystem type, Use and
 *  assigned name.  For MacOS partitions we try to pull up the
 *  disk partition name; for BSD type partitions we attempt to
 *  pull up the mount_point which will be assigned by the user.
 */
int
part_type(entry, fstyp, use, name)
	int entry;
	char *fstyp;
	char *use;
	char *name;
{
    PTYPES *ptr = (PTYPES *)&ptypes;
    EBZB *bzb;
    int fd;
    off_t seek;
    char devname[100], macosblk[512];

    while (ptr->partype) {
	if (strnicmp(ptr->partype, map.blk[entry].pmPartType,
	    sizeof(map.blk[entry].pmPartType)) == 0) {
	    strcpy(fstyp, ptr->fstyp);
	    strncpy(name, map.blk[entry].pmPartType,
		sizeof(map.blk[entry].pmPartType));
	    name[sizeof(map.blk[entry].pmPartType)] = '\0';
	    strcpy(use, "unknown");
	    switch (ptr->usable) {
		case 2:  /* MacOS HFS */
		    /*
		     * OK, this is stupid but it's damn nice to know!
		     */
	 	    snprintf (devname, sizeof(devname), "/dev/r%sc", diskdev);
		    /*
	 	     * Open the disk as a raw device
	 	     */
		    if ((fd = open(devname, O_RDONLY, 0)) >= 0) {
			seek = (off_t)map.blk[entry].pmPyPartStart + (off_t)2;
			seek *= (off_t)bsize;
			lseek(fd, seek, SEEK_SET);
			read(fd, &macosblk, sizeof(macosblk));
			macosblk[37+32] = '\0';
			bzb = (EBZB *)&map.blk[entry].pmBootArgs[0];
			sprintf(name, "%s (%s)", bzb->mount_point,
			    &macosblk[37]);
		    }
		    strcpy (use, "MacOS");
		    break;
		case 3:  /* Unix */
		    bzb = (EBZB *)&map.blk[entry].pmBootArgs[0];
		    if (bzb->magic == BZB_MAGIC) {
			switch (bzb->type) {
			    case BZB_TYPEFS:
				strcpy (fstyp, "4.2BSD");
				if (bzb->flags.root && bzb->flags.usr) {
				   strcpy(use, "Root&Usr");
				   sprintf(name, "%s", bzb->mount_point);
				}
				else if (bzb->flags.root) {
				   strcpy(use, "Root");
				   sprintf(name, "%s", bzb->mount_point);
				}
				else if (bzb->flags.usr) {
				   strcpy(use, "Usr");
				   sprintf(name, "%s", bzb->mount_point);
				}
				else {
				   strcpy (use, "unknown");
				   *name = '\0';
				}
				break;
			    case BZB_TYPESWAP:
				strcpy (fstyp, "4.2BSD");
				strcpy (use, "swap");
				*name = '\0';
				break;
			    default:
				break;
			}
		    }
		    else {
			strcpy (use, "unknown");
			*name = '\0';
		    }
		    break;
		default:
		    break;
	    }
	    return (ptr->usable);
	}
	ptr++;
    }
    *fstyp = '\0';
    *name = '\0';
    *use = '\0';
    return (0);
}

void
disp_selected_part(sel)
	int sel;
{
    int i,j;
    char name[64], use[16], fstyp[16];
    EBZB *bzb;

    msg_display_add (MSG_part_head);
    for (i=0;i<map.usable_cnt;i++) {
        if (i == sel) msg_standout();
        j = map.mblk[i];
        part_type(j, fstyp, use, name);
        bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
        msg_printf_add ("%s%c%12d%12d%8s%9s  %s\n", diskdev,
            bzb->flags.part, map.blk[j].pmPartBlkCnt,
            map.blk[j].pmPyPartStart, fstyp, use, name);
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
	msg_printf_add ("  No Disk Partition defined for Root!\n");
	errs++;
    }
    if (map.root_cnt > 1) {
	msg_printf_add ("  Multiple Disk Partitions defined for Root\n");
	errs++;
    }
    if (!map.swap_cnt) {
	msg_printf_add ("  No Disk Partition defined for SWAP!\n");
	errs++;
    }
    if (map.swap_cnt > 1) {
	msg_printf_add ("  Multiple Disk Partitions defined for SWAP\n");
	errs++;
    }
    for (i=0;i<map.usable_cnt;i++) {
	j = map.mblk[i];
	if (map.blk[j].pmPyPartStart > dlsize) {
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    msg_printf_add ("  Partition %s%c begins beyond end of disk\n",
		diskdev, bzb->flags.part);
	    errs++;
	}
	if ((map.blk[j].pmPyPartStart + map.blk[j].pmPartBlkCnt) > dlsize) {
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    msg_printf_add ("  Partition %s%c extends beyond end of disk\n",
		diskdev, bzb->flags.part);
	    errs++;
	}
    }
    if (!errs)
	msg_printf_add ("** No errors or anomalies found in disk setup.\n");
    return;
}

int
edit_diskmap(void)
{
    int i;
    char fstyp[16], use[16], name[64];

	/* Ask full/part */
	msg_display (MSG_fullpart, diskdev);
	process_menu (MENU_fullpart);

	map.selected = 0;
	sortmerge();

	/* If blowing away the whole disk, let user know if there
	 *  are any active disk partitions */
	if (usefull) {
	    if (map.usable_cnt > (map.root_cnt+map.swap_cnt+map.usr_cnt)) {
		msg_display (MSG_ovrwrite);
		process_menu (MENU_noyes);
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
		if (part_type(i, fstyp, use, name))
		    strcpy (map.blk[i].pmPartType, "Apple_Free");
	    sortmerge();
	}
	process_menu (MENU_editparttable);
	return (1);
}

int
md_get_info()
{
	struct disklabel disklabel;
	int fd, i;
	char devname[100];
	struct part_map_entry block;

	snprintf (devname, sizeof(devname), "/dev/r%sc", diskdev);

	/*
	 * Open the disk as a raw device
	 */
	fd = open(devname, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf (stderr, "Can't open %s\n", devname);
		exit(1);
	}
	/*
	 * Try to get the default disklabel info for the device
	 */
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf (stderr, "Can't read disklabel on %s\n", devname);
		close(fd);
		exit(1);
	}
	/*
	 * Get the disk parameters from the disk driver.  It should have
	 *  obained them by querying the disk itself.
	 */
	bsize = disklabel.d_secsize;
	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	/*
	 * Just in case, initialize the structures we'll need if we
	 *  need to completely initialize the disk.
	 */
	dlsize = dlsec*dlhead*dlcyl;
	new_block0.sbBlkCount = dlsize;
	for (i=0;i<NEW_MAP_SIZE;i++) {
	   if (i > 0)
		new_map[i].pmPyPartStart = new_map[i-1].pmPyPartStart +
			new_map[i-1].pmPartBlkCnt;
	   new_map[i].pmDataCnt = new_map[i].pmPartBlkCnt;
	   if (new_map[i].pmPartBlkCnt) {
		new_map[i].pmPartBlkCnt = dlsize;
		new_map[i].pmDataCnt = dlsize;
		break;
	   }
	   dlsize -= new_map[i].pmPartBlkCnt;
	}
	dlsize = dlsec*dlhead*dlcyl;
#if 0
	msg_display(MSG_dldebug, bsize, dlcyl, dlhead, dlsec, dlsize);
	process_menu(MENU_ok);
#endif
	/*
	 * Verify the disk has been initialized for MacOS use by checking
	 *  to see if the disk have a Boot Block
	 */
	if (lseek(fd, (off_t)0 * bsize, SEEK_SET) < 0 ||
	    read(fd,  &block, sizeof(struct part_map_entry)) < sizeof(block) ||
	    block.pmSig != 0x4552) {
             process_menu(MENU_nodiskmap);
        }
	else {
	   /*
	    * Scan for the Partition Map entry that describes the Partition
	    *  Map itself.  We need to know the number of blocks allocated
	    *  to it and the number currently in use.
	    */
	   for (i=0;i<MAXMAXPARTITIONS;i++) {
		lseek(fd, (off_t)(i+1) * bsize, SEEK_SET);
		read(fd, &block, sizeof(struct part_map_entry));
		if (strnicmp("Apple_partition_map", block.pmPartType,
		    sizeof(block.pmPartType)) == 0) {
		    map.size = block.pmPartBlkCnt;
		    map.in_use_cnt = block.pmMapBlkCnt;
		    map.blk = (struct part_map_entry *)malloc(map.size * bsize);
		    break;
	        }
            }
	    lseek(fd, (off_t)1 * bsize, SEEK_SET);
	    read(fd, map.blk, map.size * bsize);
	}
	close(fd);

	/*
	 * Setup the disktype so /etc/disktab gets proper info
	 */
	if (strncmp (disk->dd_name, "sd", 2) == 0) {
	    disktype = "SCSI";
	    doessf = "sf:";
	}
	else
	    disktype = "IDE";

	return edit_diskmap();
}

int
md_pre_disklabel()
{
    int fd;
    char devname[100];

    /*
     * Danger Will Robinson!  We're about to turn that nice MacOS disk
     *  into an expensive doorstop...
     *
     * Always remember that this code was written in a town called
     *  Murphy, so anything that can go wrong will.
     */
    printf ("%s", msg_string (MSG_dodiskmap));

    snprintf (devname, sizeof(devname), "dev/r%sc", diskdev);

    /*
     * Open the disk as a raw device
     */
    if ((fd = open(devname, O_WRONLY, 0)) < 0) {
	endwin();
	fprintf(stderr, "Can't open %s to rewrite the Disk Map\n", devname);
	exit (1);
    }
    /*
     * OK boys and girls, here we go.  First check the pmSigPad field of
     *  the first block in the incore Partition Map.  It should be zero,
     *  but if it's 0xa5a5 that means we need to write out Block0 too.
     */
    if (map.blk[0].pmSigPad == 0xa5a5) {
	if (lseek (fd, (off_t)0 * bsize, SEEK_SET) < 0) {
	    endwin();
	    fprintf (stderr, "Can't position to write Block0\n");
	    close (fd);
	    exit (1);
	}
	if (write (fd, &new_block0, bsize) != bsize) {
	    endwin();
	    fprintf (stderr, "I/O error writing Block0\n");
	    close (fd);
	    exit (1);
	}
	map.blk[0].pmSigPad = 0;
    }
    if (lseek (fd, (off_t)1 * bsize, SEEK_SET) < 0) {
	endwin();
	fprintf (stderr, "Can't position disk to rewrite Disk Map\n");
	close (fd);
	exit (1);
    }
    if (write (fd, map.blk, map.size * bsize) != (map.size * bsize)) {
	endwin();
	fprintf(stderr, "I/O error writing Disk Map\n");
	close (fd);
	exit (1);
    }
    /*
     * Well, if we get here the dirty deed has been done.
     */
    close (fd);
    return 0;
}

int
md_post_disklabel(void)
{
	return 0;
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

	/* Copy the instbin(s) to the disk */
	msg_display(MSG_dotar);
	if (run_prog (0, 0, NULL, "pax -X -r -w -pe / /mnt") != 0)
		return 1;

	/* Copy next-stage install profile into target /.profile. */
	if (cp_to_target ("/tmp/.hdprofile", "/.profile") != 0)
		return 1;
	return cp_to_target ("/usr/share/misc/termcap", "/.termcap");
}



int
md_make_bsd_partitions(void)
{
	FILE *f;
	int i, j, pl;
	char fstyp[16], use[16], name[64];
	EBZB *bzb;

	/*
	 * Scan for any problems and report them before continuing.
	 *  The user can abort installation and we'll take them back
	 *  to the main menu; continue ignoring the warnings, or
	 *  ask to reedit the Disk Partition Map.
	 */
	while (1) {
	    if (check_for_errors()) {
	        process_menu (MENU_sanity);
	        if (yesno < 0)
		    return (0);
	        else if (yesno)
		    break;
	        edit_diskmap();
	    } else
		break;
	}

	/* Build standard partitions */
	emptylabel(bsdlabel);

	/*
	 * The mac68k port has no predefined partitions, so we'll
	 *  start by setting everything to unused.
	 */
	for (i=0;i<MAXPARTITIONS;i++) {
	    bsdlabel[i].pi_size = 0;
	    bsdlabel[i].pi_offset = 0;
	    bsdlabel[i].pi_fstype = FS_UNUSED;
	    bsdlabel[i].pi_bsize = 0;
	    bsdlabel[i].pi_fsize = 0;
	    fsmount[i][0] = '\0';
	}
	/*
	 * Now, scan through the Disk Partition Map and transfer the
	 *  information into the incore disklabel.
	 */
	for (i=0;i<map.usable_cnt;i++) {
	    j = map.mblk[i];
	    bzb = (EBZB *)&map.blk[j].pmBootArgs[0];
	    if (bzb->flags.part) {
		pl = bzb->flags.part - 'a';
		switch (part_type(j, fstyp, use, name)) {
		    case TYP_HFS:
			bsdlabel[pl].pi_fstype = FS_HFS; 
			strcpy (fsmount[pl], bzb->mount_point);
			break;
		    case TYP_BSD:
			switch (bzb->type) {
			    case BZB_TYPEFS:
				bsdlabel[pl].pi_fstype = FS_BSDFFS;
				strcpy (fsmount[pl], bzb->mount_point);
				break;
			    case BZB_TYPESWAP:
				bsdlabel[pl].pi_fstype = FS_SWAP;
				break;
			    default:
				break;
			}
			break;
		    default:
			break;
		}
	        if (bsdlabel[pl].pi_fstype != FS_UNUSED) {
		    bsdlabel[pl].pi_size = map.blk[j].pmPartBlkCnt;
		    bsdlabel[pl].pi_offset = map.blk[j].pmPyPartStart;
		    if (bsdlabel[pl].pi_fstype != FS_SWAP) {
		        bsdlabel[pl].pi_bsize = 8192;
		        bsdlabel[pl].pi_fsize = 1024;
		    }
		}
	    }
	}

	/* Disk name  - don't bother asking, just use the physical name*/
	strcpy (bsddiskname, diskdev);

	/* Create the disktab.preinstall */
	run_prog (0, 0, NULL, "cp /etc/disktab.preinstall /etc/disktab");
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
	(void)fprintf (f, "\t:se#%d:%s\\\n", bsize, doessf);
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
				       'a'+i, bsdlabel[i].pi_bsize,
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
	md_copy_filesystem ();
	md_post_newfs();
	puts(CL);		/* XXX */
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}


void
md_cleanup_install(void)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];
	char sedcmd[STRSIZE];

	strncpy(realfrom, target_expand("/etc/rc.conf"), STRSIZE);
	strncpy(realto, target_expand("/etc/rc.conf.install"), STRSIZE);

	sprintf(sedcmd, "sed 's/rc_configured=NO/rc_configured=YES/' < %s > %s",
	    realfrom, realto);
	if (logging)
		(void)fprintf(log, "%s\n", sedcmd);
	if (scripting)
		(void)fprintf(script, "%s\n", sedcmd);
	do_system(sedcmd);

	run_prog(1, 0, NULL, "mv -f %s %s", realto, realfrom);
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/.profile"));
}

