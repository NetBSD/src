/*	$NetBSD: md.c,v 1.10 1999/01/23 06:19:17 garbled Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software develooped for the NetBSD Project by
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

/* md.c -- arm32 machine specific routines */

#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
void backtowin(void);

static u_int
filecore_checksum(u_char *bootblock);

/*
 * static u_int filecore_checksum(u_char *bootblock)
 *
 * Calculates the filecore boot block checksum. This is used to validate
 * a filecore boot block on the disc. If a boot block is validated then
 * it is used to locate the partition table. If the boot block is not
 * validated, it is assumed that the whole disc is NetBSD.
 */

/*
 * This can be coded better using add with carry but as it is used rarely
 * there is not much point writing it in assembly.
 */
 
 /* NEEDS THE LASTEST CODE IMPORTING */
static u_int
filecore_checksum(bootblock)
	u_char *bootblock;
{
	u_int sum;
	u_int loop;
    
	sum = 0;

	for (loop = 0; loop < 512; ++loop)
		sum += bootblock[loop];

	if (sum == 0) return(0xffff);

	sum = 0;
    
	for (loop = 0; loop < 511; ++loop) {
		sum += bootblock[loop];
		if (sum > 255)
			sum -= 255;
	}

	return(sum);
}


int	md_get_info (void)
{	struct disklabel disklabel;
	int fd;
	char devname[100];
	static char bb[DEV_BSIZE];
	struct filecore_bootblock *fcbb = (struct filecore_bootblock *)bb;
	int offset = 0;

	if (strncmp(disk->name, "wd", 2) == 0)
		disktype = "ST506";
	else
		disktype = "SCSI";

	snprintf(devname, 100, "/dev/r%sc", diskdev);

	fd = open(devname, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf(stderr, "Can't open %s\n", devname);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf(stderr, "Can't read disklabel on %s.\n", devname);
		close(fd);
		exit(1);
	}

	if (lseek(fd, (off_t)FILECORE_BOOT_SECTOR * DEV_BSIZE, SEEK_SET) < 0
	    || read(fd, bb, sizeof(bb)) < sizeof(bb)) {
		endwin();
		fprintf(stderr, msg_string(MSG_badreadbb));
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
			    || read(fd, bb, sizeof(bb)) < sizeof(bb)) {
				endwin();
				fprintf(stderr, msg_string(MSG_badreadriscix));
				close(fd);
				exit(1);
			}

			/* Break out as soon as we find a suitable partition */
			for (loop = 0; loop < NRISCIX_PARTITIONS; ++loop) {
				part = &riscix_part->partitions[loop];
				if (strcmp(part->rp_name, "RiscBSD") == 0
				    || strcmp(part->rp_name, "NetBSD") == 0
				    || strcmp(part->rp_name, "Empty:") == 0) {
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
				fprintf(stderr, msg_string(MSG_notnetbsdriscix));
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
			fprintf(stderr, msg_string(MSG_notnetbsd));
			close(fd);
			exit(1);
		}
	}
	close(fd);
 
	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;

	/*
	 * Compute whole disk size. Take max of (dlcyl*dlhead*dlsec)
	 * and secperunit,  just in case the disk is already labelled.  
	 * (If our new label's RAW_PART size ends up smaller than the
	 * in-core RAW_PART size  value, updating the label will fail.)
	 */
	dlsize = dlcyl*dlhead*dlsec;
	if (disklabel.d_secperunit > dlsize)
		dlsize = disklabel.d_secperunit;

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = (80 + 4*rammb) * (MEG / sectorsize);

	ptstart = offset;
/*	endwin();
	printf("dlcyl=%d\n", dlcyl);
	printf("dlhead=%d\n", dlhead);
	printf("dlsec=%d\n", dlsec);
	printf("secsz=%d\n", sectorsize);
	printf("cylsz=%d\n", dlcylsize);
	printf("dlsz=%d\n", dlsize);
	printf("pstart=%d\n", ptstart);
	printf("pstart=%d\n", partsize);
	backtowin();*/

	return 1;
}

