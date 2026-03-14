/* Call site information.

   Copyright (C) 2011-2025 Free Software Foundation, Inc.

   Contributed by Cygnus Support, using pieces from other GDB modules.

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

#ifndef GDB_DWARF2_CALL_SITE_H
#define GDB_DWARF2_CALL_SITE_H

#include "dwarf2/types.h"
#include "../frame.h"
#include "gdbsupport/function-view.h"
#include "gdbsupport/unordered_set.h"

struct dwarf2_locexpr_baton;
struct dwarf2_per_cu;
struct dwarf2_per_objfile;

/* struct call_site_parameter can be referenced in callees by several ways.  */

enum call_site_parameter_kind
{
  /* * Use field call_site_parameter.u.dwarf_reg.  */
  CALL_SITE_PARAMETER_DWARF_REG,

  /* * Use field call_site_parameter.u.fb_offset.  */
  CALL_SITE_PARAMETER_FB_OFFSET,

  /* * Use field call_site_parameter.u.param_offset.  */
  CALL_SITE_PARAMETER_PARAM_OFFSET
};

struct call_site_target
{
  /* The kind of location held by this call site target.  */
  enum kind
  {
    /* An address.  */
    PHYSADDR,
    /* A name.  */
    PHYSNAME,
    /* A DWARF block.  */
    DWARF_BLOCK,
    /* An array of addresses.  */
    ADDRESSES,
  };

  void set_loc_physaddr (unrelocated_addr physaddr)
  {
    m_loc_kind = PHYSADDR;
    m_loc.physaddr = physaddr;
  }

  void set_loc_physname (const char *physname)
    {
      m_loc_kind = PHYSNAME;
      m_loc.physname = physname;
    }

  void set_loc_dwarf_block (dwarf2_locexpr_baton *dwarf_block)
    {
      m_loc_kind = DWARF_BLOCK;
      m_loc.dwarf_block = dwarf_block;
    }

  void set_loc_array (unsigned length, const unrelocated_addr *data)
  {
    m_loc_kind = ADDRESSES;
    m_loc.addresses.length = length;
    m_loc.addresses.values = data;
  }

  /* Callback type for iterate_over_addresses.  */

  using iterate_ftype = gdb::function_view<void (CORE_ADDR)>;

  /* Call CALLBACK for each DW_TAG_call_site's DW_AT_call_target
     address.  CALLER_FRAME (for registers) can be NULL if it is not
     known.  This function always may throw NO_ENTRY_VALUE_ERROR.  */

  void iterate_over_addresses (struct gdbarch *call_site_gdbarch,
			       const struct call_site *call_site,
			       const frame_info_ptr &caller_frame,
			       iterate_ftype callback) const;

private:

  union
  {
    /* Address.  */
    unrelocated_addr physaddr;
    /* Mangled name.  */
    const char *physname;
    /* DWARF block.  */
    struct dwarf2_locexpr_baton *dwarf_block;
    /* Array of addresses.  */
    struct
    {
      unsigned length;
      const unrelocated_addr *values;
    } addresses;
  } m_loc;

  /* * Discriminant for union field_location.  */
  enum kind m_loc_kind;
};

union call_site_parameter_u
{
  /* * DW_TAG_formal_parameter's DW_AT_location's DW_OP_regX
     as DWARF register number, for register passed
     parameters.  */

  int dwarf_reg;

  /* * Offset from the callee's frame base, for stack passed
     parameters.  This equals offset from the caller's stack
     pointer.  */

  CORE_ADDR fb_offset;

  /* * Offset relative to the start of this PER_CU to
     DW_TAG_formal_parameter which is referenced by both
     caller and the callee.  */

  cu_offset param_cu_off;
};

struct call_site_parameter
{
  ENUM_BITFIELD (call_site_parameter_kind) kind : 2;

  union call_site_parameter_u u;

  /* * DW_TAG_formal_parameter's DW_AT_call_value.  It is never NULL.  */

  const gdb_byte *value;
  size_t value_size;

  /* * DW_TAG_formal_parameter's DW_AT_call_data_value.
     It may be NULL if not provided by DWARF.  */

  const gdb_byte *data_value;
  size_t data_value_size;
};

/* * A place where a function gets called from, represented by
   DW_TAG_call_site.  It can be looked up from symtab->call_site_htab.  */

struct call_site
{
  call_site (unrelocated_addr pc, dwarf2_per_cu *per_cu,
	     dwarf2_per_objfile *per_objfile)
    : per_cu (per_cu), per_objfile (per_objfile), m_unrelocated_pc (pc)
  {}

  /* Return the relocated (using the objfile from PER_OBJFILE) address of the
     first instruction after this call.  */

  CORE_ADDR pc () const;

  /* Return the unrelocated address of the first instruction after this
     call.  */

  unrelocated_addr unrelocated_pc () const noexcept
  { return m_unrelocated_pc; }

  /* Call CALLBACK for each target address.  CALLER_FRAME (for
     registers) can be NULL if it is not known.  This function may
     throw NO_ENTRY_VALUE_ERROR.  */

  void iterate_over_addresses (struct gdbarch *call_site_gdbarch,
			       const frame_info_ptr &caller_frame,
			       call_site_target::iterate_ftype callback)
    const
  {
    return target.iterate_over_addresses (call_site_gdbarch, this,
					  caller_frame, callback);
  }

  /* * List successor with head in FUNC_TYPE.TAIL_CALL_LIST.  */

  struct call_site *tail_call_next = nullptr;

  /* * Describe DW_AT_call_target.  Missing attribute uses
     m_loc_kind == DWARF_BLOCK with m_loc.dwarf_block == nullptr.  */

  struct call_site_target target {};

  /* * Size of the PARAMETER array.  */

  unsigned parameter_count = 0;

  /* * CU of the function where the call is located.  It gets used
     for DWARF blocks execution in the parameter array below.  */

  dwarf2_per_cu *const per_cu = nullptr;

  /* objfile of the function where the call is located.  */

  dwarf2_per_objfile *const per_objfile = nullptr;

private:
  /* Unrelocated address of the first instruction after this call.  */
  const unrelocated_addr m_unrelocated_pc;

public:
  /* * Describe DW_TAG_call_site's DW_TAG_formal_parameter.  */

  struct call_site_parameter parameter[];
};

/* Key hash type to store call_site objects in gdb::unordered_set, identified by
   their unrelocated PC.  */

struct call_site_hash_pc
{
  using is_transparent = void;

  std::size_t operator() (const call_site *site) const noexcept
  { return (*this) (site->unrelocated_pc ()); }

  std::size_t operator() (unrelocated_addr pc) const noexcept
  { return std::hash<unrelocated_addr> () (pc); }
};

/* Key equal type to store call_site objects in gdb::unordered_set, identified
   by their unrelocated PC.  */

struct call_site_eq_pc
{
  using is_transparent = void;

  bool operator() (const call_site *a, const call_site *b) const noexcept
  { return (*this) (a->unrelocated_pc (), b); }

  bool operator() (unrelocated_addr pc, const call_site *site) const noexcept
  { return pc == site->unrelocated_pc (); }
};

/* Set of call_site objects identified by their unrelocated PC.  */

using call_site_htab_t
  = gdb::unordered_set<call_site *, call_site_hash_pc, call_site_eq_pc>;

#endif /* GDB_DWARF2_CALL_SITE_H */
