/*	$NetBSD: md.c,v 1.10 1999/01/21 08:02:19 garbled Exp $	*/

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

/* md.c -- pc532 machine specific routines */

#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
/* Maximum */
#define MAX(i,j) ((i)>(j)?(i):(j))
   

int	md_get_info (void)
{	struct disklabel disklabel;
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

	disktype = "SCSI";
	if (disk->geom[0]*disk->geom[1]*disk->geom[2] != disk->geom[4])
		if (disk->geom[0] != dlcyl || disk->geom[1] != dlhead
		    || disk->geom[2] != dlsec)
			process_menu (MENU_scsigeom1);
		else
			process_menu (MENU_scsigeom2);

	dlsize = dlcyl*dlhead*dlsec;
	fsdsize = dlsize;
	fsdmb = fsdsize / MEG;

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = (80 + 4*rammb) * (MEG / sectorsize);

	return 1;
}

void	md_pre_disklabel (void)
{
}

void	md_post_disklabel (void)
{
	/* boot blocks ... */
	printf (msg_string(MSG_dobootblks), diskdev);
	run_prog (0, 1, "echo y | bim -c init -c 'add /usr/mdec/boot boot' -c 'default 0' -c 'exit' /dev/%sc",
		diskdev);
}

void	md_post_newfs (void)
{
}

void	md_copy_filesystem (void)
{
}

int md_make_bsd_partitions (void)
{
	int i;
	int part, partsize, partstart, remain;
	char isize[SSTRSIZE];
	int maxpart = getmaxpartitions();

	/* Ask for layout type -- standard or special */
	msg_display (MSG_layout,
			(1.0*fsptsize*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu (MENU_layout);

	if (layoutkind == 3) {
		ask_sizemult();
	} else {
		multname = msg_string(MSG_megname);
	}
		sizemult = MEG / sectorsize;


	/* Build standard partitions */
	emptylabel(bsdlabel);

	/* Partitions C and D are predefined. */
	bsdlabel[C][D_FSTYPE] = T_UNUSED;
	bsdlabel[C][D_OFFSET] = 0;
	bsdlabel[C][D_SIZE] = dlsize;

	/* Standard fstypes */
	bsdlabel[A][D_FSTYPE] = T_42BSD;
	bsdlabel[B][D_FSTYPE] = T_SWAP;
	bsdlabel[D][D_FSTYPE] = T_UNUSED;
	bsdlabel[D][D_FSTYPE] = T_BOOT;
	bsdlabel[D][D_OFFSET] = dlsize - BOOT_SIZE;
	bsdlabel[D][D_SIZE]   = BOOT_SIZE;
	bsdlabel[E][D_FSTYPE] = T_UNUSED;
	bsdlabel[F][D_FSTYPE] = T_UNUSED;
	bsdlabel[G][D_FSTYPE] = T_UNUSED;
	bsdlabel[H][D_FSTYPE] = T_UNUSED;
	fsdsize -= BOOT_SIZE;
	partstart = 0;

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c/d "unused", e /usr */
	case 2: /* standard X: a root, b swap (big), c/d "unused", e /usr */
		/* Root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		bsdlabel[A][D_OFFSET] = partstart;
		bsdlabel[A][D_SIZE] = partsize;
		bsdlabel[A][D_BSIZE] = 8192;
		bsdlabel[A][D_FSIZE] = 1024;
		strcpy (fsmount[A], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC(layoutkind * 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		bsdlabel[B][D_OFFSET] = partstart;
		bsdlabel[B][D_SIZE] = partsize;
		partstart += partsize;

		/* /usr */
		partsize = fsdsize - partstart;
		bsdlabel[E][D_FSTYPE] = T_42BSD;
		bsdlabel[E][D_OFFSET] = partstart;
		bsdlabel[E][D_SIZE] = partsize;
		bsdlabel[E][D_BSIZE] = 8192;
		bsdlabel[E][D_FSIZE] = 1024;
		strcpy (fsmount[E], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		remain = fsdsize;

		/* root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		snprintf (isize, SSTRSIZE, "%d", partsize/sizemult);
		msg_prompt (MSG_askfsroot, isize, isize, SSTRSIZE,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[A][D_OFFSET] = partstart;
		bsdlabel[A][D_SIZE] = partsize;
		bsdlabel[A][D_BSIZE] = 8192;
		bsdlabel[A][D_FSIZE] = 1024;
		strcpy (fsmount[A], "/");
		partstart += partsize;
		
		/* swap */
		remain = fsdsize - partstart;
		i = NUMSEC( 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		snprintf (isize, SSTRSIZE, "%d", partsize/sizemult);
		msg_prompt_add (MSG_askfsswap, isize, isize, SSTRSIZE,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[B][D_OFFSET] = partstart;
		bsdlabel[B][D_SIZE] = partsize;
		partstart += partsize;
		
		/* /usr */
		remain = fsdsize - partstart;
		partsize = fsdsize - partstart;
		snprintf (isize, SSTRSIZE, "%d", partsize/sizemult);
		msg_prompt_add (MSG_askfsusr, isize, isize, SSTRSIZE,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		if (remain - partsize < sizemult)
			partsize = remain;
		bsdlabel[E][D_FSTYPE] = T_42BSD;
		bsdlabel[E][D_OFFSET] = partstart;
		bsdlabel[E][D_SIZE] = partsize;
		bsdlabel[E][D_BSIZE] = 8192;
		bsdlabel[E][D_FSIZE] = 1024;
		strcpy (fsmount[E], "/usr");
		partstart += partsize;

		/* Others ... */
		remain = fsdsize - partstart;
		part = D;
		if (remain > 0)
			msg_display (MSG_otherparts);
		while (remain > 0 && part <= H) {
			if (bsdlabel[part][D_FSTYPE] == T_UNUSED) {
				partsize = fsdsize - partstart;
				snprintf (isize, SSTRSIZE, "%d",
					  partsize/sizemult);
				msg_prompt_add (MSG_askfspart, isize, isize,
						SSTRSIZE, diskdev,
						partname[part],
						remain/sizemult, multname);
				partsize = NUMSEC(atoi(isize), sizemult,
						  dlcylsize);
				if (remain - partsize < sizemult)
					partsize = remain;
				bsdlabel[part][D_FSTYPE] = T_42BSD;
				bsdlabel[part][D_OFFSET] = partstart;
				bsdlabel[part][D_SIZE] = partsize;
				bsdlabel[part][D_BSIZE] = 8192;
				bsdlabel[part][D_FSIZE] = 1024;
				msg_prompt_add (MSG_mountpoint, NULL,
						fsmount[part], 20);
				partstart += partsize;
				remain = fsdsize - partstart;
			}
			part++;
		}
		

		/* Verify Partitions. */
		process_menu(MENU_fspartok);
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
	(void) savenewlabel(bsdlabel, 8);	/* 8 partitions in label */

	/* Everything looks OK. */
	return (1);
}

/* update support */
int 
md_update(void)
{
	return 1;
}

void
md_cleanup_install(void)
{
}
