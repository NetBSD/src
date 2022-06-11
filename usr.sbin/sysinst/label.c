/*	$NetBSD: label.c,v 1.36 2022/06/11 18:27:22 martin Exp $	*/

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
__RCSID("$NetBSD: label.c,v 1.36 2022/06/11 18:27:22 martin Exp $");
#endif

#include <sys/types.h>
#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <util.h>
#include <unistd.h>
#include <sys/dkio.h>
#include <sys/param.h>
#include <sys/bootblock.h>
#include <sys/bitops.h>
#include <ufs/ffs/fs.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/*
 * local prototypes
 */
static bool boringpart(const struct disk_part_info *info);
static bool checklabel(struct disk_partitions*, char *, char *);
static void show_partition_adder(menudesc *, struct partition_usage_set*);

/*
 * Return 1 if a partition should be ignored when checking
 * for overlapping partitions.
 */
static bool
boringpart(const struct disk_part_info *info)
{

	if (info->size == 0)
		return true;
	if (info->flags &
	     (PTI_PSCHEME_INTERNAL|PTI_WHOLE_DISK|PTI_SEC_CONTAINER|
	     PTI_RAW_PART))
		return true;

	return false;
}

/*
 * We have some partitions in our "wanted" list that we may not edit,
 * like the RAW_PART in disklabel, some that just represent external
 * mount entries for the final fstab or similar.
 * We have previously sorted pset->parts and pset->infos to be in sync,
 * but the former "array" may be shorter.
 * Here are a few quick predicates to check for them.
 */
static bool
real_partition(const struct partition_usage_set *pset, int index)
{
	if (index < 0 || (size_t)index >= pset->num)
		return false;

	return pset->infos[index].cur_part_id != NO_PART;
}

/*
 * Check partitioning for overlapping partitions.
 * Returns true if no overlapping partition found.
 * Sets reference arguments ovly1 and ovly2 to the indices of
 * overlapping partitions if any are found.
 */
static bool
checklabel(struct disk_partitions *parts,
    char *ovl1, char *ovl2)
{
	part_id i, j;
	struct disk_part_info info;
	daddr_t istart, iend, jstart, jend;
	unsigned int fs_type, fs_sub_type;

	if (parts->num_part == 0)
		return true;

	for (i = 0; i < parts->num_part - 1; i ++ ) {
		if (!parts->pscheme->get_part_info(parts, i, &info))
			continue;

		/* skip unused or reserved partitions */
		if (boringpart(&info))
			continue;

		/*
		 * check succeeding partitions for overlap.
		 * O(n^2), but n is small.
		 */
		istart = info.start;
		iend = istart + info.size;
		fs_type = info.fs_type;
		fs_sub_type = info.fs_sub_type;

		for (j = i+1; j < parts->num_part; j++) {

			if (!parts->pscheme->get_part_info(parts, j, &info))
				continue;

			/* skip unused or reserved partitions */
			if (boringpart(&info))
				continue;

			jstart = info.start;
			jend = jstart + info.size;

			/* overlap? */
			if ((istart <= jstart && jstart < iend) ||
			    (jstart <= istart && istart < jend)) {
				snprintf(ovl1, MENUSTRSIZE,
				    "%" PRIu64 " - %" PRIu64 " %s, %s",
				    istart / sizemult, iend / sizemult,
				    multname,
				    getfslabelname(fs_type, fs_sub_type));
				snprintf(ovl2, MENUSTRSIZE,
				    "%" PRIu64 " - %" PRIu64 " %s, %s",
				    jstart / sizemult, jend / sizemult,
				    multname,
				    getfslabelname(info.fs_type,
				        info.fs_sub_type));
				return false;
			}
		}
	}

	return true;
}

int
checkoverlap(struct disk_partitions *parts)
{
	char desc1[MENUSTRSIZE], desc2[MENUSTRSIZE];
	if (!checklabel(parts, desc1, desc2)) {
		msg_display_subst(MSG_partitions_overlap, 2, desc1, desc2);
		return 1;
	}
	return 0;
}

/*
 * return (see post_edit_verify):
 *  0 -> abort
 *  1 -> re-edit
 *  2 -> continue installation
 */
static int
verify_parts(struct partition_usage_set *pset, bool install)
{
	struct part_usage_info *wanted;
	struct disk_partitions *parts;
	size_t i, num_root;
	daddr_t first_bsdstart, inst_start;
	int rv;

	first_bsdstart = inst_start = -1;
	num_root = 0;
	parts = pset->parts;
	for (i = 0; i < pset->num; i++) {
		wanted = &pset->infos[i];

		if (wanted->flags & PUIFLG_JUST_MOUNTPOINT)
			continue;
		if (wanted->cur_part_id == NO_PART)
			continue;
		if (!(wanted->instflags & PUIINST_MOUNT))
			continue;
		if (strcmp(wanted->mount, "/") != 0)
			continue;
		num_root++;

		if (first_bsdstart <= 0) {
			first_bsdstart = wanted->cur_start;
		}
		if (inst_start < 0 &&
		    (wanted->cur_flags & PTI_INSTALL_TARGET)) {
			inst_start = wanted->cur_start;
		}
	}

	if ((num_root == 0 && install) ||
	    (num_root > 1 && inst_start < 0)) {
		if (num_root == 0 && install)
			msg_display_subst(MSG_must_be_one_root, 2,
			    msg_string(parts->pscheme->name),
			    msg_string(parts->pscheme->short_name));
		else
			msg_display_subst(MSG_multbsdpart, 2,
			    msg_string(parts->pscheme->name),
			    msg_string(parts->pscheme->short_name));
		rv = ask_reedit(parts);
		if (rv != 2)
			return rv;
	}

	/* Check for overlaps */
	if (checkoverlap(parts) != 0) {
		rv = ask_reedit(parts);
		if (rv != 2)
			return rv;
	}

	/*
	 * post_edit_verify returns:
	 *  0 -> abort
	 *  1 -> re-edit
	 *  2 -> continue installation
	 */
	if (parts->pscheme->post_edit_verify)
		return parts->pscheme->post_edit_verify(parts, false);

	return 2;
}

static int
edit_fs_start(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	daddr_t start, end;

	start = getpartoff(edit->pset->parts, edit->info.start);
	if (edit->info.size != 0) {
		/* Try to keep end in the same place */
		end = edit->info.start + edit->info.size;
		if (end < start)
			edit->info.size = edit->pset->parts->pscheme->
			    max_free_space_at(edit->pset->parts,
			    edit->info.start);
		else
			edit->info.size = end - start;
	}
	edit->info.start = start;
	return 0;
}

static int
edit_fs_size(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	struct disk_part_info pinfo;
	daddr_t size;

	/* get original partition data, in case start moved already */
	edit->pset->parts->pscheme->get_part_info(edit->pset->parts,
	    edit->id, &pinfo);
	/* ask for new size with old start and current values */
	size = getpartsize(edit->pset->parts, pinfo.start,
	    edit->info.start, edit->info.size);
	if (size < 0)
		return 0;
	if (size > edit->pset->parts->disk_size)
		size = edit->pset->parts->disk_size - edit->info.start;
	edit->info.size = size;
	return 0;
}

static int
set_ffs_opt_pow2(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	size_t val = 1 << (edit->offset+m->cursel);

	if (edit->mode == 1) {
		edit->info.fs_opt1 = val;
		edit->wanted->fs_opt1 = val;
	} else if (edit->mode == 2) {
		edit->info.fs_opt2 = val;
		edit->wanted->fs_opt2 = val;
	}
	return 0;
}

static int
edit_fs_ffs_opt(menudesc *m, void *arg, msg head,
    size_t min_val, size_t max_val)
{
	struct single_part_fs_edit *edit = arg;
	menu_ent opts[min(MAXPHYS/4096, 8)];
	char names[min(MAXPHYS/4096, 8)][20];
	size_t i, val;
	int menu;

	edit->offset = ilog2(min_val);
	memset(opts, 0, sizeof opts);
	for (i = 0, val = min_val; val <= max_val; i++, val <<= 1) {
		snprintf(names[i], sizeof names[i], "%zu", val);
		opts[i].opt_name = names[i];
		opts[i].opt_action = set_ffs_opt_pow2;
		opts[i].opt_flags = OPT_EXIT;
	}
	menu = new_menu(head, opts, i, 40, 6, 0, 0, MC_NOEXITOPT,
	    NULL, NULL, NULL, NULL, NULL);
	if (menu < 0)
		return 1;
	process_menu(menu, arg);
	free_menu(menu);
	return 0;
}

static int
edit_fs_ffs_block(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->mode = 1;		/* edit fs_opt1 */
	return edit_fs_ffs_opt(m, arg, MSG_Select_file_system_block_size,
	    4096, MAXPHYS);
}

static int
edit_fs_ffs_frag(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	size_t bsize, sec_size;

	edit->mode = 2;		/* edit fs_opt2 */
	bsize = edit->info.fs_opt1;
	if (bsize == 0) {
		sec_size = edit->wanted->parts->bytes_per_sector;
		if (edit->wanted->size >= (daddr_t)(128L*(GIG/sec_size)))
			bsize = 32*1024;
		else if (edit->wanted->size >= (daddr_t)(1000L*(MEG/sec_size)))
			bsize = 16*1024;
		else if (edit->wanted->size >= (daddr_t)(20L*(MEG/sec_size)))
			bsize = 8*1024;
		else
			bsize = 4+1024;
	}
	return edit_fs_ffs_opt(m, arg, MSG_Select_file_system_fragment_size,
		bsize / 8, bsize);
}

static int
edit_fs_ffs_avg_size(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	char answer[12];

	snprintf(answer, sizeof answer, "%u", edit->info.fs_opt3);
	msg_prompt_win(MSG_ptn_isize_prompt, -1, 18, 0, 0,
		answer, answer, sizeof answer);
	edit->info.fs_opt3 = atol(answer);
	edit->wanted->fs_opt3 = edit->info.fs_opt3;

	return 0;
}

