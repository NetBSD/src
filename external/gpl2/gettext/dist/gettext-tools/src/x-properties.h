/* xgettext JavaProperties backend.
   Copyright (C) 2003 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2003.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */


#define EXTENSIONS_PROPERTIES \
  { "properties", "JavaProperties" },					\

#define SCANNERS_PROPERTIES \
  { "JavaProperties",	extract_properties, NULL, NULL, NULL },		\

/* Scan a JavaProperties file and add its translatable strings to mdlp.  */
extern void extract_properties (FILE *fp, const char *real_filename,
				const char *logical_filename,
				flag_context_list_table_ty *flag_table,
				msgdomain_list_ty *mdlp);
