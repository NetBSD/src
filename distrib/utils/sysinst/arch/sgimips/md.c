/*	$NetBSD: md.c,v 1.5 2003/05/21 10:05:28 dsl Exp $	*/

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

/* md.c -- sgimips machine specific routines */

#include <sys/types.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <stdio.h>
#include <curses.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include <errno.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int
md_get_info(void)
{	struct disklabel disklabel;

	if (get_real_geom(diskdev, &disklabel) == 0) {
		endwin();
		fprintf (stderr, "Can't get disklabel for %s: %s\n", diskdev,
		    strerror(errno));
		exit(1);
	}

	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;

	disktype = "SCSI";
	dlcyl  = disk->dd_cyl;
	dlhead = disk->dd_head;
	dlsec  = disk->dd_sec;

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = STDNEEDMB * (MEG / sectorsize);

	return 1;
}

int
md_pre_disklabel(void)
{
	return 0;
}

int
md_post_disklabel(void)
{
	return run_prog(RUN_DISPLAY, MSG_cmdfail,
	    "%s %s", "/usr/mdec/sgivol -f -w boot /usr/mdec/boot", diskdev);
}

int
md_post_newfs(void)
{
	return 0;
}

int
md_copy_filesystem(void)
{
	return 0;
}

int
md_make_bsd_partitions(void)
{
	int i;
	int part, partsize, partstart, remain;
	char isize[SSTRSIZE];
	int maxpart = getmaxpartitions();
	struct disklabel l;
	int ptend;

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
			(1.0*ptsize*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG,
			(1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu(MENU_layout);

	if (layoutkind == 3) {
		ask_sizemult(dlcylsize);
	} else {
		multname = msg_string(MSG_megname);
	}
		sizemult = MEG / sectorsize;


	/* Build standard partitions */
	emptylabel(bsdlabel);
	ptend = dlsize;

	/* Partitions C and D are predefined. */
	bsdlabel[C].pi_fstype = FS_UNUSED;
	bsdlabel[C].pi_offset = 0;
	bsdlabel[C].pi_size = dlsize;

#define BYTES_PER_CYL (dlcylsize * sectorsize)
#define	BOOT_CYLS ( (int)(MEG / BYTES_PER_CYL) + ( (MEG % BYTES_PER_CYL) ? 1 : 0) )
#define BOOT_SIZE (BOOT_CYLS * dlcylsize)
	bsdlabel[D].pi_fstype = FS_BOOT;	/* sgi volume header */
	bsdlabel[D].pi_offset = 0;
	bsdlabel[D].pi_size   = BOOT_SIZE;

	/* Standard fstypes */
	bsdlabel[A].pi_fstype = FS_BSDFFS;	/* root */
	bsdlabel[A].pi_bsize  = 8192;
	bsdlabel[A].pi_fsize  = 1024;
	bsdlabel[B].pi_fstype = FS_SWAP;	/* swap */
	bsdlabel[E].pi_fstype = FS_UNUSED;
	bsdlabel[F].pi_fstype = FS_UNUSED;
	bsdlabel[G].pi_fstype = FS_UNUSED;
	bsdlabel[H].pi_fstype = FS_UNUSED;
	partstart = BOOT_SIZE;

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c "unused", d "boot", e /usr */
	case 2: /* standard X: a root, b swap (big), c "unused", d "boot" e /usr */
		/* Root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy(fsmount[A], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC(layoutkind * 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;

		/* /usr */
		partsize = ptend - partstart;
		bsdlabel[E].pi_fstype = FS_BSDFFS;
		bsdlabel[E].pi_offset = partstart;
		bsdlabel[E].pi_size = partsize;
		bsdlabel[E].pi_bsize = 8192;
		bsdlabel[E].pi_fsize = 1024;
		strcpy(fsmount[E], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult(dlcylsize);
		remain = ptend;

		/* root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		snprintf(isize, SSTRSIZE, "%d", partsize/sizemult);
		msg_prompt(MSG_askfsroot, isize, isize, SSTRSIZE,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy(fsmount[A], "/");
		partstart += partsize;
		
		/* swap */
		remain = ptend - partstart;
		i = NUMSEC( 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart;
		snprintf(isize, SSTRSIZE, "%d", partsize/sizemult);
		msg_prompt_add(MSG_askfsswap, isize, isize, SSTRSIZE,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;
		
		/* /usr */
		remain = ptend - partstart;
		if (remain > 0) {
			partsize = ptend - partstart;
			snprintf(isize, SSTRSIZE, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfsusr, isize, isize, SSTRSIZE,
				    remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
			if (remain - partsize < sizemult)
				partsize = remain;
			bsdlabel[E].pi_fstype = FS_BSDFFS;
			bsdlabel[E].pi_offset = partstart;
			bsdlabel[E].pi_size = partsize;
			bsdlabel[E].pi_bsize = 8192;
			bsdlabel[E].pi_fsize = 1024;
			strcpy(fsmount[E], "/usr");
			partstart += partsize;
		}

		/* Others ... */
		remain = ptend - partstart;
		part = D;
		if (remain > 0)
			msg_display(MSG_otherparts);
		while (remain > 0 && part <= H) {
			if (bsdlabel[part].pi_fstype == FS_UNUSED) {
				partsize = ptend - partstart;
				snprintf(isize, SSTRSIZE, "%d",
					  partsize/sizemult);
				msg_prompt_add(MSG_askfspart, isize, isize,
						SSTRSIZE, diskdev,
						partition_name(part),
						remain/sizemult, multname);
				partsize = NUMSEC(atoi(isize), sizemult,
						  dlcylsize);
				if (remain - partsize < sizemult)
					partsize = remain;
				bsdlabel[part].pi_fstype = FS_BSDFFS;
				bsdlabel[part].pi_offset = partstart;
				bsdlabel[part].pi_size = partsize;
				bsdlabel[part].pi_bsize = 8192;
				bsdlabel[part].pi_fsize = 1024;
				msg_prompt_add(MSG_mountpoint, NULL,
						fsmount[part], 20);
				partstart += partsize;
				remain = ptend - partstart;
			}
			part++;
		}
		

		/* Verify Partitions. */
		process_menu(MENU_fspartok);
		break;
	case 4: /* use existing parts */
		if (get_real_geom(diskdev, &l) == 0) {
			msg_display(MSG_abort);
			return 0;
		}
#define p l.d_partitions[i]
		for (i = 0; i < getmaxpartitions(); i++) {
			/*
			 * Make sure to not overwrite the raw partition, or
			 * the boot partition.
			 */
			if (i == 2 || i == 3)
				continue;

			bsdlabel[i].pi_size = p.p_size;
			bsdlabel[i].pi_offset = p.p_offset;
			bsdlabel[i].pi_fstype = p.p_fstype;
			bsdlabel[i].pi_bsize  = p.p_fsize * p.p_frag;
			bsdlabel[i].pi_fsize  = p.p_fsize;
		}
#undef p
		msg_display(MSG_postuseexisting);
		getchar();
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
	(void) savenewlabel(bsdlabel, 8);	/* 8 partitions in label */

	/* Everything looks OK. */
	return 1;
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

int
md_pre_update()
{
	return 1;
}

void
md_init()
{
}

void
md_set_sizemultname()
{

	set_sizemultname_meg();
}

void
md_set_no_x()
{
}
