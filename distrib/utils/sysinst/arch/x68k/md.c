/*	$NetBSD: md.c,v 1.1 1999/07/09 15:29:45 minoura Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.  Modified by Minoura Makoto for x68k.
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
 */

/* md.c -- Machine specific code for x68k */

#include <stdio.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
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
#define	PART_USR	D


#ifdef notyet
#undef NDOSPART 8
#define NDOSPART 16
typedef struct parttab {
	struct dos_partition dosparts[NDOSPART];
} parttab;

parttab md_disklabel;
int md_freepart;
int md_nfreepart;
#endif
int md_need_newdisk = 0;

/* prototypes */
static int md_newdisk __P((void));
static int md_check_partition_order __P((void));

int
md_get_info()
{
	char buf[1024];
	int fd;
	char devname[100];
	struct disklabel disklabel;

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
	if (disklabel.d_secsize != 512) {
		endwin();
		fprintf(stderr, "Non-512byte/sector disk is not supported.\n");
		close(fd);
		exit(1);
	}

	dlcyl = disklabel.d_ncylinders;
	dlhead = disklabel.d_ntracks;
	dlsec = disklabel.d_nsectors;
	sectorsize = disklabel.d_secsize;
	dlcylsize = disklabel.d_secpercyl;
	dlsize = dlcyl*dlhead*dlsec;

	if (read(fd, buf, 1024) < 0) {
		endwin();
		fprintf(stderr, "Can't read %s\n", devname);
		close(fd);
		exit(1);
	}
	if (memcmp(buf, "X68SCSI1", 8) != 0)
		md_need_newdisk = 1;
#ifdef notyet
	else
		if (read(fd, md_disklabel, sizeof(md_disklabel)) < 0) {
			endwin();
			fprintf(stderr, "Can't read %s\n", devname);
			close(fd);
			exit(1);
		}
#endif
	/* preserve first 64 sectors for system. */
	dlsize -= 64;

	/* preserve existing partitions? */

	close(fd);

	/* Compute minimum NetBSD partition sizes (in sectors). */
	minfsdmb = (80 + 4*rammb) * (MEG / sectorsize);

	return 1;
}

#ifndef DEBUG
static int
md_newdisk(void)
	int fd;
	size_t size;
{
	int mbootfd;
	char devname[100];
	int fd;
	char buf[1024];
	size_t size = dlsize + 64;

	snprintf(devname, 100, "/dev/r%sc", diskdev);
	fd = open (devname, O_WRONLY);
	if (fd < 0) {
		endwin();
		fprintf(stderr, "Can't open %s\n", devname);
		exit(1);
	}

	msg_display(MSG_newdisk, diskdev, diskdev);

	/* Write disk mark */
	memset(buf, 0, 1024);
	sprintf(buf, "X68SCSI1%c%c%c%c%c%c%c%c"
		     "NetBSD/x68k SCSI primary boot. "
		     "(C) 1999 by The NetBSD Foundation, Inc.",
		2, 0,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		1, 0);
	lseek(fd, 0, SEEK_SET);
	if (write(fd, buf, 1024) < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't write mark on %s\n", devname);
		exit(1);
	}

	/* Write primary boot */
	memset(buf, 0, 1024);
	mbootfd = open("/usr/mdec/mboot", O_RDONLY);
	if (mbootfd < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't read mboot.\n");
		exit(1);
	}
	if (read(mbootfd, buf, 1024) < 0) {
		endwin();
		close(fd);
		close(mbootfd);
		fprintf(stderr, "Can't read mboot.\n");
		exit(1);
	}
	close(mbootfd);
	if (write(fd, buf, 1024) != 1024) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't write mboot.\n");
		exit(1);
	}

	/* Create empty partition map */
#ifdef notyet
	memset(&md_disklabel, 0, sizeof(md_disklabel));
	sprintf((char*) md_disklabel, "X68K%c%c%c%c%c%c%c%c%c%c%c%c",
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		((size-64)>>24)&0xff, ((size-64)>>16)&0xff,
		((size-64)>>8)&0xff, (size-64)&0xff);
	if (write(fd, md_disklabel, 1024) < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't create partition table.\n");
		exit(1);
	}
