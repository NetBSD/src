/*	$NetBSD: disks.c,v 1.39 2000/12/22 10:12:12 mrg Exp $ */

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

/* disks.c -- routines to deal with finding disks and labeling disks. */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>

#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "txtwalk.h"



/* Local prototypes */
static void get_disks (void);
static void foundffs (struct data *list, int num);
static int do_fsck(const char *diskpart);
static int fsck_root (void);
static int 
    do_ffs_newfs(const char *partname, int part, const char *mountpoint);

static int fsck_with_error_menu(const char *diskpart);
static int target_mount_with_error_menu(const char *opt, char *diskpart,
					const char *mntpt);


static void
get_disks(void)
{
	char **xd = disk_names;
	char d_name[SSTRSIZE];
	struct disklabel l;
	int i;

	while (*xd != NULL) {
		for (i=0; i< MAX_DISKS; i++) {
			snprintf(d_name, SSTRSIZE, "%s%d", *xd, i);
			if (get_geom(d_name, &l) && numdisks < MAX_DISKS) {
				strncpy (disks[numdisks].dd_name,
					 d_name, SSTRSIZE);
				strncat (disknames, d_name,
					 SSTRSIZE-1-strlen(disknames));
				strncat (disknames, " ",
					 SSTRSIZE-1-strlen(disknames));
				disks[numdisks].dd_cyl = l.d_ncylinders;
				disks[numdisks].dd_head = l.d_ntracks;
				disks[numdisks].dd_sec = l.d_nsectors;
				disks[numdisks].dd_secsize = l.d_secsize;
				disks[numdisks].dd_totsec = l.d_secperunit;
				numdisks++;
			}
		}
		xd++;
	}
}
			

