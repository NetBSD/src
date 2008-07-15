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
#include "disk-rep.h"

/*
 * Only works with powers of 2.
 */
static uint32_t _round_up(uint32_t n, uint32_t size)
{
	size--;
	return (n + size) & ~size;
}

/* Unused.
static uint32_t _div_up(uint32_t n, uint32_t size)
{
	return _round_up(n, size) / size;
}
*/

/*
 * Each chunk of metadata should be aligned to
 * METADATA_ALIGN.
 */
static uint32_t _next_base(struct data_area *area)
{
	return _round_up(area->base + area->size, METADATA_ALIGN);
}

/*
 * Quick calculation based on pe_start.
 */
static int _adjust_pe_on_disk(struct pv_disk *pvd)
{
	uint32_t pe_start = pvd->pe_start << SECTOR_SHIFT;

	if (pe_start < pvd->pe_on_disk.base + pvd->pe_on_disk.size)
		return 0;

	pvd->pe_on_disk.size = pe_start - pvd->pe_on_disk.base;
	return 1;
}

static void _calc_simple_layout(struct pv_disk *pvd)
{
	pvd->pv_on_disk.base = METADATA_BASE;
	pvd->pv_on_disk.size = PV_SIZE;

	pvd->vg_on_disk.base = _next_base(&pvd->pv_on_disk);
	pvd->vg_on_disk.size = VG_SIZE;

	pvd->pv_uuidlist_on_disk.base = _next_base(&pvd->vg_on_disk);
	pvd->pv_uuidlist_on_disk.size = MAX_PV * NAME_LEN;

	pvd->lv_on_disk.base = _next_base(&pvd->pv_uuidlist_on_disk);
	pvd->lv_on_disk.size = MAX_LV * sizeof(struct lv_disk);

	pvd->pe_on_disk.base = _next_base(&pvd->lv_on_disk);
	pvd->pe_on_disk.size = pvd->pe_total * sizeof(struct pe_disk);
}

static int _check_vg_limits(struct disk_list *dl)
{
	if (dl->vgd.lv_max > MAX_LV) {
		log_error("MaxLogicalVolumes of %d exceeds format limit of %d "
			  "for VG '%s'", dl->vgd.lv_max, MAX_LV - 1,
			  dl->pvd.vg_name);
		return 0;
	}

	if (dl->vgd.pv_max > MAX_PV) {
		log_error("MaxPhysicalVolumes of %d exceeds format limit of %d "
			  "for VG '%s'", dl->vgd.pv_max, MAX_PV - 1,
			  dl->pvd.vg_name);
		return 0;
	}

	return 1;
}

/*
 * This assumes pe_count and pe_start have already
 * been calculated correctly.
 */
int calculate_layout(struct disk_list *dl)
{
	struct pv_disk *pvd = &dl->pvd;

	_calc_simple_layout(pvd);
	if (!_adjust_pe_on_disk(pvd)) {
		log_error("Insufficient space for metadata and PE's.");
		return 0;
	}

	if (!_check_vg_limits(dl))
		return 0;

	return 1;
}

/*
 * The number of extents that can fit on a disk is metadata format dependant.
 * pe_start is any existing value for pe_start
 */
int calculate_extent_count(struct physical_volume *pv, uint32_t extent_size,
			   uint32_t max_extent_count, uint64_t pe_start)
{
	struct pv_disk *pvd = dm_malloc(sizeof(*pvd));
	uint32_t end;

	if (!pvd)
		return_0;

	/*
	 * Guess how many extents will fit, bearing in mind that
	 * one is going to be knocked off at the start of the
	 * next loop.
	 */
	if (max_extent_count)
		pvd->pe_total = max_extent_count + 1;
	else
		pvd->pe_total = (pv->size / extent_size);

	if (pvd->pe_total < PE_SIZE_PV_SIZE_REL) {
		log_error("Too few extents on %s.  Try smaller extent size.",
			  pv_dev_name(pv));
		dm_free(pvd);
		return 0;
	}

	do {
		pvd->pe_total--;
		_calc_simple_layout(pvd);
		end = ((pvd->pe_on_disk.base + pvd->pe_on_disk.size +
			SECTOR_SIZE - 1) >> SECTOR_SHIFT);

		if (pe_start && end < pe_start)
			end = pe_start;

		pvd->pe_start = _round_up(end, LVM1_PE_ALIGN);

	} while ((pvd->pe_start + (pvd->pe_total * extent_size))
		 > pv->size);

	if (pvd->pe_total > MAX_PE_TOTAL) {
		log_error("Metadata extent limit (%u) exceeded for %s - "
			  "%u required", MAX_PE_TOTAL, pv_dev_name(pv),
			  pvd->pe_total);
		dm_free(pvd);
		return 0;
	}

	pv->pe_count = pvd->pe_total;
	pv->pe_start = pvd->pe_start;
	/* We can't set pe_size here without breaking LVM1 compatibility */
	dm_free(pvd);
	return 1;
}
