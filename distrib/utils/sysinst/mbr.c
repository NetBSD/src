/*	$NetBSD: mbr.c,v 1.44 2003/07/07 21:26:32 dsl Exp $ */

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

#define NO_BOOTMENU (-0x100)

struct part_id {
	int id;
	char *name;
} part_ids[] = {
	{0,			"unused"},
	{MBR_PTYPE_NETBSD,	"NetBSD"},
	{MBR_PTYPE_EXT_LBA,	"Extended partition, LBA"},
	{MBR_PTYPE_386BSD,	"FreeBSD/386BSD"},
	{MBR_PTYPE_OPENBSD,	"OpenBSD"},
	{MBR_PTYPE_LNXEXT2,	"Linux native"},
	{MBR_PTYPE_LNXSWAP,	"Linux swap"},
	{MBR_PTYPE_FAT12,	"DOS FAT12"},
	{MBR_PTYPE_FAT16S,	"DOS FAT16, <32M"},
	{MBR_PTYPE_FAT16B,	"DOS FAT16, >32M"},
	{MBR_PTYPE_FAT16L,	"Windows FAT16, LBA"},
	{MBR_PTYPE_FAT32,	"Windows FAT32"},
	{MBR_PTYPE_FAT32L,	"Windows FAT32, LBA"},
	{MBR_PTYPE_NTFSVOL,	"NTFS volume set"},
	{MBR_PTYPE_NTFS,	"NTFS"},
	{-1,			"Unknown"},
};

static int get_mapping(struct mbr_partition *, int, int *, int *, int *,
			    unsigned long *);
static void convert_mbr_chs(int, int, int, u_int8_t *, u_int8_t *,
				 u_int8_t *, u_int32_t);

/*
 * get C/H/S geometry from user via menu interface and
 * store in globals.
 */
void
set_bios_geom(int cyl, int head, int sec)
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
disp_cur_geom(void)
{

	msg_display_add(MSG_realgeom, dlcyl, dlhead, dlsec);
	msg_display_add(MSG_biosgeom, bcyl, bhead, bsec);
}
#endif


/*
 * Then,  the partition stuff...
 */

static void
remove_old_partitions(uint start, int size)
{
	partinfo *p;
	uint end;

	if (start > 0)
		end = start + size;
	else {
		end = start;
		start = end + size;
	}

	if (end == 0)
		return;

	for (p = oldlabel; p < oldlabel + nelem(oldlabel); p++) {
		if (p->pi_offset >= end || p->pi_offset + p->pi_size <= start)
			continue;
		memset(p, 0, sizeof *p);
	}
}

static int
find_mbr_space(mbr_sector_t *mbr, uint *start, uint *size, int from, int ignore)
{
	int sz;
	int i;
	uint s, e;

    check_again:
	sz = dlsize - from;
	for (i = 0; i < NMBRPART; i++) {
		if (i == ignore)
			continue;
		s = mbr->mbr_parts[i].mbrp_start;
		e = s + mbr->mbr_parts[i].mbrp_size;
		if (s <= from && e > from) {
			from = e;
			goto check_again;
		}
		if (s > from && s - from < sz)
			sz = s - from;
	}
	if (sz == 0)
		return -1;
	if (start != NULL)
		*start = from;
	if (size != NULL)
		*size = sz;
	return 0;
}

static mbr_partition_t *
get_mbrp(mbr_info_t **mbrip, int opt)
{
	mbr_info_t *mbri = *mbrip;

	if (opt >= NMBRPART)
		for (opt -= NMBRPART - 1; opt; opt--)
			mbri = mbri->extended;

	*mbrip = mbri;
	return &mbri->mbr.mbr_parts[opt];
}

static int
set_mbr_type(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_info_t *ext;
	mbr_partition_t *mbrp;
	char *cp;
	int opt = mbri->opt;
	int type;
	int sz;
	int i;
	char numbuf[4];

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= NMBRPART)
		opt = 0;

	type = ent - m->opts;
	if (type == 0)
		return 1;
	type = part_ids[type - 1].id;
	while (type == -1) {
		snprintf(numbuf, sizeof numbuf, "%u", mbrp->mbrp_typ);
		msg_prompt_win(MSG_get_ptn_id, -1, 18, 0, 0,
			numbuf, numbuf, sizeof numbuf);
		type = strtoul(numbuf, &cp, 0);
		if (*cp != 0)
			type = -1;
	}

	if (type == mbrp->mbrp_typ)
		/* type not changed... */
		return 1;

	remove_old_partitions(mbri->sector + mbrp->mbrp_start, mbrp->mbrp_size);
	mbri->last_mounted[opt < NMBRPART ? opt : 0] = NULL;

	if (MBR_IS_EXTENDED(mbrp->mbrp_typ)) {
		/* deleting extended partition.... */
		if (mbri->sector || mbri->extended->extended)
			/* We should have stopped this happening... */
			return 0;
		free(mbri->extended);
		mbri->extended = NULL;
	}

	if (type == 0) {
		mbrp->mbrp_typ = type;
		/* Deleting partition */
#ifdef BOOTSEL
		if (ombri->bootsec == mbri->sector + mbrp->mbrp_start)
			ombri->bootsec = 0;
				
		memset(mbri->nametab[opt], 0, sizeof mbri->nametab[opt]);
#endif
		if (mbri->sector == 0) {
			memset(mbrp, 0, sizeof *mbrp);
			return 1;
		}

		/* Merge with previous and next free areas */
		ext = mbri->prev_ext;
		if (ext != NULL && ext->mbr.mbr_parts[0].mbrp_typ == 0) {
			mbri = ext;
			ombri->opt--;
		}
		while ((ext = mbri->extended)) {
			if (ext->mbr.mbr_parts[0].mbrp_typ != 0)
				break;
			sz = ext->mbr.mbr_parts[0].mbrp_start +
						ext->mbr.mbr_parts[0].mbrp_size;
			/* Increase size of our (empty) partition */
			mbri->mbr.mbr_parts[0].mbrp_size += sz;
			/* Make us describe the next partition */
			mbri->mbr.mbr_parts[1] = ext->mbr.mbr_parts[1];
			/* fix list of extended partitions */
			mbri->extended = ext->extended;
			free(ext);
			if (ext->extended != NULL)
				ext->extended->prev_ext = mbri;
			/* Make previous size cover all our ptn */
			ext = mbri->prev_ext;
			if (ext != NULL)
				ext->mbr.mbr_parts[1].mbrp_size += sz;
		}
		return 1;
	}

	if (mbrp->mbrp_start == 0) {
		/* Find first chunk of space... */
		/* Must be in the main partition */
		if (mbri->sector != 0)
			/* shouldn't be possible to have null start... */
			return 0;
		if (find_mbr_space(&mbri->mbr, &mbrp->mbrp_start,
				   &mbrp->mbrp_size, bsec, -1) != 0)
			/* no space */
			return 0;
		/* If there isn't an active partition mark this one active */
		if (!MBR_IS_EXTENDED(type)) {
			for (i = 0; i < NMBRPART; i++)
				if (mbri->mbr.mbr_parts[i].mbrp_flag != 0)
					break;
			if (i == NMBRPART)
				mbrp->mbrp_flag = MBR_FLAGS_ACTIVE;
		}
	}

	if (MBR_IS_EXTENDED(type)) {
		if (mbri->sector != 0)
			/* Can't set extended partition in an extended one */
			return 0;
		if (mbri->extended)
			/* Can't have two extended partitions */
			return 0;
		/* Create new extended partition */
		ext = calloc(1, sizeof *mbri->extended);
		if (!ext)
			return 0;
		mbri->extended = ext;
		ext->sector = mbrp->mbrp_start;
		ext->mbr.mbr_parts[0].mbrp_start = bsec;
		ext->mbr.mbr_parts[0].mbrp_size = mbrp->mbrp_size - bsec;
	}
	mbrp->mbrp_typ = type;

	return 1;
}

