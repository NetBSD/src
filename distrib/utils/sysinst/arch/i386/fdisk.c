/*	$NetBSD: fdisk.c,v 1.2.2.5 1998/11/16 07:25:36 cgd Exp $	*/

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

/* fdisk.c -- routines to deal with /sbin/fdisk ... */

#include <stdio.h>
#include "defs.h"
#include "md.h"
#include "txtwalk.h"
#include "msg_defs.h"
#include "menu_defs.h"

struct lookfor fdiskbuf[] = {
	{"DLCYL", "DLCYL=%d", "a $0", &dlcyl, NULL},
	{"DLHEAD", "DLHEAD=%d", "a $0", &dlhead, NULL},
	{"DLSEC", "DLSEC=%d", "a $0", &dlsec, NULL},
	{"BCYL", "BCYL=%d", "a $0", &bcyl, NULL},
	{"BHEAD", "BHEAD=%d", "a $0", &bhead, NULL},
	{"BSEC", "BSEC=%d", "a $0", &bsec, NULL},
	{"PART0ID", "PART0ID=%d", "a $0", &part[0][ID], NULL},
	{"PART0SIZE", "PART0SIZE=%d", "a $0", &part[0][SIZE], NULL},
	{"PART0START", "PART0START=%d", "a $0", &part[0][START], NULL},
	{"PART0FLAG", "PART0FLAG=0x%d", "a $0", &part[0][FLAG], NULL},
	{"PART1ID", "PART1ID=%d", "a $0", &part[1][ID], NULL},
	{"PART1SIZE", "PART1SIZE=%d", "a $0", &part[1][SIZE], NULL},
	{"PART1START", "PART1START=%d", "a $0", &part[1][START], NULL},
	{"PART1FLAG", "PART1FLAG=0x%d", "a $0", &part[1][FLAG], NULL},
	{"PART2ID", "PART2ID=%d", "a $0", &part[2][ID], NULL},
	{"PART2SIZE", "PART2SIZE=%d", "a $0", &part[2][SIZE], NULL},
	{"PART2START", "PART2START=%d", "a $0", &part[2][START], NULL},
	{"PART2FLAG", "PART2FLAG=0x%d", "a $0", &part[2][FLAG], NULL},
	{"PART3ID", "PART3ID=%d", "a $0", &part[3][ID], NULL},
	{"PART3SIZE", "PART3SIZE=%d", "a $0", &part[3][SIZE], NULL},
	{"PART3START", "PART3START=%d", "a $0", &part[3][START], NULL},
	{"PART3FLAG", "PART3FLAG=0x%d", "a $0", &part[3][FLAG], NULL}
};

int numfdiskbuf = sizeof(fdiskbuf) / sizeof(struct lookfor);



/*
 * Fetch current MBR from disk into core by parsing /sbin/fdisk output.
 */
void get_fdisk_info (void)
{
	char *textbuf;
	int   textsize;
	int   t1, t2;

	/* Get Fdisk information */
	textsize = collect (T_OUTPUT, &textbuf,
			    "/sbin/fdisk -S /dev/r%sd 2>/dev/null", diskdev);
	if (textsize < 0) {
		endwin();
		(void) fprintf (stderr, "Could not run fdisk.");
		exit (1);
	}
	walk (textbuf, textsize, fdiskbuf, numfdiskbuf);
	free (textbuf);

	/* A common failure of fdisk is to get the number of cylinders
	   wrong and the number of sectors and heads right.  This makes
	   a disk look very big.  In this case, we can just recompute
	   the number of cylinders and things should work just fine.
	   Also, fdisk may correctly indentify the settings to include
	   a cylinder total > 1024, because translation mode is not used.
	   Check for it. */

	if (bcyl > 1024 && disk->geom[1] == bhead && disk->geom[2] == bsec)
		bcyl = 1024;
	else if (bcyl > 1024 && bsec < 64) {
		t1 = disk->geom[0] * disk->geom[1] * disk->geom[2];
		t2 = bhead * bsec;
		if (bcyl * t2 > t1) {
			t2 = t1 / t2;
			if (t2 < 1024)
				bcyl = t2;
		}
	}
}

/*
 * Write incore MBR geometry and partition info to disk
 * using  /sbin/fdisk.
 */
void set_fdisk_info (void)
{
	int i;

	for (i=0; i<4; i++)
		if (part[i][SET])
                        run_prog("/sbin/fdisk -u -f -%d -b %d/%d/%d "
                                 "-s %d/%d/%d /dev/r%sd",
                                 i, bcyl, bhead, bsec,
                                 part[i][ID], part[i][START],
                                 part[i][SIZE],  diskdev);

	if (activepart >= 0)
		run_prog ("/sbin/fdisk -a -%d -f /dev/r%s",
			  activepart, diskdev);

}

struct part_id {
	int id;
	char *name;
} part_ids[] = {
	{0, "unused"},
	{1, "Primary DOS, 12 bit FAT"},
	{4, "Primary DOS, 16 bit FAT <32M"},
	{5, "Extended DOS"},
	{6, "Primary DOS, 16-bit FAT >32MB"},
	{7, "NTFS"},
	{131, "Linux native"},
	{131, "Linux swap"},
	{165, "NetBSD/FreeBSD/386bsd"},
	{169, "new NetBSD"},
	{-1, "Unknown"},
};

/*
 * Define symbolic name for MBR IDs if undefined, for backwards
 * source compatibility.
 */
#ifndef DOSPTYP_NETBSD
#define DOSPTYP_NETBSD	0xa9
#endif
#ifndef DOSPTYP_386BSD
#define DOSPTYP_386BSD	0xa5
#endif

