/*	$NetBSD: display.c,v 1.1.1.1.2.1 2009/05/13 18:52:42 jym Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "metadata.h"
#include "display.h"
#include "activate.h"
#include "toolcontext.h"
#include "segtype.h"

#define SIZE_BUF 128

typedef enum { SIZE_LONG = 0, SIZE_SHORT = 1, SIZE_UNIT = 2 } size_len_t;

static const struct {
	alloc_policy_t alloc;
	const char str[12]; /* must be changed when size extends 11 chars */
} _policies[] = {
	{
	ALLOC_CONTIGUOUS, "contiguous"}, {
	ALLOC_CLING, "cling"}, {
	ALLOC_NORMAL, "normal"}, {
	ALLOC_ANYWHERE, "anywhere"}, {
	ALLOC_INHERIT, "inherit"}
};

static const int _num_policies = sizeof(_policies) / sizeof(*_policies);

uint64_t units_to_bytes(const char *units, char *unit_type)
{
	char *ptr = NULL;
	uint64_t v;

	if (isdigit(*units)) {
		v = (uint64_t) strtod(units, &ptr);
		if (ptr == units)
			return 0;
		units = ptr;
	} else
		v = 1;

	if (v == 1)
		*unit_type = *units;
	else
		*unit_type = 'U';

	switch (*units) {
	case 'h':
	case 'H':
		v = UINT64_C(1);
		*unit_type = *units;
		break;
	case 's':
		v *= SECTOR_SIZE;
		break;
	case 'b':
	case 'B':
		v *= UINT64_C(1);
		break;
#define KILO UINT64_C(1024)
	case 'k':
		v *= KILO;
		break;
	case 'm':
		v *= KILO * KILO;
		break;
	case 'g':
		v *= KILO * KILO * KILO;
		break;
	case 't':
		v *= KILO * KILO * KILO * KILO;
		break;
	case 'p':
		v *= KILO * KILO * KILO * KILO * KILO;
		break;
	case 'e':
		v *= KILO * KILO * KILO * KILO * KILO * KILO;
		break;
#undef KILO
#define KILO UINT64_C(1000)
	case 'K':
		v *= KILO;
		break;
	case 'M':
		v *= KILO * KILO;
		break;
	case 'G':
		v *= KILO * KILO * KILO;
		break;
	case 'T':
		v *= KILO * KILO * KILO * KILO;
		break;
	case 'P':
		v *= KILO * KILO * KILO * KILO * KILO;
		break;
	case 'E':
		v *= KILO * KILO * KILO * KILO * KILO * KILO;
		break;
#undef KILO
	default:
		return 0;
	}

	if (*(units + 1))
		return 0;

	return v;
}

const char *get_alloc_string(alloc_policy_t alloc)
{
	int i;

	for (i = 0; i < _num_policies; i++)
		if (_policies[i].alloc == alloc)
			return _policies[i].str;

	return NULL;
}

alloc_policy_t get_alloc_from_string(const char *str)
{
	int i;

	for (i = 0; i < _num_policies; i++)
		if (!strcmp(_policies[i].str, str))
			return _policies[i].alloc;

	/* Special case for old metadata */
	if(!strcmp("next free", str))
		return ALLOC_NORMAL;

	log_error("Unrecognised allocation policy %s", str);
	return ALLOC_INVALID;
}

