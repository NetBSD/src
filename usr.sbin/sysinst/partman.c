/*	$NetBSD: partman.c,v 1.55 2022/04/08 10:17:55 andvar Exp $ */

/*
 * Copyright 2012 Eugene Lozovoy
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
 * 3. The name of Eugene Lozovoy may not be used to endorse
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
 * Copyright 2010 The NetBSD Foundation, Inc.
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

/* partman.c - extended partitioning */

#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* XXX: replace all MAX_* defines with vars that depend on kernel settings */
#define MAX_ENTRIES 96

#define MAX_RAID 8
#define MAX_IN_RAID 48
struct raid_comp {
	char name[SSTRSIZE];	/* display name for this component */
	struct disk_partitions *parts;	/* where is this on? */
	part_id id;		/* which partition in parts */
	bool is_spare;		/* is this a spare component? */
};
struct raid_desc {
	int enabled;
	int blocked;
	int node;		/* the N in raid${N} */
	int numRow, numCol, numSpare;
	int sectPerSU, SUsPerParityUnit, SUsPerReconUnit, raid_level;
	daddr_t total_size;
	struct raid_comp comp[MAX_IN_RAID];
};
struct raid_desc *raids;

#define MAX_VND 4
struct vnd_desc {
	int enabled;
	int blocked;
	int node;	/* the N in vnd${N} */
	char filepath[STRSIZE];
	daddr_t size;
	int readonly;
	int is_exist;
	int manual_geom;
	int secsize, nsectors, ntracks, ncylinders;
	struct pm_devs *pm;	/* device this is on */
	part_id pm_part;	/* which partition (in pm->parts) */
};
struct vnd_desc *vnds;

#define MAX_CGD 4
struct cgd_desc {
	int enabled;
	int blocked;
	int node;	/* the N in cgd${N} */
	char pm_name[SSTRSIZE];
	const char *keygen_type;
	const char *verify_type;
	const char *enc_type;
	const char *iv_type;
	int key_size;
	struct pm_devs *pm;	/* device this is on */
	part_id pm_part;	/* which partition (in pm->parts) */
};
struct cgd_desc *cgds;

#define MAX_LVM_VG 16
#define MAX_LVM_PV 255
#define MAX_LVM_LV 255

struct lvm_pv_reg {
	struct pm_devs *pm;
	daddr_t start;
};
struct lvm_pv_reg lvm_pvs[MAX_LVM_PV];	/* XXX - make dynamic */

typedef struct pv_t {
	struct pm_devs *pm;
	char pm_name[SSTRSIZE];
	part_id pm_part;
	int metadatasize;
	int metadatacopies;
	int labelsector;
	int setphysicalvolumesize;
} pv_t;
typedef struct lv_t {
	int blocked;
	daddr_t size;
	char name[SSTRSIZE];
	int readonly;
	int contiguous;
	char extents[SSTRSIZE];
	int minor;
	int mirrors;
	int regionsize;
	int persistent;
	int readahead;
	int stripes;
	int stripesize;
	int zero;
} lv_t;
typedef struct lvms_t {
	int enabled;
	int blocked;
	char name[SSTRSIZE];
	int maxlogicalvolumes;
	int maxphysicalvolumes;
	int physicalextentsize;
	daddr_t total_size;
	pv_t pv[MAX_LVM_PV];
	lv_t lv[MAX_LVM_LV];
} lvms_t;
lvms_t *lvms;

typedef struct structinfo_t {
	int max;
	uint entry_size;
	uint parent_size;
	void *entry_first;
	void *entry_enabled;
	void *entry_blocked;
	void *entry_node;
} structinfo_t;
structinfo_t raids_t_info, vnds_t_info, cgds_t_info, lvms_t_info, lv_t_info;

typedef struct pm_upddevlist_adv_t {
	const char *create_msg;
	int pe_type;
	structinfo_t *s;
	int sub_num;
	struct pm_upddevlist_adv_t *sub;
} pm_upddevlist_adv_t;

#define MAX_MNTS 48
struct {
    char dev[STRSIZE];
    const char *mnt_opts, *on;
} *mnts;

static int pm_cursel; /* Number of selected entry in main menu */
static int pm_changed; /* flag indicating that we have unsaved changes */
static int pm_raid_curspare; /* XXX: replace by true way */
static int pm_retvalue;

enum { /* RAIDframe menu enum */
	PMR_MENU_DEVS, PMR_MENU_DEVSSPARE, PMR_MENU_RAIDLEVEL, PMR_MENU_NUMROW,
	PMR_MENU_NUMCOL, PMR_MENU_NUMSPARE,	PMR_MENU_SECTPERSU,	PMR_MENU_SUSPERPARITYUNIT,
	PMR_MENU_SUSPERRECONUNIT, PMR_MENU_REMOVE, PMR_MENU_END
};

enum { /* VND menu enum */
	PMV_MENU_FILEPATH, PMV_MENU_EXIST, PMV_MENU_SIZE, PMV_MENU_RO, PMV_MENU_MGEOM,
	PMV_MENU_SECSIZE, PMV_MENU_NSECTORS, PMV_MENU_NTRACKS, PMV_MENU_NCYLINDERS,
	PMV_MENU_REMOVE, PMV_MENU_END
};

enum { /* CGD menu enum */
	PMC_MENU_DEV, PMC_MENU_ENCTYPE, PMC_MENU_KEYSIZE, PMC_MENU_IVTYPE,
	PMC_MENU_KEYGENTYPE, PMC_MENU_VERIFYTYPE, PMC_MENU_REMOVE, PMC_MENU_END
};

enum { /* LVM menu enum */
	PML_MENU_PV, PML_MENU_NAME, PML_MENU_MAXLOGICALVOLUMES,
	PML_MENU_MAXPHYSICALVOLUMES, PML_MENU_PHYSICALEXTENTSIZE,
	PML_MENU_REMOVE, PML_MENU_END
};

enum { /* LVM submenu (logical volumes) enum */
	PMLV_MENU_NAME, PMLV_MENU_SIZE, PMLV_MENU_READONLY, PMLV_MENU_CONTIGUOUS,
	PMLV_MENU_EXTENTS, PMLV_MENU_MINOR, PMLV_MENU_PERSISTENT,
	PMLV_MENU_MIRRORS, PMLV_MENU_REGIONSIZE, PMLV_MENU_READAHEAD,
	PMLV_MENU_STRIPES, PMLV_MENU_STRIPESIZE, PMLV_MENU_ZERO,
	PMLV_MENU_REMOVE, PMLV_MENU_END
};

struct part_entry pm_dev_list(int);
static int pm_raid_disk_add(menudesc *, void *);
static int pm_raid_disk_del(menudesc *, void *);
static int pm_cgd_disk_set(struct cgd_desc *, struct part_entry *);
static int pm_mount(struct pm_devs *, int);
static int pm_upddevlist(menudesc *, void *);
static void pm_select(struct pm_devs *);

static void
pm_edit_size_value(msg prompt_msg, daddr_t bps, daddr_t cylsec, daddr_t *size)
{

	char answer[16], dflt[16];
	daddr_t new_size_val, mult;

	snprintf(dflt, sizeof dflt, "%" PRIu64 "%s", *size / sizemult,
	    multname);

	msg_prompt_win(prompt_msg, -1, 18, 0, 0, dflt, answer, sizeof answer);

	mult = sizemult;
	new_size_val = parse_disk_pos(answer, &mult, bps, cylsec, NULL);

	if (new_size_val > 0)
		*size = new_size_val * mult;
}

static const char *
pm_get_mount(struct pm_devs *p, part_id id)
{

	if (p->mounted == NULL)
		return NULL;
	if (id >= p->parts->num_part)
		return NULL;
	return p->mounted[id];
}

bool pm_set_mount(struct pm_devs *p, part_id id, const char *path);

bool
pm_set_mount(struct pm_devs *p, part_id id, const char *path)
{

	if (p->parts == NULL || id >= p->parts->num_part)
		return false;

	if (p->mounted == NULL) {
		p->mounted = calloc(p->parts->num_part, sizeof(char*));
		if (p->mounted == NULL)
			return false;
	}
	free(p->mounted[id]);
	p->mounted[id] = strdup(path);
	return p->mounted[id] != NULL;
}

/* Universal menu for RAID/VND/CGD/LVM entry edit */
static int
pm_edit(int menu_entries_count, void (*menu_fmt)(menudesc *, int, void *),
	int (*action)(menudesc *, void *), int (*check_fun)(void *),
	void (*entry_init)(void *, void *),	void *entry_init_arg,
	void *dev_ptr, int dev_ptr_delta, structinfo_t *s)
{
	int i, ok = 0;
	menu_ent *menu_entries;

	if (dev_ptr == NULL) {
		/* We should create new device */
		for (i = 0; i < s->max && !ok; i++)
			if (*(int*)((char*)s->entry_enabled + dev_ptr_delta + s->entry_size * i) == 0) {
				dev_ptr = (char*)s->entry_first + dev_ptr_delta + s->entry_size * i;
				entry_init(dev_ptr, entry_init_arg);
				ok = 1;
			}
		if (!ok) {
			/* We do not have free device slots */
			hit_enter_to_continue(NULL, MSG_limitcount);
			return -1;
		}
	}

	menu_entries = calloc(menu_entries_count, sizeof *menu_entries);
	for (i = 0; i < menu_entries_count - 1; i++)
		menu_entries[i] = (menu_ent) { .opt_action=action };
	menu_entries[i] = (menu_ent) {	.opt_name=MSG_fremove,
					.opt_flags=OPT_EXIT,
					.opt_action=action };

	int menu_no = -1;
	menu_no = new_menu(NULL, menu_entries, menu_entries_count,
		-1, -1, 0, 40, MC_NOCLEAR | MC_SCROLL,
		NULL, menu_fmt, NULL, NULL, MSG_DONE);

	process_menu(menu_no, dev_ptr);
	free_menu(menu_no);
	free(menu_entries);

	return check_fun(dev_ptr);
}

/* Show filtered partitions menu */
struct part_entry
pm_dev_list(int type)
{
	int dev_num = -1, num_devs = 0;
	bool ok;
	part_id i;
	int menu_no;
	struct disk_part_info info;
	menu_ent menu_entries[MAX_DISKS*MAXPARTITIONS];
	struct part_entry disk_entries[MAX_DISKS*MAXPARTITIONS];
	struct pm_devs *pm_i;

	SLIST_FOREACH(pm_i, &pm_head, l) {
		if (pm_i->parts == NULL)
			continue;
		for (i = 0; i < pm_i->parts->num_part; i++) {
			ok = false;
			if (!pm_i->parts->pscheme->get_part_info(pm_i->parts,
			    i, &info))
				continue;
			if (info.flags &
			    (PTI_WHOLE_DISK|PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
				continue;
			switch (type) {
				case PM_RAID:
					if (info.fs_type == FS_RAID)
						ok = 1;
					break;
				case PM_CGD:
					if (info.fs_type == FS_CGD)
						ok = 1;
					break;
				case PM_LVM:
					if (pm_is_lvmpv(pm_i, i, &info))
						ok = 1;
					break;
			}
			if (!ok)
				continue;
			if (pm_partusage(pm_i, i, 0) != 0)
				continue;

			disk_entries[num_devs].dev_ptr = pm_i;
			disk_entries[num_devs].id = i;
			disk_entries[num_devs].parts = pm_i->parts;

			pm_i->parts->pscheme->get_part_device(
			    pm_i->parts, i, disk_entries[num_devs].fullname,
			    sizeof disk_entries[num_devs].fullname,
			    NULL, plain_name, false, true);

			menu_entries[num_devs] = (struct menu_ent) {
				.opt_name = disk_entries[num_devs].fullname,
				.opt_action = set_menu_select,
				.opt_flags = OPT_EXIT,
			};
			num_devs++;
		}
	}

	menu_no = new_menu(MSG_avdisks,
		menu_entries, num_devs, -1, -1,
		(num_devs+1<3)?3:num_devs+1, 13,
		MC_SCROLL | MC_NOCLEAR, NULL, NULL, NULL, NULL, MSG_cancel);
	if (menu_no == -1)
		return (struct part_entry) { 0 };
	process_menu(menu_no, &dev_num);
	free_menu(menu_no);

	if (dev_num < 0 || dev_num >= num_devs)
		return (struct part_entry) { 0 };

	pm_retvalue = dev_num;
	return disk_entries[dev_num];
}

/* Get unused raid*, cgd* or vnd* device */
static int
pm_manage_getfreenode(void *node, const char *d, structinfo_t *s)
{
	int i, ii, ok;
	char buf[SSTRSIZE];
	struct pm_devs *pm_i;

	*(int*)node = -1;
	for (i = 0; i < s->max; i++) {
		ok = 1;
		/* Check that node is not already reserved */
		for (ii = 0; ii < s->max; ii++) {
			if (*(int*)((char*)s->entry_enabled + s->entry_size
			    * ii) == 0)
				continue;
			if (*(int*)((char*)s->entry_node + s->entry_size * ii)
			    == i) {
				ok = 0;
				break;
			}
		}
		if (! ok)
			continue;
		/* Check that node is not in the device list */
		snprintf(buf, SSTRSIZE, "%s%d", d, i);
		SLIST_FOREACH(pm_i, &pm_head, l)
			if (! strcmp(pm_i->diskdev, buf)) {
				ok = 0;
				break;
			}
		if (ok) {
			*(int*)node = i;
			return i;
		}
	}
	hit_enter_to_continue(NULL, MSG_nofreedev);
	return -1;
}

/*
 * Show a line for a device, usually with full size in the right
 * column, alternatively (if != NULL) with no_size_display
 * instead in parentheses (used for error displays or to note
 * a action that can be done to this device.
 */
static void
pm_fmt_disk_line(WINDOW *w, const char *line, const char *on,
    daddr_t total, const char *no_size_display)
{
	char out[STRSIZE], human[6];

	if (on != NULL) {
		snprintf(out, sizeof out, "%s %s %s", line,
		    msg_string(MSG_pm_menu_on), on);
		line = out;
	}
	if (no_size_display != NULL) {
		wprintw(w, "   %-56s (%s)", line, no_size_display);
	} else {
		humanize_number(human, sizeof(human),
	            total * 512, "",
		    HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
		wprintw(w, "   %-56s %s", line, human);
	}
}

/***
 RAIDs
 ***/

static void
pm_raid_menufmt(menudesc *m, int opt, void *arg)
{
	int i, ok = 0;
	char buf[STRSIZE], rdev[STRSIZE], level[STRSIZE], *line;
	struct raid_desc *dev_ptr = ((struct part_entry *)arg)[opt].dev_ptr;

	if (dev_ptr->enabled == 0)
		return;
	buf[0] = '\0';
	sprintf(rdev, "raid%d", dev_ptr->node);
	for (i = 0; i < MAX_IN_RAID; i++) {
		if (dev_ptr->comp[i].parts != NULL) {
			strlcat(buf, dev_ptr->comp[i].name, sizeof buf);
			strlcat(buf, " ", sizeof buf);
			ok = 1;
		}
	}
	if (ok) {
		sprintf(level, "%u", dev_ptr->raid_level);
		const char *args[] = { rdev, level };
		line = str_arg_subst(msg_string(MSG_raid_menufmt),
		    __arraycount(args), args);
		pm_fmt_disk_line(m->mw, line, buf, dev_ptr->total_size, NULL);
		free(line);
	} else {
		pm_fmt_disk_line(m->mw, buf, NULL, 0,
		    msg_string(MSG_raid_err_menufmt));
	}
}

static void
pm_raid_edit_menufmt(menudesc *m, int opt, void *arg)
{
	int i;
	char buf[STRSIZE];
	struct raid_desc *dev_ptr = arg;

	buf[0] = '\0';
	switch (opt) {
		case PMR_MENU_DEVS:
			strlcpy(buf, msg_string(MSG_raid_disks_fmt),
			    sizeof buf);
			strlcat(buf, ": ", sizeof buf);
			for (i = 0; i < MAX_IN_RAID; i++) {
				if (dev_ptr->comp[i].parts == NULL ||
				     dev_ptr->comp[i].is_spare)
					continue;
				strlcat(buf, " ", sizeof buf);
				strlcat(buf, dev_ptr->comp[i].name, sizeof buf);
			}
			wprintw(m->mw, "%s", buf);
			break;
		case PMR_MENU_DEVSSPARE:
			strlcpy(buf, msg_string(MSG_raid_spares_fmt),
			    sizeof buf);
			strlcat(buf, ": ", sizeof buf);
			for (i = 0; i < MAX_IN_RAID; i++) {
				if (dev_ptr->comp[i].parts == NULL ||
				     !dev_ptr->comp[i].is_spare)
					continue;
				strlcat(buf, " ", sizeof buf);
				strlcat(buf, dev_ptr->comp[i].name, sizeof buf);
			}
			wprintw(m->mw, "%s", buf);
			break;
		case PMR_MENU_RAIDLEVEL:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_level_fmt),
			    dev_ptr->raid_level);
			break;
		case PMR_MENU_NUMROW:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_numrow_fmt), dev_ptr->numRow);
			break;
		case PMR_MENU_NUMCOL:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_numcol_fmt), dev_ptr->numCol);
			break;
		case PMR_MENU_NUMSPARE:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_numspare_fmt),
			    dev_ptr->numSpare);
			break;
		case PMR_MENU_SECTPERSU:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_sectpersu_fmt),
			    dev_ptr->sectPerSU);
			break;
		case PMR_MENU_SUSPERPARITYUNIT:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_superpar_fmt),
			    dev_ptr->SUsPerParityUnit);
			break;
		case PMR_MENU_SUSPERRECONUNIT:
			wprintw(m->mw, "%s: %u",
			    msg_string(MSG_raid_superrec_fmt),
			    dev_ptr->SUsPerReconUnit);
			break;
	}
}