static void
set_type_label(menudesc *m, int opt, void *arg)
{

	if (opt == 0) {
		wprintw(m->mw, msg_string(MSG_Dont_change));
		return;
	}
	if (opt == 1) {
		wprintw(m->mw, msg_string(MSG_Delete_partition));
		return;
	}
	if (part_ids[opt - 1].id == -1) {
		wprintw(m->mw, msg_string(MSG_Other_kind));
		return;
	}
	wprintw(m->mw, part_ids[opt - 1].name);
}

static int
edit_mbr_type(menudesc *m, menu_ent *ent, void *arg)
{
	static menu_ent type_opts[1 + nelem(part_ids)];
	static int type_menu = -1;
	int i;

	if (type_menu == -1) {
		for (i = 0; i < nelem(type_opts); i++) {
			type_opts[i].opt_menu = OPT_NOMENU;
			type_opts[i].opt_action = set_mbr_type;
		}
		type_menu = new_menu(NULL, type_opts, nelem(type_opts),
			15, 12, 0, 30,
			MC_SCROLL | MC_NOEXITOPT | MC_NOCLEAR,
			NULL, set_type_label, NULL,
			NULL, NULL);
	}

	if (type_menu != -1)
		process_menu(type_menu, arg);

	return 0;
}

static int
edit_mbr_start(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ext;
	mbr_partition_t *mbrp;
	int opt = mbri->opt;
	uint start, sz;
	uint new_r, new, limit, dflt_r;
	int delta;
	const char *errmsg;
	char *cp;
	struct {
		uint	start;
		uint	start_r;
		uint	limit;
	} freespace[NMBRPART];
	int spaces;
	int i;
	char prompt[NMBRPART * 60];
	int len;
	char numbuf[12];

	if (opt >= NMBRPART)
		/* should not be able to get here... */
		return 1;

	mbrp = mbri->mbr.mbr_parts + opt;
	/* locate the start of all free areas */
	spaces = 0;
	for (start = bsec, i = 0; i < NMBRPART; start += sz, i++) {
		if (find_mbr_space(&mbri->mbr, &start, &sz, start, opt))
			break;
		if (MBR_IS_EXTENDED(mbrp->mbrp_typ)) {
			/* Only want the area that contains this partition */
			if (mbrp->mbrp_start < start ||
			    mbrp->mbrp_start >= start + sz)
				continue;
			i = NMBRPART - 1;
		}
		freespace[spaces].start = start;
		freespace[spaces].start_r = start / sizemult;
		freespace[spaces].limit = start + sz;
		if (++spaces >= sizeof freespace)
			/* shouldn't happen... */
			break;
	}

	len = 0;
	for (i = 0; i < spaces; i++) {
		len += snprintf(prompt + len, sizeof prompt - len,
		    msg_string(MSG_ptn_starts),
		    freespace[i].start_r,
		    freespace[i].limit / sizemult, multname,
		    freespace[i].limit / sizemult - freespace[i].start_r,
		    multname);
		if (len >= sizeof prompt)
			break;
	}

	dflt_r = mbrp->mbrp_start / sizemult;
	errmsg = "";
	for (;;) {
		snprintf(numbuf, sizeof numbuf, "%d", dflt_r);
		msg_prompt_win(MSG_get_ptn_start, -1, 18, 60, spaces + 3,
			numbuf, numbuf, sizeof numbuf,
			prompt, msg_string(errmsg), multname);
		new_r = strtoul(numbuf, &cp, 0);
		if (*cp != 0) {
			errmsg = MSG_Invalid_numeric;
			continue;
		}
		if (new_r == dflt_r)
			/* Unchanged */
			return 0;
		new = new_r * sizemult;
		for (i = 0; i < spaces; i++) {
			if (new_r == freespace[i].start_r) {
				new = freespace[i].start;
				break;
			}
			if (new >= freespace[i].start &&
			    new < freespace[i].limit)
				break;
		}
		if (i >= spaces) {
			errmsg = MSG_Space_allocated;
			continue;
		}
		limit = freespace[i].limit;
		if (new > mbrp->mbrp_start &&
		    MBR_IS_EXTENDED(mbrp->mbrp_typ) &&
		    (mbri->extended->mbr.mbr_parts[0].mbrp_typ != 0 ||
			    mbri->extended->mbr.mbr_parts[0].mbrp_size <
						new - mbrp->mbrp_start)) {
			errmsg = MSG_Space_allocated;
			continue;
		}
		break;
	}

	if (new < mbrp->mbrp_start + mbrp->mbrp_size &&
	    limit > mbrp->mbrp_start)
		/* Keep end of partition in the same place */
		limit = mbrp->mbrp_start + mbrp->mbrp_size;

	if (MBR_IS_EXTENDED(mbrp->mbrp_typ)) {
		delta = new - mbrp->mbrp_start;
		ext = mbri->extended;
		if (ext->mbr.mbr_parts[0].mbrp_typ != 0) {
			/* allocate an extended ptn for the free item */
			ext = calloc(1, sizeof *ext);
			if (!ext)
				return 0;
			ext->sector = mbrp->mbrp_start;
			ext->extended = mbri->extended;
			mbri->extended->prev_ext = ext;
			mbri->extended = ext;
			ext->mbr.mbr_parts[0].mbrp_start = bsec;
			ext->mbr.mbr_parts[0].mbrp_size = -bsec;
			ext->mbr.mbr_parts[1].mbrp_typ = MBR_PTYPE_EXT;
			ext->mbr.mbr_parts[1].mbrp_start = 0;
			ext->mbr.mbr_parts[1].mbrp_size =
				ext->extended->mbr.mbr_parts[0].mbrp_start +
				ext->extended->mbr.mbr_parts[0].mbrp_size;
		}
		/* adjust size of first free item */
		ext->mbr.mbr_parts[0].mbrp_size -= delta;
		ext->sector += delta;
		/* and the link of all extended partitions */
		do
			if (ext->extended)
				ext->mbr.mbr_parts[1].mbrp_start -= delta;
		while ((ext = ext->extended));
		remove_old_partitions(mbri->sector + mbrp->mbrp_start, delta);
	} else
		remove_old_partitions(mbri->sector + mbrp->mbrp_start, mbrp->mbrp_size);

	/* finally set partition base and size */
	mbrp->mbrp_start = new;
	mbrp->mbrp_size = limit - new;
	mbri->last_mounted[opt] = NULL;

	return 0;
}

