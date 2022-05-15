/*	$NetBSD: part_edit.c,v 1.27 2022/05/15 15:06:59 jmcneill Exp $ */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


/* part_edit.c -- generic partition editing code */

#include <sys/param.h>
#include <sys/types.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <util.h>
#include "defs.h"
#include "md.h"
#include "msg_defs.h"
#include "menu_defs.h"
#include "defsizes.h"
#include "endian.h"


/*
 * A structure passed to various menu functions for partition editing
 */
struct part_edit_info {
	struct disk_partitions *parts;	/* the partitions we edit */
	struct disk_part_info cur;	/* current value (maybe incomplete) */
	part_id cur_id;			/* which partition is it? */
	int first_custom_opt;		/* scheme specific menu options
					 * start here */
	bool cancelled;			/* do not apply changes */
	bool num_changed;		/* number of partitions has changed */
};

#ifndef NO_CLONES
struct single_clone_data {
	struct selected_partitions clone_src;
	part_id *clone_ids;	/* partition IDs in target */
};
#endif
struct outer_parts_data {
	struct arg_rv av;
#ifndef NO_CLONES
	struct single_clone_data *clones;
	size_t num_clone_entries;
#endif
};

static menu_ent *part_menu_opts;		/* the currently edited partitions */
static menu_ent *outer_fill_part_menu_opts(const struct disk_partitions *parts, size_t *cnt);
static void draw_outer_part_line(menudesc *m, int opt, void *arg);

static char 	outer_part_sep_line[MENUSTRSIZE],
		outer_part_title[2*MENUSTRSIZE];

static int
maxline(const char *p, int *count)
{
	int m = 0, i = 0;

	for (;; p++) {
		if (*p == '\n' || *p == 0) {
			if (i > m)
				m = i;
			(*count)++;
			if (*p == 0)
				return m;
			i = 0;
		} else {
			i++;
		}
	}
}

int
err_msg_win(const char *errmsg)
{
	const char *cont;
	int l, l1, lines;

	errmsg = msg_string(errmsg);
	cont = msg_string(MSG_Hit_enter_to_continue);

	lines = 0;
	l = maxline(errmsg, &lines);
	l1 = maxline(cont, &lines);
	if (l < l1)
		l = l1;

	msg_fmt_prompt_win("%s.\n%s", -1, 18, l + 5, 2+lines,
			NULL, NULL, 1, "%s%s", errmsg, cont);
	return 0;
}

static int
set_part_type(menudesc *m, void *arg)
{
	struct part_edit_info *info = arg;
	const struct part_type_desc *desc;
	char buf[STRSIZE];
	const char *err;

	if (m->cursel == 0)
		return 1;	/* no change */

	desc = info->parts->pscheme->get_part_type(m->cursel-1);
	if (desc == NULL) {
		/* Create custom type */
		if (info->cur.nat_type != NULL)
			strlcpy(buf, info->cur.nat_type->short_desc,
			    sizeof(buf));
		else
			buf[0] = 0;
		for (;;) {
			msg_prompt_win(info->parts->pscheme->new_type_prompt,
			     -1, 18, 0, 0,
			    buf, buf, sizeof(buf));
			if (buf[0] == 0)
				break;
			desc = info->parts->pscheme->create_custom_part_type(
			    buf, &err);
			if (desc != NULL)
				break;
			err_msg_win(err);
		}
	}

	info->cur.nat_type = desc;
	return 1;
}

static void
set_type_label(menudesc *m, int opt, void *arg)
{
	struct part_edit_info *info = arg;
	const struct part_type_desc *desc;

	if (opt == 0) {
		wprintw(m->mw, "%s", msg_string(MSG_Dont_change));
		return;
	}

	desc = info->parts->pscheme->get_part_type(opt-1);
	if (desc == NULL) {
		wprintw(m->mw, "%s", msg_string(MSG_Other_kind));
		return;
	}
	wprintw(m->mw, "%s", desc->description);
}

static int
edit_part_type(menudesc *m, void *arg)
{
	struct part_edit_info *info = arg;
	menu_ent *type_opts;
	int type_menu = -1;
	size_t popt_cnt, i;

	/*
	 * We add one line at the start of the menu, and one at the
	 * bottom, see "set_type_label" above.
	 */
	popt_cnt =  info->parts->pscheme->get_part_types_count() + 2;
	type_opts = calloc(popt_cnt, sizeof(*type_opts));
	for (i = 0; i < popt_cnt; i++) {
		type_opts[i].opt_action = set_part_type;
	}
	type_menu = new_menu(NULL, type_opts, popt_cnt,
		13, 12, 0, 30,
		MC_SUBMENU | MC_SCROLL | MC_NOEXITOPT | MC_NOCLEAR,
		NULL, set_type_label, NULL,
		NULL, NULL);

	if (type_menu != -1) {
		process_menu(type_menu, arg);
		info->num_changed = true;	/* force reload of menu */
	}

	free_menu(type_menu);
	free(type_opts);

	return -1;
}

static int
edit_part_start(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;
	struct disk_part_info pinfo;
	daddr_t max_size;

	if (marg->cur_id == NO_PART ||
	    !marg->parts->pscheme->get_part_info(marg->parts, marg->cur_id,
	    &pinfo))
		pinfo = marg->cur;
	marg->cur.start = getpartoff(marg->parts, marg->cur.start);
	max_size = marg->parts->pscheme->max_free_space_at(marg->parts,
	    pinfo.start);
	max_size += pinfo.start - marg->cur.start;
	if (marg->cur.size > max_size)
		marg->cur.size = max_size;

	return 0;
}

