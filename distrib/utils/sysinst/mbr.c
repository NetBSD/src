/*	$NetBSD: mbr.c,v 1.34 2003/05/16 19:44:23 dsl Exp $ */

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

/*
 * Following applies to the geometry guessing code
 */

/*
 * Mach Operating System
 * Copyright (c) 1992 Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/* mbr.c -- DOS Master Boot Record editing code */

#include <sys/param.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include "defs.h"
#include "mbr.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "endian.h"

struct part_id {
	int id;
	char *name;
} part_ids[] = {
	{0, "unused"},
	{MBR_PTYPE_FAT12, "DOS FAT12"},
	{MBR_PTYPE_FAT16S, "DOS FAT16, <32M"},
	{MBR_PTYPE_EXT, "Extended partition"},
	{MBR_PTYPE_FAT16B, "DOS FAT16, >32M"},
	{MBR_PTYPE_NTFS, "NTFS"},
	{MBR_PTYPE_FAT32, "Windows FAT32"},
	{MBR_PTYPE_FAT32L, "Windows FAT32, LBA"},
	{MBR_PTYPE_FAT16L, "Windows FAT16, LBA"},
	{MBR_PTYPE_EXT_LBA, "Extended partition, LBA"},
	{MBR_PTYPE_LNXSWAP, "Linux swap"},
	{MBR_PTYPE_LNXEXT2, "Linux native"},
	{MBR_PTYPE_NTFSVOL, "NTFS volume set"},
	{MBR_PTYPE_386BSD, "old NetBSD/FreeBSD/386BSD"},
	{MBR_PTYPE_NETBSD, "NetBSD"},
	{-1, "Unknown"},
};

int dosptyp_nbsd = MBR_PTYPE_NETBSD;

static int get_mapping (struct mbr_partition *, int, int *, int *, int *,
			    long *absolute);
static void convert_mbr_chs (int, int, int, u_int8_t *, u_int8_t *,
				 u_int8_t *, u_int32_t);

#ifdef BOOTSEL
static int defbootselpart, defbootseldisk;
struct mbr_bootsel *mbs;
#endif

#ifdef notdef
/*
 * First, geometry  stuff...
 */
int
check_geom()
{

	return bcyl <= 1024 && bsec < 64 && bcyl > 0 && bhead > 0 && bsec > 0;
}
#endif

/*
 * get C/H/S geometry from user via menu interface and
 * store in globals.
 */
void
set_bios_geom(cyl, head, sec)
	int cyl, head, sec;
{
	char res[80];

	msg_display_add(MSG_setbiosgeom);
	snprintf(res, 80, "%d", cyl);
	msg_prompt_add(MSG_cylinders, res, res, 80);
	bcyl = atoi(res);

	snprintf(res, 80, "%d", head);
	msg_prompt_add(MSG_heads, res, res, 80);
	bhead = atoi(res);

	snprintf(res, 80, "%d", sec);
	msg_prompt_add(MSG_sectors, res, res, 80);
	bsec = atoi(res);
}

#ifdef notdef
void
disp_cur_geom()
{

	msg_display_add(MSG_realgeom, dlcyl, dlhead, dlsec);
	msg_display_add(MSG_biosgeom, bcyl, bhead, bsec);
}
#endif


/*
 * Then,  the partition stuff...
 */
int
otherpart(id)
	int id;
{

	return (id != 0 && id != MBR_PTYPE_386BSD && id != MBR_PTYPE_NETBSD);
}

int
ourpart(id)
	int id;
{

	return (id == MBR_PTYPE_386BSD || id == MBR_PTYPE_NETBSD);
}

/*
 * Let user change incore Master Boot Record partitions via menu.
 */
int
edit_mbr(mbr)
	struct mbr_sector *mbr;
{
	int i, j;

	/* Ask full/part */

	/* XXX this sucks ("part" is used in menus, no param passing there) */
	part = &mbr->mbr_parts[0];
	msg_display(MSG_fullpart, diskdev);
	process_menu(MENU_fullpart);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		int otherparts = 0;
		int ourparts = 0;

		/* Count nonempty, non-BSD partitions. */
		for (i = 0; i < NMBRPART; i++) {
			otherparts += otherpart(part[i].mbrp_typ);
			/* check for dualboot *bsd too */
			ourparts += ourpart(part[i].mbrp_typ);
		}					  