static int
edit_mbr_size(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_info_t *ext;
	mbr_partition_t *mbrp;
	int opt = mbri->opt;
	uint start, max, max_r, dflt, dflt_r, new;
	uint freespace;
	int delta;
	char numbuf[12];
	char *cp;
	const char *errmsg;

	mbrp = get_mbrp(&mbri, opt);
	dflt = mbrp->mbrp_size;
	if (opt < NMBRPART) {
		max = 0;
		find_mbr_space(&mbri->mbr, &start, &max, mbrp->mbrp_start, opt);
		if (start != mbrp->mbrp_start)
			return 0;
		if (dflt == 0)
			dflt = max;
	} else {
		ext = mbri->extended;
		max = dflt;
		if (ext != NULL && ext->mbr.mbr_parts[0].mbrp_typ == 0) {
			/* Easier to merge now and split later... */
			if (ext->extended)
				ext->extended->prev_ext = mbri;
			mbri->extended = ext->extended;
			if (mbri->prev_ext)
				mbri->prev_ext->mbr.mbr_parts[1].mbrp_size
					+= mbri->mbr.mbr_parts[1].mbrp_size;
			mbrp->mbrp_size += mbri->mbr.mbr_parts[1].mbrp_size;
			max += mbri->mbr.mbr_parts[1].mbrp_size;
			mbri->mbr.mbr_parts[1] = ext->mbr.mbr_parts[1];
			free(ext);
		}
	}

	start = mbri->sector + mbrp->mbrp_start;
	dflt_r = (start + dflt) / sizemult - start / sizemult;
	if (max == dflt)
		max_r = dflt_r;
	else
		max_r = max / sizemult;
	for (errmsg = "";;) {
		snprintf(numbuf, sizeof numbuf, "%d", dflt_r);
		msg_prompt_win(MSG_get_ptn_size, -1, 18, 0, 0,
			numbuf, numbuf, sizeof numbuf,
			msg_string(errmsg), max_r, multname);
		new = strtoul(numbuf, &cp, 0);
		if (*cp != 0) {
			errmsg = MSG_Invalid_numeric;
			continue;
		}
		if (new == 0 || new == max_r)
			new = max;
		else {
			if (new == dflt_r)
				new = dflt;
			else {
				/* Round end to cylinder boundary */
				if (sizemult != 1) {
					new = start + new * sizemult;
					new = ROUNDUP(new, bcylsize);
					new -= start;
				}
			}
		}
		if (new > max) {
			errmsg = MSG_Too_large;
			continue;
		}
		if (!MBR_IS_EXTENDED(mbrp->mbrp_typ) || new == dflt)
			break;
		/* Must keep extended list aligned */
		for (ext = mbri->extended; ext->extended; ext = ext->extended)
			continue;
		if ((new < dflt && (ext->mbr.mbr_parts[0].mbrp_typ != 0
			    || (mbrp->mbrp_start + new < ext->sector + bsec
				&& mbrp->mbrp_start + new != ext->sector)))
		    || (new > dflt && ext->mbr.mbr_parts[0].mbrp_typ != 0
							&& new < dflt + bsec)) {
			errmsg = MSG_Space_allocated;
			continue;
		}
		delta = new - dflt;
		if (ext->mbr.mbr_parts[0].mbrp_typ == 0) {
			/* adjust size of last item (free space) */
			if (mbrp->mbrp_start + new == ext->sector) {
				/* kill last extended ptn */
				ext = ext->prev_ext;
				free(ext->extended);
				ext->extended = NULL;
				memset(&ext->mbr.mbr_parts[1], 0,
					sizeof ext->mbr.mbr_parts[1]);
				break;
			}
			ext->mbr.mbr_parts[0].mbrp_size += delta;
			ext = ext->prev_ext;
			if (ext != NULL)
				ext->mbr.mbr_parts[1].mbrp_size += delta;
			break;
		}
		/* Joy of joys, we must allocate another extended ptn */
		mbri = ext;
		ext = calloc(1, sizeof *ext);
		if (!ext)
			return 0;
		mbri->extended = ext;
		ext->prev_ext = mbri;
		ext->mbr.mbr_parts[0].mbrp_start = bsec;
		ext->mbr.mbr_parts[0].mbrp_size = delta - bsec;
		ext->sector = mbri->sector + mbri->mbr.mbr_parts[0].mbrp_start
					   + mbri->mbr.mbr_parts[0].mbrp_size;
		mbri->mbr.mbr_parts[1].mbrp_start = ext->sector - ombri->sector;
		mbri->mbr.mbr_parts[1].mbrp_typ = MBR_PTYPE_EXT;
		mbri->mbr.mbr_parts[1].mbrp_size = delta;
		break;
	}

	if (opt >= NMBRPART && max - new <= bsec)
		/* round up if not enough space for a header */
		new = max;

	if (new != mbrp->mbrp_size) {
		mbri->last_mounted[opt < NMBRPART ? opt : 0] = NULL;
		remove_old_partitions(mbri->sector + mbrp->mbrp_start + mbrp->mbrp_size,
				      new - mbrp->mbrp_size);
	}

	mbrp->mbrp_size = new;
	if (opt < NMBRPART || new == max)
		return 0;

	/* Need to allocate an extra item for the free space */
	ext = calloc(1, sizeof *ext);
	if (!ext) {
		mbrp->mbrp_size = max;
		return 0;
	}
	/* Link into our extended chain */
	ext->extended = mbri->extended;
	mbri->extended = ext;
	ext->prev_ext = mbri;
	if (ext->extended != NULL)
		ext->extended->prev_ext = ext;
	ext->mbr.mbr_parts[1] = mbri->mbr.mbr_parts[1];
	freespace = max - new;
	if (mbri->prev_ext != NULL)
		mbri->prev_ext->mbr.mbr_parts[1].mbrp_size -= freespace;

	ext->mbr.mbr_parts[0].mbrp_start = bsec;
	ext->mbr.mbr_parts[0].mbrp_size = freespace - bsec;

	ext->sector = mbri->sector + mbri->mbr.mbr_parts[0].mbrp_start
				   + mbri->mbr.mbr_parts[0].mbrp_size;
	mbri->mbr.mbr_parts[1].mbrp_start = ext->sector - ombri->sector;
	mbri->mbr.mbr_parts[1].mbrp_typ = MBR_PTYPE_EXT;
	mbri->mbr.mbr_parts[1].mbrp_size = freespace;

	return 0;
}