static int
edit_fs_preserve(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->wanted->instflags ^= PUIINST_NEWFS;
	return 0;
}

static int
edit_install(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->info.flags ^= PTI_INSTALL_TARGET;
	return 0;
}

static int
edit_fs_mount(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->wanted->instflags ^= PUIINST_MOUNT;
	return 0;
}

static int
edit_fs_mountpt(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	char *p, *first, *last, buf[MOUNTLEN];

	strlcpy(buf, edit->wanted->mount, sizeof buf);
	msg_prompt_win(MSG_mountpoint, -1, 18, 0, 0,
		buf, buf, MOUNTLEN);

	/*
	 * Trim all leading and trailing whitespace
	 */
	for (first = NULL, last = NULL, p = buf; *p; p++) {
		if (isspace((unsigned char)*p))
			continue;
		if (first == NULL)
			first = p;
		last = p;
	}
	if (last != NULL)
		last[1] = 0;

	if (first == NULL || *first == 0 || strcmp(first, "none") == 0) {
		edit->wanted->mount[0] = 0;
		edit->wanted->instflags &= ~PUIINST_MOUNT;
		return 0;
	}

	if (*first != '/') {
		edit->wanted->mount[0] = '/';
		strlcpy(&edit->wanted->mount[1], first,
		    sizeof(edit->wanted->mount)-1);
	} else {
		strlcpy(edit->wanted->mount, first, sizeof edit->wanted->mount);
	}

	return 0;
}

static int
edit_restore(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->info = edit->old_info;
	*edit->wanted = edit->old_usage;
	return 0;
}

static int
edit_cancel(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->rv = -1;
	return 1;
}

static int
edit_delete_ptn(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;

	edit->rv = -2;
	return 1;
}

/*
 * We have added/removed partitions, all cur_part_id values are
 * out of sync. Re-fetch and reorder partitions accordingly.
 */
static void
renumber_partitions(struct partition_usage_set *pset)
{
	struct part_usage_info *ninfos;
	struct disk_part_info info;
	size_t i;
	part_id pno;

	ninfos = calloc(pset->parts->num_part, sizeof(*ninfos));
	if (ninfos == NULL) {
		err_msg_win(err_outofmem);
		return;
	}

	for (pno = 0; pno < pset->parts->num_part; pno++) {
		if (!pset->parts->pscheme->get_part_info(pset->parts, pno,
		    &info))
			continue;
		for (i = 0; i < pset->parts->num_part; i++) {
			if (pset->infos[i].cur_start != info.start)
				continue;
			if (pset->infos[i].cur_flags != info.flags)
				continue;
			if ((info.fs_type != FS_UNUSED &&
			    info.fs_type == pset->infos[i].fs_type) ||
			    (pset->infos[i].type ==
			    info.nat_type->generic_ptype)) {
				memcpy(&ninfos[pno], &pset->infos[i],
				    sizeof(ninfos[pno]));
				ninfos[pno].cur_part_id = pno;
				break;
			}
		}
	}

	memcpy(pset->infos, ninfos, sizeof(*pset->infos)*pset->parts->num_part);
	free(ninfos);
}

/*
 * Most often used file system types, we offer them in a first level menu.
 */
static const uint edit_fs_common_types[] =
    { FS_BSDFFS, FS_SWAP, FS_MSDOS, FS_EFI_SP, FS_BSDLFS, FS_EX2FS };

/*
 * Functions for uncommon file system types - we offer the full list,
 * but put FFSv2 and FFSv1 at the front and duplicat FS_MSDOS as
 * EFI system partition.
 */
static void
init_fs_type_ext(menudesc *menu, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	uint t = edit->info.fs_type;
	size_t i, ndx, max = menu->numopts;

	if (t == FS_BSDFFS) {
		if (edit->info.fs_sub_type == 2)
			menu->cursel = 0;
		else
			menu->cursel = 1;
		return;
	} else if (t == FS_EX2FS && edit->info.fs_sub_type == 1) {
		menu->cursel = FSMAXTYPES;
		return;
	}
	/* skip the two FFS entries, and do not add FFS later again */
	for (ndx = 2, i = 0; i < FSMAXTYPES && ndx < max; i++) {
		if (i == FS_UNUSED)
			continue;
		if (i == FS_BSDFFS)
			continue;
		if (fstypenames[i] == NULL)
			continue;

		if (i == t) {
			menu->cursel = ndx;
			break;
		}
		if (i == FS_MSDOS) {
			ndx++;
			if (t == FS_EFI_SP) {
				menu->cursel = ndx;
				break;
			}
		}
		ndx++;
	}
}

static int
set_fstype_ext(menudesc *menu, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	size_t i, ndx, max = menu->numopts;
	enum part_type pt;

	if (menu->cursel == 0 || menu->cursel == 1) {
		edit->info.fs_type = FS_BSDFFS;
		edit->info.fs_sub_type = menu->cursel == 0 ? 2 : 1;
		goto found_type;
	} else if (menu->cursel == FSMAXTYPES) {
		edit->info.fs_type = FS_EX2FS;
		edit->info.fs_sub_type = 1;
		goto found_type;
	}

	for (ndx = 2, i = 0; i < FSMAXTYPES && ndx < max; i++) {
		if (i == FS_UNUSED)
			continue;
		if (i == FS_BSDFFS)
			continue;
		if (fstypenames[i] == NULL)
			continue;

		if (ndx == (size_t)menu->cursel) {
			edit->info.fs_type = i;
			edit->info.fs_sub_type = 0;
			goto found_type;
		}
		ndx++;
		if (i == FS_MSDOS) {
			ndx++;
			if (ndx == (size_t)menu->cursel) {
				edit->info.fs_type = FS_EFI_SP;
				edit->info.fs_sub_type = 0;
				goto found_type;
			}
		}
	}
	return 1;

found_type:
	pt = edit->info.nat_type ? edit->info.nat_type->generic_ptype : PT_root;
	edit->info.nat_type = edit->pset->parts->pscheme->
	    get_fs_part_type(pt, edit->info.fs_type, edit->info.fs_sub_type);
	if (edit->info.nat_type == NULL)
		edit->info.nat_type = edit->pset->parts->pscheme->
		    get_generic_part_type(PT_root);
	edit->wanted->type = edit->info.nat_type->generic_ptype;
	edit->wanted->fs_type = edit->info.fs_type;
	edit->wanted->fs_version = edit->info.fs_sub_type;
	return 1;
}

/*
 * Offer a menu with "exotic" file system types, start with FFSv2 and FFSv1,
 * skip later FFS entry in the generic list.
 */
static int
edit_fs_type_ext(menudesc *menu, void *arg)
{
	menu_ent *opts;
	int m;
	size_t i, ndx, cnt;

	cnt = __arraycount(fstypenames)+1;
	opts = calloc(cnt, sizeof(*opts));
	if (opts == NULL)
		return 1;

	ndx = 0;
	opts[ndx].opt_name = msg_string(MSG_fs_type_ffsv2);
	opts[ndx].opt_action = set_fstype_ext;
	ndx++;
	opts[ndx].opt_name = msg_string(MSG_fs_type_ffs);
	opts[ndx].opt_action = set_fstype_ext;
	ndx++;
	for (i = 0; i < FSMAXTYPES && ndx < cnt; i++) {
		if (i == FS_UNUSED)
			continue;
		if (i == FS_BSDFFS)
			continue;
		if (fstypenames[i] == NULL)
			continue;
		opts[ndx].opt_name = fstypenames[i];
		opts[ndx].opt_action = set_fstype_ext;
		ndx++;
		if (i == FS_MSDOS) {
			opts[ndx] = opts[ndx-1];
			opts[ndx].opt_name = getfslabelname(FS_EFI_SP, 0);
			ndx++;
		}
	}
	opts[ndx].opt_name = msg_string(MSG_fs_type_ext2old);
	opts[ndx].opt_action = set_fstype_ext;
	ndx++;
	assert(ndx == cnt);
	m = new_menu(MSG_Select_the_type, opts, ndx,
		30, 6, 10, 0, MC_SUBMENU | MC_SCROLL,
		init_fs_type_ext, NULL, NULL, NULL, MSG_unchanged);

	if (m < 0)
		return 1;
	process_menu(m, arg);
	free_menu(m);
	free(opts);

	return 1;
}

static void
init_fs_type(menudesc *menu, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	size_t i;

	/* init menu->cursel from fs type in arg */
	if (edit->info.fs_type == FS_BSDFFS) {
		if (edit->info.fs_sub_type == 2)
			menu->cursel = 0;
		else
			menu->cursel = 1;
	}
	for (i = 1; i < __arraycount(edit_fs_common_types); i++) {
		if (edit->info.fs_type == edit_fs_common_types[i]) {
			menu->cursel = i+1;
			break;
		}
	}
}

static int
set_fstype(menudesc *menu, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	enum part_type pt;
	int ndx;

	pt = edit->info.nat_type ? edit->info.nat_type->generic_ptype : PT_root;
	if (menu->cursel < 2) {
		edit->info.fs_type = FS_BSDFFS;
		edit->info.fs_sub_type = menu->cursel == 0 ? 2 : 1;
		edit->info.nat_type = edit->pset->parts->pscheme->
		    get_fs_part_type(pt, FS_BSDFFS, 2);
		if (edit->info.nat_type == NULL)
			edit->info.nat_type = edit->pset->parts->
			    pscheme->get_generic_part_type(PT_root);
		edit->wanted->type = edit->info.nat_type->generic_ptype;
		edit->wanted->fs_type = edit->info.fs_type;
		edit->wanted->fs_version = edit->info.fs_sub_type;
		return 1;
	}
	ndx = menu->cursel-1;

	if (ndx < 0 ||
	    (size_t)ndx >= __arraycount(edit_fs_common_types))
		return 1;

	edit->info.fs_type = edit_fs_common_types[ndx];
	edit->info.fs_sub_type = 0;
	edit->info.nat_type = edit->pset->parts->pscheme->
	    get_fs_part_type(pt, edit->info.fs_type, 0);
	if (edit->info.nat_type == NULL)
		edit->info.nat_type = edit->pset->parts->
		    pscheme->get_generic_part_type(PT_root);
	edit->wanted->type = edit->info.nat_type->generic_ptype;
	edit->wanted->fs_type = edit->info.fs_type;
	edit->wanted->fs_version = edit->info.fs_sub_type;
	return 1;
}

