/* DWARF 2 location expression support for GDB.

   Copyright (C) 2003-2020 Free Software Foundation, Inc.

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

#if !defined (DWARF2LOC_H)
#define DWARF2LOC_H

#include "dwarf2/expr.h"

struct symbol_computed_ops;
struct dwarf2_per_objfile;
struct dwarf2_per_cu_data;
struct dwarf2_loclist_baton;
struct agent_expr;
struct axs_value;

/* This header is private to the DWARF-2 reader.  It is shared between
   dwarf2read.c and dwarf2loc.c.  */

/* `set debug entry-values' setting.  */
extern unsigned int entry_values_debug;

/* Find a particular location expression from a location list.  */
const gdb_byte *dwarf2_find_location_expression
  (struct dwarf2_loclist_baton *baton,
   size_t *locexpr_length,
   CORE_ADDR pc);

/* Find the frame base information for FRAMEFUNC at PC.  START is an
   out parameter which is set to point to the DWARF expression to
   compute.  LENGTH is an out parameter which is set to the length of
   the DWARF expression.  This throws an exception on error or if an
   expression is not found; the returned length will never be
   zero.  */

extern void func_get_frame_base_dwarf_block (struct symbol *framefunc,
					     CORE_ADDR pc,
					     const gdb_byte **start,
					     size_t *length);

/* Evaluate a location description, starting at DATA and with length
   SIZE, to find the current location of variable of TYPE in the context
   of FRAME.  */

struct value *dwarf2_evaluate_loc_desc (struct type *type,
					struct frame_info *frame,
					const gdb_byte *data,
					size_t size,
					dwarf2_per_cu_data *per_cu,
					dwarf2_per_objfile *per_objfile);

/* A chain of addresses that might be needed to resolve a dynamic
   property.  */

struct property_addr_info
{
  /* The type of the object whose dynamic properties, if any, are
     being resolved.  */
  struct type *type;

  /* If not NULL, a buffer containing the object's value.  */
  gdb::array_view<const gdb_byte> valaddr;

  /* The address of that object.  */
  CORE_ADDR addr;

  /* If not NULL, a pointer to the info for the object containing
     the object described by this node.  */
  struct property_addr_info *next;
};

/* Converts a dynamic property into a static one.  FRAME is the frame in which
   the property is evaluated; if NULL, the selected frame (if any) is used
   instead.

   ADDR_STACK is the stack of addresses that might be needed to evaluate the
   property. When evaluating a property that is not related to a type, it can
   be NULL.

   Returns true if PROP could be converted and the static value is passed
   back into VALUE, otherwise returns false.

   If PUSH_INITIAL_VALUE is true, then the top value of ADDR_STACK
   will be pushed before evaluating a location expression.  */

bool dwarf2_evaluate_property (const struct dynamic_prop *prop,
			       struct frame_info *frame,
			       const struct property_addr_info *addr_stack,
			       CORE_ADDR *value,
			       bool push_initial_value = false);

/* A helper for the compiler interface that compiles a single dynamic
   property to C code.

   STREAM is where the C code is to be written.
   RESULT_NAME is the name of the generated variable.
   GDBARCH is the architecture to use.
   REGISTERS_USED is a bit-vector that is filled to note which
   registers are required by the generated expression.
   PROP is the property for which code is generated.
   ADDRESS is the address at which the property is considered to be
   evaluated.
   SYM the originating symbol, used for error reporting.  */

void dwarf2_compile_property_to_c (string_file *stream,
				   const char *result_name,
				   struct gdbarch *gdbarch,
				   unsigned char *registers_used,
				   const struct dynamic_prop *prop,
				   CORE_ADDR address,
				   struct symbol *sym);

/* The symbol location baton types used by the DWARF-2 reader (i.e.
   SYMBOL_LOCATION_BATON for a LOC_COMPUTED symbol).  "struct
   dwarf2_locexpr_baton" is for a symbol with a single location
   expression; "struct dwarf2_loclist_baton" is for a symbol with a
   location list.  */

struct dwarf2_locexpr_baton
{
  /* Pointer to the start of the location expression.  Valid only if SIZE is
     not zero.  */
  const gdb_byte *data;

  /* Length of the location expression.  For optimized out expressions it is
     zero.  */
  size_t size;