static int
edit_mbr_active(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	int i;
	uint8_t *fl;

	if (mbri->opt >= NMBRPART)
		/* sanity */
		return 0;

	/* Invert active flag */
	fl = &mbri->mbr.mbr_parts[mbri->opt].mbrp_flag;
	if (*fl == MBR_FLAGS_ACTIVE) {
		*fl = 0;
		return 0;
	}
		
	/* Ensure there is at most one active partition */
	for (i = 0; i < NMBRPART; i++)
		mbri->mbr.mbr_parts[i].mbrp_flag = 0;
	*fl = MBR_FLAGS_ACTIVE;

	return 0;
}

static int
edit_mbr_install(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_partition_t *mbrp;
	int opt = mbri->opt;
	uint start;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= NMBRPART)
		opt = 0;

	start = mbri->sector + mbrp->mbrp_start;
	if (start == ombri->install)
		ombri->install = 0;
	else
		ombri->install = start;
	return 0;
}

#ifdef BOOTSEL
static int
edit_mbr_bootmenu(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_partition_t *mbrp;
	int opt = mbri->opt;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= NMBRPART)
		opt = 0;

	msg_prompt_win(/* XXX */ "bootmenu", -1, 18, 0, 0,
		mbri->nametab[opt],
		mbri->nametab[opt], sizeof mbri->nametab[opt]);
	if (mbri->nametab[opt][0] == ' ')
		mbri->nametab[opt][0] = 0;
	if (mbri->nametab[opt][0] == 0) {
		if (ombri->bootsec == mbri->sector + mbrp->mbrp_start)
			ombri->bootsec = 0;
		return 0;
	}
	return 0;
}

static int
edit_mbr_bootdefault(menudesc *m, menu_ent *ent, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_partition_t *mbrp;

	mbrp = get_mbrp(&mbri, mbri->opt);

	ombri->bootsec = mbri->sector + mbrp->mbrp_start;
	return 0;
}
#endif

static void set_ptn_label(menudesc *m, int line, void *arg);
static void set_ptn_header(menudesc *m, void *arg);

static int
edit_mbr_entry(menudesc *m, menu_ent *opt, void *arg)
{
	mbr_info_t *mbri = arg;
	static int ptn_menu = -1;

	static menu_ent ptn_opts[] = {
#define PTN_OPT_TYPE		0
		{NULL, OPT_NOMENU, 0, edit_mbr_type},
#define PTN_OPT_START		1
		{NULL, OPT_NOMENU, 0, edit_mbr_start},
#define PTN_OPT_SIZE		2
		{NULL, OPT_NOMENU, 0, edit_mbr_size},
#define PTN_OPT_END		3
		{NULL, OPT_NOMENU, OPT_IGNORE, NULL},	/* display end */
#define PTN_OPT_ACTIVE		4
		{NULL, OPT_NOMENU, 0, edit_mbr_active},
#define PTN_OPT_INSTALL		5
		{NULL, OPT_NOMENU, 0, edit_mbr_install},
#ifdef BOOTSEL
#define PTN_OPT_BOOTMENU	6
		{NULL, OPT_NOMENU, 0, edit_mbr_bootmenu},
#define PTN_OPT_BOOTDEFAULT	7
		{NULL, OPT_NOMENU, 0, edit_mbr_bootdefault},
#endif
		{MSG_askunits, MENU_sizechoice, OPT_SUB, NULL},
	};

	if (ptn_menu == -1)
		ptn_menu = new_menu(NULL, ptn_opts, nelem(ptn_opts),
			15, 6, 0, 50,
			MC_SCROLL | MC_NOCLEAR,
			set_ptn_header, set_ptn_label, NULL,
			NULL, MSG_Partition_OK);
	if (ptn_menu == -1)
		return 1;

	mbri->opt = opt - m->opts;
	process_menu(ptn_menu, mbri);
	return 0;
}

