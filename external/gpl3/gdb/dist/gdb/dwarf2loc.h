/* DWARF 2 location expression support for GDB.

   Copyright (C) 2003, 2005, 2007, 2008, 2009, 2010, 2011
   Free Software Foundation, Inc.

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

struct symbol_computed_ops;
struct objfile;
struct dwarf2_per_cu_data;
struct dwarf2_loclist_baton;
struct agent_expr;
struct axs_value;

/* This header is private to the DWARF-2 reader.  It is shared between
   dwarf2read.c and dwarf2loc.c.  */

/* Return the OBJFILE associated with the compilation unit CU.  If CU
   came from a separate debuginfo file, then the master objfile is
   returned.  */
struct objfile *dwarf2_per_cu_objfile (struct dwarf2_per_cu_data *cu);

/* Return the address size given in the compilation unit header for CU.  */
CORE_ADDR dwarf2_per_cu_addr_size (struct dwarf2_per_cu_data *cu);

/* Return the offset size given in the compilation unit header for CU.  */
int dwarf2_per_cu_offset_size (struct dwarf2_per_cu_data *cu);

/* Return the text offset of the CU.  The returned offset comes from
   this CU's objfile.  If this objfile came from a separate debuginfo
   file, then the offset may be different from the corresponding
   offset in the parent objfile.  */
CORE_ADDR dwarf2_per_cu_text_offset (struct dwarf2_per_cu_data *cu);

/* Find a particular location expression from a location list.  */
const gdb_byte *dwarf2_find_location_expression
  (struct dwarf2_loclist_baton *baton,
   size_t *locexpr_length,
   CORE_ADDR pc);

struct dwarf2_locexpr_baton dwarf2_fetch_die_location_block
  (unsigned int offset, struct dwarf2_per_cu_data *per_cu,
   CORE_ADDR (*get_frame_pc) (void *baton),
   void *baton);

/* Evaluate a location description, starting at DATA and with length
   SIZE, to find the current location of variable of TYPE in the context
   of FRAME.  */

struct value *dwarf2_evaluate_loc_desc (struct type *type,
					struct frame_info *frame,
					const gdb_byte *data,
					unsigned short size,
					struct dwarf2_per_cu_data *per_cu);

/* The symbol location baton types used by the DWARF-2 reader (i.e.
   SYMBOL_LOCATION_BATON for a LOC_COMPUTED symbol).  "struct
   dwarf2_locexpr_baton" is for a symbol with a single location
   expression; "struct dwarf2_loclist_baton" is for a symbol with a
   location list.  */

struct dwarf2_locexpr_baton
{
  /* Pointer to the start of the location expression.  */
  const gdb_byte *data;

  /* Length of the location expression.  */
  unsigned long size;

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
  unsigned long size;

  /* The compilation unit containing the symbol whose location
     we're computing.  */
  struct dwarf2_per_cu_data *per_cu;
};

extern const struct symbol_computed_ops dwarf2_locexpr_funcs;
extern const struct symbol_computed_ops dwarf2_loclist_funcs;

/* Compile a DWARF location expression to an agent expression.
   
   EXPR is the agent expression we are building.
   LOC is the agent value we modify.
   ARCH is the architecture.
   ADDR_SIZE is the size of addresses, in bytes.
   OP_PTR is the start of the location expression.
   OP_END is one past the last byte of the location expression.
   
   This will throw an exception for various kinds of errors -- for
   example, if the expression cannot be compiled, or if the expression
   is invalid.  */

extern void dwarf2_compile_expr_to_ax (struct agent_expr *expr,
				       struct axs_value *loc,
				       struct gdbarch *arch,
				       unsigned int addr_size,
				       const gdb_byte *op_ptr,
				       const gdb_byte *op_end,
				       struct dwarf2_per_cu_data *per_cu);

#endif /* dwarf2loc.h */