  /* When true this location expression is a reference and actually
     describes the address at which the value of the attribute can be
     found.  When false the expression provides the value of the attribute
     directly.  */
  bool is_reference;

  /* The objfile that was used when creating this.  */
  dwarf2_per_objfile *per_objfile;

  /* The compilation unit containing the symbol whose location
     we're computing.  */
  struct dwarf2_per_cu_data *per_cu;
};

struct dwarf2_loclist_baton
{
  /* The initial base address for the location list, based on the compilation
     unit.  */
  CORE_ADDR base_address;

  /* Pointer to the start of the location list.  */
  const gdb_byte *data;

  /* Length of the location list.  */
  size_t size;

  /* The objfile that was used when creating this.  */
  dwarf2_per_objfile *per_objfile;

  /* The compilation unit containing the symbol whose location
     we're computing.  */
  struct dwarf2_per_cu_data *per_cu;

  /* Non-zero if the location list lives in .debug_loc.dwo.
     The format of entries in this section are different.  */
  unsigned char from_dwo;
};

/* The baton used when a dynamic property is an offset to a parent
   type.  This can be used, for instance, then the bound of an array
   inside a record is determined by the value of another field inside
   that record.  */

struct dwarf2_offset_baton
{
  /* The offset from the parent type where the value of the property
     is stored.  In the example provided above, this would be the offset
     of the field being used as the array bound.  */
  LONGEST offset;

  /* The type of the object whose property is dynamic.  In the example
     provided above, this would the array's index type.  */
  struct type *type;
};

/* A dynamic property is either expressed as a single location expression
   or a location list.  If the property is an indirection, pointing to
   another die, keep track of the targeted type in PROPERTY_TYPE.
   Alternatively, if the property location gives the property value
   directly then it will have PROPERTY_TYPE.  */

struct dwarf2_property_baton
{
  /* If the property is an indirection, we need to evaluate the location
     in the context of the type PROPERTY_TYPE.  If the property is supplied
     by value then it will be of PROPERTY_TYPE.  This field should never be
     NULL.  */
  struct type *property_type;
  union
  {
    /* Location expression either evaluated in the context of
       PROPERTY_TYPE, or a value of type PROPERTY_TYPE.  */
    struct dwarf2_locexpr_baton locexpr;

    /* Location list to be evaluated in the context of PROPERTY_TYPE.  */
    struct dwarf2_loclist_baton loclist;

    /* The location is an offset to PROPERTY_TYPE.  */
    struct dwarf2_offset_baton offset_info;
  };
};

extern const struct symbol_computed_ops dwarf2_locexpr_funcs;
extern const struct symbol_computed_ops dwarf2_loclist_funcs;

extern const struct symbol_block_ops dwarf2_block_frame_base_locexpr_funcs;
extern const struct symbol_block_ops dwarf2_block_frame_base_loclist_funcs;

/* Determined tail calls for constructing virtual tail call frames.  */

struct call_site_chain
  {
    /* Initially CALLERS == CALLEES == LENGTH.  For partially ambiguous result
       CALLERS + CALLEES < LENGTH.  */
    int callers, callees, length;

    /* Variably sized array with LENGTH elements.  Later [0..CALLERS-1] contain
       top (GDB "prev") sites and [LENGTH-CALLEES..LENGTH-1] contain bottom
       (GDB "next") sites.  One is interested primarily in the PC field.  */
    struct call_site *call_site[1];
  };

struct call_site_stuff;
extern gdb::unique_xmalloc_ptr<call_site_chain> call_site_find_chain
  (struct gdbarch *gdbarch, CORE_ADDR caller_pc, CORE_ADDR callee_pc);

/* A helper function to convert a DWARF register to an arch register.
   ARCH is the architecture.
   DWARF_REG is the register.
   If DWARF_REG is bad then a complaint is issued and -1 is returned.
   Note: Some targets get this wrong.  */

extern int dwarf_reg_to_regnum (struct gdbarch *arch, int dwarf_reg);

/* A wrapper on dwarf_reg_to_regnum to throw an exception if the
   DWARF register cannot be translated to an architecture register.
   This takes a ULONGEST instead of an int because some callers actually have
   a ULONGEST.  Negative values passed as ints will still be flagged as
   invalid.  */

extern int dwarf_reg_to_regnum_or_error (struct gdbarch *arch,
					 ULONGEST dwarf_reg);

#endif /* dwarf2loc.h */