/* Size supplied in sectors */
static const char *_display_size(const struct cmd_context *cmd,
				 uint64_t size, size_len_t sl)
{
	int s;
	int suffix = 1, precision;
	uint64_t byte = UINT64_C(0);
	uint64_t units = UINT64_C(1024);
	char *size_buf = NULL;
	const char * const size_str[][3] = {
		{" Exabyte", " EB", "E"},
		{" Petabyte", " PB", "P"},
		{" Terabyte", " TB", "T"},
		{" Gigabyte", " GB", "G"},
		{" Megabyte", " MB", "M"},
		{" Kilobyte", " KB", "K"},
		{"", "", ""},
		{" Byte    ", " B ", "B"},
		{" Units   ", " Un", "U"},
		{" Sectors ", " Se", "S"},
		{"         ", "   ", " "},
	};

	if (!(size_buf = dm_pool_alloc(cmd->mem, SIZE_BUF))) {
		log_error("no memory for size display buffer");
		return "";
	}

	suffix = cmd->current_settings.suffix;

	for (s = 0; s < 10; s++)
		if (toupper((int) cmd->current_settings.unit_type) ==
		    *size_str[s][2])
			break;

	if (size == UINT64_C(0)) {
		sprintf(size_buf, "0%s", suffix ? size_str[s][sl] : "");
		return size_buf;
	}

	size *= UINT64_C(512);

	if (s < 10)
		byte = cmd->current_settings.unit_factor;
	else {
		suffix = 1;
		if (cmd->current_settings.unit_type == 'H')
			units = UINT64_C(1000);
		else
			units = UINT64_C(1024);
		byte = units * units * units * units * units * units;
		s = 0;
		while (size_str[s] && size < byte)
			s++, byte /= units;
	}

	/* FIXME Make precision configurable */
	switch(toupper((int) cmd->current_settings.unit_type)) {
	case 'B':
	case 'S':
		precision = 0;
		break;
	default:
		precision = 2;
	}

	snprintf(size_buf, SIZE_BUF - 1, "%.*f%s", precision,
		 (double) size / byte, suffix ? size_str[s][sl] : "");

	return size_buf;
}

const char *display_size_long(const struct cmd_context *cmd, uint64_t size)
{
	return _display_size(cmd, size, SIZE_LONG);
}

const char *display_size_units(const struct cmd_context *cmd, uint64_t size)
{
	return _display_size(cmd, size, SIZE_UNIT);
}

const char *display_size(const struct cmd_context *cmd, uint64_t size)
{
	return _display_size(cmd, size, SIZE_SHORT);
}

void pvdisplay_colons(const struct physical_volume *pv)
{
	char uuid[64] __attribute((aligned(8)));

	if (!pv)
		return;

	if (!id_write_format(&pv->id, uuid, sizeof(uuid))) {
		stack;
		return;
	}

	log_print("%s:%s:%" PRIu64 ":-1:%u:%u:-1:%" PRIu32 ":%u:%u:%u:%s",
		  pv_dev_name(pv), pv->vg_name, pv->size,
		  /* FIXME pv->pv_number, Derive or remove? */
		  pv->status,	/* FIXME Support old or new format here? */
		  pv->status & ALLOCATABLE_PV,	/* FIXME remove? */
		  /* FIXME pv->lv_cur, Remove? */
		  pv->pe_size / 2,
		  pv->pe_count,
		  pv->pe_count - pv->pe_alloc_count,
		  pv->pe_alloc_count, *uuid ? uuid : "none");

	return;
}

void pvdisplay_segments(const struct physical_volume *pv)
{
	const struct pv_segment *pvseg;

	if (pv->pe_size)
		log_print("--- Physical Segments ---");

	dm_list_iterate_items(pvseg, &pv->segments) {
		log_print("Physical extent %u to %u:",
			  pvseg->pe, pvseg->pe + pvseg->len - 1);

		if (pvseg_is_allocated(pvseg)) {
			log_print("  Logical volume\t%s%s/%s",
				  pvseg->lvseg->lv->vg->cmd->dev_dir,
				  pvseg->lvseg->lv->vg->name,
				  pvseg->lvseg->lv->name);
			log_print("  Logical extents\t%d to %d",
				  pvseg->lvseg->le, pvseg->lvseg->le +
				  pvseg->lvseg->len - 1);
		} else
			log_print("  FREE");
	}

	log_print(" ");
	return;
}

