/*	$NetBSD: no_locking.c,v 1.1.1.2 2009/12/02 00:26:24 haad Exp $	*/

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
#include "locking.h"
#include "locking_types.h"
#include "lvm-string.h"
#include "activate.h"

#include <signal.h>

/*
 * No locking
 */

static void _no_fin_locking(void)
{
	return;
}

static void _no_reset_locking(void)
{
	return;
}

static int _no_lock_resource(struct cmd_context *cmd, const char *resource,
			     uint32_t flags)
{
	switch (flags & LCK_SCOPE_MASK) {
	case LCK_VG:
		break;
	case LCK_LV:
		switch (flags & LCK_TYPE_MASK) {
		case LCK_NULL:
			return lv_deactivate(cmd, resource);
		case LCK_UNLOCK:
			return lv_resume_if_active(cmd, resource);
		case LCK_READ:
			return lv_activate_with_filter(cmd, resource, 0);
		case LCK_WRITE:
			return lv_suspend_if_active(cmd, resource);
		case LCK_EXCL:
			return lv_activate_with_filter(cmd, resource, 1);
		default:
			break;
		}
		break;
	default:
		log_error("Unrecognised lock scope: %d",
			  flags & LCK_SCOPE_MASK);
		return 0;
	}

	return 1;
}

static int _readonly_lock_resource(struct cmd_context *cmd,
				   const char *resource,
				   uint32_t flags)
{
	if ((flags & LCK_TYPE_MASK) == LCK_WRITE &&
	    (flags & LCK_SCOPE_MASK) == LCK_VG &&
	    !(flags & LCK_CACHE) &&
	    strcmp(resource, VG_GLOBAL)) {
		log_error("Write locks are prohibited with --ignorelockingfailure.");
		return 0;
	}

	return _no_lock_resource(cmd, resource, flags);
}

int init_no_locking(struct locking_type *locking, struct cmd_context *cmd __attribute((unused)))
{
	locking->lock_resource = _no_lock_resource;
	locking->reset_locking = _no_reset_locking;
	locking->fin_locking = _no_fin_locking;
	locking->flags = LCK_CLUSTERED;

	return 1;
}

int init_readonly_locking(struct locking_type *locking, struct cmd_context *cmd __attribute((unused)))
{
	locking->lock_resource = _readonly_lock_resource;
	locking->reset_locking = _no_reset_locking;
	locking->fin_locking = _no_fin_locking;
	locking->flags = 0;

	return 1;
}
