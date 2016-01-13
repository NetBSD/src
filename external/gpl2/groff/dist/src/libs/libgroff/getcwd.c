/*	$NetBSD: getcwd.c,v 1.1.1.1 2016/01/13 18:41:48 christos Exp $	*/

/* Copyright (C) 2000 Free Software Foundation, Inc.
     Written by James Clark (jjc@jclark.com)

This file is part of groff.

groff is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2, or (at your option) any later
version.

groff is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License along
with groff; see the file COPYING.  If not, write to the Free Software
Foundation, 51 Franklin St - Fifth Floor, Boston, MA 02110-1301, USA. */

/* Partial emulation of getcwd in terms of getwd. */

#include <sys/param.h>
#include <string.h>
#include <errno.h>

char *getwd();

char *getcwd(buf, size)
     char *buf;
     int size;			/* POSIX says this should be size_t */
{
  if (size <= 0) {
    errno = EINVAL;
    return 0;
  }
  else {
    char mybuf[MAXPATHLEN];
    int saved_errno = errno;

    errno = 0;
    if (!getwd(mybuf)) {
      if (errno == 0)
	;       /* what to do? */
      return 0;
    }
    errno = saved_errno;
    if (strlen(mybuf) + 1 > size) {
      errno = ERANGE;
      return 0;
    }
    strcpy(buf, mybuf);
    return buf;
  }
}