static int
edit_part_size(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;
	struct disk_part_info pinfo;

	if (marg->cur_id == NO_PART ||
	    !marg->parts->pscheme->get_part_info(marg->parts, marg->cur_id,
	    &pinfo))
		pinfo = marg->cur;
	marg->cur.size = getpartsize(marg->parts, pinfo.start,
	    marg->cur.start, marg->cur.size);

	return 0;
}

static int
edit_part_install(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;

	marg->cur.flags ^= PTI_INSTALL_TARGET;
	return 0;
}

static void
menu_opts_reload(menudesc *m, const struct disk_partitions *parts)
{
	size_t new_num;

	free(part_menu_opts);
	part_menu_opts = outer_fill_part_menu_opts(parts, &new_num);
	m->opts = part_menu_opts;
	m->numopts = new_num;
}

static int
delete_part(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;
	const char *err_msg = NULL;

	if (marg->cur_id == NO_PART)
		return 0;

	if (!marg->parts->pscheme->delete_partition(marg->parts, marg->cur_id,
	    &err_msg))
		err_msg_win(err_msg);

	marg->num_changed = true;	/* reload list of partitions */
	marg->cancelled = true;		/* do not write back cur data */

	return 0;
}

static void draw_outer_ptn_line(menudesc *m, int line, void *arg);
static void draw_outer_ptn_header(menudesc *m, void *arg);

static int
part_rollback(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;

	marg->cancelled = true;
	return 0;
}

static menu_ent common_ptn_edit_opts[] = {
#define PTN_OPT_TYPE		0
	{ .opt_action=edit_part_type },
#define PTN_OPT_START		1
	{ .opt_action=edit_part_start },
#define PTN_OPT_SIZE		2
	{ .opt_action=edit_part_size },
#define PTN_OPT_END		3
	{ .opt_flags=OPT_IGNORE }, /* read only "end" */

	/*
	 * Only the part upto here will be used when adding a new partition
	 */

#define PTN_OPT_INSTALL		4
	{ .opt_action=edit_part_install },

#define	PTN_OPTS_COMMON		PTN_OPT_INSTALL	/* cut off from here for add */
};

static int
edit_custom_opt(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;
	size_t attr_no = m->cursel - marg->first_custom_opt;
	char line[STRSIZE];

	switch (marg->parts->pscheme->custom_attributes[attr_no].type) {
	case pet_bool:
		marg->parts->pscheme->custom_attribute_toggle(
		    marg->parts, marg->cur_id, attr_no);
		break;
	case pet_cardinal:
	case pet_str:
		marg->parts->pscheme->format_custom_attribute(
		    marg->parts, marg->cur_id, attr_no, &marg->cur,
		    line, sizeof(line));
		msg_prompt_win(
		    marg->parts->pscheme->custom_attributes[attr_no].label,
		    -1, 18, 0, 0, line, line, sizeof(line));
		marg->parts->pscheme->custom_attribute_set_str(
		    marg->parts, marg->cur_id, attr_no, line);
		break;
	}

	return 0;
}

static menu_ent ptn_edit_opts[] = {
	{ .opt_name=MSG_askunits, .opt_menu=MENU_sizechoice,
	  .opt_flags=OPT_SUB },

	{ .opt_name=MSG_Delete_partition,
	  .opt_action = delete_part, .opt_flags = OPT_EXIT },

	{ .opt_name=MSG_cancel,
	  .opt_action = part_rollback, .opt_flags = OPT_EXIT },
};

static menu_ent ptn_add_opts[] = {
	{ .opt_name=MSG_askunits, .opt_menu=MENU_sizechoice,
	  .opt_flags=OPT_SUB },

	{ .opt_name=MSG_cancel,
	  .opt_action = part_rollback, .opt_flags = OPT_EXIT },
};

/*
 * Concatenate common_ptn_edit_opts, the partitioning scheme specific
 * custom options and the given suffix to a single menu options array.
 */
static menu_ent *
fill_part_edit_menu_opts(struct disk_partitions *parts,
    bool with_custom_attrs,
    const menu_ent *suffix, size_t suffix_count, size_t *res_cnt)
{
	size_t i;
	menu_ent *opts, *p;
	size_t count, hdr_cnt;

	if (with_custom_attrs) {
		hdr_cnt = __arraycount(common_ptn_edit_opts);
		count = hdr_cnt + parts->pscheme->custom_attribute_count
		    + suffix_count;
	} else {
		hdr_cnt = PTN_OPTS_COMMON;
		count = hdr_cnt + suffix_count;
	}

	opts = calloc(count, sizeof(*opts));
	if (opts == NULL) {
		*res_cnt = 0;
		return NULL;
	}

	memcpy(opts, common_ptn_edit_opts,
	    sizeof(*opts)*hdr_cnt);
	p = opts + hdr_cnt;
	if (with_custom_attrs) {
		for (i = 0; i < parts->pscheme->custom_attribute_count; i++) {
			p->opt_action = edit_custom_opt;
			p++;
		}
	}
	memcpy(p, suffix, sizeof(*opts)*suffix_count);

	*res_cnt = count;
	return opts;
}

