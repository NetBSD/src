/*	$NetBSD: md.c,v 1.12 1997/12/19 00:57:18 fvdl Exp $ */

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

/* md.c -- Machine specific code for i386 */

#include <stdio.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

int mbr_present;
int c1024_resp;

int edit_mbr __P((void));

/* prototypes */

int md_get_info()
{

	get_fdisk_info();

	/* Check fdisk information */
	if (part[0][ID] == 0 && part[1][ID] == 0 && part[2][ID] == 0 &&
	    part[3][ID] == 0) {
		mbr_present = 0;
		process_menu(MENU_nobiosgeom);
	} else {
		mbr_present = 1;
		msg_display(MSG_confirmbiosgeom);
		process_menu(MENU_confirmbiosgeom);
	}

	/* Ask about disk type ... */
	if (strncmp(disk->name, "wd", 2) == 0) {
		process_menu(MENU_wdtype);
		disktype = "ST506";
		/* Check against disk geometry. */
		if (disk->geom[0] != dlcyl || disk->geom[1] != dlhead
		    || disk->geom[2] != dlsec)
			process_menu (MENU_dlgeom);
	} else {
		disktype = "SCSI";
		if (disk->geom[0]*disk->geom[1]*disk->geom[2] != disk->geom[4])
			if (disk->geom[0] != dlcyl || disk->geom[1] != dlhead
			    || disk->geom[2] != dlsec)
				process_menu (MENU_scsigeom1);
			else
				process_menu (MENU_scsigeom2);
	}

	/* Compute the full sizes ... */
	dlcylsize = dlhead*dlsec;
	dlsize = dlcyl*dlcylsize;
	bcylsize = bhead*bsec;
	bsize = bcyl*bcylsize;

	msg_display(MSG_diagcyl);
	process_menu(MENU_noyes);
	if (yesno) {
		dlcyl--;
		dlsize -= dlcylsize;
		if (dlsize < bsize)
			bcyl = dlsize / bcylsize;
	}
	return edit_mbr();
}

int edit_mbr()
{
	int i, j;

	/* Ask full/part */
	msg_display (MSG_fullpart, diskdev);
	process_menu (MENU_fullpart);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		/* Ask if we really want to blow away non-BSD stuff */
		if ((part[0][ID] != 0 && part[0][ID] != 165) ||
		    (part[1][ID] != 0 && part[1][ID] != 165) ||
		    (part[2][ID] != 0 && part[2][ID] != 165) ||
		    (part[3][ID] != 0 && part[3][ID] != 165)) {
			msg_display (MSG_ovrwrite);
			process_menu (MENU_noyes);
			if (!yesno) {
				endwin();
				return 0;
			}
		}
		/* Set the partition information for full disk usage. */
		part[0][ID] = part[0][SIZE] = 0;
		part[0][SET] = 1;
		part[1][ID] = part[1][SIZE] = 0;
		part[1][SET] = 1;
		part[2][ID] = part[2][SIZE] = 0;
		part[2][SET] = 1;
		part[3][ID] = 165;
		part[3][SIZE] = bsize - bsec;
		part[3][START] = bsec;
		part[3][FLAG] = 0x80;
		part[3][SET] = 1;

		ptstart = bsec;
		ptsize = bsize - bsec;
		fsdsize = dlsize;
		fsptsize = dlsize - bsec;
		fsdmb = fsdsize / MEG;
		activepart = 3;
	} else {
		int numbsd, overlap;
		/* Ask for sizes, which partitions, ... */
		ask_sizemult();
		bsdpart = -1;
		activepart = -1;
		for (i=0; i<4; i++)
			if (part[i][FLAG] != 0)
				activepart = i;
		do {
			process_menu (MENU_editparttable);
			numbsd = 0;
			bsdpart = -1;
			overlap = 0;
			yesno = 0;
			for (i=0; i<4; i++) {
				if (part[i][ID] == 165) {
					bsdpart = i;
					numbsd++;
				}
				for (j = i+1; j<4; j++)
				       if (partsoverlap(i,j))
					       overlap = 1;
			}
			if (overlap || numbsd != 1) {
				msg_display (MSG_reeditpart);
				process_menu (MENU_yesno);
			}
		} while (yesno && (numbsd != 1 || overlap));

		if (numbsd == 0) {
			msg_display (MSG_nobsdpart);
			process_menu (MENU_ok);
			return 0;
		}
			
		if (numbsd > 1) {
			msg_display (MSG_multbsdpart, bsdpart);
			process_menu (MENU_ok);
		}
			
		ptstart = part[bsdpart][START];
		ptsize = part[bsdpart][SIZE];
		fsdsize = dlsize;
		if (ptstart + ptsize < bsize)
			fsptsize = ptsize;
		else
			fsptsize = dlsize - ptstart;
		fsdmb = fsdsize / MEG;

		/* Ask if a boot selector is wanted. XXXX */
	}

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = (80 + 4*rammb) * (MEG / sectorsize);

	if (usefull) 
	  swapadj = bsec;

	return 1;
}

void md_pre_disklabel()
{
	int i;

	/* Fdisk the disk! */
	printf ("%s", msg_string (MSG_dofdisk));
	if (bstuffset)
		run_prog ("/sbin/fdisk -i -f -b %d/%d/%d /dev/r%sd",
			  bcyl, bhead, bsec, diskdev);
	
	for (i=0; i<4; i++)
		if (part[i][SET])
			run_prog("/sbin/fdisk -u -f -%d -s %d/%d/%d /dev/r%sd",
				 i, part[i][ID], part[i][START],
				 part[i][SIZE],  diskdev);

	if (activepart >= 0)
		run_prog ("/sbin/fdisk -a -%d -f /dev/r%s",
			  activepart, diskdev);

}

