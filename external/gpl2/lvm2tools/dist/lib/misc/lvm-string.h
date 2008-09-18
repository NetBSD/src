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

#ifndef _LVM_STRING_H
#define _LVM_STRING_H

#include <stdio.h>
#include <stdarg.h>

#define NAME_LEN 128

struct pool;

int emit_to_buffer(char **buffer, size_t *size, const char *fmt, ...)
  __attribute__ ((format(printf, 3, 4)));

char *build_dm_name(struct dm_pool *mem, const char *vg,
                    const char *lv, const char *layer);

int validate_name(const char *n);

/*
 * Returns number of occurrences of c in first len characters of str.
 */
unsigned count_chars(const char *str, size_t len, const int c);

/*
 * Returns what length of escaped string would be including terminating NUL.
 */
size_t escaped_len(const char *str);

/*
 * Copies a string from src to out. 
 * Double quotation marks and backslashes are quoted with a backslash.
 * Caller must ensure *out has enough space - see escaped_len().
 * Returns *out.
 */
char *escape_double_quotes(char *out, const char *src);

/*
 * Removes quoting of double quotation marks and backslashes in situ.
 */
void unescape_double_quotes(char *src);

#endif