/*
 * Offer a menu selecting the common file system types
 */
static int
edit_fs_type(menudesc *menu, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	menu_ent *opts;
	int m, cnt;
	size_t i;

	/*
	 * Shortcut to full menu if we have an exotic value
	 */
	if (edit->info.fs_type == FS_EX2FS && edit->info.fs_sub_type == 1) {
		edit_fs_type_ext(menu, arg);
		return 0;
	}
	for (i = 0; i < __arraycount(edit_fs_common_types); i++)
		if (edit->info.fs_type == edit_fs_common_types[i])
			break;
	if (i >= __arraycount(edit_fs_common_types)) {
		edit_fs_type_ext(menu, arg);
		return 0;
	}

	/*
	 * Starting with a common type, show short menu first
	 */
	cnt = __arraycount(edit_fs_common_types) + 2;
	opts = calloc(cnt, sizeof(*opts));
	if (opts == NULL)
		return 0;

	/* special case entry 0: two FFS entries */
	for (i = 0; i < __arraycount(edit_fs_common_types); i++) {
		opts[i+1].opt_name = getfslabelname(edit_fs_common_types[i], 0);
		opts[i+1].opt_action = set_fstype;
	}
	/* duplicate FFS (at offset 1) into first entry */
	opts[0] = opts[1];
	opts[0].opt_name = msg_string(MSG_fs_type_ffsv2);
	opts[1].opt_name = msg_string(MSG_fs_type_ffs);
	/* add secondary sub-menu */
	assert(i+1 < (size_t)cnt);
	opts[i+1].opt_name = msg_string(MSG_other_fs_type);
	opts[i+1].opt_action = edit_fs_type_ext;

	m = new_menu(MSG_Select_the_type, opts, cnt,
		30, 6, 0, 0, MC_SUBMENU | MC_SCROLL,
		init_fs_type, NULL, NULL, NULL, MSG_unchanged);

	if (m < 0)
		return 0;
	process_menu(m, arg);
	free_menu(m);
	free(opts);

	return 0;
}


static void update_edit_ptn_menu(menudesc *m, void *arg);
static void draw_edit_ptn_line(menudesc *m, int opt, void *arg);
static int edit_ptn_custom_type(menudesc *m, void *arg);

static void
remember_deleted(struct partition_usage_set *pset,
    struct disk_partitions *parts)
{
	size_t i, num;
	struct disk_partitions **tab;

	/* do we have parts on record already? */
	for (i = 0; i < pset->num_write_back; i++)
		if (pset->write_back[i] == parts)
			return;
	/*
	 * Need to record this partition table for write back
	 */
	num = pset->num_write_back + 1;
	tab = realloc(pset->write_back, num*sizeof(*pset->write_back));
	if (!tab)
		return;
	tab[pset->num_write_back] = parts;
	pset->write_back = tab;
	pset->num_write_back = num;
}

int
edit_ptn(menudesc *menu, void *arg)
{
	struct partition_usage_set *pset = arg;
	struct single_part_fs_edit edit;
	int fspart_menu, num_opts;
	const char *err;
	menu_ent *mopts, *popt;
	bool is_new_part, with_inst_opt = pset->parts->parent == NULL;

	static const menu_ent edit_ptn_fields_head[] = {
		{ .opt_action=edit_fs_type },
		{ .opt_action=edit_fs_start },
		{ .opt_action=edit_fs_size },
		{ .opt_flags=OPT_IGNORE },
	};

	static const menu_ent edit_ptn_fields_head_add[] = {
		{ .opt_action=edit_install },
	};

	static const menu_ent edit_ptn_fields_head2[] = {
		{ .opt_action=edit_fs_preserve },
		{ .opt_action=edit_fs_mount },
		{ .opt_menu=MENU_mountoptions, .opt_flags=OPT_SUB },
		{ .opt_action=edit_fs_mountpt },
	};

	static const menu_ent edit_ptn_fields_ffs[] = {
		{ .opt_action=edit_fs_ffs_avg_size },
		{ .opt_action=edit_fs_ffs_block },
		{ .opt_action=edit_fs_ffs_frag },
	};

	static const menu_ent edit_ptn_fields_tail[] = {
		{ .opt_name=MSG_askunits, .opt_menu=MENU_sizechoice,
		  .opt_flags=OPT_SUB },
		{ .opt_name=MSG_restore,
		  .opt_action=edit_restore},
		{ .opt_name=MSG_Delete_partition,
		  .opt_action=edit_delete_ptn},
		{ .opt_name=MSG_cancel,
		  .opt_action=edit_cancel},
	};

	memset(&edit, 0, sizeof edit);
	edit.pset = pset;
	edit.index = menu->cursel;
	edit.wanted = &pset->infos[edit.index];

	if (menu->cursel < 0 || (size_t)menu->cursel > pset->parts->num_part)
		return 0;
	is_new_part = (size_t)menu->cursel == pset->parts->num_part;

	num_opts = __arraycount(edit_ptn_fields_head) +
	    __arraycount(edit_ptn_fields_head2) +
	    __arraycount(edit_ptn_fields_tail);
	if (edit.wanted->fs_type == FS_BSDFFS ||
	    edit.wanted->fs_type == FS_BSDLFS)
		num_opts += __arraycount(edit_ptn_fields_ffs);
	if (with_inst_opt)
		num_opts += __arraycount(edit_ptn_fields_head_add);
	if (is_new_part)
		num_opts--;
	else
		num_opts += pset->parts->pscheme->custom_attribute_count;

	mopts = calloc(num_opts, sizeof(*mopts));
	if (mopts == NULL) {
		err_msg_win(err_outofmem);
		return 0;
	}
	memcpy(mopts, edit_ptn_fields_head, sizeof(edit_ptn_fields_head));
	popt = mopts + __arraycount(edit_ptn_fields_head);
	if (with_inst_opt) {
		memcpy(popt, edit_ptn_fields_head_add,
		    sizeof(edit_ptn_fields_head_add));
		popt +=  __arraycount(edit_ptn_fields_head_add);
	}
	memcpy(popt, edit_ptn_fields_head2, sizeof(edit_ptn_fields_head2));
	popt +=  __arraycount(edit_ptn_fields_head2);
	if (edit.wanted->fs_type == FS_BSDFFS ||
	    edit.wanted->fs_type == FS_BSDLFS) {
		memcpy(popt, edit_ptn_fields_ffs, sizeof(edit_ptn_fields_ffs));
		popt +=  __arraycount(edit_ptn_fields_ffs);
	}
	edit.first_custom_attr = popt - mopts;
	if (!is_new_part) {
		for (size_t i = 0;
		    i < pset->parts->pscheme->custom_attribute_count;
		    i++, popt++) {
			popt->opt_action = edit_ptn_custom_type;
		}
	}
	memcpy(popt, edit_ptn_fields_tail, sizeof(edit_ptn_fields_tail));
	popt += __arraycount(edit_ptn_fields_tail) - 1;
	if (is_new_part)
		memcpy(popt-1, popt, sizeof(*popt));

	if (is_new_part) {
		struct disk_part_free_space space;
		daddr_t align = pset->parts->pscheme->get_part_alignment(
		    pset->parts);

		edit.id = NO_PART;
		if (pset->parts->pscheme->get_free_spaces(pset->parts,
		    &space, 1, align, align, -1, -1) == 1) {
			edit.info.start = space.start;
			edit.info.size = space.size;
			edit.info.fs_type = FS_BSDFFS;
			edit.info.fs_sub_type = 2;
			edit.info.nat_type = pset->parts->pscheme->
			    get_fs_part_type(PT_root, edit.info.fs_type,
			    edit.info.fs_sub_type);
			edit.wanted->instflags = PUIINST_NEWFS;
		}
	} else {
		edit.id = pset->infos[edit.index].cur_part_id;
		if (!pset->parts->pscheme->get_part_info(pset->parts, edit.id,
		    &edit.info)) {
			free(mopts);
			return 0;
		}
	}

	edit.old_usage = *edit.wanted;
	edit.old_info = edit.info;

	fspart_menu = new_menu(MSG_edfspart, mopts, num_opts,
		15, 2, 0, 55, MC_NOCLEAR | MC_SCROLL,
		update_edit_ptn_menu, draw_edit_ptn_line, NULL,
		NULL, MSG_OK);

	process_menu(fspart_menu, &edit);
	free(mopts);
	free_menu(fspart_menu);

	if (edit.rv == 0) {	/* OK, set new data */
		edit.info.last_mounted = edit.wanted->mount;
		if (is_new_part) {
			edit.wanted->parts = pset->parts;
			edit.wanted->cur_part_id = pset->parts->pscheme->
			    add_partition(pset->parts, &edit.info, &err);
			if (edit.wanted->cur_part_id == NO_PART)
				err_msg_win(err);
			else {
				pset->parts->pscheme->get_part_info(
				    pset->parts, edit.wanted->cur_part_id,
				    &edit.info);
				edit.wanted->cur_start = edit.info.start;
				edit.wanted->size = edit.info.size;
				edit.wanted->type =
				    edit.info.nat_type->generic_ptype;
				edit.wanted->fs_type = edit.info.fs_type;
				edit.wanted->fs_version = edit.info.fs_sub_type;
				/* things have changed, re-sort */
				renumber_partitions(pset);
			}
		} else {
			if (!pset->parts->pscheme->set_part_info(pset->parts,
			    edit.id, &edit.info, &err))
				err_msg_win(err);
		}

		/*
		 * if size has changed, we may need to add or remove
		 * the option to add partitions
		 */
		show_partition_adder(menu, pset);
	} else if (edit.rv == -1) {	/* cancel edit */
		if (is_new_part) {
			memmove(pset->infos+edit.index,
			    pset->infos+edit.index+1,
			    sizeof(*pset->infos)*(pset->num-edit.index));
			memmove(menu->opts+edit.index,
			    menu->opts+edit.index+1,
			    sizeof(*menu->opts)*(menu->numopts-edit.index));
			menu->numopts--;
			menu->cursel = 0;
			pset->num--;
			return -1;
		}
		pset->infos[edit.index] = edit.old_usage;
	} else if (!is_new_part && edit.rv == -2) {	/* delete partition */
		if (!pset->parts->pscheme->delete_partition(pset->parts,
		    edit.id, &err)) {
			err_msg_win(err);
			return 0;
		}
		remember_deleted(pset,
		    pset->infos[edit.index].parts);
		pset->cur_free_space += pset->infos[edit.index].size;
		memmove(pset->infos+edit.index,
		    pset->infos+edit.index+1,
		    sizeof(*pset->infos)*(pset->num-edit.index));
		memmove(menu->opts+edit.index,
		    menu->opts+edit.index+1,
		    sizeof(*menu->opts)*(menu->numopts-edit.index));
		menu->numopts--;
		menu->cursel = 0;
		if (pset->parts->num_part == 0)
			menu->cursel = 1;	/* skip sentinel line */

		/* things have changed, re-sort */
		pset->num--;
		renumber_partitions(pset);

		/* we can likely add new partitions now */
		show_partition_adder(menu, pset);

		return -1;
	}

	return 0;
}