static int
pm_raid_set_value(menudesc *m, void *arg)
{
	int retvalue = -1;
	int *out_var = NULL;
	char buf[SSTRSIZE];
	const char *msg_to_show = NULL;
	struct raid_desc *dev_ptr = arg;

	static menu_ent menuent_disk_adddel[] = {
	    { .opt_name=MSG_add, .opt_flags=OPT_EXIT,
	      .opt_action=pm_raid_disk_add },
	    { .opt_name=MSG_remove, .opt_flags=OPT_EXIT,
	      .opt_action=pm_raid_disk_del }
	};
	static int menu_disk_adddel = -1;
	if (menu_disk_adddel == -1) {
		menu_disk_adddel = new_menu(NULL, menuent_disk_adddel,
			__arraycount(menuent_disk_adddel),
			-1, -1, 0, 10, MC_NOCLEAR, NULL, NULL, NULL, NULL,
			MSG_cancel);
	}

	switch (m->cursel) {
		case PMR_MENU_DEVS:
			pm_raid_curspare = 0;
			process_menu(menu_disk_adddel, dev_ptr);
			return 0;
		case PMR_MENU_DEVSSPARE:
			pm_raid_curspare = 1;
			process_menu(menu_disk_adddel, dev_ptr);
			return 0;
		case PMR_MENU_RAIDLEVEL:
			process_menu(MENU_raidlevel, &retvalue);
			if (retvalue >= 0)
				dev_ptr->raid_level = retvalue;
			return 0;
		case PMR_MENU_NUMROW:
			hit_enter_to_continue(NULL, MSG_raid_nomultidim);
			return 0;
#if 0 /* notyet */
			msg_to_show = MSG_raid_numrow_ask;
			out_var = &(dev_ptr->numRow);
			break;
#endif
		case PMR_MENU_NUMCOL:
			msg_to_show = MSG_raid_numcol_ask;
			out_var = &(dev_ptr->numCol);
			break;
		case PMR_MENU_NUMSPARE:
			msg_to_show = MSG_raid_numspare_ask;
			out_var = &(dev_ptr->numSpare);
			break;
		case PMR_MENU_SECTPERSU:
			msg_to_show = MSG_raid_sectpersu_ask;
			out_var = &(dev_ptr->sectPerSU);
			break;
		case PMR_MENU_SUSPERPARITYUNIT:
			msg_to_show = MSG_raid_superpar_ask;
			out_var = &(dev_ptr->SUsPerParityUnit);
			break;
		case PMR_MENU_SUSPERRECONUNIT:
			msg_to_show = MSG_raid_superrec_ask;
			out_var = &(dev_ptr->SUsPerReconUnit);
			break;
		case PMR_MENU_REMOVE:
			dev_ptr->enabled = 0;
			return 0;
	}
	if (out_var == NULL || msg_to_show == NULL)
		return -1;
	snprintf(buf, SSTRSIZE, "%d", *out_var);
	msg_prompt_win(msg_to_show, -1, 18, 0, 0, buf, buf, SSTRSIZE);
	if (atoi(buf) >= 0)
		*out_var = atoi(buf);
	return 0;
}

static void
pm_raid_init(void *arg, void *none)
{
	struct raid_desc *dev_ptr = arg;
	memset(dev_ptr, 0, sizeof(*dev_ptr));
	*dev_ptr = (struct raid_desc) {
		.enabled = 1,
		.blocked = 0,
		.sectPerSU = 32,
		.SUsPerParityUnit = 1,
		.SUsPerReconUnit = 1,
	};
}

static int
pm_raid_check(void *arg)
{
	size_t i, dev_num = 0;
	daddr_t min_size = 0, cur_size = 0;
	struct raid_desc *dev_ptr = arg;
	struct disk_part_info info;
	struct disk_partitions *parts;

	if (dev_ptr->blocked)
		return 0;

	for (i = 0; i < MAX_IN_RAID; i++) {
		if (dev_ptr->comp[i].parts != NULL) {
			parts = dev_ptr->comp[i].parts;
			if (!parts->pscheme->get_part_info(parts,
			    dev_ptr->comp[i].id, &info))
				continue;
			cur_size = info.size;
			if (cur_size < min_size || dev_num == 0)
				min_size = cur_size;
			if (dev_ptr->comp[i].is_spare)
				continue;
			dev_num++;
		}
	}

	/* Calculate sum of available space */
	if (dev_num > 0) {
		switch (dev_ptr->raid_level) {
			case 0:
				dev_ptr->total_size = min_size * dev_num;
				break;
			case 1:
				dev_ptr->total_size = min_size;
				break;
			case 4:
			case 5:
				dev_ptr->total_size = min_size * (dev_num - 1);
				break;
		}
		pm_manage_getfreenode(&(dev_ptr->node), "raid", &raids_t_info);
		if (dev_ptr->node < 0)
			dev_ptr->enabled = 0;
	}
	else
		dev_ptr->enabled = 0;
	return dev_ptr->enabled;
}

static int
pm_raid_disk_add(menudesc *m, void *arg)
{
	int i;
	struct raid_desc *dev_ptr = arg;
	struct part_entry disk_entrie = pm_dev_list(PM_RAID);
	if (pm_retvalue < 0)
		return pm_retvalue;

	for (i = 0; i < MAX_IN_RAID; i++)
		if (dev_ptr->comp[i].parts == NULL) {
			dev_ptr->comp[i].parts = disk_entrie.parts;
			dev_ptr->comp[i].id = disk_entrie.id;
			dev_ptr->comp[i].is_spare = pm_raid_curspare;
			strlcpy(dev_ptr->comp[i].name, disk_entrie.fullname,
			    sizeof dev_ptr->comp[i].name);
			if (pm_raid_curspare)
				dev_ptr->numSpare++;
			else
				dev_ptr->numCol++;
			dev_ptr->numRow = 1;
			break;
		}
	return 0;
}

static int
pm_raid_disk_del(menudesc *m, void *arg)
{
	int retvalue = -1, num_devs = 0;
	int i, pm_cur;
	int menu_no;
	struct raid_desc *dev_ptr = arg;
	menu_ent menu_entries[MAX_IN_RAID];
	struct part_entry submenu_args[MAX_IN_RAID];

	for (i = 0; i < MAX_IN_RAID; i++) {
		if (dev_ptr->comp[i].parts == NULL ||
			dev_ptr->comp[i].is_spare != pm_raid_curspare)
			continue;
		menu_entries[num_devs] = (struct menu_ent) {
			.opt_name = dev_ptr->comp[i].name,
			.opt_action = set_menu_select,
			.opt_flags = OPT_EXIT,
		};
		submenu_args[num_devs].dev_ptr = dev_ptr;
		submenu_args[num_devs].index = i;
		num_devs++;
	}

	menu_no = new_menu(MSG_raid_disks,
		menu_entries, num_devs, -1, -1,
		(num_devs+1<3)?3:num_devs+1, 13,
		MC_SCROLL | MC_NOCLEAR, NULL, NULL, NULL, NULL, MSG_cancel);
	if (menu_no == -1)
		return -1;
	process_menu(menu_no, &retvalue);
	free_menu(menu_no);

	if (retvalue < 0 || retvalue >= num_devs)
		return -1;

	pm_cur = submenu_args[retvalue].index;

	if (dev_ptr->comp[pm_cur].is_spare)
		dev_ptr->numSpare--;
	else
		dev_ptr->numCol--;
	dev_ptr->numRow = (dev_ptr->numCol)?1:0;
	dev_ptr->comp[pm_cur].parts = NULL;

	return 0;
}

static int
pm_raid_commit(void)
{
	int i, ii;
	FILE *f;
	char f_name[STRSIZE], devname[STRSIZE];

	for (i = 0; i < MAX_RAID; i++) {
		if (! pm_raid_check(&raids[i]))
			continue;

		/* Generating configure file for our raid */
		snprintf(f_name, SSTRSIZE, "/tmp/raid.%d.conf", raids[i].node);
		f = fopen(f_name, "w");
		if (f == NULL) {
			endwin();
			(void)fprintf(stderr,
			    "Could not open %s for writing\n", f_name);
			if (logfp)
				(void)fprintf(logfp,
				    "Could not open %s for writing\n", f_name);
			return 1;
		}
		scripting_fprintf(NULL, "cat <<EOF >%s\n", f_name);
		scripting_fprintf(f, "START array\n%d %d %d\n",
		    raids[i].numRow, raids[i].numCol, raids[i].numSpare);

		scripting_fprintf(f, "\nSTART disks\n");
		for (ii = 0; ii < MAX_IN_RAID; ii++) {
			if (raids[i].comp[ii].parts != NULL &&
			    !raids[i].comp[ii].is_spare) {
				strcpy(devname, raids[i].comp[ii].name);
				if (raids[i].comp[ii].parts != NULL &&
				    raids[i].comp[ii].id != NO_PART) {
					/* wedge may have moved */
					raids[i].comp[ii].parts->pscheme->
					    get_part_device(
					    raids[i].comp[ii].parts,
					    raids[i].comp[ii].id,
					    devname, sizeof devname, NULL,
					    logical_name, true, true);
					raids[i].comp[ii].parts->pscheme->
					    get_part_device(
					    raids[i].comp[ii].parts,
					    raids[i].comp[ii].id,
					    raids[i].comp[ii].name,
					    sizeof raids[i].comp[ii].name,
					    NULL, plain_name, true, true);
				}
				scripting_fprintf(f,  "%s\n", devname);
			}
		}

		scripting_fprintf(f, "\nSTART spare\n");
		for (ii = 0; ii < MAX_IN_RAID; ii++) {
			if (raids[i].comp[ii].parts != NULL &&
			    raids[i].comp[ii].is_spare) {
				strcpy(devname, raids[i].comp[ii].name);
				if (raids[i].comp[ii].parts != NULL &&
				    raids[i].comp[ii].id != NO_PART) {
					/* wedge may have moved */
					raids[i].comp[ii].parts->pscheme->
					    get_part_device(
					    raids[i].comp[ii].parts,
					    raids[i].comp[ii].id,
					    devname, sizeof devname, NULL,
					    logical_name, true, true);
					raids[i].comp[ii].parts->pscheme->
					    get_part_device(
					    raids[i].comp[ii].parts,
					    raids[i].comp[ii].id,
					    raids[i].comp[ii].name,
					    sizeof raids[i].comp[ii].name,
					    NULL, plain_name, true, true);
				}

				scripting_fprintf(f,  "%s\n",
				    devname);
			}
		}

		scripting_fprintf(f, "\nSTART layout\n%d %d %d %d\n",
		    raids[i].sectPerSU,	raids[i].SUsPerParityUnit,
		    raids[i].SUsPerReconUnit, raids[i].raid_level);

		scripting_fprintf(f, "\nSTART queue\nfifo 100\n\n");
		scripting_fprintf(NULL, "EOF\n");
		fclose (f);
		fflush(NULL);

		/* Raid initialization */
		if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"raidctl -C %s raid%d", f_name, raids[i].node) == 0 &&
		    run_program(RUN_DISPLAY | RUN_PROGRESS,
			"raidctl -I %d raid%d", rand(), raids[i].node) == 0 &&
		    run_program(RUN_DISPLAY | RUN_PROGRESS,
		        "raidctl -vi raid%d", raids[i].node) == 0 &&
		    run_program(RUN_DISPLAY | RUN_PROGRESS,
			"raidctl -v -A yes raid%d", raids[i].node) == 0) {
			/*
			 * RAID creation done, remove it from list to
			 * prevent its repeated reinitialization
			 */
			raids[i].blocked = 1;
		}
	}
	return 0;
}

