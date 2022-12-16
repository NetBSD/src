/*	$NetBSD: bsddisklabel.c,v 1.70 2022/12/16 19:49:13 martin Exp $	*/

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
#include <assert.h>
#include <stdio.h>
#include <stddef.h>
#include <util.h>
#include <dirent.h>
#include "defs.h"
#include "md.h"
#include "defsizes.h"
#include "endian.h"
#include "msg_defs.h"
#include "menu_defs.h"

static size_t fill_ptn_menu(struct partition_usage_set *pset);

/*
 * The default partition layout.
 */
static const struct part_usage_info
default_parts_init[] =
{
	/*
	 * Pretty complex setup for boot partitions.
	 * This is copy&pasted below, please keep in sync!
	 */
#ifdef PART_BOOT
	{ .size = PART_BOOT/512,	/* PART_BOOT is in BYTE, not MB! */
#ifdef PART_BOOT_MOUNT
	  .mount = PART_BOOT_MOUNT,
	  .instflags = PUIINST_MOUNT|PUIINST_BOOT,
#else
	  .instflags = PUIINST_BOOT,
#endif
#ifdef PART_BOOT_TYPE
	  .fs_type = PART_BOOT_TYPE,
#if (PART_BOOT_TYPE == FS_MSDOS) || (PART_BOOT_TYPE == FS_EX2FS)
	  .flags = PUIFLAG_ADD_OUTER,
#endif
#endif
#ifdef PART_BOOT_SUBT
	  .fs_version = PART_BOOT_SUBT,
#endif
	},
#endif

	/*
	 * Two more copies of above for _BOOT1 and _BOOT2, please
	 * keep in sync!
	 */
#ifdef PART_BOOT1
	{ .size = PART_BOOT1/512,	/* PART_BOOT1 is in BYTE, not MB! */
#ifdef PART_BOOT1_MOUNT
	  .mount = PART_BOOT1_MOUNT,
	  .instflags = PUIINST_MOUNT|PUIINST_BOOT,
#else
	  .instflags = PUIINST_MOUNT|PUIINST_BOOT,
#endif
#ifdef PART_BOOT1_TYPE
	  .fs_type = PART_BOOT1_TYPE,
#if (PART_BOOT1_TYPE == FS_MSDOS) || (PART_BOOT1_TYPE == FS_EX2FS)
	  .flags = PUIFLAG_ADD_OUTER,
#endif
#endif
#ifdef PART_BOOT1_SUBT
	  .fs_version = PART_BOOT1_SUBT,
#endif
	},
#endif
#ifdef PART_BOOT2
	{ .size = PART_BOOT2/512,	/* PART_BOOT2 is in BYTE, not MB! */
#ifdef PART_BOOT2_MOUNT
	  .mount = PART_BOOT2_MOUNT,
	  .instflags = PUIINST_MOUNT|PUIINST_BOOT,
#else
	  .instflags = PUIINST_MOUNT|PUIINST_BOOT,
#endif
#ifdef PART_BOOT2_TYPE
	  .fs_type = PART_BOOT2_TYPE,
#if (PART_BOOT2_TYPE == FS_MSDOS) || (PART_BOOT2_TYPE == FS_EX2FS)
	  .flags = PUIFLAG_ADD_OUTER,
#endif
#endif
#ifdef PART_BOOT2_SUBT
	  .fs_version = PART_BOOT2_SUBT,
#endif
	},
#endif

	{ .size = DEFROOTSIZE*(MEG/512), .mount = "/", .type = PT_root,
	  .flags = PUIFLAG_EXTEND },
	{
#if DEFSWAPSIZE > 0
	  .size = DEFSWAPSIZE*(MEG/512),
#endif
	  .type = PT_swap, .fs_type = FS_SWAP },
#ifdef HAVE_TMPFS
	{ .type = PT_root, .mount = "/tmp", .fs_type = FS_TMPFS,
	  .flags = PUIFLG_JUST_MOUNTPOINT },
#else
	{ .type = PT_root, .mount = "/tmp", .fs_type = FS_MFS,
	  .flags = PUIFLG_JUST_MOUNTPOINT },
#endif
	{ .def_size = DEFUSRSIZE*(MEG/512), .mount = "/usr", .type = PT_root,
	  .fs_type = FS_BSDFFS, .fs_version = 3 },
	{ .def_size = DEFVARSIZE*(MEG/512), .mount = "/var", .type = PT_root,
	  .fs_type = FS_BSDFFS, .fs_version = 3 },
};

static const char size_separator[] =
    "----------------------------------- - --------------------";
static char size_menu_title[STRSIZE];
static char size_menu_exit[MENUSTRSIZE];

static void
set_pset_exit_str(struct partition_usage_set *pset)
{
	char *str, num[25];
	const char *args[2];
	bool overrun;
	daddr_t free_space = pset->cur_free_space;

	/* format exit string */
	overrun = free_space < 0;
	if (overrun)
		free_space = -free_space;

	snprintf(num, sizeof(num), "%" PRIu64, free_space / sizemult);
	args[0] = num;
	args[1] = multname;
	str = str_arg_subst(
	    msg_string(overrun ? MSG_fssizesbad : MSG_fssizesok),
	    2, args);
	strlcpy(size_menu_exit, str, sizeof(size_menu_exit));
	free(str);
}

static void
draw_size_menu_header(menudesc *m, void *arg)
{
	struct partition_usage_set *pset = arg;
	size_t i;
	char col1[70], desc[MENUSTRSIZE];
	bool need_ext = false, need_existing = false;

	msg_display(MSG_ptnsizes);

	for (i = 0; i < pset->num; i++) {
		if (pset->infos[i].flags & PUIFLG_IS_OUTER)
			need_ext = true;
		else if (pset->infos[i].cur_part_id != NO_PART)
			need_existing = true;
	}
	if (need_ext && need_existing)
		snprintf(desc, sizeof desc, "%s, %s",
		    msg_string(MSG_ptnsizes_mark_existing),
		    msg_string(MSG_ptnsizes_mark_external));
	else if (need_existing)
		strlcpy(desc, msg_string(MSG_ptnsizes_mark_existing),
		    sizeof desc);
	else if (need_ext)
		strlcpy(desc, msg_string(MSG_ptnsizes_mark_external),
		    sizeof desc);
	if (need_ext || need_existing) {
		msg_printf("\n");
		msg_display_add_subst(msg_string(MSG_ptnsizes_markers),
		    1, &desc);
	}
	msg_printf("\n\n");

	/* update menu title */
	snprintf(col1, sizeof col1, "%s (%s)", msg_string(MSG_ptnheaders_size),
	    multname);
	snprintf(size_menu_title, sizeof size_menu_title,
	    "   %-37.37s %s\n   %s", col1,
	    msg_string(MSG_ptnheaders_filesystem), size_separator);
}

