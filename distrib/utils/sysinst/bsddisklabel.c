/*	$NetBSD: bsddisklabel.c,v 1.10 2003/05/30 22:17:00 dsl Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Based on code written by Philip A. Nelson for Piermont Information
 * Systems Inc.
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

/* bsddisklabel.c -- generate standard BSD disklabel */
/* Included by appropriate arch/XXXX/md.c */

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/exec.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <machine/cpu.h>
#include <stdio.h>
#include <stddef.h>
#include <util.h>
#include <dirent.h>
#include "defs.h"
#include "md.h"
#include "endian.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* For the current state of this file blame abs@netbsd.org */
/* Even though he wasn't the last to hack it, but he did admit doing so :-) */

/* Defaults for things that might be defined in md.h */
#ifndef PART_ROOT
#define PART_ROOT	A
#endif
#ifndef PART_SWAP
#define PART_SWAP	B
#endif
#ifndef PART_USR
#define PART_USR	-1
#endif

#ifndef DEFSWAPRAM
#define DEFSWAPRAM	32
#endif

#ifndef DEFVARSIZE
#define DEFVARSIZE	32
#endif
#ifndef DEFROOTSIZE
#define DEFROOTSIZE	32
#endif
#ifndef DEFUSRSIZE
#define DEFUSRSIZE	128
#endif
#ifndef STDNEEDMB
#define STDNEEDMB	100
#endif

int	make_bsd_partitions (void);

