/* xgettext PHP backend.
   Copyright (C) 2002-2003 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2002.

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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */


#define EXTENSIONS_PHP \
  { "php",    "PHP"   },						\
  { "php3",   "PHP"   },						\
  { "php4",   "PHP"   },						\

#define SCANNERS_PHP \
  { "PHP",		extract_php,					\
			&flag_table_php, &formatstring_php, NULL },	\

/* Scan a PHP file and add its translatable strings to mdlp.  */
extern void extract_php (FILE *fp, const char *real_filename,
			 const char *logical_filename,
			 flag_context_list_table_ty *flag_table,
			 msgdomain_list_ty *mdlp);

extern void x_php_keyword (const char *keyword);
extern void x_php_extract_all (void);

extern void init_flag_table_php (void);