/* FIXME Include label fields */
void pvdisplay_full(const struct cmd_context *cmd,
		    const struct physical_volume *pv,
		    void *handle __attribute((unused)))
{
	char uuid[64] __attribute((aligned(8)));
	const char *size;

	uint32_t pe_free;
	uint64_t data_size, pvsize, unusable;

	if (!pv)
		return;

	if (!id_write_format(&pv->id, uuid, sizeof(uuid))) {
		stack;
		return;
	}

	log_print("--- %sPhysical volume ---", pv->pe_size ? "" : "NEW ");
	log_print("PV Name               %s", pv_dev_name(pv));
	log_print("VG Name               %s%s",
		  is_orphan(pv) ? "" : pv->vg_name,
		  pv->status & EXPORTED_VG ? " (exported)" : "");

	data_size = (uint64_t) pv->pe_count * pv->pe_size;
	if (pv->size > data_size + pv->pe_start) {
		pvsize = pv->size;
		unusable = pvsize - data_size;
	} else {
		pvsize = data_size + pv->pe_start;
		unusable = pvsize - pv->size;
	}

	size = display_size(cmd, pvsize);
	if (data_size)
		log_print("PV Size               %s / not usable %s",	/*  [LVM: %s]", */
			  size, display_size(cmd, unusable));
	else
		log_print("PV Size               %s", size);

	/* PV number not part of LVM2 design
	   log_print("PV#                   %u", pv->pv_number);
	 */

	pe_free = pv->pe_count - pv->pe_alloc_count;
	if (pv->pe_count && (pv->status & ALLOCATABLE_PV))
		log_print("Allocatable           yes %s",
			  (!pe_free && pv->pe_count) ? "(but full)" : "");
	else
		log_print("Allocatable           NO");

	/* LV count is no longer available when displaying PV
	   log_print("Cur LV                %u", vg->lv_count);
	 */
	log_print("PE Size (KByte)       %" PRIu32, pv->pe_size / 2);
	log_print("Total PE              %u", pv->pe_count);
	log_print("Free PE               %" PRIu32, pe_free);
	log_print("Allocated PE          %u", pv->pe_alloc_count);
	log_print("PV UUID               %s", *uuid ? uuid : "none");
	log_print(" ");

	return;
}

int pvdisplay_short(const struct cmd_context *cmd __attribute((unused)),
		    const struct volume_group *vg __attribute((unused)),
		    const struct physical_volume *pv,
		    void *handle __attribute((unused)))
{
	char uuid[64] __attribute((aligned(8)));

	if (!pv)
		return 0;

	if (!id_write_format(&pv->id, uuid, sizeof(uuid)))
		return_0;

	log_print("PV Name               %s     ", pv_dev_name(pv));
	/* FIXME  pv->pv_number); */
	log_print("PV UUID               %s", *uuid ? uuid : "none");
	log_print("PV Status             %sallocatable",
		  (pv->status & ALLOCATABLE_PV) ? "" : "NOT ");
	log_print("Total PE / Free PE    %u / %u",
		  pv->pe_count, pv->pe_count - pv->pe_alloc_count);

	log_print(" ");
	return 0;
}

void lvdisplay_colons(const struct logical_volume *lv)
{
	int inkernel;
	struct lvinfo info;
	inkernel = lv_info(lv->vg->cmd, lv, &info, 1, 0) && info.exists;

	log_print("%s%s/%s:%s:%d:%d:-1:%d:%" PRIu64 ":%d:-1:%d:%d:%d:%d",
		  lv->vg->cmd->dev_dir,
		  lv->vg->name,
		  lv->name,
		  lv->vg->name,
		  (lv->status & (LVM_READ | LVM_WRITE)) >> 8, inkernel ? 1 : 0,
		  /* FIXME lv->lv_number,  */
		  inkernel ? info.open_count : 0, lv->size, lv->le_count,
		  /* FIXME Add num allocated to struct! lv->lv_allocated_le, */
		  (lv->alloc == ALLOC_CONTIGUOUS ? 2 : 0), lv->read_ahead,
		  inkernel ? info.major : -1, inkernel ? info.minor : -1);
	return;
}