/***
 VND
 ***/

static void
pm_vnd_menufmt(menudesc *m, int opt, void *arg)
{
	struct vnd_desc *dev_ptr = ((struct part_entry *)arg)[opt].dev_ptr;
	char dev[STRSIZE];

	if (dev_ptr->enabled == 0)
		return;
	sprintf(dev, "vnd%d", dev_ptr->node);
	if (strlen(dev_ptr->filepath) < 1)
		pm_fmt_disk_line(m->mw, dev, NULL,
		    0, msg_string(MSG_vnd_err_menufmt));
	else if (dev_ptr->is_exist)
		pm_fmt_disk_line(m->mw, dev, dev_ptr->filepath,
		    0, msg_string(MSG_vnd_assign));
	else
		pm_fmt_disk_line(m->mw, dev, dev_ptr->filepath,
		    dev_ptr->size, NULL);
}

static int
max_msg_length(const msg *p, size_t cnt)
{
	int len, m = 0;

	while (cnt > 0) {
		len = strlen(msg_string(*p));
		if (len > m)
			m = len;
		cnt--; p++;
	}

	return m;
}

static void
pm_vnd_edit_menufmt(menudesc *m, int opt, void *arg)
{
	struct vnd_desc *dev_ptr = arg;
	char buf[SSTRSIZE];
	strcpy(buf, "-");
	static int lcol_width;
	if (lcol_width == 0) {
		static const msg labels[] = {
		    MSG_vnd_path_fmt, MSG_vnd_assign_fmt, MSG_vnd_size_fmt,
		    MSG_vnd_ro_fmt, MSG_vnd_geom_fmt, MSG_vnd_bps_fmt,
		    MSG_vnd_spt_fmt, MSG_vnd_tpc_fmt, MSG_vnd_cyl_fmt
		};
		lcol_width = max_msg_length(labels, __arraycount(labels)) + 3;
	}

	switch (opt) {
		case PMV_MENU_FILEPATH:
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_path_fmt), dev_ptr->filepath);
			break;
		case PMV_MENU_EXIST:
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_assign_fmt),
			    dev_ptr->is_exist?
				msg_string(MSG_No) : msg_string(MSG_Yes));
			break;
		case PMV_MENU_SIZE:
			if (!dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%" PRIu64,
				    dev_ptr->size / sizemult);
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_size_fmt), buf);
			break;
		case PMV_MENU_RO:
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_ro_fmt),
			    dev_ptr->readonly?
				msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMV_MENU_MGEOM:
			if (!dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%s",
				    dev_ptr->manual_geom?
				    msg_string(MSG_Yes) : msg_string(MSG_No));
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_geom_fmt), buf);
			break;
		case PMV_MENU_SECSIZE:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->secsize);
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_bps_fmt), buf);
			break;
		case PMV_MENU_NSECTORS:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d",
				    dev_ptr->nsectors);
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_spt_fmt), buf);
			break;
		case PMV_MENU_NTRACKS:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->ntracks);
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_tpc_fmt), buf);
			break;
		case PMV_MENU_NCYLINDERS:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d",
				    dev_ptr->ncylinders);
			wprintw(m->mw, "%*s %s", -lcol_width,
			    msg_string(MSG_vnd_cyl_fmt), buf);
			break;
	}
}

static int
pm_vnd_set_value(menudesc *m, void *arg)
{
	struct vnd_desc *dev_ptr = arg;
	char buf[STRSIZE];
	const char *msg_to_show = NULL;
	int *out_var = NULL;

	switch (m->cursel) {
		case PMV_MENU_FILEPATH:
			msg_prompt_win(MSG_vnd_path_ask, -1, 18, 0, 0,
			    dev_ptr->filepath, dev_ptr->filepath, STRSIZE);
			if (dev_ptr->filepath[0] != '/') {
				strlcpy(buf, dev_ptr->filepath, MOUNTLEN);
				snprintf(dev_ptr->filepath, MOUNTLEN, "/%s",
				    buf);
			}
			if (dev_ptr->filepath[strlen(dev_ptr->filepath) - 1]
			    == '/')
				dev_ptr->filepath[strlen(dev_ptr->filepath)
				     - 1] = '\0';
			return 0;
		case PMV_MENU_EXIST:
			dev_ptr->is_exist = !dev_ptr->is_exist;
			return 0;
		case PMV_MENU_SIZE:
			if (dev_ptr->is_exist)
				return 0;

			pm_edit_size_value(MSG_vnd_size_ask,
			    pm->sectorsize, pm->dlcylsize,
			    &dev_ptr->size);

			break;
		case PMV_MENU_RO:
			dev_ptr->readonly = !dev_ptr->readonly;
			return 0;
		case PMV_MENU_MGEOM:
			if (dev_ptr->is_exist)
				return 0;
			dev_ptr->manual_geom = !dev_ptr->manual_geom;
			return 0;
		case PMV_MENU_SECSIZE:
			if (!dev_ptr->manual_geom || dev_ptr->is_exist)
				return 0;
			msg_to_show = MSG_vnd_bps_ask;
			out_var = &(dev_ptr->secsize);
			break;
		case PMV_MENU_NSECTORS:
			if (!dev_ptr->manual_geom || dev_ptr->is_exist)
				return 0;
			msg_to_show = MSG_vnd_spt_ask;
			out_var = &(dev_ptr->nsectors);
			break;
		case PMV_MENU_NTRACKS:
			if (!dev_ptr->manual_geom || dev_ptr->is_exist)
				return 0;
			msg_to_show = MSG_vnd_tpc_ask;
			out_var = &(dev_ptr->ntracks);
			break;
		case PMV_MENU_NCYLINDERS:
			if (!dev_ptr->manual_geom || dev_ptr->is_exist)
				return 0;
			msg_to_show = MSG_vnd_cyl_ask;
			out_var = &(dev_ptr->ncylinders);
			break;
		case PMV_MENU_REMOVE:
			dev_ptr->enabled = 0;
			return 0;
	}
	if (out_var == NULL || msg_to_show == NULL)
		return -1;
	snprintf(buf, SSTRSIZE, "%d", *out_var);
	msg_prompt_win(msg_to_show, -1, 18, 0, 0, buf, buf, SSTRSIZE);
	if (atoi(buf) >= 0)
		*out_var = atoi(buf);
	return 0;
}

static void
pm_vnd_init(void *arg, void *none)
{
	struct vnd_desc *dev_ptr = arg;
	memset(dev_ptr, 0, sizeof(*dev_ptr));
	*dev_ptr = (struct vnd_desc) {
		.enabled = 1,
		.blocked = 0,
		.filepath[0] = '\0',
		.is_exist = 0,
		.size = 1024,
		.readonly = 0,
		.manual_geom = 0,
		.secsize = 512,
		.nsectors = 18,
		.ntracks = 2,
		.ncylinders = 80
	};
}

static int
pm_vnd_check(void *arg)
{
	struct vnd_desc *dev_ptr = arg;

	if (dev_ptr->blocked)
		return 0;
	if (strlen(dev_ptr->filepath) < 1 ||
			dev_ptr->size < 1)
		dev_ptr->enabled = 0;
	else {
		pm_manage_getfreenode(&(dev_ptr->node), "vnd", &vnds_t_info);
		if (dev_ptr->node < 0)
			dev_ptr->enabled = 0;
	}
	return dev_ptr->enabled;
}

/* XXX: vndconfig always return 0? */
static int
pm_vnd_commit(void)
{
	int i, error;
	char r_o[3], buf[MOUNTLEN+3], resultpath[STRSIZE];
	const char *mpath, *mp_suit = NULL, *rp;
	struct pm_devs *pm_i, *pm_suit = NULL;
	part_id id, part_suit = NO_PART;
	struct disk_part_info info;

	for (i = 0; i < MAX_VND; i++) {
		error = 0;
		if (! pm_vnd_check(&vnds[i]))
			continue;

		/* Trying to assign target device */
		SLIST_FOREACH(pm_i, &pm_head, l) {
			for (id = 0; id < pm_i->parts->num_part; id++) {
				if (!pm_i->parts->pscheme->get_part_info(
				    pm_i->parts, id, &info))
					continue;
				if (info.flags & (PTI_SEC_CONTAINER|
				    PTI_WHOLE_DISK|PTI_PSCHEME_INTERNAL|
				    PTI_RAW_PART))
					continue;
				if (info.last_mounted == NULL ||
				    info.last_mounted[0] == 0)
					continue;
				mpath = info.last_mounted;
				strcpy(buf, mpath);
				if (buf[strlen(buf)-1] != '/')
					strcat(buf, "/");
				if (strstr(vnds[i].filepath, buf) !=
				    vnds[i].filepath)
					continue;
				if (part_suit == NO_PART || pm_suit == NULL ||
				    strlen(buf) > strlen(mp_suit)) {
					part_suit = id;
					pm_suit = pm_i;
					mp_suit = mpath;
				}
			}
		}
		if (part_suit == NO_PART || pm_suit == NULL ||
		   mp_suit == NULL)
			continue;

		/* Mounting assigned partition and try to get real file path*/
		if (pm_mount(pm_suit, part_suit) != 0)
			continue;
		rp = pm_get_mount(pm_suit, part_suit);
		snprintf(resultpath, STRSIZE, "%s/%s",
			rp,
			&(vnds[i].filepath[strlen(rp)]));

		strcpy(r_o, vnds[i].readonly?"-r":"");
		/* If this is a new image */
		if (!vnds[i].is_exist) {
			run_program(RUN_DISPLAY | RUN_PROGRESS, "mkdir -p %s ",
			    dirname(resultpath));
			if (error == 0)
				error = run_program(RUN_DISPLAY | RUN_PROGRESS,
				    "dd if=/dev/zero of=%s bs=1m "
				    "count=% " PRIi64 " progress=100 "
				    "msgfmt=human",
				    resultpath, vnds[i].size*(MEG/512));
		}
		if (error)
			continue;

		/* If this is a new image with manual geometry */
		if (!vnds[i].is_exist && vnds[i].manual_geom)
			error = run_program(RUN_DISPLAY | RUN_PROGRESS,
			    "vndconfig %s vnd%d %s %d %d %d %d",
			    r_o, vnds[i].node,
			    resultpath, vnds[i].secsize,
			    vnds[i].nsectors,
			    vnds[i].ntracks, vnds[i].ncylinders);
		else
			/* If this is a existing image or image without manual
			 * geometry */
			error = run_program(RUN_DISPLAY | RUN_PROGRESS,
			    "vndconfig %s vnd%d %s",
			    r_o, vnds[i].node, resultpath);

		if (error == 0) {
			vnds[i].blocked = 1;
			vnds[i].pm_part = part_suit;
			vnds[i].pm = pm_suit;
			vnds[i].pm->blocked++;
		}
	}
	return 0;
}

/***
 CGD
 ***/

static void
pm_cgd_menufmt(menudesc *m, int opt, void *arg)
{
	struct cgd_desc *dev_ptr = ((struct part_entry *)arg)[opt].dev_ptr;
	char desc[STRSIZE];
	struct disk_part_info info;

	if (dev_ptr->enabled == 0)
		return;
	if (dev_ptr->pm == NULL)
		wprintw(m->mw, "%s", msg_string(MSG_cgd_err_menufmt));
	else {
		snprintf(desc, sizeof desc, "cgd%d (%s-%d)",
		    dev_ptr->node, dev_ptr->enc_type, dev_ptr->key_size);
		dev_ptr->pm->parts->pscheme->get_part_info(dev_ptr->pm->parts,
		    dev_ptr->pm_part, &info);
		pm_fmt_disk_line(m->mw, desc, dev_ptr->pm_name,
		    info.size, NULL);
	}
}

static void
pm_cgd_edit_menufmt(menudesc *m, int opt, void *arg)
{
	struct cgd_desc *dev_ptr = arg;
	switch (opt) {
		case PMC_MENU_DEV:
			wprintw(m->mw, "%-15s: %s",
			    msg_string(MSG_cgd_dev_fmt), dev_ptr->pm_name);
			break;
		case PMC_MENU_ENCTYPE:
			wprintw(m->mw, "%-15s: %s",
			    msg_string(MSG_cgd_enc_fmt), dev_ptr->enc_type);
			break;
		case PMC_MENU_KEYSIZE:
			wprintw(m->mw, "%-15s: %d",
			    msg_string(MSG_cgd_key_fmt), dev_ptr->key_size);
			break;
		case PMC_MENU_IVTYPE:
			wprintw(m->mw, "%-15s: %s",
			    msg_string(MSG_cgd_iv_fmt), dev_ptr->iv_type);
			break;
		case PMC_MENU_KEYGENTYPE:
			wprintw(m->mw, "%-15s: %s",
			    msg_string(MSG_cgd_keygen_fmt), dev_ptr->keygen_type);
			break;
		case PMC_MENU_VERIFYTYPE:
			wprintw(m->mw, "%-15s: %s",
			    msg_string(MSG_cgd_verif_fmt), dev_ptr->verify_type);
			break;
	}
}

