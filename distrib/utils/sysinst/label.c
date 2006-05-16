/*	$NetBSD: label.c,v 1.48 2006/05/16 00:16:59 dogcow Exp $	*/

/*
 * Copyright 1997 Jonathan Stone
 * All rights reserved.
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
 *      Jonathan Stone.
 * 4. The name of Jonathan Stone may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY JONATHAN STONE ``AS IS''
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: label.c,v 1.48 2006/05/16 00:16:59 dogcow Exp $");
#endif

#include <sys/types.h>
#include <stddef.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <util.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <ufs/ffs/fs.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

struct ptn_menu_info {
	int	flags;
#define PIF_SHOW_UNUSED		1
};

/*
 * local prototypes
 */
static int boringpart(partinfo *, int, int, int);

static int	checklabel(partinfo *, int, int, int, int *, int *);
static void	atofsb(const char *, int *, int *);


/*
 * Return 1 if partition i in lp should be ignored when checking
 * for overlapping partitions.
 */
static int
boringpart(partinfo *lp, int i, int rawpart, int bsdpart)
{

	if (i == rawpart || i == bsdpart ||
	    lp[i].pi_fstype == FS_UNUSED || lp[i].pi_size == 0)
		return 1;
	return 0;
}



/*
 * Check a sysinst label structure for overlapping partitions.
 * Returns 0 if no overlapping partition found, nonzero otherwise.
 * Sets reference arguments ovly1 and ovly2 to the indices of
 * overlapping partitions if any are found.
 */
static int
checklabel(partinfo *lp, int nparts, int rawpart, int bsdpart,
	int *ovly1, int *ovly2)
{
	int i;
	int j;

	*ovly1 = -1;
	*ovly2 = -1;

	for (i = 0; i < nparts - 1; i ++ ) {
		partinfo *ip = &lp[i];
		int istart, istop;

		/* skip unused or reserved partitions */
		if (boringpart(lp, i, rawpart, bsdpart))
			continue;

		/*
		 * check succeeding partitions for overlap.
		 * O(n^2), but n is small (currently <= 16).
		 */
		istart = ip->pi_offset;
		istop = istart + ip->pi_size;

		for (j = i+1; j < nparts; j++) {
			partinfo *jp = &lp[j];
			int jstart, jstop;

			/* skip unused or reserved partitions */
			if (boringpart(lp, j, rawpart, bsdpart))
				continue;

			jstart = jp->pi_offset;
			jstop = jstart + jp->pi_size;

			/* overlap? */
			if ((istart <= jstart && jstart < istop) ||
			    (jstart <= istart && istart < jstop)) {
				*ovly1 = i;
				*ovly2 = j;
				return (1);
			}
		}
	}

	return (0);
}

static int
check_one_root(partinfo *lp, int nparts)
{
	int part;
	int foundroot = 0;

	for (part = 0; part < nparts; lp++, part++) {
#if 0
		if (!PI_ISBSDFS(lp))
			continue;
#endif
		if (!(lp->pi_flags & PIF_MOUNT))
			continue;
		if (strcmp(lp->pi_mount, "/") != 0)
			continue;
		if (foundroot)
			/* Duplicate */
			return 0;
		foundroot = 1;
		/* Save partition number, a few things need to know it */
		rootpart = part;
	}
	return foundroot;
}

static int
edit_fs_start(menudesc *m, void *arg)
{
	partinfo *p = arg;
	int start, size;

	start = getpartoff(p->pi_offset);
	size = p->pi_size;
	if (size != 0) {
		/* Try to keep end in the same place */
		size += p->pi_offset - start;
		if (size < 0)
			size = 0;
		p->pi_size = size;
	}
	p->pi_offset = start;
	return 0;
}

static int
edit_fs_size(menudesc *m, void *arg)
{
	partinfo *p = arg;
	int size;

	size = getpartsize(p->pi_offset, p->pi_size);
	if (size == -1)
		size = dlsize - p->pi_offset;
	p->pi_size = size;
	return 0;
}

