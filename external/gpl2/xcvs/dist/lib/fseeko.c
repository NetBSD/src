/* fseeko.c -- an implementation of fseek() with an off_t argument.
   Copyright (C) 2003, Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif
#ifndef LONG_MAX
#define LONG_MAX ((long) ((unsigned long) ~0 >> 1))
#endif
#ifndef LONG_MIN
#define LONG_MIN (-1 - LONG_MAX)
#endif

/*
 * A replacement/substitute for fseeko, for hosts that don't have it.
 */

int
fseeko (FILE *stream, off_t offset, int whence)
{
    while (offset != (long) offset)
    {
	long pos = (offset < 0) ? LONG_MIN : LONG_MAX;

	if (fseek (stream, pos, whence) != 0)
	    return -1;
	offset -= pos;
	whence = SEEK_CUR;
    }
    return fseek (stream, (long) offset, whence);
}