static int
pm_cgd_set_value(menudesc *m, void *arg)
{
	char *retstring;
	struct cgd_desc *dev_ptr = arg;

	switch (m->cursel) {
		case PMC_MENU_DEV:
			pm_cgd_disk_set(dev_ptr, NULL);
			return 0;
		case PMC_MENU_ENCTYPE:
			process_menu(MENU_cgd_enctype, &retstring);
			dev_ptr->enc_type = retstring;
			if (! strcmp(retstring, "aes-xts"))
				dev_ptr->key_size = 256;
			if (! strcmp(retstring, "aes-cbc"))
				dev_ptr->key_size = 192;
			if (! strcmp(retstring, "blowfish-cbc"))
				dev_ptr->key_size = 128;
			if (! strcmp(retstring, "3des-cbc"))
				dev_ptr->key_size = 192;
			return 0;
		case PMC_MENU_KEYSIZE:
			if (! strcmp(dev_ptr->enc_type, "aes-xts"))
				dev_ptr->key_size +=
					(dev_ptr->key_size < 512)? 256 : -256;
			if (! strcmp(dev_ptr->enc_type, "aes-cbc"))
				dev_ptr->key_size +=
					(dev_ptr->key_size < 256)? 64 : -128;
			if (! strcmp(dev_ptr->enc_type, "blowfish-cbc"))
				dev_ptr->key_size = 128;
			if (! strcmp(dev_ptr->enc_type, "3des-cbc"))
				dev_ptr->key_size = 192;
			return 0;
		case PMC_MENU_IVTYPE:
			process_menu(MENU_cgd_ivtype, &retstring);
			dev_ptr->iv_type = retstring;
			return 0;
		case PMC_MENU_KEYGENTYPE:
			process_menu(MENU_cgd_keygentype, &retstring);
			dev_ptr->keygen_type = retstring;
			return 0;
		case PMC_MENU_VERIFYTYPE:
			process_menu(MENU_cgd_verifytype, &retstring);
			dev_ptr->verify_type = retstring;
			return 0;
		case PMC_MENU_REMOVE:
			dev_ptr->enabled = 0;
			return 0;
	}
	return -1;
}

static void
pm_cgd_init(void *arg1, void *arg2)
{
	struct cgd_desc *dev_ptr = arg1;
	struct part_entry *disk_entrie = arg2;

	memset(dev_ptr, 0, sizeof(*dev_ptr));
	*dev_ptr = (struct cgd_desc) {
		.enabled = 1,
		.blocked = 0,
		.pm = NULL,
		.pm_name[0] = '\0',
		.pm_part = 0,
		.keygen_type = "pkcs5_pbkdf2/sha1",
		.verify_type = "disklabel",
		.enc_type = "aes-xts",
		.iv_type = "encblkno1",
		.key_size = 256,
	};
	if (disk_entrie != NULL) {
		disk_entrie->parts->pscheme->get_part_device(
		    disk_entrie->parts,  disk_entrie->id,
		    disk_entrie->fullname, sizeof(disk_entrie->fullname),
		    NULL, logical_name, false, true);
		pm_cgd_disk_set(dev_ptr, disk_entrie);
	}
}

static int
pm_cgd_check(void *arg)
{
	struct cgd_desc *dev_ptr = arg;

	if (dev_ptr->blocked)
		return 0;
	if (dev_ptr->pm == NULL)
		dev_ptr->enabled = 0;
	else {
		pm_manage_getfreenode(&(dev_ptr->node), "cgd", &cgds_t_info);
		if (dev_ptr->node < 0)
			dev_ptr->enabled = 0;
	}
	return dev_ptr->enabled;
}

static int
pm_cgd_disk_set(struct cgd_desc *dev_ptr, struct part_entry *disk_entrie)
{
	int alloc_disk_entrie = 0;

	if (disk_entrie == NULL) {
		alloc_disk_entrie = 1;
		disk_entrie = malloc (sizeof(struct part_entry));
		if (disk_entrie == NULL)
			return -2;
		*disk_entrie = pm_dev_list(PM_CGD);
		if (pm_retvalue < 0) {
			free(disk_entrie);
			return -1;
		}
	}
	dev_ptr->pm = disk_entrie->dev_ptr;
	dev_ptr->pm_part = disk_entrie->id;
	strlcpy(dev_ptr->pm_name, disk_entrie->fullname, SSTRSIZE);

	if (alloc_disk_entrie)
		free(disk_entrie);
	return 0;
}

int
pm_cgd_edit_new(struct pm_devs *mypm, part_id id)
{
	struct part_entry pe = { .id = id, .parts = mypm->parts,
	    .dev_ptr = mypm, .type = PM_CGD };

	return pm_edit(PMC_MENU_END, pm_cgd_edit_menufmt,
		pm_cgd_set_value, pm_cgd_check, pm_cgd_init,
		&pe, NULL, 0, &cgds_t_info);
}

int
pm_cgd_edit_old(struct part_entry *pe)
{
	return pm_edit(PMC_MENU_END, pm_cgd_edit_menufmt,
		pm_cgd_set_value, pm_cgd_check, pm_cgd_init,
		pe->dev_ptr != NULL ? pe : NULL,
		pe->dev_ptr, 0, &cgds_t_info);
}

static int
pm_cgd_commit(void)
{
	char devname[STRSIZE];
	int i, error = 0;

	for (i = 0; i < MAX_CGD; i++) {
		if (! pm_cgd_check(&cgds[i]))
			continue;
		if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"cgdconfig -g -V %s -i %s -k %s -o /tmp/cgd.%d.conf"
			" %s %d", cgds[i].verify_type,
			cgds[i].iv_type, cgds[i].keygen_type, cgds[i].node,
			cgds[i].enc_type, cgds[i].key_size) != 0) {
			error++;
			continue;
		}
		if (cgds[i].pm != NULL && cgds[i].pm->parts != NULL) {
			/* wedge device names may have changed */
			cgds[i].pm->parts->pscheme->get_part_device(
			    cgds[i].pm->parts, cgds[i].pm_part,
			    devname, sizeof devname, NULL,
			    logical_name, true, true);
			cgds[i].pm->parts->pscheme->get_part_device(
			    cgds[i].pm->parts, cgds[i].pm_part,
			    cgds[i].pm_name, sizeof cgds[i].pm_name, NULL,
			    plain_name, false, true);
		} else {
			continue;
		}
		if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"cgdconfig -V re-enter cgd%d '%s' /tmp/cgd.%d.conf",
			cgds[i].node, devname, cgds[i].node) != 0) {
			error++;
			continue;
		}
		cgds[i].pm->blocked++;
		cgds[i].blocked = 1;
	}
	return error;
}

/***
 LVM
 ***/

/* Add lvm logical volumes to pm list */
/* XXX: rewrite */
static void
pm_lvm_find(void)
{
	int i, ii, already_found;
	char dev[STRSIZE];
	struct pm_devs *pm_i;

	for (i = 0; i < MAX_LVM_VG; i++) {
		if (! lvms[i].blocked)
			continue;
		for (ii = 0; ii < MAX_LVM_LV; ii++) {
			if (! lvms[i].lv[ii].blocked || lvms[i].lv[ii].size < 1)
				continue;
			snprintf(dev, STRSIZE, "%s/%s", lvms[i].name,
			    lvms[i].lv[ii].name);
			already_found = 0;
			SLIST_FOREACH(pm_i, &pm_head, l)
				if (!already_found && strcmp(pm_i->diskdev,
				    dev) == 0) {
					pm_i->found = 1;
					already_found = 1;
				}
			if (already_found)
				/* We already added this device, skipping */
				continue;
			pm_new->found = 1;
			pm_new->ptstart = 0;
			pm_new->ptsize = 0;
			pm_new->no_part = true;
			pm_new->refdev = &lvms[i].lv[ii];
			pm_new->sectorsize = 1;
			pm_new->dlcylsize = MEG;
			strlcpy(pm_new->diskdev, dev, SSTRSIZE);
			strlcpy(pm_new->diskdev_descr, dev, STRSIZE);

			if (SLIST_EMPTY(&pm_head))
				 SLIST_INSERT_HEAD(&pm_head, pm_new, l);
			else
				 SLIST_INSERT_AFTER(pm_i, pm_new, l);
			pm_new = malloc(sizeof (struct pm_devs));
			memset(pm_new, 0, sizeof *pm_new);
		}
	}
}

static int
pm_lvm_disk_add(menudesc *m, void *arg)
{
	int i;
	lvms_t *dev_ptr = arg;
	struct part_entry disk_entrie = pm_dev_list(PM_LVM);
	if (pm_retvalue < 0)
		return pm_retvalue;

	for (i = 0; i < MAX_LVM_PV; i++) {
		if (dev_ptr->pv[i].pm == NULL) {
			dev_ptr->pv[i].pm = disk_entrie.dev_ptr;
			dev_ptr->pv[i].pm_part = disk_entrie.id;
			strlcpy(dev_ptr->pv[i].pm_name, disk_entrie.fullname,
			    sizeof(dev_ptr->pv[i].pm_name));
			break;
		}
	}
	pm_retvalue = 1;
	return 0;
}

static int
pm_lvm_disk_del(menudesc *m, void *arg)
{
	int retvalue = -1, num_devs = 0;
	size_t i;
	int menu_no;
	lvms_t *dev_ptr = arg;
	menu_ent menu_entries[MAX_LVM_PV];
	struct part_entry submenu_args[MAX_LVM_PV];

	for (i = 0; i < MAX_LVM_PV; i++) {
		if (dev_ptr->pv[i].pm == NULL)
			continue;
		menu_entries[num_devs] = (struct menu_ent) {
			.opt_name = dev_ptr->pv[i].pm_name,
			.opt_action = set_menu_select,
			.opt_flags = OPT_EXIT,
		};
		submenu_args[num_devs].index = i;
		num_devs++;
	}

	menu_no = new_menu(MSG_lvm_disks,
		menu_entries, num_devs, -1, -1,
		(num_devs+1<3)?3:num_devs+1, 13,
		MC_SCROLL | MC_NOCLEAR, NULL, NULL, NULL, NULL, MSG_cancel);
	if (menu_no == -1)
		return -1;
	process_menu(menu_no, &retvalue);
	free_menu(menu_no);

	if (retvalue < 0 || retvalue >= num_devs)
		return -1;

	dev_ptr->pv[submenu_args[retvalue].index].pm = NULL;

	return 0;
}

static void
pm_lvm_menufmt(menudesc *m, int opt, void *arg)
{
	int i, ok = 0;
	daddr_t used_size = 0;
	char buf1[STRSIZE]; buf1[0] = 0;
	char buf2[STRSIZE]; buf2[0] = 0;
	char devs[STRSIZE]; devs[0] = 0;
	lvms_t *dev_ptr = ((struct part_entry *)arg)[opt].dev_ptr;

	if (dev_ptr->enabled == 0)
		return;
	snprintf(buf1, STRSIZE, "VG '%s'", dev_ptr->name);
	for (i = 0; i < MAX_LVM_PV; i++)
		if (dev_ptr->pv[i].pm != NULL) {
			strlcat(devs, dev_ptr->pv[i].pm_name, STRSIZE);
			strlcat(devs, " ", STRSIZE);
			ok = 1;
		}
	for (i = 0; i < MAX_LVM_LV; i++)
		used_size += dev_ptr->lv[i].size;
	if (ok) {
		snprintf(buf2, STRSIZE, "%" PRIi64 "/%" PRIi64,
			dev_ptr->total_size - used_size, dev_ptr->total_size);
		pm_fmt_disk_line(m->mw, buf1, devs, 0, buf2);
	} else {
		pm_fmt_disk_line(m->mw, buf1, NULL, 0,
		    msg_string(MSG_lvm_err_menufmt));
	}
}

static void
pm_lvm_edit_menufmt(menudesc *m, int opt, void *arg)
{
	int i;
	char buf[STRSIZE];
	lvms_t *dev_ptr = arg;
	strlcpy(buf, msg_string(MSG_auto), sizeof buf);

	switch (opt) {
		case PML_MENU_PV:
			buf[0] = '\0';
			for (i = 0; i < MAX_LVM_PV; i++) {
				if (dev_ptr->pv[i].pm != NULL) {
					strlcat(buf, " ", sizeof buf);
					strlcat(buf, dev_ptr->pv[i].pm_name,
					    sizeof buf);
				}
			}
			wprintw(m->mw, "%-20s: %s",
			     msg_string(MSG_lvm_disks_fmt), buf);
			break;
		case PML_MENU_NAME:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvm_name_fmt), dev_ptr->name);
			break;
		case PML_MENU_MAXLOGICALVOLUMES:
			if (dev_ptr->maxlogicalvolumes > 0)
				snprintf(buf, STRSIZE, "%d",
				    dev_ptr->maxlogicalvolumes);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvm_maxlv_fmt), buf);
			break;
		case PML_MENU_MAXPHYSICALVOLUMES:
			if (dev_ptr->maxphysicalvolumes > 0)
				snprintf(buf, STRSIZE, "%d",
				    dev_ptr->maxphysicalvolumes);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvm_maxpv_fmt), buf);
			break;
		case PML_MENU_PHYSICALEXTENTSIZE:
			if (dev_ptr->physicalextentsize > 0)
				snprintf(buf, STRSIZE, "%dM",
				    dev_ptr->physicalextentsize);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvm_extsiz_fmt), buf);
			break;
	}
}

static int
pm_lvm_set_value(menudesc *m, void *arg)
{
	char buf[STRSIZE];
	const char *msg_to_show = NULL;
	int *out_var = NULL;
	lvms_t *dev_ptr = arg;

	static menu_ent menuent_disk_adddel[] = {
	    { .opt_name=MSG_add, .opt_flags=OPT_EXIT,
	      .opt_action=pm_lvm_disk_add },
	    { .opt_name=MSG_remove, .opt_flags=OPT_EXIT,
	      .opt_action=pm_lvm_disk_del }
	};
	static int menu_disk_adddel = -1;
	if (menu_disk_adddel == -1) {
		menu_disk_adddel = new_menu(NULL, menuent_disk_adddel,
			__arraycount(menuent_disk_adddel),
			-1, -1, 0, 10, MC_NOCLEAR, NULL, NULL, NULL, NULL,
			MSG_cancel);
	}

	switch (m->cursel) {
		case PML_MENU_PV:
			process_menu(menu_disk_adddel, arg);
			return 0;
		case PML_MENU_NAME:
			msg_prompt_win(MSG_lvm_name_ask, -1, 18, 0, 0,
			    dev_ptr->name, dev_ptr->name, SSTRSIZE);
			return 0;
		case PML_MENU_MAXLOGICALVOLUMES:
			msg_to_show = MSG_lvm_maxlv_ask;
			out_var = &(dev_ptr->maxlogicalvolumes);
			break;
		case PML_MENU_MAXPHYSICALVOLUMES:
			msg_to_show = MSG_lvm_maxpv_ask;
			out_var = &(dev_ptr->maxphysicalvolumes);
			break;
		case PML_MENU_PHYSICALEXTENTSIZE:
			msg_to_show = MSG_lvm_extsiz_ask;
			out_var = &(dev_ptr->physicalextentsize);
			break;
		case PML_MENU_REMOVE:
			dev_ptr->enabled = 0;
			return 0;
	}
	if (out_var == NULL || msg_to_show == NULL)
		return -1;
	snprintf(buf, SSTRSIZE, "%d", *out_var);
	msg_prompt_win(msg_to_show, -1, 18, 0, 0, buf, buf, SSTRSIZE);
	if (atoi(buf) >= 0)
		*out_var = atoi(buf);
	return 0;
}