void
set_ptype(partinfo *p, int fstype, int flag)
{
	p->pi_flags = (p->pi_flags & ~PIF_FFSv2) | flag;

	if (p->pi_fstype == fstype)
		return;

	p->pi_fstype = fstype;
	if (fstype == FS_BSDFFS || fstype == FS_BSDLFS) {
		p->pi_frag = 8;
		/* match newfs defaults for fragments size (2k if >= 1024MB) */
		p->pi_fsize = p->pi_size > 1024*1024*1024 / 512 ? 2048 : 1024;
	} else {
		/* zero - fields not used */
		p->pi_frag = 0;
		p->pi_fsize = 0;
	}
}

void
set_bsize(partinfo *p, int size)
{
	int frags, sz;

	sz = p->pi_fsize;
	if (sz <= 0)
		sz = 512;
	frags = size / sz;
	if (frags > 8)
		frags = 8;
	if (frags <= 0)
		frags = 1;

	p->pi_frag = frags;
	p->pi_fsize = size / frags;
}

void
set_fsize(partinfo *p, int fsize)
{
	int bsz = p->pi_fsize * p->pi_frag;
	int frags = bsz / fsize;

	if (frags > 8)
		frags = 8;
	if (frags <= 0)
		frags = 1;

	p->pi_fsize = fsize;
	p->pi_frag = frags;
}

static int
edit_fs_isize(menudesc *m, void *arg)
{
	partinfo *p = arg;
	char answer[12];

	snprintf(answer, sizeof answer, "%u", p->pi_isize);
	msg_prompt_win(MSG_fs_isize, -1, 18, 0, 0,
		answer, answer, sizeof answer);
	p->pi_isize = atol(answer);
	return 0;
}       


static int
edit_fs_preserve(menudesc *m, void *arg)
{
	partinfo *p = arg;

	p->pi_flags ^= PIF_NEWFS;
	return 0;
}

static int
edit_fs_mount(menudesc *m, void *arg)
{
	partinfo *p = arg;

	p->pi_flags ^= PIF_MOUNT;
	return 0;
}

static int
edit_fs_mountpt(menudesc *m, void *arg)
{
	partinfo *p = arg;

	msg_prompt_win(MSG_mountpoint, -1, 18, 0, 0,
		p->pi_mount, p->pi_mount, sizeof p->pi_mount);

	if (p->pi_mount[0] == ' ' || strcmp(p->pi_mount, "none") == 0) {
		p->pi_mount[0] = 0;
		return 0;
	}

	if (p->pi_mount[0] != '/') {
		memmove(p->pi_mount + 1, p->pi_mount,
		    sizeof p->pi_mount - 1);
		p->pi_mount[sizeof p->pi_mount - 1] = 0;
		p->pi_mount[0] = '/';
	}

	return 0;
}

static int
edit_restore(menudesc *m, void *arg)
{
	partinfo *p = arg;

	p->pi_flags |= PIF_RESET;
	return 1;
}

static int
set_fstype(menudesc *m, void *arg)
{
	partinfo *p = arg;

	if (m->cursel == FS_UNUSED)
		memset(p, 0, sizeof *p);
	else
		p->pi_fstype = m->cursel;
	return 1;
}

static void
get_fstype(menudesc *m, void *arg)
{
	partinfo *p = arg;

	m->cursel = p->pi_fstype;
}

static void set_ptn_header(menudesc *m, void *arg);
static void set_ptn_label(menudesc *m, int opt, void *arg);
int all_fstype_menu = -1;