int lvdisplay_full(struct cmd_context *cmd,
		   const struct logical_volume *lv,
		   void *handle __attribute((unused)))
{
	struct lvinfo info;
	int inkernel, snap_active = 0;
	char uuid[64] __attribute((aligned(8)));
	struct lv_segment *snap_seg = NULL;
	float snap_percent;	/* fused, fsize; */

	if (!id_write_format(&lv->lvid.id[1], uuid, sizeof(uuid)))
		return_0;

	inkernel = lv_info(cmd, lv, &info, 1, 1) && info.exists;

	log_print("--- Logical volume ---");

	log_print("LV Name                %s%s/%s", lv->vg->cmd->dev_dir,
		  lv->vg->name, lv->name);
	log_print("VG Name                %s", lv->vg->name);

	log_print("LV UUID                %s", uuid);

	log_print("LV Write Access        %s",
		  (lv->status & LVM_WRITE) ? "read/write" : "read only");

	if (lv_is_origin(lv)) {
		log_print("LV snapshot status     source of");

		dm_list_iterate_items_gen(snap_seg, &lv->snapshot_segs,
				       origin_list) {
			if (inkernel &&
			    (snap_active = lv_snapshot_percent(snap_seg->cow,
							       &snap_percent)))
				if (snap_percent < 0 || snap_percent >= 100)
					snap_active = 0;
			log_print("                       %s%s/%s [%s]",
				  lv->vg->cmd->dev_dir, lv->vg->name,
				  snap_seg->cow->name,
				  (snap_active > 0) ? "active" : "INACTIVE");
		}
		snap_seg = NULL;
	} else if ((snap_seg = find_cow(lv))) {
		if (inkernel &&
		    (snap_active = lv_snapshot_percent(snap_seg->cow,
						       &snap_percent)))
			if (snap_percent < 0 || snap_percent >= 100)
				snap_active = 0;

		log_print("LV snapshot status     %s destination for %s%s/%s",
			  (snap_active > 0) ? "active" : "INACTIVE",
			  lv->vg->cmd->dev_dir, lv->vg->name,
			  snap_seg->origin->name);
	}

	if (inkernel && info.suspended)
		log_print("LV Status              suspended");
	else
		log_print("LV Status              %savailable",
			  inkernel ? "" : "NOT ");

/********* FIXME lv_number
    log_print("LV #                   %u", lv->lv_number + 1);
************/

	if (inkernel)
		log_print("# open                 %u", info.open_count);

	log_print("LV Size                %s",
		  display_size(cmd,
			       snap_seg ? snap_seg->origin->size : lv->size));

	log_print("Current LE             %u",
		  snap_seg ? snap_seg->origin->le_count : lv->le_count);

	if (snap_seg) {
		log_print("COW-table size         %s",
			  display_size(cmd, (uint64_t) lv->size));
		log_print("COW-table LE           %u", lv->le_count);

		if (snap_active)
			log_print("Allocated to snapshot  %.2f%% ", snap_percent);	

		log_print("Snapshot chunk size    %s",
			  display_size(cmd, (uint64_t) snap_seg->chunk_size));
	}

	log_print("Segments               %u", dm_list_size(&lv->segments));

/********* FIXME Stripes & stripesize for each segment
	log_print("Stripe size (KByte)    %u", lv->stripesize / 2);
***********/

	log_print("Allocation             %s", get_alloc_string(lv->alloc));
	if (lv->read_ahead == DM_READ_AHEAD_AUTO)
		log_print("Read ahead sectors     auto");
	else if (lv->read_ahead == DM_READ_AHEAD_NONE)
		log_print("Read ahead sectors     0");
	else
		log_print("Read ahead sectors     %u", lv->read_ahead);

	if (inkernel && lv->read_ahead != info.read_ahead)
		log_print("- currently set to     %u", info.read_ahead);

	if (lv->status & FIXED_MINOR) {
		if (lv->major >= 0)
			log_print("Persistent major       %d", lv->major);
		log_print("Persistent minor       %d", lv->minor);
	}

	if (inkernel)
		log_print("Block device           %d:%d", info.major,
			  info.minor);

	log_print(" ");

	return 0;
}