static void
set_ptn_label(menudesc *m, int line, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_partition_t *mbrp;
	int opt;
	static const char *yes, *no;

	if (yes == NULL) {
		yes = msg_string(MSG_Yes);
		no = msg_string(MSG_No);
	}

	opt = mbri->opt;
	mbrp = get_mbrp(&mbri, opt);
	if (opt >= NMBRPART)
		opt = 0;

	switch (line) {
	case PTN_OPT_TYPE:
		wprintw(m->mw, msg_string(MSG_ptn_type),
			get_partname(mbrp->mbrp_typ));
		break;
	case PTN_OPT_START:
		wprintw(m->mw, msg_string(MSG_ptn_start),
		    (mbri->sector + mbrp->mbrp_start) / sizemult, multname);
		break;
	case PTN_OPT_SIZE:
		wprintw(m->mw, msg_string(MSG_ptn_size),
		    (mbri->sector + mbrp->mbrp_start + mbrp->mbrp_size) /
			    sizemult -
		    (mbri->sector + mbrp->mbrp_start) / sizemult, multname);
		break;
	case PTN_OPT_END:
		wprintw(m->mw, msg_string(MSG_ptn_end),
		    (mbri->sector + mbrp->mbrp_start + mbrp->mbrp_size) /
			    sizemult, multname);
		break;
	case PTN_OPT_ACTIVE:
		wprintw(m->mw, msg_string(MSG_ptn_active),
		    mbrp->mbrp_flag == MBR_FLAGS_ACTIVE ? yes : no);
		break;
	case PTN_OPT_INSTALL:
		wprintw(m->mw, msg_string(MSG_ptn_install),
		    mbri->sector + mbrp->mbrp_start == ombri->install &&
			mbrp->mbrp_typ == MBR_PTYPE_NETBSD ? yes : no);
		break;
#ifdef BOOTSEL
	case PTN_OPT_BOOTMENU:
		wprintw(m->mw, msg_string(MSG_bootmenu), mbri->nametab[opt]);
		break;
	case PTN_OPT_BOOTDEFAULT:
		wprintw(m->mw, msg_string(MSG_boot_dflt),
		    ombri->bootsec == mbri->sector + mbrp->mbrp_start ? yes
								      : no);
		break;
#endif
	}

}

static void
set_ptn_header(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_partition_t *mbrp;
	int opt = mbri->opt;
	int typ;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= NMBRPART)
		opt = 0;
	typ = mbrp->mbrp_typ;

#define DISABLE(opt,cond) \
	if (cond) \
		m->opts[opt].opt_flags |= OPT_IGNORE; \
	else \
		m->opts[opt].opt_flags &= ~OPT_IGNORE;

	/* Can't change type of the extended partition unless it is empty */
	DISABLE(PTN_OPT_TYPE, MBR_IS_EXTENDED(typ) &&
	    (mbri->extended->mbr.mbr_parts[0].mbrp_typ != 0 ||
					    mbri->extended->extended != NULL));

	/* It is unnecessary to be able to change the base of an extended ptn */
	DISABLE(PTN_OPT_START, mbri->sector || typ == 0);

	/* or the size of a free area */
	DISABLE(PTN_OPT_SIZE, typ == 0);

	/* Only 'normal' partitions can be 'Active' */
	DISABLE(PTN_OPT_ACTIVE, mbri->sector != 0 || MBR_IS_EXTENDED(typ) || typ == 0);

	/* Can only install into NetBSD partition */
	DISABLE(PTN_OPT_INSTALL, typ != MBR_PTYPE_NETBSD);

#ifdef BOOTSEL
	/* The extended partition isn't bootable */
	DISABLE(PTN_OPT_BOOTMENU, MBR_IS_EXTENDED(typ) || typ == 0);

	if (typ == 0)
		mbri->nametab[opt][0] = 0;

	/* Only partitions with bootmenu names can be made the default */
	DISABLE(PTN_OPT_BOOTDEFAULT, mbri->nametab[opt][0] == 0);
#endif
#undef DISABLE
}

static void
set_mbr_label(menudesc *m, int opt, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_partition_t *mbrp;
	uint rstart, rend;
	const char *name, *cp, *mounted;
	int len;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= NMBRPART)
		opt = 0;

	if (mbrp->mbrp_typ == 0 && mbri->sector == 0) {
		len = snprintf(0, 0, msg_string(MSG_part_row_used), 0, 0, 0);
		wprintw(m->mw, "%*s", len, "");
	} else {
		rstart = mbri->sector + mbrp->mbrp_start;
		rend = (rstart + mbrp->mbrp_size) / sizemult;
		rstart = rstart / sizemult;
		wprintw(m->mw, msg_string(MSG_part_row_used),
		    rstart, rend - rstart,
		    mbrp->mbrp_flag == MBR_FLAGS_ACTIVE ? 'a' : ' ',
#ifdef BOOTSEL
		    ombri->bootsec == mbri->sector + mbrp->mbrp_start ? 'b' :
#endif
			' ',
		    mbri->sector + mbrp->mbrp_start == ombri->install &&
			mbrp->mbrp_typ == MBR_PTYPE_NETBSD ? 'I' : ' ');
	}
	name = get_partname(mbrp->mbrp_typ);
	mounted = mbri->last_mounted[opt];
	len = strlen(name);
	cp = strchr(name, ',');
	if (cp != NULL)
		len = cp - name;
	if (mounted && *mounted != 0) {
		wprintw(m->mw, " %*s (%s)", len, name, mounted);
	} else
		wprintw(m->mw, " %.*s", len, name);
#ifdef BOOTSEL
	if (mbri->nametab[opt][0] != 0) {
		int x, y;
		if (opt >= NMBRPART)
			opt = 0;
		getyx(m->mw, y, x);
		if (x > 52) {
			x = 52;
			wmove(m->mw, y, x);
		}
		wprintw(m->mw, "%*s %s", 53 - x, "", mbri->nametab[opt]);
	}
