/*	$NetBSD: bsddisklabel.c,v 1.2.20.2 2018/06/09 15:19:27 martin Exp $	*/

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
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
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

/* For the current state of this file blame abs@NetBSD.org */
/* Even though he wasn't the last to hack it, but he did admit doing so :-) */

#define	PART_ANY		-1
#define	PART_EXTRA		-2
#define	PART_TMP_RAMDISK	-3

/* Defaults for things that might be defined in md.h */
#ifndef PART_ROOT
#define PART_ROOT	PART_A
#endif
#ifndef PART_SWAP
#define PART_SWAP	PART_B
#endif
#ifndef PART_USR
#define PART_USR	PART_ANY
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
#ifndef DEFSWAPSIZE
#define DEFSWAPSIZE	128
#endif

int
save_ptn(int ptn, daddr_t start, daddr_t size, int fstype, const char *mountpt)
{
	static int maxptn;
	partinfo *p;
	int pp;
	char *buf;

	if (maxptn == 0)
		maxptn = getmaxpartitions();

	if (ptn < 0 || pm->bsdlabel[ptn].pi_fstype != FS_UNUSED) {
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
			if (pm->bsdlabel[ptn].pi_fstype == FS_UNUSED)
				break;
		}
	}

	if (fstype == FS_UNUSED)
		return ptn;

	p = pm->bsdlabel + ptn;
	p->pi_offset = start;
	p->pi_size = size;
	set_ptype(p, fstype, mountpt ? PIF_NEWFS : 0);

	/* Hack because we does not have something like FS_LVMPV */
	p->lvmpv = 0;
	if (mountpt != NULL && strcmp(mountpt, "lvm") == 0)
		p->lvmpv = 1;
	else if (mountpt != NULL) {
		for (pp = 0; pp < maxptn; pp++) {
			if (strcmp(pm->bsdlabel[pp].pi_mount, mountpt) == 0)
				pm->bsdlabel[pp].pi_flags &= ~PIF_MOUNT;
		}
		if (mountpt[0] != '/')
			asprintf(&buf, "/%s", mountpt);
		else
			asprintf(&buf, "%s", mountpt);
		strlcpy(p->pi_mount, buf, sizeof p->pi_mount);
		p->pi_flags |= PIF_MOUNT;
		/* Default to UFS2. */
		if (p->pi_fstype == FS_BSDFFS) {
#ifdef DEFAULT_UFS2
#ifndef HAVE_UFS2_BOOT
			if (strcmp(mountpt, "/") != 0)
#endif
				p->pi_flags |= PIF_FFSv2;
#endif
		}
		free(buf);
	}

	return ptn;
}

void
set_ptn_titles(menudesc *m, int opt, void *arg)
{
	struct ptn_info *pi = arg;
	struct ptn_size *p;
	int sm = MEG / pm->sectorsize;
	daddr_t size;
	char inc_free[12];

	p = &pi->ptn_sizes[opt];
	if (p->mount[0] == 0) {
		wprintw(m->mw, "%s", msg_string(MSG_add_another_ptn));
		return;
	}
	size = p->size;
	if (p == pi->pool_part)
		snprintf(inc_free, sizeof inc_free, "(%" PRIi64 ")",
		    (size + pi->free_space) / sm);
	else
		inc_free[0] = 0;
	wprintw(m->mw, "%6" PRIi64 "%8s%10" PRIi64 "%10" PRIi64 " %c %s",
		size / sm, inc_free, size / pm->dlcylsize, size,
		p == pi->pool_part ? '+' : ' ', p->mount);
}