static int
edit_part_entry(menudesc *m, void *arg)
{
	struct outer_parts_data *pdata = arg;
	struct part_edit_info data = { .parts = pdata->av.arg,
	    .cur_id = m->cursel,
	    .first_custom_opt = __arraycount(common_ptn_edit_opts) };
	int ptn_menu;
	const char *err;
	menu_ent *opts;
	size_t num_opts;

	opts = fill_part_edit_menu_opts(data.parts, true, ptn_edit_opts,
	    __arraycount(ptn_edit_opts), &num_opts);
	if (opts == NULL)
		return 1;

	if (data.cur_id < data.parts->num_part)
		data.parts->pscheme->get_part_info(data.parts, data.cur_id,
		    &data.cur);

	ptn_menu = new_menu(NULL, opts, num_opts,
		15, 2, 0, 54,
		MC_SUBMENU | MC_SCROLL | MC_NOCLEAR,
		draw_outer_ptn_header, draw_outer_ptn_line, NULL,
		NULL, MSG_Partition_OK);
	if (ptn_menu == -1) {
		free(opts);
		return 1;
	}

	process_menu(ptn_menu, &data);
	free_menu(ptn_menu);
	free(opts);

	if (!data.cancelled && data.cur_id < data.parts->num_part)
		if (!data.parts->pscheme->set_part_info(data.parts,
		    data.cur_id, &data.cur, &err))
			err_msg_win(err);

	if (data.num_changed) {
		menu_opts_reload(m, data.parts);
		m->cursel = data.parts->num_part > 0 ? 0 : 2;
		return -1;
	}

	return 0;
}

#ifndef NO_CLONES
static int
add_part_clone(menudesc *menu, void *arg)
{
	struct outer_parts_data *pdata = arg;
	struct disk_partitions *parts = pdata->av.arg;
	struct clone_target_menu_data data;
	menu_ent *men;
	int num_men, i;
	struct disk_part_info sinfo, cinfo;
	struct disk_partitions *csrc;
	struct disk_part_free_space space;
	daddr_t offset, align;
	size_t s;
	part_id cid;
	struct selected_partitions selected;
	struct single_clone_data *new_clones;

	if (!select_partitions(&selected, parts))
		return 0;

	new_clones = realloc(pdata->clones,
	    sizeof(*pdata->clones)*(pdata->num_clone_entries+1));
	if (new_clones == NULL)
		return 0;
	pdata->num_clone_entries++;
	pdata->clones = new_clones;
	new_clones += (pdata->num_clone_entries-1);
	memset(new_clones, 0, sizeof *new_clones);
	new_clones->clone_src = selected;

	memset(&data, 0, sizeof data);
	data.usage.parts = parts;

	/* if we already have partitions, ask for the target position */
	if (parts->num_part > 0) {
		data.res = -1;
		num_men = parts->num_part+1;
		men = calloc(num_men, sizeof *men);
		if (men == NULL)
			return 0;
		for (i = 0; i < num_men; i++)
			men[i].opt_action = clone_target_select;
		men[num_men-1].opt_name = MSG_clone_target_end;

		data.usage.menu = new_menu(MSG_clone_target_hdr,
		    men, num_men, 3, 2, 0, 65, MC_SCROLL,
		    NULL, draw_outer_part_line, NULL, NULL, MSG_cancel);
		process_menu(data.usage.menu, &data);
		free_menu(data.usage.menu);
		free(men);

		if (data.res < 0)
			goto err;
	} else {
		data.res = 0;
	}

	/* find selected offset from data.res and insert clones there */
	align = parts->pscheme->get_part_alignment(parts);
	offset = -1;
	if (data.res > 0) {
		for (cid = 0; cid < (size_t)data.res; cid++) {
			if (!parts->pscheme->get_part_info(parts, cid, &sinfo))
			continue;
			offset = sinfo.start + sinfo.size;
		}
	} else {
		offset = 0;
	}

	new_clones->clone_ids = calloc(selected.num_sel,
	    sizeof(*new_clones->clone_ids));
	if (new_clones->clone_ids == NULL)
		goto err;
	for (s = 0; s < selected.num_sel; s++) {
		csrc = selected.selection[s].parts;
		cid = selected.selection[s].id;
		csrc->pscheme->get_part_info(csrc, cid, &sinfo);
		if (!parts->pscheme->adapt_foreign_part_info(
		    parts, &cinfo, csrc->pscheme, &sinfo))
			continue;
		size_t cnt = parts->pscheme->get_free_spaces(
		    parts, &space, 1, cinfo.size-align, align,
		    offset, -1);
		if (cnt == 0)
			continue;
		cinfo.start = space.start;
		cid = parts->pscheme->add_partition(
		    parts, &cinfo, NULL);
		new_clones->clone_ids[s] = cid;
		if (cid == NO_PART)
			continue;
		parts->pscheme->get_part_info(parts, cid, &cinfo);
		offset = roundup(cinfo.start+cinfo.size, align);
	}

	/* reload menu and start again */
	menu_opts_reload(menu, parts);
	menu->cursel = parts->num_part+1;
	if (parts->num_part == 0)
		menu->cursel++;
	return -1;

err:
	free_selected_partitions(&selected);
	return -1;
}
#endif