static void
update_edit_ptn_menu(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	int i;
	uint t = edit->info.fs_type;
	size_t attr_no;

	/* Determine which of the properties can be changed */
	for (i = 0; i < m->numopts; i++) {
		if (m->opts[i].opt_action == NULL &&
		    m->opts[i].opt_menu != MENU_mountoptions)
			continue;

		/* Default to disabled... */
		m->opts[i].opt_flags |= OPT_IGNORE;
		if ((t == FS_UNUSED || t == FS_SWAP) &&
		    (m->opts[i].opt_action == edit_fs_preserve ||
		     m->opts[i].opt_action == edit_fs_mount ||
		     m->opts[i].opt_action == edit_fs_mountpt ||
		     m->opts[i].opt_menu == MENU_mountoptions))
			continue;
		if (m->opts[i].opt_action == edit_install &&
		    edit->info.nat_type &&
		    edit->info.nat_type->generic_ptype != PT_root)
			/* can only install onto PT_root partitions */
			continue;
		if (m->opts[i].opt_action == edit_fs_preserve &&
		    t != FS_BSDFFS && t != FS_BSDLFS && t != FS_APPLEUFS &&
		    t != FS_MSDOS && t != FS_EFI_SP && t != FS_EX2FS) {
			/* Can not newfs this filesystem */
			edit->wanted->instflags &= ~PUIINST_NEWFS;
			continue;
		}
		if (m->opts[i].opt_action == edit_ptn_custom_type) {
			attr_no = (size_t)i - edit->first_custom_attr;
			if (!edit->pset->parts->pscheme->
			    custom_attribute_writable(
			    edit->pset->parts, edit->id, attr_no))
				continue;
		}
		/*
		 * Do not allow editing of size/start/type when partition
		 * is defined in some outer partition table already
		 */
		if ((edit->pset->infos[edit->index].flags & PUIFLG_IS_OUTER)
		    && (m->opts[i].opt_action == edit_fs_type
			|| m->opts[i].opt_action == edit_fs_start
			|| m->opts[i].opt_action == edit_fs_size))
				continue;
		/* Ok: we want this one */
		m->opts[i].opt_flags &= ~OPT_IGNORE;
	}

	/* Avoid starting at a (now) disabled menu item */
	while (m->cursel >= 0 && m->cursel < m->numopts
	    && (m->opts[m->cursel].opt_flags & OPT_IGNORE))
		m->cursel++;
}

static void
draw_edit_ptn_line(menudesc *m, int opt, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	static int col_width;
	static const char *ptn_type, *ptn_start, *ptn_size, *ptn_end,
	     *ptn_newfs, *ptn_mount, *ptn_mount_options, *ptn_mountpt,
	     *ptn_install, *ptn_bsize, *ptn_fsize, *ptn_isize;
	const char *c;
	char val[MENUSTRSIZE];
	const char *attrname;
	size_t attr_no;

	if (col_width == 0) {
		int l;

#define	LOAD(STR)	STR = msg_string(MSG_##STR); l = strlen(STR); \
			if (l > col_width) col_width = l

		LOAD(ptn_type);
		LOAD(ptn_start);
		LOAD(ptn_size);
		LOAD(ptn_end);
		LOAD(ptn_install);
		LOAD(ptn_newfs);
		LOAD(ptn_mount);
		LOAD(ptn_mount_options);
		LOAD(ptn_mountpt);
		LOAD(ptn_bsize);
		LOAD(ptn_fsize);
		LOAD(ptn_isize);
#undef LOAD

		for (size_t i = 0;
		    i < edit->pset->parts->pscheme->custom_attribute_count;
		    i++) {
			attrname = msg_string(
			    edit->pset->parts->pscheme->custom_attributes[i]
			    .label);
			l = strlen(attrname);
			if (l > col_width) col_width = l;
		}

		col_width += 3;
	}

	if (m->opts[opt].opt_flags & OPT_IGNORE
	    && (opt != 3 || edit->info.fs_type == FS_UNUSED)
	    && m->opts[opt].opt_action != edit_ptn_custom_type
	    && m->opts[opt].opt_action != edit_fs_type
	    && m->opts[opt].opt_action != edit_fs_start
	    && m->opts[opt].opt_action != edit_fs_size) {
		wprintw(m->mw, "%*s -", col_width, "");
		return;
	}

	if (opt < 4) {
		switch (opt) {
		case 0:
			if (edit->info.fs_type == FS_BSDFFS)
				if (edit->info.fs_sub_type == 2)
					c = msg_string(MSG_fs_type_ffsv2);
				else
					c = msg_string(MSG_fs_type_ffs);
			else
				c = getfslabelname(edit->info.fs_type,
				    edit->info.fs_sub_type);
			wprintw(m->mw, "%*s : %s", col_width, ptn_type, c);
			return;
		case 1:
			wprintw(m->mw, "%*s : %" PRIu64 " %s", col_width,
			    ptn_start, edit->info.start / sizemult, multname);
			return;
		case 2:
			wprintw(m->mw, "%*s : %" PRIu64 " %s", col_width,
			    ptn_size, edit->info.size / sizemult, multname);
			return;
		case 3:
			wprintw(m->mw, "%*s : %" PRIu64 " %s", col_width,
			    ptn_end, (edit->info.start + edit->info.size)
			    / sizemult, multname);
			return;
		 }
	}
	if (m->opts[opt].opt_action == edit_install) {
		wprintw(m->mw, "%*s : %s", col_width, ptn_install,
			msg_string((edit->info.flags & PTI_INSTALL_TARGET)
			    ? MSG_Yes : MSG_No));
		return;
	}
	if (m->opts[opt].opt_action == edit_fs_preserve) {
		wprintw(m->mw, "%*s : %s", col_width, ptn_newfs,
			msg_string(edit->wanted->instflags & PUIINST_NEWFS
			    ? MSG_Yes : MSG_No));
		return;
	}
	if (m->opts[opt].opt_action == edit_fs_mount) {
		wprintw(m->mw, "%*s : %s", col_width, ptn_mount,
			msg_string(edit->wanted->instflags & PUIINST_MOUNT
			    ? MSG_Yes : MSG_No));
		return;
	}
	if (m->opts[opt].opt_action == edit_fs_ffs_block) {
		wprintw(m->mw, "%*s : %u", col_width, ptn_bsize,
			edit->wanted->fs_opt1);
		return;
	}
	if (m->opts[opt].opt_action == edit_fs_ffs_frag) {
		wprintw(m->mw, "%*s : %u", col_width, ptn_fsize,
			edit->wanted->fs_opt2);
		return;
	}
	if (m->opts[opt].opt_action == edit_fs_ffs_avg_size) {
		if (edit->wanted->fs_opt3 == 0)
			wprintw(m->mw, "%*s : %s", col_width, ptn_isize,
				msg_string(MSG_ptn_isize_dflt));
		else {
        	        char buf[24], *line;
			const char *t = buf;

			snprintf(buf, sizeof buf, "%u", edit->wanted->fs_opt3);
			line = str_arg_subst(msg_string(MSG_ptn_isize_bytes),
			    1, &t);
			wprintw(m->mw, "%*s : %s", col_width, ptn_isize,
				line);
			free(line);
		}
		return;
	}
	if (m->opts[opt].opt_menu == MENU_mountoptions) {
		wprintw(m->mw, "%*s : ", col_width, ptn_mount_options);
		if (edit->wanted->mountflags & PUIMNT_ASYNC)
			wprintw(m->mw, "async ");
		if (edit->wanted->mountflags & PUIMNT_NOATIME)
			wprintw(m->mw, "noatime ");
		if (edit->wanted->mountflags & PUIMNT_NODEV)
			wprintw(m->mw, "nodev ");
		if (edit->wanted->mountflags & PUIMNT_NODEVMTIME)
			wprintw(m->mw, "nodevmtime ");
		if (edit->wanted->mountflags & PUIMNT_NOEXEC)
			wprintw(m->mw, "noexec ");
		if (edit->wanted->mountflags & PUIMNT_NOSUID)
			wprintw(m->mw, "nosuid ");
		if (edit->wanted->mountflags & PUIMNT_LOG)
			wprintw(m->mw, "log ");
		if (edit->wanted->mountflags & PUIMNT_NOAUTO)
			wprintw(m->mw, "noauto  ");
		return;
	}
	if (m->opts[opt].opt_action == edit_fs_mountpt) {
		wprintw(m->mw, "%*s : %s", col_width, ptn_mountpt,
		    edit->wanted->mount);
		return;
	}

	attr_no = opt - edit->first_custom_attr;
	edit->pset->parts->pscheme->format_custom_attribute(
	    edit->pset->parts, edit->id, attr_no, &edit->info,
	    val, sizeof val);
	attrname = msg_string(edit->pset->parts->pscheme->
	    custom_attributes[attr_no].label);
	wprintw(m->mw, "%*s : %s", col_width, attrname, val);
}