void md_post_disklabel (void)
{
	/* Sector forwarding / badblocks ... */
	if (*doessf) {
		printf ("%s", msg_string (MSG_dobad144));
		run_prog ("/usr/sbin/bad144 %s 0", diskdev);
	}
}

void md_post_newfs (void)
{
	/* boot blocks ... */
	printf (msg_string(MSG_dobootblks), diskdev);
	run_prog ("/usr/mdec/installboot -v /usr/mdec/biosboot.sym "
		  "/dev/r%sa", diskdev);
}

void md_copy_filesystem (void)
{
	/* Copy the instbin(s) to the disk */
	printf ("%s", msg_string(MSG_dotar));
	run_prog ("tar --one-file-system -cf - -C / . |"
		  "(cd /mnt ; tar --unlink -xpf - )");
	run_prog ("/bin/cp /tmp/.hdprofile /mnt/.profile");
	run_prog ("/bin/cp /usr/share/misc/termcap /mnt/.termcap");
}



int md_make_bsd_partitions (void)
{
	FILE *f;
	int i;
	int part;
	int maxpart = getmaxpartitions();
	int remain;
	char isize[20];

editlab:
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

	/* Partitions C and D are predefined. */
	bsdlabel[C][D_FSTYPE] = T_UNUSED;
	bsdlabel[C][D_OFFSET] = ptstart;
	bsdlabel[C][D_SIZE] = fsptsize;
	
	bsdlabel[D][D_FSTYPE] = T_UNUSED;
	bsdlabel[D][D_OFFSET] = 0;
	bsdlabel[D][D_SIZE] = fsdsize;

	/* Standard fstypes */
	bsdlabel[A][D_FSTYPE] = T_42BSD;
	bsdlabel[B][D_FSTYPE] = T_SWAP;
	bsdlabel[E][D_FSTYPE] = T_UNUSED;
	bsdlabel[F][D_FSTYPE] = T_UNUSED;
	bsdlabel[G][D_FSTYPE] = T_UNUSED;
	bsdlabel[H][D_FSTYPE] = T_UNUSED;

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c/d "unused", e /usr */
	case 2: /* standard X: a root, b swap (big), c/d "unused", e /usr */
		partstart = ptstart;

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
		strcpy (fsmount[E], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		partstart = ptstart;
		remain = fsptsize;

		/* root */
		i = NUMSEC(20+2*rammb, MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		snprintf (isize, 20, "%d", partsize/sizemult);
		msg_prompt (MSG_askfsroot, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[A][D_OFFSET] = partstart;
		bsdlabel[A][D_SIZE] = partsize;
		bsdlabel[A][D_BSIZE] = 8192;
		bsdlabel[A][D_FSIZE] = 1024;
		strcpy (fsmount[A], "/");
		partstart += partsize;
		remain -= partsize;
		
		/* swap */
		i = NUMSEC( 2 * (rammb < 16 ? 16 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart - swapadj;
		snprintf (isize, 20, "%d", partsize/sizemult);
		msg_prompt_add (MSG_askfsswap, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize) - swapadj;
		bsdlabel[B][D_OFFSET] = partstart;
		bsdlabel[B][D_SIZE] = partsize;
		partstart += partsize;
		remain -= partsize;
		
		/* Others E, F, G, H */
		part = E;
		if (remain > 0)
			msg_display (MSG_otherparts);
		while (remain > 0 && part <= H) {
			partsize = remain;
			snprintf (isize, 20, "%d", partsize/sizemult);
			msg_prompt_add (MSG_askfspart, isize, isize, 20,
					diskdev, partname[part],
					remain/sizemult, multname);
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
					strcpy (fsmount[E], "/usr");
				msg_prompt_add (MSG_mountpoint, fsmount[part],
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

	if ((bsdlabel[A][D_OFFSET] + bsdlabel[A][D_SIZE]) / bcylsize > 1024) {
		process_menu(MENU_cyl1024);
		/* XXX UGH! need arguments to process_menu */
		switch (c1024_resp) {
		case 1:
			edit_mbr();
			/*FALLTHROUGH*/
		case 2:
			goto editlab;
		default:
			break;
		}
	}

	/* Disk name */
	msg_prompt (MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* Create the disktab.preinstall */
	run_prog ("cp /etc/disktab.preinstall /etc/disktab");
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
	(void)fprintf (f, "\t:se#%d:%s\\\n", sectorsize, doessf);
	for (i=0; i<8; i++) {
		(void)fprintf (f, "\t:p%c#%d:o%c#%d:t%c=%s:",
			       'a'+i, bsdlabel[i][D_SIZE],
			       'a'+i, bsdlabel[i][D_OFFSET],
			       'a'+i, fstype[bsdlabel[i][D_FSTYPE]]);
		if (bsdlabel[i][D_FSTYPE] == T_42BSD)
			(void)fprintf (f, "b%c#%d:f%c#%d",
				       'a'+i, bsdlabel[i][D_BSIZE],
				       'a'+i, bsdlabel[i][D_FSIZE]);
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
	puts (CL);
	wrefresh(stdscr);
	return 1;
}