static int
add_part_entry(menudesc *m, void *arg)
{
	struct outer_parts_data *pdata = arg;
	struct part_edit_info data = { .parts = pdata->av.arg,
	    .first_custom_opt = PTN_OPTS_COMMON };
	int ptn_menu;
	daddr_t ptn_alignment, start;
	menu_ent *opts;
	size_t num_opts;
	struct disk_part_free_space space;
	const char *err;

	opts = fill_part_edit_menu_opts(data.parts, false, ptn_add_opts,
	    __arraycount(ptn_add_opts), &num_opts);
	if (opts == NULL)
		return 1;

	ptn_alignment = data.parts->pscheme->get_part_alignment(data.parts);
	start = pm->ptstart > 0 ? pm->ptstart : -1;
	data.cur_id = NO_PART;
	memset(&data.cur, 0, sizeof(data.cur));
	data.cur.nat_type = data.parts->pscheme->
	    get_generic_part_type(PT_root);
	if (data.parts->pscheme->get_free_spaces(data.parts, &space, 1,
	    max(sizemult, ptn_alignment), ptn_alignment, start, -1) > 0) {
		data.cur.start = space.start;
		data.cur.size = space.size;
	} else {
		return 0;
	}

	ptn_menu = new_menu(NULL, opts, num_opts,
		15, -1, 0, 54,
		MC_SUBMENU | MC_SCROLL | MC_NOCLEAR,
		draw_outer_ptn_header, draw_outer_ptn_line, NULL,
		NULL, MSG_Partition_OK);
	if (ptn_menu == -1) {
		free(opts);
		return 1;
	}

	process_menu(ptn_menu, &data);
	free_menu(ptn_menu);
	free(opts);

	if (!data.cancelled &&
	    data.parts->pscheme->add_partition(data.parts, &data.cur, &err)
	    == NO_PART)
		err_msg_win(err);

	menu_opts_reload(m, data.parts);
	m->cursel = data.parts->num_part+1;
	if (data.parts->num_part == 0)
		m->cursel++;
	return -1;
}

static void
draw_outer_ptn_line(menudesc *m, int line, void *arg)
{
	struct part_edit_info *marg = arg;
	char value[STRSIZE];
	static int col_width;
	static const char *yes, *no, *ptn_type, *ptn_start, *ptn_size,
	    *ptn_end, *ptn_install;

	if (yes == NULL) {
		int i;

#define CHECK(str)	i = strlen(str); if (i > col_width) col_width = i;

		col_width = 0;
		yes = msg_string(MSG_Yes); CHECK(yes);
		no = msg_string(MSG_No); CHECK(no);
		ptn_type = msg_string(MSG_ptn_type); CHECK(ptn_type);
		ptn_start = msg_string(MSG_ptn_start); CHECK(ptn_start);
		ptn_size = msg_string(MSG_ptn_size); CHECK(ptn_size);
		ptn_end = msg_string(MSG_ptn_end); CHECK(ptn_end);
		ptn_install = msg_string(MSG_ptn_install); CHECK(ptn_install);

#undef CHECK

		for (size_t n = 0;
		    n < marg->parts->pscheme->custom_attribute_count; n++) {
			i = strlen(msg_string(
			    marg->parts->pscheme->custom_attributes[n].label));
			if (i > col_width)
				col_width = i;
		}
		col_width += 3;
	}

	if (line >= marg->first_custom_opt) {
		size_t attr_no = line-marg->first_custom_opt;
		marg->parts->pscheme->format_custom_attribute(
		    marg->parts, marg->cur_id, attr_no, &marg->cur,
		    value, sizeof(value));
		wprintw(m->mw, "%*s : %s", col_width,
		    msg_string(
		    marg->parts->pscheme->custom_attributes[attr_no].label),
		    value);
		return;
	}

	switch (line) {
	case PTN_OPT_TYPE:
		wprintw(m->mw, "%*s : %s", col_width, ptn_type,
		    marg->cur.nat_type != NULL
		    ? marg->cur.nat_type->description
		    : "-");
		break;
	case PTN_OPT_START:
		wprintw(m->mw, "%*s : %" PRIu64 " %s", col_width, ptn_start,
		    marg->cur.start / (daddr_t)sizemult, multname);
		break;
	case PTN_OPT_SIZE:
		wprintw(m->mw, "%*s : %" PRIu64 " %s", col_width, ptn_size,
		    marg->cur.size / (daddr_t)sizemult, multname);
		break;
	case PTN_OPT_END:
		wprintw(m->mw, "%*s : %" PRIu64 " %s", col_width, ptn_end,
		    (marg->cur.start + marg->cur.size - 1) / (daddr_t)sizemult,
		    multname);
		break;
	case PTN_OPT_INSTALL:
		wprintw(m->mw, "%*s : %s", col_width, ptn_install,
		    (marg->cur.nat_type->generic_ptype == PT_root &&
		    (marg->cur.flags & PTI_INSTALL_TARGET)) ? yes : no);
		break;
	}

}

static void
draw_outer_ptn_header(menudesc *m, void *arg)
{
	struct part_edit_info *marg = arg;
	size_t attr_no;
	bool may_change_type;

#define DISABLE(opt,cond) \
	if (cond) \
		m->opts[opt].opt_flags |= OPT_IGNORE; \
	else \
		m->opts[opt].opt_flags &= ~OPT_IGNORE;

	/* e.g. MBR extended partitions can only change if empty */
	may_change_type = marg->cur_id == NO_PART
	     || marg->parts->pscheme->part_type_can_change == NULL
	     || marg->parts->pscheme->part_type_can_change(
		    marg->parts, marg->cur_id);

	DISABLE(PTN_OPT_TYPE, !may_change_type);
	if (!may_change_type && m->cursel == PTN_OPT_TYPE)
		m->cursel++;
	if (marg->cur_id != NO_PART) {
		for (int i = 0; i < m->numopts; i++) {
			if (m->opts[i].opt_action == delete_part) {
				DISABLE(i, !may_change_type);
			}
		}
	}

	/* Can only install into NetBSD partition */
	if (marg->cur_id != NO_PART) {
		DISABLE(PTN_OPT_INSTALL, marg->cur.nat_type == NULL
		    || marg->cur.nat_type->generic_ptype != PT_root);
	}

	if (marg->cur_id == NO_PART)
		return;

	for (attr_no = 0; attr_no <
	    marg->parts->pscheme->custom_attribute_count; attr_no++) {
		bool writable =
		    marg->parts->pscheme->custom_attribute_writable(
			marg->parts, marg->cur_id, attr_no);
		DISABLE(attr_no+marg->first_custom_opt, !writable);
	}
}