void display_stripe(const struct lv_segment *seg, uint32_t s, const char *pre)
{
	switch (seg_type(seg, s)) {
	case AREA_PV:
		/* FIXME Re-check the conditions for 'Missing' */
		log_print("%sPhysical volume\t%s", pre,
			  seg_pv(seg, s) ?
			  pv_dev_name(seg_pv(seg, s)) :
			    "Missing");

		if (seg_pv(seg, s))
			log_print("%sPhysical extents\t%d to %d", pre,
				  seg_pe(seg, s),
				  seg_pe(seg, s) + seg->area_len - 1);
		break;
	case AREA_LV:
		log_print("%sLogical volume\t%s", pre,
			  seg_lv(seg, s) ?
			  seg_lv(seg, s)->name : "Missing");

		if (seg_lv(seg, s))
			log_print("%sLogical extents\t%d to %d", pre,
				  seg_le(seg, s),
				  seg_le(seg, s) + seg->area_len - 1);
		break;
	case AREA_UNASSIGNED:
		log_print("%sUnassigned area", pre);
	}
}

int lvdisplay_segments(const struct logical_volume *lv)
{
	const struct lv_segment *seg;

	log_print("--- Segments ---");

	dm_list_iterate_items(seg, &lv->segments) {
		log_print("Logical extent %u to %u:",
			  seg->le, seg->le + seg->len - 1);

		log_print("  Type\t\t%s", seg->segtype->ops->name(seg));

		if (seg->segtype->ops->display)
			seg->segtype->ops->display(seg);
	}

	log_print(" ");
	return 1;
}

void vgdisplay_extents(const struct volume_group *vg __attribute((unused)))
{
	return;
}

void vgdisplay_full(const struct volume_group *vg)
{
	uint32_t access_str;
	uint32_t active_pvs;
	char uuid[64] __attribute((aligned(8)));

	active_pvs = vg->pv_count - vg_missing_pv_count(vg);

	log_print("--- Volume group ---");
	log_print("VG Name               %s", vg->name);
	log_print("System ID             %s", vg->system_id);
	log_print("Format                %s", vg->fid->fmt->name);
	if (vg->fid->fmt->features & FMT_MDAS) {
		log_print("Metadata Areas        %d",
			  dm_list_size(&vg->fid->metadata_areas));
		log_print("Metadata Sequence No  %d", vg->seqno);
	}
	access_str = vg->status & (LVM_READ | LVM_WRITE);
	log_print("VG Access             %s%s%s%s",
		  access_str == (LVM_READ | LVM_WRITE) ? "read/write" : "",
		  access_str == LVM_READ ? "read" : "",
		  access_str == LVM_WRITE ? "write" : "",
		  access_str == 0 ? "error" : "");
	log_print("VG Status             %s%sresizable",
		  vg->status & EXPORTED_VG ? "exported/" : "",
		  vg->status & RESIZEABLE_VG ? "" : "NOT ");
	/* vg number not part of LVM2 design
	   log_print ("VG #                  %u\n", vg->vg_number);
	 */
	if (vg_is_clustered(vg)) {
		log_print("Clustered             yes");
		log_print("Shared                %s",
			  vg->status & SHARED ? "yes" : "no");
	}

	log_print("MAX LV                %u", vg->max_lv);
	log_print("Cur LV                %u", displayable_lvs_in_vg(vg));
	log_print("Open LV               %u", lvs_in_vg_opened(vg));
/****** FIXME Max LV Size
      log_print ( "MAX LV Size           %s",
               ( s1 = display_size ( LVM_LV_SIZE_MAX(vg))));
      free ( s1);
*********/
	log_print("Max PV                %u", vg->max_pv);
	log_print("Cur PV                %u", vg->pv_count);
	log_print("Act PV                %u", active_pvs);

	log_print("VG Size               %s",
		  display_size(vg->cmd,
			       (uint64_t) vg->extent_count * vg->extent_size));

	log_print("PE Size               %s",
		  display_size(vg->cmd, (uint64_t) vg->extent_size));

	log_print("Total PE              %u", vg->extent_count);

	log_print("Alloc PE / Size       %u / %s",
		  vg->extent_count - vg->free_count,
		  display_size(vg->cmd,
			       ((uint64_t) vg->extent_count - vg->free_count) *
			       vg->extent_size));

	log_print("Free  PE / Size       %u / %s", vg->free_count,
		  display_size(vg->cmd,
			       (uint64_t) vg->free_count * vg->extent_size));

	if (!id_write_format(&vg->id, uuid, sizeof(uuid))) {
		stack;
		return;
	}

	log_print("VG UUID               %s", uuid);
	log_print(" ");

	return;
}

