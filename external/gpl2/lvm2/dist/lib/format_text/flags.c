/*	$NetBSD: flags.c,v 1.1.1.2 2009/12/02 00:26:29 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2006 Red Hat, Inc. All rights reserved.
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
#include "import-export.h"
#include "lvm-string.h"

/*
 * Bitsets held in the 'status' flags get
 * converted into arrays of strings.
 */
struct flag {
	const int mask;
	const char *description;
	int kind;
};

static struct flag _vg_flags[] = {
	{EXPORTED_VG, "EXPORTED", STATUS_FLAG},
	{RESIZEABLE_VG, "RESIZEABLE", STATUS_FLAG},
	{PVMOVE, "PVMOVE", STATUS_FLAG},
	{LVM_READ, "READ", STATUS_FLAG},
	{LVM_WRITE, "WRITE", STATUS_FLAG},
	{CLUSTERED, "CLUSTERED", STATUS_FLAG},
	{SHARED, "SHARED", STATUS_FLAG},
	{PARTIAL_VG, NULL, 0},
	{PRECOMMITTED, NULL, 0},
	{0, NULL, 0}
};

static struct flag _pv_flags[] = {
	{ALLOCATABLE_PV, "ALLOCATABLE", STATUS_FLAG},
	{EXPORTED_VG, "EXPORTED", STATUS_FLAG},
	{MISSING_PV, "MISSING", COMPATIBLE_FLAG},
	{0, NULL, 0}
};

static struct flag _lv_flags[] = {
	{LVM_READ, "READ", STATUS_FLAG},
	{LVM_WRITE, "WRITE", STATUS_FLAG},
	{FIXED_MINOR, "FIXED_MINOR", STATUS_FLAG},
	{VISIBLE_LV, "VISIBLE", STATUS_FLAG},
	{PVMOVE, "PVMOVE", STATUS_FLAG},
	{LOCKED, "LOCKED", STATUS_FLAG},
	{MIRROR_NOTSYNCED, "NOTSYNCED", STATUS_FLAG},
	{MIRROR_IMAGE, NULL, 0},
	{MIRROR_LOG, NULL, 0},
	{MIRRORED, NULL, 0},
	{VIRTUAL, NULL, 0},
	{SNAPSHOT, NULL, 0},
	{ACTIVATE_EXCL, NULL, 0},
	{CONVERTING, NULL, 0},
	{PARTIAL_LV, NULL, 0},
	{POSTORDER_FLAG, NULL, 0},
	{VIRTUAL_ORIGIN, NULL, 0},
	{0, NULL, 0}
};

static struct flag *_get_flags(int type)
{
	switch (type & ~STATUS_FLAG) {
	case VG_FLAGS:
		return _vg_flags;

	case PV_FLAGS:
		return _pv_flags;

	case LV_FLAGS:
		return _lv_flags;
	}

	log_error("Unknown flag set requested.");
	return NULL;
}

/*
 * Converts a bitset to an array of string values,
 * using one of the tables defined at the top of
 * the file.
 */
int print_flags(uint32_t status, int type, char *buffer, size_t size)
{
	int f, first = 1;
	struct flag *flags;

	if (!(flags = _get_flags(type)))
		return_0;

	if (!emit_to_buffer(&buffer, &size, "["))
		return 0;

	for (f = 0; flags[f].mask; f++) {
		if (status & flags[f].mask) {
			status &= ~flags[f].mask;

			if ((type & STATUS_FLAG) != flags[f].kind)
				continue;

			/* Internal-only flag? */
			if (!flags[f].description)
				continue;

			if (!first) {
				if (!emit_to_buffer(&buffer, &size, ", "))
					return 0;
			} else
				first = 0;
	
			if (!emit_to_buffer(&buffer, &size, "\"%s\"",
			    flags[f].description))
				return 0;
		}
	}

	if (!emit_to_buffer(&buffer, &size, "]"))
		return 0;

	if (status)
		log_error("Metadata inconsistency: Not all flags successfully "
			  "exported.");

	return 1;
}

int read_flags(uint32_t *status, int type, struct config_value *cv)
{
	int f;
	uint32_t s = 0;
	struct flag *flags;

	if (!(flags = _get_flags(type)))
		return_0;

	if (cv->type == CFG_EMPTY_ARRAY)
		goto out;

	while (cv) {
		if (cv->type != CFG_STRING) {
			log_error("Status value is not a string.");
			return 0;
		}

		for (f = 0; flags[f].description; f++)
			if (!strcmp(flags[f].description, cv->v.str)) {
				s |= flags[f].mask;
				break;
			}

		if (type == VG_FLAGS && !strcmp(cv->v.str, "PARTIAL")) {
			/*
			 * Exception: We no longer write this flag out, but it
			 * might be encountered in old backup files, so restore
			 * it in that case. It is never part of live metadata
			 * though, so only vgcfgrestore needs to be concerned
			 * by this case.
			 */
			s |= PARTIAL_VG;
		} else if (!flags[f].description && (type & STATUS_FLAG)) {
			log_error("Unknown status flag '%s'.", cv->v.str);
			return 0;
		}

		cv = cv->next;
	}

      out:
	*status |= s;
	return 1;
}
