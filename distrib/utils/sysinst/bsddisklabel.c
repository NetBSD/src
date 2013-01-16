/*	$NetBSD: bsddisklabel.c,v 1.56.4.1 2013/01/16 05:26:11 yamt Exp $	*/

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

static int check_partitions(void);

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

	p = bsdlabel + ptn;
	p->pi_offset = start;
	p->pi_size = size;
	set_ptype(p, fstype, mountpt ? PIF_NEWFS : 0);

	if (mountpt != NULL) {
		for (pp = 0; pp < maxptn; pp++) {
			if (strcmp(bsdlabel[pp].pi_mount, mountpt) == 0)
				bsdlabel[pp].pi_flags &= ~PIF_MOUNT;
		}
		strlcpy(p->pi_mount, mountpt, sizeof p->pi_mount);
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
	}

	return ptn;
}

void
set_ptn_titles(menudesc *m, int opt, void *arg)
{
	struct ptn_info *pi = arg;
	struct ptn_size *p;
	int sm = MEG / sectorsize;
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
		size / sm, inc_free, size / dlcylsize, size,
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
		msg_prompt_win(MSG_askfsmount, -1, 18, 0, 0,
			NULL, p->mount, sizeof p->mount);
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
			mult = dlcylsize;
			break;
		case 'm':
		case 'M':
			mult = MEG / sectorsize;
			break;
		case 'g':
		case 'G':
			mult = 1024 * MEG / sectorsize;
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

	size = NUMSEC(size, mult, dlcylsize);
	if (p->ptn_id == PART_TMP_RAMDISK) {
		p->size = size;
		return 0;
	}
	if (p == pi->pool_part)
		pi->pool_part = NULL;
	if (*cp == '+' && p->limit == 0) {
		pi->pool_part = p;
		if (size == 0)
			size = dlcylsize;
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
				f = -roundup(-f, dlcylsize);
			else
				f = rounddown(f, dlcylsize);
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

void
get_ptn_sizes(daddr_t part_start, daddr_t sectors, int no_swap)
{
	int i;
	int maxpart = getmaxpartitions();
	int sm;				/* sectors in 1MB */
	struct ptn_size *p;
	daddr_t size;

	static struct ptn_info pi = { -1, {
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

	if (pi.menu_no < 0) {
		/* If there is a swap partition elsewhere, don't add one here.*/
		if (no_swap) {
			pi.ptn_sizes[PI_SWAP].size = 0;
		} else {
#if DEFSWAPSIZE == -1
			/* Dynamic swap size. */
			pi.ptn_sizes[PI_SWAP].dflt_size = get_ramsize();
			pi.ptn_sizes[PI_SWAP].size =
			    pi.ptn_sizes[PI_SWAP].dflt_size;
#endif
		}

		/* If installing X increase default size of /usr */
		if (set_X11_selected())
			pi.ptn_sizes[PI_USR].dflt_size += XNEEDMB;

		/* Start of planning to give free space to / */
		pi.pool_part = &pi.ptn_sizes[PI_ROOT];
		/* Make size of root include default size of /usr */
		pi.ptn_sizes[PI_ROOT].size += pi.ptn_sizes[PI_USR].dflt_size;

		sm = MEG / sectorsize;

		if (root_limit != 0) {
			/* Bah - bios can not read all the disk, limit root */
			pi.ptn_sizes[PI_ROOT].limit = root_limit - part_start;
			/* Allocate a /usr partition if bios can't read
			 * everything except swap.
			 */
			if (pi.ptn_sizes[PI_ROOT].limit
			    < sectors - pi.ptn_sizes[PI_SWAP].size * sm) {
				/* Root won't be able to access all the space */
				/* Claw back space for /usr */
				pi.ptn_sizes[PI_USR].size =
						pi.ptn_sizes[PI_USR].dflt_size;
				pi.ptn_sizes[PI_ROOT].size -=
						pi.ptn_sizes[PI_USR].dflt_size;
				pi.ptn_sizes[PI_ROOT].changed = 1;
				/* Give free space to /usr */
				pi.pool_part = &pi.ptn_sizes[PI_USR];
			}
		}

		/* Change preset sizes from MB to sectors */
		pi.free_space = sectors;
		for (p = pi.ptn_sizes; p->mount[0]; p++) {
			p->size = NUMSEC(p->size, sm, dlcylsize);
			p->dflt_size = NUMSEC(p->dflt_size, sm, dlcylsize);
			pi.free_space -= p->size;
		}

		/* Steal space from swap to make things fit.. */
		if (pi.free_space < 0) {
			i = roundup(-pi.free_space, dlcylsize);
			if (i > pi.ptn_sizes[PI_SWAP].size)
				i = pi.ptn_sizes[PI_SWAP].size;
			pi.ptn_sizes[PI_SWAP].size -= i;
			pi.free_space += i;
		}

		/* Add space for 2 system dumps to / (traditional) */
		i = get_ramsize() * sm;
		i = roundup(i, dlcylsize);
		if (pi.free_space > i * 2)
			i *= 2;
		if (pi.free_space > i) {
			pi.ptn_sizes[PI_ROOT].size += i;
			pi.free_space -= i;
		}

		/* Ensure all of / is readable by the system boot code */
		i = pi.ptn_sizes[PI_ROOT].limit;
		if (i != 0 && (i -= pi.ptn_sizes[PI_ROOT].size) < 0) {
			pi.ptn_sizes[PI_ROOT].size += i;
			pi.free_space -= i;
		}

		/* Count free partition slots */
		pi.free_parts = 0;
		for (i = 0; i < maxpart; i++) {
			if (bsdlabel[i].pi_size == 0)
				pi.free_parts++;
		}
		for (i = 0; i < MAXPARTITIONS; i++) {
			p = &pi.ptn_sizes[i];
			if (i != 0 && p->ptn_id == 0)
				p->ptn_id = PART_EXTRA;
			if (p->size != 0)
				pi.free_parts--;
		}

		pi.menu_no = new_menu(0, pi.ptn_menus, nelem(pi.ptn_menus),
			3, -1, 12, 70,
			MC_NOSHORTCUT |
			MC_ALWAYS_SCROLL | MC_NOBOX | MC_NOCLEAR,
			NULL, set_ptn_titles, NULL,
			"help", pi.exit_msg);

		if (pi.menu_no < 0)
			return;
	}

	do {
		set_ptn_menu(&pi);
		current_cylsize = dlcylsize;
		process_menu(pi.menu_no, &pi);
	} while (pi.free_space < 0 || pi.free_parts < 0);

	/* Give any cylinder fragment to last partition */
	if (pi.pool_part != NULL || pi.free_space < dlcylsize) {
		for (p = pi.ptn_sizes + nelem(pi.ptn_sizes) - 1; ;p--) {
			if (p->size == 0) {
				if (p == pi.ptn_sizes)
					break;
				continue;
			}
			if (p->ptn_id == PART_TMP_RAMDISK)
				continue;
			p->size += pi.free_space % dlcylsize;
			break;
		}
	}

	for (p = pi.ptn_sizes; p->mount[0]; p++, part_start += size) {
		size = p->size;
		if (p == pi.pool_part) {
			size += rounddown(pi.free_space, dlcylsize);
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
		if (i == PART_SWAP) {
			save_ptn(i, part_start, size, FS_SWAP, NULL);
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
	partinfo *p;

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
		    (int) (ptsize / (MEG / sectorsize)),
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE,
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE + XNEEDMB);

	process_menu(MENU_layout, NULL);

	/* Set so we use the 'real' geometry for rounding, input in MB */
	current_cylsize = dlcylsize;
	set_sizemultname_meg();

	/* Build standard partitions */
	memset(&bsdlabel, 0, sizeof bsdlabel);

	/* Set initial partition types to unused */
	for (part = 0 ; part < maxpart ; ++part)
		bsdlabel[part].pi_fstype = FS_UNUSED;

	/* Whole disk partition */
	part_raw = getrawpartition();
	if (part_raw == -1)
		part_raw = PART_C;	/* for sanity... */
	bsdlabel[part_raw].pi_offset = 0;
	bsdlabel[part_raw].pi_size = dlsize;

	if (part_raw == PART_D) {
		/* Probably a system that expects an i386 style mbr */
		part_bsd = PART_C;
		bsdlabel[PART_C].pi_offset = ptstart;
		bsdlabel[PART_C].pi_size = ptsize;
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
#elif defined(PART_BOOT)
	if (bootsize != 0) {
		bsdlabel[PART_BOOT].pi_fstype = FS_BOOT;
		bsdlabel[PART_BOOT].pi_size = bootsize;
		bsdlabel[PART_BOOT].pi_offset = bootstart;
#if defined(PART_BOOT_PI_FLAGS)
		bsdlabel[PART_BOOT].pi_flags |= PART_BOOT_PI_FLAGS;
#endif
#if defined(PART_BOOT_PI_MOUNT)
		strlcpy(bsdlabel[PART_BOOT].pi_mount, PART_BOOT_PI_MOUNT,
		    sizeof bsdlabel[PART_BOOT].pi_mount);
#endif
	}
#endif /* PART_BOOT w/o BOOT_SIZE */

#if defined(PART_SYSVBFS) && defined(SYSVBFS_SIZE)
	bsdlabel[PART_SYSVBFS].pi_offset = partstart;
	bsdlabel[PART_SYSVBFS].pi_fstype = FS_SYSVBFS;
	bsdlabel[PART_SYSVBFS].pi_size = SYSVBFS_SIZE;
	bsdlabel[PART_SYSVBFS].pi_flags |= PIF_NEWFS | PIF_MOUNT;
	strlcpy(bsdlabel[PART_SYSVBFS].pi_mount, "/stand",
	    sizeof bsdlabel[PART_SYSVBFS].pi_mount);
	partstart += SYSVBFS_SIZE;
#endif

#ifdef PART_REST
	bsdlabel[PART_REST].pi_offset = 0;
	bsdlabel[PART_REST].pi_size = ptstart;
#endif

	if (layoutkind == 4) {
		/*
		 * If 'oldlabel' is a default label created by the kernel it
		 * will have exactly one valid partition besides raw_part
		 * which covers the whole disk - but might lie outside the
		 * mbr partition we (by now) have offset by a few sectors.
		 * Check for this and and fix ut up.
		 */
		valid_part = -1;
		for (i = 0; i < maxpart; i++) {
			if (i == part_raw)
				continue;
			if (oldlabel[i].pi_size > 0 && PI_ISBSDFS(&oldlabel[i])) {
				if (valid_part >= 0) {
					/* nope, not the default case */
					valid_part = -1;
					break;
				}
				valid_part = i;
			}
		}
		if (valid_part >= 0 && oldlabel[valid_part].pi_offset < ptstart) {
			oldlabel[valid_part].pi_offset = ptstart;
			oldlabel[valid_part].pi_size -= ptstart;
		}
	}

	/*
	 * Save any partitions that are outside the area we are
	 * going to use.
	 * In particular this saves details of the other MBR
	 * partitions on a multiboot i386 system.
	 */
	 for (i = maxpart; i--;) {
		if (bsdlabel[i].pi_size != 0)
			/* Don't overwrite special partitions */
			continue;
		p = &oldlabel[i];
		if (p->pi_fstype == FS_UNUSED || p->pi_size == 0)
			continue;
		if (layoutkind == 4) {
			if (PI_ISBSDFS(p)) {
				p->pi_flags |= PIF_MOUNT;
				if (layoutkind == 4 && i == valid_part) {
					int fstype = p->pi_fstype;
					p->pi_fstype = 0;
					strcpy(p->pi_mount, "/");
					set_ptype(p, fstype, PIF_NEWFS);
				}
			}
		} else {
			if (p->pi_offset < ptstart + ptsize &&
			    p->pi_offset + p->pi_size > ptstart)
				/* Not outside area we are allocating */
				continue;
			if (p->pi_fstype == FS_SWAP)
				no_swap = 1;
		}
		bsdlabel[i] = oldlabel[i];
	 }

	if (layoutkind != 4)
		get_ptn_sizes(partstart, ptend - partstart, no_swap);

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 edit_check:
	if (edit_and_check_label(bsdlabel, maxpart, part_raw, part_bsd) == 0) {
		msg_display(MSG_abort);
		return 0;
	}
	if (check_partitions() == 0)
		goto edit_check;

	/* Disk name */
	msg_prompt(MSG_packname, bsddiskname, bsddiskname, sizeof bsddiskname);

	/* save label to disk for MI code to update. */
	(void) savenewlabel(bsdlabel, maxpart);

	/* Everything looks OK. */
	return (1);
}

/*
 * check that there is at least a / somewhere.
 */
static int
check_partitions(void)
{
#ifdef HAVE_BOOTXX_xFS
	int rv;
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
	}
	if (bootxx == NULL || rv != 0) {
		process_menu(MENU_ok, deconst(MSG_No_Bootcode));
		return 0;
	}
#endif
#ifndef HAVE_UFS2_BOOT
	fstype = bsdlabel[rootpart].pi_fstype;
	if (fstype == FS_BSDFFS &&
	    (bsdlabel[rootpart].pi_flags & PIF_FFSv2) != 0) {
		process_menu(MENU_ok, deconst(MSG_cannot_ufs2_root));
		return 0;
	}
#endif

	return md_check_partitions();
}
