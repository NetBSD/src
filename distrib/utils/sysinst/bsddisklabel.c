/*	$NetBSD: bsddisklabel.c,v 1.12 2003/06/04 20:05:12 dsl Exp $	*/

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

int	make_bsd_partitions (void);

static int
save_ptn(int ptn, int start, int size, int fstype, const char *mountpt)
{
	static int maxptn;
	int p;

	if (maxptn == 0)
		maxptn = getmaxpartitions();

	if (ptn < 0 || bsdlabel[ptn].pi_fstype != FS_UNUSED) {
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
		bsdlabel[ptn].pi_newfs = 1;
	}

	if (mountpt != NULL) {
		for (p = 0; p < maxptn; p++)
			if (strcmp(bsdlabel[p].pi_mount, mountpt) == 0)
				bsdlabel[p].pi_mount[0] = 0;
		strlcpy(bsdlabel[ptn].pi_mount, mountpt,
			sizeof bsdlabel[0].pi_mount);
	}

	return ptn;
}

struct ptn_info {
	struct ptn_size {
		int	ptn_id;
		char	mount[20];
		int	dflt_size;
		int	size;
	}		ptn_sizes[MAXPARTITIONS];
	int		free_parts;
	int		free_space;
	struct ptn_size *pool_part;
	int		menu_no;
	WINDOW		*msg_win;
	menu_ent	ptn_menus[MAXPARTITIONS + 1];
	char		ptn_titles[MAXPARTITIONS + 1][70];
	char		exit_msg[70];
};

static int set_ptn_size(menudesc *, menu_ent *, void *);

static void
set_ptn_menu_texts(struct ptn_info *pi)
{
	struct ptn_size *p;
	menu_ent *m;
	int i;
	int sm = MEG / sectorsize;
	int size;

	for (i = 0; i < MAXPARTITIONS; i++) {
		p = &pi->ptn_sizes[i];
		m = &pi->ptn_menus[i];
		m->opt_menu = OPT_NOMENU;
		m->opt_flags = 0;
		m->opt_action = set_ptn_size;
		if (p->mount[0] == 0)
			break;
		size = p->size;
		snprintf(pi->ptn_titles[i], sizeof pi->ptn_titles[0],
			"%6u%10u%10u  %c %s",
			size / sm, size / dlcylsize, size,
			p == pi->pool_part ? '+' : ' ',
			p->mount);
	}

	if (pi->free_parts > 0) {
		snprintf(pi->ptn_titles[i], sizeof pi->ptn_titles[0], "%s",
			msg_string(MSG_add_another_ptn));
		i++;
	}

	if (i < MAXPARTITIONS) {	/* it always is... */
		m = &pi->ptn_menus[i];
		current_cylsize = dlcylsize;
		m->opt_menu = MENU_sizechoice;
		m->opt_flags = OPT_SUB;
		m->opt_action = NULL;
		snprintf(pi->ptn_titles[i], sizeof pi->ptn_titles[0], "%s",
			msg_string(MSG_askunits));
		i++;
	}

	if (pi->free_space >= 0)
		snprintf(pi->exit_msg, sizeof pi->exit_msg,
			msg_string(MSG_fssizesok),
			pi->free_space / sizemult, multname, pi->free_parts);
	else
		snprintf(pi->exit_msg, sizeof pi->exit_msg,
			msg_string(MSG_fssizesbad),
			-pi->free_space / sizemult, multname, -pi->free_space);

	set_menu_numopts(pi->menu_no, i);
}