static int
edit_ptn(menudesc *menu, void *arg)
{
	static menu_ent fs_fields[] = {
#define PTN_MENU_FSKIND		0
	    {NULL, MENU_selfskind, OPT_SUB, NULL},
#define PTN_MENU_START		1
	    {NULL, OPT_NOMENU, 0, edit_fs_start},
#define PTN_MENU_SIZE		2
	    {NULL, OPT_NOMENU, 0, edit_fs_size},
#define PTN_MENU_END		3
	    {NULL, OPT_NOMENU, OPT_IGNORE, NULL},	/* displays 'end' */
#define PTN_MENU_NEWFS		4
	    {NULL, OPT_NOMENU, 0, edit_fs_preserve},
#define PTN_MENU_ISIZE		5
	    {NULL, OPT_NOMENU, 0, edit_fs_isize},
#define PTN_MENU_BSIZE		6
	    {NULL, MENU_selbsize, OPT_SUB, NULL},
#define PTN_MENU_FSIZE		7
	    {NULL, MENU_selfsize, OPT_SUB, NULL},
#define PTN_MENU_MOUNT		8
	    {NULL, OPT_NOMENU, 0, edit_fs_mount},
#define PTN_MENU_MOUNTOPT	9
	    {NULL, MENU_mountoptions, OPT_SUB, NULL},
#define PTN_MENU_MOUNTPT	10
	    {NULL, OPT_NOMENU, 0, edit_fs_mountpt},
	    {MSG_askunits, MENU_sizechoice, OPT_SUB, NULL},
	    {MSG_restore, OPT_NOMENU, 0, edit_restore},
	};
	static int fspart_menu = -1;
	static menu_ent all_fstypes[FSMAXTYPES];
	partinfo *p, p_save;
	int i;

	if (fspart_menu == -1) {
		fspart_menu = new_menu(NULL, fs_fields, nelem(fs_fields),
			0, -1, 0, 70,
			MC_NOBOX | MC_NOCLEAR | MC_SCROLL,
			set_ptn_header, set_ptn_label, NULL,
			NULL, MSG_partition_sizes_ok);
	}

	if (all_fstype_menu == -1) {
		for (i = 0; i < nelem(all_fstypes); i++) {
			all_fstypes[i].opt_name = fstypenames[i];
			all_fstypes[i].opt_menu = OPT_NOMENU;
			all_fstypes[i].opt_flags = 0;
			all_fstypes[i].opt_action = set_fstype;
		}
		all_fstype_menu = new_menu(MSG_Select_the_type,
			all_fstypes, nelem(all_fstypes),
			30, 6, 10, 0, MC_SUBMENU | MC_SCROLL,
			get_fstype, NULL, NULL, NULL, MSG_unchanged);
	}

	p = bsdlabel + menu->cursel;
	p->pi_flags &= ~PIF_RESET;
	p_save = *p;
	for (;;) {
		process_menu(fspart_menu, p);
		if (!(p->pi_flags & PIF_RESET))
			break;
		*p = p_save;
	}

	return 0;
}

static void
set_ptn_header(menudesc *m, void *arg)
{
	partinfo *p = arg;
	int i;
	int t;

	msg_clear();
	msg_table_add(MSG_edfspart, 'a' + (p - bsdlabel));

	/* Determine which of the properties can be changed */
	for (i = PTN_MENU_START; i <= PTN_MENU_MOUNTPT; i++) {
		/* Default to disabled... */
		m->opts[i].opt_flags |= OPT_IGNORE;
		t = p->pi_fstype;
		if (i == PTN_MENU_END)
			/* The 'end address' is calculated from the size */
			continue;
		if (t == FS_UNUSED || (t == FS_SWAP && i > PTN_MENU_END)) {
			/* Nothing after 'size' can be set for swap/unused */
			p->pi_flags &= ~(PIF_NEWFS | PIF_MOUNT);
			p->pi_mount[0] = 0;
			continue;
		}
		if (i == PTN_MENU_NEWFS && t != FS_BSDFFS && t != FS_BSDLFS
		    && t != FS_APPLEUFS) {
			/* Can only newfs UFS and LFS filesystems */
			p->pi_flags &= ~PIF_NEWFS;
			continue;
		}
		if (i >= PTN_MENU_ISIZE && i <= PTN_MENU_FSIZE) {
			/* Parameters for newfs... */
			if (!(p->pi_flags & PIF_NEWFS))
				/* Not if we aren't going to run newfs */
				continue;
			 if (t == FS_APPLEUFS && i != PTN_MENU_ISIZE)
				/* Can only set # inodes for appleufs */
				continue;
			 if (t == FS_BSDLFS && i != PTN_MENU_BSIZE)
				/* LFS doesn't have fragments */
				continue;
		}
		/* Ok: we want this one */
		m->opts[i].opt_flags &= ~OPT_IGNORE;
	}
}

