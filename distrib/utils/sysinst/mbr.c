/*	$NetBSD: mbr.c,v 1.9 1999/01/21 08:02:18 garbled Exp $ */

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 * * Copyright 1997 Piermont Information Systems Inc.
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

/* mbr.c -- DOS Master Boot Record editing code */

#include <stdio.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"

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
	{165, "old NetBSD/FreeBSD/386BSD"},
	{169, "NetBSD"},
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
int otherpart __P((int id));
int ourpart __P((int id));
int edit_mbr __P((void));

/*
 * First, geometry  stuff...
 */
int
check_geom()
{

	return bcyl <= 1024 && bsec < 64 && bcyl > 0 && bhead > 0 && bsec > 0;
}

/*
 * get C/H/S geometry from user via menu interface and
 * store in globals.
 */
void
set_fdisk_geom()
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

void
disp_cur_geom()
{

	msg_display_add(MSG_realgeom, dlcyl, dlhead, dlsec);
	msg_display_add(MSG_biosgeom, bcyl, bhead, bsec);
}


/*
 * Then,  the partition stuff...
 */
int
otherpart(id)
	int id;
{

	return (id != 0 && id != DOSPTYP_386BSD && id != DOSPTYP_NETBSD);
}

int
ourpart(id)
	int id;
{

	return (id != 0 && (id == DOSPTYP_386BSD || id == DOSPTYP_NETBSD));
}

/*
 * Let user change incore Master Boot Record partitions via menu.
 */
int
edit_mbr()
{
	int i, j;

	/* Ask full/part */
	msg_display(MSG_fullpart, diskdev);
	process_menu(MENU_fullpart);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		int otherparts = 0;
		int ourparts = 0;

		int i;
		/* Count nonempty, non-BSD partitions. */
		for (i = 0; i < 4; i++) {
			otherparts += otherpart(part[0][ID]);
			/* check for dualboot *bsd too */
			ourparts += ourpart(part[0][ID]);
		}					  

		/* Ask if we really want to blow away non-NetBSD stuff */
		if (otherparts != 0 || ourparts > 1) {
			msg_display(MSG_ovrwrite);
			process_menu(MENU_noyes);
			if (!yesno) {
				if (logging)
					(void)fprintf(log, "User answered no to destroy other data, aborting.\n");
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
		for (i = 0; i<4; i++)
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
				/* Count 386bsd/FreeBSD/NetBSD(old) partitions */
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
				msg_display(MSG_reeditpart);
				process_menu(MENU_yesno);
			}
		} while (yesno && (numbsd != 1 || overlap));

		if (numbsd == 0) {
			msg_display(MSG_nobsdpart);
			process_menu(MENU_ok);
			return 0;
		}
			
		if (numbsd > 1) {
			msg_display(MSG_multbsdpart, bsdpart);
			process_menu(MENU_ok);
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

int
partsoverlap(i, j)
	int i;
	int j;
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

void
disp_cur_part(sel, disp)
	int sel;
	int disp;
{
	int i, j, start, stop, rsize, rend;

	if (disp < 0)
		start = 0, stop = 4;
	else
		start = disp, stop = disp+1;
	msg_display_add(MSG_part_head, multname, multname, multname);
	for (i = start; i < stop; i++) {
		if (sel == i)
			msg_standout();
		if (part[i][SIZE] == 0 && part[i][START] == 0)
			msg_printf_add("%d %36s  ", i, "");
		else {
			rsize = part[i][SIZE] / sizemult;
			if (part[i][SIZE] % sizemult)
				rsize++;
			rend = (part[i][START] + part[i][SIZE]) / sizemult;
			if ((part[i][SIZE] + part[i][SIZE]) % sizemult)
				rend++;
			msg_printf_add("%d %12d%12d%12d  ", i,
			    part[i][START] / sizemult, rsize, rend);
		}
		for (j = 0; part_ids[j].id != -1 &&
			    part_ids[j].id != part[i][ID]; j++);
		msg_printf_add ("%s\n", part_ids[j].name);
		if (sel == i) msg_standend();
	}
}