void
set_ptn_menu(struct ptn_info *pi)
{
	struct ptn_size *p;
	menu_ent *m;

	for (m = pi->ptn_menus, p = pi->ptn_sizes;; m++, p++) {
		m->opt_name = NULL;
		m->opt_menu = OPT_NOMENU;
		m->opt_flags = 0;
		m->opt_action = set_ptn_size;
		if (p->mount[0] == 0)
			break;
	}
	if (pi->free_parts != 0)
		m++;
	m->opt_name = MSG_askunits;
	m->opt_menu = MENU_sizechoice;
	m->opt_flags = OPT_SUB;
	m->opt_action = NULL;
	m++;

	if (pi->free_space >= 0)
		snprintf(pi->exit_msg, sizeof pi->exit_msg,
			msg_string(MSG_fssizesok),
			(int)(pi->free_space / sizemult), multname, pi->free_parts);
	else
		snprintf(pi->exit_msg, sizeof pi->exit_msg,
			msg_string(MSG_fssizesbad),
			(int)(-pi->free_space / sizemult), multname, (uint) -pi->free_space);

	set_menu_numopts(pi->menu_no, m - pi->ptn_menus);
}

int
set_ptn_size(menudesc *m, void *arg)
{
	struct ptn_info *pi = arg;
	struct ptn_size *p;
	char answer[10];
	char dflt[10];
	char *cp;
	daddr_t size, old_size;
	int mult;

	p = pi->ptn_sizes + m->cursel;

	if (pi->free_parts == 0 && p->size == 0)
		/* Don't allow 'free_parts' to go negative */
		return 0;

	if (p->mount[0] == 0) {
		msg_prompt_win(partman_go?MSG_askfsmountadv:MSG_askfsmount,
			-1, 18, 0, 0, NULL, p->mount, sizeof p->mount);
		if (p->mount[0] == 0)
			return 0;
	}

	size = p->size;
	old_size = size;
	if (size == 0)
		size = p->dflt_size;
	size /= sizemult;
	snprintf(dflt, sizeof dflt, "%" PRIi64 "%s",
	    size, p == pi->pool_part ? "+" : "");

	for (;;) {
		mult = sizemult;
		msg_prompt_win(MSG_askfssize, -1, 18, 0, 0,
			dflt, answer, sizeof answer,
			p->mount, multname);
		/* Some special cases when /usr is first given a size */
		if (old_size == 0 && !strcmp(p->mount, "/usr")) {
			/* Remove space for /usr from / */
			if (!pi->ptn_sizes[0].changed) {
				pi->ptn_sizes[0].size -= p->dflt_size;
				pi->free_space += p->dflt_size;
				pi->ptn_sizes[0].changed = 1;
			}
			/* hack to add free space to default sized /usr */
			if (!strcmp(answer, dflt)) {
				size = p->dflt_size;
				pi->pool_part = p;
				goto adjust_free;
			}
		}
		size = strtoul(answer, &cp, 0);
		switch (*cp++) {
		default:
			continue;
		case 's':
			mult = 1;
			break;
		case 'c':
			mult = pm->dlcylsize;
			break;
		case 'm':
		case 'M':
			mult = MEG / pm->sectorsize;
			break;
		case 'g':
		case 'G':
			mult = 1024 * MEG / pm->sectorsize;
			break;
		case '+':
			cp--;
			if (cp != answer)
				break;
			mult = 1;
			size = old_size;
			break;
		case 0:
			cp--;
			break;
		}
		if (*cp == 0 || *cp == '+')
			break;
	}

	size = NUMSEC(size, mult, pm->dlcylsize);
	if (p->ptn_id == PART_TMP_RAMDISK) {
		p->size = size;
		return 0;
	}
	if (p == pi->pool_part)
		pi->pool_part = NULL;
	if (*cp == '+' && p->limit == 0) {
		pi->pool_part = p;
		if (size == 0)
			size = pm->dlcylsize;
	}
	if (p->limit != 0 && size > p->limit)
		size = p->limit;
    adjust_free:
	if (size != old_size)
		p->changed = 1;
	pi->free_space += old_size - size;
	p->size = size;
	if (size == 0) {
		if (old_size != 0)
			pi->free_parts++;
		if (p->ptn_id == PART_EXTRA)
			memmove(p, p + 1,
				(char *)&pi->ptn_sizes[MAXPARTITIONS]
				- (char *)p);
	} else {
		int f = pi->free_space;
		if (old_size == 0)
			pi->free_parts--;
		if (f < mult && -f < mult) {
			/*
			 * Round size to end of available space,
			 * but keep cylinder alignment
			 */
			if (f < 0)
				f = -roundup(-f, pm->dlcylsize);
			else
				f = rounddown(f, pm->dlcylsize);
			size += f;
			if (size != 0) {
				pi->free_space -= f;
				p->size += f;
			}
		}
	}

	set_ptn_menu(pi);

	return 0;
}