void	md_pre_disklabel (void)
{
}

void	md_post_disklabel (void)
{
}

void	md_post_newfs (void)
{
#if 0
	/* XXX boot blocks ... */
	printf(msg_string(MSG_dobootblks), diskdev);
	run_prog(0, 1, "/sbin/disklabel -B %s /dev/r%sc",
	    "-b /usr/mdec/rzboot -s /usr/mdec/bootrz", diskdev);
#endif
}

void	md_copy_filesystem (void)
{
	if (target_already_root()) {
		return;
	}

	/* Copy the instbin(s) to the disk */
	printf("%s", msg_string(MSG_dotar));
	run_prog(0, 1, "pax -X -r -w / /mnt");

	/* Copy next-stage install profile into target /.profile. */
	cp_to_target("/tmp/.hdprofile", "/.profile");
	cp_to_target("/usr/share/misc/termcap", "/.termcap");
}

int md_make_bsd_partitions (void)
{
	int i, part;
	int remain;
	char isize[20];
	int maxpart = getmaxpartitions();

	/*
	 * Initialize global variables that track  space used on this disk.
	 * Standard 4.3BSD 8-partition labels always cover whole disk.
	 */
	ptsize = dlsize - ptstart;
	fsdsize = dlsize;		/* actually means `whole disk' */
	fsptsize = dlsize - ptstart;	/* netbsd partition -- same as above */
	fsdmb = fsdsize / MEG;

/*	endwin();
	printf("ptsize=%d\n", ptsize);
	printf("fsdsize=%d\n", fsdsize);
	printf("fsptsize=%d\n", fsptsize);
	printf("fsdmb=%d\n", fsdmb);
	backtowin();*/

/*editlab:*/
	/* Ask for layout type -- standard or special */
	msg_display (MSG_layout,
			(1.0*fsptsize*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu(MENU_layout);

	if (layoutkind == 3) {
		ask_sizemult();
	} else {
		sizemult = MEG / sectorsize;
		multname = msg_string(MSG_megname);
	}


	/* Build standard partitions */
	emptylabel(bsdlabel);

	/* Partitions C is predefined (whole  disk). */
	bsdlabel[C][D_FSTYPE] = T_UNUSED;
	bsdlabel[C][D_OFFSET] = 0;
	bsdlabel[C][D_SIZE] = dlsize;
	
	/* Standard fstypes */
	bsdlabel[A][D_FSTYPE] = T_42BSD;
	bsdlabel[B][D_FSTYPE] = T_SWAP;
	/* Conventionally, C is whole disk and D in the non NetBSD bit */
	bsdlabel[D][D_FSTYPE] = T_UNUSED;
	bsdlabel[D][D_OFFSET] = 0;
	bsdlabel[D][D_SIZE]   = ptstart;
/*	if (ptstart > 0)
		bsdlabel[D][D_FSTYPE] = T_FILECORE;*/
	bsdlabel[E][D_FSTYPE] = T_UNUSED;	/* fill out below */
	bsdlabel[F][D_FSTYPE] = T_UNUSED;
	bsdlabel[G][D_FSTYPE] = T_UNUSED;
	bsdlabel[H][D_FSTYPE] = T_UNUSED;


	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c/d "unused", e /usr */
	case 2: /* standard X: a root, b swap (big), c/d "unused", e /usr */
		partstart = ptstart;

		/* Root */
		i = NUMSEC(24+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
		    dlcylsize) - partstart;
		bsdlabel[A][D_OFFSET] = partstart;
		bsdlabel[A][D_SIZE] = partsize;
		bsdlabel[A][D_BSIZE] = 8192;
		bsdlabel[A][D_FSIZE] = 1024;
		strcpy(fsmount[A], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC(layoutkind * 2 * (rammb < 32 ? 32 : rammb),
		    MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
		    dlcylsize) - partstart - swapadj;
		bsdlabel[B][D_OFFSET] = partstart;
		bsdlabel[B][D_SIZE] = partsize;
		partstart += partsize;

		/* /usr */
		partsize = fsptsize - (partstart - ptstart);
		bsdlabel[E][D_FSTYPE] = T_42BSD;
		bsdlabel[E][D_OFFSET] = partstart;
		bsdlabel[E][D_SIZE] = partsize;
		bsdlabel[E][D_BSIZE] = 8192;
		bsdlabel[E][D_FSIZE] = 1024;
		strcpy(fsmount[E], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		partstart = ptstart;
		remain = fsptsize;

		/* root */
		i = NUMSEC(24+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
		    dlcylsize) - partstart;
		snprintf(isize, 20, "%d", partsize / sizemult);
		msg_prompt(MSG_askfsroot, isize, isize, 20,
		    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize), sizemult, dlcylsize);
		bsdlabel[A][D_OFFSET] = partstart;
		bsdlabel[A][D_SIZE] = partsize;
		bsdlabel[A][D_BSIZE] = 8192;
		bsdlabel[A][D_FSIZE] = 1024;
		strcpy(fsmount[A], "/");
		partstart += partsize;
		remain -= partsize;
	
		/* swap */
		i = NUMSEC(4 * (rammb < 32 ? 32 : rammb),
		    MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
		    dlcylsize) - partstart - swapadj;
		snprintf(isize, 20, "%d", partsize/sizemult);
		msg_prompt_add(MSG_askfsswap, isize, isize, 20,
		    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize) - swapadj;
		bsdlabel[B][D_OFFSET] = partstart;
		bsdlabel[B][D_SIZE] = partsize;
		partstart += partsize;
		remain -= partsize;
		
		/* Others E, F, G, H */
		part = E;
		if (remain > 0)
			msg_display(MSG_otherparts);
		while (remain > 0 && part <= H) {
			partsize = remain;
			snprintf (isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfspart, isize, isize, 20,
			    diskdev, partname[part], remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
			if (partsize > 0) {
				if (remain - partsize < sizemult)
					partsize = remain;
				bsdlabel[part][D_FSTYPE] = T_42BSD;
				bsdlabel[part][D_OFFSET] = partstart;
				bsdlabel[part][D_SIZE] = partsize;
				bsdlabel[part][D_BSIZE] = 8192;
				bsdlabel[part][D_FSIZE] = 1024;
				if (part == E)
					strcpy(fsmount[E], "/usr");
				msg_prompt_add(MSG_mountpoint, fsmount[part],
				    fsmount[part], 20);
				partstart += partsize;
				remain -= partsize;
			}
			part++;
		}

		break;
	}

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
	if (edit_and_check_label(bsdlabel, maxpart, RAW_PART, RAW_PART) == 0) {
		msg_display(MSG_abort);
		return 0;
	}

	/* Disk name */
	msg_prompt(MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, 8);	/* save 8-partition label */

	/* Everything looks OK. */
	return (1);
}


/* Upgrade support */
int
md_update(void)
{
	endwin();
	md_copy_filesystem();
	md_post_newfs();
	puts(CL);
	wrefresh(stdscr);
	return 1;
}

void
md_cleanup_install(void)
{
#ifndef DEBUG
	char realfrom[STRSIZE];
	char realto[STRSIZE];

	strncpy(realfrom, target_expand("/etc/rc.conf"), STRSIZE);
	strncpy(realto, target_expand("/etc/rc.conf.install"), STRSIZE);

	run_prog(1, 0,
	    "sed 's/rc_configured=NO/rc_configured=YES/' < %s > %s",
	    realfrom, realto);
	run_prog(1, 0, "mv -f %s %s", realto, realfrom);
	run_prog(0, 0, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, 0, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, 0, "rm -f %s", target_expand("/.profile"));
#endif
}