static void
pm_lvm_init(void *arg, void* none)
{
	lvms_t *dev_ptr = arg;

	memset(dev_ptr, 0, sizeof *dev_ptr);
	*dev_ptr = (struct lvms_t) {
		.enabled = 1,
		.blocked = 0,
		.maxlogicalvolumes = MAX_LVM_PV,
		.maxphysicalvolumes = MAX_LVM_LV,
		.physicalextentsize = -1,
	};
	sprintf(dev_ptr->name, "vg%.2d", rand()%100);
}

static int
pm_lvm_check(void *arg)
{
	size_t i;
	bool ok = false;
	lvms_t *dev_ptr = arg;
	dev_ptr->total_size = 0;
	struct disk_part_info info;

	for (i = 0; i < MAX_LVM_PV; i++) {
		if (dev_ptr->pv[i].pm != NULL) {
			if (!dev_ptr->pv[i].pm->parts->pscheme->get_part_info(
			    dev_ptr->pv[i].pm->parts, dev_ptr->pv[i].pm_part,
			    &info))
				continue;
			ok = 1;
			dev_ptr->total_size += info.size;
		}
	}
	if (!ok)
		dev_ptr->enabled = 0;
	return dev_ptr->enabled;
}

static void
pm_lvmlv_menufmt(menudesc *m, int opt, void *arg)
{
	char buf[STRSIZE];
	lv_t *dev_ptr = ((struct part_entry *)arg)[opt].dev_ptr;

	if (dev_ptr->size > 0) {
		snprintf(buf, STRSIZE, "'%s'", dev_ptr->name);
		pm_fmt_disk_line(m->mw, buf, NULL, dev_ptr->size, NULL);
	}
}

static void
pm_lvmlv_edit_menufmt(menudesc *m, int opt, void *arg)
{

	lv_t *dev_ptr = arg;
	char buf[STRSIZE];
	strlcpy(buf, msg_string(MSG_auto), STRSIZE);

	switch (opt) {
		case PMLV_MENU_NAME:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_name_fmt), dev_ptr->name);
			break;
		case PMLV_MENU_SIZE:
			wprintw(m->mw, "%-20s: %" PRIi64,
			    msg_string(MSG_lvmlv_size_fmt), dev_ptr->size);
			break;
		case PMLV_MENU_READONLY:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_ro_fmt),
			    dev_ptr->readonly? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMLV_MENU_CONTIGUOUS:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_cont_fmt),
			    dev_ptr->contiguous? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMLV_MENU_EXTENTS:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_extnum_fmt),
			    (strlen(dev_ptr->extents) > 0) ?
			    dev_ptr->extents : msg_string(MSG_auto));
			break;
		case PMLV_MENU_MINOR:
			if (dev_ptr->minor > 0)
				snprintf(buf, STRSIZE, "%dK", dev_ptr->minor);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_minor_fmt), buf);
			break;
		case PMLV_MENU_MIRRORS:
			wprintw(m->mw, "%-20s: %d",
			    msg_string(MSG_lvmlv_mirrors_fmt),
			    dev_ptr->mirrors);
			break;
		case PMLV_MENU_REGIONSIZE:
			if (dev_ptr->regionsize > 0)
				snprintf(buf, STRSIZE, "%dM",
				    dev_ptr->regionsize);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_regsiz_fmt), buf);
			break;
		case PMLV_MENU_PERSISTENT:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_pers_fmt),
			    dev_ptr->persistent ?
			    msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMLV_MENU_READAHEAD:
			if (dev_ptr->readahead > 0)
				snprintf(buf, STRSIZE, "%d",
				    dev_ptr->readahead);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_readahsect_fmt), buf);
			break;
		case PMLV_MENU_STRIPES:
			if (dev_ptr->stripes > 0)
				snprintf(buf, STRSIZE, "%d", dev_ptr->stripes);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_stripes_fmt), buf);
			break;
		case PMLV_MENU_STRIPESIZE:
			if (dev_ptr->stripesize > 0)
				snprintf(buf, STRSIZE, "%dK", dev_ptr->stripesize);
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_stripesiz_fmt), buf);
			break;
		case PMLV_MENU_ZERO:
			wprintw(m->mw, "%-20s: %s",
			    msg_string(MSG_lvmlv_zero_fmt),
			    dev_ptr->zero ?
			    msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
	}
}

static int
pm_lvmlv_set_value(menudesc *m, void *arg)
{
	char buf[STRSIZE];
	const char *msg_to_show = NULL;
	int *out_var = NULL;
	lv_t *dev_ptr = arg;

	switch (m->cursel) {
		case PMLV_MENU_NAME:
			msg_prompt_win(MSG_lvmlv_name_ask, -1, 18, 0, 0,
			    dev_ptr->name, dev_ptr->name, SSTRSIZE);
			return 0;
		case PMLV_MENU_SIZE:
			pm_edit_size_value(MSG_lvmlv_size_ask,
			    pm->sectorsize, pm->dlcylsize,
			    &dev_ptr->size); /* XXX cylsize? */
			break;
		case PMLV_MENU_READONLY:
			dev_ptr->readonly = !dev_ptr->readonly;
			return 0;
		case PMLV_MENU_CONTIGUOUS:
			dev_ptr->contiguous = !dev_ptr->contiguous;
			return 0;
		case PMLV_MENU_EXTENTS:
			msg_prompt_win(MSG_lvmlv_extnum_ask, -1, 18, 0, 0,
			    dev_ptr->extents, dev_ptr->extents, SSTRSIZE);
			return 0;
		case PMLV_MENU_MINOR:
			msg_to_show = MSG_lvmlv_minor_ask;
			out_var = &(dev_ptr->minor);
			break;
		case PMLV_MENU_MIRRORS:
			msg_to_show = MSG_lvmlv_mirrors_ask;
			out_var = &(dev_ptr->mirrors);
			break;
		case PMLV_MENU_REGIONSIZE:
			msg_to_show = MSG_lvmlv_regsiz_ask;
			out_var = &(dev_ptr->regionsize);
			break;
		case PMLV_MENU_PERSISTENT:
			dev_ptr->persistent = !dev_ptr->persistent;
			return 0;
		case PMLV_MENU_READAHEAD:
			msg_to_show = MSG_lvmlv_readahsect_ask;
			out_var = &(dev_ptr->readahead);
			break;
		case PMLV_MENU_STRIPES:
			msg_to_show = MSG_lvmlv_stripes_ask;
			out_var = &(dev_ptr->stripes);
			break;
		case PMLV_MENU_STRIPESIZE:
			if (dev_ptr->stripesize << 1 > 512)
				dev_ptr->stripesize = 4;
			else
				dev_ptr->stripesize <<= 1;
			return 0;
		case PMLV_MENU_ZERO:
			dev_ptr->zero = !dev_ptr->zero;
			return 0;
		case PMLV_MENU_REMOVE:
			dev_ptr->size = 0;
			return 0;
	}
	if (out_var == NULL || msg_to_show == NULL)
		return -1;
	snprintf(buf, SSTRSIZE, "%d", *out_var);
	msg_prompt_win(msg_to_show, -1, 18, 0, 0, buf, buf, SSTRSIZE);
	if (atoi(buf) >= 0)
		*out_var = atoi(buf);
	return 0;
}

static void
pm_lvmlv_init(void *arg, void *none)
{
	lv_t *dev_ptr = arg;
	memset(dev_ptr, 0, sizeof *(dev_ptr));
	*dev_ptr = (struct lv_t) {
		.blocked = 0,
		.size = 1024,
		.stripesize = 64,
	};
	sprintf (dev_ptr->name, "lvol%.2d", rand()%100);
}

static int
pm_lvmlv_check(void *arg)
{
	lv_t *dev_ptr = arg;
	if (dev_ptr->size > 0 && strlen(dev_ptr->name) > 0)
		return 1;
	else {
		dev_ptr->size = 0;
		return 0;
	}
}

static int
pm_lvm_commit(void)
{
	int i, ii, error;
	uint used_size = 0;
	char params[STRSIZE*3], devs[STRSIZE*3], arg[STRSIZE];

	for (i = 0; i < MAX_LVM_VG; i++) {
		/* Stage 0: checks */
		if (! pm_lvm_check(&lvms[i]))
			continue;
		for (ii = 0; ii < MAX_LVM_LV; ii++)
			used_size += lvms[i].lv[ii].size;
		if (used_size > lvms[i].total_size)
			continue;

		params[0] = '\0';
		devs[0] = '\0';
		error = 0;
		/* Stage 1: creating Physical Volumes (PV's) */
		for (ii = 0; ii < MAX_LVM_PV && ! error; ii++)
			if (lvms[i].pv[ii].pm != NULL) {
				run_program(RUN_SILENT | RUN_ERROR_OK,
				    "lvm pvremove -ffy /dev/r%s",
				    (char*)lvms[i].pv[ii].pm_name);
				error += run_program(RUN_DISPLAY | RUN_PROGRESS,
				    "lvm pvcreate -ffy /dev/r%s",
				    (char*)lvms[i].pv[ii].pm_name);
				if (error)
					break;
				strlcat(devs, " /dev/r", sizeof devs);
				strlcat(devs, lvms[i].pv[ii].pm_name,
				    sizeof devs);
			}
		if (error)
			continue;
		/* Stage 2: creating Volume Groups (VG's) */
		if (lvms[i].maxlogicalvolumes > 0) {
			snprintf(arg, sizeof arg, " -l %d",
			    lvms[i].maxlogicalvolumes);
			strlcat(params, arg, sizeof params);
		}
		if (lvms[i].maxphysicalvolumes > 0) {
			snprintf(arg, sizeof arg, " -p %d",
			    lvms[i].maxphysicalvolumes);
			strlcat(params, arg, sizeof params);
		}
		if (lvms[i].physicalextentsize > 0) {
			snprintf(arg, sizeof arg, " -s %d",
			    lvms[i].physicalextentsize);
			strlcat(params, arg, sizeof params);
		}
		error += run_program(RUN_DISPLAY | RUN_PROGRESS,
		    "lvm vgcreate %s %s %s", params, lvms[i].name, devs);
		if (error)
			continue;
		/* Stage 3: creating Logical Volumes (LV's) */
		for (ii = 0; ii < MAX_LVM_LV; ii++) {
			if (lvms[i].lv[ii].size <= 0)
				continue;

			params[0] = '\0';
			snprintf(arg, sizeof arg, " -C %c",
			    lvms[i].lv[ii].contiguous?'y':'n');
			strlcat(params, arg, sizeof params);
			snprintf(arg, sizeof arg, " -M %c",
			    lvms[i].lv[ii].persistent?'y':'n');
			strlcat(params, arg, sizeof params);
			snprintf(arg, sizeof arg, " -p %s",
			    lvms[i].lv[ii].readonly?"r":"rw");
			strlcat(params, arg, sizeof params);
			snprintf(arg, sizeof arg, " -Z %c",
			    lvms[i].lv[ii].zero?'y':'n');
			strlcat(params, arg, sizeof params);
			if (strlen(lvms[i].lv[ii].name) > 0) {
				snprintf(arg, sizeof arg, " -n %s",
				    lvms[i].lv[ii].name);
				strlcat(params, arg, sizeof params);
			}
			if (strlen(lvms[i].lv[ii].extents) > 0) {
				snprintf(arg, sizeof arg, " -l %s",
				    lvms[i].lv[ii].extents);
				strlcat(params, arg, sizeof params);
			}
			if (lvms[i].lv[ii].minor > 0) {
				snprintf(arg, sizeof arg, " --minor %d",
				    lvms[i].lv[ii].minor);
				strlcat(params, arg, sizeof params);
			}
			if (lvms[i].lv[ii].mirrors > 0) {
				snprintf(arg, sizeof arg, " -m %d",
				    lvms[i].lv[ii].mirrors);
				strlcat(params, arg, sizeof params);
				if (lvms[i].lv[ii].regionsize > 0) {
					snprintf(arg, sizeof arg, " -R %d",
					     lvms[i].lv[ii].regionsize);
					strlcat(params, arg, sizeof params);
				}
			}
			if (lvms[i].lv[ii].readahead > 0) {
				snprintf(arg, sizeof arg, " -r %d",
				    lvms[i].lv[ii].readahead);
				strlcat(params, arg, sizeof params);
			}
			if (lvms[i].lv[ii].stripes > 0) {
				snprintf(arg, sizeof arg, " -i %d",
				    lvms[i].lv[ii].stripes);
				strlcat(params, arg, sizeof params);
				if (lvms[i].lv[ii].stripesize > 0) {
					snprintf(arg, sizeof arg, " -I %d",
					    lvms[i].lv[ii].stripesize);
					strlcat(params, arg, sizeof params);
				}
			}
			snprintf(arg, sizeof arg, " -L %" PRIi64 "M",
			    lvms[i].lv[ii].size);
			strlcat(params, arg, sizeof params);

			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
			    "lvm lvcreate %s %s", params, lvms[i].name);
		}
		if (! error) {
			lvms[i].blocked = 1;
			for (ii = 0; ii < MAX_LVM_PV; ii++)
				if (lvms[i].pv[ii].pm != NULL)
					lvms[i].pv[ii].pm->blocked++;
			for (ii = 0; ii < MAX_LVM_LV; ii++)
				if (lvms[i].lv[ii].size > 0)
					lvms[i].lv[ii].blocked = 1;
		}
	}

	return 0;
}

