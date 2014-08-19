/*	$NetBSD: partman.c,v 1.6 2014/08/19 13:44:31 martin Exp $ */

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

/* partman.c - extended partitioning */

#include <fcntl.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>

#include "defs.h"
#include "msg_defs.h"
#include "menu_defs.h"

/* flags whether to offer the respective options (depending on helper
   programs available on install media */
static int have_raid, have_vnd, have_cgd, have_lvm, have_gpt, have_dk;

/* XXX: replace all MAX_* defines with vars that depend on kernel settings */
#define MAX_ENTRIES 96

#define MAX_RAID 8
#define MAX_IN_RAID 48
typedef struct raids_t {
	int enabled;
	int blocked;
	int node;
	char pm_name[MAX_IN_RAID][SSTRSIZE];
	int pm_part[MAX_IN_RAID];
	int pm_is_spare[MAX_IN_RAID];
	int numRow, numCol, numSpare;
	int sectPerSU, SUsPerParityUnit, SUsPerReconUnit, raid_level;
	uint total_size;
	pm_devs_t *pm[MAX_IN_RAID];
} raids_t;
raids_t raids[MAX_RAID];

#define MAX_VND 4
typedef struct vnds_t {
	int enabled;
	int blocked;
	int node;
	char filepath[STRSIZE];
	int size;
	int readonly;
	int is_exist;
	int manual_geom;
	int secsize, nsectors, ntracks, ncylinders;
	int pm_part;    /* Used only for */
	pm_devs_t *pm;  /* referring device */
} vnds_t;
vnds_t vnds[MAX_VND];

#define MAX_CGD 4
typedef struct cgds_t {
	int enabled;
	int blocked;
	int node;
	pm_devs_t *pm;
	char pm_name[SSTRSIZE];
	int pm_part;
	const char *keygen_type;
	const char *verify_type;
	const char *enc_type;
	const char *iv_type;
	int key_size;
} cgds_t;
cgds_t cgds[MAX_CGD];

#define MAX_LVM_VG 16
#define MAX_LVM_PV 255
#define MAX_LVM_LV 255
typedef struct pv_t {
	pm_devs_t *pm;
	char pm_name[SSTRSIZE];
	int pm_part;
	int metadatasize;
	int metadatacopies;
	int labelsector;
	int setphysicalvolumesize;
} pv_t;
typedef struct lv_t {
	int blocked;
	int size;
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
	uint total_size;
	pv_t pv[MAX_LVM_PV];
	lv_t lv[MAX_LVM_LV];
} lvms_t;
lvms_t lvms[MAX_LVM_VG];

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
} mnts[MAX_MNTS];

int cursel; /* Number of selected entry in main menu */
int changed; /* flag indicating that we have unsaved changes */
int raid_curspare; /* XXX: replace by true way */

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

part_entry_t pm_dev_list(int);
static int pm_raid_disk_add(menudesc *, void *);
static int pm_raid_disk_del(menudesc *, void *);
static int pm_cgd_disk_set(cgds_t *, part_entry_t *);
static int pm_mount(pm_devs_t *, int);
static int pm_upddevlist(menudesc *, void *);
static void pm_select(pm_devs_t *);

/* Universal menu for RAID/VND/CGD/LVM entry edit */
static int
pm_edit(int menu_entries_count, void (*menu_fmt)(menudesc *, int, void *),
	int (*action)(menudesc *, void *), int (*check_fun)(void *),
	void (*entry_init)(void *, void *),	void *entry_init_arg,
	void *dev_ptr, int dev_ptr_delta, structinfo_t *s)
{
	int i, ok = 0;

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
			process_menu(MENU_ok, deconst(MSG_limitcount));
			return -1;
		}
	}

	menu_ent menu_entries[menu_entries_count];
	for (i = 0; i < menu_entries_count - 1; i++)
		menu_entries[i] = (menu_ent) {NULL, OPT_NOMENU, 0, action};
	menu_entries[i] = (menu_ent) {MSG_fremove, OPT_NOMENU, OPT_EXIT, action};

	int menu_no = -1;
	menu_no = new_menu(NULL, menu_entries, menu_entries_count,
		-1, -1, 0, 40, MC_NOCLEAR | MC_SCROLL,
		NULL, menu_fmt, NULL, NULL, MSG_DONE);
	
	process_menu(menu_no, dev_ptr);
	free_menu(menu_no);

	return check_fun(dev_ptr);
}

static void
pm_getdevstring(char *buf, int len, pm_devs_t *pm_cur, int num)
{
	int i;

	if (pm_cur->isspecial)
		snprintf(buf, len, "%s", pm_cur->diskdev);
	else if (num + 'a' < 'a' || num + 'a' > 'a' + MAXPARTITIONS)
		snprintf(buf, len, "%sd", pm_cur->diskdev);
	else if (pm_cur->gpt) {
		for (i = 0; i < MAX_WEDGES; i++)
			if (wedges[i].pm == pm_cur &&
				wedges[i].ptn == num)
				snprintf(buf, len, "dk%d", i); // XXX: xxx
	} else
		snprintf(buf, len, "%s%c", pm_cur->diskdev, num + 'a');

	return;
}

/* Show filtered partitions menu */
part_entry_t
pm_dev_list(int type)
{
	int dev_num = -1, num_devs = 0;
	int i, ok;
	int menu_no;
	menu_ent menu_entries[MAX_DISKS*MAXPARTITIONS];
	part_entry_t disk_entries[MAX_DISKS*MAXPARTITIONS];
	pm_devs_t *pm_i;

	SLIST_FOREACH(pm_i, &pm_head, l)
		for (i = 0; i < MAXPARTITIONS; i++) {
			ok = 0;
			switch (type) {
				case PM_RAID_T:
					if (pm_i->bsdlabel[i].pi_fstype == FS_RAID)
						ok = 1;
					break;
				case PM_CGD_T:
					if (pm_i->bsdlabel[i].pi_fstype == FS_CGD)
						ok = 1;
					break;
				case PM_LVM_T:
					if (pm_i->bsdlabel[i].lvmpv)
						ok = 1;
					break;
			}
			if (ok && pm_partusage(pm_i, i, 0) == 0) {
				disk_entries[num_devs].dev_ptr = pm_i;
				disk_entries[num_devs].dev_num = i;
				pm_getdevstring(disk_entries[num_devs].fullname, SSTRSIZE, pm_i, i);

				menu_entries[num_devs] = (struct menu_ent) {
					.opt_name = disk_entries[num_devs].fullname,
					.opt_action = set_menu_select,
					.opt_menu = OPT_NOMENU,
					.opt_flags = OPT_EXIT,
				};
				num_devs++;
			}
		}

	menu_no = new_menu(MSG_avdisks,
		menu_entries, num_devs, -1, -1, (num_devs+1<3)?3:num_devs+1, 13,
		MC_SCROLL | MC_NOCLEAR, NULL, NULL, NULL, NULL, NULL);
	if (menu_no == -1)
		return (part_entry_t) { .retvalue = -1, };
	process_menu(menu_no, &dev_num);
	free_menu(menu_no);

	if (dev_num < 0 || dev_num >= num_devs)
		return (part_entry_t) { .retvalue = -1, };

	disk_entries[dev_num].retvalue = dev_num;
	return disk_entries[dev_num];
}

