/*	$NetBSD: disks.c,v 1.1.1.1 1997/09/26 23:02:54 phil Exp $	*/

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

/* disks.c -- routines to deal with finding disks and labeling disks. */


#include <stdio.h>
#include <stdlib.h>
#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

struct lookfor msgbuf[] = {
	{"real mem", "real mem = %d", "a $0", (void *)&ramsize, NULL},
	{"wd0:", "%i, %d cyl, %d head, %d sec, %d",
	 	"a $0 $1 $2 $3", wd0, NULL},
	{"wd1:", "%i, %d cyl, %d head, %d sec, %d", 
	 	"a $0 $1 $2 $3", wd1, NULL},
	{"sd0:", "%i, %d cyl, %d head, %d sec, %d b%i x %d", 
	 	"a $0 $1 $2 $3 $4", sd0, NULL},
	{"sd1:", "%i, %d cyl, %d head, %d sec, %d b%i x %d", 
	 	"a $0 $1 $2 $3 $4", sd1, NULL}
};
int nummsgbuf = sizeof(msgbuf) / sizeof(struct lookfor);

int find_disks_and_mem_size(void)
{
	char *textbuf;
	int  textsize;
	char *tp;
	char defname[80];

	/* initialize */
	disknames[0] = 0;
	numdisks = 0;

	/* Probe for hardware ... */
	textsize = collect (T_FILE, &textbuf, "/kern/msgbuf");
	if (textsize < 0) {
		endwin();
		fprintf (stderr, "Could not open /kern/msgbuf\n");
		exit(1);
	}

	walk (textbuf, textsize, msgbuf, nummsgbuf);
	free(textbuf);

	if (ramsize == 0) {
		/* Lost information in boot! */
		msg_display (MSG_lostinfo);
		process_menu (MENU_noyes);
		if (!yesno)
			return -1;
	}

	/* Find out how many Megs ... round up. */
	rammb = (ramsize + MEG - 1) / MEG;

	/* Ask for which disk */
	if (wd0[0] > 0) {
		numdisks++;
		strcat (disknames, "wd0 ");
	}
	if (wd1[0] > 0) {
		numdisks++;
		strcat (disknames, "wd1 ");
	}
	if (sd0[0] > 0) {
		numdisks++;
		strcat (disknames, "sd0 ");
	}
	if (sd1[0] > 0) {
		numdisks++;
		strcat (disknames, "sd1 ");
	}

	if (numdisks == 0) {
		/* No disks found! */
		msg_display (MSG_nodisk);
		process_menu (MENU_ok);
		endwin();
		return -1;
	} else if (numdisks == 1) {
		/* One disk found! */
		/* Remove that space we added. */
		disknames[strlen(disknames)-1] = 0;
		msg_display (MSG_onedisk, disknames, doingwhat);
		process_menu (MENU_ok);
		strcpy (diskdev, disknames);
	} else {
		/* Multiple disks found! */
		strcpy (defname, disknames);
		tp = defname;
		strsep(&tp, " ");
		msg_prompt (MSG_askdisk, defname,  diskdev, 10, disknames);
		tp = diskdev;
		strsep(&tp, " ");
		diskdev[strlen(diskdev)+1] = 0;
		diskdev[strlen(diskdev)] = ' ';
		while (!(*diskdev == 'w' || *diskdev == 's') ||
		       strstr(disknames, diskdev) == NULL) {
			msg_prompt (MSG_badname, defname,  diskdev, 10,
				    disknames);
			tp = diskdev;
			strsep(&tp, " ");
			diskdev[strlen(diskdev)+1] = 0;
			diskdev[strlen(diskdev)] = ' ';
		}

		/* Remove that space we added. */
		diskdev[strlen(diskdev)-1] = 0;
	}
	
	/* Set diskgeom. */
	if (strcmp(diskdev, "wd0") == 0)
		diskgeom = wd0;
	else if (strcmp(diskdev, "wd1") == 0)
		diskgeom = wd1;
	else if (strcmp(diskdev, "sd0") == 0)
		diskgeom = sd0;
	else if (strcmp(diskdev, "sd1") == 0)
		diskgeom = sd1;

	sectorsize = diskgeom[3];

	return numdisks;
}


void disp_cur_fspart (int disp, int showall)
{
	int i;
	int start, stop;

	if (disp < 0) {
		start = 0;
		stop = 8;
	} else {
		start = disp;
		stop = disp+1;
	}

	msg_display_add (MSG_fspart_head);
	for (i=start; i<stop; i++) {
		if (showall || bsdlabel[i][D_SIZE] > 0) {
			msg_printf_add (" %c:%10d%10d %6s",
					'a'+i,
					bsdlabel[i][D_SIZE]/sizemult ,
					bsdlabel[i][D_OFFSET]/sizemult,
					fstype[bsdlabel[i][D_FSTYPE]]);
			if (bsdlabel[i][D_FSTYPE] == T_42BSD)
				msg_printf_add ("%6d%6d %s",
						bsdlabel[i][D_BSIZE],
						bsdlabel[i][D_FSIZE],
						fsmount[i]);
			else if (bsdlabel[i][D_FSTYPE] == T_MSDOS)
				msg_printf_add ("%12s %s", "", fsmount[i]);
			msg_printf_add("\n");
		}
	}
	msg_printf_add("\n");
}


