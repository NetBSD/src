/* xgettext Perl backend.
   Copyright (C) 2002-2003 Free Software Foundation, Inc.
   Written by Guido Flohr <guido@imperia.net>, 2002-2003

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


#define EXTENSIONS_PERL \
  { "pl",    "perl"   },						\
  { "PL",    "perl"   },						\
  { "pm",    "perl"   },						\
  { "cgi",   "perl"   },						\

#define SCANNERS_PERL \
  { "perl",		extract_perl,					\
	    &flag_table_perl, &formatstring_perl, &formatstring_perl_brace }, \

/* Scan a Perl file and add its translatable strings to mdlp.  */
extern void extract_perl (FILE *fp, const char *real_filename,
			  const char *logical_filename,
			  flag_context_list_table_ty *flag_table,
			  msgdomain_list_ty *mdlp);

extern void x_perl_keyword (const char *keyword);
extern void x_perl_extract_all (void);

extern void init_flag_table_perl (void);