void vgdisplay_colons(const struct volume_group *vg)
{
	uint32_t active_pvs;
	const char *access_str;
	char uuid[64] __attribute((aligned(8)));

	active_pvs = vg->pv_count - vg_missing_pv_count(vg);

	switch (vg->status & (LVM_READ | LVM_WRITE)) {
		case LVM_READ | LVM_WRITE:
			access_str = "r/w";
			break;
		case LVM_READ:
			access_str = "r";
			break;
		case LVM_WRITE:
			access_str = "w";
			break;
		default:
			access_str = "";
	}

	if (!id_write_format(&vg->id, uuid, sizeof(uuid))) {
		stack;
		return;
	}

	log_print("%s:%s:%d:-1:%u:%u:%u:-1:%u:%u:%u:%" PRIu64 ":%" PRIu32
		  ":%u:%u:%u:%s",
		vg->name,
		access_str,
		vg->status,
		/* internal volume group number; obsolete */
		vg->max_lv,
		displayable_lvs_in_vg(vg),
		lvs_in_vg_opened(vg),
		/* FIXME: maximum logical volume size */
		vg->max_pv,
		vg->pv_count,
		active_pvs,
		(uint64_t) vg->extent_count * (vg->extent_size / 2),
		vg->extent_size / 2,
		vg->extent_count,
		vg->extent_count - vg->free_count,
		vg->free_count,
		uuid[0] ? uuid : "none");
	return;
}

void vgdisplay_short(const struct volume_group *vg)
{
	log_print("\"%s\" %-9s [%-9s used / %s free]", vg->name,
/********* FIXME if "open" print "/used" else print "/idle"???  ******/
		  display_size(vg->cmd,
			       (uint64_t) vg->extent_count * vg->extent_size),
		  display_size(vg->cmd,
			       ((uint64_t) vg->extent_count -
				vg->free_count) * vg->extent_size),
		  display_size(vg->cmd,
			       (uint64_t) vg->free_count * vg->extent_size));
	return;
}

void display_formats(const struct cmd_context *cmd)
{
	const struct format_type *fmt;

	dm_list_iterate_items(fmt, &cmd->formats) {
		log_print("%s", fmt->name);
	}
}

void display_segtypes(const struct cmd_context *cmd)
{
	const struct segment_type *segtype;

	dm_list_iterate_items(segtype, &cmd->segtypes) {
		log_print("%s", segtype->name);
	}
}

char yes_no_prompt(const char *prompt, ...)
{
	int c = 0, ret = 0;
	va_list ap;

	sigint_allow();
	do {
		if (c == '\n' || !c) {
			va_start(ap, prompt);
			vprintf(prompt, ap);
			va_end(ap);
		}

		if ((c = getchar()) == EOF) {
			ret = 'n';
			break;
		}

		c = tolower(c);
		if ((c == 'y') || (c == 'n'))
			ret = c;
	} while (!ret || c != '\n');

	sigint_restore();

	if (c != '\n')
		printf("\n");

	return ret;
}