/* choose a fake geometry. */
void scsi_fake (void)
{
	long fact[20];
	int  numf;

	int geom[5][4] = {{0}};
	int i, j;
	int sects = diskgeom[4];
	int head, sec;

	int stop = diskgeom[0]*diskgeom[1]*diskgeom[2]; 

	i=0;
	while (i < 4 && sects > stop) {
	       factor (sects, fact, 20, &numf);
	       if (numf >= 3) {
		      head =  fact[0];
		      j = 1;
		      while (j < numf-2 && head*fact[j] < 50)
			      head *= fact[j++];
		      sec = fact[j++];
		      while (j < numf-1 && sec*fact[j] < 500)
			      sec *= fact[j++];
		      if (head >= 5  &&
			  sec >= 50) {
			      geom[i][0] = sects / (head*sec);
			      geom[i][1] = head;
			      geom[i][2] = sec;
			      geom[i][3] = head * sec * geom[i][0];
			      i++;
		      }
	       }
	       sects--;
	}
	while (i < 5) {
		geom[i][0] = diskgeom[0];
		geom[i][1] = diskgeom[1];
		geom[i][2] = diskgeom[2];
		geom[i][3] = diskgeom[0]*diskgeom[1]*diskgeom[2];
		i++;
	}
	
	msg_display (MSG_scsi_fake, diskgeom[4],
		     geom[0][0], geom[0][1], geom[0][2], geom[0][3],
		     geom[1][0], geom[1][1], geom[1][2], geom[1][3],
		     geom[2][0], geom[2][1], geom[2][2], geom[2][3],
		     geom[3][0], geom[3][1], geom[3][2], geom[3][3],
		     geom[4][0], geom[4][1], geom[4][2], geom[4][3]);
	process_menu (MENU_scsi_fake);
	if (fake_sel >= 0) {
		dlcyl  = diskgeom[0] = geom[fake_sel][0];
		dlhead = diskgeom[1] = geom[fake_sel][1];
		dlsec  = diskgeom[2] = geom[fake_sel][2];
		dlsize = diskgeom[4] = geom[fake_sel][3];
	}
}


void write_disklabel (void)
{
	/* disklabel the disk */
	printf ("%s", msg_string (MSG_dodisklabel));
	run_prog ("/sbin/disklabel -w -r %s %s", diskdev, bsddiskname);
}

void make_filesystems (void)
{
	int i;

	/* Making new file systems and mounting them*/
	printf ("%s", msg_string (MSG_donewfs));
	for (i=0; i<8; i++)
		if (bsdlabel[i][D_FSTYPE] == T_42BSD) {
			run_prog ("/sbin/newfs /dev/r%s%c", diskdev, 'a'+i);
			if (*fsmount[i]) { 
				if (i > 0) {
					run_prog ("/bin/mkdir /mnt%s",
						  fsmount[i]);
					run_prog ("/sbin/mount -v /dev/%s%c"
						  " /mnt%s",
						  diskdev, 'a'+i,
						  fsmount[i]);
				} else
					run_prog ("/sbin/mount -v /dev/%s%c"
						  " /mnt", diskdev, 'a'+i);
			}
		}

}

void make_fstab (void)
{
	FILE *f;
	int i;

	/* Create the fstab. */
	f = fopen ("/mnt/etc/fstab", "w");
	if (f == NULL) {
#ifndef DEBUG
		(void)fprintf (stderr, "There is a big problem!  "
			       "Can not create /mnt/etc/fstab\n");
		exit(1);
#else
		f = stdout;
#endif		
	}
	(void)fprintf (f, "/dev/%sa / ffs rw 1 1\n", diskdev);
	(void)fprintf (f, "/dev/%sb none swap sw 0 0\n", diskdev);
	for (i=4; i<8; i++)
		if (bsdlabel[i][D_FSTYPE] == T_42BSD)
			(void)fprintf (f, "/dev/%s%c %s ffs rw 1 2\n",
				       diskdev, 'a'+i, fsmount[i]);
		else if (bsdlabel[i][D_FSTYPE] == T_MSDOS )
			(void)fprintf (f, "/dev/%s%c %s msdos rw 0 0\n",
				       diskdev, 'a'+i, fsmount[i]);
#ifndef DEBUG
	fclose(f);
#endif
}

void
fsck_disks (void)
{
}