static int
set_ptn_size(menudesc *m, menu_ent *opt, void *arg)
{
	struct ptn_info *pi = arg;
	struct ptn_size *p;
	char answer[20];
	char *cp;
	int size;
	int mult;

	p = pi->ptn_sizes + (opt - m->opts);

	if (p->mount[0] == 0) {
		msg_prompt_win(MSG_askfsmount, pi->msg_win,
			NULL, p->mount, sizeof p->mount);
		if (p->mount[0] == 0)
			return 0;
	}

	for (;;) {
		size = p->size;
		if (size == 0)
			size = p->dflt_size;
		size /= sizemult;
		snprintf(answer, sizeof answer, "%d", size);

		msg_prompt_win(MSG_askfssize, pi->msg_win,
			answer, answer, sizeof answer,
			p->mount, multname);
		size = strtoul(answer, &cp, 0);
		mult = sizemult;
		switch (*cp++) {
		default:
			continue;
		case 's':
			mult = 1;
			break;
		case 'c':
			mult = dlcylsize;
			break;
		case 'm':
			mult = MEG / sectorsize;
			break;
		case '+':
			cp--;
			if (cp != answer)
				break;
			mult = 1;
			size = p->size;
			break;
		case 0:
			cp--;
			break;
		}
		if (*cp == 0 || *cp == '+')
			break;
	}

	size = NUMSEC(size, mult, dlcylsize);
	if (pi->free_parts == 0 && size != 0 && p->size == 0)
		/* Don't allow 'free_parts' to go negative */
		return 0;
	if (p == pi->pool_part)
		pi->pool_part = NULL;
	if (size != 0 && *cp == '+')
		pi->pool_part = p;
	pi->free_space += p->size - size;
	if (size == 0) {
		if (p->size != 0)
			pi->free_parts++;
		if (p->ptn_id == -2)
			memmove(p, p + 1,
				(char *)&pi->ptn_sizes[MAXPARTITIONS - 1]
				- (char *)p);
	} else {
		if (p->size == 0)
			pi->free_parts--;
		if (pi->free_space < mult && -pi->free_space < mult
		    && mult > 1) {
			size += pi->free_space;
			pi->free_space = 0;
		}
	}
	p->size = size;

	set_ptn_menu_texts(pi);

	return 0;
}