static int
save_ptn(int ptn, int start, int size, int fstype, const char *mountpt)
{
	int maxptn;

	if (ptn == -1) {
		maxptn = getmaxpartitions();
		ptn = getrawpartition() + 1;
#ifdef PART_FIRST_FREE
		if (ptn < PART_FIRST_FREE)
			ptn = PART_FIRST_FREE;
#endif
		for (;; ptn++) {
			if (ptn >= maxptn)
				return -1;
			if (ptn == PART_USR)
				continue;
			if (bsdlabel[ptn].pi_fstype == FS_UNUSED)
				break;
		}
	}

	if (fstype == FS_UNUSED)
		return ptn;

	bsdlabel[ptn].pi_fstype = fstype;
	bsdlabel[ptn].pi_offset = start;
	bsdlabel[ptn].pi_size = size;
	if (fstype == FS_BSDFFS) {
		bsdlabel[ptn].pi_bsize = 8192;
		bsdlabel[ptn].pi_fsize = 1024;
	}
	if (mountpt)
		strlcpy(fsmount[ptn], mountpt, sizeof fsmount[0]);

	return ptn;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
make_bsd_partitions(void)
{
	int i;
	int part;
	int remain;
	char isize[20];
	int maxpart = getmaxpartitions();
	struct disklabel l;
	int varsz = 0, swapsz = 0;
	int part_raw, part_bsd;
	int partstart, partsize;
	int ptend;

	/*
	 * Initialize global variables that track space used on this disk.
	 * Standard 4.4BSD 8-partition labels always cover whole disk.
	 */
	if (ptsize == 0)
		ptsize = dlsize - ptstart;
	if (dlsize == 0)
		dlsize = ptstart + ptsize;

	partstart = ptstart;
	ptend = ptstart + ptsize;

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
		    (1.0*ptsize*sectorsize)/MEG,
		    (1.0*minfsdmb*sectorsize)/MEG,
		    (1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);
	process_menu(MENU_layout);

	if (layoutkind == 3)
		ask_sizemult(dlcylsize);
	else
		md_set_sizemultname();

	/* Build standard partitions */
	emptylabel(bsdlabel);

	/* Set initial partition types to unused */
	for (part = 0 ; part < maxpart ; ++part)
		bsdlabel[part].pi_fstype = FS_UNUSED;

	/* Whole disk partition */
	part_raw = getrawpartition();
	if (part_raw == -1)
		part_raw = C;	/* for sanity... */
	bsdlabel[part_raw].pi_offset = 0;
	bsdlabel[part_raw].pi_size = dlsize;

	if (part_raw == D) {
		part_bsd = C;
		bsdlabel[C].pi_offset = ptstart;
		bsdlabel[C].pi_size = ptsize;
	} else {
		part_bsd = part_raw;
	}

#if defined(PART_BOOT) && defined(BOOT_SIZE)
	partsize = BOOT_SIZE;
	if (partsize >= 1024) {
		/* Treat big numbers as a byte count */
		partsize = (partsize + dlcylsize * sectorsize - 1)
				/ (dlcylsize * sectorsize);
		partsize *= dlcylsize;
	}
	bsdlabel[PART_BOOT].pi_fstype = FS_BOOT;
	bsdlabel[PART_BOOT].pi_size = partsize;
#ifdef BOOT_HIGH
	bsdlabel[PART_BOOT].pi_offset = ptend - partsize;
	ptend -= partsize;
#else
	bsdlabel[PART_BOOT].pi_offset = ptstart;
	partstart += partsize;
#endif
#endif

#ifdef PART_REST
	bsdlabel[PART_REST].pi_offset = 0;
	bsdlabel[PART_REST].pi_size = ptstart;
#endif

	switch (layoutkind) {
	case 1: /* standard: a root, b swap, c "unused", PART_USR /usr */
	case 2: /* standard X: as above, but with larger swap, and more
		 * space in /usr */

		/*
		 * Defaults: 
		 *	root	- always (PART_ROOT)
		 *	swap	- ask, default (PART_SWAP)
		 *	/usr	- ask, default (PART_USR)
		 *	/var	- ask (PART_FIRST_FREE != PART_USR)
		 *	/home	- ask (PART_FIRST_FREE+1 != PART_USR)
		 *	/tmp	- ask mfs
		 */
		layout_swap = layout_usr = 1;
		layout_var = layout_home = layout_tmp = 0;
		process_menu(MENU_layoutparts);

		/* Root */
		if (layout_usr == 0 && layout_home == 0) {
			if (layout_swap) {
				i = NUMSEC(layoutkind * 2 *
				    (rammb < DEFSWAPRAM ? DEFSWAPRAM : rammb),
				    MEG/sectorsize, dlcylsize);
				swapsz = NUMSEC(i/(MEG/sectorsize)+1,
				    MEG/sectorsize, dlcylsize);
			}
			if (layout_var)
				varsz = NUMSEC(DEFVARSIZE, MEG/sectorsize,
				    dlcylsize);
			partsize = ptsize - swapsz - varsz;
		} else if (layout_usr)
			partsize = NUMSEC(DEFROOTSIZE, MEG/sectorsize,
			    dlcylsize);
		else if (layoutkind == 2)
			partsize = NUMSEC(STDNEEDMB + XNEEDMB, MEG/sectorsize,
			    dlcylsize);
		else
			partsize = NUMSEC(STDNEEDMB, MEG/sectorsize, dlcylsize);
		
		save_ptn(PART_ROOT, partstart, partsize, FS_BSDFFS, "/");
		partstart += partsize;
#if notyet
		if (partstart > ptend)
			error ... 
#endif

		/* swap */
		if (layout_swap) {
			if (swapsz)
				partsize = swapsz;
			else {
				i = NUMSEC(layoutkind * 2 *
				    (rammb < DEFSWAPRAM ? DEFSWAPRAM : rammb),
				    MEG/sectorsize, dlcylsize) + partstart;
				partsize = NUMSEC(i/(MEG/sectorsize)+1,
				    MEG/sectorsize, dlcylsize) - partstart;
			}
			save_ptn(PART_SWAP, partstart, partsize, FS_SWAP, 0);
			partstart += partsize;
#if notyet
			if (partstart > ptend)
				error ... 
#endif
		}

		/* var */
		if (layout_var) {
			if (varsz)
				partsize = varsz;
			else
				partsize = NUMSEC(DEFVARSIZE, MEG/sectorsize,
				    dlcylsize);
			save_ptn(-1, partstart, partsize, FS_BSDFFS, "/var");
			partstart += partsize;
#if notyet
			if (partstart > ptend)
				error ... 
#endif
		}

		/* /usr */
		if (layout_usr) {
			if (layout_home) {
				if (layoutkind == 2)
					partsize = NUMSEC(DEFUSRSIZE + XNEEDMB,
					    MEG/sectorsize, dlcylsize);
				else
					partsize = NUMSEC(DEFUSRSIZE,
					    MEG/sectorsize, dlcylsize);
			} else
				partsize = ptend - partstart;
			save_ptn(PART_USR, partstart, partsize, FS_BSDFFS, "/usr");
			partstart += partsize;
#if notyet
			if (partstart > ptend)
				error ... 
#endif
		}

		/* home */
		if (layout_home) {
			partsize = ptend - partstart;
			save_ptn(-1, partstart, partsize, FS_BSDFFS, "/home");
			partstart += partsize;
#if notyet
			if (partstart > ptend)
				error ... 
#endif
		}

		break;

	case 3: /* custom: ask user for all sizes */
		ask_sizemult(dlcylsize);
		/* root */
		remain = ptend - partstart;
		partsize = NUMSEC(DEFROOTSIZE, MEG/sectorsize, dlcylsize);
		snprintf(isize, 20, "%d", partsize/sizemult);
		msg_prompt(MSG_askfsroot, isize, isize, 20,
			    remain/sizemult, multname);
		partsize = NUMSEC(atoi(isize), sizemult, dlcylsize);
		/* If less than a 'unit' left, also use it */
		if (remain - partsize < sizemult)
			partsize = remain;
		save_ptn(PART_ROOT, partstart, partsize, FS_BSDFFS, "/");
		partstart += partsize;

		/* swap */
		remain = ptend - partstart;
		if (remain > 0) {
			i = NUMSEC(2 *
				   (rammb < DEFSWAPRAM ? DEFSWAPRAM : rammb),
				   MEG/sectorsize, dlcylsize) + partstart;
			partsize = NUMSEC(i/(MEG/sectorsize)+1, MEG/sectorsize,
				   dlcylsize) - partstart;
			if (partsize > remain)
				partsize = remain;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfsswap, isize, isize, 20,
				    remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize), sizemult, dlcylsize);
			if (partsize) {
				/* If less than a 'unit' left, also use it */
				if (remain - partsize < sizemult)
					partsize = remain;
				save_ptn(PART_SWAP, partstart, partsize, FS_SWAP, 0);
				partstart += partsize;
			}
		}

		/* /usr */
		remain = ptend - partstart;
		if (remain > 0) {
			partsize = remain;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfsusr, isize, isize, 20,
				    remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize), sizemult, dlcylsize);
			if (partsize) {
				/* If less than a 'unit' left, also use it */
				if (remain - partsize < sizemult)
					partsize = remain;
				save_ptn(PART_USR, partstart, partsize, FS_BSDFFS, "/usr");
				partstart += partsize;
			}
		}

		/* Others ... */
		remain = ptend - partstart;
		if (remain > 0)
			msg_display(MSG_otherparts);
		part = save_ptn(-1, 0, 0, FS_UNUSED, 0);
		for (; remain > 0 && part < maxpart; ++part) {
			if (bsdlabel[part].pi_fstype != FS_UNUSED)
				continue;
			partsize = ptend - partstart;
			snprintf(isize, 20, "%d", partsize/sizemult);
			msg_prompt_add(MSG_askfspart, isize, isize, 20,
					diskdev, partition_name(part),
					remain/sizemult, multname);
			partsize = NUMSEC(atoi(isize), sizemult, dlcylsize);
			/* If less than a 'unit' left, also use it */
			if (remain - partsize < sizemult)
				partsize = remain;
			save_ptn(-1, partstart, partsize, FS_BSDFFS, 0);
			msg_prompt_add(MSG_mountpoint, NULL, fsmount[part], 20);
			partstart += partsize;
			remain = ptend - partstart;
		}

		break;

	case 4: /* use existing disklabel */

		if (get_real_geom(diskdev, &l) == 0) {
			msg_display(MSG_abort);	/* XXX more informative */
			return 0;
		}

		for (i = 0; i < maxpart; i++) {
#define p l.d_partitions[i]
			if (i == part_raw || i == part_bsd)
				continue;
#ifdef PART_BOOT
			if (i == PART_BOOT)
				continue;
#endif
#ifdef PART_REST
			if (i == PART_REST)
				continue;
#endif
			bsdlabel[i].pi_size = p.p_size;
			bsdlabel[i].pi_offset = p.p_offset;
			bsdlabel[i].pi_fstype = p.p_fstype;
			bsdlabel[i].pi_bsize = p.p_fsize * p.p_frag;
			bsdlabel[i].pi_fsize = p.p_fsize;
			/* menu to get fsmount[] entry */
#undef p
		}
		msg_display(MSG_postuseexisting);
		getchar();
		break;
	}

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 edit_check:
	if (edit_and_check_label(bsdlabel, maxpart, part_raw, part_bsd) == 0) {
		msg_display(MSG_abort);
		return 0;
	}
	if (md_check_partitions() == 0)
		goto edit_check;

	/* Disk name */
	msg_prompt(MSG_packname, "mydisk", bsddiskname, DISKNAME_SIZE);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, maxpart);

	/* Everything looks OK. */
	return (1);
}
