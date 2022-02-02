/*	$NetBSD: md.c,v 1.5.2.2 2022/02/02 04:25:38 msaitoh Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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
 */

/* md.c -- arm32 machine specific routines */

#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <sys/types.h>
#include <sys/disklabel_acorn.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

static int filecore_checksum(u_char *);

void
md_init(void)
{
}

void
md_init_set_status(int flags)
{
	(void)flags;
}

bool
md_get_info(struct install_partition_desc *install)
{
	struct disklabel disklabel;
	int fd;
	char dev_name[100];
	static unsigned char bb[DEV_BSIZE];
	struct filecore_bootblock *fcbb = (struct filecore_bootblock *)bb;
	int offset = 0;

	snprintf(dev_name, 100, "/dev/r%s%c", pm->diskdev, 'a' + getrawpartition());

	fd = open(dev_name, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf(stderr, "Can't open %s\n", dev_name);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", dev_name);
		close(fd);
		exit(1);
	}

	if (lseek(fd, (off_t)FILECORE_BOOT_SECTOR * DEV_BSIZE, SEEK_SET) < 0
	    || read(fd, bb, sizeof(bb)) - sizeof(bb) != 0) {
		endwin();
		fprintf(stderr, "%s", msg_string(MSG_badreadbb));
		close(fd);
		exit(1);
	}

	/* Check if table is valid. */
	if (filecore_checksum(bb) == fcbb->checksum) {
		/*
		 * Check for NetBSD/arm32 (RiscBSD) partition marker.
		 * If found the NetBSD disklabel location is easy.
		 */

		offset = (fcbb->partition_cyl_low +
		    (fcbb->partition_cyl_high << 8)) *
		    fcbb->heads * fcbb->secspertrack;

		if (fcbb->partition_type == PARTITION_FORMAT_RISCBSD)
			;
		else if (fcbb->partition_type == PARTITION_FORMAT_RISCIX) {
			/*
     			 * Ok we need to read the RISCiX partition table and
			 * search for a partition named RiscBSD, NetBSD or
			 * Empty:
			 */

			struct riscix_partition_table *riscix_part =
			    (struct riscix_partition_table *)bb;
			struct riscix_partition *part;
			int loop;

			if (lseek(fd, (off_t)offset * DEV_BSIZE, SEEK_SET) < 0
			    || read(fd, bb, sizeof(bb)) - sizeof(bb) != 0) {
				endwin();
				fprintf(stderr, "%s",
				    msg_string(MSG_badreadriscix));
				close(fd);
				exit(1);
			}

			/* Break out as soon as we find a suitable partition */
			for (loop = 0; loop < NRISCIX_PARTITIONS; ++loop) {
				part = &riscix_part->partitions[loop];
				if (strcmp((char *)part->rp_name, "RiscBSD") == 0
				    || strcmp((char *)part->rp_name, "NetBSD") == 0
				    || strcmp((char *)part->rp_name, "Empty:") == 0) {
					offset = part->rp_start;
					break;
				}
			}
			if (loop == NRISCIX_PARTITIONS) {
				/*
				 * Valid filecore boot block, RISCiX partition
				 * table but no NetBSD partition. We should
				 * leave this disc alone.
				 */
				endwin();
				fprintf(stderr, "%s",
				    msg_string(MSG_notnetbsdriscix));
				close(fd);
				exit(1);
			}
		} else {
			/*
			 * Valid filecore boot block and no non-ADFS partition.
			 * This means that the whole disc is allocated for ADFS
			 * so do not trash ! If the user really wants to put a
			 * NetBSD disklabel on the disc then they should remove
			 * the filecore boot block first with dd.
			 */
			endwin();
			fprintf(stderr, "%s", msg_string(MSG_notnetbsd));
			close(fd);
			exit(1);
		}
	}
	close(fd);

	pm->dlcyl = disklabel.d_ncylinders;
	pm->dlhead = disklabel.d_ntracks;
	pm->dlsec = disklabel.d_nsectors;
	pm->sectorsize = disklabel.d_secsize;
	pm->dlcylsize = disklabel.d_secpercyl;

	/*
	 * Compute whole disk size. Take max of (pm->dlcyl*pm->dlhead*pm->dlsec)
	 * and secperunit,  just in case the disk is already labelled.
	 * (If our new label's RAW_PART size ends up smaller than the
	 * in-core RAW_PART size  value, updating the label will fail.)
	 */
	pm->dlsize = pm->dlcyl*pm->dlhead*pm->dlsec;
	if (disklabel.d_secperunit > pm->dlsize)
		pm->dlsize = disklabel.d_secperunit;

	pm->ptstart = offset;
/*	endwin();
	printf("pm->dlcyl=%d\n", pm->dlcyl);
	printf("pm->dlhead=%d\n", pm->dlhead);
	printf("pm->dlsec=%d\n", pm->dlsec);
	printf("secsz=%d\n", pm->sectorsize);
	printf("cylsz=%d\n", pm->dlcylsize);
	printf("dlsz=%d\n", pm->dlsize);
	printf("pstart=%d\n", pm->ptstart);
	printf("pstart=%d\n", partsize);
	printf("secpun=%d\n", disklabel.d_secperunit);
	backtowin();*/

	return true;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
md_make_bsd_partitions(struct install_partition_desc *install)
{
	return make_bsd_partitions(install);
}

/*
 * any additional partition validation
 */
bool
md_check_partitions(struct install_partition_desc *install)
{
	return 1;
}

/*
 * hook called before writing new disklabel.
 */
bool
md_pre_disklabel(struct install_partition_desc *install,
   struct disk_partitions *part)
{
	return 0;
}

/*
 * hook called after writing disklabel to new target disk.
 */
bool
md_post_disklabel(struct install_partition_desc *install,
   struct disk_partitions *part)
{
	return true;
}

/*
 * hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message.
 */
int
md_post_newfs(struct install_partition_desc *install)
{
	return 0;
}

int
md_post_extract(struct install_partition_desc *install, bool upgrade)
{
	return 0;
}

void
md_cleanup_install(struct install_partition_desc *install)
{
#ifndef DEBUG
	enable_rc_conf();
#endif
}

int
md_pre_update(struct install_partition_desc *install)
{
	return 1;
}

/* Upgrade support */
int
md_update(struct install_partition_desc *install)
{
	md_post_newfs(install);
	return 1;
}

/*
 * static int filecore_checksum(u_char *bootblock)
 *
 * Calculates the filecore boot block checksum. This is used to validate
 * a filecore boot block on the disk.  If a boot block is validated then
 * it is used to locate the partition table. If the boot block is not
 * validated, it is assumed that the whole disk is NetBSD.
 *
 * The basic algorithm is:
 *
 *	for (each byte in block, excluding checksum) {
 *		sum += byte;
 *		if (sum > 255)
 *			sum -= 255;
 *	}
 *
 * That's equivalent to summing all of the bytes in the block
 * (excluding the checksum byte, of course), then calculating the
 * checksum as "cksum = sum - ((sum - 1) / 255) * 255)".  That
 * expression may or may not yield a faster checksum function,
 * but it's easier to reason about.
 *
 * Note that if you have a block filled with bytes of a single
 * value "X" (regardless of that value!) and calculate the cksum
 * of the block (excluding the checksum byte), you will _always_
 * end up with a checksum of X.  (Do the math; that can be derived
 * from the checksum calculation function!)  That means that
 * blocks which contain bytes which all have the same value will
 * always checksum properly.  That's a _very_ unlikely occurence
 * (probably impossible, actually) for a valid filecore boot block,
 * so we treat such blocks as invalid.
 */

static int
filecore_checksum(u_char *bootblock)
{
	u_char byte0, accum_diff;
	u_int sum;
	int i;

	sum = 0;
	accum_diff = 0;
	byte0 = bootblock[0];

	/*
	 * Sum the contents of the block, keeping track of whether
	 * or not all bytes are the same.  If 'accum_diff' ends up
	 * being zero, all of the bytes are, in fact, the same.
	 */
	for (i = 0; i < 511; ++i) {
		sum += bootblock[i];
		accum_diff |= bootblock[i] ^ byte0;
	}

	/*
	 * Check to see if the checksum byte is the same as the
	 * rest of the bytes, too.  (Note that if all of the bytes
	 * are the same except the checksum, a checksum compare
	 * won't succeed, but that's not our problem.)
	 */
	accum_diff |= bootblock[i] ^ byte0;

	/* All bytes in block are the same; call it invalid. */
	if (accum_diff == 0)
		return (-1);

	return (sum - ((sum - 1) / 255) * 255);
}

int
md_pre_mount(struct install_partition_desc *install, size_t ndx)
{
	return 0;
}

#ifdef HAVE_GPT
/*
 * New GPT partitions have been written, update bootloader or remember
 * data untill needed in md_post_newfs
 */
bool
md_gpt_post_write(struct disk_partitions *parts, part_id root_id,
    bool root_is_new, part_id efi_id, bool efi_is_new)
{

	return true;
}
#endif

bool
md_parts_use_wholedisk(struct disk_partitions *parts)
{
	return parts_use_wholedisk(parts, 0, NULL);
}
