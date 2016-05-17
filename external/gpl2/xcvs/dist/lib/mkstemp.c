/* Copyright (C) 1998, 1999, 2001, 2005 Free Software Foundation, Inc.
   This file is derived from the one in the GNU C Library.

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */
#include <sys/cdefs.h>
__RCSID("$NetBSD: mkstemp.c,v 1.2 2016/05/17 14:00:09 christos Exp $");


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Disable the definition of mkstemp to rpl_mkstemp (from config.h) in this
   file.  Otherwise, we'd get conflicting prototypes for rpl_mkstemp on
   most systems.  */
#undef mkstemp

#include <stdio.h>
#include <stdlib.h>

#ifndef __GT_FILE
# define __GT_FILE 0
#endif

int __gen_tempname ();

/* Generate a unique temporary file name from TEMPLATE.
   The last six characters of TEMPLATE must be "XXXXXX";
   they are replaced with a string that makes the file name unique.
   Then open the file and return a fd. */
int
rpl_mkstemp (char *template)
{
  return __gen_tempname (template, __GT_FILE);
}