#endif
}

static void
set_mbr_header(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	static menu_ent *opts;
	static int num_opts;
	mbr_info_t *ext;
	menu_ent *op;
	int i;
	int left;

	msg_display(MSG_editparttable);

	msg_table_add(MSG_part_header, dlsize/sizemult, multname, multname,
	    multname, multname);

	if (num_opts == 0) {
		num_opts = 6;
		opts = malloc(6 * sizeof *opts);
		if (opts == NULL) {
			m->numopts = 0;
			return;
		}
	}

	/* First four items are the main partitions */
	for (op = opts, i = 0; i < NMBRPART; op++, i++) {
		op->opt_name = NULL;
		op->opt_menu = OPT_NOMENU;
		op->opt_flags = OPT_SUB;
		op->opt_action = edit_mbr_entry;
	}
	left = num_opts - NMBRPART;

	/* Followed by the extended partitions */
	for (ext = mbri->extended; ext; left--, op++, ext = ext->extended) {
		if (left <= 1) {
			menu_ent *new = realloc(opts,
						(num_opts + 4) * sizeof *opts);
			if (new == NULL)
				break;
			num_opts += 4;
			left += 4;
			op = new + (op - opts);
			opts = new;
		}
		op->opt_name = NULL;
		op->opt_menu = OPT_NOMENU;
		op->opt_flags = 0;
		op->opt_action = edit_mbr_entry;
	}

	/* and unit changer */
	op->opt_name = MSG_askunits;
	op->opt_menu = MENU_sizechoice;
	op->opt_flags = OPT_SUB;
	op->opt_action = NULL;
	op++;

	m->opts = opts;
	m->numopts = op - opts;
}

/*
 * Let user change incore Master Boot Record partitions via menu.
 */
int
edit_mbr(mbr_info_t *mbri)
{
	mbr_sector_t *mbr = &mbri->mbr;
	mbr_info_t *ext;
	struct mbr_partition *part;
	int i, j;
	int usefull;
	int mbr_menu;
	int activepart;
	int numbsd;
	uint bsdstart, bsdsize;
	uint start;

	/* Ask full/part */

	part = &mbr->mbr_parts[0];
	msg_display(MSG_fullpart, diskdev);
	process_menu(MENU_fullpart, &usefull);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		/* Count nonempty, non-BSD partitions. */
		numbsd = 0;
		for (i = 0; i < NMBRPART; i++) {
			j = part[i].mbrp_typ;
			if (j == 0)
				continue;
			numbsd++;
			if (j != MBR_PTYPE_NETBSD)
				numbsd++;
		}

		/* Ask if we really want to blow away non-NetBSD stuff */
		if (numbsd > 1) {
			msg_display(MSG_ovrwrite);
			process_menu(MENU_noyes, NULL);
			if (!yesno) {
				if (logging)
					(void)fprintf(logfp, "User answered no to destroy other data, aborting.\n");
				return 0;
			}
		}

		/* Set the partition information for full disk usage. */
		while ((ext = mbri->extended)) {
			mbri->extended = ext->extended;
			free(ext);
		}
		memset(part, 0, NMBRPART * sizeof *part);
#ifdef BOOTSEL
		memset(&mbri->nametab, 0, sizeof mbri->nametab);
#endif
		part[0].mbrp_typ = MBR_PTYPE_NETBSD;
		part[0].mbrp_size = dlsize - bsec;
		part[0].mbrp_start = bsec;
		part[0].mbrp_flag = MBR_FLAGS_ACTIVE;

		ptstart = bsec;
		ptsize = dlsize - bsec;
		return 1;
	}

	mbr_menu = new_menu(NULL, NULL, 16, 0, 7, 15, 70,
			MC_NOBOX | MC_SCROLL | MC_NOCLEAR,
			set_mbr_header, set_mbr_label, NULL,
			NULL, MSG_Partition_table_ok);
	if (mbr_menu == -1)
		return 0;

	/* Ask for sizes, which partitions, ... */
	ask_sizemult(bcylsize);

	for (;;) {
		ptstart = 0;
		ptsize = 0;
		process_menu(mbr_menu, mbri);

		activepart = 0;
		bsdstart = 0;
		bsdsize = 0;
		for (ext = mbri; ext; ext = ext->extended) {
			part = ext->mbr.mbr_parts;
			for (i = 0; i < NMBRPART; part++, i++) {
				if (part->mbrp_flag != 0)
					activepart = 1;
				if (part->mbrp_typ != MBR_PTYPE_NETBSD)
					continue;
				start = ext->sector + part->mbrp_start;
				if (start == mbri->install) {
					ptstart = mbri->install;
					ptsize = part->mbrp_size;
				}
				if (bsdstart != 0)
					bsdstart = ~0;
				else {
					bsdstart = start;
					bsdsize = part->mbrp_size;
				}
			}
		}

		/* Install in only netbsd partition if none tagged */
		if (ptstart == 0 && bsdstart != ~0) {
			ptstart = bsdstart;
			ptsize = bsdsize;
		}

		if (ptstart == 0) {
			if (bsdstart == 0)
				msg_display(MSG_nobsdpart);
			else
				msg_display(MSG_multbsdpart, 0);
			msg_display_add(MSG_reeditpart, 0);
			process_menu(MENU_yesno, NULL);
			if (!yesno)
				return 0;
			continue;
		}

		if (activepart == 0) {
			msg_display(MSG_noactivepart);
			process_menu(MENU_yesno, NULL);
			if (yesno)
				continue;
		}
		break;
	}

	free_menu(mbr_menu);

	return 1;
}

char *
get_partname(int typ)
{
	int j;
	static char unknown[32];

	for (j = 0; part_ids[j].id != -1; j++)
		if (part_ids[j].id == typ)
			return part_ids[j].name;

	snprintf(unknown, sizeof unknown, "Unknown (%d)", typ);
	return unknown;
}

