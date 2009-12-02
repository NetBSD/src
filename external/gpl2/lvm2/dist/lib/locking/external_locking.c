/*	$NetBSD: external_locking.c,v 1.1.1.2 2009/12/02 00:26:24 haad Exp $	*/

/*
 * Copyright (C) 2002-2004 Sistina Software, Inc. All rights reserved.
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
#include "locking_types.h"
#include "defaults.h"
#include "sharedlib.h"
#include "toolcontext.h"

static void *_locking_lib = NULL;
static void (*_reset_fn) (void) = NULL;
static void (*_end_fn) (void) = NULL;
static int (*_lock_fn) (struct cmd_context * cmd, const char *resource,
			uint32_t flags) = NULL;
static int (*_init_fn) (int type, struct config_tree * cft,
			uint32_t *flags) = NULL;
static int (*_lock_query_fn) (const char *resource, int *mode) = NULL;

static int _lock_resource(struct cmd_context *cmd, const char *resource,
			  uint32_t flags)
{
	if (_lock_fn)
		return _lock_fn(cmd, resource, flags);
	else
		return 0;
}

static void _fin_external_locking(void)
{
	if (_end_fn)
		_end_fn();

	dlclose(_locking_lib);

	_locking_lib = NULL;
	_init_fn = NULL;
	_end_fn = NULL;
	_lock_fn = NULL;
	_reset_fn = NULL;
}

static void _reset_external_locking(void)
{
	if (_reset_fn)
		_reset_fn();
}

int init_external_locking(struct locking_type *locking, struct cmd_context *cmd)
{
	const char *libname;

	if (_locking_lib) {
		log_error("External locking already initialised");
		return 1;
	}

	locking->lock_resource = _lock_resource;
	locking->fin_locking = _fin_external_locking;
	locking->reset_locking = _reset_external_locking;
	locking->flags = 0;

	libname = find_config_tree_str(cmd, "global/locking_library",
				       DEFAULT_LOCKING_LIB);

	if (!(_locking_lib = load_shared_library(cmd, libname, "locking", 1)))
		return_0;

	/* Get the functions we need */
	if (!(_init_fn = dlsym(_locking_lib, "locking_init")) ||
	    !(_lock_fn = dlsym(_locking_lib, "lock_resource")) ||
	    !(_reset_fn = dlsym(_locking_lib, "reset_locking")) ||
	    !(_end_fn = dlsym(_locking_lib, "locking_end"))) {
		log_error("Shared library %s does not contain locking "
			  "functions", libname);
		dlclose(_locking_lib);
		_locking_lib = NULL;
		return 0;
	}

	if (!(_lock_query_fn = dlsym(_locking_lib, "query_resource")))
		log_warn("WARNING: %s: _query_resource() missing: "
			 "Using inferior activation method.", libname);

	log_verbose("Loaded external locking library %s", libname);
	return _init_fn(2, cmd->cft, &locking->flags);
}