/***
 Partman generic functions
 ***/

int
pm_getrefdev(struct pm_devs *pm_cur)
{
	int i, ii, dev_num, num_devs, num_devs_s;
	char descr[MENUSTRSIZE], dev[MENUSTRSIZE] = "";

	pm_cur->refdev = NULL;
	if (! strncmp(pm_cur->diskdev, "cgd", 3)) {
		dev_num = pm_cur->diskdev[3] - '0';
		for (i = 0; i < MAX_CGD; i++)
			if (cgds[i].blocked && cgds[i].node == dev_num) {
				pm_cur->refdev = &cgds[i];
				snprintf(descr, sizeof descr,
				    " (%s, %s-%d)", cgds[i].pm_name,
				    cgds[i].enc_type, cgds[i].key_size);
				strlcat(pm_cur->diskdev_descr, descr,
				    sizeof(pm_cur->diskdev_descr));
				break;
			}
 	} else if (! strncmp(pm_cur->diskdev, "vnd", 3)) {
 		dev_num = pm_cur->diskdev[3] - '0';
 		for (i = 0; i < MAX_VND; i++)
			if (vnds[i].blocked && vnds[i].node == dev_num) {
				pm_cur->refdev = &vnds[i];
				vnds[i].pm->parts->pscheme->get_part_device(
				    vnds[i].pm->parts, vnds[i].pm_part,
				    dev, sizeof dev, NULL, plain_name, false,
				    true);
				snprintf(descr, sizeof descr, " (%s, %s)",
				    dev, vnds[i].filepath);
				strlcat(pm_cur->diskdev_descr, descr,
				    sizeof(pm_cur->diskdev_descr));
				break;
			}
	} else if (! strncmp(pm_cur->diskdev, "raid", 4)) {
		dev_num = pm_cur->diskdev[4] - '0';
 		for (i = 0; i < MAX_RAID; i++)
			if (raids[i].blocked && raids[i].node == dev_num) {
				pm_cur->refdev = &raids[i];
				num_devs = 0; num_devs_s = 0;
				for (ii = 0; ii < MAX_IN_RAID; ii++)
					if (raids[i].comp[ii].parts != NULL) {
						if(raids[i].comp[ii].is_spare)
							num_devs_s++;
						else
							num_devs++;
					}
				snprintf(descr, sizeof descr,
				    " (lvl %d, %d disks, %d spare)",
				    raids[i].raid_level, num_devs,
				    num_devs_s);
				strlcat(pm_cur->diskdev_descr, descr,
				    sizeof(pm_cur->diskdev_descr));
				break;
			}
	} else
		return -1;
	return 0;
}

/*
 * Enable/disable items in the extended partition disk/partition action
 * menu
 */
void
pmdiskentry_enable(menudesc *menu, struct part_entry *pe)
{
	int i;
	menu_ent *m;
	bool enable;

	for (i = 0; i < menu->numopts; i++) {
		m = &menu->opts[i];

		enable = false;
		if (m->opt_name == MSG_unconfig) {
			if (pe->type == PM_DISK)
				enable = ((struct pm_devs *)pe->dev_ptr)
				    ->refdev != NULL;
		} else if (m->opt_name == MSG_undo) {
			if (pe->type != PM_DISK)
				continue;
			enable = ((struct pm_devs *)pe->dev_ptr)->unsaved;
		} else if (m->opt_name == MSG_switch_parts) {
			enable = pm_from_pe(pe)->parts != NULL;
		} else {
			continue;
		}

		if (enable)
			m->opt_flags &= ~OPT_IGNORE;
		else
			m->opt_flags |= OPT_IGNORE;
	}
}

/* Detect that partition is in use */
int
pm_partusage(struct pm_devs *pm_cur, int part_num, int do_del)
{
	int i, ii, retvalue = 0;
	struct disk_part_info info;
	part_id id;

	if (pm_cur->parts == NULL)
		return -1;	/* nothing can be in use */

	if (part_num < 0) {
		/* Check all partitions on device */
		for (id = 0; id < pm_cur->parts->num_part; id++) {
			if (!pm_cur->parts->pscheme->get_part_info(
			    pm_cur->parts, id, &info))
				continue;
			if (info.flags & (PTI_SEC_CONTAINER|
			    PTI_WHOLE_DISK|PTI_PSCHEME_INTERNAL|
			    PTI_RAW_PART))
				continue;
			retvalue += pm_partusage(pm_cur, id, do_del);
		}
		return retvalue;
	}

	id = (part_id)part_num;
	if (id >= pm_cur->parts->num_part)
		return 0;

	for (i = 0; i < MAX_CGD; i++)
		if (cgds[i].enabled &&
			cgds[i].pm == pm_cur &&
			cgds[i].pm_part == id) {
			if (do_del) {
				cgds[i].pm = NULL;
				cgds[i].pm_name[0] = '\0';
			}
			return 1;
		}
	for (i = 0; i < MAX_RAID; i++)
		for (ii = 0; ii < MAX_IN_RAID; ii++)
			if (raids[i].enabled &&
				raids[i].comp[ii].parts == pm_cur->parts &&
				raids[i].comp[ii].id == id) {
					if (do_del)
						raids[i].comp[ii].parts = NULL;
					return 1;
			}
	for (i = 0; i < MAX_LVM_VG; i++)
		for (ii = 0; ii < MAX_LVM_PV; ii++)
			if (lvms[i].enabled &&
				lvms[i].pv[ii].pm == pm_cur &&
				lvms[i].pv[ii].pm_part == id) {
					if (do_del)
						lvms[i].pv[ii].pm = NULL;
					return 1;
			}

	return 0;
}

static void
pm_destroy_one(struct pm_devs *pm_i)
{
	part_id i;

	if (pm_i->parts != NULL) {
		if (pm_i->mounted != NULL) {
			for (i = 0; i < pm_i->parts->num_part; i++)
				free(pm_i->mounted[i]);
		}

		pm_i->parts->pscheme->free(pm_i->parts);
	}
	free(pm_i);
}

/* Clean up removed devices */
static int
pm_clean(void)
{
	int count = 0;
	struct pm_devs *pm_i, *tmp;

	SLIST_FOREACH_SAFE(pm_i, &pm_head, l, tmp)
		if (! pm_i->found) {
			count++;
			SLIST_REMOVE(&pm_head, pm_i, pm_devs, l);
			pm_destroy_one(pm_i);
		}
	return count;
}

/* Free all pm storage */
void
pm_destroy_all(void)
{
	struct pm_devs *pm_i, *tmp;

	if (pm_new != pm)
		pm_destroy_one(pm_new);
	SLIST_FOREACH_SAFE(pm_i, &pm_head, l, tmp) {
		SLIST_REMOVE(&pm_head, pm_i, pm_devs, l);
		pm_destroy_one(pm_i);
	}
}

void
pm_set_lvmpv(struct pm_devs *my_pm, part_id pno, bool add)
{
	size_t i;
	struct disk_part_info info;

	if (!my_pm->parts->pscheme->get_part_info(my_pm->parts, pno, &info))
		return;

	if (add) {
		for (i = 0; i < __arraycount(lvm_pvs); i++)
			if (lvm_pvs[i].pm == NULL && lvm_pvs[i].start == 0)
				break;
		if (i >= __arraycount(lvm_pvs))
			return;
		lvm_pvs[i].pm = my_pm;
		lvm_pvs[i].start = info.start;
		return;
	} else {
		for (i = 0; i < __arraycount(lvm_pvs); i++)
			if (lvm_pvs[i].pm == my_pm &&
			    lvm_pvs[i].start == info.start)
				break;
		if (i >= __arraycount(lvm_pvs))
			return;
		lvm_pvs[i].pm = NULL;
		lvm_pvs[i].start = 0;
	}
}

bool
pm_is_lvmpv(struct pm_devs *my_pm, part_id id,
    const struct disk_part_info *info)
{
	size_t i;

	for (i = 0; i < __arraycount(lvm_pvs); i++) {
		if (lvm_pvs[i].pm != my_pm)
			continue;
		if (lvm_pvs[i].start == info->start)
			return true;
	}

	return false;
}

void
pm_setfstype(struct pm_devs *pm_cur, part_id id, int fstype, int fs_subtype)
{
	struct disk_part_info info;

	if (!pm_cur->parts->pscheme->get_part_info(pm_cur->parts, id, &info))
		return;

	info.nat_type = pm_cur->parts->pscheme->get_fs_part_type(PT_root,
	    fstype, fs_subtype);
	if (info.nat_type == NULL)
		return;
	info.fs_type = fstype;
	info.fs_sub_type = fs_subtype;
	pm_cur->parts->pscheme->set_part_info(pm_cur->parts, id, &info, NULL);
}

static void
pm_select(struct pm_devs *pm_devs_in)
{
	pm = pm_devs_in;
	if (logfp)
		(void)fprintf(logfp, "Partman device: %s\n", pm->diskdev);
}

void
pm_rename(struct pm_devs *pm_cur)
{
#if 0	// XXX - convert to custom attribute or handle similar
	pm_select(pm_cur);
	msg_prompt_win(MSG_packname, -1, 18, 0, 0, pm_cur->bsddiskname,
		pm_cur->bsddiskname, sizeof pm_cur->bsddiskname);
#ifndef NO_DISKLABEL
	(void) savenewlabel(pm_cur->bsdlabel, MAXPARTITIONS);
#endif
#endif
}

int
pm_editpart(int part_num)
{
	struct partition_usage_set pset = {};

	usage_set_from_parts(&pset, pm->parts);
	edit_ptn(&(struct menudesc){.cursel = part_num}, &pset);
	free_usage_set(&pset);
	if (checkoverlap(pm->parts)) {
		hit_enter_to_continue(MSG_cantsave, NULL);
		return -1;
	}
	pm->unsaved = 1;
	return 0;
}

/* Safe erase of disk */
void
pm_shred(struct part_entry *pe, int shredtype)
{
	const char *srcdev;
	char dev[SSTRSIZE];
	struct pm_devs *my_pm;

	my_pm = pe->dev_ptr;
	if (pe->type == PM_DISK) {
		snprintf(dev, sizeof dev,
		    _PATH_DEV "r%s%c", my_pm->diskdev, 'a' + RAW_PART);
		if (pe->parts != NULL) {
			pe->parts->pscheme->free(pe->parts);
			pe->parts = NULL;
			my_pm->parts = NULL;
		}
	} else if (pe->type == PM_PART) {
		pe->parts->pscheme->get_part_device(pe->parts, pe->id,
		    dev, sizeof dev, NULL, raw_dev_name, true, true);
	}

	switch (shredtype) {
		case SHRED_ZEROS:
			srcdev = _PATH_DEVZERO;
			break;
		case SHRED_RANDOM:
			srcdev = _PATH_URANDOM;
			break;
		default:
			return;
	}
	run_program(RUN_DISPLAY | RUN_PROGRESS,
	    "progress -f %s -b 1m dd bs=1m of=%s", srcdev, dev);
	pm_partusage(my_pm, -1, 1);
}

#if 0 // XXX
static int
pm_mountall_sort(const void *a, const void *b)
{
	return strcmp(mnts[*(const int *)a].on, mnts[*(const int *)b].on);
}
#endif

#if 0	// XXX
/* Mount all available partitions */
static int
pm_mountall(void)
{
	int num_devs = 0;
	int mnts_order[MAX_MNTS];
	int i, ii, error, ok;
	char dev[SSTRSIZE]; dev[0] = '\0';
	struct pm_devs *pm_i;

	localfs_dev[0] = '\0';
	if (mnts == NULL)
		mnts = calloc(MAX_MNTS, sizeof(*mnts));

	SLIST_FOREACH(pm_i, &pm_head, l) {
		ok = 0;
		for (i = 0; i < MAXPARTITIONS; i++) {
			if (!(pm_i->bsdlabel[i].pi_flags & PIF_MOUNT &&
					pm_i->bsdlabel[i].mnt_opts != NULL))
				continue;
			mnts[num_devs].mnt_opts = pm_i->bsdlabel[i].mnt_opts;
			if (strlen(pm_i->bsdlabel[i].mounted) > 0) {
				/* Device is already mounted. So, doing mount_null */
				strlcpy(mnts[num_devs].dev, pm_i->bsdlabel[i].mounted, MOUNTLEN);
				mnts[num_devs].mnt_opts = "-t null";
			} else {
				pm_getdevstring(dev, SSTRSIZE, pm_i, i);
				snprintf(mnts[num_devs].dev, STRSIZE, "/dev/%s", dev);
			}
			mnts[num_devs].on = pm_i->bsdlabel[i].pi_mount;
			if (strcmp(pm_i->bsdlabel[i].pi_mount, "/") == 0) {
				/* Use disk with / as a default if the user has
				the sets on a local disk */
				strlcpy(localfs_dev, pm_i->diskdev, SSTRSIZE);
			}
			num_devs++;
			ok = 1;
		}
		if (ok)
			md_pre_mount(NULL, 0);
	}
	if (strlen(localfs_dev) < 1) {
		hit_enter_to_continue(MSG_noroot, NULL);
		return -1;
	}
	for (i = 0; i < num_devs; i++)
		mnts_order[i] = i;
	qsort(mnts_order, num_devs, sizeof mnts_order[0], pm_mountall_sort);

	for (i = 0; i < num_devs; i++) {
		ii = mnts_order[i];
		make_target_dir(mnts[ii].on);
		error = target_mount_do(mnts[ii].mnt_opts, mnts[ii].dev, mnts[ii].on);
		if (error)
			return error;
	}
	return 0;
}
#endif

