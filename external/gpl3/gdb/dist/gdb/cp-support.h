/* Helper routines for C++ support in GDB.
   Copyright (C) 2002-2016 Free Software Foundation, Inc.

   Contributed by MontaVista Software.
   Namespace support contributed by David Carlton.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef CP_SUPPORT_H
#define CP_SUPPORT_H

/* We need this for 'domain_enum', alas...  */

#include "symtab.h"
#include "vec.h"
#include "gdb_vecs.h"
#include "gdb_obstack.h"

/* Opaque declarations.  */

struct symbol;
struct block;
struct objfile;
struct type;
struct demangle_component;
struct using_direct;

/* A string representing the name of the anonymous namespace used in GDB.  */

#define CP_ANONYMOUS_NAMESPACE_STR "(anonymous namespace)"

/* The length of the string representing the anonymous namespace.  */

#define CP_ANONYMOUS_NAMESPACE_LEN 21

/* The result of parsing a name.  */

struct demangle_parse_info
{
  /* The memory used during the parse.  */
  struct demangle_info *info;

  /* The result of the parse.  */
  struct demangle_component *tree;

  /* Any temporary memory used during typedef replacement.  */
  struct obstack obstack;
};


/* Functions from cp-support.c.  */

extern char *cp_canonicalize_string (const char *string);

extern char *cp_canonicalize_string_no_typedefs (const char *string);

typedef const char *(canonicalization_ftype) (struct type *, void *);

extern char *cp_canonicalize_string_full (const char *string,
					  canonicalization_ftype *finder,
					  void *data);

extern char *cp_class_name_from_physname (const char *physname);

extern char *method_name_from_physname (const char *physname);

extern unsigned int cp_find_first_component (const char *name);

extern unsigned int cp_entire_prefix_len (const char *name);

extern char *cp_func_name (const char *full_name);

extern char *cp_remove_params (const char *demangled_name);

extern struct symbol **make_symbol_overload_list (const char *,
						  const char *);

extern struct symbol **make_symbol_overload_list_adl (struct type **arg_types,
                                                      int nargs,
                                                      const char *func_name);

extern struct type *cp_lookup_rtti_type (const char *name,
					 struct block *block);

/* Functions/variables from cp-namespace.c.  */

extern int cp_is_in_anonymous (const char *symbol_name);

extern void cp_scan_for_anonymous_namespaces (const struct symbol *symbol,
					      struct objfile *objfile);

extern struct block_symbol cp_lookup_symbol_nonlocal
     (const struct language_defn *langdef,
      const char *name,
      const struct block *block,
      const domain_enum domain);

extern struct block_symbol
  cp_lookup_symbol_namespace (const char *the_namespace,
			      const char *name,
			      const struct block *block,
			      const domain_enum domain);

extern struct block_symbol cp_lookup_symbol_imports_or_template
     (const char *scope,
      const char *name,
      const struct block *block,
      const domain_enum domain);

extern struct block_symbol
  cp_lookup_nested_symbol (struct type *parent_type,
			   const char *nested_name,
			   const struct block *block,
			   const domain_enum domain);

struct type *cp_lookup_transparent_type (const char *name);

/* See description in cp-namespace.c.  */

struct type *cp_find_type_baseclass_by_name (struct type *parent_type,
					     const char *name);

/* Functions from cp-name-parser.y.  */

extern struct demangle_parse_info *cp_demangled_name_to_comp
     (const char *demangled_name, const char **errmsg);

extern char *cp_comp_to_string (struct demangle_component *result,
				int estimated_len);

extern void cp_demangled_name_parse_free (struct demangle_parse_info *);
extern struct cleanup *make_cleanup_cp_demangled_name_parse_free
     (struct demangle_parse_info *);
extern void cp_merge_demangle_parse_infos (struct demangle_parse_info *,
					   struct demangle_component *,
					   struct demangle_parse_info *);

extern struct demangle_parse_info *cp_new_demangle_parse_info (void);

/* The list of "maint cplus" commands.  */

extern struct cmd_list_element *maint_cplus_cmd_list;

/* A wrapper for bfd_demangle.  */

char *gdb_demangle (const char *name, int options);

/* Like gdb_demangle, but suitable for use as la_sniff_from_mangled_name.  */

int gdb_sniff_from_mangled_name (const char *mangled, char **demangled);

#endif /* CP_SUPPORT_H */