int
read_mbr(const char *disk, mbr_info_t *mbri)
{
	struct mbr_partition *mbrp;
	mbr_sector_t *mbr = &mbri->mbr;
	mbr_info_t *ext = NULL;
	char diskpath[MAXPATHLEN];
	int fd, i;
	uint32_t ext_base = 0, next_ext = 0, ext_size = 0;
	int rval = -1;
#ifdef BOOTSEL
	mbr_info_t *ombri = mbri;
	int bootkey;
#endif

	memset(mbri, 0, sizeof *mbri);

	/* Open the disk. */
	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		goto bad_mbr;

	for (;;) {
		if (pread(fd, mbr, sizeof *mbr,
		    (ext_base + next_ext) * (off_t)MBR_SECSIZE) < sizeof *mbr)
			break;

		if (!valid_mbr(mbr))
			break;

		mbrp = &mbr->mbr_parts[0];
		if (ext_base != 0) {
			/* sanity check extended chain */
			if (MBR_IS_EXTENDED(mbrp[0].mbrp_typ))
				break;
			if (mbrp[1].mbrp_typ != 0 &&
			    !MBR_IS_EXTENDED(mbrp[1].mbrp_typ))
				break;
			if (mbrp[2].mbrp_typ != 0 || mbrp[3].mbrp_typ != 0)
				break;
			mbri->extended = ext;
			ext->prev_ext = ext_base != 0 ? mbri : NULL;
			ext->extended = NULL;
			mbri = ext;
			ext = NULL;
		}
#if BOOTSEL
		else {
			if (mbr->mbr_bootsel.mbrb_magic == htole16(MBR_MAGIC))
				bootkey = mbr->mbr_bootsel.mbrb_defkey;
			else
				bootkey = 0;
			bootkey -= SCAN_1;
		}
		if (mbr->mbr_bootsel.mbrb_magic == htole16(MBR_MAGIC))
			memcpy(mbri->nametab, mbr->mbr_bootsel.mbrb_nametab,
				sizeof mbri->nametab);
#endif
		mbri->sector = next_ext + ext_base;
		next_ext = 0;
		rval = 0;
		for (i = 0; i < NMBRPART; mbrp++, i++) {
			if (mbrp->mbrp_typ == 0) {
				/* type is unused, discard scum */
				memset(mbrp, 0, sizeof *mbrp);
				continue;
			}
			mbrp->mbrp_start = le32toh(mbrp->mbrp_start);
			mbrp->mbrp_size = le32toh(mbrp->mbrp_size);
			if (MBR_IS_EXTENDED(mbrp->mbrp_typ)) {
				next_ext = mbrp->mbrp_start;
				if (ext_base == 0)
					ext_size = mbrp->mbrp_size;
			} else
				mbri->last_mounted[i] = strdup(get_last_mounted(
					fd, mbri->sector + mbrp->mbrp_start));
#if BOOTSEL
			if (mbri->nametab[i][0] != 0 && bootkey-- == 0)
				ombri->bootsec = mbri->sector +
							mbrp->mbrp_start;
#endif
		}

		if (ext_base != 0) {
			/* Is there a gap before the next partition? */
			unsigned int limit = next_ext;
			unsigned int base;
			if (limit == 0)
				limit = ext_size;
			mbrp -= NMBRPART;
			base =mbri->sector + mbrp->mbrp_start + mbrp->mbrp_size;
			if (mbrp->mbrp_typ != 0 && ext_base + limit != base) {
				/* Mock up an extry for the space */
				ext = calloc(1, sizeof *ext);
				if (!ext)
					break;
				ext->sector = base;
				ext->mbr.mbr_signature = htole16(MBR_MAGIC);
				ext->mbr.mbr_parts[1] = mbrp[1];
				mbrp[1].mbrp_typ = MBR_PTYPE_EXT;
				mbrp[1].mbrp_start = base - ext_base;
				mbrp[1].mbrp_size = limit - mbrp[1].mbrp_start;
				mbri->extended = ext;
				ext->prev_ext = mbri;
				ext->extended = NULL;
				mbri = ext;
				ext = NULL;
			}
		}

		if (next_ext == 0 || ext_base + next_ext <= mbri->sector)
			break;
		if (ext_base == 0) {
			ext_base = next_ext;
			next_ext = 0;
		}
		ext = malloc(sizeof *ext);
		if (!ext)
			break;
		mbr = &ext->mbr;
	}

    bad_mbr:
	free(ext);
	if (fd >= 0)
		close(fd);
	if (rval == -1) {
		memset(&mbr->mbr_parts, 0, sizeof mbr->mbr_parts);
		mbr->mbr_signature = htole16(MBR_MAGIC);
	}
	return rval;
}

