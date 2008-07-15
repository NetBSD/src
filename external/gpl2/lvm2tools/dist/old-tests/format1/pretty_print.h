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

#ifndef _LVM_PRETTY_PRINT
#define _LVM_PRETTY_PRINT

#include "metadata.h"

#include <stdio.h>

void dump_pv(struct physical_volume *pv, FILE *fp);
void dump_lv(struct logical_volume *lv, FILE *fp);
void dump_vg(struct volume_group *vg, FILE *fp);
void dump_vg_names(struct list_head *vg_names, FILE *fp);

#endif