int
find_disks(void)
{
	char *tp;
	char defname[STRSIZE];
	int  i;

	/* initialize */
	disknames[0] = 0;
	numdisks = 0;

	/* Find disks. */
	get_disks();

	/* need a redraw here, kernel messages hose everything */
	touchwin(stdscr);
	refresh();

	if (numdisks == 0) {
		/* No disks found! */
		msg_display(MSG_nodisk);
		process_menu(MENU_ok);
		/*endwin();*/
		return -1;
	} else if (numdisks == 1) {
		/* One disk found! */
		/* Remove that space we added. */
		disknames[strlen(disknames)-1] = 0;
		msg_display(MSG_onedisk, disknames, doingwhat);
		process_menu(MENU_ok);
		strcpy(diskdev, disknames);
	} else {
		/* Multiple disks found! */
		strcpy(defname, disknames);
		tp = defname;
		strsep(&tp, " ");
		msg_prompt(MSG_askdisk, defname,  diskdev, 10, disknames);
		tp = diskdev;
		strsep(&tp, " ");
		diskdev[strlen(diskdev)+1] = 0;
		diskdev[strlen(diskdev)] = ' ';
		while (!ISDISKSTART(*diskdev) ||
		       strstr(disknames, diskdev) == NULL) {
			msg_prompt(MSG_badname, defname,  diskdev, 10,
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
	for (i = 0; i < numdisks; i++)
		if (strcmp(diskdev, disks[i].dd_name) == 0)
			disk = &disks[i];

	sectorsize = disk->dd_secsize;
	if (disk->dd_totsec == 0)
		disk->dd_totsec = disk->dd_cyl * disk->dd_head * disk->dd_sec;
	dlcyl = disk->dd_cyl;
	dlhead = disk->dd_head;
	dlsec = disk->dd_sec;
	dlsize = disk->dd_totsec;
	dlcylsize = dlhead * dlsec;

	return numdisks;
}


void
disp_cur_fspart(int disp, int showall)
{
	int i;
	int start, stop;
	int poffset, psize, pend;

	if (disp < 0) {
		start = 0;
		stop = getmaxpartitions();
	} else {
		start = disp;
		stop = disp+1;
	}

	msg_table_add (MSG_fspart_header);
	for (i = start; i < stop; i++) {
		if (showall || bsdlabel[i].pi_size > 0) {
			poffset = bsdlabel[i].pi_offset / sizemult;
			psize = bsdlabel[i].pi_size / sizemult;
			if (psize == 0)
				pend = 0;
			else
				pend = (bsdlabel[i].pi_offset +
				bsdlabel[i].pi_size) / sizemult - 1;
			msg_table_add(MSG_fspart_row_start,
					'a'+i, psize, poffset, pend,
					fstypenames[bsdlabel[i].pi_fstype]);
			if (bsdlabel[i].pi_fstype == FS_BSDFFS)
				msg_table_add(MSG_fspart_row_end_bsd,
						bsdlabel[i].pi_bsize,
						bsdlabel[i].pi_fsize,
						fsmount[i]);
			else if (bsdlabel[i].pi_fstype == FS_MSDOS)
				msg_table_add(MSG_fspart_row_end_msdos,
						fsmount[i]);
			else
				msg_table_add(MSG_fspart_row_end_other);
		}
	}
	msg_display_add(MSG_newline);
}

/*
 * Label a disk using an MD-specific string DISKLABEL_CMD for
 * to invoke  disklabel.
 * if MD code does not define DISKLABEL_CMD, this is a no-op.
 *
 * i386  port uses "/sbin/disklabel -w -r", just like i386
 * miniroot scripts, though this may leave a bogus incore label.
 *
 * Sun ports should use  DISKLABEL_CMD "/sbin/disklabel -w"	
 * to get incore  to ondisk inode translation for the Sun proms.
 */
int
write_disklabel (void)
{

#ifdef DISKLABEL_CMD
	/* disklabel the disk */
	return run_prog(RUN_DISPLAY, MSG_cmdfail,
	    "%s %s %s", DISKLABEL_CMD, diskdev, bsddiskname);
#endif
	return 0;
}

int
make_filesystems(void)
{
	int i;
	char partname[STRSIZE];
	int error;

	/* Making new file systems and mounting them*/
	for (i = 0; i < getmaxpartitions(); i++) {
		/*
		 * newfs and mount. For now, process only BSD filesystems. 
		 * but if this is the  mounted-on root, don't touch it! 
		 */
	  	snprintf(partname, STRSIZE, "%s%c", diskdev, 'a'+i);
		if (bsdlabel[i].pi_fstype == FS_BSDFFS && 
		    !is_active_rootpart(partname)) {
			error = do_ffs_newfs(partname, i, fsmount[i]);
			if (error)
				return error;
		}
	}
	return 0;
}

/* newfs and mount an ffs filesystem. */
static int 
do_ffs_newfs(const char *partname, int partno, const char *mountpoint)
{
	char devname[STRSIZE];
	int error;

	error = run_prog(RUN_DISPLAY, MSG_cmdfail,
	    "/sbin/newfs /dev/r%s", partname);
	if (*mountpoint && error == 0) { 
		snprintf(devname, STRSIZE, "/dev/%s", partname);
		if (partno > 0) {
			make_target_dir(mountpoint);
			error = target_mount("-v -o async",
			    devname, mountpoint);
		} else
			error = target_mount("-v -o async",
			    devname, mountpoint);
		if (error) {
			msg_display(MSG_mountfail, devname, mountpoint);
			process_menu (MENU_ok);
		}
	}
	return error;
}

int
make_fstab(void)
{
	FILE *f;
	int i;

	/* Create the fstab. */
	make_target_dir("/etc");
	f = target_fopen("/etc/fstab", "w");
	if (logging)
		(void)fprintf(log, "Creating %s/etc/fstab.\n", target_prefix());
	if (scripting)
		(void)fprintf(script, "cat <<EOF >%s/etc/fstab\n", target_prefix());

	if (f == NULL) {
#ifndef DEBUG
		msg_display(MSG_createfstab);
		if (logging)
			(void)fprintf(log, "Failed to make /etc/fstab!\n");
		process_menu(MENU_ok);
		return 1;
#else
		f = stdout;
#endif		
	}
	(void)fprintf (f, "/dev/%sa / ffs rw 1 1\n", diskdev);
	if (scripting)
		(void)fprintf (script, "/dev/%sa / ffs rw 1 1\n", diskdev);
	(void)fprintf (f, "/dev/%sb none swap sw 0 0\n", diskdev);
	if (scripting)
		(void)fprintf (script, "/dev/%sb none swap sw 0 0\n", diskdev);
	for (i = getrawpartition() + 1; i < getmaxpartitions(); i++)
		if (bsdlabel[i].pi_fstype == FS_BSDFFS) {
			(void)fprintf (f, "/dev/%s%c %s ffs rw 1 2\n",
				       diskdev, 'a'+i, fsmount[i]);
			if (scripting)
				(void)fprintf (script, "/dev/%s%c %s ffs rw 1 2\n",
					diskdev, 'a'+i, fsmount[i]);
		} else if (bsdlabel[i].pi_fstype == FS_MSDOS ) {
			(void)fprintf(f, "/dev/%s%c %s msdos rw 0 0\n",
				       diskdev, 'a'+i, fsmount[i]);
			if (scripting)
				(void)fprintf(script, "/dev/%s%c %s msdos rw 0 0\n",
					diskdev, 'a'+i, fsmount[i]);
		}
	(void)fprintf(f, "/kern /kern kernfs rw\n");
	if (scripting) {
		(void)fprintf(script, "/kern /kern kernfs rw\n");
		(void)fprintf(script, "EOF\n");
	}
#ifndef DEBUG
	fclose(f);
	fflush(NULL);
#endif
	/* We added /kern to fstab,  make mountpoint. */
	make_target_dir("/kern");
	return 0;
}


/* Get information on the file systems mounted from the root filesystem.
 * Offer to convert them into 4.4BSD inodes if they are not 4.4BSD
 * inodes.  Fsck them.  Mount them.
 */


static struct lookfor fstabbuf[] = {
	{"/dev/", "/dev/%s %s ffs %s", "c", NULL, 0, 0, foundffs},
	{"/dev/", "/dev/%s %s ufs %s", "c", NULL, 0, 0, foundffs},
};
static int numfstabbuf = sizeof(fstabbuf) / sizeof(struct lookfor);

#define MAXDEVS 40

static char dev[MAXDEVS][SSTRSIZE];
static char mnt[MAXDEVS][STRSIZE];
static int  devcnt = 0;

static void
foundffs(struct data *list, int num)
{
	if (strcmp(list[1].u.s_val, "/") != 0 &&
	    strstr(list[2].u.s_val, "noauto") == NULL) {
		strncpy(dev[devcnt], list[0].u.s_val, SSTRSIZE);
		strncpy(mnt[devcnt], list[1].u.s_val, STRSIZE);
		devcnt++;
	}
}

static int
inode_kind(char *dev)
{
	union {
		struct fs fs;
		char pad[SBSIZE];
	} fs;
	int fd;
	int ret;

	fd = open(dev, O_RDONLY, 0);
	if (fd < 0)
		return fd;
        if (lseek(fd, (off_t)SBOFF, SEEK_SET) == (off_t)-1) {
		close(fd);
                return -1;
	}
        if ((ret = read(fd, &fs, SBSIZE)) != SBSIZE) {
		close(fd);
                return -2;
	}
	close(fd);
	if (fs.fs.fs_magic != FS_MAGIC)
		return -3;
	if (fs.fs.fs_inodefmt < FS_44INODEFMT)
		return 0;
	return 1;
}

/*
 * Do an fsck. For now, assume ffs filesystems.
 * Returns zero if the fsck completed with no errors.
 * Returns a negative numbe to indicate a bad inode type,
 * and a positive  non-zero value  if exec'ing fsck returns an error.
 * If the filesystem is an out-of-date version, prompt the user
 * whether to upgrade the filesystem  level.
 * 
 */
static int
do_fsck(const char *diskpart)
{
	char raw[SSTRSIZE];
	int inodetype;
	char * upgr = "";
	int err;

	/* cons up raw partition name. */
	snprintf (raw, SSTRSIZE, "/dev/r%s", diskpart);
	inodetype = inode_kind (raw);

	if (inodetype < 0) {
		/* error */
		return inodetype;
	}
	else if (inodetype == 0) {
		/* Ask to upgrade */
		msg_display(MSG_upgrinode, raw);
		process_menu(MENU_yesno);
		if (yesno)
			upgr = "-c 3 ";
	}

	/*endwin();*/
#ifndef	DEBUG_SETS
	err = run_prog(RUN_DISPLAY, NULL, "/sbin/fsck_ffs %s%s", upgr, raw);
#else
	err = run_prog(RUN_DISPLAY, NULL, "/sbin/fsck_ffs -f %s%s", upgr, raw);
#endif	
		wrefresh(stdscr);
	return err;
}

/*
 * Do an fsck. On failure,  inform the user by showing a warning
 * message and doing menu_ok() before proceeding.
 * acknowledge the warning before continuing.
 * Returns 0 on success, or nonzero return code from do_fsck() on failure.
 */
int
fsck_with_error_menu(const char *diskpart)
{
	register int error;
	if ((error = do_fsck (diskpart)) != 0) {
#ifdef DEBUG
		fprintf(stderr, "sysinst: do_fsck() returned err %d\n", error);
#endif
		msg_display(MSG_badfs, diskpart, "", error);
		process_menu(MENU_ok);
	}
	return error;
}


/*
 * Do target_mount, but print a message and do menu_ok() before
 * proceeding, to inform the user.
 * returns 0 if  the mount completed without indicating errors,
 *  and an nonzero error code from target_mount() otherwise.
 */
int target_mount_with_error_menu(const char *opt, 
		 char *diskpart, const char *mntpoint)
{
	register int error;
	char devname[STRSIZE];

	snprintf(devname, STRSIZE, "/dev/%s", diskpart);
#ifdef DEBUG
	fprintf(stderr, "sysinst: mount_with_error_menu: %s %s %s\n",
		opt, devname, mntpoint);
#endif

	if ((error = target_mount(opt, devname, mntpoint)) != 0) {
		msg_display (MSG_badmount, devname, "");
		process_menu (MENU_ok);
		return error;
	} else {
#ifdef DEBUG
	  printf("mount %s %s %s OK\n", opt, diskpart, mntpoint);
#endif
	}

	return error;
}

/*
 * fsck and mount the root partition.
 */
int
fsck_root()
{
	int   error;
	char	rootdev[STRSIZE];

	/* cons up the root name:  partition 'a' on the target diskdev.*/
	snprintf(rootdev, STRSIZE, "%s%c", diskdev, 'a');
#ifdef DEBUG
	printf("fsck_root: rootdev is %s\n", rootdev);
#endif
	error = fsck_with_error_menu(rootdev);
	if (error != 0)
		return error;

	if (target_already_root()) {
		return (0);
	}

	/* Mount /dev/<diskdev>a on  target's "".  
	 * If we pass "" as mount-on, Prefixing will DTRT. 
	 * for now, use no options.
	 * XXX consider -o remount in case target root is
	 * current root, still  readonly from single-user? 
	 */
	error = target_mount_with_error_menu("", rootdev, "");

#ifdef DEBUG
	printf("fsck_root: mount of %s returns %d\n", rootdev, error);
#endif
	return (error);
}

int
fsck_disks(void)
{	char *fstab;
	int   fstabsize;
	int   i;
	int   error;

	/* First the root device. */
	if (!target_already_root()) {
		error = fsck_root();
		if (error != 0 && error != EBUSY) {
			return 0;
		}
	}

	/* Check the target /etc/fstab exists before trying to parse it. */
	if (target_dir_exists_p("/etc") == 0 || 
	    target_file_exists_p("/etc/fstab") == 0) {
		msg_display(MSG_noetcfstab, diskdev);
		process_menu(MENU_ok);
		return 0;
	}


	/* Get fstab entries from the target-root /etc/fstab. */
	fstabsize = target_collect_file(T_FILE, &fstab, "/etc/fstab");
	if (fstabsize < 0) {
		/* error ! */
		msg_display(MSG_badetcfstab, diskdev);
		process_menu(MENU_ok);
		return 0;
	}
	walk(fstab, fstabsize, fstabbuf, numfstabbuf);
	free(fstab);

	for (i = 0; i < devcnt; i++) {
	  	if (fsck_with_error_menu(dev[i]))
			return 0;

#ifdef DEBUG
		printf("sysinst: mount %s\n", dev[i]);
#endif
		if (target_mount_with_error_menu("", dev[i], mnt[i]) != 0) {
			return 0;
		}
	}
	
	return 1;
}

int
set_swap(dev, pp, enable)
	const char *dev;
	partinfo *pp;
{
	partinfo parts[16];
	int i, maxpart;

	if (pp == NULL) {
		emptylabel(parts);
		if (incorelabel(dev, parts) < 0)
			return -1;
		pp = parts;
	}

	maxpart = getmaxpartitions();

	for (i = 0; i < maxpart; i++) {
		if (pp[i].pi_fstype == FS_SWAP) {
			if (run_prog(0, NULL,
			    "/sbin/swapctl -%c /dev/%s%c",
			    enable ? 'a' : 'd', dev, 'a' + i) != 0)
				return -1;
			if (enable)
				strcpy(swapdev, dev);
			else
				swapdev[0] = '\0';
			break;
		}
	}

	return 0;
}