int
write_mbr(const char *disk, mbr_info_t *mbri, int convert)
{
	char diskpath[MAXPATHLEN];
	int fd, i, ret = 0;
	struct mbr_partition *mbrp;
	u_int32_t pstart, psize;
	mbr_sector_t *mbr;
	mbr_info_t *ext;
#ifdef BOOTSEL
	int netbsd_bootcode;
	int8_t key = SCAN_1;

	if (mbri->mbr.mbr_bootsel.mbrb_magic == htole16(MBR_MAGIC)) {
		netbsd_bootcode = 1;
		mbri->mbr.mbr_bootsel.mbrb_defkey = SCAN_ENTER;
	} else
		netbsd_bootcode = 0;
#endif

	/* Open the disk. */
	fd = opendisk(disk, O_WRONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return -1;

	for (ext = mbri; ext != NULL; ext = ext->extended) {
		mbr = &ext->mbr;
#ifdef BOOTSEL
		if (netbsd_bootcode) {
			mbri->mbr.mbr_bootsel.mbrb_magic = htole16(MBR_MAGIC);
			memcpy(&mbr->mbr_bootsel.mbrb_nametab, &ext->nametab,
			    sizeof mbr->mbr_bootsel.mbrb_nametab);
		}
#endif
		mbrp = &mbr->mbr_parts[0];
		for (i = 0; i < NMBRPART; i++) {
			if (mbrp[i].mbrp_start == 0 && mbrp[i].mbrp_size == 0) {
				mbrp[i].mbrp_scyl = 0;
				mbrp[i].mbrp_shd = 0;
				mbrp[i].mbrp_ssect = 0;
				mbrp[i].mbrp_ecyl = 0;
				mbrp[i].mbrp_ehd = 0;
				mbrp[i].mbrp_esect = 0;
				continue;
			}
			pstart = mbrp[i].mbrp_start;
			psize = mbrp[i].mbrp_size;
			mbrp[i].mbrp_start = htole32(pstart);
			mbrp[i].mbrp_size = htole32(psize);
			if (convert) {
				convert_mbr_chs(bcyl, bhead, bsec,
				    &mbrp[i].mbrp_scyl, &mbrp[i].mbrp_shd,
				    &mbrp[i].mbrp_ssect, pstart);
				convert_mbr_chs(bcyl, bhead, bsec,
				    &mbrp[i].mbrp_ecyl, &mbrp[i].mbrp_ehd,
				    &mbrp[i].mbrp_esect, pstart + psize - 1);
			}
#ifdef BOOTSEL
			if (netbsd_bootcode && ext->nametab[i][0] != 0) {
				if (ext->sector + pstart == mbri->bootsec)
					mbri->mbr.mbr_bootsel.mbrb_defkey = key;
				key++;
			}
#endif
		}

		mbr->mbr_signature = htole16(MBR_MAGIC);
		if (ext->sector != 0 && pwrite(fd, mbr, sizeof *mbr,
		    ext->sector * (off_t)MBR_SECSIZE) < 0)
			ret = -1;
	}

	if (pwrite(fd, &mbri->mbr, sizeof mbri->mbr, 0) < 0)
		ret = -1;

	(void)close(fd);
	return ret;
}

int
valid_mbr(mbr_sector_t *mbr)
{

	return (le16toh(mbr->mbr_signature) == MBR_MAGIC);
}

static void
convert_mbr_chs(int cyl, int head, int sec,
		u_int8_t *cylp, u_int8_t *headp, u_int8_t *secp,
		u_int32_t relsecs)
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

#define MAXCYL		1023    /* Possibly 1024 */
#define MAXHEAD		255     /* Possibly 256 */
#define MAXSECTOR	63

int
guess_biosgeom_from_mbr(mbr_info_t *mbri, int *cyl, int *head, int *sec)
{
	mbr_sector_t *mbr = &mbri->mbr;
	struct mbr_partition *parts = &mbr->mbr_parts[0];
	int xcylinders, xheads, xsectors, i, j;
	int c1, h1, s1, c2, h2, s2;
	unsigned long a1, a2;
	uint64_t num, denom;

	/*
	 * The physical parameters may be invalid as bios geometry.
	 * If we cannot determine the actual bios geometry, we are
	 * better off picking a likely 'faked' geometry than leaving
	 * the invalid physical one.
	 */

	xcylinders = dlcyl;
	xheads = dlhead;
	xsectors = dlsec;
	if (xcylinders > MAXCYL || xheads > MAXHEAD || xsectors > MAXSECTOR) {
		xsectors = MAXSECTOR;
		xheads = MAXHEAD;
		xcylinders = disk->dd_totsec / (MAXSECTOR * MAXHEAD);
		if (xcylinders > MAXCYL)
			xcylinders = MAXCYL;
	}
	*cyl = xcylinders;
	*head = xheads;
	*sec = xsectors;

	xheads = -1;

	/* Try to deduce the number of heads from two different mappings. */
	for (i = 0; i < NMBRPART * 2 - 1; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		for (j = i + 1; j < NMBRPART * 2; j++) {
			if (get_mapping(parts, j, &c2, &h2, &s2, &a2) < 0)
				continue;
			a1 -= s1;
			a2 -= s2;
			num = (uint64_t)h1 * a2 - (quad_t)h2 * a1;
			denom = (uint64_t)c2 * a1 - (quad_t)c1 * a2;
			if (denom != 0 && num % denom == 0) {
				xheads = (int)(num / denom);
				xsectors = a1 / (c1 * xheads + h1);
				break;
			}
		}
		if (xheads != -1)
			break;
	}

	if (xheads == -1)
		return -1;

	/*
	 * Estimate the number of cylinders.
	 * XXX relies on get_disks having been called.
	 */
	xcylinders = disk->dd_totsec / xheads / xsectors;

	/* Now verify consistency with each of the partition table entries.
	 * Be willing to shove cylinders up a little bit to make things work,
	 * but translation mismatches are fatal. */
	for (i = 0; i < NMBRPART * 2; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		if (xsectors * (c1 * xheads + h1) + s1 != a1)
			return -1;
		if (c1 >= xcylinders)
			xcylinders = c1 + 1;
	}

	/*
	 * Everything checks out.  Reset the geometry to use for further
	 * calculations.
	 */
	*cyl = MIN(xcylinders, MAXCYL);
	*head = xheads;
	*sec = xsectors;
	return 0;
}

static int
get_mapping(mbr_partition_t *parts, int i,
	    int *cylinder, int *head, int *sector, unsigned long *absolute)
{
	struct mbr_partition *apart = &parts[i / 2];

	if (apart->mbrp_typ == 0)
		return -1;
	if (i % 2 == 0) {
		*cylinder = MBR_PCYL(apart->mbrp_scyl, apart->mbrp_ssect);
		*head = apart->mbrp_shd;
		*sector = MBR_PSECT(apart->mbrp_ssect) - 1;
		*absolute = le32toh(apart->mbrp_start);
	} else {
		*cylinder = MBR_PCYL(apart->mbrp_ecyl, apart->mbrp_esect);
		*head = apart->mbrp_ehd;
		*sector = MBR_PSECT(apart->mbrp_esect) - 1;
		*absolute = le32toh(apart->mbrp_start)
			+ le32toh(apart->mbrp_size) - 1;
	}
	/* Sanity check the data against max values */
	if ((((*cylinder * MAXHEAD) + *head) * MAXSECTOR + *sector) < *absolute)
		/* cannot be a CHS mapping */
		return -1;

	return 0;
}