/*
 * Which MBR partition ID to use for a newly-created NetBSD partition.
 * Left patchable as we're in the middle of changing partition IDs.
 */
#ifdef notyet
#define DOSPTYP_OURS	DOSPTYP_NETBSD	/* NetBSD >= 1.3D */
#else
#define DOSPTYP_OURS	DOSPTYP_386BSD	/* 386bsd, FreeBSD, old NetBSD */
#endif

int dosptyp_nbsd = DOSPTYP_OURS;	

/* prototypes */
int otherpart(int id);
int ourpart(int id);
int edit_mbr __P((void));



/*
 * First, geometry  stuff...
 */
int check_geom (void)
{
	return bcyl <= 1024 && bsec < 64 && bcyl > 0 && bhead > 0 && bsec > 0;
}


/*
 * get C/H/S geometry from user via menu interface and
 * store in globals.
 */
void set_fdisk_geom (void)
{
	char res[80];

	msg_display_add(MSG_setbiosgeom);
	disp_cur_geom();
	msg_printf_add("\n");
	snprintf(res, 80, "%d", bcyl);
	msg_prompt_add(MSG_cylinders, res, res, 80);
	bcyl = atoi(res);

	snprintf(res, 80, "%d", bhead);
	msg_prompt_add(MSG_heads, res, res, 80);
	bhead = atoi(res);

	snprintf(res, 80, "%d", bsec);
	msg_prompt_add(MSG_sectors, res, res, 80);
	bsec = atoi(res);
}


void disp_cur_geom (void)
{
	msg_display_add(MSG_realgeom, dlcyl, dlhead, dlsec);
	msg_display_add(MSG_biosgeom, bcyl, bhead, bsec);
}


/*
 * Then,  the partition stuff...
 */
int
otherpart(int id)
{
	return (id != 0 && id != DOSPTYP_386BSD && id != DOSPTYP_NETBSD);
}

int
ourpart(int id)
{
	return (id != 0 && (id == DOSPTYP_386BSD || id == DOSPTYP_NETBSD));
}

/*
 * Let user change incore Master Boot Record partitions via menu.
 */
int edit_mbr()
{
	int i, j;

	/* Ask full/part */
	msg_display (MSG_fullpart, diskdev);
	process_menu (MENU_fullpart);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		int otherparts = 0;
		int ourparts = 0;

		int i;
		/* Count nonempty, non-BSD partitions. */
		for (i = 0; i < 4; i++) {
			otherparts += otherpart(part[0][ID]);
			/* check for dualboot *bsd too */
			ourparts   += ourpart(part[0][ID]);
		}					  

		/* Ask if we really want to blow away non-NetBSD stuff */
		if (otherparts != 0 || ourparts > 1) {
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
		part[3][ID] = dosptyp_nbsd;
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
		int numfreebsd, freebsdpart;	/* dual-boot */

		/* Ask for sizes, which partitions, ... */
		ask_sizemult();
		bsdpart = freebsdpart = -1;
		activepart = -1;
		for (i=0; i<4; i++)
			if (part[i][FLAG] != 0)
				activepart = i;
		do {
			process_menu (MENU_editparttable);
			numbsd = 0;
			bsdpart = -1;
			freebsdpart = -1;
			numfreebsd = 0;
			overlap = 0;
			yesno = 0;
			for (i=0; i<4; i++) {
				/* Count 386bsd/FreeBSD/NetBSD partitions */
				if (part[i][ID] == DOSPTYP_386BSD) {
					freebsdpart = i;
					numfreebsd++;
				}
				/* Count NetBSD-only partitions */
				if (part[i][ID] == DOSPTYP_NETBSD) {
					bsdpart = i;
					numbsd++;
				}
				for (j = i+1; j<4; j++)
				       if (partsoverlap(i,j))
					       overlap = 1;
			}

			/* If no new-NetBSD partition, use 386bsd instead */
			if (numbsd == 0 && numfreebsd > 0) {
				numbsd = numfreebsd;
				bsdpart = freebsdpart;
				/* XXX check partition type? */
			}

			/* Check for overlap or multiple native partitions */
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

int partsoverlap(int i, int j)
{
	if (part[i][SIZE] == 0 || part[j][SIZE] == 0)
		return 0;

	return 
		(part[i][START] < part[j][START] &&
		 part[i][START] + part[i][SIZE] > part[j][START])
		||
		(part[i][START] > part[j][START] &&
		 part[i][START] < part[j][START] + part[j][SIZE])
		||
		(part[i][START] == part[j][START]);

}

void disp_cur_part(int sel, int disp)
{
	int i, j, start, stop, rsize, rend;

	if (disp < 0)
		start = 0, stop = 4;
	else
		start = disp, stop = disp+1;
	msg_display_add (MSG_part_head, multname, multname, multname);
	for (i=start; i<stop; i++) {
		if (sel == i) msg_standout();
		if (part[i][SIZE] == 0 && part[i][START] == 0)
			msg_printf_add ("%d %36s  ", i, "");
		else {
			rsize = part[i][SIZE] / sizemult;
			if (part[i][SIZE] % sizemult)
				rsize++;
			rend = (part[i][START] + part[i][SIZE]) / sizemult;
			if ((part[i][SIZE] + part[i][SIZE]) % sizemult)
				rend++;
			msg_printf_add("%d %12d%12d%12d  ", i,
					part[i][START] / sizemult,
					rsize, rend);
		}
		for (j = 0; part_ids[j].id != -1 &&
			    part_ids[j].id != part[i][ID]; j++);
		msg_printf_add ("%s\n", part_ids[j].name);
		if (sel == i) msg_standend();
	}
}