static void
draw_outer_part_line(menudesc *m, int opt, void *arg)
{
	struct outer_parts_data *pdata = arg;
	struct disk_partitions *parts = pdata->av.arg;
	int len;
	part_id pno = opt;
	struct disk_part_info info;
	char buf[SSTRSIZE], *astr, colval[STRSIZE], line[STRSIZE];
	size_t astr_avail, x;
	static char install_flag = 0;

#define PART_ROW_USED_FMT	"%13" PRIu64 " %13" PRIu64 " %-4s"

	len = snprintf(0, 0, PART_ROW_USED_FMT, (daddr_t)0, (daddr_t)0, "");

	if (pno >= parts->num_part ||
	    !parts->pscheme->get_part_info(parts, pno, &info)) {
		wprintw(m->mw, "%*s", len, "");
		// XXX
		return;
	}

	if ((info.flags & PTI_INSTALL_TARGET) &&
	    info.nat_type->generic_ptype == PT_root) {
		if (install_flag == 0)
			install_flag = msg_string(MSG_install_flag)[0];
		astr_avail = sizeof(buf)-1;
		buf[0] = install_flag;
		buf[1] = 0;
		astr = buf+1;
	} else {
		buf[0] = 0;
		astr = buf;
		astr_avail = sizeof(buf);
	}
	if (parts->pscheme->get_part_attr_str != NULL)
		parts->pscheme->get_part_attr_str(parts, pno, astr,
		    astr_avail);

	daddr_t start = info.start / sizemult;
	daddr_t size = info.size / sizemult;
	wprintw(m->mw, PART_ROW_USED_FMT,
	    start, size, buf);

	line[0] = 0; x = 0;
	for (size_t col = 0; col < parts->pscheme->edit_columns_count; col++) {
		if (parts->pscheme->format_partition_table_str(parts, pno,
		    col, colval, sizeof(colval)) && colval[0] != 0
		    && x < sizeof(line)-2) {
			for (size_t i = strlen(line); i < x; i++)
				line[i] = ' ';
			line[x] = ' ';
			strlcpy(line+x+1, colval, sizeof(line)-x-1);
		}
		x += parts->pscheme->edit_columns[col].width + 1;
	}
	wprintw(m->mw, "%s", line);
}

static int
part_edit_abort(menudesc *m, void *arg)
{
	struct outer_parts_data *pdata = arg;

	pdata->av.rv = -1;
	return 0;
}

static menu_ent *
outer_fill_part_menu_opts(const struct disk_partitions *parts, size_t *cnt)
{
	menu_ent *opts, *op;
	size_t num_opts;
	size_t i;
	bool may_add;

	may_add = parts->pscheme->can_add_partition(parts);
	num_opts = 3 + parts->num_part;
#ifndef NO_CLONES
	num_opts++;
#endif
	if (parts->num_part == 0)
		num_opts++;
	if (may_add)
		num_opts++;
	opts = calloc(num_opts, sizeof *opts);
	if (opts == NULL) {
		*cnt = 0;
		return NULL;
	}

	/* add all existing partitions */
	for (op = opts, i = 0; i < parts->num_part && i < (num_opts-2);
	    op++, i++) {
		op->opt_flags = OPT_SUB;
		op->opt_action = edit_part_entry;
	}

	/* if empty, hint that partitions are missing */
	if (parts->num_part == 0) {
		op->opt_name = MSG_nopart;
		op->opt_flags = OPT_IGNORE|OPT_NOSHORT;
		op++;
	}

	/* separator line between partitions and actions */
	op->opt_name = outer_part_sep_line;
	op->opt_flags = OPT_IGNORE|OPT_NOSHORT;
	op++;

	/* followed by new partition adder */
	if (may_add) {
		op->opt_name = MSG_addpart;
		op->opt_flags = OPT_SUB;
		op->opt_action = add_part_entry;
		op++;
	}

#ifndef NO_CLONES
	/* and a partition cloner */
	op->opt_name = MSG_clone_from_elsewhere;
	op->opt_action = add_part_clone;
	op++;
#endif

	/* and unit changer */
	op->opt_name = MSG_askunits;
	op->opt_menu = MENU_sizechoice;
	op->opt_flags = OPT_SUB;
	op->opt_action = NULL;
	op++;

	/* and abort option */
	op->opt_name = MSG_cancel;
	op->opt_flags = OPT_EXIT;
	op->opt_action = part_edit_abort;
	op++;

	/* counts are consistent? */
	assert((op - opts) >= 0 && (size_t)(op - opts) == num_opts);

	*cnt = num_opts;
	return opts;
}