/* Menu to change sizes of /, /usr, /home and etc. partitions */
void
get_ptn_sizes(daddr_t part_start, daddr_t sectors, int no_swap)
{
	int i;
	int maxpart = getmaxpartitions();
	int sm;				/* sectors in 1MB */
	struct ptn_size *p;
	daddr_t size;
	static int swap_created = 0, root_created = 0;

	if (pm->pi.menu_no < 0)
	pm->pi = (struct ptn_info) { -1, {
#define PI_ROOT 0
		{ PART_ROOT,	{ '/', '\0' },
		  DEFROOTSIZE,	DEFROOTSIZE , 0, 0},
#define PI_SWAP 1
		{ PART_SWAP,	{ 's', 'w', 'a', 'p', '\0' },
	 	  DEFSWAPSIZE,	DEFSWAPSIZE, 0, 0 },
		{ PART_TMP_RAMDISK,
#ifdef HAVE_TMPFS
		  { '/', 't', 'm', 'p', ' ', '(', 't', 'm', 'p', 'f', 's', ')', '\0' },
#else
		  { '/', 't', 'm', 'p', ' ', '(', 'm', 'f', 's', ')', '\0' },
#endif
		  64, 0, 0, 0 },
#define PI_USR 3
		{ PART_USR,	{ '/', 'u', 's', 'r', '\0' },	DEFUSRSIZE,
		  0, 0, 0 },
		{ PART_ANY,	{ '/', 'v', 'a', 'r', '\0' },	DEFVARSIZE,
		  0, 0, 0 },
		{ PART_ANY,	{ '/', 'h', 'o', 'm', 'e', '\0' },	0,
		  0, 0, 0 },
	}, {
		{ NULL, OPT_NOMENU, 0, set_ptn_size },
		{ MSG_askunits, MENU_sizechoice, OPT_SUB, NULL },
	}, 0, 0, NULL, { 0 } };

	if (maxpart > MAXPARTITIONS)
		maxpart = MAXPARTITIONS;	/* sanity */

	msg_display(MSG_ptnsizes);
	msg_table_add(MSG_ptnheaders);

	if (pm->pi.menu_no < 0) {
		/* If there is a swap partition elsewhere, don't add one here.*/
		if (no_swap || (swap_created && partman_go)) {
			pm->pi.ptn_sizes[PI_SWAP].size = 0;
		} else {
#if DEFSWAPSIZE == -1
			/* Dynamic swap size. */
			pm->pi.ptn_sizes[PI_SWAP].dflt_size = get_ramsize();
			pm->pi.ptn_sizes[PI_SWAP].size =
			    pm->pi.ptn_sizes[PI_SWAP].dflt_size;
#endif
		}

		/* If installing X increase default size of /usr */
		if (set_X11_selected())
			pm->pi.ptn_sizes[PI_USR].dflt_size += XNEEDMB;

		/* Start of planning to give free space to / */
		pm->pi.pool_part = &pm->pi.ptn_sizes[PI_ROOT];
		/* Make size of root include default size of /usr */
		pm->pi.ptn_sizes[PI_ROOT].size += pm->pi.ptn_sizes[PI_USR].dflt_size;

		sm = MEG / pm->sectorsize;

		if (root_limit != 0) {
			/* Bah - bios can not read all the disk, limit root */
			pm->pi.ptn_sizes[PI_ROOT].limit = root_limit - part_start;
			/* Allocate a /usr partition if bios can't read
			 * everything except swap.
			 */
			if (pm->pi.ptn_sizes[PI_ROOT].limit
			    < sectors - pm->pi.ptn_sizes[PI_SWAP].size * sm) {
				/* Root won't be able to access all the space */
				/* Claw back space for /usr */
				pm->pi.ptn_sizes[PI_USR].size =
						pm->pi.ptn_sizes[PI_USR].dflt_size;
				pm->pi.ptn_sizes[PI_ROOT].size -=
						pm->pi.ptn_sizes[PI_USR].dflt_size;
				pm->pi.ptn_sizes[PI_ROOT].changed = 1;
				/* Give free space to /usr */
				pm->pi.pool_part = &pm->pi.ptn_sizes[PI_USR];
			}
		}

		/* Change preset sizes from MB to sectors */
		pm->pi.free_space = sectors;
		for (p = pm->pi.ptn_sizes; p->mount[0]; p++) {
			p->size = NUMSEC(p->size, sm, pm->dlcylsize);
			p->dflt_size = NUMSEC(p->dflt_size, sm, pm->dlcylsize);
			pm->pi.free_space -= p->size;
		}

		/* Steal space from swap to make things fit.. */
		if (pm->pi.free_space < 0) {
			i = roundup(-pm->pi.free_space, pm->dlcylsize);
			if (i > pm->pi.ptn_sizes[PI_SWAP].size)
				i = pm->pi.ptn_sizes[PI_SWAP].size;
			pm->pi.ptn_sizes[PI_SWAP].size -= i;
			pm->pi.free_space += i;
		}

		/* Add space for 2 system dumps to / (traditional) */
		i = get_ramsize() * sm;
		i = roundup(i, pm->dlcylsize);
		if (pm->pi.free_space > i * 2)
			i *= 2;
		if (pm->pi.free_space > i) {
			pm->pi.ptn_sizes[PI_ROOT].size += i;
			pm->pi.free_space -= i;
		}

		if (root_created && partman_go) {
			pm->pi.ptn_sizes[PI_ROOT].size = 0;
			pm->pi.pool_part = 0;
		}

		/* Ensure all of / is readable by the system boot code */
		i = pm->pi.ptn_sizes[PI_ROOT].limit;
		if (i != 0 && (i -= pm->pi.ptn_sizes[PI_ROOT].size) < 0) {
			pm->pi.ptn_sizes[PI_ROOT].size += i;
			pm->pi.free_space -= i;
		}

		/* Count free partition slots */
		pm->pi.free_parts = 0;
		for (i = 0; i < maxpart; i++) {
			if (pm->bsdlabel[i].pi_size == 0)
				pm->pi.free_parts++;
		}
		for (i = 0; i < MAXPARTITIONS; i++) {
			p = &pm->pi.ptn_sizes[i];
			if (i != 0 && p->ptn_id == 0)
				p->ptn_id = PART_EXTRA;
			if (p->size != 0)
				pm->pi.free_parts--;
		}

		pm->pi.menu_no = new_menu(0, pm->pi.ptn_menus, nelem(pm->pi.ptn_menus),
			3, -1, 12, 70,
			MC_ALWAYS_SCROLL | MC_NOBOX | MC_NOCLEAR,
			NULL, set_ptn_titles, NULL,
			"help", pm->pi.exit_msg);

		if (pm->pi.menu_no < 0)
			return;
	}

	do {
		set_ptn_menu(&pm->pi);
		pm->current_cylsize = pm->dlcylsize;
		process_menu(pm->pi.menu_no, &pm->pi);
	} while (pm->pi.free_space < 0 || pm->pi.free_parts < 0);

	/* Give any cylinder fragment to last partition */
	if (pm->pi.pool_part != NULL || pm->pi.free_space < pm->dlcylsize) {
		for (p = pm->pi.ptn_sizes + nelem(pm->pi.ptn_sizes) - 1; ;p--) {
			if (p->size == 0) {
				if (p == pm->pi.ptn_sizes)
					break;
				continue;
			}
			if (p->ptn_id == PART_TMP_RAMDISK)
				continue;
			p->size += pm->pi.free_space % pm->dlcylsize;
			pm->pi.free_space -= pm->pi.free_space % pm->dlcylsize;
			break;
		}
	}

	for (p = pm->pi.ptn_sizes; p->mount[0]; p++, part_start += size) {
		size = p->size;
		if (p == pm->pi.pool_part) {
			size += rounddown(pm->pi.free_space, pm->dlcylsize);
			if (p->limit != 0 && size > p->limit)
				size = p->limit;
		}
		i = p->ptn_id;
		if (i == PART_TMP_RAMDISK) {
			tmp_ramdisk_size = size;
			size = 0;
			continue;
		}
		if (size == 0)
			continue;
		if (i == PART_ROOT && size > 0)
			root_created = 1;
		if (i == PART_SWAP) {
			if (size > 0)
				swap_created = 1;
			save_ptn(i, part_start, size, FS_SWAP, NULL);
			continue;
		}
		if (!strcmp(p->mount, "raid")) {
			save_ptn(i, part_start, size, FS_RAID, NULL);
			continue;			
		} else if (!strcmp(p->mount, "cgd")) {
			save_ptn(i, part_start, size, FS_CGD, NULL);
			continue;
		}
		save_ptn(i, part_start, size, FS_BSDFFS, p->mount);
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
	daddr_t partstart;
	int part_raw, part_bsd;
	daddr_t ptend;
	int no_swap = 0, valid_part = -1;
	partinfo *p, savedlabel[MAXPARTITIONS];

	if (pm && pm->no_part)
		return 1;

	memcpy(&savedlabel, &pm->bsdlabel, sizeof savedlabel);

	/*
	 * Initialize global variables that track space used on this disk.
	 * Standard 4.4BSD 8-partition labels always cover whole disk.
	 */
	if (pm->ptsize == 0)
		pm->ptsize = pm->dlsize - pm->ptstart;
	if (pm->dlsize == 0)
		pm->dlsize = pm->ptstart + pm->ptsize;
	if (logfp) fprintf(logfp, "dlsize=%" PRId64 " ptsize=%" PRId64
	    " ptstart=%" PRId64 "\n",
	    pm->dlsize, pm->ptsize, pm->ptstart);

	partstart = pm->ptstart;
	ptend = pm->ptstart + pm->ptsize;

	/* Ask for layout type -- standard or special */
	if (partman_go == 0) {
		msg_display(MSG_layout,
		    (int) (pm->ptsize / (MEG / pm->sectorsize)),
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE,
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE + XNEEDMB);
		process_menu(MENU_layout, NULL);
	}

	/* Set so we use the 'real' geometry for rounding, input in MB */
	pm->current_cylsize = pm->dlcylsize;
	set_sizemultname_meg();

	/* Build standard partitions */
	memset(&pm->bsdlabel, 0, sizeof pm->bsdlabel);

	/* Set initial partition types to unused */
	for (part = 0 ; part < maxpart ; ++part)
		pm->bsdlabel[part].pi_fstype = FS_UNUSED;

	/* Whole disk partition */
	part_raw = getrawpartition();
	if (part_raw == -1)
		part_raw = PART_C;	/* for sanity... */
	pm->bsdlabel[part_raw].pi_offset = 0;
	pm->bsdlabel[part_raw].pi_size = pm->dlsize;

	if (part_raw == PART_D) {
		/* Probably a system that expects an i386 style mbr */
		part_bsd = PART_C;
		pm->bsdlabel[PART_C].pi_offset = pm->ptstart;
		pm->bsdlabel[PART_C].pi_size = pm->ptsize;
	} else {
		part_bsd = part_raw;
	}

#if defined(PART_BOOT) && defined(BOOT_SIZE)
	i = BOOT_SIZE;
	if (i >= 1024) {
		/* Treat big numbers as a byte count */
		i = (i + pm->dlcylsize * pm->sectorsize - 1) / (pm->dlcylsize * pm->sectorsize);
		i *= pm->dlcylsize;
	}
#if defined(PART_BOOT_FFS)
	pm->bsdlabel[PART_BOOT].pi_fstype = FS_BSDFFS;
	pm->bsdlabel[PART_BOOT].pi_flags = PIF_NEWFS;
#else
	pm->bsdlabel[PART_BOOT].pi_fstype = FS_BOOT;
#endif
	pm->bsdlabel[PART_BOOT].pi_size = i;
#ifdef BOOT_HIGH
	pm->bsdlabel[PART_BOOT].pi_offset = ptend - i;
	ptend -= i;
#else
	pm->bsdlabel[PART_BOOT].pi_offset = pm->ptstart;
	partstart += i;
#endif
#elif defined(PART_BOOT)
	if (pm->bootsize != 0) {
#if defined(PART_BOOT_MSDOS)
		pm->bsdlabel[PART_BOOT].pi_fstype = FS_MSDOS;
#else
		pm->bsdlabel[PART_BOOT].pi_fstype = FS_BOOT;
#endif
		pm->bsdlabel[PART_BOOT].pi_size = pm->bootsize;
		pm->bsdlabel[PART_BOOT].pi_offset = pm->bootstart;
#if defined(PART_BOOT_PI_FLAGS)
		pm->bsdlabel[PART_BOOT].pi_flags |= PART_BOOT_PI_FLAGS;
#endif
#if defined(PART_BOOT_PI_MOUNT)
		strlcpy(pm->bsdlabel[PART_BOOT].pi_mount, PART_BOOT_PI_MOUNT,
		    sizeof pm->bsdlabel[PART_BOOT].pi_mount);
#endif
	}
#endif /* PART_BOOT w/o BOOT_SIZE */

#if defined(PART_SYSVBFS) && defined(SYSVBFS_SIZE)
	pm->bsdlabel[PART_SYSVBFS].pi_offset = partstart;
	pm->bsdlabel[PART_SYSVBFS].pi_fstype = FS_SYSVBFS;
	pm->bsdlabel[PART_SYSVBFS].pi_size = SYSVBFS_SIZE;
	pm->bsdlabel[PART_SYSVBFS].pi_flags |= PIF_NEWFS | PIF_MOUNT;
	strlcpy(pm->bsdlabel[PART_SYSVBFS].pi_mount, "/stand",
	    sizeof pm->bsdlabel[PART_SYSVBFS].pi_mount);
	partstart += SYSVBFS_SIZE;
#endif

#ifdef PART_REST
	pm->bsdlabel[PART_REST].pi_offset = 0;
	pm->bsdlabel[PART_REST].pi_size = pm->ptstart;
#endif

	if (layoutkind == LY_USEEXIST) {
		/*
		 * If 'pm->oldlabel' is a default label created by the kernel it
		 * will have exactly one valid partition besides raw_part
		 * which covers the whole disk - but might lie outside the
		 * mbr partition we (by now) have offset by a few sectors.
		 * Check for this and and fix ut up.
		 */
		valid_part = -1;
		for (i = 0; i < maxpart; i++) {
			if (i == part_raw)
				continue;
			if (pm->oldlabel[i].pi_size > 0 && PI_ISBSDFS(&pm->oldlabel[i])) {
				if (valid_part >= 0) {
					/* nope, not the default case */
					valid_part = -1;
					break;
				}
				valid_part = i;
			}
		}
		if (valid_part >= 0 && pm->oldlabel[valid_part].pi_offset < pm->ptstart) {
			pm->oldlabel[valid_part].pi_offset = pm->ptstart;
			pm->oldlabel[valid_part].pi_size -= pm->ptstart;
		}
	}

	/*
	 * Save any partitions that are outside the area we are
	 * going to use.
	 * In particular this saves details of the other MBR
	 * partitions on a multiboot i386 system.
	 */
	 for (i = maxpart; i--;) {
		if (pm->bsdlabel[i].pi_size != 0)
			/* Don't overwrite special partitions */
			continue;
		p = &pm->oldlabel[i];
		if (p->pi_fstype == FS_UNUSED || p->pi_size == 0)
			continue;
		if (layoutkind == LY_USEEXIST) {
			if (PI_ISBSDFS(p)) {
				p->pi_flags |= PIF_MOUNT;
				if (layoutkind == LY_USEEXIST && i == valid_part) {
					int fstype = p->pi_fstype;
					p->pi_fstype = 0;
					strcpy(p->pi_mount, "/");
					set_ptype(p, fstype, PIF_NEWFS);
				}
			}
		} else {
			if (p->pi_offset < pm->ptstart + pm->ptsize &&
			    p->pi_offset + p->pi_size > pm->ptstart)
				/* Not outside area we are allocating */
				continue;
			if (p->pi_fstype == FS_SWAP)
				no_swap = 1;
		}
		pm->bsdlabel[i] = pm->oldlabel[i];
	 }

	if (layoutkind == LY_SETNEW)
		get_ptn_sizes(partstart, ptend - partstart, no_swap);
	
	else if (layoutkind == LY_NEWRAID) {
		set_ptype(&(pm->bsdlabel[PART_E]), FS_RAID, 0);
		pm->bsdlabel[PART_E].pi_size = pm->ptsize;
	}
	else if (layoutkind == LY_NEWCGD) {
		set_ptype(&(pm->bsdlabel[PART_E]), FS_CGD, 0);
		pm->bsdlabel[PART_E].pi_size = pm->ptsize;
	}
	else if (layoutkind == LY_NEWLVM) {
		set_ptype(&(pm->bsdlabel[PART_E]), FS_BSDFFS, 0);
		pm->bsdlabel[PART_E].pi_size = pm->ptsize;
		pm->bsdlabel[PART_E].lvmpv = 1;
	}

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 	do {
		if (edit_and_check_label(pm->bsdlabel, maxpart, part_raw, part_bsd) == 0) {
			msg_display(MSG_abort);
			memcpy(&pm->bsdlabel, &savedlabel, sizeof pm->bsdlabel);
			return 0;
		}
	} while (partman_go == 0 && check_partitions() == 0);

	/* Disk name */
	if (!partman_go)
		msg_prompt(MSG_packname, pm->bsddiskname, pm->bsddiskname,
			sizeof pm->bsddiskname);

	/* save label to disk for MI code to update. */
	if (! partman_go)
		(void) savenewlabel(pm->bsdlabel, maxpart);

	/* Everything looks OK. */
	return (1);
}

/*
 * check that there is at least a / somewhere.
 */
int
check_partitions(void)
{
#ifdef HAVE_BOOTXX_xFS
	int rv = 1;
	char *bootxx;
#endif
#ifndef HAVE_UFS2_BOOT
	int fstype;
#endif

#ifdef HAVE_BOOTXX_xFS
	/* check if we have boot code for the root partition type */
	bootxx = bootxx_name();
	if (bootxx != NULL) {
		rv = access(bootxx, R_OK);
		free(bootxx);
	} else
		rv = -1;
	if (rv != 0) {
		process_menu(MENU_ok, __UNCONST(MSG_No_Bootcode));
		return 0;
	}
#endif
#ifndef HAVE_UFS2_BOOT
	fstype = pm->bsdlabel[pm->rootpart].pi_fstype;
	if (fstype == FS_BSDFFS &&
	    (pm->bsdlabel[pm->rootpart].pi_flags & PIF_FFSv2) != 0) {
		process_menu(MENU_ok, __UNCONST(MSG_cannot_ufs2_root));
		return 0;
	}
#endif

	return md_check_partitions();
}