#else
	memset(buf, 0, 1024);
	sprintf(buf, "X68K%c%c%c%c%c%c%c%c%c%c%c%c",
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		(size>>24)&0xff, (size>>16)&0xff, (size>>8)&0xff, size&0xff,
		((size-64)>>24)&0xff, ((size-64)>>16)&0xff,
		((size-64)>>8)&0xff, (size-64)&0xff);
	if (write(fd, buf, 1024) < 0) {
		endwin();
		close(fd);
		fprintf(stderr, "Can't create partition table.\n");
		exit(1);
	}
#endif

	close (fd);

	return 0;
}
#else
static int
md_newdisk(fd, size, buf)
	int fd;
	size_t size;
	char *buf;
{
	return 0;
}
#endif

#ifdef notyet
static int
md_check_partitions(void)
{
	int i, j;
	int preserve;

	/* check existing BSD partitions. */
	for (i = 0; i < NDOSPART; i++) {
		if (md_disklabel.dosparts[i].dp_size == 0)
			break;
		if (memcmp(md_disklabel.dosparts[i].dp_typename, "Human68k", 8)) {
			msg_display(MSG_existing);
			process_menu(MENU_noyes);
			preserve = yesno;
			break;
		}
	}
	emptylabel(bsdlabel);
	bsdlabel[C].pi_fstype = FS_UNUSED;
	bsdlabel[C].pi_offset = 0;
	bsdlabel[C].pi_size = dlsize;
	for (i = 0, j = A; i < NDOSPART;) {
		if (j == C) {
			j++;
			continue;
		}
		if (!preserve &&
		    memcmp(md_disklabel.dosparts[i].dp_typename,
			    "Human68k", 8)) {
			/* discard it. */
			i++;
			continue;
		}
		bsdlabel[j].pi_fstype = (i == 1) ? FS_SWAP : FS_BSDFFS;
		bsdlabel[j].pi_offset = md_disklabel.dosparts[i].dp_start*2;
		bsdlabel[j].pi_size = md_disklabel.dosparts[i].dp_size*2;
		i++;
		j++;
	}
	if (j > 6) {
		msg_display(MSG_nofreepart, diskdev);
		return 0;
	}
	md_nfreepart = 8 - j;

	/* check for free space */
	fsptsize = bsdlabel[A].pi_offset - 64;
	if (fptsize <= 0) {	/* XXX: should not be 0; minfsdb?  */
		msg_display(MSG_notfirst, diskdev);
		process_menu(MENU_ok);
		exit(1);
	}

	/* Partitions should be preserved in md_make_bsdpartitions() */
}	
#endif

/*
 * hook called before writing new disklabel.
 */