/* Get unused raid*, cgd* or vnd* device */
static int
pm_manage_getfreenode(void *node, const char *d, structinfo_t *s)
{
	int i, ii, ok;
	char buf[SSTRSIZE];
	pm_devs_t *pm_i;

	*(int*)node = -1;
	for (i = 0; i < s->max; i++) {
		ok = 1;
		/* Check that node is not already reserved */
		for (ii = 0; ii < s->max; ii++)
			if (*(int*)((char*)s->entry_node + s->entry_size * ii) == i) {
				ok = 0;
				break;
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
	process_menu(MENU_ok, deconst(MSG_nofreedev));
	return -1;
}

/***
 RAIDs
 ***/

static void
pm_raid_menufmt(menudesc *m, int opt, void *arg)
{
	int i, ok = 0;
	char buf[STRSIZE]; buf[0] = '\0';
	raids_t *dev_ptr = ((part_entry_t *)arg)[opt].dev_ptr;

	if (dev_ptr->enabled == 0)
		return;
	for (i = 0; i < MAX_IN_RAID; i++)
		if (dev_ptr->pm[i] != NULL) {
			strncat(buf, dev_ptr->pm_name[i], STRSIZE);
			strncat(buf, " ", STRSIZE);
			ok = 1;
		}
	if (ok)
		wprintw(m->mw, msg_string(MSG_raid_menufmt), dev_ptr->node,
			dev_ptr->raid_level, buf, dev_ptr->total_size);
	else
		wprintw(m->mw, "%s", msg_string(MSG_raid_err_menufmt));
	return;
}

static void
pm_raid_edit_menufmt(menudesc *m, int opt, void *arg)
{
	int i;
	char buf[STRSIZE]; buf[0] = '\0';
	raids_t *dev_ptr = arg;

	switch (opt) {
		case PMR_MENU_DEVS:
			for (i = 0; i < MAX_IN_RAID; i++)
				if (dev_ptr->pm[i] != NULL && dev_ptr->pm_is_spare[i] == 0)
					snprintf(buf, STRSIZE, "%s %s", buf, dev_ptr->pm_name[i]);
			wprintw(m->mw, msg_string(MSG_raid_disks_fmt), buf);
			break;
		case PMR_MENU_DEVSSPARE:
			for (i = 0; i < MAX_IN_RAID; i++)
				if (dev_ptr->pm[i] != NULL && dev_ptr->pm_is_spare[i] != 0)
					snprintf(buf, STRSIZE, "%s %s", buf, dev_ptr->pm_name[i]);
			wprintw(m->mw, msg_string(MSG_raid_spares_fmt), buf);
			break;
		case PMR_MENU_RAIDLEVEL:
			wprintw(m->mw, msg_string(MSG_raid_level_fmt), dev_ptr->raid_level);
			break;
		case PMR_MENU_NUMROW:
			wprintw(m->mw, msg_string(MSG_raid_numrow_fmt), dev_ptr->numRow);
			break;
		case PMR_MENU_NUMCOL:
			wprintw(m->mw, msg_string(MSG_raid_numcol_fmt), dev_ptr->numCol);
			break;
		case PMR_MENU_NUMSPARE:
			wprintw(m->mw, msg_string(MSG_raid_numspare_fmt), dev_ptr->numSpare);
			break;
		case PMR_MENU_SECTPERSU:
			wprintw(m->mw, msg_string(MSG_raid_sectpersu_fmt), dev_ptr->sectPerSU);
			break;
		case PMR_MENU_SUSPERPARITYUNIT:
			wprintw(m->mw, msg_string(MSG_raid_superpar_fmt), dev_ptr->SUsPerParityUnit);
			break;
		case PMR_MENU_SUSPERRECONUNIT:
			wprintw(m->mw, msg_string(MSG_raid_superrec_fmt), dev_ptr->SUsPerReconUnit);
			break;
	}
	return;
}

static int
pm_raid_set_value(menudesc *m, void *arg)
{
	int retvalue = -1;
	int *out_var = NULL;
	char buf[SSTRSIZE];
	const char *msg_to_show = NULL;
	raids_t *dev_ptr = arg;

	static menu_ent menuent_disk_adddel[] = {
	    {MSG_add, OPT_NOMENU, OPT_EXIT, pm_raid_disk_add}, 
	    {MSG_remove, OPT_NOMENU, OPT_EXIT, pm_raid_disk_del}
	};
	static int menu_disk_adddel = -1;
	if (menu_disk_adddel == -1) {
		menu_disk_adddel = new_menu(NULL, menuent_disk_adddel, nelem(menuent_disk_adddel),
			-1, -1, 0, 10, MC_NOCLEAR, NULL, NULL, NULL, NULL, NULL);
	}
	
	switch (m->cursel) {
		case PMR_MENU_DEVS:
			raid_curspare = 0;
			process_menu(menu_disk_adddel, dev_ptr);
			return 0;
		case PMR_MENU_DEVSSPARE:
			raid_curspare = 1;
			process_menu(menu_disk_adddel, dev_ptr);
			return 0;
		case PMR_MENU_RAIDLEVEL:
			process_menu(MENU_raidlevel, &retvalue);
			if (retvalue >= 0)
				dev_ptr->raid_level = retvalue;
			return 0;
		case PMR_MENU_NUMROW:
			process_menu(MENU_ok, deconst(MSG_raid_nomultidim));
			return 0;
			msg_to_show = MSG_raid_numrow_ask;
			out_var = &(dev_ptr->numRow);
			break;
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
	raids_t *dev_ptr = arg;
	memset(dev_ptr, 0, sizeof(*dev_ptr));
	*dev_ptr = (struct raids_t) {
		.enabled = 1,
		.blocked = 0,
		.sectPerSU = 32,
		.SUsPerParityUnit = 1,
		.SUsPerReconUnit = 1,
	};
	return;
}

static int
pm_raid_check(void *arg)
{
	int i, dev_num = 0, min_size = 0, cur_size = 0;
	raids_t *dev_ptr = arg;

	if (dev_ptr->blocked)
		return 0;
	for (i = 0; i < MAX_IN_RAID; i++)
		if (dev_ptr->pm[i] != NULL) {
			cur_size = dev_ptr->pm[i]->bsdlabel[dev_ptr->pm_part[i]].pi_size /
						(MEG / dev_ptr->pm[i]->sectorsize);
			if (cur_size < min_size || dev_num == 0)
				min_size = cur_size;
			if (!dev_ptr->pm_is_spare[i])
				dev_num++;
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
	raids_t *dev_ptr = arg;
	part_entry_t disk_entrie = pm_dev_list(PM_RAID_T);
	if (disk_entrie.retvalue < 0)
		return disk_entrie.retvalue;

	for (i = 0; i < MAX_IN_RAID; i++)
		if (dev_ptr->pm[i] == NULL) {
			dev_ptr->pm[i] = disk_entrie.dev_ptr;
			dev_ptr->pm_part[i] = disk_entrie.dev_num;
			dev_ptr->pm_is_spare[i] = raid_curspare;
			strlcpy(dev_ptr->pm_name[i], disk_entrie.fullname, SSTRSIZE);
			if (raid_curspare)
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
	raids_t *dev_ptr = arg;
	menu_ent menu_entries[MAX_IN_RAID];
	part_entry_t submenu_args[MAX_IN_RAID];

	for (i = 0; i < MAX_IN_RAID; i++) {
		if (dev_ptr->pm[i] == NULL ||
			dev_ptr->pm_is_spare[i] != raid_curspare)
			continue;
		menu_entries[num_devs] = (struct menu_ent) {
			.opt_name = dev_ptr->pm_name[i],
			.opt_action = set_menu_select,
			.opt_menu = OPT_NOMENU,
			.opt_flags = OPT_EXIT,
		};
		submenu_args[num_devs].dev_num = i;
		num_devs++;
	}

	menu_no = new_menu(MSG_raid_disks,
		menu_entries, num_devs, -1, -1, (num_devs+1<3)?3:num_devs+1, 13,
		MC_SCROLL | MC_NOCLEAR, NULL, NULL, NULL, NULL, NULL);
	if (menu_no == -1)
		return -1;
	process_menu(menu_no, &retvalue);
	free_menu(menu_no);

	if (retvalue < 0 || retvalue >= num_devs)
		return -1;

	pm_cur = submenu_args[retvalue].dev_num;

	if (dev_ptr->pm_is_spare[pm_cur])
		dev_ptr->numSpare--;
	else
		dev_ptr->numCol--;
	dev_ptr->numRow = (dev_ptr->numCol)?1:0;
	dev_ptr->pm[pm_cur] = NULL;

	return 0;
}

static int
pm_raid_commit(void)
{
	int i, ii;
	FILE *f;
	char f_name[STRSIZE];

	for (i = 0; i < MAX_RAID; i++) {
		if (! pm_raid_check(&raids[i]))
			continue;

		/* Generating configure file for our raid */
		snprintf(f_name, SSTRSIZE, "/tmp/raid.%d.conf", raids[i].node);
		f = fopen(f_name, "w");
		if (f == NULL) {
			endwin();
			(void)fprintf(stderr, "Could not open %s for writing\n", f_name);
			if (logfp)
				(void)fprintf(logfp, "Could not open %s for writing\n", f_name);
			return 1;
		}
		scripting_fprintf(NULL, "cat <<EOF >%s\n", f_name);
		scripting_fprintf(f, "START array\n%d %d %d\n", raids[i].numRow,
							raids[i].numCol, raids[i].numSpare);

		scripting_fprintf(f, "\nSTART disks\n");
		for (ii = 0; ii < MAX_IN_RAID; ii++)
			if (raids[i].pm[ii] != NULL && raids[i].pm_is_spare[ii] == 0) {
				scripting_fprintf(f,  "/dev/%s\n", raids[i].pm_name[ii]);
			}

		scripting_fprintf(f, "\nSTART spare\n");
		for (ii = 0; ii < MAX_IN_RAID; ii++)
			if (raids[i].pm[ii] != NULL && raids[i].pm_is_spare[ii] != 0) {
				scripting_fprintf(f,  "/dev/%s\n", raids[i].pm_name[ii]);
			}

		scripting_fprintf(f, "\nSTART layout\n%d %d %d %d\n", raids[i].sectPerSU,
						raids[i].SUsPerParityUnit, raids[i].SUsPerReconUnit,
						raids[i].raid_level);

		scripting_fprintf(f, "\nSTART queue\nfifo 100\n\n");
		scripting_fprintf(NULL, "EOF\n");
		fclose (f);
		fflush(NULL);

		/* Raid initialization */
		if (
			run_program(RUN_DISPLAY | RUN_PROGRESS, "raidctl -C %s raid%d",
							f_name, raids[i].node) == 0 &&
			run_program(RUN_DISPLAY | RUN_PROGRESS, "raidctl -I %d raid%d",
							rand(), raids[i].node) == 0 &&
			run_program(RUN_DISPLAY | RUN_PROGRESS, "raidctl -vi raid%d",
							raids[i].node) == 0 &&
			run_program(RUN_DISPLAY | RUN_PROGRESS, "raidctl -v -A yes raid%d",
							raids[i].node) == 0
			) {
			raids[i].blocked = 1; /* RAID creation done, remove it from list to 
									 prevent it's repeated reinitialization */
			for (ii = 0; ii < MAX_IN_RAID; ii++)
				if (raids[i].pm[ii] != NULL)
					raids[i].pm[ii]->blocked++;
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
	vnds_t *dev_ptr = ((part_entry_t *)arg)[opt].dev_ptr;

	if (dev_ptr->enabled == 0)
		return;
	if (strlen(dev_ptr->filepath) < 1)
		wprintw(m->mw, "%s", msg_string(MSG_vnd_err_menufmt));
	else if (dev_ptr->is_exist)
		wprintw(m->mw, msg_string(MSG_vnd_assgn_menufmt),
			dev_ptr->node, dev_ptr->filepath);
	else 
		wprintw(m->mw, msg_string(MSG_vnd_menufmt),
			dev_ptr->node, dev_ptr->filepath, dev_ptr->size);
	return;
}

static void
pm_vnd_edit_menufmt(menudesc *m, int opt, void *arg)
{
	vnds_t *dev_ptr = arg;
	char buf[SSTRSIZE];
	strcpy(buf, "-");

	switch (opt) {
		case PMV_MENU_FILEPATH:
			wprintw(m->mw, msg_string(MSG_vnd_path_fmt), dev_ptr->filepath);
			break;
		case PMV_MENU_EXIST:
			wprintw(m->mw, msg_string(MSG_vnd_assgn_fmt),
				dev_ptr->is_exist? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMV_MENU_SIZE:
			if (!dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->size);
			wprintw(m->mw, msg_string(MSG_vnd_size_fmt), buf);
			break;
		case PMV_MENU_RO:
			wprintw(m->mw, msg_string(MSG_vnd_ro_fmt),
				dev_ptr->readonly? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMV_MENU_MGEOM:
			if (!dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%s",
					dev_ptr->manual_geom? msg_string(MSG_Yes) : msg_string(MSG_No));
			wprintw(m->mw, msg_string(MSG_vnd_geom_fmt), buf);
			break;
		case PMV_MENU_SECSIZE:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->secsize);
			wprintw(m->mw, msg_string(MSG_vnd_bps_fmt), buf);
			break;
		case PMV_MENU_NSECTORS:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->nsectors);
			wprintw(m->mw, msg_string(MSG_vnd_spt_fmt), buf);
			break;
		case PMV_MENU_NTRACKS:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->ntracks);
			wprintw(m->mw, msg_string(MSG_vnd_tpc_fmt), buf);
			break;
		case PMV_MENU_NCYLINDERS:
			if (dev_ptr->manual_geom && !dev_ptr->is_exist)
				snprintf(buf, SSTRSIZE, "%d", dev_ptr->ncylinders);
			wprintw(m->mw, msg_string(MSG_vnd_cyl_fmt), buf);
			break;
	}
	return;
}

static int
pm_vnd_set_value(menudesc *m, void *arg)
{
	vnds_t *dev_ptr = arg;
	char buf[STRSIZE];
	const char *msg_to_show = NULL;
	int *out_var = NULL;
	
	switch (m->cursel) {
		case PMV_MENU_FILEPATH:
			msg_prompt_win(MSG_vnd_path_ask, -1, 18, 0, 0, dev_ptr->filepath,
				dev_ptr->filepath, STRSIZE);
			if (dev_ptr->filepath[0] != '/') {
				strlcpy(buf, dev_ptr->filepath, MOUNTLEN);
				snprintf(dev_ptr->filepath, MOUNTLEN, "/%s", buf);
			}
			if (dev_ptr->filepath[strlen(dev_ptr->filepath) - 1] == '/')
				dev_ptr->filepath[strlen(dev_ptr->filepath) - 1] = '\0';
			return 0;
		case PMV_MENU_EXIST:
			dev_ptr->is_exist = !dev_ptr->is_exist;
			return 0;
		case PMV_MENU_SIZE:
			if (dev_ptr->is_exist)
				return 0;
			msg_to_show = MSG_vnd_size_ask;
			out_var = &(dev_ptr->size);
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
	vnds_t *dev_ptr = arg;
	memset(dev_ptr, 0, sizeof(*dev_ptr));
	*dev_ptr = (struct vnds_t) {
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
	return;
}

static int
pm_vnd_check(void *arg)
{
	vnds_t *dev_ptr = arg;

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

/* XXX: vnconfig always return 0? */
static int
pm_vnd_commit(void)
{
	int i, ii, error, part_suit = -1;
	char r_o[3], buf[MOUNTLEN+3], resultpath[STRSIZE]; 
	pm_devs_t *pm_i, *pm_suit = NULL;

	for (i = 0; i < MAX_VND; i++) {
		error = 0;
		if (! pm_vnd_check(&vnds[i]))
			continue;

		/* Trying to assign target device */
		SLIST_FOREACH(pm_i, &pm_head, l)
			for (ii = 0; ii < 6; ii++) {
				strcpy(buf, pm_i->bsdlabel[ii].pi_mount);
				if (buf[strlen(buf)-1] != '/')
					sprintf(buf,"%s/", buf);
				printf("%s\n",buf);
				if (strstr(vnds[i].filepath, buf) == vnds[i].filepath)
					if (part_suit < 0 || pm_suit == NULL ||
						strlen(buf) > strlen(pm_suit->bsdlabel[part_suit].pi_mount)) {
						part_suit = ii;
						pm_suit = pm_i;
					}
			}
		if (part_suit < 0 || pm_suit == NULL || 
			pm_suit->bsdlabel[part_suit].mnt_opts == NULL ||
			! (pm_suit->bsdlabel[part_suit].pi_flags & PIF_MOUNT))
			continue;
		
		/* Mounting assigned partition and try to get real file path*/
		if (pm_mount(pm_suit, part_suit) != 0)
			continue;
		snprintf(resultpath, STRSIZE, "%s/%s",
			pm_suit->bsdlabel[part_suit].mounted,
			&(vnds[i].filepath[strlen(pm_suit->bsdlabel[part_suit].pi_mount)]));

		strcpy(r_o, vnds[i].readonly?"-r":"");
		/* If this is a new image */
		if (!vnds[i].is_exist) {
			run_program(RUN_DISPLAY | RUN_PROGRESS, "mkdir -p %s ", dirname(resultpath));
			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
						"dd if=/dev/zero of=%s bs=1m count=%d progress=100 msgfmt=human",
						resultpath, vnds[i].size);
		}
		if (error)
			continue;
		/* If this is a new image with manual geometry */
		if (!vnds[i].is_exist && vnds[i].manual_geom)
			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
						"vnconfig %s vnd%d %s %d %d %d %d", r_o, vnds[i].node,
						resultpath, vnds[i].secsize, vnds[i].nsectors,
						vnds[i].ntracks, vnds[i].ncylinders);
		/* If this is a existing image or image without manual geometry */
		else
			error += run_program(RUN_DISPLAY | RUN_PROGRESS, "vnconfig %s vnd%d %s",
						r_o, vnds[i].node, resultpath);

		if (! error) {
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
	cgds_t *dev_ptr = ((part_entry_t *)arg)[opt].dev_ptr;
	char desc[STRSIZE];

	if (dev_ptr->enabled == 0)
		return;
	if (dev_ptr->pm == NULL)
		wprintw(m->mw, "%s", msg_string(MSG_cgd_err_menufmt));
	else {
		snprintf(desc, STRSIZE, "(%s-%d) on %s", dev_ptr->enc_type,
			dev_ptr->key_size, dev_ptr->pm_name);
		wprintw(m->mw, msg_string(MSG_cgd_menufmt), dev_ptr->node, desc,
			dev_ptr->pm->bsdlabel[dev_ptr->pm_part].pi_size /
							(MEG / dev_ptr->pm->sectorsize));
	}
	return;
}

static void
pm_cgd_edit_menufmt(menudesc *m, int opt, void *arg)
{
	cgds_t *dev_ptr = arg;
	switch (opt) {
		case PMC_MENU_DEV:
			wprintw(m->mw, msg_string(MSG_cgd_dev_fmt), dev_ptr->pm_name);
			break;
		case PMC_MENU_ENCTYPE:
			wprintw(m->mw, msg_string(MSG_cgd_enc_fmt), dev_ptr->enc_type);
			break;
		case PMC_MENU_KEYSIZE:
			wprintw(m->mw, msg_string(MSG_cgd_key_fmt), dev_ptr->key_size);
			break;
		case PMC_MENU_IVTYPE:
			wprintw(m->mw, msg_string(MSG_cgd_iv_fmt), dev_ptr->iv_type);
			break;
		case PMC_MENU_KEYGENTYPE:
			wprintw(m->mw, msg_string(MSG_cgd_keygen_fmt), dev_ptr->keygen_type);
			break;
		case PMC_MENU_VERIFYTYPE:
			wprintw(m->mw, msg_string(MSG_cgd_verif_fmt), dev_ptr->verify_type);
			break;
	}
	return;
}

static int
pm_cgd_set_value(menudesc *m, void *arg)
{
	char *retstring;
	cgds_t *dev_ptr = arg;

	switch (m->cursel) {
		case PMC_MENU_DEV:
			pm_cgd_disk_set(dev_ptr, NULL);
			return 0;
		case PMC_MENU_ENCTYPE:
			process_menu(MENU_cgd_enctype, &retstring);
			dev_ptr->enc_type = retstring;
			if (! strcmp(retstring, "blowfish-cbc"))
				dev_ptr->key_size = 128;
			if (! strcmp(retstring, "3des-cbc"))
				dev_ptr->key_size = 192;
			return 0;
		case PMC_MENU_KEYSIZE:
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
	cgds_t *dev_ptr = arg1;
	part_entry_t *disk_entrie = arg2;

	memset(dev_ptr, 0, sizeof(*dev_ptr));
	*dev_ptr = (struct cgds_t) {
		.enabled = 1,
		.blocked = 0,
		.pm = NULL,
		.pm_name[0] = '\0',
		.pm_part = 0,
		.keygen_type = "pkcs5_pbkdf2/sha1",
		.verify_type = "disklabel",
		.enc_type = "aes-cbc",
		.iv_type = "encblkno1",
		.key_size = 192,
	};
	if (disk_entrie != NULL) {
		pm_getdevstring(disk_entrie->fullname, SSTRSIZE,
			disk_entrie->dev_ptr, disk_entrie->dev_num);
		pm_cgd_disk_set(dev_ptr, disk_entrie);
	}
	return;
}

static int
pm_cgd_check(void *arg)
{
	cgds_t *dev_ptr = arg;

	if (dev_ptr->blocked)
		return 0;
	if (dev_ptr->pm == NULL)
		dev_ptr->enabled = 0;
	else
		pm_manage_getfreenode(&(dev_ptr->node), "cgd", &cgds_t_info);
		if (dev_ptr->node < 0)
			dev_ptr->enabled = 0;
	return dev_ptr->enabled;
}

static int
pm_cgd_disk_set(cgds_t *dev_ptr, part_entry_t *disk_entrie)
{
	int alloc_disk_entrie = 0;
	if (disk_entrie == NULL) {
		alloc_disk_entrie = 1;
		disk_entrie = malloc (sizeof(part_entry_t));
		if (disk_entrie == NULL)
			return -2;
		*disk_entrie = pm_dev_list(PM_CGD_T);
		if (disk_entrie->retvalue < 0) {
			free(disk_entrie);
			return -1;
		}
	}
	dev_ptr->pm = disk_entrie->dev_ptr;
	dev_ptr->pm_part = disk_entrie->dev_num;
	strlcpy(dev_ptr->pm_name, disk_entrie->fullname, SSTRSIZE);

	if (alloc_disk_entrie)
		free(disk_entrie);
	return 0;
}

int
pm_cgd_edit(void *dev_ptr, part_entry_t *disk_entrie)
{
	if (disk_entrie != NULL)
		dev_ptr = NULL;
	return pm_edit(PMC_MENU_END, pm_cgd_edit_menufmt,
		pm_cgd_set_value, pm_cgd_check, pm_cgd_init,
		disk_entrie, dev_ptr, 0, &cgds_t_info);
}

static int
pm_cgd_commit(void)
{
	int i, error = 0;
	for (i = 0; i < MAX_CGD; i++) {
		if (! pm_cgd_check(&cgds[i]))
			continue;
		if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"cgdconfig -g -i %s -k %s -o /tmp/cgd.%d.conf %s %d",
			cgds[i].iv_type, cgds[i].keygen_type, cgds[i].node,
			cgds[i].enc_type, cgds[i].key_size) != 0) {
			error++;
			continue;
		}
		if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"cgdconfig -V re-enter cgd%d /dev/%s /tmp/cgd.%d.conf",
			cgds[i].node, cgds[i].pm_name, cgds[i].node) != 0) {
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
static int
pm_lvm_find(void)
{
	int i, ii, already_found;
	char dev[STRSIZE];
	pm_devs_t *pm_i;

	for (i = 0; i < MAX_LVM_VG; i++) {
		if (! lvms[i].blocked)
			continue;
		for (ii = 0; ii < MAX_LVM_LV; ii++) {
			if (! lvms[i].lv[ii].blocked || lvms[i].lv[ii].size < 1)
				continue;
			snprintf(dev, STRSIZE, "%s/%s", lvms[i].name, lvms[i].lv[ii].name);
			already_found = 0;
			SLIST_FOREACH(pm_i, &pm_head, l)
				if (!already_found && strcmp(pm_i->diskdev, dev) == 0) {
					pm_i->found = 1;
					already_found = 1;
				}
			if (already_found)
				/* We already added this device, skipping */
				continue;
			pm_new->found = 1;
			pm_new->isspecial = 1;
			pm_new->refdev = &lvms[i].lv[ii];
			pm_new->sectorsize = 1;
			pm_new->dlcylsize = MEG;
			pm_new->bsdlabel[0].pi_size = MEG * lvms[i].lv[ii].size;
			strlcpy(pm_new->diskdev, dev, SSTRSIZE);
			strlcpy(pm_new->diskdev_descr, dev, STRSIZE);

			if (SLIST_EMPTY(&pm_head))
				 SLIST_INSERT_HEAD(&pm_head, pm_new, l);
			else
				 SLIST_INSERT_AFTER(pm_i, pm_new, l);
			pm_new = malloc(sizeof (pm_devs_t));
			memset(pm_new, 0, sizeof *pm_new);
		}
	}
	return 0;
}

static int
pm_lvm_disk_add(menudesc *m, void *arg)
{
	int i;
	lvms_t *dev_ptr = arg;
	part_entry_t disk_entrie = pm_dev_list(PM_LVM_T);
	if (disk_entrie.retvalue < 0)
		return disk_entrie.retvalue;

	for (i = 0; i < MAX_LVM_PV; i++)
		if (dev_ptr->pv[i].pm == NULL) {
			dev_ptr->pv[i].pm = disk_entrie.dev_ptr;
			dev_ptr->pv[i].pm_part = disk_entrie.dev_num;
			strlcpy(dev_ptr->pv[i].pm_name, disk_entrie.fullname, SSTRSIZE);
			break;
		}
	return 0;
}

static int
pm_lvm_disk_del(menudesc *m, void *arg)
{
	int i, retvalue = -1, num_devs = 0;
	int menu_no;
	lvms_t *dev_ptr = arg;
	menu_ent menu_entries[MAX_LVM_PV];
	part_entry_t submenu_args[MAX_LVM_PV];

	for (i = 0; i < MAX_LVM_PV; i++) {
		if (dev_ptr->pv[i].pm == NULL)
			continue;
		menu_entries[num_devs] = (struct menu_ent) {
			.opt_name = dev_ptr->pv[i].pm_name,
			.opt_action = set_menu_select,
			.opt_menu = OPT_NOMENU,
			.opt_flags = OPT_EXIT,
		};
		submenu_args[num_devs].dev_num = i;
		num_devs++;
	}

	menu_no = new_menu(MSG_lvm_disks,
		menu_entries, num_devs, -1, -1, (num_devs+1<3)?3:num_devs+1, 13,
		MC_SCROLL | MC_NOCLEAR, NULL, NULL, NULL, NULL, NULL);
	if (menu_no == -1)
		return -1;
	process_menu(menu_no, &retvalue);
	free_menu(menu_no);

	if (retvalue < 0 || retvalue >= num_devs)
		return -1;

	dev_ptr->pv[submenu_args[retvalue].dev_num].pm = NULL;

	return 0;
}

static void
pm_lvm_menufmt(menudesc *m, int opt, void *arg)
{
	int i, ok = 0, used_size = 0;
	char buf1[STRSIZE]; buf1[0] = '\0';
	char buf2[STRSIZE]; buf2[0] = '\0';
	lvms_t *dev_ptr = ((part_entry_t *)arg)[opt].dev_ptr;

	if (dev_ptr->enabled == 0)
		return;
	snprintf(buf1, STRSIZE, "VG '%s' on ", dev_ptr->name);
	for (i = 0; i < MAX_LVM_PV; i++)
		if (dev_ptr->pv[i].pm != NULL) {
			strncat(buf1, dev_ptr->pv[i].pm_name, STRSIZE);
			strncat(buf1, " ", STRSIZE);
			ok = 1;
		}
	for (i = 0; i < MAX_LVM_LV; i++)
		used_size += dev_ptr->lv[i].size;
	if (ok) {
		snprintf(buf2, STRSIZE, "%d/%u",
			dev_ptr->total_size - used_size, dev_ptr->total_size);
		wprintw(m->mw, msg_string(MSG_lvm_menufmt), buf1, buf2);
	} else
		wprintw(m->mw, "%s", msg_string(MSG_lvm_err_menufmt));
	return;
}

static void
pm_lvm_edit_menufmt(menudesc *m, int opt, void *arg)
{
	int i;
	char buf[STRSIZE];
	lvms_t *dev_ptr = arg;
	strlcpy(buf, msg_string(MSG_auto), STRSIZE);

	switch (opt) {
		case PML_MENU_PV:
			buf[0] = '\0';
			for (i = 0; i < MAX_LVM_PV; i++)
				if (dev_ptr->pv[i].pm != NULL)
					snprintf(buf, STRSIZE, "%s %s", buf, dev_ptr->pv[i].pm_name);
			wprintw(m->mw, msg_string(MSG_lvm_disks_fmt), buf);
			break;
		case PML_MENU_NAME:
			wprintw(m->mw, msg_string(MSG_lvm_name_fmt), dev_ptr->name);
			break;
		case PML_MENU_MAXLOGICALVOLUMES:
			if (dev_ptr->maxlogicalvolumes > 0)
				snprintf(buf, STRSIZE, "%d", dev_ptr->maxlogicalvolumes);
			wprintw(m->mw, msg_string(MSG_lvm_maxlv_fmt), buf);
			break;
		case PML_MENU_MAXPHYSICALVOLUMES:
			if (dev_ptr->maxphysicalvolumes > 0)
				snprintf(buf, STRSIZE, "%d", dev_ptr->maxphysicalvolumes);
			wprintw(m->mw, msg_string(MSG_lvm_maxpv_fmt), buf);
			break;
		case PML_MENU_PHYSICALEXTENTSIZE:
			if (dev_ptr->physicalextentsize > 0)
				snprintf(buf, STRSIZE, "%dM", dev_ptr->physicalextentsize);
			wprintw(m->mw, msg_string(MSG_lvm_extsiz_fmt), buf);
			break;
	}
	return;
}

static int
pm_lvm_set_value(menudesc *m, void *arg)
{
	char buf[STRSIZE];
	const char *msg_to_show = NULL;
	int *out_var = NULL;
	lvms_t *dev_ptr = arg;

	static menu_ent menuent_disk_adddel[] = {
	    {MSG_add, OPT_NOMENU, OPT_EXIT, pm_lvm_disk_add}, 
	    {MSG_remove, OPT_NOMENU, OPT_EXIT, pm_lvm_disk_del}
	};
	static int menu_disk_adddel = -1;
	if (menu_disk_adddel == -1) {
		menu_disk_adddel = new_menu(NULL, menuent_disk_adddel, nelem(menuent_disk_adddel),
			-1, -1, 0, 10, MC_NOCLEAR, NULL, NULL, NULL, NULL, NULL);
	}

	switch (m->cursel) {
		case PML_MENU_PV:
			process_menu(menu_disk_adddel, arg);
			return 0;
		case PML_MENU_NAME:
			msg_prompt_win(MSG_lvm_name_ask, -1, 18, 0, 0, dev_ptr->name, dev_ptr->name, SSTRSIZE);
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
	return;
}

static int
pm_lvm_check(void *arg)
{
	int i, ok = 0;
	lvms_t *dev_ptr = arg;
	dev_ptr->total_size = 0;

	for (i = 0; i < MAX_LVM_PV; i++)
		if (dev_ptr->pv[i].pm != NULL) {
			ok = 1;
			dev_ptr->total_size += dev_ptr->pv[i].pm->
						bsdlabel[dev_ptr->pv[i].pm_part].pi_size /
						(MEG / dev_ptr->pv[i].pm->sectorsize);
		}
	if (! ok)
		dev_ptr->enabled = 0;
	return dev_ptr->enabled;
}

static void
pm_lvmlv_menufmt(menudesc *m, int opt, void *arg)
{
	char buf[STRSIZE];
	lv_t *dev_ptr = ((part_entry_t *)arg)[opt].dev_ptr;

	if (dev_ptr->size > 0) {
		snprintf(buf, STRSIZE, "'%s'", dev_ptr->name);
		wprintw(m->mw, msg_string(MSG_lvmlv_menufmt), buf, dev_ptr->size);
	}
	return;
}

static void
pm_lvmlv_edit_menufmt(menudesc *m, int opt, void *arg)
{

	lv_t *dev_ptr = arg;
	char buf[STRSIZE];
	strlcpy(buf, msg_string(MSG_auto), STRSIZE);

	switch (opt) {
		case PMLV_MENU_NAME:
			wprintw(m->mw, msg_string(MSG_lvmlv_name_fmt), dev_ptr->name);
			break;
		case PMLV_MENU_SIZE:
			wprintw(m->mw, msg_string(MSG_lvmlv_size_fmt), dev_ptr->size);
			break;
		case PMLV_MENU_READONLY:
			wprintw(m->mw, msg_string(MSG_lvmlv_ro_fmt),
				dev_ptr->readonly? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMLV_MENU_CONTIGUOUS:
			wprintw(m->mw, msg_string(MSG_lvmlv_cont_fmt),
				dev_ptr->contiguous? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMLV_MENU_EXTENTS:
			wprintw(m->mw, msg_string(MSG_lvmlv_extnum_fmt),
				(strlen(dev_ptr->extents) > 0)? dev_ptr->extents : msg_string(MSG_auto));
			break;
		case PMLV_MENU_MINOR:
			if (dev_ptr->minor > 0)
				snprintf(buf, STRSIZE, "%dK", dev_ptr->minor);
			wprintw(m->mw, msg_string(MSG_lvmlv_minor_fmt), buf);
			break;
		case PMLV_MENU_MIRRORS:
			wprintw(m->mw, msg_string(MSG_lvmlv_mirrors_fmt), dev_ptr->mirrors);
			break;
		case PMLV_MENU_REGIONSIZE:
			if (dev_ptr->regionsize > 0)
				snprintf(buf, STRSIZE, "%dM", dev_ptr->regionsize);
			wprintw(m->mw, msg_string(MSG_lvmlv_regsiz_fmt), buf);
			break;
		case PMLV_MENU_PERSISTENT:
			wprintw(m->mw, msg_string(MSG_lvmlv_pers_fmt),
				dev_ptr->persistent ? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
		case PMLV_MENU_READAHEAD:
			if (dev_ptr->readahead > 0)
				snprintf(buf, STRSIZE, "%d", dev_ptr->readahead);
			wprintw(m->mw, msg_string(MSG_lvmlv_readahsect_fmt), buf);
			break;
		case PMLV_MENU_STRIPES:
			if (dev_ptr->stripes > 0)
				snprintf(buf, STRSIZE, "%d", dev_ptr->stripes);
			wprintw(m->mw, msg_string(MSG_lvmlv_stripes_fmt), buf);
			break;
		case PMLV_MENU_STRIPESIZE:
			if (dev_ptr->stripesize > 0)
				snprintf(buf, STRSIZE, "%dK", dev_ptr->stripesize);
			wprintw(m->mw, msg_string(MSG_lvmlv_stripesiz_fmt), buf);
			break;
		case PMLV_MENU_ZERO:
			wprintw(m->mw, msg_string(MSG_lvmlv_zero_fmt),
				dev_ptr->zero? msg_string(MSG_Yes) : msg_string(MSG_No));
			break;
	}
	return;
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
			msg_prompt_win(MSG_lvmlv_name_ask, -1, 18, 0, 0, dev_ptr->name, dev_ptr->name,
				SSTRSIZE);
			return 0;
		case PMLV_MENU_SIZE:
			msg_to_show = MSG_lvmlv_size_ask;
			out_var = &(dev_ptr->size);
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
	return;
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
	char params[STRSIZE*3];
	char devs[STRSIZE*3];

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
				snprintf(devs, STRSIZE*3, "%s /dev/r%s", devs, (char*)lvms[i].pv[ii].pm_name);
			}
		if (error)
			continue;
		/* Stage 2: creating Volume Groups (VG's) */
		if (lvms[i].maxlogicalvolumes > 0)
			snprintf(params, STRSIZE*3, "%s -l %d", params, lvms[i].maxlogicalvolumes);
		if (lvms[i].maxphysicalvolumes > 0)
			snprintf(params, STRSIZE*3, "%s -p %d", params, lvms[i].maxphysicalvolumes);
		if (lvms[i].physicalextentsize > 0)
			snprintf(params, STRSIZE*3, "%s -s %d", params, lvms[i].physicalextentsize);
		error += run_program(RUN_DISPLAY | RUN_PROGRESS, "lvm vgcreate %s %s %s",
							params, lvms[i].name, devs);
		if (error)
			continue;
		/* Stage 3: creating Logical Volumes (LV's) */
		for (ii = 0; ii < MAX_LVM_LV; ii++) {
			if (lvms[i].lv[ii].size <= 0)
				continue;

			params[0] = '\0';
			snprintf(params, STRSIZE*3, "%s -C %c", params, lvms[i].lv[ii].contiguous?'y':'n');
			snprintf(params, STRSIZE*3, "%s -M %c", params, lvms[i].lv[ii].persistent?'y':'n');
			snprintf(params, STRSIZE*3, "%s -p %s", params, lvms[i].lv[ii].readonly?"r":"rw");
			snprintf(params, STRSIZE*3, "%s -Z %c", params, lvms[i].lv[ii].zero?'y':'n');
			if (strlen(lvms[i].lv[ii].name) > 0)
				snprintf(params, STRSIZE*3, "%s -n %s", params, lvms[i].lv[ii].name);
			if (strlen(lvms[i].lv[ii].extents) > 0)
				snprintf(params, STRSIZE*3, "%s -l %s", params, lvms[i].lv[ii].extents);
			if (lvms[i].lv[ii].minor > 0)
				snprintf(params, STRSIZE*3, "%s --minor %d", params, lvms[i].lv[ii].minor);
			if (lvms[i].lv[ii].mirrors > 0) {
				snprintf(params, STRSIZE*3, "%s -m %d", params, lvms[i].lv[ii].mirrors);
				if (lvms[i].lv[ii].regionsize > 0)
					snprintf(params, STRSIZE*3, "%s -R %d", params, lvms[i].lv[ii].regionsize);
			}
			if (lvms[i].lv[ii].readahead > 0)
				snprintf(params, STRSIZE*3, "%s -r %d", params, lvms[i].lv[ii].readahead);
			if (lvms[i].lv[ii].stripes > 0) {
				snprintf(params, STRSIZE*3, "%s -i %d", params, lvms[i].lv[ii].stripes);
				if (lvms[i].lv[ii].stripesize > 0)
					snprintf(params, STRSIZE*3, "%s -I %d", params, lvms[i].lv[ii].stripesize);
			}
			snprintf(params, STRSIZE*3, "%s -L %uM", params, lvms[i].lv[ii].size);
			error += run_program(RUN_DISPLAY | RUN_PROGRESS, "lvm lvcreate %s %s",
									params, lvms[i].name);
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
 GPT
 ***/

int
pm_gpt_convert(pm_devs_t *pm_cur)
{
	int i, error = 0;

	msg_display(MSG_removepartswarn);
	process_menu(MENU_noyes, NULL);
	if (! yesno)
		return -1;

	if (! pm_cur->gpt)
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "gpt create -f %s", 
				pm_cur->diskdev);
	else {
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "gpt destroy %s", 
				pm_cur->diskdev);
		for (i = 0; i < MAX_WEDGES; i++)
			if (wedges[i].pm == pm_cur)
				wedges[i].pm = NULL;
	}
	if (!error) {
		pm_cur->gpt = !pm_cur->gpt;
		pm_select(pm_cur);
		label_read();
	}
	return error;
}

static int
pm_wedge_getfree(void)
{
	int i;
	for (i = 0; i < MAX_WEDGES; i++)
		if (! wedges[i].allocated && wedges[i].pm == NULL)
			return i;
	return -1;
}

void
pm_wedges_fill(pm_devs_t *pm_cur)
{
	int i, current;

	for (i = 0; i < MAX_WEDGES; i++)
		if (wedges[i].pm == pm_cur && ! wedges[i].allocated)
			wedges[i].pm = NULL;

	for (i = 0; i < min(MAXPARTITIONS,MAX_WEDGES); i++)
		if (pm_cur->bsdlabel[i].pi_fstype != FS_UNUSED) {
			current = pm_wedge_getfree();
			if (current < 0) {
				process_menu(MENU_ok, deconst(MSG_limitcount));
				return;
			}
			wedges[current].pm = pm_cur;
			wedges[current].ptn = i;
		}
	return;
}

static int
pm_wedge_create(int num, pm_devs_t **pm_dk)
{
	int i, ii, hackerr, error;
	partinfo *p = &(wedges[num].pm->bsdlabel[wedges[num].ptn]);

	if (num > MAX_WEDGES)
		return -1;

	/* There is no ability to use required dkX device, so that's a hack */
	for (i = 0; i < num; i++)
		if (! wedges[i].allocated) {
			hackerr = 1;
			for (ii = 0; hackerr && ii < 3; ii++)
				hackerr = run_program(RUN_SILENT | RUN_ERROR_OK,
					"dkctl %s addwedge HACK%d %d %d unused",
					wedges[num].pm->diskdev,
					rand()%33, rand()%33, 0);
			wedges[i].todel = 1;
		}
	
	run_program(RUN_SILENT | RUN_ERROR_OK,
					"dkctl %s listwedges", wedges[num].pm->diskdev);

	error = run_program(RUN_DISPLAY | RUN_PROGRESS,
			"dkctl %s addwedge dk%d %u %u %s",
			wedges[num].pm->diskdev,
			num,
			p->pi_offset,
			p->pi_size,
			getfstypename(p->pi_fstype));
	if (!error) {
		wedges[num].allocated = 1;
		wedges[num].pm->blocked++;
		if (pm_dk != NULL) {
			*pm_dk = malloc(sizeof(pm_devs_t));
			memset(*pm_dk, 0, sizeof **pm_dk);
			memcpy(&(*pm_dk)->bsdlabel[0], p, sizeof (*pm_dk)->bsdlabel[0]);
			(*pm_dk)->found = -1;
			(*pm_dk)->isspecial = 1;
			(*pm_dk)->sectorsize = 512; /* XXX: xxx */
			(*pm_dk)->dlcylsize = MEG;
			(*pm_dk)->refdev = wedges[num].pm;
			snprintf((*pm_dk)->diskdev, SSTRSIZE, "dk%d", num);
			snprintf((*pm_dk)->diskdev_descr, STRSIZE, "   %s (%s)",
				(*pm_dk)->diskdev, getfstypename(p->pi_fstype));
		}
	} else
		if (logfp)
			fprintf(logfp, "Cannot create wedge dk%d for %s.\n",
				num, wedges[num].pm->diskdev);

	/* Hack, see above */
	for (i = 0; i < num; i ++)
		if (! wedges[i].allocated && wedges[i].todel) {
			hackerr = run_program(RUN_SILENT | RUN_ERROR_OK,
				"dkctl %s delwedge dk%d", wedges[num].pm->diskdev, i);
			wedges[i].todel = 0;
		}

	return error;
}

static int
pm_gpt_commit(void)
{
	uint i, error;
	pm_devs_t *pm_i, *pm_dk = NULL;
	char fstype[STRSIZE]; fstype[0] = '\0';
	partinfo *p;

	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->gpt && pm_i->unsaved)
			run_program(RUN_DISPLAY | RUN_PROGRESS,
				"gpt remove -a %s",
				pm_i->diskdev);

	for (i = 0; i < MAX_WEDGES; i++) {
		if (wedges[i].pm == NULL ||
			! wedges[i].pm->unsaved ||
			wedges[i].ptn < 0)
			continue;
		error = 0;
		p = &(wedges[i].pm->bsdlabel[wedges[i].ptn]);
		if (wedges[i].pm->gpt) {
			if (get_gptfs_by_id(p->pi_fstype) != NULL)
				snprintf(fstype, STRSIZE, "-t %s", get_gptfs_by_id(p->pi_fstype));
			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
				"gpt add -i %u -b %u -s %u %s %s",
				wedges[i].ptn + 1,
				p->pi_offset, p->pi_size,
				fstype, wedges[i].pm->diskdev);
		}
		
		if (error || wedges[i].allocated) {
			if (logfp && error)
				fprintf(logfp, "Cannot create GPT partition #%d on %s\n",
					i, wedges[i].pm->diskdev);
			continue;
		}

		error += pm_wedge_create(i, &pm_dk);
		if (pm_dk != NULL) {
			/* Create file system on wedge and add it to list */
			pm_select(pm_dk);
			make_filesystems();
			SLIST_INSERT_AFTER(wedges[i].pm, pm_dk, l);
			memset(p, 0, sizeof *p);
			if (error)
				return -1;
		}
	}

	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->gpt && pm_i->unsaved)
			pm_i->unsaved = 0;

	return 0;
}

/***
 Partman generic functions 
 ***/

int
pm_getrefdev(pm_devs_t *pm_cur)
{
	int i, ii, dev_num, num_devs, num_devs_s;
	char dev[SSTRSIZE]; dev[0] = '\0';

	pm_cur->refdev = NULL;
	if (! strncmp(pm_cur->diskdev, "cgd", 3)) {
		dev_num = pm_cur->diskdev[3] - '0';
		for (i = 0; i < MAX_CGD; i++)
			if (cgds[i].blocked && cgds[i].node == dev_num) {
				pm_cur->refdev = &cgds[i];

				snprintf(pm_cur->diskdev_descr, STRSIZE, "%s (%s, %s-%d)",
					pm_cur->diskdev_descr, cgds[i].pm_name,
					cgds[i].enc_type, cgds[i].key_size);
				break;
			}
 	} else if (! strncmp(pm_cur->diskdev, "vnd", 3)) {
 		dev_num = pm_cur->diskdev[3] - '0';
 		for (i = 0; i < MAX_VND; i++)
			if (vnds[i].blocked && vnds[i].node == dev_num) {
				pm_cur->refdev = &vnds[i];
				pm_getdevstring(dev, SSTRSIZE, vnds[i].pm, vnds[i].pm_part);
				snprintf(pm_cur->diskdev_descr, STRSIZE, "%s (%s, %s)",
					pm_cur->diskdev_descr, dev, vnds[i].filepath);
				break;
			}
	} else if (! strncmp(pm_cur->diskdev, "raid", 4)) {
		dev_num = pm_cur->diskdev[4] - '0';
 		for (i = 0; i < MAX_RAID; i++)
			if (raids[i].blocked && raids[i].node == dev_num) {
				pm_cur->refdev = &raids[i];
				num_devs = 0; num_devs_s = 0;
				for (ii = 0; ii < MAX_IN_RAID; ii++)
					if (raids[i].pm[ii] != NULL) {
						if(raids[i].pm_is_spare[ii])
							num_devs_s++;
						else
							num_devs++;
					}
				snprintf(pm_cur->diskdev_descr, STRSIZE,
					"%s (lvl %d, %d disks, %d spare)", pm_cur->diskdev_descr,
					raids[i].raid_level, num_devs, num_devs_s);
				break;
			}
	} else
		return -1;
	return 0;
}

/* Detect that partition is in use */
int
pm_partusage(pm_devs_t *pm_cur, int part_num, int do_del)
{
	int i, ii, retvalue = 0;

	if (part_num < 0) {
		/* Check all partitions on device */
		for (i = 0; i < MAXPARTITIONS; i++)
			retvalue += pm_partusage(pm_cur, i, do_del);
		return retvalue;
	}

	for (i = 0; i < MAX_CGD; i++)
		if (cgds[i].enabled &&
			cgds[i].pm == pm_cur &&
			cgds[i].pm_part == part_num) {
			if (do_del) {
				cgds[i].pm = NULL;
				strcpy(cgds[i].pm_name, "");
			}
			return 1;
		}
	for (i = 0; i < MAX_RAID; i++)
		for (ii = 0; ii < MAX_IN_RAID; ii++)
			if (raids[i].enabled &&
				raids[i].pm[ii] == pm_cur &&
				raids[i].pm_part[ii] == part_num) {
					if (do_del)
						raids[i].pm[ii] = NULL;
					return 1;
			}
	for (i = 0; i < MAX_LVM_VG; i++)
		for (ii = 0; ii < MAX_LVM_PV; ii++)
			if (lvms[i].enabled &&
				lvms[i].pv[ii].pm == pm_cur &&
				lvms[i].pv[ii].pm_part == part_num) {
					if (do_del)
						lvms[i].pv[ii].pm = NULL;
					return 1;
			}

	return 0;
}

/* Clean up removed devices */
static int
pm_clean(void)
{
	int count = 0;
	pm_devs_t *pm_i;

	SLIST_FOREACH(pm_i, &pm_head, l)
		if (! pm_i->found) {
			count++;
			SLIST_REMOVE(&pm_head, pm_i, pm_devs_t, l);
			free(pm_i);
		}
	return count;
}

void 
pm_make_bsd_partitions(pm_devs_t *pm_cur)
{
	int i;
	partinfo baklabel[MAXPARTITIONS];
	memcpy(baklabel, &pm_cur->bsdlabel, sizeof baklabel);

	pm_cur->unsaved = 1;
	layoutkind = LY_SETNEW;
	md_make_bsd_partitions();
	pm_cur->bootable = (pm_cur->bsdlabel[pm_cur->rootpart].pi_flags & PIF_MOUNT) ?
		1 : 0;
	for (i = 0; i < MAXPARTITIONS; i++)
		if (pm_cur->bsdlabel[i].pi_fstype != baklabel[i].pi_fstype ||
				pm_cur->bsdlabel[i].lvmpv != baklabel[i].lvmpv)
			pm_partusage(pm_cur, i, 1);
}

void
pm_setfstype(pm_devs_t *pm_cur, int part_num, int fstype)
{
	pm_cur->bsdlabel[part_num].pi_mount[0] = '\0';
	pm_cur->bsdlabel[part_num].pi_flags = 0;
	pm_cur->bsdlabel[part_num].lvmpv = 0;
	pm_cur->bsdlabel[part_num].pi_fstype = fstype;
	return;	
}

static void
pm_select(pm_devs_t *pm_devs_in)
{
	pm = pm_devs_in;
	if (logfp)
		(void)fprintf(logfp, "Partman device: %s\n", pm->diskdev);
	return;
}

void
pm_rename(pm_devs_t *pm_cur)
{
	pm_select(pm_cur);
	msg_prompt_win(MSG_packname, -1, 18, 0, 0, pm_cur->bsddiskname,
		pm_cur->bsddiskname, sizeof pm_cur->bsddiskname);
#ifndef NO_DISKLABEL
	(void) savenewlabel(pm_cur->bsdlabel, MAXPARTITIONS);
#endif
	return;
}

int
pm_editpart(int part_num)
{
	partinfo backup = pm->bsdlabel[part_num];

	edit_ptn(&(struct menudesc){.cursel = part_num}, NULL);
	if (checkoverlap(pm->bsdlabel, MAXPARTITIONS, PART_D, PART_C)) {
		msg_display_add(MSG_cantsave);
		process_menu(MENU_ok, NULL);
		pm->bsdlabel[part_num] = backup;
		return -1;
	}
	pm->unsaved = 1;
	return 0;
}

/* Safe erase of disk */
int
pm_shred(pm_devs_t *pm_cur, int part, int shredtype)
{
	int error = -1;
	char dev[SSTRSIZE];

	pm_getdevstring(dev, SSTRSIZE, pm_cur, part);
	switch(shredtype) {
		case SHRED_ZEROS:
			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
				"dd of=/dev/%s if=/dev/zero bs=1m progress=100 msgfmt=human",
				dev);
			break;
		case SHRED_RANDOM:
			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
				"dd of=/dev/%s if=/dev/urandom bs=1m progress=100 msgfmt=human",
				dev);
			break;
		case SHRED_CRYPTO:
			error += run_program(RUN_DISPLAY | RUN_PROGRESS,
				"sh -c 'cgdconfig -s cgd0 /dev/%s aes-cbc 128 < /dev/urandom'",
				dev); /* XXX: cgd0?! */
			if (! error) {
				error += run_program(RUN_DISPLAY | RUN_PROGRESS,
					"dd of=/dev/rcgd0d if=/dev/urandom bs=1m progress=100 msgfmt=human");
				error += run_program(RUN_DISPLAY | RUN_PROGRESS,
					"cgdconfig -u cgd0");
			}
			break;
		default:
			return -1;
	}
	pm_partusage(pm_cur, -1, 1);
	memset(&pm_cur->oldlabel, 0, sizeof pm_cur->oldlabel);
	memset(&pm_cur->bsdlabel, 0, sizeof pm_cur->bsdlabel);
	return error;
}

static int
pm_mountall_sort(const void *a, const void *b)
{
	return strcmp(mnts[*(const int *)a].on, mnts[*(const int *)b].on);
}

/* Mount all available partitions */
static int
pm_mountall(void)
{
	int num_devs = 0;
	int mnts_order[MAX_MNTS];
	int i, ii, error, ok;
	char dev[SSTRSIZE]; dev[0] = '\0';
	pm_devs_t *pm_i;
	
	localfs_dev[0] = '\0';
	memset(&mnts, 0, sizeof mnts);

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
				if (pm_i->isspecial)
					pm_i->bootable = 1;
				/* Use disk with / as a default if the user has 
				the sets on a local disk */
				strlcpy(localfs_dev, pm_i->diskdev, SSTRSIZE);
			}
			num_devs++;
			ok = 1;
		}
		if (ok)
			md_pre_mount();
	}
	if (strlen(localfs_dev) < 1) {
		msg_display(MSG_noroot);
		process_menu(MENU_ok, NULL);
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

/* Mount partition bypassing ordinary order */
static int
pm_mount(pm_devs_t *pm_cur, int part_num)
{
	int error = 0;
	char buf[MOUNTLEN];

	if (strlen(pm_cur->bsdlabel[part_num].mounted) > 0)
		return 0;

	snprintf(buf, MOUNTLEN, "/tmp/%s%c", pm_cur->diskdev, part_num + 'a');
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
	return error;
}

void
pm_umount(pm_devs_t *pm_cur, int part_num)
{
	char buf[SSTRSIZE]; buf[0] = '\0';
	pm_getdevstring(buf, SSTRSIZE, pm_cur, part_num);

	if (run_program(RUN_DISPLAY | RUN_PROGRESS,
			"umount -f /dev/%s", buf) == 0) {
		pm_cur->bsdlabel[part_num].mounted[0] = '\0';
		if (pm_cur->blocked > 0)
			pm_cur->blocked--;
	}
	return;
}

int
pm_unconfigure(pm_devs_t *pm_cur)
{
	int i, error = 0, num = 0;
	if (! strncmp(pm_cur->diskdev, "cgd", 3)) {
 		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "cgdconfig -u %s", pm_cur->diskdev);
 		if (! error && pm_cur->refdev != NULL) {
			((cgds_t*)pm_cur->refdev)->pm->blocked--;
			((cgds_t*)pm_cur->refdev)->blocked = 0;
 		}
 	} else if (! strncmp(pm_cur->diskdev, "vnd", 3)) {
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "vnconfig -u %s", pm_cur->diskdev);
 		if (! error && pm_cur->refdev != NULL) {
			((vnds_t*)pm_cur->refdev)->pm->blocked--;
			((vnds_t*)pm_cur->refdev)->blocked = 0;
 		}
	} else if (! strncmp(pm_cur->diskdev, "raid", 4)) {
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "raidctl -u %s", pm_cur->diskdev);
		if (! error && pm_cur->refdev != NULL) {
			((raids_t*)pm_cur->refdev)->blocked = 0;
			for (i = 0; i < MAX_IN_RAID; i++)
				if (((raids_t*)pm_cur->refdev)->pm[i] != NULL)
					((raids_t*)pm_cur->refdev)->pm[i]->blocked--;
		}
	} else if (! strncmp(pm_cur->diskdev, "dk", 2)) {
		if (pm_cur->refdev == NULL)
			return -2;
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "dkctl %s delwedge %s",
			((pm_devs_t*)pm_cur->refdev)->diskdev, pm_cur->diskdev);
		if (! error) {
			if (pm_cur->refdev != NULL && ((pm_devs_t*)pm_cur->refdev)->blocked > 0)
				((pm_devs_t*)pm_cur->refdev)->blocked--;
			sscanf(pm_cur->diskdev, "dk%d", &num);
			if (num >= 0 && num < MAX_WEDGES)
				wedges[num].allocated = 0;
		}
	} /* XXX: lvm */
	else
		error = run_program(RUN_DISPLAY | RUN_PROGRESS, "eject -t disk /dev/%sd",
			pm_cur->diskdev);
	if (!error)
		pm_cur->found = 0;
	return error;
}

/* Last checks before leaving partition manager */
static int
pm_lastcheck(void)
{
	FILE *file_tmp = fopen(concat_paths(targetroot_mnt, "/etc/fstab"), "r");
	if (file_tmp == NULL)
		return 1;
	fclose(file_tmp);
	return 0;
}

/* Are there unsaved changes? */
static int
pm_needsave(void)
{
	pm_devs_t *pm_i;
	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->unsaved) {
			/* Oops, we have unsaved changes */
			changed = 1;
			msg_display(MSG_saveprompt);
			process_menu(MENU_yesno, NULL);
			return (yesno);
		}
	return 0;
}

/* Write all changes to disk */
static int
pm_commit(menudesc *m, void *arg)
{
	int retcode;
	pm_devs_t *pm_i;
	if (m != NULL && arg != NULL)
		((part_entry_t *)arg)[0].retvalue = m->cursel + 1;

	pm_gpt_commit();

	SLIST_FOREACH(pm_i, &pm_head, l) {
		if (! pm_i->unsaved)
			continue;
		pm_select(pm_i);

		if (pm_i->isspecial) {
			if (make_filesystems() != 0) {
				if (logfp)
					fprintf(logfp, "Special disk %s preparing error\n", pm_i->diskdev);
				continue;
			}
			pm_i->unsaved = 0;
		} else if (! pm_i->gpt) {
#ifndef NO_DISKLABEL
			savenewlabel(pm_i->bsdlabel, MAXPARTITIONS);
#endif
			if (
				write_disklabel() != 0   || /* Write slices table (disklabel) */
				md_post_disklabel() != 0 || /* Enable swap and check badblock */
				make_filesystems() != 0     /* Create filesystems with newfs */
			) {
				/* Oops, something failed... */
				if (logfp)
					fprintf(logfp, "Disk %s preparing error\n", pm_i->diskdev);
				continue;
			}
			pm_i->unsaved = 0;
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
    return 0;
}

static int
pm_savebootsector(void)
{
	pm_devs_t *pm_i;
	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->bootable) {
			if (! strncmp("raid", pm_i->diskdev, 4))
				if (run_program(RUN_DISPLAY | RUN_PROGRESS,
					"raidctl -v -A root %s", pm_i->diskdev) != 0) {
					if (logfp)
						fprintf(logfp, "Error writting RAID bootsector to %s\n",
							pm_i->diskdev);
					continue;
				}
			if (pm_i->gpt && ! pm_i->isspecial) {
				if (pm_i->rootpart < 0 ||
					run_program(RUN_DISPLAY | RUN_PROGRESS,
					"gpt biosboot -i %d %s",
					pm_i->rootpart + 1, pm_i->diskdev) != 0) {
			 		if (logfp)
						fprintf(logfp, "Error writting GPT bootsector to %s\n",
							pm_i->diskdev);
					continue;
				}
			} else {
				pm_select(pm_i);
			 	if (
#ifndef NO_DISKLABEL
				    check_partitions() == 0 ||
#endif
				    md_post_newfs() != 0) {
		 			if (logfp)
						fprintf(logfp, "Error writting bootsector to %s\n",
							pm_i->diskdev);
					continue;
				}
			}
		}
	return 0;
}

/* Function for 'Enter'-menu */
static int
pm_submenu(menudesc *m, void *arg)
{
	int part_num = -1;
	pm_devs_t *pm_cur = NULL;
	((part_entry_t *)arg)[0].retvalue = m->cursel + 1;

	switch (((part_entry_t *)arg)[m->cursel].type) {
		case PM_DISK_T:
		case PM_WEDGE_T:
		case PM_PART_T:
		case PM_SPEC_T:
			if (((part_entry_t *)arg)[m->cursel].dev_ptr != NULL) {
				pm_cur = ((part_entry_t *)arg)[m->cursel].dev_ptr;
				if (pm_cur == NULL) 
					return -1;
				if (pm_cur->blocked) {
					msg_display(MSG_wannaunblock);
					process_menu(MENU_noyes, NULL);
					if (!yesno)
						return -2;
					pm_cur->blocked = 0;
				}
				pm_select(pm_cur);
			}
		default:
			break;
	}

	switch (((part_entry_t *)arg)[m->cursel].type) {
		case PM_DISK_T:
			if (pm_cur != NULL && pm_cur->gpt) {
				process_menu(MENU_pmgptentry, &part_num);
				pm_wedges_fill(pm_cur);
			} else {
				process_menu(MENU_pmdiskentry, &part_num);
			}
			break;
		case PM_WEDGE_T:
		case PM_PART_T:
			part_num = ((part_entry_t *)arg)[m->cursel].dev_num;
			process_menu(MENU_pmpartentry, &part_num);
			if (pm_cur != NULL && pm_cur->gpt)
				pm_wedges_fill(pm_cur);
			break;
		case PM_SPEC_T:
			part_num = 0;
			process_menu(MENU_pmpartentry, &part_num);
			break;
		case PM_RAID_T:
			return pm_edit(PMR_MENU_END, pm_raid_edit_menufmt,
				pm_raid_set_value, pm_raid_check, pm_raid_init,
				NULL, ((part_entry_t *)arg)[m->cursel].dev_ptr, 0, &raids_t_info);
		case PM_VND_T:
			return pm_edit(PMV_MENU_END, pm_vnd_edit_menufmt,
				pm_vnd_set_value, pm_vnd_check, pm_vnd_init,
				NULL, ((part_entry_t *)arg)[m->cursel].dev_ptr, 0, &vnds_t_info);
		case PM_CGD_T:
			return pm_cgd_edit(((part_entry_t *)arg)[m->cursel].dev_ptr,
				NULL);
		case PM_LVM_T:
			return pm_edit(PML_MENU_END, pm_lvm_edit_menufmt,
				pm_lvm_set_value, pm_lvm_check, pm_lvm_init,
				NULL, ((part_entry_t *)arg)[m->cursel].dev_ptr, 0, &lvms_t_info);
		case PM_LVMLV_T:
			return pm_edit(PMLV_MENU_END, pm_lvmlv_edit_menufmt,
				pm_lvmlv_set_value, pm_lvmlv_check, pm_lvmlv_init,
				NULL, ((part_entry_t *)arg)[m->cursel].dev_ptr,
				((part_entry_t *)arg)[m->cursel].dev_ptr_delta, &lv_t_info);
	}
	return 0;
}

/* Functions that generate menu entries text */
static void
pm_menufmt(menudesc *m, int opt, void *arg)
{
	const char *dev_status = "";
	char buf[STRSIZE];
	int part_num = ((part_entry_t *)arg)[opt].dev_num;
	pm_devs_t *pm_cur = ((part_entry_t *)arg)[opt].dev_ptr;

	switch (((part_entry_t *)arg)[opt].type) {
		case PM_DISK_T:
			if (pm_cur->blocked)
				dev_status = msg_string(MSG_pmblocked);
			else if (! pm_cur->unsaved)
				dev_status = msg_string(MSG_pmunchanged);
			else if (pm_cur->bootable)
				dev_status = msg_string(MSG_pmsetboot);
			else
				dev_status = msg_string(MSG_pmused);
			wprintw(m->mw, "%-33.32s %-22.21s %12.12s",
				pm_cur->diskdev_descr,
				(pm_cur->gpt) ?
					msg_string(MSG_pmgptdisk) :
					pm_cur->bsddiskname,
				dev_status);
			break;
		case PM_WEDGE_T:
			part_num = ((part_entry_t *)arg)[opt].wedge_num;
			snprintf(buf, STRSIZE, "dk%d: %s",
				part_num,
				wedges[part_num].pm->bsdlabel[wedges[part_num].ptn].pi_mount);
			wprintw(m->mw, "   %-30.29s %-22.21s %11uM",
				buf,
				(wedges[part_num].pm->bsdlabel[wedges[part_num].ptn].lvmpv) ? 
					"lvm pv" :
					getfslabelname(wedges[part_num].pm->bsdlabel[wedges[part_num].
						ptn].pi_fstype),
				wedges[part_num].pm->bsdlabel[wedges[part_num].ptn].pi_size /
					(MEG / pm_cur->sectorsize));
			break;
		case PM_PART_T:
			snprintf(buf, STRSIZE, "%s%c: %s %s",
				pm_cur->diskdev,
				'a' + part_num,
				pm_cur->bsdlabel[part_num].pi_mount,
				(strlen(pm_cur->bsdlabel[part_num].mounted) > 0) ? msg_string(MSG_pmmounted) : 
					(part_num == PART_B ||
						strlen (pm_cur->bsdlabel[part_num].pi_mount ) < 1 ||
						pm_cur->bsdlabel[part_num].pi_flags & PIF_MOUNT) ?
						"" : msg_string(MSG_pmunused));
			wprintw(m->mw, "   %-30.29s %-22.21s %11uM",
				buf,
				(pm_cur->bsdlabel[part_num].lvmpv) ? 
					"lvm pv" :
					getfslabelname(pm_cur->bsdlabel[part_num].pi_fstype),
				pm_cur->bsdlabel[part_num].pi_size / (MEG / pm_cur->sectorsize));
			break;
		case PM_SPEC_T:
			snprintf(buf, STRSIZE, "%s: %s",
				pm_cur->diskdev_descr, pm_cur->bsdlabel[0].pi_mount);
			wprintw(m->mw, "%-33.32s %-22.21s %11uM", buf,
				getfslabelname(pm_cur->bsdlabel[0].pi_fstype),
				pm_cur->bsdlabel[0].pi_size / (MEG / pm_cur->sectorsize));
			break;
		case PM_RAID_T:
			pm_raid_menufmt(m, opt, arg);
			break;
		case PM_VND_T:
			pm_vnd_menufmt(m, opt, arg);
			break;
		case PM_CGD_T:
			pm_cgd_menufmt(m, opt, arg);
			break;
		case PM_LVM_T:
			pm_lvm_menufmt(m, opt, arg);
			break;
		case PM_LVMLV_T:
			pm_lvmlv_menufmt(m, opt, arg);
			break;
	}
	return;
}

/* Submenu for RAID/LVM/CGD/VND */
static void
pm_upddevlist_adv(menudesc *m, void *arg, int *i,
	pm_upddevlist_adv_t *d)
{
	int ii;
	if (d->create_msg != NULL) {
		/* We want to have menu entry that creates a new device */
		((part_entry_t *)arg)[*i].type = d->pe_type;
		((part_entry_t *)arg)[*i].dev_ptr = NULL;
		((part_entry_t *)arg)[*i].dev_ptr_delta = d->s->parent_size * d->sub_num;
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
		changed = 1;
		m->opts[*i] = (struct menu_ent) {
			.opt_name = NULL,
			.opt_action = pm_submenu,
		};
		((part_entry_t *)arg)[*i].type = d->pe_type;
		((part_entry_t *)arg)[*i].dev_ptr = (char*)d->s->entry_first +
			d->s->entry_size * ii + d->s->parent_size * d->sub_num;
		(*i)++;
		/* We should show submenu for current entry */
		if (d->sub != NULL) {
			d->sub->sub_num = ii;
			pm_upddevlist_adv(m, arg, i, d->sub);
		}
	}
	return;
}

/* Update partman main menu with devices list */
static int
pm_upddevlist(menudesc *m, void *arg)
{
	int i = 0, ii;
	pm_devs_t *pm_i;

	if (arg != NULL)
		((part_entry_t *)arg)[0].retvalue = m->cursel + 1;

	changed = 0;
	/* Mark all devices as not found */
	SLIST_FOREACH(pm_i, &pm_head, l)
		if (pm_i->found > 0)
			pm_i->found = 0;
	/* Detect all present devices */
	(void)find_disks("partman");
	pm_lvm_find();
	pm_clean();

	if (m == NULL || arg == NULL)
		return -1;

	SLIST_FOREACH(pm_i, &pm_head, l) {
		m->opts[i].opt_name = NULL;
		m->opts[i].opt_action = pm_submenu;
		((part_entry_t *)arg)[i].dev_ptr = pm_i;
		((part_entry_t *)arg)[i].dev_num = -1;
		if (pm_i->isspecial)
			((part_entry_t *)arg)[i].type = PM_SPEC_T;
		else {
			((part_entry_t *)arg)[i].type = PM_DISK_T;
			if (pm_i->gpt) {
				pm_wedges_fill(pm_i);
				for (ii = 0; ii < MAX_WEDGES; ii++)
					if (wedges[ii].pm == pm_i) {
						i++;
						m->opts[i].opt_name = NULL;
						m->opts[i].opt_action = pm_submenu;
						((part_entry_t *)arg)[i].dev_ptr = pm_i;
						((part_entry_t *)arg)[i].dev_num = wedges[ii].ptn;
						((part_entry_t *)arg)[i].wedge_num = ii;
						((part_entry_t *)arg)[i].type = PM_WEDGE_T;
					}
			} else {
				for (ii = 0; ii < MAXPARTITIONS; ii++)
					if (pm_i->bsdlabel[ii].pi_fstype != FS_UNUSED) {
						i++;
						m->opts[i].opt_name = NULL;
						m->opts[i].opt_action = pm_submenu;
						((part_entry_t *)arg)[i].dev_ptr = pm_i;
						((part_entry_t *)arg)[i].dev_num = ii;
						((part_entry_t *)arg)[i].type = PM_PART_T;
					}
			}
		}
		i++;
	}
	if (have_cgd) {
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_cgd, PM_CGD_T, &cgds_t_info, 0, NULL});
	}
	if (have_lvm) {
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_cnd, PM_VND_T, &vnds_t_info, 0, NULL});
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_vg, PM_LVM_T, &lvms_t_info, 0,
		    &(pm_upddevlist_adv_t) {MSG_create_lv, PM_LVMLV_T, &lv_t_info, 0, NULL}});
	}
	if (have_raid) {
		pm_upddevlist_adv(m, arg, &i,
		    &(pm_upddevlist_adv_t) {MSG_create_raid, PM_RAID_T, &raids_t_info, 0, NULL});
	}

	m->opts[i++] = (struct menu_ent) {
		.opt_name = MSG_updpmlist,
		.opt_action = pm_upddevlist,
	};
	m->opts[i  ] = (struct menu_ent) {
		.opt_name = MSG_savepm,
		.opt_action = pm_commit,
	};
	for (ii = 0; ii <= i; ii++) {
		m->opts[ii].opt_menu = OPT_NOMENU;
		m->opts[ii].opt_flags = OPT_EXIT;
	}

	if (((part_entry_t *)arg)[0].retvalue >= 0)
		m->cursel = ((part_entry_t *)arg)[0].retvalue - 1;
	return i;
}