static int
edit_ptn_custom_type(menudesc *m, void *arg)
{
	struct single_part_fs_edit *edit = arg;
	size_t attr_no = m->cursel - edit->first_custom_attr;
	char line[STRSIZE];

	switch (edit->pset->parts->pscheme->custom_attributes[attr_no].type) {
	case pet_bool:
		edit->pset->parts->pscheme->custom_attribute_toggle(
		    edit->pset->parts, edit->id, attr_no);
		break;
	case pet_cardinal:
	case pet_str:
		edit->pset->parts->pscheme->format_custom_attribute(
		    edit->pset->parts, edit->id, attr_no, &edit->info,
		    line, sizeof(line));
		msg_prompt_win(
		    edit->pset->parts->pscheme->custom_attributes[attr_no].
		    label, -1, 18, 0, 0, line, line, sizeof(line));
		edit->pset->parts->pscheme->custom_attribute_set_str(
		    edit->pset->parts, edit->id, attr_no, line);
		break;
	}

	return 0;
}


/*
 * Some column width depend on translation, we will set these in
 * fmt_fspart_header and later use it when formatting single entries
 * in fmt_fspart_row.
 * The table consist of 3 "size like" columns, all fixed width, then
 * ptnheaders_fstype, part_header_col_flag, and finally the mount point
 * (which is variable width).
 */
static int fstype_width, flags_width;
static char fspart_separator[MENUSTRSIZE];
static char fspart_title[2*MENUSTRSIZE];

/*
 * Format the header of the main partition editor menu.
 */
