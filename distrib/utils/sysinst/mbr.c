/*	$NetBSD: mbr.c,v 1.74 2006/05/16 00:16:59 dogcow Exp $ */

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

#define MAXCYL		1023    /* Possibly 1024 */
#define MAXHEAD		255     /* Possibly 256 */
#define MAXSECTOR	63

struct part_id {
	int id;
	const char *name;
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
	{MBR_PTYPE_PREP,	"PReP Boot"},
#ifdef MBR_PTYPE_SOLARIS
	{MBR_PTYPE_SOLARIS,	"Solaris"},
#endif
	{-1,			"Unknown"},
};

static int get_mapping(struct mbr_partition *, int, int *, int *, int *,
			    unsigned long *);
static void convert_mbr_chs(int, int, int, uint8_t *, uint8_t *,
				 uint8_t *, uint32_t);

/*
 * Notes on the extended partition editor.
 *
 * The extended partition structure is actually a singly linked list.
 * Each of the 'mbr' sectors can only contain 2 items, the first describes
 * a user partition (relative to that mbr sector), the second describes
 * the following partition (relative to the start of the extended partition).
 *
 * The 'start' sector for the user partition is always the size of one
 * track - very often 63.  The extended partitions themselves should
 * always start on a cylinder boundary using the BIOS geometry - often
 * 16065 sectors per cylinder.
 *
 * The disk is also always described in increasing sector order.
 *
 * During editing we keep the mbr sectors accurate (it might have been
 * easier to use absolute sector numbers though), and keep a copy of the
 * entire sector - to preserve any information any other OS has tried
 * to squirrel away in the (apparantly) unused space.
 *
 * For simplicity we add entries for unused space.  These should not
 * get written to the disk.
 *
 * Typical disk (with some small numbers):
 *
 *      0 -> a       63       37	dos
 *           b      100     1000	extended LBA (type 15)
 *
 *    100 -> a       63       37        user
 *           b      100      200	extended partiton (type 5)
 *
 *    200 -> a       63       37        user
 *           b      200      300	extended partiton (type 5)
 *
 *    300 -> a       63       37	user
 *           b        0        0        0 (end of chain)
 *
 * If there is a gap, the 'b' partition will start beyond the area
 * described by the 'a' partition.
 *
 * While writing this comment, I can't remember what happens is there
 * is space at the start of the extended partition.
 */

#ifndef debug_extended
#define dump_mbr(mbr, msg)
#else
void
dump_mbr(mbr_info_t *mbr, const char *msg)
{
	int i;

	fprintf(stderr, "%s: bsec %d\n", msg, bsec);
	do {
		fprintf(stderr, "%9p: %9d %9p %6.6s:",
		    mbr, mbr->sector, mbr->extended,
		    mbr->prev_ext, mbr->last_mounted);
		for (i = 0; i < 4; i++)
			fprintf(stderr, " %*d %9d %9d %9d,\n",
			    i ? 41 : 3,
			    mbr->mbr.mbr_parts[i].mbrp_type,
			    mbr->mbr.mbr_parts[i].mbrp_start,
			    mbr->mbr.mbr_parts[i].mbrp_size,
			    mbr->mbr.mbr_parts[i].mbrp_start +
				mbr->mbr.mbr_parts[i].mbrp_size);
	} while ((mbr = mbr->extended));
}
#endif

/*
 * get C/H/S geometry from user via menu interface and
 * store in globals.
 */
