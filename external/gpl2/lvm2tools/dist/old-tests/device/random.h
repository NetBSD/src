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

/*
 * Random number generator snarfed from the
 * Stanford Graphbase.
 */

#ifndef _LVM_RANDOM_H
#define _LVM_RANDOM_H

#include "lvm-types.h"

void rand_init(int32_t seed);
int32_t rand_get(void);

/*
 * Note this will reset the seed.
 */
int rand_check(void);

#endif