static void
fmt_fspart_header(menudesc *menu, void *arg)
{
	struct partition_usage_set *pset = arg;
	char total[6], free_space[6], scol[13], ecol[13], szcol[13],
	    sepline[MENUSTRSIZE], *p, desc[MENUSTRSIZE];
	const char *fstype, *flags;
	int i;
	size_t ptn;
	bool with_clone, with_inst_flag = pset->parts->parent == NULL;

	with_clone = false;
	for (ptn = 0; ptn < pset->num && !with_clone; ptn++)
		if (pset->infos[ptn].flags & PUIFLG_CLONE_PARTS)
			with_clone = true;
	humanize_number(total, sizeof total,
	    pset->parts->disk_size * pset->parts->bytes_per_sector,
	    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
	humanize_number(free_space, sizeof free_space,
	    pset->cur_free_space * pset->parts->bytes_per_sector,
	    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

	if (with_clone)
		strlcpy(desc, msg_string(MSG_clone_flag_desc), sizeof desc);
	else
		desc[0] = 0;
	if (pset->parts->pscheme->part_flag_desc)
		strlcat(desc, msg_string(pset->parts->pscheme->part_flag_desc),
		sizeof desc);

	msg_display_subst(MSG_fspart, 7, pset->parts->disk,
	    msg_string(pset->parts->pscheme->name),
	    msg_string(pset->parts->pscheme->short_name),
	    with_inst_flag ? msg_string(MSG_ptn_instflag_desc) : "",
	    desc, total, free_space);

	snprintf(scol, sizeof scol, "%s (%s)",
	    msg_string(MSG_ptnheaders_start), multname);
	snprintf(ecol, sizeof ecol, "%s (%s)",
	    msg_string(MSG_ptnheaders_end), multname);
	snprintf(szcol, sizeof szcol, "%s (%s)",
	    msg_string(MSG_ptnheaders_size), multname);

	fstype = msg_string(MSG_ptnheaders_fstype);
	flags = msg_string(MSG_part_header_col_flag);
	fstype_width = max(strlen(fstype), 8);
	flags_width = strlen(flags);
	for (i = 0, p = sepline; i < fstype_width; i++)
		*p++ = '-';
	for (i = 0, *p++ = ' '; i < flags_width; i++)
		*p++ = '-';
	*p = 0;

	snprintf(fspart_separator, sizeof(fspart_separator),
	    "------------ ------------ ------------ %s ----------------",
	    sepline);

	snprintf(fspart_title, sizeof(fspart_title),
	    "   %12.12s %12.12s %12.12s %*s %*s %s\n"
	    "   %s", scol, ecol, szcol, fstype_width, fstype,
	    flags_width, flags, msg_string(MSG_ptnheaders_filesystem),
	    fspart_separator);

	msg_table_add("\n\n");
}

/*
 * Format one partition entry in the main partition editor.
 */
static void
fmt_fspart_row(menudesc *m, int ptn, void *arg)
{
	struct partition_usage_set *pset = arg;
	struct disk_part_info info;
	daddr_t poffset, psize, pend;
	const char *desc;
	static const char *Yes;
	char flag_str[MENUSTRSIZE], *fp;
	unsigned inst_flags;
#ifndef NO_CLONES
	size_t clone_cnt;
#endif
	bool with_inst_flag = pset->parts->parent == NULL;

	if (Yes == NULL)
		Yes = msg_string(MSG_Yes);

#ifndef NO_CLONES
	if ((pset->infos[ptn].flags & PUIFLG_CLONE_PARTS) &&
	   pset->infos[ptn].cur_part_id == NO_PART) {
		psize = pset->infos[ptn].size / sizemult;
		if (pset->infos[ptn].clone_ndx <
		    pset->infos[ptn].clone_src->num_sel)
			clone_cnt = 1;
		else
			clone_cnt = pset->infos[ptn].clone_src->num_sel;
		if (pset->infos[ptn].cur_part_id == NO_PART)
			wprintw(m->mw, "                          %12" PRIu64
			    " [%zu %s]", psize, clone_cnt,
			    msg_string(MSG_clone_target_disp));
		else {
			poffset = pset->infos[ptn].cur_start / sizemult;
			pend = (pset->infos[ptn].cur_start +
			    pset->infos[ptn].size) / sizemult - 1;
			wprintw(m->mw, "%12" PRIu64 " %12" PRIu64 " %12" PRIu64
			    " [%zu %s]",
			    poffset, pend, psize, clone_cnt,
			    msg_string(MSG_clone_target_disp));
		}
		if (m->title == fspart_title)
			m->opts[ptn].opt_flags |= OPT_IGNORE;
		else
			m->opts[ptn].opt_flags &= ~OPT_IGNORE;
		return;
	}
#endif

	if (!real_partition(pset, ptn))
		return;

	if (!pset->parts->pscheme->get_part_info(pset->parts,
	    pset->infos[ptn].cur_part_id, &info))
		return;

	/*
	 * We use this function in multiple menus, but only want it
	 * to play with enable/disable in a single one:
	 */
	if (m->title == fspart_title) {
		/*
		 * Enable / disable this line if it is something
		 * like RAW_PART
		 */
		if ((info.flags &
		    (PTI_WHOLE_DISK|PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
		    || (pset->infos[ptn].flags & PUIFLG_CLONE_PARTS))
			m->opts[ptn].opt_flags |= OPT_IGNORE;
		else
			m->opts[ptn].opt_flags &= ~OPT_IGNORE;
	}

	poffset = info.start / sizemult;
	psize = info.size / sizemult;
	if (psize == 0)
		pend = 0;
	else
		pend = (info.start + info.size) / sizemult - 1;

	if (info.flags & PTI_WHOLE_DISK)
		desc = msg_string(MSG_NetBSD_partition_cant_change);
	else if (info.flags & PTI_RAW_PART)
		desc = msg_string(MSG_Whole_disk_cant_change);
	else if (info.flags & PTI_BOOT)
		desc = msg_string(MSG_Boot_partition_cant_change);
	else
		desc = getfslabelname(info.fs_type, info.fs_sub_type);

	fp = flag_str;
	inst_flags = pset->infos[ptn].instflags;
	if (with_inst_flag && (info.flags & PTI_INSTALL_TARGET) &&
	    info.nat_type->generic_ptype == PT_root) {
		static char inst_flag;

		if (inst_flag == 0)
			inst_flag = msg_string(MSG_install_flag)[0];
		*fp++ = inst_flag;
	}
	if (inst_flags & PUIINST_NEWFS)
		*fp++ = msg_string(MSG_newfs_flag)[0];
	if (pset->infos[ptn].flags & PUIFLG_CLONE_PARTS)
		*fp++ = msg_string(MSG_clone_flag)[0];
	*fp = 0;
	if (pset->parts->pscheme->get_part_attr_str != NULL)
		pset->parts->pscheme->get_part_attr_str(pset->parts,
		    pset->infos[ptn].cur_part_id, fp, sizeof(flag_str)-1);

	/* if the fstype description does not fit, check if we can overrun */
	if (strlen(desc) > (size_t)fstype_width &&
	     flag_str[0] == 0 && (info.last_mounted == NULL ||
	     info.last_mounted[0] == 0))
		wprintw(m->mw, "%12" PRIu64 " %12" PRIu64 " %12" PRIu64
		    " %s",
		    poffset, pend, psize, desc);
	else
		wprintw(m->mw, "%12" PRIu64 " %12" PRIu64 " %12" PRIu64
		    " %*.*s %*s %s",
		    poffset, pend, psize, fstype_width, fstype_width, desc,
		    -flags_width, flag_str,
		    (inst_flags & PUIINST_MOUNT) && info.last_mounted &&
		     info.last_mounted[0] ? info.last_mounted : "");
}

#ifndef NO_CLONES
static int
part_ext_clone(menudesc *m, void *arg)
{
	struct selected_partitions selected, *clone_src;
	struct clone_target_menu_data data;
	struct partition_usage_set *pset = arg;
	struct part_usage_info *p;
	struct disk_part_info sinfo, cinfo;
	struct disk_partitions *csrc;
	struct disk_part_free_space space;
	menu_ent *men;
	daddr_t clone_size, free_size, offset, align;
	int num_men, i;
	size_t s, clone_cnt;
	part_id cid;
	struct clone_data {
		struct disk_part_info info;
		part_id new_id;
		size_t ndx;
	};
	struct clone_data *clones = NULL;

	if (!select_partitions(&selected, pm->parts))
		return 0;

	clone_size = selected_parts_size(&selected);
	num_men = pset->num+1;
	men = calloc(num_men, sizeof *men);
	if (men == NULL)
		return 0;
	for (i = 0; i < num_men; i++) {
		men[i].opt_action = clone_target_select;
		if (i == 0)
			free_size = pset->infos[i].cur_start;
		else if (i > 0 && (size_t)i < pset->num)
			free_size = pset->infos[i].cur_start -
			    pset->infos[i-1].cur_start - pset->infos[i-1].size;
		else
			free_size = pset->parts->free_space;
		if (free_size < clone_size)
			men[i].opt_flags = OPT_IGNORE;
	}
	men[num_men-1].opt_name = MSG_clone_target_end;

	memset(&data, 0, sizeof data);
	data.usage = *pset;
	data.res = -1;

	data.usage.menu = new_menu(MSG_clone_target_hdr,
	    men, num_men, 3, 2, 0, 65, MC_SCROLL,
	    NULL, fmt_fspart_row, NULL, NULL, MSG_cancel);
	process_menu(data.usage.menu, &data);
	free_menu(data.usage.menu);
	free(men);

	if (data.res < 0)
		goto err;

	/* create temporary infos for all clones that work out */
	clone_cnt = 0;
	clones = calloc(selected.num_sel, sizeof(*clones));
	if (clones == NULL)
		goto err;

	clone_src = malloc(sizeof(selected));
	if (clone_src == NULL)
		goto err;
	*clone_src = selected;

	/* find selected offset from data.res and insert clones there */
	align = pset->parts->pscheme->get_part_alignment(pset->parts);
	offset = -1;
	if (data.res > 0)
		offset = pset->infos[data.res-1].cur_start
		    + pset->infos[data.res-1].size;
	else
		offset = 0;
	for (s = 0; s < selected.num_sel; s++) {
		csrc = selected.selection[s].parts;
		cid = selected.selection[s].id;
		csrc->pscheme->get_part_info(csrc, cid, &sinfo);
		if (!pset->parts->pscheme->adapt_foreign_part_info(
		    pset->parts, &cinfo, csrc->pscheme, &sinfo))
			continue;
		size_t cnt = pset->parts->pscheme->get_free_spaces(
		    pset->parts, &space, 1, cinfo.size-align, align,
		    offset, -1);
		if (cnt == 0)
			continue;
		cinfo.start = space.start;
		cid = pset->parts->pscheme->add_partition(
		    pset->parts, &cinfo, NULL);
		if (cid == NO_PART)
			continue;
		pset->parts->pscheme->get_part_info(pset->parts, cid, &cinfo);
		clones[clone_cnt].info = cinfo;
		clones[clone_cnt].new_id = cid;
		clones[clone_cnt].ndx = s;
		clone_cnt++;
		offset = roundup(cinfo.start+cinfo.size, align);
	}

	/* insert new clone records at offset data.res */
	men = realloc(m->opts, (m->numopts+clone_cnt)*sizeof(*m->opts));
	if (men == NULL)
		goto err;
	pset->menu_opts = men;
	m->opts = men;
	m->numopts += clone_cnt;

	p = realloc(pset->infos, (pset->num+clone_cnt)*sizeof(*pset->infos));
	if (p == NULL)
		goto err;
	pset->infos = p;

	men += data.res;
	p += data.res;
	memmove(men+clone_cnt, men,
	    sizeof(*men)*(m->numopts-data.res-clone_cnt));
	if (pset->num > (size_t)data.res)
		memmove(p+clone_cnt, p, sizeof(*p)*(pset->num-data.res));
	memset(men, 0, sizeof(*men)*clone_cnt);
	memset(p, 0, sizeof(*p)*clone_cnt);
	for (s = 0; s < clone_cnt; s++) {
		p[s].cur_part_id = clones[s].new_id;
		p[s].cur_start = clones[s].info.start;
		p[s].size = clones[s].info.size;
		p[s].cur_flags = clones[s].info.flags;
		p[s].flags = PUIFLG_CLONE_PARTS;
		p[s].parts = pset->parts;
		p[s].clone_src = clone_src;
		p[s].clone_ndx = s;
	}
	free(clones);
	m->cursel = ((size_t)data.res >= pset->num) ? 0 : data.res+clone_cnt;
	pset->num += clone_cnt;
	m->h = 0;
	resize_menu_height(m);

	return -1;

err:
	free(clones);
	free_selected_partitions(&selected);
	return 0;
}
#endif

static int
edit_fspart_pack(menudesc *m, void *arg)
{
	struct partition_usage_set *pset = arg;
	char buf[STRSIZE];

	if (!pset->parts->pscheme->get_disk_pack_name(pset->parts,
	    buf, sizeof buf))
		return 0;

	msg_prompt_win(MSG_edit_disk_pack_hdr,
	    -1, 18, 0, -1, buf, buf, sizeof(buf));

	pset->parts->pscheme->set_disk_pack_name(pset->parts, buf);
	return 0;
}

static int
edit_fspart_add(menudesc *m, void *arg)
{
	struct partition_usage_set *pset = arg;
	struct part_usage_info *ninfo;
	menu_ent *nmenopts;
	size_t cnt, off;

	ninfo = realloc(pset->infos, (pset->num+1)*sizeof(*pset->infos));
	if (ninfo == NULL)
		return 0;
	pset->infos = ninfo;
	off = pset->parts->num_part;
	cnt = pset->num-pset->parts->num_part;
	if (cnt > 0)
		memmove(pset->infos+off+1,pset->infos+off,
		    cnt*sizeof(*pset->infos));
	memset(pset->infos+off, 0, sizeof(*pset->infos));

	nmenopts = realloc(m->opts, (m->numopts+1)*sizeof(*m->opts));
	if (nmenopts == NULL)
		return 0;
	memmove(nmenopts+off+1, nmenopts+off,
	    (m->numopts-off)*sizeof(*nmenopts));
	memset(&nmenopts[off], 0, sizeof(nmenopts[off]));
	nmenopts[off].opt_action = edit_ptn;
	pset->menu_opts = m->opts = nmenopts;
	m->numopts++;
	m->cursel = off;
	pset->num++;

	/* now update edit menu to fit */
	m->h = 0;
	resize_menu_height(m);

	/* and directly invoke the partition editor for the new part */
	edit_ptn(m, arg);

	show_partition_adder(m, pset);

	return -1;
}

static void
add_partition_adder(menudesc *m, struct partition_usage_set *pset)
{
	struct part_usage_info *ninfo;
	menu_ent *nmenopts;
	size_t off;

	ninfo = realloc(pset->infos, (pset->num+1)*sizeof(*pset->infos));
	if (ninfo == NULL)
		return;
	pset->infos = ninfo;
	off = pset->parts->num_part+1;

	nmenopts = realloc(m->opts, (m->numopts+1)*sizeof(*m->opts));
	if (nmenopts == NULL)
		return;
	memmove(nmenopts+off+1, nmenopts+off,
	    (m->numopts-off)*sizeof(*nmenopts));
	memset(&nmenopts[off], 0, sizeof(nmenopts[off]));

	nmenopts[off].opt_name = MSG_addpart;
	nmenopts[off].opt_flags = OPT_SUB;
	nmenopts[off].opt_action = edit_fspart_add;

	m->opts = nmenopts;
	m->numopts++;
}

static void
remove_partition_adder(menudesc *m, struct partition_usage_set *pset)
{
	size_t off;

	off = pset->parts->num_part+1;
	memmove(m->opts+off, m->opts+off+1,
	    (m->numopts-off-1)*sizeof(*m->opts));
	m->numopts--;
}

/*
 * Called whenever the "add a partition" option may need to be removed
 * or added
 */
static void
show_partition_adder(menudesc *m, struct partition_usage_set *pset)
{
	if (m->opts == NULL)
		return;

	bool can_add_partition = pset->parts->pscheme->can_add_partition(
	    pset->parts);
	bool part_adder_present =
	    (m->opts[pset->parts->num_part].opt_flags & OPT_IGNORE) &&
	    (m->opts[pset->parts->num_part+1].opt_action == edit_fspart_add);

	if (can_add_partition == part_adder_present)
		return;

	if (can_add_partition)
		add_partition_adder(m, pset);
	else
		remove_partition_adder(m, pset);

	/* now update edit menu to fit */
	m->h = 0;
	resize_menu_height(m);
}

static int
edit_fspart_abort(menudesc *m, void *arg)
{
	struct partition_usage_set *pset = arg;

	pset->ok = false;
	return 1;
}

/*
 * Check a disklabel.
 * If there are overlapping active partitions,
 * Ask the user if they want to edit the partition or give up.
 */
int
edit_and_check_label(struct pm_devs *p, struct partition_usage_set *pset,
    bool install)
{
	menu_ent *op;
	size_t cnt, i;
	bool may_add = pset->parts->pscheme->can_add_partition(pset->parts);
	bool may_edit_pack =
	    pset->parts->pscheme->get_disk_pack_name != NULL &&
	    pset->parts->pscheme->set_disk_pack_name != NULL;

#ifdef NO_CLONES
#define	C_M_ITEMS	0
#else
#define	C_M_ITEMS	1
#endif
	pset->menu_opts = calloc(pset->parts->num_part
	     +3+C_M_ITEMS+may_add+may_edit_pack,
	     sizeof *pset->menu_opts);
	if (pset->menu_opts == NULL)
		return 0;

	op = pset->menu_opts;
	for (i = 0; i < pset->parts->num_part; i++) {
		op->opt_action = edit_ptn;
		op++;
	}
	/* separator line between partitions and actions */
	op->opt_name = fspart_separator;
	op->opt_flags = OPT_IGNORE|OPT_NOSHORT;
	op++;

	/* followed by new partition adder */
	if (may_add) {
		op->opt_name = MSG_addpart;
		op->opt_flags = OPT_SUB;
		op->opt_action = edit_fspart_add;
		op++;
	}

	/* and unit changer */
	op->opt_name = MSG_askunits;
	op->opt_menu = MENU_sizechoice;
	op->opt_flags = OPT_SUB;
	op->opt_action = NULL;
	op++;

	if (may_edit_pack) {
		op->opt_name = MSG_editpack;
		op->opt_flags = OPT_SUB;
		op->opt_action = edit_fspart_pack;
		op++;
	}

#ifndef NO_CLONES
	/* add a clone-from-elsewhere option */
	op->opt_name = MSG_clone_from_elsewhere;
	op->opt_action = part_ext_clone;
	op++;
#endif

	/* and abort option */
	op->opt_name = MSG_cancel;
	op->opt_flags = OPT_EXIT;
	op->opt_action = edit_fspart_abort;
	op++;
	cnt = op - pset->menu_opts;
	assert(cnt == pset->parts->num_part+3+C_M_ITEMS+may_add+may_edit_pack);

	pset->menu = new_menu(fspart_title, pset->menu_opts, cnt,
			0, -1, 0, 74,
			MC_ALWAYS_SCROLL|MC_NOBOX|MC_DFLTEXIT|
			MC_NOCLEAR|MC_CONTINUOUS,
			fmt_fspart_header, fmt_fspart_row, NULL, NULL,
			MSG_partition_sizes_ok);

	if (pset->menu < 0) {
		free(pset->menu_opts);
		pset->menu_opts = NULL;
		return 0;
	}

	p->current_cylsize = p->dlcylsize;

	for (;;) {
		/* first give the user the option to edit the label... */
		pset->ok = true;
		process_menu(pset->menu, pset);
		if (!pset->ok) {
			i = 0;
			break;
		}

		/* User thinks the label is OK. */
		i = verify_parts(pset, install);
		if (i == 1)
			continue;
		break;
	}
	free(pset->menu_opts);
	pset->menu_opts = NULL;
	free_menu(pset->menu);
	pset->menu = -1;

	return i != 0;
}

/*
 * strip trailing / to avoid confusion in path comparisons later
 */
void
canonicalize_last_mounted(char *path)
{
	char *p;

	if (path == NULL)
		return;

	if (strcmp(path, "/") == 0)
		return;	/* in this case a "trailing" slash is allowed */

	for (;;) {
		p = strrchr(path, '/');
		if (p == NULL)
			return;
		if (p[1] != 0)
			return;
		p[0] = 0;
	}
}

/*
 * Try to get 'last mounted on' (or equiv) from fs superblock.
 */
const char *
get_last_mounted(int fd, daddr_t partstart, uint *fs_type, uint *fs_sub_type,
    uint flags)
{
	static char sblk[SBLOCKSIZE];		/* is this enough? */
	struct fs *SB = (struct fs *)sblk;
	static const off_t sblocks[] = SBLOCKSEARCH;
	const off_t *sbp;
	const char *mnt = NULL;
	int len;

	if (fd == -1)
		return "";

	if (fs_type)
		*fs_type = 0;
	if (fs_sub_type)
		*fs_sub_type = 0;

	/* Check UFS1/2 (and hence LFS) superblock */
	for (sbp = sblocks; mnt == NULL && *sbp != -1; sbp++) {
		if (pread(fd, sblk, sizeof sblk,
		    (off_t)partstart * (off_t)512 + *sbp) != sizeof sblk)
			continue;

		/*
		 * If start of partition and allowed by flags check
		 * for other fs types
		 */
		if (*sbp == 0 && (flags & GLM_MAYBE_FAT32) &&
		    sblk[0x42] == 0x29 && memcmp(sblk + 0x52, "FAT", 3) == 0) {
			/* Probably a FAT filesystem, report volume name */
			size_t i;
			for (i = 0x51; i >= 0x47; i--) {
				if (sblk[i] != ' ')
					break;
				sblk[i] = 0;
			}
			sblk[0x52] = 0;
			if (fs_type)
				*fs_type = FS_MSDOS;
			if (fs_sub_type)
				*fs_sub_type = sblk[0x53];
			return sblk + 0x47;
		} else if (*sbp == 0 && (flags & GLM_MAYBE_NTFS) &&
		    memcmp(sblk+3, "NTFS    ", 8) == 0) {
			if (fs_type)
				*fs_type = FS_NTFS;
			if (fs_sub_type)
				*fs_sub_type = MBR_PTYPE_NTFS;
			/* XXX dig for volume name attribute ? */
			return "";
		}

		if (!(flags & GLM_LIKELY_FFS))
			continue;

		/* Maybe we should validate the checksum??? */
		switch (SB->fs_magic) {
		case FS_UFS1_MAGIC:
		case FS_UFS1_MAGIC_SWAPPED:
			if (!(SB->fs_old_flags & FS_FLAGS_UPDATED)) {
				if (*sbp == SBLOCK_UFS1)
					mnt = (const char *)SB->fs_fsmnt;
			} else {
				/* Check we have the main superblock */
				if (SB->fs_sblockloc == *sbp)
					mnt = (const char *)SB->fs_fsmnt;
			}
			if (fs_type)
				*fs_type = FS_BSDFFS;
			if (fs_sub_type)
				*fs_sub_type = 1;
			continue;
		case FS_UFS2_MAGIC:
		case FS_UFS2_MAGIC_SWAPPED:
			/* Check we have the main superblock */
			if (SB->fs_sblockloc == *sbp) {
				mnt = (const char *)SB->fs_fsmnt;
				if (fs_type)
					*fs_type = FS_BSDFFS;
				if (fs_sub_type)
					*fs_sub_type = 2;
			}
			continue;
		}
	}

	if (mnt == NULL)
		return "";

	/* If sysinst mounted this last then strip prefix */
	len = strlen(targetroot_mnt);
	if (memcmp(mnt, targetroot_mnt, len) == 0) {
		if (mnt[len] == 0)
			return "/";
		if (mnt[len] == '/')
			return mnt + len;
	}
	return mnt;
	#undef SB
}

/* Ask for a partition offset, check bounds and do the needed roundups */
daddr_t
getpartoff(struct disk_partitions *parts, daddr_t defpartstart)
{
	char defstart[24], isize[24], maxpart, minspace, maxspace,
	    *prompt, *label_msg, valid_parts[4], valid_spaces[4],
	    space_prompt[1024], *head, *hint_part, *hint_space, *tail;
	size_t num_freespace, spaces, ndx;
	struct disk_part_free_space *freespace;
	daddr_t i, localsizemult, ptn_alignment, min, max;
	part_id partn;
	struct disk_part_info info;
	const char *errmsg = NULL;

	min = parts->disk_start;
	max = min + parts->disk_size;

	/* upper bound on the number of free spaces, plus some slope */
	num_freespace = parts->num_part * 2 + 5;
	freespace = calloc(num_freespace, sizeof(*freespace));
	if (freespace == NULL)
		return -1;

	ptn_alignment = parts->pscheme->get_part_alignment(parts);
	spaces = parts->pscheme->get_free_spaces(parts, freespace,
	    num_freespace, max(sizemult, ptn_alignment), ptn_alignment, -1,
	    defpartstart);

	maxpart = 'a' + parts->num_part -1;
	if (parts->num_part > 1) {
		snprintf(valid_parts, sizeof valid_parts, "a-%c", maxpart);
	} else if (parts->num_part == 1) {
		snprintf(valid_parts, sizeof valid_parts, "  %c", maxpart);
	} else {
		strcpy(valid_parts, "---");
	}
	if (spaces > 1) {
		minspace = maxpart + 1;
		maxspace = minspace + spaces -1;
		snprintf(valid_spaces, sizeof valid_spaces, "%c-%c", minspace,
		    maxspace);
	} else if (spaces == 1) {
		maxspace = minspace = maxpart + 1;
		snprintf(valid_spaces, sizeof valid_spaces, "  %c", minspace);
	} else {
		minspace = 0;
		maxspace = 0;
		strcpy(valid_spaces, "---");
	}

	/* Add description of start/size to user prompt */
	const char *mstr = msg_string(MSG_free_space_line);
        space_prompt[0] = 0;
        for (ndx = 0; ndx < spaces; ndx++) {
                char str_start[40], str_end[40], str_size[40], str_tag[4];

		sprintf(str_tag, "%c: ", minspace+(int)ndx);
                sprintf(str_start, "%" PRIu64, freespace[ndx].start / sizemult);
                sprintf(str_end, "%" PRIu64,
                    (freespace[ndx].start + freespace[ndx].size) / sizemult);
                sprintf(str_size, "%" PRIu64, freespace[ndx].size / sizemult);
                const char *args[4] = { str_start, str_end, str_size,
                    multname };
                char *line = str_arg_subst(mstr, 4, args);
		strlcat(space_prompt, str_tag, sizeof(space_prompt));
                size_t len = strlcat(space_prompt, line, sizeof(space_prompt));
                free (line);
                if (len >= sizeof space_prompt)
                        break;
        }

	const char *args[] = { valid_parts, valid_spaces, multname };
	hint_part = NULL;
	hint_space = NULL;
	head = str_arg_subst(msg_string(MSG_label_offset_head),
	    __arraycount(args), args);
	if (parts->num_part)
		hint_part = str_arg_subst(msg_string(
		    MSG_label_offset_part_hint), __arraycount(args), args);
	if (spaces)
		hint_space = str_arg_subst(msg_string(
		    MSG_label_offset_space_hint), __arraycount(args), args);
	tail = str_arg_subst(msg_string(MSG_label_offset_tail),
	    __arraycount(args), args);

	if (hint_part && hint_space)
		asprintf(&label_msg, "%s\n%s\n%s\n\n%s\n%s",
		    head, hint_part, hint_space, space_prompt, tail);
	else if (hint_part)
		asprintf(&label_msg, "%s\n%s\n\n%s",
		    head, hint_part, tail);
	else if (hint_space)
		asprintf(&label_msg, "%s\n%s\n\n%s\n%s",
		    head, hint_space, space_prompt, tail);
	else
		asprintf(&label_msg, "%s\n\n%s",
		    head, tail);
	free(head); free(hint_part); free(hint_space); free(tail);

	localsizemult = sizemult;
	errmsg = NULL;
	for (;;) {
		snprintf(defstart, sizeof defstart, "%" PRIu64,
		    defpartstart/sizemult);
		if (errmsg != NULL && errmsg[0] != 0)
			asprintf(&prompt, "%s\n\n%s", errmsg, label_msg);
		else
			prompt = label_msg;
		msg_prompt_win(prompt, -1, 13, 70, -1,
		    (defpartstart > 0) ? defstart : NULL, isize, sizeof isize);
		if (label_msg != prompt)
			free(prompt);
		if (strcmp(defstart, isize) == 0) {
			/* Don't do rounding if default accepted */
			i = defpartstart;
			break;
		}
		if (isize[1] == '\0' && isize[0] >= 'a' &&
		    isize[0] <= maxpart) {
			partn = isize[0] - 'a';
			if (parts->pscheme->get_part_info(parts, partn,
			    &info)) {
				i = info.start + info.size;
				localsizemult = 1;
			} else {
				errmsg = msg_string(MSG_invalid_sector_number);
				continue;
			}
		} else if (isize[1] == '\0' && isize[0] >= minspace &&
		    isize[0] <= maxspace) {
			ndx = isize[0] - minspace;
			i = freespace[ndx].start;
			localsizemult = 1;
		} else if (atoi(isize) == -1) {
			i = min;
			localsizemult = 1;
		} else {
			i = parse_disk_pos(isize, &localsizemult,
			    parts->bytes_per_sector,
			    parts->pscheme->get_cylinder_size(parts), NULL);
			if (i < 0) {
				errmsg = msg_string(MSG_invalid_sector_number);
				continue;
			}
		}
		/* round to cylinder size if localsizemult != 1 */
		int cylsize = parts->pscheme->get_cylinder_size(parts);
		i = NUMSEC(i, localsizemult, cylsize);
		/* Adjust to start of slice if needed */
		if ((i < min && (min - i) < localsizemult) ||
		    (i > min && (i - min) < localsizemult)) {
			i = min;
		}
		if (max == 0 || i <= max)
			break;
		errmsg = msg_string(MSG_startoutsidedisk);
	}
	free(label_msg);
	free(freespace);

	return i;
}


/* Ask for a partition size, check bounds and do the needed roundups */
daddr_t
getpartsize(struct disk_partitions *parts, daddr_t orig_start,
    daddr_t partstart, daddr_t dflt)
{
	char dsize[24], isize[24], max_size[24], maxpartc, valid_parts[4],
	    *label_msg, *prompt, *head, *hint, *tail;
	const char *errmsg = NULL;
	daddr_t i, partend, diskend, localsizemult, max, max_r, dflt_r;
	struct disk_part_info info;
	part_id partn;

	diskend = parts->disk_start + parts->disk_size;
	max = parts->pscheme->max_free_space_at(parts, orig_start);
	max += orig_start - partstart;
	if (sizemult == 1)
		max--;	/* with hugher scale proper rounding later will be ok */

	/* We need to keep both the unrounded and rounded (_r) max and dflt */
	dflt_r = (partstart + dflt) / sizemult - partstart / sizemult;
	if (max == dflt)
		max_r = dflt_r;
	else
		max_r = max / sizemult;
	/* the partition may have been moved and now not fit any longer */
	if (dflt > max)
		dflt = max;
	if (dflt_r > max_r)
		dflt_r = max_r;

	snprintf(max_size, sizeof max_size, "%" PRIu64, max_r);

	maxpartc = 'a' + parts->num_part -1;
	if (parts->num_part > 1) {
		snprintf(valid_parts, sizeof valid_parts, "a-%c", maxpartc);
	} else if (parts->num_part == 1) {
		snprintf(valid_parts, sizeof valid_parts, "  %c", maxpartc);
	} else {
		strcpy(valid_parts, "---");
	}

	const char *args[] = { valid_parts, max_size, multname };
	hint = NULL;
	head = str_arg_subst(msg_string(MSG_label_size_head),
	    __arraycount(args), args);
	if (parts->num_part)
		hint  = str_arg_subst(msg_string(MSG_label_size_part_hint),
		    __arraycount(args), args);
	tail = str_arg_subst(msg_string(MSG_label_size_tail),
	    __arraycount(args), args);

	if (hint != NULL)
		asprintf(&label_msg, "%s\n%s\n\n%s", head, hint, tail);
	else
		asprintf(&label_msg, "%s\n\n%s", head, tail);
	free(head); free(hint); free(tail);

	localsizemult = sizemult;
	i = -1;
	for (;;) {
		snprintf(dsize, sizeof dsize, "%" PRIu64, dflt_r);

		if (errmsg != NULL && errmsg[0] != 0)
			asprintf(&prompt, "%s\n\n%s", errmsg, label_msg);
		else
			prompt = label_msg;
		msg_prompt_win(prompt, -1, 12, 70, -1,
		    (dflt != 0) ? dsize : 0, isize, sizeof isize);
		if (prompt != label_msg)
			free(prompt);

		if (strcmp(isize, dsize) == 0) {
			free(label_msg);
			return dflt;
		}
		if (parts->num_part && isize[1] == '\0' && isize[0] >= 'a' &&
		    isize[0] <= maxpartc) {
			partn = isize[0] - 'a';
			if (parts->pscheme->get_part_info(parts, partn,
			    &info)) {
				i = info.start - partstart -1;
				localsizemult = 1;
				max_r = max;
			}
		} else if (atoi(isize) == -1) {
			i = max;
			localsizemult = 1;
			max_r = max;
		} else {
			i = parse_disk_pos(isize, &localsizemult,
			    parts->bytes_per_sector,
			    parts->pscheme->get_cylinder_size(parts), NULL);
			if (localsizemult != sizemult)
				max_r = max;
		}
		if (i < 0) {
			errmsg = msg_string(MSG_Invalid_numeric);
			continue;
		} else if (i > max_r) {
			errmsg = msg_string(MSG_Too_large);
			continue;
		}
		/*
		 * partend is aligned to a cylinder if localsizemult
		 * is not 1 sector
		 */
		int cylsize = parts->pscheme->get_cylinder_size(parts);
		partend = NUMSEC((partstart + i*localsizemult) / localsizemult,
		    localsizemult, cylsize);
		/* Align to end-of-disk or end-of-slice if close enough */
		if (partend > (diskend - sizemult)
		    && partend < (diskend + sizemult))
			partend = diskend;
		if (partend > (partstart + max - sizemult)
		    && partend < (partstart + max + sizemult))
			partend = partstart + max;
		/* sanity checks */
		if (partend > diskend) {
			partend = diskend;
			errmsg = msg_string(MSG_endoutsidedisk);
			continue;
		}
		free(label_msg);
		if (partend < partstart)
			return 0;
		return (partend - partstart);
	}
	/* NOTREACHED */
}

/*
 * convert a string to a number of sectors, with a possible unit
 * 150M = 150 Megabytes
 * 2000c = 2000 cylinders
 * 150256s = 150256 sectors
 * Without units, use the default (sizemult).
 * returns the raw input value, and the unit used. Caller needs to multiply!
 * On invalid inputs, returns -1.
 */
daddr_t
parse_disk_pos(
	const char *str,
	daddr_t *localsizemult,
	daddr_t bps,
	daddr_t cyl_size,
	bool *extend_this)
{
	daddr_t val;
	char *cp;
	bool mult_found;

	if (str[0] == '\0') {
		return -1;
	}
	val = strtoull(str, &cp, 10);
	mult_found = false;
	if (extend_this)
		*extend_this = false;
	while (*cp != 0) {
		if (*cp == 'G' || *cp == 'g') {
			if (mult_found)
				return -1;
			*localsizemult = GIG / bps;
			goto next;
		}
		if (*cp == 'M' || *cp == 'm') {
			if (mult_found)
				return -1;
			*localsizemult = MEG / bps;
			goto next;
		}
		if (*cp == 'c' || *cp == 'C') {
			if (mult_found)
				return -1;
			*localsizemult = cyl_size;
			goto next;
		}
		if (*cp == 's' || *cp == 'S') {
			if (mult_found)
				return -1;
			*localsizemult = 1;
			goto next;
		}
		if (*cp == '+' && extend_this) {
			*extend_this = true;
			cp++;
			break;
		}

		/* not a known unit */
		return -1;

next:
		mult_found = true;
		cp++;
		continue;
	}
	if (*cp != 0)
		return -1;

	return val;
}