void
set_bios_geom(int cyl, int head, int sec)
{
	char res[80];

	msg_display_add(MSG_setbiosgeom);

	do {
		snprintf(res, 80, "%d", sec);
		msg_prompt_add(MSG_sectors, res, res, 80);
		bsec = atoi(res);
	} while (bsec <= 0 || bsec > 63);

	do {
		snprintf(res, 80, "%d", head);
		msg_prompt_add(MSG_heads, res, res, 80);
		bhead = atoi(res);
	} while (bhead <= 0 || bhead > 256);

	bcyl = dlsize / bsec / bhead;
	if (dlsize != bcyl * bsec * bhead)
		bcyl++;
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

/*
 * If we change the mbr partitioning, the we must remove any references
 * in the netbsd disklabel to the part we changed.
 */
static void
remove_old_partitions(uint start, int size)
{
	partinfo *p;
	uint end;

	/* Allow for size being -ve, get it right for very large partitions */
	end = start + size;
	if (end < start) {
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
find_mbr_space(struct mbr_sector *mbrs, uint *start, uint *size, int from, int ignore)
{
	int sz;
	int i;
	uint s, e;

    check_again:
	sz = dlsize - from;
	for (i = 0; i < MBR_PART_COUNT; i++) {
		if (i == ignore)
			continue;
		s = mbrs->mbr_parts[i].mbrp_start;
		e = s + mbrs->mbr_parts[i].mbrp_size;
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

static struct mbr_partition *
get_mbrp(mbr_info_t **mbrip, int opt)
{
	mbr_info_t *mbri = *mbrip;

	if (opt >= MBR_PART_COUNT)
		for (opt -= MBR_PART_COUNT - 1; opt; opt--)
			mbri = mbri->extended;

	*mbrip = mbri;
	return &mbri->mbr.mbr_parts[opt];
}

static int
err_msg_win(const char *errmsg)
{
	const char *cont;
	int l, l1;

	errmsg = msg_string(errmsg);
	cont = msg_string(MSG_Hit_enter_to_continue);

	l = strlen(errmsg);
	l1 = strlen(cont);
	if (l < l1)
		l = l1;

	msg_prompt_win("%s.\n%s", -1, 18, l + 5, 4,
			NULL, NULL, 1, errmsg, cont);
	return 0;
}

static int
set_mbr_type(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_info_t *ext;
	struct mbr_partition *mbrp;
	char *cp;
	int opt = mbri->opt;
	int type;
	u_int start, sz;
	int i;
	char numbuf[4];

	dump_mbr(ombri, "set type");

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= MBR_PART_COUNT)
		opt = 0;

	type = m->cursel;
	if (type == 0)
		return 1;
	type = part_ids[type - 1].id;
	while (type == -1) {
		snprintf(numbuf, sizeof numbuf, "%u", mbrp->mbrp_type);
		msg_prompt_win(MSG_get_ptn_id, -1, 18, 0, 0,
			numbuf, numbuf, sizeof numbuf);
		type = strtoul(numbuf, &cp, 0);
		if (*cp != 0)
			type = -1;
	}

	if (type == mbrp->mbrp_type)
		/* type not changed... */
		return 1;

	mbri->last_mounted[opt < MBR_PART_COUNT ? opt : 0] = NULL;

	if (MBR_IS_EXTENDED(mbrp->mbrp_type)) {
		/* deleting extended partition.... */
		if (mbri->sector || mbri->extended->extended)
			/* We should have stopped this happening... */
			return err_msg_win("can't delete extended");
		free(mbri->extended);
		mbri->extended = NULL;
	}

	if (type == 0) {
		/* Deleting partition */
		mbrp->mbrp_type = 0;
		/* Remove references to this space from the NetBSD label */
		remove_old_partitions(mbri->sector + mbrp->mbrp_start,
		    mbrp->mbrp_size);
#ifdef BOOTSEL
		if (ombri->bootsec == mbri->sector + mbrp->mbrp_start)
			ombri->bootsec = 0;
				
		memset(mbri->mbrb.mbrbs_nametab[opt], 0,
		    sizeof mbri->mbrb.mbrbs_nametab[opt]);
#endif
		if (mbri->sector == 0) {
			/* A main partition */
			memset(mbrp, 0, sizeof *mbrp);
			return 1;
		}

		/* Merge with previous and next free areas */
		ext = mbri->prev_ext;
		if (ext != NULL && ext->mbr.mbr_parts[0].mbrp_type == 0) {
			/* previous was free - back up one entry */
			mbri = ext;
			ombri->opt--;
		}
		while ((ext = mbri->extended)) {
			if (ext->mbr.mbr_parts[0].mbrp_type != 0)
				break;
			sz = ext->mbr.mbr_parts[0].mbrp_start +
						ext->mbr.mbr_parts[0].mbrp_size;
			/* Increase size of our (empty) partition */
			mbri->mbr.mbr_parts[0].mbrp_size += sz;
			/* Make us describe the next partition */
			mbri->mbr.mbr_parts[1] = ext->mbr.mbr_parts[1];
			/* fix list of extended partitions */
			mbri->extended = ext->extended;
			if (ext->extended != NULL)
				ext->extended->prev_ext = mbri;
			free(ext);
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
			return err_msg_win("main-extended mixup");
		if (find_mbr_space(&mbri->mbr, &start, &sz, bsec, -1) != 0)
			/* no space */
			return err_msg_win(MSG_No_free_space);
		mbrp->mbrp_start = start;
		mbrp->mbrp_size = sz;
		/* If there isn't an active partition mark this one active */
		if (!MBR_IS_EXTENDED(type)) {
			for (i = 0; i < MBR_PART_COUNT; i++)
				if (mbri->mbr.mbr_parts[i].mbrp_flag != 0)
					break;
			if (i == MBR_PART_COUNT)
				mbrp->mbrp_flag = MBR_PFLAG_ACTIVE;
		}
	}

	if (MBR_IS_EXTENDED(type)) {
		if (mbri->sector != 0)
			/* Can't set extended partition in an extended one */
			return err_msg_win(MSG_Only_one_extended_ptn);
		if (mbri->extended)
			/* Can't have two extended partitions */
			return err_msg_win(MSG_Only_one_extended_ptn);
		/* Create new extended partition */
		ext = calloc(1, sizeof *mbri->extended);
		if (!ext)
			return 0;
		mbri->extended = ext;
		ext->sector = mbrp->mbrp_start;
		ext->mbr.mbr_parts[0].mbrp_start = bsec;
		ext->mbr.mbr_parts[0].mbrp_size = mbrp->mbrp_size - bsec;
	}
	mbrp->mbrp_type = type;

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
edit_mbr_type(menudesc *m, void *arg)
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
			13, 12, 0, 30,
			MC_SUBMENU | MC_SCROLL | MC_NOEXITOPT | MC_NOCLEAR,
			NULL, set_type_label, NULL,
			NULL, NULL);
	}

	if (type_menu != -1)
		process_menu(type_menu, arg);

	return 0;
}

static int
edit_mbr_start(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ext;
	struct mbr_partition *mbrp;
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
	} freespace[MBR_PART_COUNT];
	int spaces;
	int i;
	char prompt[MBR_PART_COUNT * 60];
	int len;
	char numbuf[12];

	if (opt >= MBR_PART_COUNT)
		/* should not be able to get here... */
		return 1;

	mbrp = mbri->mbr.mbr_parts + opt;
	/* locate the start of all free areas */
	spaces = 0;
	for (start = bsec, i = 0; i < MBR_PART_COUNT; start += sz, i++) {
		if (find_mbr_space(&mbri->mbr, &start, &sz, start, opt))
			break;
		if (MBR_IS_EXTENDED(mbrp->mbrp_type)) {
			/* Only want the area that contains this partition */
			if (mbrp->mbrp_start < start ||
			    mbrp->mbrp_start >= start + sz)
				continue;
			i = MBR_PART_COUNT - 1;
		}
		freespace[spaces].start = start;
		freespace[spaces].start_r = start / sizemult;
		freespace[spaces].limit = start + sz;
		if (++spaces >= sizeof freespace)
			/* shouldn't happen... */
			break;
	}

	/* Add description of start/size to user prompt */
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

	/* And loop until the user gives a sensible answer */
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
		/*
		 * Check that the start address from the user is inside one
		 * of the free areas.
		 */
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
		/*
		 * We can only increase the start of an extended partition
		 * if the corresponding space inside the partition isn't used.
		 */
		if (new > mbrp->mbrp_start &&
		    MBR_IS_EXTENDED(mbrp->mbrp_type) &&
		    (mbri->extended->mbr.mbr_parts[0].mbrp_type != 0 ||
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

	delta = new - mbrp->mbrp_start;
	if (MBR_IS_EXTENDED(mbrp->mbrp_type)) {
		ext = mbri->extended;
		if (ext->mbr.mbr_parts[0].mbrp_type != 0) {
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
			ext->mbr.mbr_parts[1].mbrp_type = MBR_PTYPE_EXT;
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
	}
	remove_old_partitions(mbri->sector + mbrp->mbrp_start, delta);

	/* finally set partition base and size */
	mbrp->mbrp_start = new;
	mbrp->mbrp_size = limit - new;
	mbri->last_mounted[opt] = NULL;

	return 0;
}

static int
edit_mbr_size(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	mbr_info_t *ext;
	struct mbr_partition *mbrp;
	int opt = mbri->opt;
	uint start, max, max_r, dflt, dflt_r, new;
	uint freespace;
	int delta;
	char numbuf[12];
	char *cp;
	const char *errmsg;

	mbrp = get_mbrp(&mbri, opt);
	dflt = mbrp->mbrp_size;
	if (opt < MBR_PART_COUNT) {
		max = 0;
		find_mbr_space(&mbri->mbr, &start, &max, mbrp->mbrp_start, opt);
		if (start != mbrp->mbrp_start)
			return 0;
		if (dflt == 0)
			dflt = max;
	} else {
		ext = mbri->extended;
		max = dflt;
		/*
		 * If the next extended partition describes a free area,
		 * then merge it onto this area.
		 */
		if (ext != NULL && ext->mbr.mbr_parts[0].mbrp_type == 0) {
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
	/* We need to keep both the unrounded and rounded (_r) max and dflt */
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
		if (new > max_r) {
			errmsg = MSG_Too_large;
			continue;
		}
		if (new == 0)
			/* Treat zero as a request for the maximum */
			new = max_r;
		if (new == dflt_r)
			/* If unchanged, don't re-round size */
			new = dflt;
		else {
			/* Round end to cylinder boundary */
			if (sizemult != 1) {
				new *= sizemult;
				new += ROUNDDOWN(start,current_cylsize);
				new = ROUNDUP(new, current_cylsize);
				new -= start;
				while (new <= 0)
					new += current_cylsize;
			}
		}
		if (new > max)
			/* We rounded the value to above the max */
			new = max;

		if (new == dflt || opt >= MBR_PART_COUNT
		    || !MBR_IS_EXTENDED(mbrp->mbrp_type))
			break;
		/*
		 * We've been asked to change the size of the main extended
		 * partition.  If this reduces the size, then that space
		 * must be unallocated.  If it increases the size then
		 * we must add a description ofthe new free space.
		 */
		/* Find last extended partition */
		for (ext = mbri->extended; ext->extended; ext = ext->extended)
			continue;
		if ((new < dflt && (ext->mbr.mbr_parts[0].mbrp_type != 0
			    || (mbrp->mbrp_start + new < ext->sector + bsec
				&& mbrp->mbrp_start + new != ext->sector)))
		    || (new > dflt && ext->mbr.mbr_parts[0].mbrp_type != 0
							&& new < dflt + bsec)) {
			errmsg = MSG_Space_allocated;
			continue;
		}
		delta = new - dflt;
		if (ext->mbr.mbr_parts[0].mbrp_type == 0) {
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
		mbri->mbr.mbr_parts[1].mbrp_start = ext->sector - ombri->extended->sector;
		mbri->mbr.mbr_parts[1].mbrp_type = MBR_PTYPE_EXT;
		mbri->mbr.mbr_parts[1].mbrp_size = delta;
		break;
	}

	if (opt >= MBR_PART_COUNT && max - new <= bsec)
		/* Round up if not enough space for a header for free area */
		new = max;

	if (new != mbrp->mbrp_size) {
		/* Kill information about old partition from label */
		mbri->last_mounted[opt < MBR_PART_COUNT ? opt : 0] = NULL;
		remove_old_partitions(mbri->sector + mbrp->mbrp_start +
					mbrp->mbrp_size, new - mbrp->mbrp_size);
	}

	mbrp->mbrp_size = new;
	if (opt < MBR_PART_COUNT || new == max)
		return 0;

	/* Add extended partition for the free space */
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
	mbri->mbr.mbr_parts[1].mbrp_start = ext->sector - ombri->extended->sector;
	mbri->mbr.mbr_parts[1].mbrp_type = MBR_PTYPE_EXT;
	mbri->mbr.mbr_parts[1].mbrp_size = freespace;

	return 0;
}

static int
edit_mbr_active(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	int i;
	uint8_t *fl;

	if (mbri->opt >= MBR_PART_COUNT)
		/* sanity */
		return 0;

	/* Invert active flag */
	fl = &mbri->mbr.mbr_parts[mbri->opt].mbrp_flag;
	if (*fl == MBR_PFLAG_ACTIVE) {
		*fl = 0;
		return 0;
	}
		
	/* Ensure there is at most one active partition */
	for (i = 0; i < MBR_PART_COUNT; i++)
		mbri->mbr.mbr_parts[i].mbrp_flag = 0;
	*fl = MBR_PFLAG_ACTIVE;

	return 0;
}

static int
edit_mbr_install(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	struct mbr_partition *mbrp;
	int opt = mbri->opt;
	uint start;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= MBR_PART_COUNT)
		opt = 0;

	start = mbri->sector + mbrp->mbrp_start;
	/* We just remember the start address of the partition... */
	if (start == ombri->install)
		ombri->install = 0;
	else
		ombri->install = start;
	return 0;
}

#ifdef BOOTSEL
static int
edit_mbr_bootmenu(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	struct mbr_partition *mbrp;
	int opt = mbri->opt;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= MBR_PART_COUNT)
		opt = 0;

	msg_prompt_win(/* XXX translate? */ "bootmenu", -1, 18, 0, 0,
		(char *) mbri->mbrb.mbrbs_nametab[opt],
		(char *) mbri->mbrb.mbrbs_nametab[opt],
		sizeof mbri->mbrb.mbrbs_nametab[opt]);
	if (mbri->mbrb.mbrbs_nametab[opt][0] == ' ')
		mbri->mbrb.mbrbs_nametab[opt][0] = 0;
	if (mbri->mbrb.mbrbs_nametab[opt][0] == 0
	    && ombri->bootsec == mbri->sector + mbrp->mbrp_start)
		ombri->bootsec = 0;
	return 0;
}

static int
edit_mbr_bootdefault(menudesc *m, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	struct mbr_partition *mbrp;

	mbrp = get_mbrp(&mbri, mbri->opt);

	ombri->bootsec = mbri->sector + mbrp->mbrp_start;
	return 0;
}
#endif

static void set_ptn_label(menudesc *m, int line, void *arg);
static void set_ptn_header(menudesc *m, void *arg);

static int
edit_mbr_entry(menudesc *m, void *arg)
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
			15, 6, 0, 54,
			MC_SUBMENU | MC_SCROLL | MC_NOCLEAR,
			set_ptn_header, set_ptn_label, NULL,
			NULL, MSG_Partition_OK);
	if (ptn_menu == -1)
		return 1;

	mbri->opt = m->cursel;
	process_menu(ptn_menu, mbri);
	return 0;
}

static void
set_ptn_label(menudesc *m, int line, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	struct mbr_partition *mbrp;
	int opt;
	static const char *yes, *no;

	if (yes == NULL) {
		yes = msg_string(MSG_Yes);
		no = msg_string(MSG_No);
	}

	opt = mbri->opt;
	mbrp = get_mbrp(&mbri, opt);
	if (opt >= MBR_PART_COUNT)
		opt = 0;

	switch (line) {
	case PTN_OPT_TYPE:
		wprintw(m->mw, msg_string(MSG_ptn_type),
			get_partname(mbrp->mbrp_type));
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
		    mbrp->mbrp_flag == MBR_PFLAG_ACTIVE ? yes : no);
		break;
	case PTN_OPT_INSTALL:
		wprintw(m->mw, msg_string(MSG_ptn_install),
		    mbri->sector + mbrp->mbrp_start == ombri->install &&
			mbrp->mbrp_type == MBR_PTYPE_NETBSD ? yes : no);
		break;
#ifdef BOOTSEL
	case PTN_OPT_BOOTMENU:
		wprintw(m->mw, msg_string(MSG_bootmenu),
		    mbri->mbrb.mbrbs_nametab[opt]);
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
	struct mbr_partition *mbrp;
	int opt = mbri->opt;
	int typ;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= MBR_PART_COUNT)
		opt = 0;
	typ = mbrp->mbrp_type;

#define DISABLE(opt,cond) \
	if (cond) \
		m->opts[opt].opt_flags |= OPT_IGNORE; \
	else \
		m->opts[opt].opt_flags &= ~OPT_IGNORE;

	/* Can't change type of the extended partition unless it is empty */
	DISABLE(PTN_OPT_TYPE, MBR_IS_EXTENDED(typ) &&
	    (mbri->extended->mbr.mbr_parts[0].mbrp_type != 0 ||
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
		mbri->mbrb.mbrbs_nametab[opt][0] = 0;

	/* Only partitions with bootmenu names can be made the default */
	DISABLE(PTN_OPT_BOOTDEFAULT, mbri->mbrb.mbrbs_nametab[opt][0] == 0);
#endif
#undef DISABLE
}

static void
set_mbr_label(menudesc *m, int opt, void *arg)
{
	mbr_info_t *mbri = arg;
	mbr_info_t *ombri = arg;
	struct mbr_partition *mbrp;
	uint rstart, rend;
	const char *name, *cp, *mounted;
	int len;

	mbrp = get_mbrp(&mbri, opt);
	if (opt >= MBR_PART_COUNT)
		opt = 0;

	if (mbrp->mbrp_type == 0 && mbri->sector == 0) {
		len = snprintf(0, 0, msg_string(MSG_part_row_used), 0, 0, 0);
		wprintw(m->mw, "%*s", len, "");
	} else {
		rstart = mbri->sector + mbrp->mbrp_start;
		rend = (rstart + mbrp->mbrp_size) / sizemult;
		rstart = rstart / sizemult;
		wprintw(m->mw, msg_string(MSG_part_row_used),
		    rstart, rend - rstart,
		    mbrp->mbrp_flag == MBR_PFLAG_ACTIVE ? 'a' : ' ',
#ifdef BOOTSEL
		    ombri->bootsec == mbri->sector + mbrp->mbrp_start ? 'd' :
#endif
			' ',
		    mbri->sector + mbrp->mbrp_start == ombri->install &&
			mbrp->mbrp_type == MBR_PTYPE_NETBSD ? 'I' : ' ');
	}
	name = get_partname(mbrp->mbrp_type);
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
	if (mbri->mbrb.mbrbs_nametab[opt][0] != 0) {
		int x, y;
		if (opt >= MBR_PART_COUNT)
			opt = 0;
		getyx(m->mw, y, x);
		if (x > 52) {
			x = 52;
			wmove(m->mw, y, x);
		}
		wprintw(m->mw, "%*s %s", 53 - x, "",
		    mbri->mbrb.mbrbs_nametab[opt]);
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
	for (op = opts, i = 0; i < MBR_PART_COUNT; op++, i++) {
		op->opt_name = NULL;
		op->opt_menu = OPT_NOMENU;
		op->opt_flags = OPT_SUB;
		op->opt_action = edit_mbr_entry;
	}
	left = num_opts - MBR_PART_COUNT;

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

int
mbr_use_wholedisk(mbr_info_t *mbri)
{
	struct mbr_sector *mbrs = &mbri->mbr;
	mbr_info_t *ext;
	struct mbr_partition *part;

	part = &mbrs->mbr_parts[0];
	/* Set the partition information for full disk usage. */
	while ((ext = mbri->extended)) {
		mbri->extended = ext->extended;
		free(ext);
	}
	memset(part, 0, MBR_PART_COUNT * sizeof *part);
#ifdef BOOTSEL
	memset(&mbri->mbrb, 0, sizeof mbri->mbrb);
#endif
	part[0].mbrp_type = MBR_PTYPE_NETBSD;
	part[0].mbrp_size = dlsize - bsec;
	part[0].mbrp_start = bsec;
	part[0].mbrp_flag = MBR_PFLAG_ACTIVE;

	ptstart = bsec;
	ptsize = dlsize - bsec;
	return 1;
}

/*
 * Let user change incore Master Boot Record partitions via menu.
 */
int
edit_mbr(mbr_info_t *mbri)
{
	struct mbr_sector *mbrs = &mbri->mbr;
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

	part = &mbrs->mbr_parts[0];
	msg_display(MSG_fullpart, diskdev);
	process_menu(MENU_fullpart, &usefull);

	/* DOS fdisk label checking and value setting. */
	if (usefull) {
		/* Count nonempty, non-BSD partitions. */
		numbsd = 0;
		for (i = 0; i < MBR_PART_COUNT; i++) {
			j = part[i].mbrp_type;
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
		return(md_mbr_use_wholedisk(mbri));
	}

	mbr_menu = new_menu(NULL, NULL, 16, 0, -1, 15, 70,
			MC_NOBOX | MC_ALWAYS_SCROLL | MC_NOCLEAR,
			set_mbr_header, set_mbr_label, NULL,
			NULL, MSG_Partition_table_ok);
	if (mbr_menu == -1)
		return 0;

	/* Default to MB, and use bios geometry for cylinder size */
	set_sizemultname_meg();
	current_cylsize = bhead * bsec;

	for (;;) {
		ptstart = 0;
		ptsize = 0;
		process_menu(mbr_menu, mbri);

		activepart = 0;
		bsdstart = 0;
		bsdsize = 0;
		for (ext = mbri; ext; ext = ext->extended) {
			part = ext->mbr.mbr_parts;
			for (i = 0; i < MBR_PART_COUNT; part++, i++) {
				if (part->mbrp_flag != 0)
					activepart = 1;
				if (part->mbrp_type != MBR_PTYPE_NETBSD)
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
		/* the md_check_mbr function has 3 ret codes to deal with
		 * the different possible states. 0, 1, >1
		 */
		j = md_check_mbr(mbri);
		if (j == 0)
			return 0;
		if (j == 1)
			continue;

		break;
	}

	free_menu(mbr_menu);

	return 1;
}

const char *
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
	struct mbr_sector *mbrs = &mbri->mbr;
	mbr_info_t *ext = NULL;
	char diskpath[MAXPATHLEN];
	int fd, i;
	uint32_t ext_base = 0, next_ext = 0, ext_size = 0;
	int rval = -1;
#ifdef BOOTSEL
	mbr_info_t *ombri = mbri;
	int bootkey = 0;
#endif

	/*
	 * Fake up a likely 'bios sectors per track' for any extended
	 * partition headers we might have to produce.
	 */
	if (bsec == 0)
		bsec = dlsec;

	memset(mbri, 0, sizeof *mbri);

	/* Open the disk. */
	fd = opendisk(disk, O_RDONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		goto bad_mbr;

	for (;;) {
		if (pread(fd, mbrs, sizeof *mbrs,
		    (ext_base + next_ext) * (off_t)MBR_SECSIZE) < sizeof *mbrs)
			break;

		if (!valid_mbr(mbrs))
			break;

		mbrp = &mbrs->mbr_parts[0];
		if (ext_base != 0) {
			/* sanity check extended chain */
			if (MBR_IS_EXTENDED(mbrp[0].mbrp_type))
				break;
			if (mbrp[1].mbrp_type != 0 &&
			    !MBR_IS_EXTENDED(mbrp[1].mbrp_type))
				break;
			if (mbrp[2].mbrp_type != 0 || mbrp[3].mbrp_type != 0)
				break;
			/* Looks ok, link into extended chain */
			mbri->extended = ext;
			ext->prev_ext = next_ext != 0 ? mbri : NULL;
			ext->extended = NULL;
			mbri = ext;
			ext = NULL;
		}
#if BOOTSEL
		if (mbrs->mbr_bootsel_magic == htole16(MBR_MAGIC)) {
			/* old bootsel, grab bootsel info */
			mbri->mbrb = *(struct mbr_bootsel *)
				((uint8_t *)mbrs + MBR_BS_OLD_OFFSET);
			if (ext_base == 0)
				bootkey = mbri->mbrb.mbrbs_defkey - SCAN_1;
		} else if (mbrs->mbr_bootsel_magic == htole16(MBR_BS_MAGIC)) {
			/* new location */
			mbri->mbrb = mbrs->mbr_bootsel;
			if (ext_base == 0)
				bootkey = mbri->mbrb.mbrbs_defkey - SCAN_1;
		}
		/* Save original flags for mbr code update tests */
		mbri->oflags = mbri->mbrb.mbrbs_flags;
#endif
		mbri->sector = next_ext + ext_base;
		next_ext = 0;
		rval = 0;
		for (i = 0; i < MBR_PART_COUNT; mbrp++, i++) {
			if (mbrp->mbrp_type == 0) {
				/* type is unused, discard scum */
				memset(mbrp, 0, sizeof *mbrp);
				continue;
			}
			mbrp->mbrp_start = le32toh(mbrp->mbrp_start);
			mbrp->mbrp_size = le32toh(mbrp->mbrp_size);
			if (MBR_IS_EXTENDED(mbrp->mbrp_type)) {
				next_ext = mbrp->mbrp_start;
				if (ext_base == 0)
					ext_size = mbrp->mbrp_size;
			} else {
				mbri->last_mounted[i] = strdup(get_last_mounted(
					fd, mbri->sector + mbrp->mbrp_start));
#if BOOTSEL
				if (ombri->install == 0 &&
				    strcmp(mbri->last_mounted[i], "/") == 0)
					ombri->install = mbri->sector +
							mbrp->mbrp_start;
#endif
			}
#if BOOTSEL
			if (mbri->mbrb.mbrbs_nametab[i][0] != 0
			    && bootkey-- == 0)
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
			mbrp -= MBR_PART_COUNT;
			base =mbri->sector + mbrp->mbrp_start + mbrp->mbrp_size;
			if (mbrp->mbrp_type != 0 && ext_base + limit != base) {
				/* Mock up an extry for the space */
				ext = calloc(1, sizeof *ext);
				if (!ext)
					break;
				ext->sector = base;
				ext->mbr.mbr_magic = htole16(MBR_MAGIC);
				ext->mbr.mbr_parts[1] = mbrp[1];
				ext->mbr.mbr_parts[0].mbrp_start = bsec;
				ext->mbr.mbr_parts[0].mbrp_size =
				    ext_base + limit - base - bsec;
				mbrp[1].mbrp_type = MBR_PTYPE_EXT;
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
		ext = calloc(sizeof *ext, 1);
		if (!ext)
			break;
		mbrs = &ext->mbr;
	}

    bad_mbr:
	free(ext);
	if (fd >= 0)
		close(fd);
	if (rval == -1) {
		memset(&mbrs->mbr_parts, 0, sizeof mbrs->mbr_parts);
		mbrs->mbr_magic = htole16(MBR_MAGIC);
	}
	dump_mbr(ombri, "read");
	return rval;
}

int
write_mbr(const char *disk, mbr_info_t *mbri, int convert)
{
	char diskpath[MAXPATHLEN];
	int fd, i, ret = 0;
	struct mbr_partition *mbrp;
	u_int32_t pstart, psize;
#ifdef BOOTSEL
	struct mbr_sector *mbrs;
#endif
	struct mbr_sector mbrsec;
	mbr_info_t *ext;
	uint sector;

	/* Open the disk. */
	fd = opendisk(disk, O_WRONLY, diskpath, sizeof(diskpath), 0);
	if (fd < 0)
		return -1;

#ifdef BOOTSEL
	/*
	 * If the main boot code (appears to) contain the netbsd bootcode,
	 * copy in all the menu strings and set the default keycode
	 * to be that for the default partition.
	 * Unfortunately we can't rely on the user having actually updated
	 * to the new mbr code :-(
	 */
	if (mbri->mbr.mbr_bootsel_magic == htole16(MBR_BS_MAGIC)
	    || mbri->mbr.mbr_bootsel_magic == htole16(MBR_MAGIC)) {
		int8_t key = SCAN_1;
		uint offset = MBR_BS_OFFSET;
		if (mbri->mbr.mbr_bootsel_magic == htole16(MBR_MAGIC))
			offset = MBR_BS_OLD_OFFSET;
		mbri->mbrb.mbrbs_defkey = SCAN_ENTER;
		if (mbri->mbrb.mbrbs_timeo == 0)
			mbri->mbrb.mbrbs_timeo = 182;	/* 10 seconds */
		for (ext = mbri; ext != NULL; ext = ext->extended) {
			mbrs = &ext->mbr;
			mbrp = &mbrs->mbr_parts[0];
			/* Ensure marker is set in each sector */
			mbrs->mbr_bootsel_magic = mbri->mbr.mbr_bootsel_magic;
			/* and copy in bootsel parameters */
			*(struct mbr_bootsel *)((uint8_t *)mbrs + offset) =
								    ext->mbrb;
			for (i = 0; i < MBR_PART_COUNT; i++) {
				if (ext->mbrb.mbrbs_nametab[i][0] == 0)
					continue;
				if (ext->sector + mbrp->mbrp_start ==
								mbri->bootsec)
					mbri->mbrb.mbrbs_defkey = key;
				key++;
			}
		}
		/* copy main data (again) since we've put the 'key' in */
		*(struct mbr_bootsel *)((uint8_t *)&mbri->mbr + offset) =
								    mbri->mbrb;
	}
#endif

	for (ext = mbri; ext != NULL; ext = ext->extended) {
		sector = ext->sector;
		mbrsec = ext->mbr;	/* copy sector */
		mbrp = &mbrsec.mbr_parts[0];

		if (sector != 0 && ext->extended != NULL
		    && ext->extended->mbr.mbr_parts[0].mbrp_type == 0) {
			/* We are followed by an empty slot, collapse out */
			ext = ext->extended;
			/* Make us describe the next non-empty partition */
			mbrp[1] = ext->mbr.mbr_parts[1];
		}

		for (i = 0; i < MBR_PART_COUNT; i++) {
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
		}

		mbrsec.mbr_magic = htole16(MBR_MAGIC);
		if (pwrite(fd, &mbrsec, sizeof mbrsec,
					    sector * (off_t)MBR_SECSIZE) < 0) {
			ret = -1;
			break;
		}
	}

	(void)close(fd);
	return ret;
}

int
valid_mbr(struct mbr_sector *mbrs)
{

	return (le16toh(mbrs->mbr_magic) == MBR_MAGIC);
}

static void
convert_mbr_chs(int cyl, int head, int sec,
		uint8_t *cylp, uint8_t *headp, uint8_t *secp,
		uint32_t relsecs)
{
	unsigned int tcyl, temp, thead, tsec;

	temp = head * sec;
	tcyl = relsecs / temp;
	relsecs -= tcyl * temp;

	thead = relsecs / sec;
	tsec = relsecs - thead * sec + 1;

	if (tcyl > MAXCYL)
		tcyl = MAXCYL;

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
guess_biosgeom_from_mbr(mbr_info_t *mbri, int *cyl, int *head, int *sec)
{
	struct mbr_sector *mbrs = &mbri->mbr;
	struct mbr_partition *parts = &mbrs->mbr_parts[0];
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
		xcylinders = dlsize / (MAXSECTOR * MAXHEAD);
		if (xcylinders > MAXCYL)
			xcylinders = MAXCYL;
	}
	*cyl = xcylinders;
	*head = xheads;
	*sec = xsectors;

	xheads = -1;

	/* Try to deduce the number of heads from two different mappings. */
	for (i = 0; i < MBR_PART_COUNT * 2 - 1; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		a1 -= s1;
		for (j = i + 1; j < MBR_PART_COUNT * 2; j++) {
			if (get_mapping(parts, j, &c2, &h2, &s2, &a2) < 0)
				continue;
			a2 -= s2;
			num = (uint64_t)h1 * a2 - (quad_t)h2 * a1;
			denom = (uint64_t)c2 * a1 - (quad_t)c1 * a2;
			if (num != 0 && denom != 0 && num % denom == 0) {
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
	xcylinders = dlsize / xheads / xsectors;
	if (dlsize != xcylinders * xheads * xsectors)
		xcylinders++;

	/*
	 * Now verify consistency with each of the partition table entries.
	 * Be willing to shove cylinders up a little bit to make things work,
	 * but translation mismatches are fatal.
	 */
	for (i = 0; i < MBR_PART_COUNT * 2; i++) {
		if (get_mapping(parts, i, &c1, &h1, &s1, &a1) < 0)
			continue;
		if (c1 >= MAXCYL - 1)
			/* Ignore anything that is near the CHS limit */
			continue;
		if (xsectors * (c1 * xheads + h1) + s1 != a1)
			return -1;
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
get_mapping(struct mbr_partition *parts, int i,
	    int *cylinder, int *head, int *sector, unsigned long *absolute)
{
	struct mbr_partition *apart = &parts[i / 2];

	if (apart->mbrp_type == 0)
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