static void
pm_menuin(menudesc *m, void *arg)
{
	if (cursel > m->numopts)
		m->cursel = m->numopts;
	else if (cursel < 0)
		m->cursel = 0;
	else
		m->cursel = cursel;
}

static void
pm_menuout(menudesc *m, void *arg)
{
	cursel = m->cursel;
}

/* Main partman function */
int
partman(void)
{
	int menu_no, menu_num_entries;
	static int firstrun = 1;
	menu_ent menu_entries[MAX_ENTRIES+6];
	part_entry_t args[MAX_ENTRIES];

	if (firstrun) {
		have_raid = binary_available("raidctl");
		have_vnd = binary_available("vnconfig");
		have_cgd = binary_available("cgdconfig");
		have_lvm = binary_available("lvm");
		have_gpt = binary_available("gpt");
		have_dk = binary_available("dkctl");

		if (!have_raid)
			remove_raid_options();
		if (!have_lvm)
			remove_lvm_options();
		if (!have_gpt)
			remove_gpt_options();
		if (!have_cgd)
			remove_cgd_options();

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

		memset(&raids, 0, sizeof raids);
		memset(&cgds, 0, sizeof cgds);
		memset(&vnds, 0, sizeof vnds);
		memset(&lvms, 0, sizeof lvms);
		cursel = 0;
		changed = 0;
		firstrun = 0;
	}

	do {
		menu_num_entries = pm_upddevlist(&(menudesc){.opts = menu_entries}, args);
		menu_no = new_menu(MSG_partman_header,
			menu_entries, menu_num_entries+1, 1, 1, 0, 75, /* Fixed width */
			MC_ALWAYS_SCROLL | MC_NOBOX | MC_NOCLEAR,
			pm_menuin, pm_menufmt, pm_menuout, NULL, MSG_finishpm);
		if (menu_no == -1)
			args[0].retvalue = -1;
		else {
			args[0].retvalue = 0;
			clear();
			refresh();
			process_menu(menu_no, &args);
			free_menu(menu_no);
		}

		if (args[0].retvalue == 0) {
			if (pm_needsave())
				pm_commit(NULL, NULL);
			if (pm_mountall() != 0 ||
				make_fstab() != 0 ||
				pm_lastcheck() != 0 ||
				pm_savebootsector() != 0) {
					msg_display(MSG_wannatry);
					process_menu(MENU_yesno, NULL);
					args[0].retvalue = (yesno) ? 1:-1;
			}
		}
	} while (args[0].retvalue > 0);
	
	/* retvalue <0 - error, retvalue ==0 - user quits, retvalue >0 - all ok */
	return (args[0].retvalue >= 0)?0:-1;
}
