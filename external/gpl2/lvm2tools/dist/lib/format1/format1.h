/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004 Red Hat, Inc. All rights reserved.
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

#ifndef _LVM_FORMAT1_H
#define _LVM_FORMAT1_H

#include "metadata.h"
#include "lvmcache.h"

#define FMT_LVM1_NAME "lvm1"
#define FMT_LVM1_ORPHAN_VG_NAME ORPHAN_VG_NAME(FMT_LVM1_NAME)

#ifdef LVM1_INTERNAL
struct format_type *init_lvm1_format(struct cmd_context *cmd);
#endif

#endif
