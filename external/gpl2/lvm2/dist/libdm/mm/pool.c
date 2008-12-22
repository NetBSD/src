/*	$NetBSD: pool.c,v 1.1.1.1 2008/12/22 00:18:35 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2005 Red Hat, Inc. All rights reserved.
 *
 * This file is part of the device-mapper userspace tools.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "dmlib.h"

#ifdef DEBUG_POOL
#include "pool-debug.c"
#else
#include "pool-fast.c"
#endif

char *dm_pool_strdup(struct dm_pool *p, const char *str)
{
	char *ret = dm_pool_alloc(p, strlen(str) + 1);

	if (ret)
		strcpy(ret, str);

	return ret;
}

char *dm_pool_strndup(struct dm_pool *p, const char *str, size_t n)
{
	char *ret = dm_pool_alloc(p, n + 1);

	if (ret) {
		strncpy(ret, str, n);
		ret[n] = '\0';
	}

	return ret;
}

void *dm_pool_zalloc(struct dm_pool *p, size_t s)
{
	void *ptr = dm_pool_alloc(p, s);

	if (ptr)
		memset(ptr, 0, s);

	return ptr;
}
