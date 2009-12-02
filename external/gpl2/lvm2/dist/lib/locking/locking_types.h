/*	$NetBSD: locking_types.h,v 1.1.1.2 2009/12/02 00:26:25 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
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

#include "metadata.h"
#include "config.h"

typedef int (*lock_resource_fn) (struct cmd_context * cmd, const char *resource,
				 uint32_t flags);
typedef int (*query_resource_fn) (const char *resource, int *mode);

typedef void (*fin_lock_fn) (void);
typedef void (*reset_lock_fn) (void);

#define LCK_PRE_MEMLOCK	0x00000001	/* Is memlock() needed before calls? */
#define LCK_CLUSTERED	0x00000002

struct locking_type {
	uint32_t flags;
	lock_resource_fn lock_resource;
	query_resource_fn query_resource;

	reset_lock_fn reset_locking;
	fin_lock_fn fin_locking;
};

/*
 * Locking types
 */
int init_no_locking(struct locking_type *locking, struct cmd_context *cmd);

int init_readonly_locking(struct locking_type *locking, struct cmd_context *cmd);

int init_file_locking(struct locking_type *locking, struct cmd_context *cmd);

int init_external_locking(struct locking_type *locking, struct cmd_context *cmd);

int init_cluster_locking(struct locking_type *locking, struct cmd_context *cmd);