static void
draw_size_menu_line(menudesc *m, int opt, void *arg)
{
	struct partition_usage_set *pset = arg;
	daddr_t size;
	char psize[38], inc_free[16], flag, swap[40];
	const char *mount;
	bool free_mount = false;

	if (opt < 0 || (size_t)opt >= pset->num)
		return;

	inc_free[0] = 0;
	if ((pset->infos[opt].flags & PUIFLAG_EXTEND) &&
	     pset->cur_free_space > 0) {
		size = pset->infos[opt].size + pset->cur_free_space;
		snprintf(inc_free, sizeof inc_free, " (%" PRIu64 ")",
		    size / sizemult);
	}
	size = pset->infos[opt].size;
	if (pset->infos[opt].fs_type == FS_TMPFS) {
		if (pset->infos[opt].size < 0)
			snprintf(psize, sizeof psize, "%" PRIu64 "%%", -size);
		else
			snprintf(psize, sizeof psize, "%" PRIu64 " %s", size,
			    msg_string(MSG_megname));
	} else {
		snprintf(psize, sizeof psize, "%" PRIu64 "%s",
		    size / sizemult, inc_free);
	}

	if (pset->infos[opt].type == PT_swap) {
		snprintf(swap, sizeof swap, "<%s>",
		    msg_string(MSG_swap_display));
		mount = swap;
	} else if (pset->infos[opt].flags & PUIFLG_JUST_MOUNTPOINT) {
		snprintf(swap, sizeof swap, "%s (%s)",
		    pset->infos[opt].mount,
		    getfslabelname(pset->infos[opt].fs_type,
		    pset->infos[opt].fs_version));
		mount = swap;
	} else if (pset->infos[opt].mount[0]) {
		if (pset->infos[opt].instflags & PUIINST_BOOT) {
			snprintf(swap, sizeof swap, "%s <%s>",
			    pset->infos[opt].mount, msg_string(MSG_ptn_boot));
			mount = swap;
		} else {
			mount = pset->infos[opt].mount;
		}
#ifndef NO_CLONES
	} else if (pset->infos[opt].flags & PUIFLG_CLONE_PARTS) {
		snprintf(swap, sizeof swap, "%zu %s",
		    pset->infos[opt].clone_src->num_sel,
		    msg_string(MSG_clone_target_disp));
		mount = swap;
#endif
	} else {
		mount = NULL;
		if (pset->infos[opt].parts->pscheme->other_partition_identifier
		    && pset->infos[opt].cur_part_id != NO_PART)
			mount = pset->infos[opt].parts->pscheme->
			    other_partition_identifier(pset->infos[opt].parts,
			    pset->infos[opt].cur_part_id);
		if (mount == NULL)
			mount = getfslabelname(pset->infos[opt].fs_type,
			    pset->infos[opt].fs_version);
		if (pset->infos[opt].instflags & PUIINST_BOOT) {
			snprintf(swap, sizeof swap, "%s <%s>",
			    mount, msg_string(MSG_ptn_boot));
			mount = swap;
		}
		mount = str_arg_subst(msg_string(MSG_size_ptn_not_mounted),
		    1, &mount);
		free_mount = true;
	}
	flag = ' ';
	if (pset->infos[opt].flags & PUIFLAG_EXTEND)
		flag = '+';
	else if (pset->infos[opt].flags & PUIFLG_IS_OUTER)
		flag = '@';
	else if (pset->infos[opt].cur_part_id != NO_PART)
		flag = '=';
	wprintw(m->mw, "%-35.35s %c %s", psize, flag, mount);
	if (free_mount)
		free(__UNCONST(mount));

	if (opt == 0)
		set_pset_exit_str(pset);
}

static int
add_other_ptn_size(menudesc *menu, void *arg)
{
	struct partition_usage_set *pset = arg;
	struct part_usage_info *p;
	struct menu_ent *m;
	char new_mp[MOUNTLEN], *err;
	const char *args;

	for (;;) {
		msg_prompt_win(partman_go?MSG_askfsmountadv:MSG_askfsmount,
		    -1, 18, 0, 0, NULL,  new_mp, sizeof(new_mp));
		if (new_mp[0] == 0)
			return 0;
		if (new_mp[0] != '/') {
			/* we need absolute mount paths */
			memmove(new_mp+1, new_mp, sizeof(new_mp)-1);
			new_mp[0] = '/';
		}

		/* duplicates? */
		bool duplicate = false;
		for (size_t i = 0; i < pset->num; i++) {
			if (strcmp(pset->infos[i].mount,
			    new_mp) == 0) {
			    	args = new_mp;
				err = str_arg_subst(
				    msg_string(MSG_mp_already_exists),
				    1, &args);
				err_msg_win(err);
				free(err);
				duplicate = true;
				break;
			}
		}
		if (!duplicate)
			break;
	}

	m = realloc(pset->menu_opts, (pset->num+5)*sizeof(*pset->menu_opts));
	if (m == NULL)
		return 0;
	p = realloc(pset->infos, (pset->num+1)*sizeof(*pset->infos));
	if (p == NULL)
		return 0;

	pset->infos = p;
	pset->menu_opts = m;
	menu->opts = m;
	menu->numopts = pset->num+4;
	m += pset->num;
	p += pset->num;
	memset(m, 0, sizeof(*m));
	memset(p, 0, sizeof(*p));
	p->parts = pset->parts;
	p->cur_part_id = NO_PART;
	p->type = PT_root;
	p->fs_type = FS_BSDFFS;
	p->fs_version = 3;
	strncpy(p->mount, new_mp, sizeof(p->mount));

	menu->cursel = pset->num;
	pset->num++;
	fill_ptn_menu(pset);

	return -1;
}

#ifndef NO_CLONES
static int
inst_ext_clone(menudesc *menu, void *arg)
{
	struct selected_partitions selected;
	struct clone_target_menu_data data;
	struct partition_usage_set *pset = arg;
	struct part_usage_info *p;
	menu_ent *men;
	int num_men, i;

	if (!select_partitions(&selected, pm->parts))
		return 0;

	num_men = pset->num+1;
	men = calloc(num_men, sizeof *men);
	if (men == NULL)
		return 0;
	for (i = 0; i < num_men; i++)
		men[i].opt_action = clone_target_select;
	men[num_men-1].opt_name = MSG_clone_target_end;

	memset(&data, 0, sizeof data);
	data.usage = *pset;
	data.res = -1;

	data.usage.menu = new_menu(MSG_clone_target_hdr,
	    men, num_men, 3, 2, 0, 65, MC_SCROLL,
	    NULL, draw_size_menu_line, NULL, NULL, MSG_cancel);
	process_menu(data.usage.menu, &data);
	free_menu(data.usage.menu);
	free(men);

	if (data.res < 0)
		goto err;

	/* insert clone record */
	men = realloc(pset->menu_opts, (pset->num+5)*sizeof(*pset->menu_opts));
	if (men == NULL)
		goto err;
	pset->menu_opts = men;
	menu->opts = men;
	menu->numopts = pset->num+4;

	p = realloc(pset->infos, (pset->num+1)*sizeof(*pset->infos));
	if (p == NULL)
		goto err;
	pset->infos = p;

	men += data.res;
	p += data.res;
	memmove(men+1, men, sizeof(*men)*((pset->num+4)-data.res));
	memmove(p+1, p, sizeof(*p)*((pset->num)-data.res));
	memset(men, 0, sizeof(*men));
	memset(p, 0, sizeof(*p));
	p->flags = PUIFLG_CLONE_PARTS;
	p->cur_part_id = NO_PART;
	p->clone_src = malloc(sizeof(selected));
	if (p->clone_src != NULL) {
		*p->clone_src = selected;
		p->clone_ndx = ~0U;
		p->size = selected_parts_size(&selected);
		p->parts = pset->parts;
	} else {
		p->clone_ndx = 0;
		free_selected_partitions(&selected);
	}

	menu->cursel = data.res == 0 ? 1 : 0;
	pset->num++;
	fill_ptn_menu(pset);

	return -1;

err:
	free_selected_partitions(&selected);
	return 0;
}
#endif

