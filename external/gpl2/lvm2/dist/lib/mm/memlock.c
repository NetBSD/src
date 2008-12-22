/*	$NetBSD: memlock.c,v 1.1.1.1 2008/12/22 00:18:13 haad Exp $	*/

/*
 * Copyright (C) 2003-2004 Sistina Software, Inc. All rights reserved.
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
#include "memlock.h"
#include "defaults.h"
#include "config.h"
#include "toolcontext.h"

#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef DEVMAPPER_SUPPORT

void memlock_inc(void)
{
	return;
}
void memlock_dec(void)
{
	return;
}
int memlock(void)
{
	return 0;
}
void memlock_init(struct cmd_context *cmd)
{
	return;
}

#else				/* DEVMAPPER_SUPPORT */

static size_t _size_stack;
static size_t _size_malloc_tmp;
static size_t _size_malloc = 2000000;

static void *_malloc_mem = NULL;
static int _memlock_count = 0;
static int _priority;
static int _default_priority;

static void _touch_memory(void *mem, size_t size)
{
	size_t pagesize = lvm_getpagesize();
	void *pos = mem;
	void *end = mem + size - sizeof(long);

	while (pos < end) {
		*(long *) pos = 1;
		pos += pagesize;
	}
}

static void _allocate_memory(void)
{
	void *stack_mem, *temp_malloc_mem;

	if ((stack_mem = alloca(_size_stack)))
		_touch_memory(stack_mem, _size_stack);

	if ((temp_malloc_mem = malloc(_size_malloc_tmp)))
		_touch_memory(temp_malloc_mem, _size_malloc_tmp);

	if ((_malloc_mem = malloc(_size_malloc)))
		_touch_memory(_malloc_mem, _size_malloc);

	free(temp_malloc_mem);
}

static void _release_memory(void)
{
	free(_malloc_mem);
}

/* Stop memory getting swapped out */
static void _lock_mem(void)
{
#ifdef MCL_CURRENT
	if (mlockall(MCL_CURRENT | MCL_FUTURE))
		log_sys_error("mlockall", "");
	else
		log_very_verbose("Locking memory");
#endif
	_allocate_memory();

	errno = 0;
	if (((_priority = getpriority(PRIO_PROCESS, 0)) == -1) && errno)
		log_sys_error("getpriority", "");
	else
		if (setpriority(PRIO_PROCESS, 0, _default_priority))
			log_error("setpriority %d failed: %s",
				  _default_priority, strerror(errno));
}

static void _unlock_mem(void)
{
#ifdef MCL_CURRENT
	if (munlockall())
		log_sys_error("munlockall", "");
	else
		log_very_verbose("Unlocking memory");
#endif
	_release_memory();
	if (setpriority(PRIO_PROCESS, 0, _priority))
		log_error("setpriority %u failed: %s", _priority,
			  strerror(errno));
}

void memlock_inc(void)
{
	if (!_memlock_count++)
		_lock_mem();
	log_debug("memlock_count inc to %d", _memlock_count);
}

void memlock_dec(void)
{
	if (_memlock_count && (!--_memlock_count))
		_unlock_mem();
	log_debug("memlock_count dec to %d", _memlock_count);
}

int memlock(void)
{
	return _memlock_count;
}

void memlock_init(struct cmd_context *cmd)
{
	_size_stack = find_config_tree_int(cmd,
				      "activation/reserved_stack",
				      DEFAULT_RESERVED_STACK) * 1024;
	_size_malloc_tmp = find_config_tree_int(cmd,
					   "activation/reserved_memory",
					   DEFAULT_RESERVED_MEMORY) * 1024;
	_default_priority = find_config_tree_int(cmd,
					    "activation/process_priority",
					    DEFAULT_PROCESS_PRIORITY);
}

#endif
