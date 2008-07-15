/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.  
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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

#ifndef _DM_DBG_MALLOC_H
#define _DM_DBG_MALLOC_H

#include <stdlib.h>
#include <string.h>

void *malloc_aux(size_t s, const char *file, int line);
#define dm_malloc(s) malloc_aux((s), __FILE__, __LINE__)

char *dbg_strdup(const char *str);

#ifdef DEBUG_MEM

void free_aux(void *p);
void *realloc_aux(void *p, unsigned int s, const char *file, int line);
int dump_memory(void);
void bounds_check(void);

#  define dm_free(p) free_aux(p)
#  define dbg_realloc(p, s) realloc_aux(p, s, __FILE__, __LINE__)

#else

#  define dm_free(p) do {if (p) free(p); } while (0)
#  define dbg_realloc(p, s) realloc(p, s)
#  define dump_memory()
#  define bounds_check()

#endif

#endif