static void
disp_sector_count(menudesc *m, msg fmt, uint s)
{
	uint ms = MEG / sectorsize;

	wprintw(m->mw, msg_string(fmt),
		s / ms, s / dlcylsize, s % dlcylsize ? '*' : ' ', s );
}

static void
set_ptn_label(menudesc *m, int opt, void *arg)
{
	partinfo *p = arg;
	const char *c;

	if (m->opts[opt].opt_flags & OPT_IGNORE
	    && (opt != PTN_MENU_END || p->pi_fstype == FS_UNUSED)) {
		wprintw(m->mw, "            -");
		return;
	}

	switch (opt) {
	case PTN_MENU_FSKIND:
		if (p->pi_fstype == FS_BSDFFS)
			if (p->pi_flags & PIF_FFSv2)
				c = "FFSv2";
			else
				c = "FFSv1";
		else
			c = fstypenames[p->pi_fstype];
		wprintw(m->mw, msg_string(MSG_fstype_fmt), c);
		break;
	case PTN_MENU_START:
		disp_sector_count(m, MSG_start_fmt, p->pi_offset);
		break;
	case PTN_MENU_SIZE:
		disp_sector_count(m, MSG_size_fmt, p->pi_size);
		break;
	case PTN_MENU_END:
		disp_sector_count(m, MSG_end_fmt, p->pi_offset + p->pi_size);
		break;
	case PTN_MENU_NEWFS:
		wprintw(m->mw, msg_string(MSG_newfs_fmt),
			msg_string(p->pi_flags & PIF_NEWFS ? MSG_Yes : MSG_No));
		break;
	case PTN_MENU_ISIZE:
		wprintw(m->mw, msg_string(p->pi_isize > 0 ?
			MSG_isize_fmt : MSG_isize_fmt_dflt), p->pi_isize);
		break;
	case PTN_MENU_BSIZE:
		wprintw(m->mw, msg_string(MSG_bsize_fmt),
			p->pi_fsize * p->pi_frag);
		break;
	case PTN_MENU_FSIZE:
		wprintw(m->mw, msg_string(MSG_fsize_fmt), p->pi_fsize);
		break;
	case PTN_MENU_MOUNT:
		wprintw(m->mw, msg_string(MSG_mount_fmt),
			msg_string(p->pi_flags & PIF_MOUNT ? MSG_Yes : MSG_No));
		break;
	case PTN_MENU_MOUNTOPT:
		wprintw(m->mw, msg_string(MSG_mount_options_fmt));
		if (p->pi_flags & PIF_ASYNC)
			wprintw(m->mw, "async ");
		if (p->pi_flags & PIF_NOATIME)
			wprintw(m->mw, "noatime ");
		if (p->pi_flags & PIF_NODEV)
			wprintw(m->mw, "nodev ");
		if (p->pi_flags & PIF_NODEVMTIME)
			wprintw(m->mw, "nodevmtime ");
		if (p->pi_flags & PIF_NOEXEC)
			wprintw(m->mw, "noexec ");
		if (p->pi_flags & PIF_NOSUID)
			wprintw(m->mw, "nosuid ");
		if (p->pi_flags & PIF_SOFTDEP)
			wprintw(m->mw, "softdep ");
		break;
	case PTN_MENU_MOUNTPT:
		wprintw(m->mw, msg_string(MSG_mountpt_fmt), p->pi_mount);
		break;
	}
}

static int
show_all_unused(menudesc *m, void *arg)
{
	struct ptn_menu_info *pi = arg;

	pi->flags |= PIF_SHOW_UNUSED;
	return 0;
}