static void
draw_outer_part_header(menudesc *m, void *arg)
{
	struct outer_parts_data *pdata = arg;
	struct disk_partitions *parts = pdata->av.arg;
	char start[SSTRSIZE], size[SSTRSIZE], col[SSTRSIZE],
	    *disk_info, total[SSTRSIZE], avail[SSTRSIZE];
	size_t sep;
	const char *args[3];

	msg_display_subst(MSG_editparttable, 4,
	    parts->disk,
	    msg_string(parts->pscheme->name),
	    msg_string(parts->pscheme->short_name),
	    parts->pscheme->part_flag_desc ?
		msg_string(parts->pscheme->part_flag_desc)
		: "");

	snprintf(total, sizeof(total), "%" PRIu64 " %s",
	    parts->disk_size / sizemult, multname);
	snprintf(avail, sizeof(total), "%" PRIu64 " %s",
	    parts->free_space / sizemult, multname);
	args[0] = parts->disk;
	args[1] = total;
	args[2] = avail;
	disk_info = str_arg_subst(msg_string(MSG_part_header), 3, args);
	msg_table_add(disk_info);
	free(disk_info);


	strcpy(outer_part_sep_line, "------------- ------------- ----");
	sep = strlen(outer_part_sep_line);
	snprintf(start, sizeof(start), "%s(%s)",
	    msg_string(MSG_part_header_col_start), multname);
	snprintf(size, sizeof(size), "%s(%s)",
	    msg_string(MSG_part_header_col_size), multname);
	snprintf(outer_part_title, sizeof(outer_part_title),
	    "   %13s %13s %-4s", start, size,
	    msg_string(MSG_part_header_col_flag));

	for (size_t i = 0; i < parts->pscheme->edit_columns_count; i++) {
		char *np = outer_part_sep_line+sep;
		unsigned int w = parts->pscheme->edit_columns[i].width;
		snprintf(col, sizeof(col), " %*s", -w,
		    msg_string(parts->pscheme->edit_columns[i].title));
		strlcat(outer_part_title, col, sizeof(outer_part_title));
		if (sep < sizeof(outer_part_sep_line)-1) {
			*np++ = ' ';
			sep++;
		}
		for (unsigned int p = 0; p < w &&
		    sep < sizeof(outer_part_sep_line)-1; p++)
			*np++ = '-', sep++;
		*np = 0;
	}

	strlcat(outer_part_title, "\n   ", sizeof(outer_part_title));
	strlcat(outer_part_title, outer_part_sep_line,
	    sizeof(outer_part_title));

	msg_table_add("\n\n");
}

/*
 * Use the whole disk for NetBSD, but (if any) create required helper
 * partitions (usually for booting and stuff).
 */
bool
parts_use_wholedisk(struct disk_partitions *parts,
    size_t add_ext_parts, const struct disk_part_info *ext_parts)
{
	part_id nbsd;
	struct disk_part_info info;
	struct disk_part_free_space space;
	daddr_t align, start;
	size_t i;

	parts->pscheme->delete_all_partitions(parts);
	align = parts->pscheme->get_part_alignment(parts);
	start = pm->ptstart > 0 ? pm->ptstart : -1;

	if (ext_parts != NULL) {
		for (i = 0; i < add_ext_parts; i++) {
			info = ext_parts[i];
			if (parts->pscheme->get_free_spaces(parts, &space,
			    1, info.size, align, start, -1) != 1)
				return false;
			info.start = space.start;
			if (info.nat_type == NULL)
				info.nat_type = parts->pscheme->
				    get_fs_part_type(PT_undef, info.fs_type,
				    info.fs_sub_type);
			if (parts->pscheme->add_partition(parts, &info, NULL)
			    == NO_PART)
				return false;
		}
	}

	if (parts->pscheme->get_free_spaces(parts, &space, 1, 3*align,
	    align, start, -1) != 1)
		return false;

	memset(&info, 0, sizeof(info));
	info.start = space.start;
	info.size = space.size;
	info.flags = PTI_INSTALL_TARGET;
	if (parts->pscheme->secondary_scheme != NULL)
		info.flags |= PTI_SEC_CONTAINER;
	info.nat_type = parts->pscheme->get_generic_part_type(PT_root);
	nbsd = parts->pscheme->add_partition(parts, &info, NULL);

	if (nbsd == NO_PART)
		return false;

	if (!parts->pscheme->get_part_info(parts, nbsd, &info))
		return false;

	if (parts->pscheme->secondary_scheme != NULL) {
		/* force empty secondary partitions */
		parts->pscheme->secondary_partitions(parts, info.start, true);
	}

	return true;
}

static int
set_keep_existing(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_KEEPEXISTING;
	return 0;
}

static int
set_use_only_part(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_SETSIZES;
	return 0;
}

static int
set_use_entire_disk(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_USEFULL;
	return 0;
}

static int
set_switch_scheme(menudesc *m, void *arg)
{
	((arg_rep_int*)arg)->rv = LY_OTHERSCHEME;
	return 0;
}