int
md_pre_disklabel()
{
	if (md_need_newdisk)
		md_newdisk ();
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
 * On the sparc, we use this opportunity to install the boot blocks.
 */
int
md_post_newfs(void)
{
	/* boot blocks ... */
	msg_display(MSG_dobootblks, diskdev);
	return run_prog(0, 1, NULL,
	    "/usr/mdec/installboot -v /usr/mdec/sdboot /dev/r%sa",
	    diskdev);
}

/*
 * some ports use this to copy the MD filesystem, we do not.
 */
int
md_copy_filesystem(void)
{
	return 0;
}

int
md_make_bsd_partitions(void)
{
	int i, part;
	int remain;
	char isize[20];

	/*
	 * Initialize global variables that track  space used on this disk.
	 * Standard 4.3BSD 8-partition labels always cover whole disk.
	 */
	ptstart = 64;		/* assume 512 byte/sector */
	ptsize = dlsize - ptstart;
	fsdsize = dlsize;
	fsptsize = dlsize - ptstart;	/* netbsd partition -- same as above */
	fsdmb = fsdsize / MEG;

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
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

	/* Partitions C is predefined (whole disk). */
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
		/* 20Mbyte for root */
		partsize= NUMSEC(20, MEG/sectorsize, dlcylsize);
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy(fsmount[A], "/");
		partstart += partsize;

		/* swap */
		i = NUMSEC (layoutkind * 2 * (rammb < 32 ? 32 : rammb),
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
		strcpy(fsmount[PART_USR], "/usr");

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult();
		/* root */
		partstart = ptstart;
		remain = fsdsize - partstart;
		/* 20Mbyte for root */
		partsize = NUMSEC (20, MEG/sectorsize, dlcylsize);
		snprintf(isize, 20, "%d", partsize/sizemult);
		msg_prompt(MSG_askfsroot, isize, isize, 20,
			   remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[A].pi_offset = partstart;
		bsdlabel[A].pi_size = partsize;
		bsdlabel[A].pi_bsize = 8192;
		bsdlabel[A].pi_fsize = 1024;
		strcpy(fsmount[A], "/");
		partstart += partsize;
		
		/* swap */
		remain = fsdsize - partstart;
		i = NUMSEC(2 * (rammb < 32 ? 32 : rammb),
			   MEG/sectorsize, dlcylsize) + partstart;
		partsize = NUMSEC (i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
		snprintf(isize, 20, "%d", partsize/sizemult);
		msg_prompt_add(MSG_askfsswap, isize, isize, 20,
			       remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		bsdlabel[B].pi_offset = partstart;
		bsdlabel[B].pi_size = partsize;
		partstart += partsize;
		
		/* /usr */
		remain = fsdsize - partstart;
		partsize = fsdsize - partstart;
		snprintf(isize, 20, "%d", partsize/sizemult);
		msg_prompt_add(MSG_askfsusr, isize, isize, 20,
			       remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize),sizemult, dlcylsize);
		if (remain - partsize < sizemult)
			partsize = remain;
		bsdlabel[PART_USR].pi_fstype = FS_BSDFFS;
		bsdlabel[PART_USR].pi_offset = partstart;
		bsdlabel[PART_USR].pi_size = partsize;
		bsdlabel[PART_USR].pi_bsize = 8192;
		bsdlabel[PART_USR].pi_fsize = 1024;
		strcpy(fsmount[PART_USR], "/usr");
		partstart += partsize;

		/* Others ... */
		remain = fsdsize - partstart;
		part = F;
		if (remain > 0)
			msg_display(MSG_otherparts);
		while (remain > 0 && part <= H) {
			partsize = fsdsize - partstart;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfspart, isize, isize, 20,
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
			msg_prompt_add(MSG_mountpoint, NULL,
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
 edit_check:
	if (edit_and_check_label(bsdlabel, 8, RAW_PART, RAW_PART) == 0) {
		msg_display(MSG_abort);
		return 0;
	}
	if (md_check_partition_order() == 0)
		goto edit_check;

	/* Disk name */
	msg_prompt(MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, 8);	/* 8 partitions in  label */

	/* Everything looks OK. */
	return (1);
}

static int
md_check_partition_order(void)
{
	/* X68k partitions must be in order of the range. */
	int part, start = 0, last = A-1;

	for (part = A; part < 8; part++) {
		if (part == C)
			continue;
		if (last >= A && bsdlabel[part].pi_size > 0) {
			msg_display(MSG_emptypart, part+'a');
			process_menu(MENU_ok);
			return 0;
		}
		if (bsdlabel[part].pi_size == 0) {
			if (last < A)
				last = part;
		} else {
			if (start >= bsdlabel[part].pi_offset) {
				msg_display(MSG_ordering, part+'a');
				process_menu(MENU_yesno);
				if (yesno)
					return 0;
			}
			start = bsdlabel[part].pi_offset;
		}
	}

	return 1;
}

/* Upgrade support */
int
md_update(void)
{
	endwin();
	md_copy_filesystem();
	md_post_newfs();
	puts(CL);		/* XXX */
	wclear(stdscr);
	wrefresh(stdscr);
	return 1;
}

void
md_cleanup_install(void)
{
	char realfrom[STRSIZE];
	char realto[STRSIZE];
	char sedcmd[STRSIZE];

	strncpy(realfrom, target_expand("/etc/rc.conf"), STRSIZE);
	strncpy(realto, target_expand("/etc/rc.conf.install"), STRSIZE);

	sprintf(sedcmd, "sed 's/rc_configured=NO/rc_configured=YES/' < %s > %s",
	    realfrom, realto);
	if (logging)
		(void)fprintf(log, "%s\n", sedcmd);
	if (scripting)
		(void)fprintf(script, "%s\n", sedcmd);
	do_system(sedcmd);

	run_prog(1, 0, NULL, "mv -f %s %s", realto, realfrom);
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/sysinst"));
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/.termcap"));
	run_prog(0, 0, NULL, "rm -f %s", target_expand("/.profile"));
}