static void
get_ptn_sizes(int layoutkind, int part_start, int sectors)
{
	int i;
	int maxpart = getmaxpartitions();
	int sm;				/* sectors in 1MB */
	struct ptn_size *p;
	int size;

#define ROOT_SIZE (DEFROOTSIZE + DEFUSRSIZE)
	struct ptn_info pi = { {
		{ PART_ROOT,	"/",	ROOT_SIZE,	ROOT_SIZE },
		{ PART_SWAP,	"swap",	128,		128 },
		{ PART_USR,	"/usr",	DEFUSRSIZE },
		{ -1,		"/var",	DEFVARSIZE },
		{ -1,		"/home",	0 },
		{ -2 }, { -2 }, { -2 }, { -2 }, { -2 }, { -2 },
		{ -2 }, { -2 }, { -2 }, { -2 }, { -2 }, 
	} };
	menu_ent *m;

	if (maxpart > MAXPARTITIONS)
		maxpart = MAXPARTITIONS;	/* sanity */

	/* If installing X increase default size of /usr */
	if (layoutkind == 2) {
		pi.ptn_sizes[0].dflt_size += XNEEDMB;
		pi.ptn_sizes[2].dflt_size += XNEEDMB;
	}

	/* Change preset size from MB to sectors */
	sm = MEG / sectorsize;
	pi.free_space = sectors;
	for (p = pi.ptn_sizes; p->mount[0]; p++) {
		p->size = NUMSEC(p->size, sm, dlcylsize);
		p->dflt_size = NUMSEC(p->dflt_size, sm, dlcylsize);
		pi.free_space -= p->size;
	}

	/* Count free partition slots */
	pi.free_parts = -2;		/* allow for root and swap */
	for (i = 0; i < maxpart; i++) {
		if (bsdlabel[i].pi_size == 0)
			pi.free_parts++;
	}

	/* Give free space to one of the partitions */
	pi.pool_part = &pi.ptn_sizes[0];

	pi.msg_win = newwin(3, getmaxx(stdscr) - 30, getmaxy(stdscr) - 4,  15);
	if (pi.msg_win == NULL)
		return;
	if (has_colors()) {
		/*
		 * XXX This color trick should be done so much better,
		 * but is it worth it?
		 */
		wbkgd(pi.msg_win, COLOR_PAIR(1));
		wattrset(pi.msg_win, COLOR_PAIR(1));
	}

	/* Link data areas together for menu */
	for (i = 0; i < MAXPARTITIONS; i++) {
		m = &pi.ptn_menus[i];
		m->opt_name = pi.ptn_titles[i];
	}
	msg_display(MSG_ptnsizes);
	msg_table_add(MSG_ptnheaders);

	pi.menu_no = new_menu(0, pi.ptn_menus, MAXPARTITIONS,
		0, 9, 12, sizeof pi.ptn_titles[0],
		MC_SCROLL | MC_NOBOX | MC_NOCLEAR, NULL, NULL,
		"help", pi.exit_msg);
	if (pi.menu_no < 0)
		return;

	do {
		set_ptn_menu_texts(&pi);
		process_menu(pi.menu_no, &pi);
	} while (pi.free_space < 0 || pi.free_parts < 0);
	free_menu(pi.menu_no);

	for (p = pi.ptn_sizes; p->mount[0]; p++, part_start += size) {
		size = p->size;
		if (p == pi.pool_part)
			size += pi.free_space;
		if (size == 0)
			continue;
		if (p->ptn_id == PART_SWAP) {
			save_ptn(p->ptn_id, part_start, size, FS_SWAP, NULL);
			continue;
		}
		save_ptn(p->ptn_id, part_start, size, FS_BSDFFS, p->mount);
	}
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 */
int
make_bsd_partitions(void)
{
	int i;
	int part;
	int maxpart = getmaxpartitions();
	struct disklabel l;
	int partstart;
	int part_raw, part_bsd;
	int ptend;
	struct partition *p;

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
		/* Probably a system that expects an i386 style mbr */
		part_bsd = C;
		bsdlabel[C].pi_offset = ptstart;
		bsdlabel[C].pi_size = ptsize;
	} else {
		part_bsd = part_raw;
	}

#if defined(PART_BOOT) && defined(BOOT_SIZE)
	i = BOOT_SIZE;
	if (i >= 1024) {
		/* Treat big numbers as a byte count */
		i = (i + dlcylsize * sectorsize - 1) / (dlcylsize * sectorsize);
		i *= dlcylsize;
	}
	bsdlabel[PART_BOOT].pi_fstype = FS_BOOT;
	bsdlabel[PART_BOOT].pi_size = i;
#ifdef BOOT_HIGH
	bsdlabel[PART_BOOT].pi_offset = ptend - i;
	ptend -= i;
#else
	bsdlabel[PART_BOOT].pi_offset = ptstart;
	partstart += i;
#endif
#endif

#ifdef PART_REST
	bsdlabel[PART_REST].pi_offset = 0;
	bsdlabel[PART_REST].pi_size = ptstart;
#endif

	/* Ask for layout type -- standard or special */
	msg_display(MSG_layout,
		    (1.0*ptsize*sectorsize)/MEG,
		    (1.0*minfsdmb*sectorsize)/MEG,
		    (1.0*minfsdmb*sectorsize)/MEG+rammb+XNEEDMB);

	process_menu(MENU_layout, NULL);

	if (layoutkind == 3)
		reask_sizemult(dlcylsize);
	else
		md_set_sizemultname();

	if (get_real_geom(diskdev, &l) == 0) {
		if (layoutkind == 4) {
			/* Must have an old label to do 'existing' */
			msg_display(MSG_abort);	/* XXX more informative */
			return 0;
		}
	} else {
		/*
		 * Save any partitions that are outside the area we are
		 * going to use.
		 * In particular this saves details of the other MBR
		 * partitons on a multiboot i386 system.
		 */
		 for (i = MIN(l.d_npartitions, maxpart); i--;) {
			if (bsdlabel[i].pi_size != 0)
				/* Don't overwrite special partitions */
				continue;
			p = &l.d_partitions[i];
			if (p->p_fstype == FS_UNUSED || p->p_size == 0)
				continue;
			if (layoutkind != 4 &&
			    p->p_offset < ptstart + ptsize &&
			    p->p_offset + p->p_size > ptstart)
				/* Not outside area we are allocating */
				continue;
			bsdlabel[i].pi_size = p->p_size;
			bsdlabel[i].pi_offset = p->p_offset;
			bsdlabel[i].pi_fstype = p->p_fstype;
			bsdlabel[i].pi_bsize = p->p_fsize * p->p_frag;
			bsdlabel[i].pi_fsize = p->p_fsize;
			/* XXX get 'last mounted' */
		 }
	}

	if (layoutkind != 4)
		get_ptn_sizes(layoutkind, partstart, ptend - partstart);


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
