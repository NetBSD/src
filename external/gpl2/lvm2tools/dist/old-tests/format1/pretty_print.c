/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU General Public License v.2.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "pretty_print.h"

void dump_pv(struct physical_volume *pv, FILE *fp)
{
	fprintf(fp, "physical_volume {\n");
	fprintf(fp, "\tname = '%s'\n", pv->dev->name);
	fprintf(fp, "\tvg_name = '%s'\n", pv->vg_name);
	fprintf(fp, "\tsize = %llu\n", pv->size);
	fprintf(fp, "\tpe_size = %llu\n", pv->pe_size);
	fprintf(fp, "\tpe_start = %llu\n", pv->pe_start);
	fprintf(fp, "\tpe_count = %u\n", pv->pe_count);
	fprintf(fp, "\tpe_allocated = %u\n", pv->pe_allocated);
	fprintf(fp, "}\n\n");
}

void dump_lv(struct logical_volume *lv, FILE *fp)
{
	int i;

	fprintf(fp, "logical_volume {\n");
	fprintf(fp, "\tname = '%s'\n", lv->name);
	fprintf(fp, "\tsize = %llu\n", lv->size);
	fprintf(fp, "\tle_count = %u\n", lv->le_count);

	fprintf(fp, "\tmap {\n");
	for (i = 0; i < lv->le_count; i++) {
		struct physical_volume *pv = lv->map[i].pv;

		fprintf(fp, "\t\tpv = '%s', ",
			pv ? pv->dev->name : "null ???");
		fprintf(fp, "\textent = %u\n", lv->map[i].pe);
	}
	fprintf(fp, "\t}\n}\n\n");
}

void dump_vg(struct volume_group *vg, FILE *fp)
{
	struct list_head *tmp;

	fprintf(fp, "volume_group {\n");
	fprintf(fp, "\tname = '%s'\n", vg->name);
	fprintf(fp, "\textent_size = %llu\n", vg->extent_size);
	fprintf(fp, "\textent_count = %d\n", vg->extent_count);
	fprintf(fp, "\tfree_count = %d\n", vg->free_count);
	fprintf(fp, "\tmax_lv = %d\n", vg->max_lv);
	fprintf(fp, "\tmax_pv = %d\n", vg->max_pv);
	fprintf(fp, "\tpv_count = %d\n", vg->pv_count);
	fprintf(fp, "\tlv_count = %d\n", vg->lv_count);
	fprintf(fp, "}\n\n");

	list_for_each(tmp, &vg->pvs) {
		struct pv_list *pvl = list_entry(tmp, struct pv_list, list);
		dump_pv(&pvl->pv, fp);
	}

	list_for_each(tmp, &vg->lvs) {
		struct lv_list *lvl = list_entry(tmp, struct lv_list, list);
		dump_lv(&lvl->lv, fp);
	}
}

void dump_vg_names(struct list_head *vg_names, FILE *fp)
{
	struct list_head *tmp;
	struct name_list *nl;

	list_for_each(tmp, vg_names) {
		nl = list_entry(tmp, struct name_list, list);
		fprintf(fp, "%s\n", nl->name);
	}
}