static void
set_label_texts(menudesc *menu, void *arg)
{
	struct ptn_menu_info *pi = arg;
	menu_ent *m;
	int ptn, show_unused_ptn;
	int rawptn = getrawpartition();
	int maxpart = getmaxpartitions();

	msg_display(MSG_fspart);
	msg_table_add(MSG_fspart_header, multname, multname, multname);

	for (show_unused_ptn = 0, ptn = 0; ptn < maxpart; ptn++) {
		m = &menu->opts[ptn];
		m->opt_menu = OPT_NOMENU;
		m->opt_name = NULL;
		m->opt_action = edit_ptn;
		if (ptn == rawptn
#ifdef PART_BOOT
		    || ptn == PART_BOOT
#endif
		    || ptn == PART_C) {
			m->opt_flags = OPT_IGNORE;
		} else {
			m->opt_flags = 0;
			if (bsdlabel[ptn].pi_fstype == FS_UNUSED)
				continue;
		}
		show_unused_ptn = ptn + 2;
	}

	if (!(pi->flags & PIF_SHOW_UNUSED) && ptn > show_unused_ptn) {
		ptn = show_unused_ptn;
		m = &menu->opts[ptn];
		m->opt_name = MSG_show_all_unused_partitions;
		m->opt_action = show_all_unused;
		ptn++;
	}

	m = &menu->opts[ptn];
	m->opt_menu = MENU_sizechoice; 
	m->opt_flags = OPT_SUB; 
	m->opt_action = NULL;
	m->opt_name = MSG_askunits;

	menu->numopts = ptn + 1;
}

/*
 * Check a disklabel.
 * If there are overlapping active partitions,
 * Ask the user if they want to edit the partition or give up.
 */
int
edit_and_check_label(partinfo *lp, int nparts, int rawpart, int bsdpart)
{
	static struct menu_ent *menu;
	static int menu_no = -1;
	static struct ptn_menu_info pi;
	int maxpart = getmaxpartitions();

	if (menu == NULL) {
		menu = malloc((maxpart + 1) * sizeof *menu);
		if (!menu)
			return 1;
	}

	if (menu_no == -1) {
		menu_no = new_menu(NULL, menu, maxpart + 1,
			0, -1, maxpart + 2, 74,
			MC_ALWAYS_SCROLL | MC_NOBOX | MC_DFLTEXIT,
			set_label_texts, fmt_fspart, NULL, NULL,
			MSG_partition_sizes_ok);
	}

	if (menu_no < 0)
		return 1;

	pi.flags = 0;
	current_cylsize = dlcylsize;

	for (;;) {
		int i, j;

		/* first give the user the option to edit the label... */
		process_menu(menu_no, &pi);

		/* User thinks the label is OK. */
		/* check we have a single root fs */
		if (check_one_root(lp, nparts) == 0)
			msg_display(MSG_must_be_one_root);
		else 
			/* Check for overlaps */
			if (checklabel(lp, nparts, rawpart, bsdpart, &i, &j))
				/* partitions overlap */
				msg_display(MSG_partitions_overlap,'a'+i,'a'+j);
			else
				return 1;

		/*XXX ???*/
		msg_display_add(MSG_edit_partitions_again);
		process_menu(MENU_yesno, NULL);
		if (!yesno)
			return(0);
	}

	/*NOTREACHED*/
}

/*
 * Read a label from disk into a sysinst label structure.
 */
int
incorelabel(const char *dkname, partinfo *lp)
{
	struct disklabel lab;
	int i, maxpart;
	struct partition *pp;
	int fd;
	char buf[64];

	if (get_real_geom(dkname, &lab) == 0)
		return -1;

	touchwin(stdscr);
	maxpart = getmaxpartitions();
	if (maxpart > MAXPARTITIONS)
		maxpart = MAXPARTITIONS;
	if (maxpart > lab.d_npartitions)
		maxpart = lab.d_npartitions;

	/*
	 * Historically sysinst writes the name to d_typename rather
	 * than d_diskname.  Who knows why, but pull the value back here.
	 */
	if (lab.d_typename[0] != 0 && strcmp(lab.d_typename, "unknown") != 0)
		strlcpy(bsddiskname, lab.d_typename, sizeof bsddiskname);

	fd = opendisk(dkname, O_RDONLY, buf, sizeof buf, 0);
	pp = &lab.d_partitions[0];
	for (i = 0; i < maxpart; lp++, pp++, i++) {
		lp->pi_partition = *pp;
		if (lp->pi_fstype >= FSMAXTYPES)
			lp->pi_fstype = FS_OTHER;
		strlcpy(lp->pi_mount, get_last_mounted(fd, pp->p_offset),
			sizeof lp->pi_mount);
	}
	if (fd != -1)
		close(fd);

	return 0;
}

