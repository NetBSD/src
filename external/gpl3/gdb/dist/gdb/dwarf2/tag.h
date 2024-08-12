/* Tag attributes

   Copyright (C) 2022-2024 Free Software Foundation, Inc.

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

#ifndef GDB_DWARF2_TAG_H
#define GDB_DWARF2_TAG_H

#include "dwarf2.h"
#include "symtab.h"

/* Return true if TAG represents a type, false otherwise.  */

static inline bool
tag_is_type (dwarf_tag tag)
{
  switch (tag)
    {
    case DW_TAG_padding:
    case DW_TAG_array_type:
    case DW_TAG_class_type:
    case DW_TAG_enumeration_type:
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_string_type:
    case DW_TAG_structure_type:
    case DW_TAG_subroutine_type:
    case DW_TAG_typedef:
    case DW_TAG_union_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_set_type:
    case DW_TAG_subrange_type:
    case DW_TAG_base_type:
    case DW_TAG_const_type:
    case DW_TAG_packed_type:
    case DW_TAG_template_type_param:
    case DW_TAG_volatile_type:
    case DW_TAG_restrict_type:
    case DW_TAG_interface_type:
    case DW_TAG_namespace:
    case DW_TAG_unspecified_type:
    case DW_TAG_shared_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_coarray_type:
    case DW_TAG_dynamic_type:
    case DW_TAG_atomic_type:
    case DW_TAG_immutable_type:
      return true;
    default:
      return false;
    }
}

/* Return true if the given DWARF tag matches the specified search
   domain flags.  LANG may affect the result, due to the "C++ tag
   hack".  */

static inline bool
tag_matches_domain (dwarf_tag tag, domain_search_flags search, language lang)
{
  domain_search_flags flags = 0;
  switch (tag)
    {
    case DW_TAG_variable:
    case DW_TAG_enumerator:
    case DW_TAG_constant:
      flags = SEARCH_VAR_DOMAIN;
      break;

    case DW_TAG_subprogram:
    case DW_TAG_entry_point:
      flags = SEARCH_FUNCTION_DOMAIN;
      break;

    case DW_TAG_structure_type:
    case DW_TAG_class_type:
    case DW_TAG_union_type:
    case DW_TAG_enumeration_type:
      {
	if (lang == language_c
	    || lang == language_objc
	    || lang == language_opencl
	    || lang == language_minimal)
	  flags = SEARCH_STRUCT_DOMAIN;
	else if (lang == language_cplus)
	  flags = SEARCH_STRUCT_DOMAIN | SEARCH_TYPE_DOMAIN;
	else
	  flags = SEARCH_TYPE_DOMAIN;
      }
      break;

    case DW_TAG_padding:
    case DW_TAG_array_type:
    case DW_TAG_pointer_type:
    case DW_TAG_reference_type:
    case DW_TAG_string_type:
    case DW_TAG_subroutine_type:
    case DW_TAG_ptr_to_member_type:
    case DW_TAG_set_type:
    case DW_TAG_subrange_type:
    case DW_TAG_base_type:
    case DW_TAG_const_type:
    case DW_TAG_packed_type:
    case DW_TAG_template_type_param:
    case DW_TAG_volatile_type:
    case DW_TAG_restrict_type:
    case DW_TAG_interface_type:
    case DW_TAG_namespace:
    case DW_TAG_unspecified_type:
    case DW_TAG_shared_type:
    case DW_TAG_rvalue_reference_type:
    case DW_TAG_coarray_type:
    case DW_TAG_dynamic_type:
    case DW_TAG_atomic_type:
    case DW_TAG_immutable_type:
    case DW_TAG_typedef:
      flags = SEARCH_TYPE_DOMAIN;
      break;

    case DW_TAG_label:
      flags = SEARCH_LABEL_DOMAIN;
      break;

    case DW_TAG_module:
      flags = SEARCH_MODULE_DOMAIN;
      break;
    }

  return (flags & search) != 0;
}

#endif /* GDB_DWARF2_TAG_H */