static size_t
fill_ptn_menu(struct partition_usage_set *pset)
{
	struct part_usage_info *p;
	struct disk_part_info info;
	menu_ent *m;
	size_t i;
	daddr_t free_space;

#ifdef NO_CLONES
#define	ADD_ITEMS	3
#else
#define	ADD_ITEMS	4
#endif

	memset(pset->menu_opts, 0, (pset->num+ADD_ITEMS)
	    *sizeof(*pset->menu_opts));
	for (m = pset->menu_opts, p = pset->infos, i = 0; i < pset->num;
	    m++, p++, i++) {
		if (p->flags & PUIFLG_CLONE_PARTS)
			m->opt_flags = OPT_IGNORE|OPT_NOSHORT;
		else
		m->opt_action = set_ptn_size;
	}

	m->opt_name = size_separator;
	m->opt_flags = OPT_IGNORE|OPT_NOSHORT;
	m++;

	m->opt_name = MSG_add_another_ptn;
	m->opt_action = add_other_ptn_size;
	m++;

#ifndef NO_CLONES
	m->opt_name = MSG_clone_from_elsewhere;
	m->opt_action = inst_ext_clone;
	m++;
#endif

	m->opt_name = MSG_askunits;
	m->opt_menu = MENU_sizechoice;
	m->opt_flags = OPT_SUB;
	m++;

	/* calculate free space */
	free_space = pset->parts->free_space - pset->reserved_space;
	for (i = 0; i < pset->parts->num_part; i++) {
		if (!pset->parts->pscheme->get_part_info(pset->parts, i,
		    &info))
			continue;
		if (info.flags & (PTI_SEC_CONTAINER|PTI_WHOLE_DISK|
		    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
			continue;
		free_space += info.size;
	}
	for (i = 0; i < pset->num; i++) {
		if (pset->infos[i].flags &
		    (PUIFLG_IS_OUTER|PUIFLG_JUST_MOUNTPOINT))
			continue;
		free_space -= pset->infos[i].size;
	}
	pset->cur_free_space = free_space;
	set_pset_exit_str(pset);

	if (pset->menu >= 0)
		set_menu_numopts(pset->menu, m - pset->menu_opts);

	return m - pset->menu_opts;
}

static part_id
find_part_at(struct disk_partitions *parts, daddr_t start)
{
	size_t i;
	struct disk_part_info info;

	for (i = 0; i < parts->num_part; i++) {
		if (!parts->pscheme->get_part_info(parts, i, &info))
			continue;
		if (info.start == start)
			return i;
	}

	return NO_PART;
}

static daddr_t
parse_ram_size(const char *str, bool *is_percent)
{
	daddr_t val;
	char *cp;

	val = strtoull(str, &cp, 10);
	while (*cp && isspace((unsigned char)*cp))
		cp++;

	*is_percent = *cp == '%';
	return val;
}

int
set_ptn_size(menudesc *m, void *arg)
{
	struct partition_usage_set *pset = arg;
	struct part_usage_info *p = &pset->infos[m->cursel];
	char answer[16], dflt[16];
	const char *err_msg;
	size_t i, root = ~0U;
	daddr_t size, old_size, new_size_val, mult;
	int rv;
	bool non_zero, extend, is_ram_size, is_percent = false;

	if (pset->cur_free_space == 0 && p->size == 0 &&
	    !(p->flags & PUIFLG_JUST_MOUNTPOINT))
		/* Don't allow 'free_parts' to go negative */
		return 0;

	if (p->cur_part_id != NO_PART) {
		rv = 0;
		process_menu(MENU_ptnsize_replace_existing_partition, &rv);
		if (rv == 0)
			return 0;
		if (!pset->parts->pscheme->delete_partition(pset->parts,
		    p->cur_part_id, &err_msg)) {
			if (err_msg)
				err_msg_win(err_msg);
			return 0;
		}
		p->cur_part_id = NO_PART;
		/*
		 * All other part ids are invalid now too - update them!
		 */
		for (i = 0; i < pset->num; i++) {
			if (pset->infos[i].cur_part_id == NO_PART)
				continue;
			pset->infos[i].cur_part_id =
			    find_part_at(pset->parts, pset->infos[i].cur_start);
		}
	}

	is_ram_size = (p->flags & PUIFLG_JUST_MOUNTPOINT)
	    && p->fs_type == FS_TMPFS;

	size = p->size;
	if (is_ram_size && size < 0) {
		is_percent = true;
		size = -size;
	}
	old_size = size;
	if (size == 0)
		size = p->def_size;
	if (!is_ram_size)
		size /= sizemult;

	if (is_ram_size) {
		snprintf(dflt, sizeof dflt, "%" PRIu64 "%s",
		    size, is_percent ? "%" : "");
	} else {
		snprintf(dflt, sizeof dflt, "%" PRIu64 "%s",
		    size, p->flags & PUIFLAG_EXTEND ? "+" : "");
	}

	for (;;) {
		msg_fmt_prompt_win(MSG_askfssize, -1, 18, 0, 0,
		    dflt, answer, sizeof answer, "%s%s", p->mount,
		    is_ram_size ? msg_string(MSG_megname) : multname);

		if (is_ram_size) {
			new_size_val = parse_ram_size(answer, &is_percent);
			if (is_percent &&
			    (new_size_val < 0 || new_size_val > 100))
				continue;
			if (!is_percent && new_size_val < 0)
				continue;
			size = new_size_val;
			extend = false;
			break;
		}
		mult = sizemult;
		new_size_val = parse_disk_pos(answer, &mult, pm->sectorsize,
		    pm->dlcylsize, &extend);

		if (strcmp(answer, dflt) == 0)
			non_zero = p->def_size > 0;
		else
			non_zero = new_size_val > 0;

		/* Some special cases when /usr is first given a size */
		if (old_size == 0 && non_zero &&
		    strcmp(p->mount, "/usr") == 0) {
			for (i = 0; i < pset->num; i++) {
				if (strcmp(pset->infos[i].mount, "/") == 0) {
					root = i;
					break;
				}
			}
			/* Remove space for /usr from / */
			if (root < pset->num &&
			     pset->infos[root].cur_part_id == NO_PART &&
			     pset->infos[root].size ==
					pset->infos[root].def_size) {
				/*
				 * root partition does not yet exist and
				 * has default size
				 */
				pset->infos[root].size -= p->def_size;
				pset->cur_free_space += p->def_size;
			}
			/*
			 * hack to add free space to /usr if
			 * previously / got it
			 */
			if (pset->infos[root].flags & PUIFLAG_EXTEND)
				extend = true;
		}
		if (new_size_val < 0)
			continue;
		size = new_size_val;
		break;
	}

	daddr_t align = pset->parts->pscheme->get_part_alignment(pset->parts);
	if (!is_ram_size) {
		size = NUMSEC(size, mult, align);
	}
	if (p->flags & PUIFLAG_EXTEND)
		p->flags &= ~PUIFLAG_EXTEND;
	if (extend && (p->limit == 0 || p->limit > p->size)) {
		for (size_t k = 0; k < pset->num; k++)
			pset->infos[k].flags &= ~PUIFLAG_EXTEND;
		p->flags |= PUIFLAG_EXTEND;
		if (size == 0)
			size = align;
	}
	if (p->limit != 0 && size > p->limit)
		size = p->limit;
	if ((p->flags & (PUIFLG_IS_OUTER|PUIFLG_JUST_MOUNTPOINT)) == 0)
		pset->cur_free_space += p->size - size;
	p->size = is_percent ? -size : size;
	set_pset_exit_str(pset);

	return 0;
}

/*
 * User interface to edit a "wanted" partition layout "pset" as first
 * abstract phase (not concrete partitions).
 * Make sure to have everything (at least theoretically) fit the
 * available space.
 * During editing we keep the part_usage_info and the menu_opts
 * in pset in sync, that is: we always allocate just enough entries
 * in pset->infos as we have usage infos in the list (pset->num),
 * and two additional menu entries ("add a partition" and "select units").
 * The menu exit string changes depending on content, and implies
 * abort while the partition set is not valid (does not fit).
 * Return true when the user wants to continue (by editing the concrete
 * partitions), return false to abort.
 */
bool
get_ptn_sizes(struct partition_usage_set *pset)
{
	size_t num;

	wclear(stdscr);
	wrefresh(stdscr);

	if (pset->menu_opts == NULL)
		pset->menu_opts = calloc(pset->num+4, sizeof(*pset->menu_opts));

	pset->menu = -1;
	num = fill_ptn_menu(pset);

	pset->menu = new_menu(size_menu_title, pset->menu_opts, num,
			3, -1, 12, 70,
			MC_ALWAYS_SCROLL|MC_NOBOX|MC_NOCLEAR|MC_CONTINUOUS,
			draw_size_menu_header, draw_size_menu_line, NULL,
			NULL, size_menu_exit);

	if (pset->menu < 0) {
		free(pset->menu_opts);
		pset->menu_opts = NULL;
		return false;
	}

	pset->ok = true;
	process_menu(pset->menu, pset);

	free_menu(pset->menu);
	free(pset->menu_opts);
	pset->menu = -1;
	pset->menu_opts = NULL;

	return pset->ok;
}

static int
set_keep_existing(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_KEEPEXISTING;
	return 0;
}

static int
set_switch_scheme(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_OTHERSCHEME;
	return 0;
}

static int
set_edit_part_sizes(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_SETSIZES;
	return 0;
}

static int
set_use_default_sizes(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_USEDEFAULT;
	return 0;
}

static int
set_use_empty_parts(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_USENONE;
	return 0;
}

/*
 * Check if there is a reasonable pre-existing partition for
 * NetBSD.
 */
static bool
check_existing_netbsd(struct disk_partitions *parts)
{
	size_t nbsd_parts;
	struct disk_part_info info;

	nbsd_parts = 0;
	for (part_id p = 0; p < parts->num_part; p++) {
		if (!parts->pscheme->get_part_info(parts, p, &info))
			continue;
		if (info.flags & (PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
			continue;
		if (info.nat_type && info.nat_type->generic_ptype == PT_root)
			nbsd_parts++;
	}

	return nbsd_parts > 0;
}

/*
 * Query a partition layout type (with available options depending on
 * pre-existing partitions).
 */
static enum layout_type
ask_layout(struct disk_partitions *parts, bool have_existing)
{
	arg_rep_int ai;
	const char *args[2];
	int menu;
	size_t num_opts;
	menu_ent options[5], *opt;

	args[0] = msg_string(parts->pscheme->name);
	args[1] = msg_string(parts->pscheme->short_name);
	ai.args.argv = args;
	ai.args.argc = 2;
	ai.rv = LY_ERROR;

	memset(options, 0, sizeof(options));
	num_opts = 0;
	opt = &options[0];

	if (have_existing) {
		opt->opt_name = MSG_Keep_existing_partitions;
		opt->opt_flags = OPT_EXIT;
		opt->opt_action = set_keep_existing;
		opt++;
		num_opts++;
	}
	opt->opt_name = MSG_Set_Sizes;
	opt->opt_flags = OPT_EXIT;
	opt->opt_action = set_edit_part_sizes;
	opt++;
	num_opts++;

	opt->opt_name = MSG_Use_Default_Parts;
	opt->opt_flags = OPT_EXIT;
	opt->opt_action = set_use_default_sizes;
	opt++;
	num_opts++;

	opt->opt_name = MSG_Use_Empty_Parts;
	opt->opt_flags = OPT_EXIT;
	opt->opt_action = set_use_empty_parts;
	opt++;
	num_opts++;

	if (num_available_part_schemes > 1 &&
	    parts->parent == NULL) {
		opt->opt_name = MSG_Use_Different_Part_Scheme;
		opt->opt_flags = OPT_EXIT;
		opt->opt_action = set_switch_scheme;
		opt++;
		num_opts++;
	}

	menu = new_menu(MSG_Select_your_choice, options, num_opts,
	    -1, -10, 0, 0, 0, NULL, NULL, NULL, NULL, MSG_cancel);
	if (menu != -1) {
		get_menudesc(menu)->expand_act = expand_all_option_texts;
		process_menu(menu, &ai);
		free_menu(menu);
	}

	return ai.rv;
}

static void
merge_part_with_wanted(struct disk_partitions *parts, part_id pno,
    const struct disk_part_info *info, struct partition_usage_set *wanted,
    size_t wanted_num, bool is_outer)
{
	struct part_usage_info *infos;

	/*
	 * does this partition match something in the wanted set?
	 */
	for (size_t i = 0; i < wanted_num; i++) {
		if (wanted->infos[i].type != info->nat_type->generic_ptype)
			continue;
		if (wanted->infos[i].type == PT_root &&
		    info->last_mounted != NULL && info->last_mounted[0] != 0 &&
		    strcmp(info->last_mounted, wanted->infos[i].mount) != 0)
			continue;
		if (wanted->infos[i].cur_part_id != NO_PART)
			continue;
		wanted->infos[i].cur_part_id = pno;
		wanted->infos[i].parts = parts;
		wanted->infos[i].size = info->size;
		wanted->infos[i].cur_start = info->start;
		wanted->infos[i].flags &= ~PUIFLAG_EXTEND;
		if (wanted->infos[i].fs_type != FS_UNUSED &&
		    wanted->infos[i].type != PT_swap &&
		    info->last_mounted != NULL &&
		    info->last_mounted[0] != 0)
			wanted->infos[i].instflags |= PUIINST_MOUNT;
		if (is_outer)
			wanted->infos[i].flags |= PUIFLG_IS_OUTER;
		else
			wanted->infos[i].flags &= ~PUIFLG_IS_OUTER;
		return;
	}

	/*
	 * no match - if this is from the outer scheme, we are done.
	 * otherwise it must be inserted into the wanted set.
	 */
	if (is_outer)
		return;

	/*
	 * create a new entry for this
	 */
	infos = realloc(wanted->infos, sizeof(*infos)*(wanted->num+1));
	if (infos == NULL)
		return;
	wanted->infos = infos;
	infos += wanted->num;
	wanted->num++;
	memset(infos, 0, sizeof(*infos));
	if (info->last_mounted != NULL && info->last_mounted[0] != 0)
		strlcpy(infos->mount, info->last_mounted,
		    sizeof(infos->mount));
	infos->type = info->nat_type->generic_ptype;
	infos->cur_part_id = pno;
	infos->parts = parts;
	infos->size = info->size;
	infos->cur_start = info->start;
	infos->fs_type = info->fs_type;
	infos->fs_version = info->fs_sub_type;
	if (is_outer)
		infos->flags |= PUIFLG_IS_OUTER;
}

static bool
have_x11_by_default(void)
{
	static const uint8_t def_sets[] = { MD_SETS_SELECTED };

	for (size_t i = 0; i < __arraycount(def_sets); i++)
		if (def_sets[i] >= SET_X11_FIRST &&
		    def_sets[i] <= SET_X11_LAST)
			return true;

	return false;
}

static void
fill_defaults(struct partition_usage_set *wanted, struct disk_partitions *parts,
    daddr_t ptstart, daddr_t ptsize)
{
	size_t i, root = ~0U, usr = ~0U, swap = ~0U, def_usr = ~0U;
	daddr_t free_space, dump_space, required;
#if defined(DEFAULT_UFS2) && !defined(HAVE_UFS2_BOOT)
	size_t boot = ~0U;
#endif

	memset(wanted, 0, sizeof(*wanted));
	wanted->parts = parts;
	if (ptstart > parts->disk_start)
		wanted->reserved_space = ptstart - parts->disk_start;
	if ((ptstart + ptsize) < (parts->disk_start+parts->disk_size))
		wanted->reserved_space +=
		    (parts->disk_start+parts->disk_size) -
		    (ptstart + ptsize);
	wanted->num = __arraycount(default_parts_init);
	wanted->infos = calloc(wanted->num, sizeof(*wanted->infos));
	if (wanted->infos == NULL) {
		err_msg_win(err_outofmem);
		return;
	}

	memcpy(wanted->infos, default_parts_init, sizeof(default_parts_init));

#ifdef HAVE_TMPFS
	if (get_ramsize() >= SMALL_RAM_SIZE) {
		for (i = 0; i < wanted->num; i++) {
			if (wanted->infos[i].type != PT_root ||
			    wanted->infos[i].fs_type != FS_TMPFS)
				continue;
			/* default tmpfs to 1/4 RAM */
			wanted->infos[i].size = -25;
			wanted->infos[i].def_size = -25;
			break;
		}
	}
#endif

#ifdef MD_PART_DEFAULTS
	MD_PART_DEFAULTS(pm, wanted->infos, wanted->num);
#endif

	for (i = 0; i < wanted->num; i++) {
		wanted->infos[i].parts = parts;
		wanted->infos[i].cur_part_id = NO_PART;
		if (wanted->infos[i].type == PT_undef &&
		    wanted->infos[i].fs_type != FS_UNUSED) {
			const struct part_type_desc *pt =
			    parts->pscheme->get_fs_part_type(PT_undef,
			    wanted->infos[i].fs_type,
			    wanted->infos[i].fs_version);
			if (pt != NULL)
				wanted->infos[i].type = pt->generic_ptype;
		}
		if (wanted->parts->parent != NULL &&
		    (wanted->infos[i].fs_type == FS_MSDOS ||
		     wanted->infos[i].fs_type == FS_EX2FS))
			wanted->infos[i].flags |=
			    PUIFLG_ADD_INNER|PUIFLAG_ADD_OUTER;

#if DEFSWAPSIZE == -1
		if (wanted->infos[i].type == PT_swap) {
#ifdef	MD_MAY_SWAP_TO
			if (MD_MAY_SWAP_TO(wanted->parts->disk))
#endif
				wanted->infos[i].size =
				    get_ramsize() * (MEG / 512);
		}
#endif
		if (wanted->infos[i].type == PT_swap && swap > wanted->num)
			swap = i;
#if defined(DEFAULT_UFS2) && !defined(HAVE_UFS2_BOOT)
		if (wanted->infos[i].instflags & PUIINST_BOOT)
			boot = i;
#endif
		if (wanted->infos[i].type == PT_root) {
			if (strcmp(wanted->infos[i].mount, "/") == 0) {
				root = i;
			} else if (
			    strcmp(wanted->infos[i].mount, "/usr") == 0) {
				if (wanted->infos[i].size > 0)
					usr = i;
				else
					def_usr = i;
			}
			if (wanted->infos[i].fs_type == FS_UNUSED)
				wanted->infos[i].fs_type = FS_BSDFFS;
			if (wanted->infos[i].fs_type == FS_BSDFFS) {
#ifdef DEFAULT_UFS2
#ifndef HAVE_UFS2_BOOT
				if (boot < wanted->num || i != root)
#endif
					wanted->infos[i].fs_version = 3;
#endif
			}
		}
	}

	/*
	 * Now we have the defaults as if we were installing to an
	 * empty disk. Merge the partitions in target range that are already
	 * there (match with wanted) or are there additionally.
	 * The only thing outside of target range that we care for
	 * are FAT partitions, EXT2FS partitions, and a potential
	 * swap partition - we assume one is enough.
	 */
	size_t num = wanted->num;
	if (parts->parent) {
		for (part_id pno = 0; pno < parts->parent->num_part; pno++) {
			struct disk_part_info info;

			if (!parts->parent->pscheme->get_part_info(
			    parts->parent, pno, &info))
				continue;
			if (info.nat_type->generic_ptype != PT_swap &&
			    info.fs_type != FS_MSDOS &&
			    info.fs_type != FS_EX2FS)
				continue;
			merge_part_with_wanted(parts->parent, pno, &info,
			    wanted, num, true);
			break;
		}
	}
	for (part_id pno = 0; pno < parts->num_part; pno++) {
		struct disk_part_info info;

		if (!parts->pscheme->get_part_info(parts, pno, &info))
			continue;

		if (info.flags & PTI_PSCHEME_INTERNAL)
			continue;

		if (info.nat_type->generic_ptype != PT_swap &&
		    (info.start < ptstart ||
		    (info.start + info.size) > (ptstart+ptsize)))
			continue;

		merge_part_with_wanted(parts, pno, &info,
		    wanted, num, false);
	}

	daddr_t align = parts->pscheme->get_part_alignment(parts);

	if (root < wanted->num && wanted->infos[root].cur_part_id == NO_PART) {
		daddr_t max_root_size = parts->disk_start + parts->disk_size;
		if (root_limit > 0) {
			/* Bah - bios can not read all the disk, limit root */
			max_root_size = root_limit - parts->disk_start;
		}
		wanted->infos[root].limit = max_root_size;
	}

	if (have_x11_by_default()) {
		daddr_t xsize = XNEEDMB * (MEG / 512);
		if (usr < wanted->num) {
			if (wanted->infos[usr].cur_part_id == NO_PART) {
				wanted->infos[usr].size += xsize;
				wanted->infos[usr].def_size += xsize;
			}
		} else if (root < wanted->num &&
		    wanted->infos[root].cur_part_id == NO_PART &&
		    (wanted->infos[root].limit == 0 ||
		    (wanted->infos[root].size + xsize) <=
		    wanted->infos[root].limit)) {
			wanted->infos[root].size += xsize;
		}
	}
	if (wanted->infos[root].limit > 0 &&
	    wanted->infos[root].size > wanted->infos[root].limit) {
		if (usr < wanted->num) {
			/* move space from root to usr */
			daddr_t spill = wanted->infos[root].size -
			    wanted->infos[root].limit;
			spill = roundup(spill, align);
			wanted->infos[root].size =
			    wanted->infos[root].limit;
			wanted->infos[usr].size = spill;
		} else {
			wanted->infos[root].size =
			    wanted->infos[root].limit;
		}
	}

	/*
	 * Preliminary calc additional space to allocate and how much
	 * we likely will have left over. Use that to do further
	 * adjustments, so we don't present the user inherently
	 * impossible defaults.
	 */
	free_space = parts->free_space - wanted->reserved_space;
	required = 0;
	if (root < wanted->num)
		required += wanted->infos[root].size;
	if (usr < wanted->num)
		required += wanted->infos[usr].size;
	else if (def_usr < wanted->num)
		required += wanted->infos[def_usr].def_size;
	free_space -= required;
	for (i = 0; i < wanted->num; i++) {
		if (i == root || i == usr)
			continue;	/* already accounted above */
		if (wanted->infos[i].cur_part_id != NO_PART)
			continue;
		if (wanted->infos[i].size == 0)
			continue;
		if (wanted->infos[i].flags
		    & (PUIFLG_IS_OUTER|PUIFLG_JUST_MOUNTPOINT))
			continue;
		free_space -= wanted->infos[i].size;
	}
	if (free_space < 0 && swap < wanted->num &&
	    get_ramsize() > TINY_RAM_SIZE) {
		/* steel from swap partition */
		daddr_t d = wanted->infos[swap].size;
		daddr_t inc = roundup(-free_space, align);
		if (inc > d)
			inc = d;
		free_space += inc;
		wanted->infos[swap].size -= inc;
	}
	if (root < wanted->num) {
		/* Add space for 2 system dumps to / (traditional) */
		dump_space = get_ramsize() * (MEG/512);
		dump_space = roundup(dump_space, align);
		if (free_space > dump_space*2)
			dump_space *= 2;
		if (free_space > dump_space) {
			wanted->infos[root].size += dump_space;
			free_space -= dump_space;
		}
	}
	if (wanted->infos[root].limit > 0 &&
	    (wanted->infos[root].cur_start + wanted->infos[root].size >
		wanted->infos[root].limit ||
	    (wanted->infos[root].flags & PUIFLAG_EXTEND &&
	    (wanted->infos[root].cur_start + wanted->infos[root].size
	     + free_space > wanted->infos[root].limit)))) {
		if (usr >= wanted->num && def_usr < wanted->num) {
			usr = def_usr;
			wanted->infos[usr].size = wanted->infos[root].size
			    - wanted->infos[root].limit;
			if (wanted->infos[usr].size <= 0)
				wanted->infos[usr].size = max(1,
				    wanted->infos[usr].def_size);
			wanted->infos[root].size =
			    wanted->infos[root].limit;
			if (wanted->infos[root].flags & PUIFLAG_EXTEND) {
				wanted->infos[root].flags &= ~PUIFLAG_EXTEND;
				wanted->infos[usr].flags |= PUIFLAG_EXTEND;
			}
		} else if (usr < wanted->num) {
			/* move space from root to usr */
			daddr_t spill = wanted->infos[root].size -
			    wanted->infos[root].limit;
			spill = roundup(spill, align);
			wanted->infos[root].size =
			    wanted->infos[root].limit;
			wanted->infos[usr].size = spill;
		} else {
			wanted->infos[root].size =
			    wanted->infos[root].limit;
		}
	}
	wanted->infos[root].def_size = wanted->infos[root].size;
}

/*
 * We sort pset->infos to sync with pset->parts and
 * the cur_part_id, to allow using the same index into both
 * "array" in later phases. This may include inserting
 * dummy  entries (when we do not actually want the
 * partition, but it is forced upon us, like RAW_PART in
 * disklabel).
 */
static void
sort_and_sync_parts(struct partition_usage_set *pset)
{
	struct part_usage_info *infos;
	size_t i, j, no;
	part_id pno;

	pset->cur_free_space = pset->parts->free_space - pset->reserved_space;

	/* count non-empty entries that are not in pset->parts */
	no = pset->parts->num_part;
	for (i = 0; i < pset->num; i++) {
		if (pset->infos[i].size == 0)
			continue;
		if (pset->infos[i].cur_part_id != NO_PART)
			continue;
		no++;
	}

	/* allocate new infos */
	infos = calloc(no, sizeof *infos);
	if (infos == NULL)
		return;

	/* pre-initialize the first entries as dummy entries */
	for (i = 0; i < pset->parts->num_part; i++) {
		infos[i].cur_part_id = NO_PART;
		infos[i].cur_flags = PTI_PSCHEME_INTERNAL;
	}
	/*
	 * Now copy over everything from our old entries that points to
	 * a real partition.
	 */
	for (i = 0; i < pset->num; i++) {
		pno = pset->infos[i].cur_part_id;
		if (pno == NO_PART)
			continue;
		if (pset->parts != pset->infos[i].parts)
			continue;
		if (pset->infos[i].flags & PUIFLG_JUST_MOUNTPOINT)
			continue;
		if ((pset->infos[i].flags & (PUIFLG_IS_OUTER|PUIFLG_ADD_INNER))
		    == PUIFLG_IS_OUTER)
			continue;
		if (pno >= pset->parts->num_part)
			continue;
		memcpy(infos+pno, pset->infos+i, sizeof(*infos));
	}
	/* Fill in the infos for real partitions where we had no data */
	for (pno = 0; pno < pset->parts->num_part; pno++) {
		struct disk_part_info info;

		if (infos[pno].cur_part_id != NO_PART)
			continue;

		if (!pset->parts->pscheme->get_part_info(pset->parts, pno,
		    &info))
			continue;

		infos[pno].parts = pset->parts;
		infos[pno].cur_part_id = pno;
		infos[pno].cur_flags = info.flags;
		infos[pno].size = info.size;
		infos[pno].type = info.nat_type->generic_ptype;
		infos[pno].cur_start = info.start;
		infos[pno].fs_type = info.fs_type;
		infos[pno].fs_version = info.fs_sub_type;
	}
	/* Add the non-partition entries after that */
	j = pset->parts->num_part;
	for (i = 0; i < pset->num; i++) {
		if (j >= no)
			break;
		if (pset->infos[i].size == 0)
			continue;
		if (pset->infos[i].cur_part_id != NO_PART)
			continue;
		memcpy(infos+j, pset->infos+i, sizeof(*infos));
		j++;
	}

	/* done, replace infos */
	free(pset->infos);
	pset->num = no;
	pset->infos = infos;
}

#ifndef NO_CLONES
/*
 * Convert clone entries with more than one source into
 * several entries with a single source each.
 */
static void
normalize_clones(struct part_usage_info **infos, size_t *num)
{
	size_t i, j, add_clones;
	struct part_usage_info *ui, *src, *target;
	struct disk_part_info info;
	struct selected_partition *clone;

	for (add_clones = 0, i = 0; i < *num; i++) {
		if ((*infos)[i].clone_src != NULL &&
		    (*infos)[i].flags & PUIFLG_CLONE_PARTS &&
		    (*infos)[i].cur_part_id == NO_PART)
			add_clones += (*infos)[i].clone_src->num_sel-1;
	}
	if (add_clones == 0)
		return;

	ui = calloc(*num+add_clones, sizeof(**infos));
	if (ui == NULL)
		return;	/* can not handle this well here, drop some clones */

	/* walk the list and dedup clones */
	for (src = *infos, target = ui, i = 0; i < *num; i++) {
		if (src != target)
			*target = *src;
		if (target->clone_src != NULL &&
		    (target->flags & PUIFLG_CLONE_PARTS) &&
		    target->cur_part_id == NO_PART) {
			for (j = 0; j < src->clone_src->num_sel; j++) {
				if (j > 0) {
					target++;
					*target = *src;
				}
				target->clone_ndx = j;
				clone = &target->clone_src->selection[j];
				clone->parts->pscheme->get_part_info(
				    clone->parts, clone->id, &info);
				target->size = info.size;
			}
		}
		target++;
		src++;
	}
	*num += add_clones;
	assert((target-ui) >= 0 && (size_t)(target-ui) == *num);
	free(*infos);
	*infos = ui;
}
#endif

static void
apply_settings_to_partitions(struct disk_partitions *parts,
    struct partition_usage_set *wanted, daddr_t start, daddr_t xsize)
{
	size_t i, exp_ndx = ~0U;
	daddr_t planned_space = 0, nsp, from, align;
	struct disk_part_info *infos;
#ifndef NO_CLONES
	struct disk_part_info cinfo, srcinfo;
	struct selected_partition *sp;
#endif
	struct disk_part_free_space space;
	struct disk_partitions *ps = NULL;
	part_id pno, new_part_id;

#ifndef NO_CLONES
	normalize_clones(&wanted->infos, &wanted->num);
#endif

	infos = calloc(wanted->num, sizeof(*infos));
	if (infos == NULL) {
		err_msg_win(err_outofmem);
		return;
	}

	align = wanted->parts->pscheme->get_part_alignment(wanted->parts);
	/*
	 * Pass one: calculate space available for expanding
	 * the marked partition.
	 */
	if (parts->free_space != parts->disk_size)
		planned_space = align;	/* align first part */
	for (i = 0; i < wanted->num; i++) {
		if ((wanted->infos[i].flags & PUIFLAG_EXTEND) &&
		    exp_ndx == ~0U)
			exp_ndx = i;
		if (wanted->infos[i].flags & PUIFLG_JUST_MOUNTPOINT)
			continue;
		nsp = wanted->infos[i].size;
		if (wanted->infos[i].cur_part_id != NO_PART) {
			ps = wanted->infos[i].flags & PUIFLG_IS_OUTER ?
			    parts->parent : parts;

			ps->pscheme->get_part_info(ps,
			     wanted->infos[i].cur_part_id, &infos[i]);
			if (!(wanted->infos[i].flags & PUIFLG_IS_OUTER))
				nsp -= infos[i].size;
		}
		if (nsp <= 0)
			continue;
		planned_space += roundup(nsp, align);
	}

	/*
	 * Expand the pool partition (or shrink, if we overran),
	 * but check size limits.
	 */
	if (exp_ndx < wanted->num) {
		daddr_t free_space = parts->free_space - planned_space;
		daddr_t new_size = wanted->infos[exp_ndx].size;
		if (free_space > 0)
			new_size += roundup(free_space,align);

		if (wanted->infos[exp_ndx].limit > 0 &&
		    (new_size + wanted->infos[exp_ndx].cur_start)
		     > wanted->infos[exp_ndx].limit) {
			wanted->infos[exp_ndx].size =
			    wanted->infos[exp_ndx].limit
			    - wanted->infos[exp_ndx].cur_start;
		} else {
			wanted->infos[exp_ndx].size = new_size;
		}
	}

	/*
	 * Now it gets tricky: we want the wanted partitions in order
	 * as defined, but any already existing partitions should not
	 * be moved. We allow them to change size though.
	 * To keep it simple, we just assign in order and skip blocked
	 * spaces. This may shuffle the order of the resulting partitions
	 * compared to the wanted list.
	 */

	/* Adjust sizes of existing partitions */
	for (i = 0; i < wanted->num; i++) {
		ps = wanted->infos[i].flags & PUIFLG_IS_OUTER ?
		    parts->parent : parts;
		const struct part_usage_info *want = &wanted->infos[i];

		if (want->cur_part_id == NO_PART)
			continue;
		if (i == exp_ndx)	/* the exp. part. can not exist yet */
			continue;
		daddr_t free_size = ps->pscheme->max_free_space_at(ps,
		    infos[i].start);
		if (free_size < wanted->infos[i].size)
			continue;
		if (infos[i].size != wanted->infos[i].size) {
			infos[i].size = wanted->infos[i].size;
			ps->pscheme->set_part_info(ps, want->cur_part_id,
			    &infos[i], NULL);
		}
	}

	from = start > 0 ? start : -1;
	/*
	 * First add all outer partitions - we need to align those exactly
	 * with the inner counterpart later.
	 */
	if (parts->parent) {
		ps = parts->parent;
		daddr_t outer_align = ps->pscheme->get_part_alignment(ps);

		for (i = 0; i < wanted->num; i++) {
			struct part_usage_info *want = &wanted->infos[i];

			if (want->cur_part_id != NO_PART)
				continue;
			if (!(want->flags & PUIFLAG_ADD_OUTER))
				continue;
			if (want->size <= 0)
				continue;

			size_t cnt = ps->pscheme->get_free_spaces(ps,
			    &space, 1, want->size-2*outer_align,
			    outer_align, from, -1);

			if (cnt == 0)	/* no free space for this partition */
				continue;

			infos[i].start = space.start;
			infos[i].size = min(want->size, space.size);
			infos[i].nat_type =
			    ps->pscheme->get_fs_part_type(
			        want->type, want->fs_type, want->fs_version);
			infos[i].last_mounted = want->mount;
			infos[i].fs_type = want->fs_type;
			infos[i].fs_sub_type = want->fs_version;
			infos[i].fs_opt1 = want->fs_opt1;
			infos[i].fs_opt2 = want->fs_opt2;
			infos[i].fs_opt3 = want->fs_opt3;
			new_part_id = ps->pscheme->add_partition(ps,
			    &infos[i], NULL);
			if (new_part_id == NO_PART)
				continue;	/* failed to add, skip */

			ps->pscheme->get_part_info(ps,
			    new_part_id, &infos[i]);
			want->cur_part_id = new_part_id;

			want->flags |= PUIFLG_ADD_INNER|PUIFLG_IS_OUTER;
			from = roundup(infos[i].start +
			    infos[i].size, outer_align);
		}
	}

	/*
	 * Now add new inner partitions (and cloned partitions)
	 */
	for (i = 0; i < wanted->num; i++) {

		daddr_t limit = wanted->parts->disk_size + wanted->parts->disk_start;
		if (from >= limit)
			break;

		struct part_usage_info *want = &wanted->infos[i];

		if (want->cur_part_id != NO_PART)
			continue;
		if (want->flags & (PUIFLG_JUST_MOUNTPOINT|PUIFLG_IS_OUTER))
			continue;
#ifndef NO_CLONES
		if ((want->flags & PUIFLG_CLONE_PARTS) &&
		    want->clone_src != NULL &&
		    want->clone_ndx < want->clone_src->num_sel) {
			sp = &want->clone_src->selection[want->clone_ndx];
			if (!sp->parts->pscheme->get_part_info(
			    sp->parts, sp->id, &srcinfo))
				continue;
			if (!wanted->parts->pscheme->
			    adapt_foreign_part_info(wanted->parts,
			    &cinfo, sp->parts->pscheme, &srcinfo))
				continue;

			/* find space for cinfo and add a partition */
			size_t cnt = wanted->parts->pscheme->get_free_spaces(
			    wanted->parts, &space, 1, want->size-align, align,
			    from, -1);
			if (cnt == 0)
				cnt = wanted->parts->pscheme->get_free_spaces(
				    wanted->parts, &space, 1,
				    want->size-5*align, align, from, -1);

			if (cnt == 0)
				continue; /* no free space for this clone */

			infos[i] = cinfo;
			infos[i].start = space.start;
			new_part_id = wanted->parts->pscheme->add_partition(
			    wanted->parts, &infos[i], NULL);
		} else {
#else
		{
#endif
			if (want->size <= 0)
				continue;
			size_t cnt = wanted->parts->pscheme->get_free_spaces(
			    wanted->parts, &space, 1, want->size-align, align,
			    from, -1);
			if (cnt == 0)
				cnt = wanted->parts->pscheme->get_free_spaces(
				    wanted->parts, &space, 1,
				    want->size-5*align, align, from, -1);

			if (cnt == 0)
				continue; /* no free space for this partition */

			infos[i].start = space.start;
			infos[i].size = min(want->size, space.size);
			infos[i].nat_type =
			    wanted->parts->pscheme->get_fs_part_type(
			    want->type, want->fs_type, want->fs_version);
			infos[i].last_mounted = want->mount;
			infos[i].fs_type = want->fs_type;
			infos[i].fs_sub_type = want->fs_version;
			infos[i].fs_opt1 = want->fs_opt1;
			infos[i].fs_opt2 = want->fs_opt2;
			infos[i].fs_opt3 = want->fs_opt3;
			if (want->fs_type != FS_UNUSED &&
			    want->type != PT_swap) {
				want->instflags |= PUIINST_NEWFS;
				if (want->mount[0] != 0)
					want->instflags |= PUIINST_MOUNT;
			}
			new_part_id = wanted->parts->pscheme->add_partition(
			    wanted->parts, &infos[i], NULL);
		}

		if (new_part_id == NO_PART)
			continue;	/* failed to add, skip */

		wanted->parts->pscheme->get_part_info(
		    wanted->parts, new_part_id, &infos[i]);
		from = roundup(infos[i].start+infos[i].size, align);
	}


	/*
	* If there are any outer partitions that we need as inner ones
	 * too, add them to the inner partitioning scheme.
	 */
	for (i = 0; i < wanted->num; i++) {
		struct part_usage_info *want = &wanted->infos[i];

		if (want->cur_part_id == NO_PART)
			continue;
		if (want->flags & PUIFLG_JUST_MOUNTPOINT)
			continue;
		if (want->size <= 0)
			continue;

		if ((want->flags & (PUIFLG_ADD_INNER|PUIFLG_IS_OUTER)) !=
		    (PUIFLG_ADD_INNER|PUIFLG_IS_OUTER))
			continue;

		new_part_id = NO_PART;
		for (part_id j = 0; new_part_id == NO_PART &&
		    j < wanted->parts->num_part; j++) {
			struct disk_part_info test;

			if (!wanted->parts->pscheme->get_part_info(
			    wanted->parts, j, &test))
				continue;
			if (test.start == want->cur_start &&
			    test.size == want->size)
				new_part_id = j;
		}

		if (new_part_id == NO_PART) {
			infos[i].start = want->cur_start;
			infos[i].size = want->size;
			infos[i].nat_type = wanted->parts->pscheme->
			    get_fs_part_type(want->type, want->fs_type,
			    want->fs_version);
			infos[i].last_mounted = want->mount;
			infos[i].fs_type = want->fs_type;
			infos[i].fs_sub_type = want->fs_version;
			infos[i].fs_opt1 = want->fs_opt1;
			infos[i].fs_opt2 = want->fs_opt2;
			infos[i].fs_opt3 = want->fs_opt3;

			if (wanted->parts->pscheme->add_outer_partition
			    != NULL)
				new_part_id = wanted->parts->pscheme->
				    add_outer_partition(
				    wanted->parts, &infos[i], NULL);
			else
				new_part_id = wanted->parts->pscheme->
				    add_partition(
				    wanted->parts, &infos[i], NULL);

			if (new_part_id == NO_PART)
				continue;	/* failed to add, skip */
		}

		wanted->parts->pscheme->get_part_info(
		    wanted->parts, new_part_id, &infos[i]);
		want->parts = wanted->parts;
		if (want->fs_type != FS_UNUSED &&
		    want->type != PT_swap) {
			want->instflags |= PUIINST_NEWFS;
			if (want->mount[0] != 0)
				want->instflags |= PUIINST_MOUNT;
		}
	}

	/*
	 * Note: all part_ids are invalid now, as we have added things!
	 */
	for (i = 0; i < wanted->num; i++)
		wanted->infos[i].cur_part_id = NO_PART;
	for (pno = 0; pno < parts->num_part; pno++) {
		struct disk_part_info t;

		if (!parts->pscheme->get_part_info(parts, pno, &t))
			continue;

		for (i = 0; i < wanted->num; i++) {
			if (wanted->infos[i].cur_part_id != NO_PART)
				continue;
			if (wanted->infos[i].size <= 0)
				continue;
			if (t.start == infos[i].start) {
				wanted->infos[i].cur_part_id = pno;
				wanted->infos[i].cur_start = infos[i].start;
				wanted->infos[i].cur_flags = infos[i].flags;
				break;
			}
		}
	}
	free(infos);

	/* sort, and sync part ids and wanted->infos[] indices */
	sort_and_sync_parts(wanted);
}

static void
replace_by_default(struct disk_partitions *parts,
    daddr_t start, daddr_t size, struct partition_usage_set *wanted)
{

	if (start == 0 && size == parts->disk_size)
		parts->pscheme->delete_all_partitions(parts);
	else if (parts->pscheme->delete_partitions_in_range != NULL)
		parts->pscheme->delete_partitions_in_range(parts, start, size);
	else
		assert(parts->num_part == 0);

	fill_defaults(wanted, parts, start, size);
	apply_settings_to_partitions(parts, wanted, start, size);
}

static bool
edit_with_defaults(struct disk_partitions *parts,
    daddr_t start, daddr_t size, struct partition_usage_set *wanted)
{
	bool ok;

	fill_defaults(wanted, parts, start, size);
	ok = get_ptn_sizes(wanted);
	if (ok)
		apply_settings_to_partitions(parts, wanted, start, size);
	return ok;
}

/*
 * md back-end code for menu-driven BSD disklabel editor.
 * returns 0 on failure, 1 on success, -1 for restart.
 * fills the install target with a list for newfs/fstab.
 */
int
make_bsd_partitions(struct install_partition_desc *install)
{
	struct disk_partitions *parts = pm->parts;
	const struct disk_partitioning_scheme *pscheme;
	struct partition_usage_set wanted;
	daddr_t p_start, p_size;
	enum layout_type layoutkind = LY_SETSIZES;
	bool have_existing;

	if (pm && pm->no_part && parts == NULL)
		return 1;
	if (parts == NULL) {
		pscheme = select_part_scheme(pm, NULL, !pm->no_mbr, NULL);
		if (pscheme == NULL)
			return 0;
		parts = pscheme->create_new_for_disk(pm->diskdev,
		    0, pm->dlsize, true, NULL);
		if (parts == NULL)
			return 0;
		pm->parts = parts;
	} else {
		pscheme = parts->pscheme;
	}

	if (pscheme->secondary_partitions) {
		struct disk_partitions *p;

		p = pscheme->secondary_partitions(parts, pm->ptstart, false);
		if (p) {
			parts = p;
			pscheme = parts->pscheme;
		}
	}

	have_existing = check_existing_netbsd(parts);

	/*
	 * Make sure the cylinder size multiplier/divisor and disk size are
	 * valid
	 */
	if (pm->current_cylsize == 0)
		pm->current_cylsize = pm->dlcylsize;
	if (pm->ptsize == 0)
		pm->ptsize = pm->dlsize;

	/* Ask for layout type -- standard or special */
	if (partman_go == 0) {
		char bsd_size[6], min_size[6], x_size[6];

		humanize_number(bsd_size, sizeof(bsd_size),
		    (uint64_t)pm->ptsize*pm->sectorsize,
		    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
		humanize_number(min_size, sizeof(min_size),
		    (uint64_t)(DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE)*MEG,
		    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
		humanize_number(x_size, sizeof(x_size),
		    (uint64_t)(DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE
		    + XNEEDMB)*MEG,
		    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

		msg_display_subst(
		    have_existing ? MSG_layout_prologue_existing
		    : MSG_layout_prologue_none, 6, pm->diskdev,
		    msg_string(parts->pscheme->name),
		    msg_string(parts->pscheme->short_name),
		    bsd_size, min_size, x_size);
		msg_display_add_subst(MSG_layout_main, 6,
		    pm->diskdev,
		    msg_string(parts->pscheme->name),
		    msg_string(parts->pscheme->short_name),
		    bsd_size, min_size, x_size);
		msg_display_add("\n\n");
		layoutkind = ask_layout(parts, have_existing);
		if (layoutkind == LY_ERROR)
			return 0;
	}

	if (layoutkind == LY_USEDEFAULT || layoutkind == LY_SETSIZES) {
		/* calc available disk area for the NetBSD partitions */
		p_start = pm->ptstart;
		p_size = pm->ptsize;
		if (parts->parent != NULL &&
		    parts->parent->pscheme->guess_install_target != NULL)
			parts->parent->pscheme->guess_install_target(
			    parts->parent, &p_start, &p_size);
	}
	if (layoutkind == LY_OTHERSCHEME) {
		parts->pscheme->destroy_part_scheme(parts);
		return -1;
	} else if (layoutkind == LY_USENONE) {
		struct disk_part_free_space space;
		size_t cnt;

		empty_usage_set_from_parts(&wanted, parts);
		cnt = parts->pscheme->get_free_spaces(parts, &space, 1,
		0, parts->pscheme->get_part_alignment(parts), 0, -1);
		p_start = p_size = 0;
		if (cnt == 1) {
			p_start = space.start;
			p_size = space.size;
			wanted.cur_free_space = space.size;
		}
	} else if (layoutkind == LY_USEDEFAULT) {
		replace_by_default(parts, p_start, p_size,
		    &wanted);
	} else if (layoutkind == LY_SETSIZES) {
		if (!edit_with_defaults(parts, p_start, p_size,
		    &wanted)) {
			free_usage_set(&wanted);
			return 0;
		}
	} else {
		usage_set_from_parts(&wanted, parts);
	}

	/*
	 * Make sure the target root partition is properly marked,
	 * check for existing EFI boot partition.
	 */
	bool have_inst_target = false;
#ifdef HAVE_EFI_BOOT
	daddr_t target_start = -1;
#endif
	for (size_t i = 0; i < wanted.num; i++) {
		if (wanted.infos[i].cur_flags & PTI_INSTALL_TARGET) {
			have_inst_target = true;
#ifdef HAVE_EFI_BOOT
			target_start = wanted.infos[i].cur_start;
#endif
			break;
		 }
	}
	if (!have_inst_target) {
		for (size_t i = 0; i < wanted.num; i++) {
			struct disk_part_info info;

			if (wanted.infos[i].type != PT_root ||
			    strcmp(wanted.infos[i].mount, "/") != 0)
				continue;
			wanted.infos[i].cur_flags |= PTI_INSTALL_TARGET;

			if (!wanted.parts->pscheme->get_part_info(wanted.parts,
			    wanted.infos[i].cur_part_id, &info))
				break;
			info.flags |= PTI_INSTALL_TARGET;
			wanted.parts->pscheme->set_part_info(wanted.parts,
			    wanted.infos[i].cur_part_id, &info, NULL);
#ifdef HAVE_EFI_BOOT
			target_start = wanted.infos[i].cur_start;
#endif
			break;
		}
	}
#ifdef HAVE_EFI_BOOT
	size_t boot_part = ~0U;
	for (part_id i = 0; i < wanted.num; i++) {
		if ((wanted.infos[i].cur_flags & PTI_BOOT) != 0 ||
		    wanted.infos[i].type ==  PT_EFI_SYSTEM) {
			boot_part = i;
			break;
		}
	}
	if (boot_part == ~0U) {
		for (part_id i = 0; i < wanted.num; i++) {
			/*
			 * heuristic to recognize existing MBR FAT
			 * partitions as EFI without looking for
			 * details
			 */
			if ((wanted.infos[i].type != PT_FAT &&
			    wanted.infos[i].type != PT_EFI_SYSTEM) ||
			    wanted.infos[i].fs_type != FS_MSDOS)
				continue;
			daddr_t ps = wanted.infos[i].cur_start;
			daddr_t pe = ps + wanted.infos[i].size;
			if (target_start >= 0 &&
			   (ps >= target_start || pe >= target_start))
				continue;
			boot_part = i;
			break;
		}
	}
	if (boot_part != ~0U) {
		struct disk_part_info info;

		if (wanted.parts->pscheme->get_part_info(wanted.parts,
		    wanted.infos[boot_part].cur_part_id, &info)) {
			info.flags |= PTI_BOOT;
			wanted.parts->pscheme->set_part_info(wanted.parts,
			    wanted.infos[boot_part].cur_part_id, &info, NULL);
		}
		wanted.infos[boot_part].instflags |= PUIINST_BOOT;
	}
#endif

	/*
	 * OK, we have a partition table. Give the user the chance to
	 * edit it and verify it's OK, or abort altogether.
	 */
 	for (;;) {
		int rv = edit_and_check_label(pm, &wanted, true);
		if (rv == 0) {
			msg_display(MSG_abort_part);
			free_usage_set(&wanted);
			return 0;
		}
		/* update install infos */
		install->num = wanted.num;
		install->infos = wanted.infos;
		install->write_back = wanted.write_back;
		install->num_write_back = wanted.num_write_back;
		/* and check them */
		if (check_partitions(install))
			break;
	}

	/* we moved infos from wanted to install target */
	wanted.infos = NULL;
	wanted.write_back = NULL;
	free_usage_set(&wanted);

	/* Everything looks OK. */
	return 1;
}

#ifndef MD_NEED_BOOTBLOCK
#define MD_NEED_BOOTBLOCK(A)	true
#endif

/*
 * check that there is at least a / somewhere.
 */
bool
check_partitions(struct install_partition_desc *install)
{
#ifdef HAVE_BOOTXX_xFS
	int rv = 1;
	char *bootxx;
#endif
#ifndef HAVE_UFS2_BOOT
	size_t i;
#endif

#ifdef HAVE_BOOTXX_xFS
	if (MD_NEED_BOOTBLOCK(install)) {
		/* check if we have boot code for the root partition type */
		bootxx = bootxx_name(install);
		if (bootxx != NULL) {
			rv = access(bootxx, R_OK);
			free(bootxx);
		} else
			rv = -1;
		if (rv != 0) {
			hit_enter_to_continue(NULL, MSG_No_Bootcode);
			return false;
		}
	}
#endif
#ifndef HAVE_UFS2_BOOT
	if (MD_NEED_BOOTBLOCK(install)) {
		for (i = 0; i < install->num; i++) {
			if (install->infos[i].type != PT_root)
				continue;
			if (strcmp(install->infos[i].mount, "/") != 0)
				continue;
			if (install->infos[i].fs_type != FS_BSDFFS)
				continue;
			if (install->infos[i].fs_version < 2)
				continue;
			hit_enter_to_continue(NULL, MSG_cannot_ufs2_root);
			return false;
		}
	}
#endif

	return md_check_partitions(install);
}