/*
 * Try to get 'last mounted on' (or equiv) from fs superblock.
 */
const char *
get_last_mounted(int fd, int partstart)
{
	static char sblk[SBLOCKSIZE];		/* is this enough? */
	#define SB ((struct fs *)sblk)
	const static int sblocks[] = SBLOCKSEARCH;
	const int *sbp;
	char *cp;
	const char *mnt = "";
	int l;

	if (fd == -1)
		return mnt;

	/* Check UFS1/2 (and hence LFS) superblock */
	for (sbp = sblocks; *mnt == 0 && *sbp != -1; sbp++) {
		if (pread(fd, sblk, sizeof sblk,
		    partstart * (off_t)512 + *sbp) != sizeof sblk)
			continue;
		/* Maybe we should validate the checksum??? */
		switch (((struct fs *)sblk)->fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
			if (!(SB->fs_old_flags & FS_FLAGS_UPDATED)) {
				if (*sbp == SBLOCK_UFS1)
					mnt = (const char *) SB->fs_fsmnt;
				continue;
			}
			/* FALLTHROUGH */
		case FS_UFS2_MAGIC:
		case FS_UFS2_MAGIC_SWAPPED:
			/* Check we have the main superblock */
			if (SB->fs_sblockloc == *sbp)
				mnt = (const char *) SB->fs_fsmnt;
			continue;
		}

#if 0	/* Requires fs/ext2fs/ext2fs.h which collides badly... */
		if ((struct ext2fs *)sblk)->e2fs_magic == E2FS_MAGIC) {
			mnt = ((struct ext2fs *)sblk)->e2fs_fsmnt;
			continue;
		}
#endif
		if (*sbp != 0)
			continue;

		/* If start of partition check for other fs types */
		if (sblk[0x42] == 0x29 && memcmp(sblk + 0x52, "FAT", 3) == 0) {
			/* Probably a FAT filesystem, report volume name */
			cp = strchr(sblk + 0x47, ' ');
			if (cp == NULL || cp - (sblk + 0x47) > 11)
				cp = sblk + 0x47 + 11;
			*cp = 0;
			return sblk + 0x47;
		}
	}

	/* If sysinst mounted this last then strip prefix */
	l = strlen(targetroot_mnt);
	if (memcmp(mnt, targetroot_mnt, l) == 0) {
		if (mnt[l] == 0)
			return "/";
		if (mnt[l] == '/')
			return mnt + l;
	}
	return mnt;
	#undef SB
}

/* Ask for a partition offset, check bounds and do the needed roundups */
int
getpartoff(int defpartstart)
{
	char defsize[20], isize[20], maxpartc;
	int i, localsizemult, partn;
	const char *errmsg = "\n";

	maxpartc = 'a' + getmaxpartitions() - 1;
	for (;;) {
		snprintf(defsize, sizeof defsize, "%d", defpartstart/sizemult);
		msg_prompt_win(MSG_label_offset, -1, 13, 70, 9,
		    (defpartstart > 0) ? defsize : NULL, isize, sizeof isize,
		    errmsg, maxpartc, maxpartc, multname);
		if (strcmp(defsize, isize) == 0)
			/* Don't do rounding if default accepted */
			return defpartstart;
		if (isize[1] == '\0' && isize[0] >= 'a' &&
		    isize[0] <= maxpartc) {
			partn = isize[0] - 'a';
			i = bsdlabel[partn].pi_size + bsdlabel[partn].pi_offset;
			localsizemult = 1;
		} else if (atoi(isize) == -1) {
			i = ptstart;
			localsizemult = 1;
		} else
			atofsb(isize, &i, &localsizemult);
		if (i < 0) {
			errmsg = msg_string(MSG_invalid_sector_number);
			continue;
		}
		/* round to cylinder size if localsizemult != 1 */
		i = NUMSEC(i/localsizemult, localsizemult, dlcylsize);
		/* Adjust to start of slice if needed */
		if ((i < ptstart && (ptstart - i) < localsizemult) ||
		    (i > ptstart && (i - ptstart) < localsizemult)) {
			i = ptstart;
		}
		if (i <= dlsize)
			break;
		errmsg = msg_string(MSG_startoutsidedisk);
	}
	return i;
}