/* Mount partition bypassing ordinary order */
static int
pm_mount(struct pm_devs *pm_cur, int part_num)
{
	int error = 0;
#if 0 // XXX
	char buf[MOUNTLEN];

	if (strlen(pm_cur->bsdlabel[part_num].mounted) > 0)
		return 0;

	snprintf(buf, sizeof(buf), "/tmp/%s%c", pm_cur->diskdev,
	    part_num + 'a');
	if (! dir_exists_p(buf))
		run_program(RUN_DISPLAY | RUN_PROGRESS, "/bin/mkdir -p %s", buf);
	if (pm_cur->bsdlabel[part_num].pi_flags & PIF_MOUNT &&
		pm_cur->bsdlabel[part_num].mnt_opts != NULL &&
		strlen(pm_cur->bsdlabel[part_num].mounted) < 1)
		error += run_program(RUN_DISPLAY | RUN_PROGRESS, "/sbin/mount %s /dev/%s%c %s",
				pm_cur->bsdlabel[part_num].mnt_opts,
				pm_cur->diskdev, part_num + 'a', buf);

	if (error)
		pm_cur->bsdlabel[part_num].mounted[0] = '\0';
	else {
		strlcpy(pm_cur->bsdlabel[part_num].mounted, buf, MOUNTLEN);
		pm_cur->blocked++;
	}
#endif
	return error;
}

void
pm_umount(struct pm_devs *pm_cur, int part_num)
{
	char buf[SSTRSIZE]; buf[0] = '\0';
	part_id id;

	if (part_num < 0)
		return;
	id = (part_id)part_num;

	pm_cur->parts->pscheme->get_part_device(pm_cur->parts, id, buf,
	    sizeof buf, NULL, plain_name, false, true);

	if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"umount -f /dev/%s", buf) == 0) {
		free(pm_cur->mounted[id]);
		pm_cur->mounted[id] = NULL;
		if (pm_cur->blocked > 0)
			pm_cur->blocked--;
	}
}

int
pm_unconfigure(struct pm_devs *pm_cur)
{
	int error = 0;
	if (! strncmp(pm_cur->diskdev, "cgd", 3)) {
 		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "cgdconfig -u %s", pm_cur->diskdev);
 		if (! error && pm_cur->refdev != NULL) {
			((struct cgd_desc*)pm_cur->refdev)->pm->blocked--;
			((struct cgd_desc*)pm_cur->refdev)->blocked = 0;
 		}
 	} else if (! strncmp(pm_cur->diskdev, "vnd", 3)) {
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "vndconfig -u %s", pm_cur->diskdev);
 		if (! error && pm_cur->refdev != NULL) {
			((struct vnd_desc*)pm_cur->refdev)->pm->blocked--;
			((struct vnd_desc*)pm_cur->refdev)->blocked = 0;
 		}
	} else if (! strncmp(pm_cur->diskdev, "raid", 4)) {
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "raidctl -u %s", pm_cur->diskdev);
		if (! error && pm_cur->refdev != NULL) {
			((struct raid_desc*)pm_cur->refdev)->blocked = 0;
#if 0 // XXX
			for (i = 0; i < MAX_IN_RAID; i++)
				if (((struct raid_desc*)pm_cur->refdev)->comp[i].parts != NULL)
					((raids_t*)pm_cur->refdev)->pm[i]->blocked--;
#endif
		}
	} else if (! strncmp(pm_cur->diskdev, "dk", 2)) {
		if (pm_cur->refdev == NULL)
			return -2;
		/* error = */
		run_program(RUN_DISPLAY | RUN_PROGRESS, "dkctl %s delwedge %s",
			((struct pm_devs*)pm_cur->refdev)->diskdev, pm_cur->diskdev);
#if 0 // XXX
		if (! error) {
			if (pm_cur->refdev != NULL && ((struct pm_devs*)pm_cur->refdev)->blocked > 0)
				((struct pm_devs*)pm_cur->refdev)->blocked--;
			sscanf(pm_cur->diskdev, "dk%d", &num);
			if (num >= 0 && num < MAX_WEDGES)
				wedges[num].allocated = 0;
		}
#endif
	} /* XXX: lvm */
	else
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "eject -t disk /dev/%sd",
			pm_cur->diskdev);
	if (!error)
		pm_cur->found = 0;
	return error;
}

/* Last checks before leaving partition manager */
#if 0
static int
pm_lastcheck(void)
{
	FILE *file_tmp = fopen(concat_paths(targetroot_mnt, "/etc/fstab"), "r");
	if (file_tmp == NULL)
		return 1;
	fclose(file_tmp);
	return 0;
}
#endif

/* Are there unsaved changes? */
static int
pm_needsave(void)
{
	struct pm_devs *pm_i;
	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->unsaved) {
			/* Oops, we have unsaved changes */
			pm_changed = 1;
			msg_display(MSG_saveprompt);
			return ask_yesno(NULL);
		}
	return 0;
}

/* Write all changes to disk */
static int
pm_commit(menudesc *m, void *arg)
{
	int retcode;
	struct pm_devs *pm_i;
	struct disk_partitions *secondary;

	pm_retvalue = -1;
	SLIST_FOREACH(pm_i, &pm_head, l) {
		if (! pm_i->unsaved)
			continue;
		if (pm_i->parts == NULL) {
			pm_i->unsaved = false;
			continue;
		}
		if (!pm_i->parts->pscheme->write_to_disk(pm_i->parts)) {
			if (logfp)
				fprintf(logfp, "partitining error %s\n",
				    pm_i->diskdev);
			return -1;
		}
		if (pm_i->parts->pscheme->secondary_scheme != NULL) {
			secondary = pm_i->parts->pscheme->
			    secondary_partitions(pm_i->parts, -1, false);
			if (secondary != NULL) {
				if (!secondary->pscheme->write_to_disk(
				    secondary)) {
					if (logfp)
						fprintf(logfp,
						    "partitining error %s\n",
						    pm_i->diskdev);
					return -1;
				}
			}
		}
	}

	/* Call all functions that may create new devices */
	if ((retcode = pm_raid_commit()) != 0) {
		if (logfp)
			fprintf(logfp, "RAIDframe configuring error #%d\n", retcode);
		return -1;
	}
	if ((retcode = pm_cgd_commit()) != 0) {
		if (logfp)
			fprintf(logfp, "CGD configuring error #%d\n", retcode);
		return -1;
	}
	if ((retcode = pm_lvm_commit()) != 0) {
		if (logfp)
			fprintf(logfp, "LVM configuring error #%d\n", retcode);
		return -1;
	}
	if ((retcode = pm_vnd_commit()) != 0) {
		if (logfp)
			fprintf(logfp, "VND configuring error #%d\n", retcode);
		return -1;
	}
	if (m != NULL && arg != NULL)
		pm_upddevlist(m, arg);
	if (logfp)
		fflush (logfp);

	pm_retvalue = 0;
	return 0;
}

#if 0 // XXX
static int
pm_savebootsector(void)
{
	struct pm_devs *pm_i;
	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->bootable) {
			if (! strncmp("raid", pm_i->diskdev, 4))
				if (run_program(RUN_DISPLAY | RUN_PROGRESS,
					"raidctl -v -A root %s", pm_i->diskdev) != 0) {
					if (logfp)
						fprintf(logfp, "Error writing RAID bootsector to %s\n",
							pm_i->diskdev);
					continue;
				}
			if (pm_i->no_part) {
				if (pm_i->rootpart < 0 ||
					run_program(RUN_DISPLAY | RUN_PROGRESS,
					"gpt biosboot -i %d %s",
					pm_i->rootpart + 1, pm_i->diskdev) != 0) {
			 		if (logfp)
						fprintf(logfp, "Error writing GPT bootsector to %s\n",
							pm_i->diskdev);
					continue;
				}
			} else {
				pm_select(pm_i);
			 	if (
#ifndef NO_DISKLABEL
				    !check_partitions(pm_i, pm_i->parts) ||
#endif
				    md_post_newfs() != 0) {
		 			if (logfp)
						fprintf(logfp, "Error writing bootsector to %s\n",
							pm_i->diskdev);
					continue;
				}
			}
		}
	return 0;
}
#endif

/* Function for 'Enter'-menu */
static int
pm_submenu(menudesc *m, void *arg)
{
	struct pm_devs *pm_cur = NULL;
	pm_retvalue = m->cursel + 1;
	struct part_entry *cur_pe = (struct part_entry *)arg + m->cursel;

	switch (cur_pe->type) {
		case PM_DISK:
		case PM_PART:
		case PM_SPEC:
			if (cur_pe->dev_ptr != NULL) {
				pm_cur = cur_pe->dev_ptr;
				if (pm_cur == NULL)
					return -1;
				if (pm_cur->blocked) {
					clear();
					refresh();
					msg_display(MSG_wannaunblock);
					if (!ask_noyes(NULL))
						return -2;
					pm_cur->blocked = 0;
				}
				pm_select(pm_cur);
			}
		default:
			break;
	}

	switch (cur_pe->type) {
		case PM_DISK:
			process_menu(MENU_pmdiskentry, cur_pe);
			break;
		case PM_PART:
			process_menu(MENU_pmpartentry, cur_pe);
			break;
		case PM_SPEC:
			process_menu(MENU_pmpartentry, cur_pe);
			break;
		case PM_RAID:
			pm_edit(PMR_MENU_END, pm_raid_edit_menufmt,
			    pm_raid_set_value, pm_raid_check, pm_raid_init,
			    NULL, cur_pe->dev_ptr, 0, &raids_t_info);
			break;
		case PM_VND:
			return pm_edit(PMV_MENU_END, pm_vnd_edit_menufmt,
			    pm_vnd_set_value, pm_vnd_check, pm_vnd_init,
			    NULL, cur_pe->dev_ptr, 0, &vnds_t_info);
		case PM_CGD:
			pm_cgd_edit_old(cur_pe);
			break;
		case PM_LVM:
			return pm_edit(PML_MENU_END, pm_lvm_edit_menufmt,
			    pm_lvm_set_value, pm_lvm_check, pm_lvm_init,
			    NULL, cur_pe->dev_ptr, 0, &lvms_t_info);
		case PM_LVMLV:
			return pm_edit(PMLV_MENU_END, pm_lvmlv_edit_menufmt,
			    pm_lvmlv_set_value, pm_lvmlv_check, pm_lvmlv_init,
			    NULL, cur_pe->dev_ptr,
			    cur_pe->dev_ptr_delta, &lv_t_info);
	}
	return 0;
}

/* Functions that generate menu entries text */
static void
pm_menufmt(menudesc *m, int opt, void *arg)
{
	const char *dev_status = "";
	char buf[STRSIZE], dev[STRSIZE];
	part_id part_num = ((struct part_entry *)arg)[opt].id;
	struct pm_devs *pm_cur = ((struct part_entry *)arg)[opt].dev_ptr;
	struct disk_partitions *parts = ((struct part_entry *)arg)[opt].parts;
	struct disk_part_info info;
	const char *mount_point, *fstype;

	switch (((struct part_entry *)arg)[opt].type) {
		case PM_DISK:
			if (pm_cur->blocked)
				dev_status = msg_string(MSG_pmblocked);
			else if (! pm_cur->unsaved)
				dev_status = msg_string(MSG_pmunchanged);
			else
				dev_status = msg_string(MSG_pmused);
			wprintw(m->mw, "%-43.42s %25.24s",
				pm_cur->diskdev_descr,
				dev_status);
			break;
		case PM_PART:
			if (parts->pscheme->get_part_device == NULL ||
			    !parts->pscheme->get_part_device(
				parts,  part_num,
				dev, sizeof dev, NULL, plain_name, false,
				true))
					strcpy(dev, "-");

			parts->pscheme->get_part_info(parts,
			    part_num, &info);
			if (pm_cur->mounted != NULL &&
			    pm_cur->mounted[part_num] != NULL &&
			    pm_cur->mounted[part_num][0] != 0)
				mount_point = msg_string(MSG_pmmounted);
			else
				mount_point = msg_string(MSG_pmunused);
			fstype = getfslabelname(info.fs_type,
			    info.fs_sub_type);
			if (info.last_mounted != NULL) {
				snprintf(buf, STRSIZE, "%s (%s) %s",
				    info.last_mounted, fstype,
				     mount_point);
				pm_fmt_disk_line(m->mw, dev, buf,
				    info.size, NULL);
			} else {
				if (fstype != NULL) {
					strlcat(dev, " (", sizeof(dev));
					strlcat(dev, fstype, sizeof(dev));
					strlcat(dev, ")", sizeof(dev));
				}
				pm_fmt_disk_line(m->mw, dev, NULL,
				    info.size, NULL);
			}
			break;
		case PM_SPEC:
			/* XXX ? */
			pm_fmt_disk_line(m->mw, pm_cur->diskdev_descr, NULL,
			    pm_cur->dlsize, NULL);
			break;
		case PM_RAID:
			pm_raid_menufmt(m, opt, arg);
			break;
		case PM_VND:
			pm_vnd_menufmt(m, opt, arg);
			break;
		case PM_CGD:
			pm_cgd_menufmt(m, opt, arg);
			break;
		case PM_LVM:
			pm_lvm_menufmt(m, opt, arg);
			break;
		case PM_LVMLV:
			pm_lvmlv_menufmt(m, opt, arg);
			break;
	}
}

/* Submenu for RAID/LVM/CGD/VND */
static void
pm_upddevlist_adv(menudesc *m, void *arg, int *i,
	pm_upddevlist_adv_t *d)
{
	int ii;
	if (d->create_msg != NULL) {
		/* We want to have menu entry that creates a new device */
		((struct part_entry *)arg)[*i].type = d->pe_type;
		((struct part_entry *)arg)[*i].dev_ptr = NULL;
		((struct part_entry *)arg)[*i].dev_ptr_delta = d->s->parent_size * d->sub_num;
		m->opts[(*i)++] = (struct menu_ent) {
			.opt_name = d->create_msg,
			.opt_action = pm_submenu,
		};
	}
	for (ii = 0; ii < d->s->max; ii++) {
		if (d->s->entry_enabled == NULL ||
			d->s->entry_blocked == NULL ||
			*(int*)((char*)d->s->entry_enabled + d->s->entry_size * ii +
				d->s->parent_size * d->sub_num) == 0 ||
			*(int*)((char*)d->s->entry_blocked + d->s->entry_size * ii +
				d->s->parent_size * d->sub_num) != 0)
			continue;
		/* We have a entry for displaying */
		pm_changed = 1;
		m->opts[*i] = (struct menu_ent) {
			.opt_name = NULL,
			.opt_action = pm_submenu,
		};
		((struct part_entry *)arg)[*i].type = d->pe_type;
		((struct part_entry *)arg)[*i].dev_ptr = (char*)d->s->entry_first +
			d->s->entry_size * ii + d->s->parent_size * d->sub_num;
		(*i)++;
		/* We should show submenu for current entry */
		if (d->sub != NULL) {
			d->sub->sub_num = ii;
			pm_upddevlist_adv(m, arg, i, d->sub);
		}
	}
}

