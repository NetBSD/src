/*	$NetBSD: md.c,v 1.4 1997/10/17 22:17:56 phil Exp $ */

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
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* prototypes */

int md_get_info()
{
	int i, j;
	get_fdisk_info ();

	/* Ask about disk type ... */
	if (strncmp(disk->name, "wd", 2) == 0) {
		process_menu (MENU_wdtype);
		disktype = "ST506";
	} else {
		disktype = "SCSI";
		if (disk->geom[0]*disk->geom[1]*disk->geom[2] != disk->geom[4])
			if (disk->geom[0] != dlcyl || disk->geom[1] != dlhead
			    || disk->geom[2] != dlsec)
				process_menu (MENU_scsigeom1);
			else
				process_menu (MENU_scsigeom2);
	}
		
	/* Check against disk geometry. */
	if (disk->geom[0] != dlcyl || disk->geom[1] != dlhead
	    || disk->geom[2] != dlsec)
			process_menu (MENU_dlgeom);

	/* Check fdisk information */
	if (bsec <= 0 || bcyl <= 0 || bsec <= 0 ||  bcyl > 1024 || bsec > 63) 
		process_menu (MENU_biosgeom);

	/* Compute the full sizes ... */
	dlcylsize = dlhead*dlsec;
	dlsize = dlcyl*dlcylsize;
	bcylsize = bhead*bsec;
	bsize = bcyl*bcylsize;

	/* Ask full/part */
	msg_display (MSG_fullpart, diskdev);
	process_menu (MENU_fullpart);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		/* Ask if we really want to blow away non-BSD stuff */
		if (!(part[0][ID] == 0 || part[0][ID] == 165 ||
		      part[1][ID] == 0 || part[1][ID] == 165 ||
		      part[2][ID] == 0 || part[2][ID] == 165 ||
		      part[3][ID] == 0 || part[3][ID] == 165)) {
			msg_display (MSG_ovrwrite);
			process_menu (MENU_noyes);
			if (!yesno) {
				endwin();
				return 0;
			}
		}
		/* Set the partition information for full disk usage. */
		part[0][ID] = part[0][SIZE] = 0;
		part[1][ID] = part[1][SIZE] = 0;
		part[2][ID] = part[2][SIZE] = 0;
		part[3][ID] = 165;
		part[3][SIZE] = bsize - bsec;
		part[3][START] = bsec;
		part[3][FLAG] = 0x80;

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
			fsptsize = part[bsdpart][SIZE];
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
		run_prog ("/sbin/fdisk -u -f -%d -s %d/%d/%d /dev/r%sd",
			  i, part[i][ID], part[i][START], part[i][SIZE],
			  diskdev);

	if (activepart >= 0)
		run_prog ("/sbin/fdisk -a -%d -f", activepart);

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
	run_prog ("tar --one-file-system -cf - . |"
		  "(cd /mnt ; tar --unlink -xpf - )");
	run_prog ("/bin/cp /tmp/.hdprofile /mnt/.profile");
}



void md_make_bsd_partitions (void)
{
	FILE *f;
	int i, part;
	int remain;
	char isize[20];

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
	bsdlabel[E][D_FSTYPE] = T_42BSD;
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
		i = NUMSEC(layoutkind * 2 * (rammb < 32 ? 32 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
			   dlcylsize) - partstart - swapadj;
		bsdlabel[B][D_OFFSET] = partstart;
		bsdlabel[B][D_SIZE] = partsize;
		partstart += partsize;

		/* /usr */
		partsize = fsdsize - partstart;
		bsdlabel[E][D_OFFSET] = partstart;
		bsdlabel[E][D_SIZE] = partsize;
		bsdlabel[E][D_BSIZE] = 8192;
		bsdlabel[E][D_FSIZE] = 1024;
		strcpy (fsmount[E], "/usr");


		/* Verify Partitions. */
		process_menu (MENU_fspartok);
		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		/* root */
		partstart = ptstart;
		remain = fsdsize - partstart;
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
		
		/* swap */
		remain = fsdsize - partstart;
		i = NUMSEC(layoutkind * 2 * (rammb < 32 ? 32 : rammb),
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
		
		/* /usr */
		remain = fsdsize - partstart;
		partsize = fsdsize - partstart;
		snprintf (isize, 20, "%d", partsize/sizemult);
		msg_prompt_add (MSG_askfsusr, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		if (remain - partsize < sizemult)
			partsize = remain;
		bsdlabel[E][D_OFFSET] = partstart;
		bsdlabel[E][D_SIZE] = partsize;
		bsdlabel[E][D_BSIZE] = 8192;
		bsdlabel[E][D_FSIZE] = 1024;
		strcpy (fsmount[E], "/usr");
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
			bsdlabel[part][D_FSTYPE] = T_42BSD;
			bsdlabel[part][D_OFFSET] = partstart;
			bsdlabel[part][D_SIZE] = partsize;
			bsdlabel[part][D_BSIZE] = 8192;
			bsdlabel[part][D_FSIZE] = 1024;
			msg_prompt_add (MSG_mountpoint, NULL,
					fsmount[part], 20);
			partstart += partsize;
			remain = fsdsize - partstart;
			part++;
		}
		

		/* Verify Partitions. */
		process_menu(MENU_fspartok);
		break;
	}

	/* Disk name */
	msg_prompt (MSG_packname, "mydisk", bsddiskname, 80);

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

}


/* Upgrade support */
int
md_update (void)
{
	endwin();
	md_copy_filesystem ();
	md_post_newfs();
	wrefresh(stdscr);
	return 1;
}