		/* Ask if we really want to blow away non-NetBSD stuff */
		if (otherparts != 0 || ourparts > 1) {
			msg_display(MSG_ovrwrite);
			process_menu(MENU_noyes);
			if (!yesno) {
				if (logging)
					(void)fprintf(logfp, "User answered no to destroy other data, aborting.\n");
				return 0;
			}
		}

		/* Set the partition information for full disk usage. */
		part[0].mbrp_typ = part[0].mbrp_flag = 0;
		part[0].mbrp_start = part[0].mbrp_size = 0;
		part[1].mbrp_typ = part[0].mbrp_flag = 0;
		part[1].mbrp_start = part[0].mbrp_size = 0;
		part[2].mbrp_typ = part[0].mbrp_flag = 0;
		part[2].mbrp_start = part[0].mbrp_size = 0;
		part[3].mbrp_typ = dosptyp_nbsd;
		part[3].mbrp_size = bsize - bsec;
		part[3].mbrp_start = bsec;
		part[3].mbrp_flag = 0x80;

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
		ask_sizemult(bcylsize);
		bsdpart = freebsdpart = -1;
		activepart = -1;
		for (i = 0; i < NMBRPART; i++)
			if (part[i].mbrp_flag != 0) {
				activepart = i;
				part[i].mbrp_flag = 0;
			}
		do {
			process_menu (MENU_editparttable);
			numbsd = 0;
			bsdpart = -1;
			freebsdpart = -1;
			numfreebsd = 0;
			overlap = 0;
			yesno = 0;
			for (i=0; i < NMBRPART; i++) {
				/* Count 386bsd/FreeBSD/NetBSD(old) partitions */
				if (part[i].mbrp_typ == MBR_PTYPE_386BSD) {
					freebsdpart = i;
					numfreebsd++;
				}
				/* Count NetBSD-only partitions */
				if (part[i].mbrp_typ == MBR_PTYPE_NETBSD) {
					if (bsdpart == -1)
						bsdpart = i;
					numbsd++;
				}
				for (j = i + 1; j < NMBRPART; j++)
				       if (partsoverlap(part, i, j))
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

		if (activepart == -1) {
			msg_display(MSG_noactivepart);
			process_menu(MENU_yesno);
			if (yesno)
				part[bsdpart].mbrp_flag = 0x80;
		} else
			part[activepart].mbrp_flag = 0x80;

		if (bsdpart == freebsdpart) {
			msg_display(MSG_upgradeparttype);
			process_menu(MENU_yesno);
			if (yesno)
				part[bsdpart].mbrp_typ = dosptyp_nbsd;
		}
			
		ptstart = part[bsdpart].mbrp_start;
		ptsize = part[bsdpart].mbrp_size;
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

	return 1;
}

void
edit_ptn_bounds(void)
{
	char buf[40]; int start, size, inp, partn;

	msg_table_add(MSG_mbrpart_start_special);
	msg_prompt_add(MSG_start, NULL, buf, sizeof buf);
	inp = atoi(buf);
	/*
	 * -0, -1, -2, -3: start at end of part # given
	 * 0: start of disk.
	 */
	if ((inp == 0 && buf[0] == '-') || (inp < 0 && inp >= -3)) {
		partn = -inp;
		start = part[partn].mbrp_start + part[partn].mbrp_size;
	} else if (inp == 0)
		start = bsec;
	else
		start = NUMSEC(inp, sizemult, dlcylsize);

	if (sizemult > 1 && start < bsec)
		start = bsec;
	msg_table_add(MSG_mbrpart_size_special);
	msg_prompt_add (MSG_size, NULL, buf, 40);
	inp = atoi(buf);
	/*
	 * -0, -1, -2, -3: until start of part # given
	 * 0: end of disk
	 */
	if ((inp == 0 && buf[0] == '-') || (inp < 0 && inp >= -3)) {
		partn = -inp;
		size = part[partn].mbrp_start - start;
	} else if (inp == 0)
		size = dlsize - start;
	else
		size = NUMSEC(inp, sizemult, dlcylsize);
	if (sizemult > 1 && start == bsec)
		size -= bsec;
	if (start + size > bsize)
		size = bsize - start;
	if (size < 0) {
		size = 0;
		start = 0;
	}
	part[editpart].mbrp_start = start;
	part[editpart].mbrp_size = size;
}

int
partsoverlap(part, i, j)
	struct mbr_partition *part;
	int i;
	int j;
{

	/*
	 * If the size or type of either partition is zero, they don't
	 * overlap by definition.
	 */
	if (part[i].mbrp_size == 0 || part[j].mbrp_size == 0 ||
	    part[i].mbrp_typ == 0 || part[j].mbrp_typ == 0)
		return 0;

	return 
		(part[i].mbrp_start < part[j].mbrp_start &&
		 part[i].mbrp_start + part[i].mbrp_size > part[j].mbrp_start)
		||
		(part[i].mbrp_start > part[j].mbrp_start &&
		 part[i].mbrp_start < part[j].mbrp_start + part[j].mbrp_size)
		||
		(part[i].mbrp_start == part[j].mbrp_start);
}

char *
get_partname(i)
	int i;
{
	int j;
	static char unknown[32];

	for (j = 0; part_ids[j].id != -1; j++)
		if (part_ids[j].id == part[i].mbrp_typ)
			return part_ids[j].name;

	snprintf(unknown, sizeof unknown, "Unknown (%d)", part[i].mbrp_typ);
	return unknown;
}

void
disp_cur_part(part, sel, disp)
	struct mbr_partition *part;
	int sel;
	int disp;
{
	int i, start, stop, rsize, rend;

	if (disp < 0)
		start = 0, stop = 4;
	else
		start = disp, stop = disp+1;
	msg_table_add(MSG_part_header, dlsize/sizemult, multname, multname,
	    multname, multname);
	for (i = start; i < stop; i++) {
		if (sel == i)
			msg_standout();
		if (part[i].mbrp_typ == 0 ||
		    (part[i].mbrp_size == 0 && part[i].mbrp_start == 0))
			msg_table_add(MSG_part_row_start_unused, i);
		else {
			rsize = part[i].mbrp_size / sizemult;
			if (part[i].mbrp_size % sizemult)
				rsize++;
			rend = (part[i].mbrp_start + part[i].mbrp_size) / sizemult;
			if ((part[i].mbrp_start + part[i].mbrp_size) % sizemult)
				rend++;
			msg_table_add(MSG_part_row_start_used, i,
			    part[i].mbrp_start / sizemult, rend, rsize);
		}
		msg_table_add(MSG_part_row_end, get_partname(i));
		if (sel == i)
			msg_standend();
	}
}

int
read_mbr(disk, mbr, len)
	const char *disk;
	mbr_sector_t *mbr;
	size_t len;
{
	char diskpath[MAXPATHLEN];
	int fd, i;
	struct mbr_partition *mbrp;

	/* Open the disk. */
	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0) 
		return -1;

	if (lseek(fd, (off_t)(MBR_BBSECTOR * MBR_SECSIZE), SEEK_SET) < 0) {
		close(fd);
		return -1;
	}
	if (read(fd, mbr, len) < len) {
		close(fd);
		return -1;
	}

	if (valid_mbr(mbr)) {
		mbrp = &mbr->mbr_parts[0];
		for (i = 0; i < NMBRPART; i++) {
			if (mbrp[i].mbrp_typ != 0) {
				mbrp[i].mbrp_start =
				    le_to_native32(mbrp[i].mbrp_start);
				mbrp[i].mbrp_size =
				    le_to_native32(mbrp[i].mbrp_size);
			} else {
				/* type is unused, discard scum */
				mbrp[i].mbrp_start = 0;
				mbrp[i].mbrp_size  = 0;
			}
		}
	}

	(void)close(fd);
	return 0;
}

int
write_mbr(disk, mbr, len, convert)
	const char *disk;
	mbr_sector_t *mbr;
	size_t len;
	int convert;
{
	char diskpath[MAXPATHLEN];
	int fd, i, ret = 0;
	struct mbr_partition *mbrp;
	u_int32_t pstart, psize;

	/* Open the disk. */
	fd = opendisk(disk, O_WRONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0) 
		return -1;

	if (lseek(fd, (off_t)(MBR_BBSECTOR * MBR_SECSIZE), SEEK_SET) < 0) {
		close(fd);
		return -1;
	}

	mbrp = &mbr->mbr_parts[0];
	for (i = 0; i < NMBRPART; i++) {
		if (mbrp[i].mbrp_start == 0 &&
		    mbrp[i].mbrp_size == 0) {
			mbrp[i].mbrp_scyl = 0;
			mbrp[i].mbrp_shd = 0;
			mbrp[i].mbrp_ssect = 0;
			mbrp[i].mbrp_ecyl = 0;
			mbrp[i].mbrp_ehd = 0;
			mbrp[i].mbrp_esect = 0;
		} else {
			pstart = mbrp[i].mbrp_start;
			psize = mbrp[i].mbrp_size;
			mbrp[i].mbrp_start = native_to_le32(pstart);
			mbrp[i].mbrp_size = native_to_le32(psize);
			if (convert) {
				convert_mbr_chs(bcyl, bhead, bsec,
				    &mbrp[i].mbrp_scyl, &mbrp[i].mbrp_shd,
				    &mbrp[i].mbrp_ssect, pstart);
				convert_mbr_chs(bcyl, bhead, bsec,
				    &mbrp[i].mbrp_ecyl, &mbrp[i].mbrp_ehd,
				    &mbrp[i].mbrp_esect, pstart + psize - 1);
			}
		}
	}

	if (write(fd, mbr, len) < 0)
		ret = -1;

	(void)close(fd);
	return ret;
}

int
valid_mbr(mbr)
	mbr_sector_t *mbr;
{

	return (le_to_native16(mbr->mbr_signature) == MBR_MAGIC);
}

static void
convert_mbr_chs(cyl, head, sec, cylp, headp, secp, relsecs)
	int cyl, head, sec;
	u_int8_t *cylp, *headp, *secp;
	u_int32_t relsecs;
{
	unsigned int tcyl, temp, thead, tsec;

	temp = cyl * head * sec - 1;
	if (relsecs >= temp)
		relsecs = temp;

	temp = head * sec;
	tcyl = relsecs / temp;

	relsecs %= temp;
	thead = relsecs / sec;

	tsec = (relsecs % sec) + 1;

	*cylp = MBR_PUT_LSCYL(tcyl);
	*headp = thead;
	*secp = MBR_PUT_MSCYLANDSEC(tcyl, tsec);
}

/*
 * This function is ONLY to be used as a last resort to provide a
 * hint for the user. Ports should provide a more reliable way
 * of getting the BIOS geometry. The i386 code, for example,
 * uses the BIOS geometry as passed on from the bootblocks,
 * and only uses this as a hint to the user when that information
 * is not present, or a match could not be made with a NetBSD
 * device.
 */
int
guess_biosgeom_from_mbr(mbr, cyl, head, sec)
	mbr_sector_t *mbr;
	int *cyl, *head, *sec;
{
	struct mbr_partition *parts = &mbr->mbr_parts[0];
	int cylinders = -1, heads = -1, sectors = -1, i, j;
	int c1, h1, s1, c2, h2, s2;
	long a1, a2;
	quad_t num, denom;

	*cyl = *head = *sec = -1;

	/* Try to deduce the number of heads from two different mappings. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		for (j = 0; j < 8; j++) {
			if (get_mapping(parts, j, &c2, &h2, &s2, &a2) < 0)
				continue;
			num = (quad_t)h1*(a2-s2) - (quad_t)h2*(a1-s1);
			denom = (quad_t)c2*(a1-s1) - (quad_t)c1*(a2-s2);
			if (denom != 0 && num % denom == 0) {
				heads = (int)(num / denom);
				break;
			}
		}
		if (heads != -1)	
			break;
	}

	if (heads == -1)
		return -1;

	/* Now figure out the number of sectors from a single mapping. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		num = a1 - s1;
		denom = c1 * heads + h1;
		if (denom != 0 && num % denom == 0) {
			sectors = (int)(num / denom);
			break;
		}
	}

	if (sectors == -1)
		return -1;

	/*
	 * Estimate the number of cylinders.
	 * XXX relies on get_disks having been called.
	 */
	cylinders = disk->dd_totsec / heads / sectors;

	/* Now verify consistency with each of the partition table entries.
	 * Be willing to shove cylinders up a little bit to make things work,
	 * but translation mismatches are fatal. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		if (sectors * (c1 * heads + h1) + s1 != a1)
			return -1;
		if (c1 >= cylinders)
			cylinders = c1 + 1;
	}

	/* Everything checks out.  Reset the geometry to use for further
	 * calculations. */
	*cyl = cylinders;
	*head = heads;
	*sec = sectors;
	return 0;
}

static int
get_mapping(parts, i, cylinder, head, sector, absolute)
	struct mbr_partition *parts;
	int i, *cylinder, *head, *sector;
	long *absolute;
{
	struct mbr_partition *apart = &parts[i / 2];

	if (apart->mbrp_typ == 0)
		return -1;
	if (i % 2 == 0) {
		*cylinder = MBR_PCYL(apart->mbrp_scyl, apart->mbrp_ssect);
		*head = apart->mbrp_shd;
		*sector = MBR_PSECT(apart->mbrp_ssect) - 1;
		*absolute = apart->mbrp_start;
	} else {
		*cylinder = MBR_PCYL(apart->mbrp_ecyl, apart->mbrp_esect);
		*head = apart->mbrp_ehd;
		*sector = MBR_PSECT(apart->mbrp_esect) - 1;
		*absolute = apart->mbrp_start + apart->mbrp_size - 1;
	}
	return 0;
}

#ifdef BOOTSEL

void
disp_bootsel(void)
{
	int i;

	msg_display(MSG_configbootsel);
	msg_table_add(MSG_bootsel_header);
	for (i = 0; i < NMBRPART; i++) {
		msg_table_add(MSG_bootsel_row, i, get_partname(i),
			mbs->mbrb_nametab[i]);
	}
	msg_display_add(MSG_newline);

	msg_display_add(MSG_bootseltimeout, (10 * mbs->mbrb_timeo + 9) / 182);
	msg_display_add(MSG_defbootselopt);
	if (mbs->mbrb_defkey == SCAN_ENTER)
		msg_display_add(MSG_defbootseloptactive);
	else if (mbs->mbrb_defkey < (SCAN_1 + 10))
		msg_display_add(MSG_defbootseloptpart, defbootselpart);
	else
		msg_display_add(MSG_defbootseloptdisk, defbootseldisk);
}

void
edit_bootsel_entry(int ptn)
{
	if (part[ptn].mbrp_typ != 0)
		msg_prompt(MSG_bootselitemname, mbs->mbrb_nametab[ptn],
			    mbs->mbrb_nametab[ptn], 8);
}

void
edit_bootsel_timeout(void)
{
	char tstr[8];
	unsigned timo;

	do {
		snprintf(tstr, 8, "%u", (10 * mbs->mbrb_timeo + 9) / 182);
		msg_prompt(MSG_bootseltimeoutval, tstr, tstr, 8);
		timo = (unsigned)atoi(tstr);
	} while (timo > 3600);
	mbs->mbrb_timeo = (u_int16_t)((timo * 182) / 10);
}

void
edit_bootsel_default_ptn(int ptn)
{
	int i;
	int key = SCAN_1;

	if (mbs->mbrb_nametab[ptn][0] == 0 || part[ptn].mbrp_typ == 0)
		return;
	for (i = 0; i < ptn; i++)
		if (mbs->mbrb_nametab[i][0] != 0 && part[i].mbrp_typ != 0)
			key++;
	mbs->mbrb_defkey = key;
	defbootselpart = ptn;
}

void
edit_bootsel_default_disk(int disk)
{

	mbs->mbrb_defkey = SCAN_F1 + disk;
	defbootseldisk = disk;
}

void
configure_bootsel(void)
{
	struct mbr_partition *parts = &mbr.mbr_parts[0];
	int i;
	const char *name, *cp;

	mbs = &mbr.mbr_bootsel;

	/*
	 * Setup default labels for partitions, since if not done by user
	 * they don't get set and bootselector doesn't 'appear' when
	 * it's loaded.
	 */
	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ == MBR_PTYPE_NETBSD &&
		    mbs->mbrb_nametab[i][0] != 0)
			goto labels_ok;
	}

	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ == 0 ||
		    parts[i].mbrp_typ == MBR_PTYPE_LNXSWAP ||
		    MBR_IS_EXTENDED(parts[i].mbrp_typ) ||
		    mbs->mbrb_nametab[i][0] != '\0')
			continue;
		/* Default to first word of partition name */
		name = get_partname(i);
		cp = strchr(name, ' ');
		snprintf(mbs->mbrb_nametab[i], sizeof(mbs->mbrb_nametab[0]),
			 "%.*s", cp != NULL ? cp - name : 256, name);
	}

    labels_ok:
	process_menu(MENU_configbootsel);

#if 0
	/* The current bootselect code doesn't need this... */
	mbs->mbrb_flags &= ~BFL_EXTINT13;
	for (i = 0; i < NMBRPART; i++) {
		if (parts[i].mbrp_typ != 0 &&
		   parts[i].mbrp_start >= (bcyl * bhead * bsec)) {
			mbs->mbrb_flags |= BFL_EXTINT13;
			break;
		}
	}
#endif
}

#endif
