/*	$NetBSD: disks.c,v 1.4.2.1 1997/10/27 19:36:16 thorpej Exp $ */

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
#include <unistd.h>
#include <fcntl.h>

#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <sys/disklabel.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"

/* Local prototypes */
static void get_disks (void);
static void foundffs (struct data *list, int num);

static void get_disks(void)
{
	char *names[] = {"wd", "sd", "rd", NULL};
	char **xd = names;
	char d_name[SSTRSIZE];
	struct disklabel l;
	int i;

	while (*xd != NULL) {
		for (i=0; i<5; i++) {
			sprintf (d_name, "%s%d", *xd, i);
			if (get_geom (d_name, &l) && numdisks < MAX_DISKS) {
				strncpy (disks[numdisks].name,
					 d_name, SSTRSIZE);
				strncat (disknames, d_name,
					 SSTRSIZE-1-strlen(disknames));
				strncat (disknames, " ",
					 SSTRSIZE-1-strlen(disknames));
				disks[numdisks].geom[0] = l.d_ncylinders;
				disks[numdisks].geom[1] = l.d_ntracks;
				disks[numdisks].geom[2] = l.d_nsectors;
				disks[numdisks].geom[3] = l.d_secsize;
				disks[numdisks].geom[4] = l.d_secperunit;
				numdisks++;
			}
		}
		xd++;
	}
}
			

int find_disks (void)
{
	char *tp;
	char defname[STRSIZE];
	int  i;

	/* initialize */
	disknames[0] = 0;
	numdisks = 0;

	/* Find disks. */
	get_disks();

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
	
	/* Set disk. */
	for (i=0; i<numdisks; i++)
		if (strcmp(diskdev, disks[i].name) == 0)
			disk = &disks[i];

	sectorsize = disk->geom[3];
	if (disk->geom[4] == 0)
		disk->geom[4] = disk->geom[0] * disk->geom[1] * disk->geom[2];

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
	int sects = disk->geom[4];
	int head, sec;

	int stop = disk->geom[0]*disk->geom[1]*disk->geom[2];

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
		geom[i][0] = disk->geom[0];
		geom[i][1] = disk->geom[1];
		geom[i][2] = disk->geom[2];
		geom[i][3] = stop;
		i++;
	}
	
	msg_display (MSG_scsi_fake, disk->geom[4],
		     geom[0][0], geom[0][1], geom[0][2], geom[0][3],
		     geom[1][0], geom[1][1], geom[1][2], geom[1][3],
		     geom[2][0], geom[2][1], geom[2][2], geom[2][3],
		     geom[3][0], geom[3][1], geom[3][2], geom[3][3],
		     geom[4][0], geom[4][1], geom[4][2], geom[4][3]);
	process_menu (MENU_scsi_fake);
	if (fake_sel >= 0) {
		dlcyl  = disk->geom[0] = geom[fake_sel][0];
		dlhead = disk->geom[1] = geom[fake_sel][1];
		dlsec  = disk->geom[2] = geom[fake_sel][2];
		dlsize = disk->geom[4] = geom[fake_sel][3];
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
		(void)fprintf (stderr, msg_string (MSG_createfstab));
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
	(void)fprintf (f, "/kern /kern kernfs rw\n");
#ifndef DEBUG
	fclose(f);
#endif
}

/* Get information on the file systems mounted from the root filesystem.
 * Offer to convert them into 4.4BSD inodes if they are not 4.4BSD
 * inodes.  Fsck them.  Mount them.
 */


static struct lookfor fstabbuf[] = {
	{"/dev/", "/dev/%s %s ffs", "c", NULL, foundffs},
	{"/dev/", "/dev/%s %s ufs", "c", NULL, foundffs},
};
static int numfstabbuf = sizeof(fstabbuf) / sizeof(struct lookfor);

#define MAXDEVS 40

static char dev[MAXDEVS][SSTRSIZE];
static char mnt[MAXDEVS][STRSIZE];
static int  devcnt = 0;

static void
foundffs (struct data *list, int num)
{
	if (strcmp(list[1].u.s_val, "/") != 0) {
		strncpy(dev[devcnt], list[0].u.s_val, SSTRSIZE);
		strncpy(mnt[devcnt], list[1].u.s_val, STRSIZE);
		devcnt++;
	}
}

static int
inode_kind (char *dev)
{
	union { struct fs fs; char pad[SBSIZE];} fs;
	int fd;
	int ret;

	fd = open (dev, O_RDONLY, 0);
	if (fd < 0)
		return fd;
        if (lseek(fd, (off_t)SBOFF, SEEK_SET) == (off_t)-1)
                return -1;
        if ((ret = read(fd, &fs, SBSIZE)) != SBSIZE) {
		close (fd);
                return -2;
	}
	close (fd);
	if (fs.fs.fs_magic != FS_MAGIC)
		return -3;
	if (fs.fs.fs_inodefmt < FS_44INODEFMT)
		return 0;
	return 1;
}

static int
do_fsck(char *disk, char *part)
{
	char raw[SSTRSIZE];
	int inodetype;
	char * upgr = "";

	snprintf (raw, SSTRSIZE, "/dev/r%s%s", disk, part);
	inodetype = inode_kind (raw);

	if (inodetype < 0) {
		/* error */
		return inodetype;
	}
	else if (inodetype == 0) {
		/* Ask to upgrade */
		msg_display (MSG_upgrinode, raw);
		process_menu (MENU_yesno);
		if (yesno)
			upgr = "-c ";
	}

	endwin();
	if (run_prog ("/sbin/fsck_ffs -f %s%s", upgr, raw)) {
		wrefresh(stdscr);
		return 0;
	}
	wrefresh(stdscr);
	return 1;
}

int
fsck_disks (void)
{	char *fstab;
	int   fstabsize;
	int   i;
	int   res;

	/* First the root device. */
	if ((res = do_fsck (diskdev, "a")) <= 0) {
		msg_display (MSG_badfs, diskdev, "a", res);
		process_menu (MENU_ok);
		return 0;
	}

	if (run_prog ("/sbin/mount /dev/%sa /mnt", diskdev)) {
		msg_display (MSG_badmount, diskdev, "a");
		process_menu (MENU_ok);
		return 0;
	}

	/* Get the fstab. */
	fs_num = 0;
	fstabsize = collect (T_FILE, &fstab, "/mnt/etc/fstab");
	if (fstabsize < 0) {
		/* error ! */
		return 0;
	}
	walk (fstab, fstabsize, fstabbuf, numfstabbuf);
	free(fstab);

	for (i=0; i < devcnt; i++) {
		if (!do_fsck (dev[i], "")) {
			msg_display (MSG_badfs, dev[i], "");
			process_menu (MENU_ok);
			return 0;
		}
		if (run_prog ("/sbin/mount /dev/%s /mnt/%s", dev[i], mnt[i])) {
			msg_display (MSG_badmount, dev[i], "");
			process_menu (MENU_ok);
			return 0;
		}
	}
	
	return 1;
}
