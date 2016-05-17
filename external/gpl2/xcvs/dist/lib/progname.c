/* Declare program_name for support functions compiled with the getdate.y test
   program.

   Copyright (C) 1990-1998, 2000-2002, 2003 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
#include <sys/cdefs.h>
__RCSID("$NetBSD: progname.c,v 1.2 2016/05/17 14:00:09 christos Exp $");


/* Written by Derek R. Price <derek@ximbiot.com>.  */

/* This gets set to the name of the executing program.  */
char *program_name = "getdate";
