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
};

static struct flag _vg_flags[] = {
	{EXPORTED_VG, "EXPORTED"},
	{RESIZEABLE_VG, "RESIZEABLE"},
	{PARTIAL_VG, "PARTIAL"},
	{PVMOVE, "PVMOVE"},
	{LVM_READ, "READ"},
	{LVM_WRITE, "WRITE"},
	{CLUSTERED, "CLUSTERED"},
	{SHARED, "SHARED"},
	{PRECOMMITTED, NULL},
	{0, NULL}
};

static struct flag _pv_flags[] = {
	{ALLOCATABLE_PV, "ALLOCATABLE"},
	{EXPORTED_VG, "EXPORTED"},
	{0, NULL}
};

static struct flag _lv_flags[] = {
	{LVM_READ, "READ"},
	{LVM_WRITE, "WRITE"},
	{FIXED_MINOR, "FIXED_MINOR"},
	{VISIBLE_LV, "VISIBLE"},
	{PVMOVE, "PVMOVE"},
	{LOCKED, "LOCKED"},
	{MIRROR_NOTSYNCED, "NOTSYNCED"},
	{MIRROR_IMAGE, NULL},
	{MIRROR_LOG, NULL},
	{MIRRORED, NULL},
	{VIRTUAL, NULL},
	{SNAPSHOT, NULL},
	{ACTIVATE_EXCL, NULL},
	{CONVERTING, NULL},
	{0, NULL}
};

static struct flag *_get_flags(int type)
{
	switch (type) {
	case VG_FLAGS:
		return _vg_flags;

	case PV_FLAGS:
		return _pv_flags;

	case LV_FLAGS:
		return _lv_flags;
	}

	log_err("Unknown flag set requested.");
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
			log_err("Status value is not a string.");
			return 0;
		}

		for (f = 0; flags[f].description; f++)
			if (!strcmp(flags[f].description, cv->v.str)) {
				s |= flags[f].mask;
				break;
			}

		if (!flags[f].description) {
			log_err("Unknown status flag '%s'.", cv->v.str);
			return 0;
		}

		cv = cv->next;
	}

      out:
	*status = s;
	return 1;
}