static enum layout_type
ask_fullpart(struct disk_partitions *parts)
{
	arg_rep_int ai;
	const char *args[2];
	int menu;
	size_t num_opts;
	menu_ent options[4], *opt;
	daddr_t start, size;

	args[0] = msg_string(pm->parts->pscheme->name);
	args[1] = msg_string(pm->parts->pscheme->short_name);
	ai.args.argv = args;
	ai.args.argc = 2;
	ai.rv = LY_ERROR;

	memset(options, 0, sizeof(options));
	num_opts = 0;
	opt = &options[0];
	if (parts->pscheme->guess_install_target != NULL &&
	    parts->pscheme->guess_install_target(parts, &start, &size)) {
		opt->opt_name = MSG_Keep_existing_partitions;
		opt->opt_flags = OPT_EXIT;
		opt->opt_action = set_keep_existing;
		opt++;
		num_opts++;
	}
	opt->opt_name = MSG_Use_only_part_of_the_disk;
	opt->opt_flags = OPT_EXIT;
	opt->opt_action = set_use_only_part;
	opt++;
	num_opts++;

	opt->opt_name = MSG_Use_the_entire_disk;
	opt->opt_flags = OPT_EXIT;
	opt->opt_action = set_use_entire_disk;
	opt++;
	num_opts++;

	if (num_available_part_schemes > 1) {
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

/*
 * return (see post_edit_verify):
 *  0 -> abort
 *  1 -> re-edit
 *  2 -> continue installation
 */
static int
verify_outer_parts(struct disk_partitions *parts, bool quiet)
{
	part_id i;
	int num_bsdparts;
	daddr_t first_bsdstart, inst_start, inst_size;

	first_bsdstart = inst_start = -1;
	inst_size = 0;
	num_bsdparts = 0;
	for (i = 0; i < parts->num_part; i++) {
		struct disk_part_info info;
		if (!parts->pscheme->get_part_info(parts, i, &info))
			continue;
		if (!(info.flags & PTI_SEC_CONTAINER))
			continue;
		if (info.nat_type->generic_ptype != PT_root)
			continue;
		num_bsdparts++;

		if (first_bsdstart < 0) {
			first_bsdstart = info.start;
		}
		if (inst_start<  0 && (info.flags & PTI_INSTALL_TARGET)) {
			inst_start = info.start;
			inst_size = info.size;
		}
	}

	if (num_bsdparts == 0 ||
	    (num_bsdparts > 1 && inst_start < 0)) {
		if (quiet && num_bsdparts == 0)
			return 0;
		if (!quiet || parts->pscheme->guess_install_target == NULL ||
		    !parts->pscheme->guess_install_target(parts,
		    &inst_start, &inst_size)) {
			if (num_bsdparts == 0)
				msg_display_subst(MSG_nobsdpart, 2,
				    msg_string(parts->pscheme->name),
				    msg_string(parts->pscheme->short_name));
			else
				msg_display_subst(MSG_multbsdpart, 2,
				    msg_string(parts->pscheme->name),
				    msg_string(parts->pscheme->short_name));

			return ask_reedit(parts);
		}
	}

	/*
	 * post_edit_verify returns:
	 *  0 -> abort
	 *  1 -> re-edit
	 *  2 -> continue installation
	 */
	if (parts->pscheme->post_edit_verify)
		return parts->pscheme->post_edit_verify(parts, quiet);

	return 2;
}

static bool
ask_outer_partsizes(struct disk_partitions *parts)
{
	int j;
	int part_menu;
	size_t num_opts;
#ifndef NO_CLONES
	size_t i, ci;
#endif
	struct outer_parts_data data;

	part_menu_opts = outer_fill_part_menu_opts(parts, &num_opts);
	part_menu = new_menu(outer_part_title, part_menu_opts, num_opts,
			0, -1, 15, 70,
			MC_NOBOX|MC_ALWAYS_SCROLL|MC_NOCLEAR|MC_CONTINUOUS,
			draw_outer_part_header, draw_outer_part_line, NULL,
			NULL, MSG_Partition_table_ok);
	if (part_menu == -1) {
		free(part_menu_opts);
		return false;
	}

	/* Default to MB, and use bios geometry for cylinder size */
	set_default_sizemult(parts->disk, MEG, parts->bytes_per_sector);
	if (pm->current_cylsize == 0)
		pm->current_cylsize = 16065;	/* noone cares nowadays */
	memset(&data, 0, sizeof data);
	data.av.arg = parts;

	for (;;) {
		data.av.rv = 0;
		process_menu(part_menu, &data);
		if (data.av.rv < 0)
			break;

		j = verify_outer_parts(parts, false);
		if (j == 0) {
			data.av.rv = -1;
			return false;
		} else if (j == 1) {
			continue;
		}
		break;
	}

#ifndef NO_CLONES
	/* handle cloned partitions content copies now */
	for (i = 0; i < data.num_clone_entries; i++) {
		for (ci = 0; ci < data.clones[i].clone_src.num_sel; ci++) {
			if (data.clones[i].clone_src.with_data)
				clone_partition_data(parts,
				    data.clones[i].clone_ids[ci],
				    data.clones[i].clone_src.selection[ci].
				    parts,
				    data.clones[i].clone_src.selection[ci].id);
		}
	}

	/* free clone data */
	if (data.clones) {
		for (i = 0; i < data.num_clone_entries; i++)
			free_selected_partitions(&data.clones[i].clone_src);
		free(data.clones);
	}
#endif

	free_menu(part_menu);
	free(part_menu_opts);

	return data.av.rv == 0;
}

int
edit_outer_parts(struct disk_partitions *parts)
{
	part_id i;
	enum layout_type layout;
	int num_foreign_parts;

	/* If targeting a wedge, do not ask for further partitioning */
	if (pm && (pm->no_part || pm->no_mbr))
		return 1;

	/* Make sure parts has been properly initialized */
	assert(parts && parts->pscheme);

	if (parts->pscheme->secondary_scheme == NULL)
		return 1;	/* no outer parts */

	if (partman_go) {
		layout = LY_SETSIZES;
	} else {
		/* Ask full/part */
		const struct disk_partitioning_scheme *sec =
		    parts->pscheme->secondary_scheme;

		uint64_t m_size =
		    DEFROOTSIZE + DEFSWAPSIZE + DEFUSRSIZE + XNEEDMB;
		char min_size[5], build_size[5];
		const char
		    *prim_name = msg_string(parts->pscheme->name),
		    *prim_short = msg_string(parts->pscheme->short_name),
		    *sec_name = msg_string(sec->name),
		    *sec_short = msg_string(sec->short_name);

		humanize_number(min_size, sizeof(min_size),
		    m_size * MEG,
		    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
		humanize_number(build_size, sizeof(build_size),
		     SYSTEM_BUILD_SIZE * MEG, "", HN_AUTOSCALE,
		     HN_B | HN_NOSPACE | HN_DECIMAL);

		msg_display_subst(MSG_fullpart, 7,
		    pm->diskdev,
		    prim_name, sec_name,
		    prim_short, sec_short,
		    min_size, build_size);
		msg_display_add("\n\n");

		layout = ask_fullpart(parts);
		if (layout == LY_ERROR)
			return 0;
		else if (layout == LY_OTHERSCHEME)
			return -1;
	}

	if (layout == LY_USEFULL) {
		struct disk_part_info info;

		/* Count nonempty, non-BSD partitions. */
		num_foreign_parts = 0;
		for (i = 0; i < parts->num_part; i++) {
			if (!parts->pscheme->get_part_info(parts, i, &info))
				continue;
			if (info.size == 0)
				continue;
			if (info.flags & (PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
				continue;
			if (info.nat_type != NULL
			    && info.nat_type->generic_ptype != PT_root
			    && info.nat_type->generic_ptype != PT_swap)
				num_foreign_parts++;
		}

		/* Ask if we really want to blow away non-NetBSD stuff */
		if (num_foreign_parts > 0) {
			msg_display(MSG_ovrwrite);
			if (!ask_noyes(NULL)) {
				if (logfp)
					(void)fprintf(logfp,
					    "User answered no to destroy "
					    "other data, aborting.\n");
				return 0;
			}
		}
		if (!md_parts_use_wholedisk(parts)) {
			hit_enter_to_continue(MSG_No_free_space, NULL);
			return 0;
		}
		if (parts->pscheme->post_edit_verify) {
			return
			    parts->pscheme->post_edit_verify(parts, true) == 2;
		}
		return 1;
	} else if (layout == LY_SETSIZES) {
		return ask_outer_partsizes(parts);
	} else {
		return verify_outer_parts(parts, true) == 2;
	}
}

static int
set_part_scheme(menudesc *m, void *arg)
{
	size_t *res = arg;

	*res = (size_t)m->cursel;
	return 1;
}

const struct disk_partitioning_scheme *
select_part_scheme(
	struct pm_devs *dev,
	const struct disk_partitioning_scheme *skip,
	bool bootable,
	const char *hdr)
{
	int ps_menu = -1;
	menu_ent *opt;
	char **str, *ms = NULL;
	const struct disk_partitioning_scheme **options, *res;
	const char *title;
	size_t ndx, selected = ~0U, used;
	const struct disk_partitioning_scheme *p;
	bool showing_limit = false;

	if (hdr == NULL)
		hdr = MSG_select_part_scheme;

	opt = calloc(num_available_part_schemes, sizeof *opt);
	if (!opt)
		return NULL;
	str = calloc(num_available_part_schemes, sizeof *str);
	if (!str) {
		free(opt);
		return NULL;
	}
	options = calloc(num_available_part_schemes, sizeof *options);
	if (!options) {
		free(str);
		free(opt);
		return NULL;
	}

	for (used = 0, ndx = 0; ndx < num_available_part_schemes; ndx++) {
		p = available_part_schemes[ndx];
		/*
		 * Do not match exactly, we want to skip all lookalikes
		 * too (only_disklabel_parts vs. disklabel_parts)
		 */
		if (skip != NULL &&
		    p->create_new_for_disk == skip->create_new_for_disk)
			continue;
		if (bootable && p->have_boot_support != NULL &&
		    !p->have_boot_support(dev->diskdev))
			continue;
#ifdef HAVE_MBR
		if (dev->no_mbr && p->name == MSG_parttype_mbr)
			continue;
#endif
		if (p->size_limit && dev->dlsize*(dev->sectorsize/512) >
		    p->size_limit) {
			char buf[255], hum_lim[5];

			humanize_number(hum_lim, sizeof(hum_lim),
			    (uint64_t)p->size_limit*512UL,
			    "", HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
			sprintf(buf, "%s [%s %s]", msg_string(p->name),
			    msg_string(MSG_size_limit), hum_lim);
			str[used] = strdup(buf);
			showing_limit = true;
		} else {
			str[used] = strdup(msg_string(p->name));
		}
		if (!str[used])
			goto out;

		opt[used].opt_name = str[used];
		opt[used].opt_action = set_part_scheme;
		options[used] = p;
		used++;
	}

	/* do not bother to ask if there are no options */
	if (used <= 1) {
		selected = (used == 1) ? 0 : ~0U;
		goto out;
	}

	if (showing_limit) {
		char hum_lim[5], *tmp;
		size_t total;

		const char *p1 = msg_string(hdr);
		const char *p2 = msg_string(MSG_select_part_limit);

		humanize_number(hum_lim, sizeof(hum_lim),
		    (uint64_t)dev->dlsize*dev->sectorsize, "",
		    HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);

		const char *args[] = { dev->diskdev, hum_lim };
		char *p3 = str_arg_subst(msg_string(MSG_part_limit_disksize),
		    __arraycount(args), args);

		total = strlen(p1) + strlen(p2) + strlen(p3)
		    + sizeof(hum_lim) + 5;
		ms = tmp = malloc(total);
		title = tmp;
		strcpy(tmp, p1); tmp += strlen(p1);
		*tmp++ = '\n'; *tmp++ = '\n';
		strcpy(tmp, p2); tmp += strlen(p2);
		*tmp++ = '\n'; *tmp++ = '\n';
		strcpy(tmp, p3);
		free(p3);
		assert(strlen(ms) < total);
	} else {
		title = msg_string(hdr);
	}
	ps_menu = new_menu(title, opt, used,
	    -1, 5, 0, 0, 0, NULL, NULL, NULL, NULL, MSG_exit_menu_generic);
	if (ps_menu != -1)
		process_menu(ps_menu, &selected);
out:
	res = selected >= used ? NULL : options[selected];
	for (ndx = 0; ndx < used; ndx++)
		free(str[ndx]);
	if (showing_limit && ms)
		free(ms);
	free(str);
	free(opt);
	free(options);
	if (ps_menu != -1)
		free_menu(ps_menu);

	return res;
}
