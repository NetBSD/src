/*	$NetBSD: md.c,v 1.11 1999/06/20 06:08:21 cgd Exp $	*/

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

/* md.c -- macppc machine specific routines */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * Symbolic names for disk partitions.
 */
#define	PART_ROOT	A
#define	PART_RAW	C
#define	PART_USR	G

int
md_get_info()
{
	struct disklabel disklabel;
	int fd;
	char devname[100];

	snprintf (devname, 100, "/dev/r%sc", diskdev);

	fd = open (devname, O_RDONLY, 0);
	if (fd < 0) {
		endwin();
		fprintf (stderr, "Can't open %s\n", devname);
		exit(1);
	}
	if (ioctl(fd, DIOCGDINFO, &disklabel) == -1) {
		endwin();
		fprintf (stderr, "Can't read disklabel on %s.\n", devname);
		close(fd);
		exit(1);
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

	return 1;
}

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel()
{
	return 0;
}

/*
 * hook called after writing disklabel to new target disk.
 */
int
md_post_disklabel(void)
{
	return 0;
}

/*
 * MD hook called after upgrade() or install() has finished setting
 * up the target disk but immediately before the user is given the
 * ``disks are now set up'' message that, if power fails, they can
 * continue installation by booting the target disk and doing an
 * `upgrade'.
 *
 * On the macppc, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs()
{
	const char *bootfile = target_expand("/boot");	/*XXX*/

	printf (msg_string(MSG_dobootblks), diskdev);
	cp_to_target("/usr/mdec/ofwboot", "/boot");
	sync();
	run_prog(0, 1, NULL, "/usr/mdec/installboot %s %s /dev/r%sa",
	    bootfile,  "/usr/mdec/bootxx", diskdev);
	return 0;
}

/*
 * some ports use this to copy the MD filesystem, we do not.
 */
int
md_copy_filesystem()
{
	return 0;
}

int
md_make_bsd_partitions()
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

	/* Ask for layout type -- standard or special */
	msg_display (MSG_layout,
			(1.0*fsptsize*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu (MENU_layout);

	if (layoutkind == 3) {
		ask_sizemult();
	} else {
		sizemult = MEG / sectorsize;
		multname = msg_string(MSG_megname);
	}


	/* Build standard partitions */
	emptylabel(bsdlabel);

	/* Partitions C is predefined (whole  disk). */
	bsdlabel[C].pi_fstype = FS_UNUSED;
	bsdlabel[C].pi_offset = 0;
	bsdlabel[C].pi_size = dlsize;
	
	/* Standard fstypes */
	bsdlabel[A].pi_fstype = FS_BSDFFS;
	bsdlabel[B].pi_fstype = FS_SWAP;
	/* Conventionally, C is whole disk. */
	bsdlabel[D].pi_fstype = FS_UNUSED;	/* fill out below */
	bsdlabel[E].pi_fstype = FS_UNUSED;
	bsdlabel[F].pi_fstype = FS_UNUSED;
	bsdlabel[G].pi_fstype = FS_UNUSED;
	bsdlabel[H].pi_fstype = FS_UNUSED;
	part = D;


	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c "unused", d /usr */
	case 2: /* standard X: a root, b swap (big), c "unused", d /usr */
		partstart = ptstart;

		/* Root */
		/* By convention, NetBSD/macppc uses a 32Mbyte root */
		partsize= NUMSEC(32, MEG/sectorsize, dlcylsize);
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy (fsmount[A], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC(layoutkind * 2 * (rammb < 32 ? 32 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;

		/* /usr */
		partsize = fsdsize - partstart;
		bsdlabel[PART_USR].pi_fstype = FS_BSDFFS;
		bsdlabel[PART_USR].pi_offset = partstart;
		bsdlabel[PART_USR].pi_size = partsize;
		bsdlabel[PART_USR].pi_bsize = 8192;
		bsdlabel[PART_USR].pi_fsize = 1024;
		strcpy (fsmount[PART_USR], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		/* root */
		partstart = ptstart;
		remain = fsdsize - partstart;
		/* By convention, NetBSD/macppc uses a 32Mbyte root */
		partsize = NUMSEC (32, MEG/sectorsize, dlcylsize);
		snprintf (isize, 20, "%d", partsize/sizemult);
		msg_prompt (MSG_askfsroot, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy (fsmount[A], "/");
		partstart += partsize;
		
		/* swap */
		remain = fsdsize - partstart;
		i = NUMSEC(2 * (rammb < 32 ? 32 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		snprintf (isize, 20, "%d", partsize/sizemult);
		msg_prompt_add (MSG_askfsswap, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;
		
		/* /usr */
		remain = fsdsize - partstart;
		partsize = fsdsize - partstart;
		snprintf (isize, 20, "%d", partsize/sizemult);
		msg_prompt_add (MSG_askfsusr, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		if (remain - partsize < sizemult)
			partsize = remain;
		bsdlabel[PART_USR].pi_fstype = FS_BSDFFS;
		bsdlabel[PART_USR].pi_offset = partstart;
		bsdlabel[PART_USR].pi_size = partsize;
		bsdlabel[PART_USR].pi_bsize = 8192;
		bsdlabel[PART_USR].pi_fsize = 1024;
		strcpy (fsmount[PART_USR], "/usr");
		partstart += partsize;

		/* Others ... */
		remain = fsdsize - partstart;
		part = F;
		if (remain > 0)
			msg_display (MSG_otherparts);
		while (remain > 0 && part <= H) {
			partsize = fsdsize - partstart;
			snprintf (isize, 20, "%d", partsize/sizemult);
			msg_prompt_add (MSG_askfspart, isize, isize, 20,
					diskdev, partname[part],
					remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
			if (remain - partsize < sizemult)
				partsize = remain;
			bsdlabel[part].pi_fstype = FS_BSDFFS;
			bsdlabel[part].pi_offset = partstart;
			bsdlabel[part].pi_size = partsize;
			bsdlabel[part].pi_bsize = 8192;
			bsdlabel[part].pi_fsize = 1024;
			msg_prompt_add (MSG_mountpoint, NULL,
					fsmount[part], 20);
			partstart += partsize;
			remain = fsdsize - partstart;
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
	msg_prompt (MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, 8);	/* 8 partitions in  label */

	/* Everything looks OK. */
	return (1);
}

/* Upgrade support */
int
md_update()
{

	endwin();
	md_copy_filesystem ();
	md_post_newfs();
	puts (CL);
	wrefresh(stdscr);
	return 1;
}

void
md_cleanup_install()
{

}
