/* alphavms.h -- BFD definitions for an openVMS host
   Copyright 1996, 2000, 2001, 2005, 2007, 2008, 2009
   Free Software Foundation, Inc.
   Written by Klaus K�mpf (kkaempf@progis.de)
   of proGIS Softwareentwicklung, Aachen, Germany

   This file is part of BFD, the Binary File Descriptor library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston,
   MA 02110-1301, USA.  */

#ifdef PACKAGE
#error sysdep.h must be included in lieu of config.h
#endif

#include "config.h"
#include "ansidecl.h"

#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <stdlib.h>
#include <unixlib.h>
#include <unixio.h>
#include <time.h>

#include "filenames.h"
#include "fopen-vms.h"

#define NO_FCNTL 1

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

extern int getpagesize (void);
extern char *stpcpy (char *, const char *);

/* No intl.  */
#define gettext(Msgid) (Msgid)
#define dgettext(Domainname, Msgid) (Msgid)
#define dcgettext(Domainname, Msgid, Category) (Msgid)
#define textdomain(Domainname) while (0) /* nothing */
#define bindtextdomain(Domainname, Dirname) while (0) /* nothing */
#define _(String) (String)
#define N_(String) (String)