/* Ask for a partition size, check bounds and do the needed roundups */
int
getpartsize(int partstart, int defpartsize)
{
	char dsize[20], isize[20], maxpartc;
	const char *errmsg = "\n";
	int i, partend, localsizemult;
	int fsptend = ptstart + ptsize;
	int partn;

	maxpartc = 'a' + getmaxpartitions() - 1;
	for (;;) {
		snprintf(dsize, sizeof dsize, "%d", defpartsize/sizemult);
		msg_prompt_win(MSG_label_size, -1, 12, 70, 9,
		    (defpartsize != 0) ? dsize : 0, isize, sizeof isize,
		    errmsg, maxpartc, multname);
		if (strcmp(isize, dsize) == 0)
			return defpartsize;
		if (isize[1] == '\0' && isize[0] >= 'a' &&
		    isize[0] <= maxpartc) {
			partn = isize[0] - 'a';
			i = bsdlabel[partn].pi_offset - partstart;
			localsizemult = 1;
		} else if (atoi(isize) == -1) {
			i = fsptend - partstart;
			localsizemult = 1;
		} else
			atofsb(isize, &i, &localsizemult);
		if (i < 0) {
			errmsg = msg_string(MSG_invalid_sector_number);
			continue;
		}
		/*
		 * partend is aligned to a cylinder if localsizemult
		 * is not 1 sector
		 */
		partend = NUMSEC((partstart + i) / localsizemult,
		    localsizemult, dlcylsize);
		/* Align to end-of-disk or end-of-slice if close enough */
		i = dlsize - partend;
		if (i > -localsizemult && i < localsizemult)
			partend = dlsize;
		i = fsptend - partend;
		if (i > -localsizemult && i < localsizemult)
			partend = fsptend;
		/* sanity checks */
		if (partend > dlsize) {
			partend = dlsize;
			msg_prompt_win(MSG_endoutsidedisk, -1, 13, 70, 6,
			    NULL, isize, 1,
			    (partend - partstart) / sizemult, multname);
		}
		/* return value */
		return (partend - partstart);
	}
	/* NOTREACHED */
}

/*
 * convert a string to a number of sectors, with a possible unit
 * 150M = 150 Megabytes
 * 2000c = 2000 cylinders
 * 150256s = 150256 sectors
 * Without units, use the default (sizemult)
 * returns the number of sectors, and the unit used (for roundups).
 */

static void
atofsb(const char *str, int *p_val, int *localsizemult)
{
	int i;
	int val;

	*localsizemult = sizemult;
	if (str[0] == '\0') {
		*p_val = -1;
		return;
	}
	val = 0;
	for (i = 0; str[i] != '\0'; i++) {
		if (str[i] >= '0' && str[i] <= '9') {
			val = val * 10 + str[i] - '0';
			continue;
		}
		if (str[i + 1] != '\0') {
			/* A non-digit caracter, not at the end */
			*p_val = -1;
			return;
		}
		if (str[i] == 'G' || str[i] == 'g') {
			val *= 1024;
			*localsizemult = MEG / sectorsize;
			break;
		}
		if (str[i] == 'M' || str[i] == 'm') {
			*localsizemult = MEG / sectorsize;
			break;
		}
		if (str[i] == 'c') {
			*localsizemult = dlcylsize;
			break;
		}
		if (str[i] == 's') {
			*localsizemult = 1;
			break;
		}
		/* not a known unit */
		*p_val = -1;
		return;
	}
	*p_val = val * (*localsizemult);
	return;
}