/* Update partman main menu with devices list */
static int
pm_upddevlist(menudesc *m, void *arg)
{
	int i = 0;
	size_t ii;
	struct pm_devs *pm_i;
	struct disk_partitions *secondary;
	const struct disk_partitioning_scheme *ps;
	struct disk_part_info info;

	if (arg != NULL)
		pm_retvalue = m->cursel + 1;

	pm_changed = 0;
	/* Mark all devices as not found */
	SLIST_FOREACH(pm_i, &pm_head, l) {
		if (pm_i->parts != NULL && !pm_i->unsaved) {
			pm_i->parts->pscheme->free(pm_i->parts);
			pm_i->parts = NULL;
		}
		if (pm_i->found > 0)
			pm_i->found = 0;
	}

	/* Detect all present devices */
	(void)find_disks("partman", false);
	if (have_lvm)
		pm_lvm_find();
	pm_clean();

	if (m == NULL || arg == NULL)
		return -1;

	SLIST_FOREACH(pm_i, &pm_head, l) {
		memset(&m->opts[i], 0, sizeof m->opts[i]);
		m->opts[i].opt_action = pm_submenu;
		((struct part_entry *)arg)[i].dev_ptr = pm_i;
		((struct part_entry *)arg)[i].id = NO_PART;
		if (pm_i->no_part)
			((struct part_entry *)arg)[i].type = PM_SPEC;
		else {
			ps = pm_i->parts != NULL ? pm_i->parts->pscheme : NULL;
			secondary = NULL;

			((struct part_entry *)arg)[i].type = PM_DISK;

			for (ii = 0; pm_i->parts != NULL &&
			    ii < pm_i->parts->num_part; ii++) {
				if (!ps->get_part_info(
				    pm_i->parts, ii, &info))
					continue;
				if (info.flags & PTI_SEC_CONTAINER) {
					if (secondary == NULL &&
					    ps->secondary_scheme != NULL)
						secondary = ps->
						    secondary_partitions(
						    pm_i->parts,
						    info.start, false);
					continue;
				}
				if (info.flags & (PTI_WHOLE_DISK|
				    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
					continue;
				if (info.fs_type == FS_UNUSED)
					continue;
				if (i >= MAX_ENTRIES)
					break;
				i++;
				memset(&m->opts[i], 0, sizeof m->opts[i]);
				m->opts[i].opt_action = pm_submenu;
				((struct part_entry *)arg)[i].parts =
				    pm_i->parts;
				((struct part_entry *)arg)[i].dev_ptr = pm_i;
				((struct part_entry *)arg)[i].id = ii;
				((struct part_entry *)arg)[i].type = PM_PART;
			}

			for (ii = 0; secondary != NULL &&
			    ii < secondary->num_part; ii++) {
				if (!secondary->pscheme->get_part_info(
				    secondary, ii, &info))
					continue;
				if (info.flags & (PTI_WHOLE_DISK|
				    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
					continue;
				if (info.fs_type == FS_UNUSED)
					continue;
				if (i >= MAX_ENTRIES)
					break;
				i++;
				memset(&m->opts[i], 0, sizeof m->opts[i]);
				m->opts[i].opt_action = pm_submenu;
				((struct part_entry *)arg)[i].parts = secondary;
				((struct part_entry *)arg)[i].dev_ptr = pm_i;
				((struct part_entry *)arg)[i].id = ii;
				((struct part_entry *)arg)[i].type = PM_PART;
			}
		}
		i++;
	}
	for (ii = 0; ii <= (size_t)i; ii++) {
		m->opts[ii].opt_flags = OPT_EXIT;
	}
	if (have_cgd) {
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_cgd, PM_CGD,
		    &cgds_t_info, 0, NULL});
	}
	pm_upddevlist_adv(m, arg, &i,
	    &(pm_upddevlist_adv_t) {MSG_create_vnd, PM_VND,
	    &vnds_t_info, 0, NULL});
	if (have_lvm) {
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_vg, PM_LVM,
		    &lvms_t_info, 0,
		    &(pm_upddevlist_adv_t) {MSG_create_lv, PM_LVMLV,
		    &lv_t_info, 0,
		    NULL}});
	}
	if (have_raid) {
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_raid, PM_RAID, &raids_t_info, 0, NULL});
	}

	m->opts[i++] = (struct menu_ent) {
		.opt_name = MSG_updpmlist,
		.opt_action = pm_upddevlist,
	};
	m->opts[i  ] = (struct menu_ent) {
		.opt_name = MSG_savepm,
		.opt_action = pm_commit,
	};

	if (pm_retvalue >= 0)
		m->cursel = pm_retvalue - 1;
	return i;
}

static void
pm_menuin(menudesc *m, void *arg)
{
	if (pm_cursel > m->numopts)
		m->cursel = m->numopts;
	else if (pm_cursel < 0)
		m->cursel = 0;
	else
		m->cursel = pm_cursel;
}

static void
pm_menuout(menudesc *m, void *arg)
{
	pm_cursel = m->cursel;
}

/* Main partman function */
int
partman(void)
{
	int menu_no, menu_num_entries;
	static int firstrun = 1;
	menu_ent menu_entries[MAX_ENTRIES+6];
	struct part_entry args[MAX_ENTRIES];

	if (firstrun) {
		check_available_binaries();

		if (!have_raid)
			remove_raid_options();
		else if (!(raids = calloc(MAX_RAID, sizeof(*raids))))
			have_raid = 0;

#define remove_vnd_options() (void)0
		if (!have_vnd)
			remove_vnd_options();
		else if (!(vnds = calloc(MAX_VND, sizeof(*vnds))))
			have_vnd = 0;

		if (!have_cgd)
			remove_cgd_options();
		else if (!(cgds = calloc(MAX_CGD, sizeof(*cgds))))
			have_cgd = 0;

		if (!have_lvm)
			remove_lvm_options();
		else if (!(lvms = calloc(MAX_LVM_VG, sizeof(*lvms))))
			have_lvm = 0;

		raids_t_info = (structinfo_t) {
			.max = MAX_RAID,
			.entry_size = sizeof raids[0],
			.entry_first = &raids[0],
			.entry_enabled = &(raids[0].enabled),
			.entry_blocked = &(raids[0].blocked),
			.entry_node = &(raids[0].node),
		};
		vnds_t_info = (structinfo_t) {
			.max = MAX_VND,
			.entry_size = sizeof vnds[0],
			.entry_first = &vnds[0],
			.entry_enabled = &(vnds[0].enabled),
			.entry_blocked = &(vnds[0].blocked),
			.entry_node = &(vnds[0].node),
		};
		cgds_t_info = (structinfo_t) {
			.max = MAX_CGD,
			.entry_size = sizeof cgds[0],
			.entry_first = &cgds[0],
			.entry_enabled = &(cgds[0].enabled),
			.entry_blocked = &(cgds[0].blocked),
			.entry_node = &(cgds[0].node),
		};
		lvms_t_info = (structinfo_t) {
			.max = MAX_LVM_VG,
			.entry_size = sizeof lvms[0],
			.entry_first = &lvms[0],
			.entry_enabled = &(lvms[0].enabled),
			.entry_blocked = &(lvms[0].blocked),
			.entry_node = NULL,
		};
		lv_t_info = (structinfo_t) {
			.max = MAX_LVM_LV,
			.entry_size = sizeof lvms[0].lv[0],
			.entry_first = &lvms[0].lv[0],
			.entry_enabled = &(lvms[0].lv[0].size),
			.entry_blocked = &(lvms[0].lv[0].blocked),
			.parent_size = sizeof lvms[0],
		};

		pm_cursel = 0;
		pm_changed = 0;
		firstrun = 0;
	}

	do {
		menu_num_entries = pm_upddevlist(&(menudesc){.opts = menu_entries}, args);
		menu_no = new_menu(MSG_partman_header,
			menu_entries, menu_num_entries+1, 1, 1, 0, 75, /* Fixed width */
			MC_ALWAYS_SCROLL | MC_NOBOX | MC_NOCLEAR,
			pm_menuin, pm_menufmt, pm_menuout, NULL, MSG_finishpm);
		if (menu_no == -1)
			pm_retvalue = -1;
		else {
			pm_retvalue = 0;
			clear();
			refresh();
			process_menu(menu_no, &args);
			free_menu(menu_no);
		}

		if (pm_retvalue == 0 && pm->parts != NULL)
			if (pm_needsave())
				pm_commit(NULL, NULL);

	} while (pm_retvalue > 0);

	/* retvalue <0 - error, retvalue ==0 - user quits, retvalue >0 - all ok */
	return (pm_retvalue >= 0)?0:-1;
}

void
update_wedges(const char *disk)
{
	check_available_binaries();

	if (!have_dk)
		return;

	run_program(RUN_SILENT | RUN_ERROR_OK,
	    "dkctl %s makewedges", disk);
}

bool
pm_force_parts(struct pm_devs *my_pm)
{
	if (my_pm == NULL)
		return false;
	if (my_pm->parts != NULL)
		return true;

	const struct disk_partitioning_scheme *ps =
	    select_part_scheme(my_pm, NULL, false, NULL);
	if (ps == NULL)
		return false;

	struct disk_partitions *parts =
	   (*ps->create_new_for_disk)(my_pm->diskdev, 0,
	   my_pm->dlsize, false, NULL);
	if (parts == NULL)
		return false;

	my_pm->parts = parts;
	if (pm->dlsize > ps->size_limit)
		pm->dlsize = ps->size_limit;

	return true;
}

void
pm_edit_partitions(struct part_entry *pe)
{
	struct pm_devs *my_pm = pm_from_pe(pe);
	struct partition_usage_set pset = { 0 };
	struct disk_partitions *parts, *np;

	if (!my_pm)
		return;

	if (!pm_force_parts(my_pm))
		return;
	parts = my_pm->parts;

	clear();
	refresh();

	if (my_pm->parts->pscheme->secondary_scheme != NULL) {
		if (!edit_outer_parts(my_pm->parts))
			goto done;
		np = get_inner_parts(parts);
		if (np != NULL)
			parts = np;
	}

	if (parts != NULL) {
		usage_set_from_parts(&pset, parts);
		edit_and_check_label(my_pm, &pset, false);
		free_usage_set(&pset);
	}

done:
	pm_partusage(my_pm, -1, -1);
	my_pm->unsaved = true;
	pm_retvalue = 1;
}

part_id
pm_whole_disk(struct part_entry *pe, int t)
{
	struct pm_devs *my_pm = pm_from_pe(pe);
	struct disk_partitions *parts, *outer;
	struct disk_part_info info, oinfo;
	struct disk_part_free_space space;
	daddr_t align;
	int fst;
	struct partition_usage_set pset = { 0 };
	part_id new_part, id;
	size_t i, cnt;

	if (!my_pm)
		return NO_PART;

	if (!pm_force_parts(my_pm))
		return NO_PART;

	parts = my_pm->parts;
	parts->pscheme->delete_all_partitions(parts);
	if (parts->pscheme->secondary_scheme != NULL) {
		outer = parts;
		parts = parts->pscheme->secondary_partitions(outer,
		    0, true);
		if (parts == NULL) {
			parts = outer;
		} else {
			if (outer->pscheme->write_to_disk(outer))
				my_pm->parts = parts;
		}
	}

	align = parts->pscheme->get_part_alignment(parts);

	memset(&info, 0, sizeof info);
	switch (t) {
	case SY_NEWRAID:
		fst = FS_RAID;
		break;
	case SY_NEWLVM:
		fst = FS_BSDFFS;
		break;
	case SY_NEWCGD:
		fst = FS_CGD;
		break;
	default:
		assert(false);
		return NO_PART;
	}
	info.nat_type = parts->pscheme->get_fs_part_type(PT_root, fst, 0);
	if (info.nat_type != NULL && parts->pscheme->get_default_fstype != NULL)
		parts->pscheme->get_default_fstype(info.nat_type,
		    &info.fs_type, &info.fs_sub_type);
	if (parts->pscheme->get_free_spaces(parts, &space, 1,
	    5*align, align, -1, -1) != 1)
		return NO_PART;
	info.start = space.start;
	info.size = space.size;
	new_part = parts->pscheme->add_partition(parts, &info, NULL);
	if (new_part == NO_PART)
		return NO_PART;

	parts->pscheme->get_part_info(parts, new_part, &oinfo);

	clear();
	refresh();

	usage_set_from_parts(&pset, parts);
	edit_and_check_label(my_pm, &pset, false);
	free_usage_set(&pset);

	/*
	 * Try to match our new partition after user edit
	 */
	new_part = NO_PART;
	for (cnt = i = 0; i < parts->num_part; i++) {
		if (!parts->pscheme->get_part_info(parts,i, &info))
			continue;
		if (info.flags & (PTI_SEC_CONTAINER|PTI_WHOLE_DISK|
		    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
			continue;
		if (info.nat_type != oinfo.nat_type)
			continue;
		if (new_part == NO_PART)
			new_part = i;
		cnt++;
	}
	if (cnt > 1) {
		/* multiple matches, retry matching with start */
		id = NO_PART;
		for (cnt = i = 0; i < parts->num_part; i++) {
			if (!parts->pscheme->get_part_info(parts, i, &info))
				continue;
			if (info.flags & (PTI_SEC_CONTAINER|PTI_WHOLE_DISK|
			    PTI_PSCHEME_INTERNAL|PTI_RAW_PART))
				continue;
			if (info.nat_type != oinfo.nat_type)
				continue;
			if (info.start != oinfo.start)
				continue;
			if (id == NO_PART)
				id = i;
			cnt++;
		}
		if (id != NO_PART)
			new_part = id;
	}

	clear();
	refresh();

	pm_partusage(my_pm, -1, -1);
	my_pm->unsaved = true;
	pm_retvalue = 1;

	return new_part;
}

struct pm_devs *
pm_from_pe(struct part_entry *pe)
{
	switch (pe->type) {
	case PM_DISK:
		return pe->dev_ptr;
	default:
		assert(false);
	}
	return NULL;
}
