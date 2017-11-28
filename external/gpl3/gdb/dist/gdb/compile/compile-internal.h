/* Header file for GDB compile command and supporting functions.
   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

#ifndef GDB_COMPILE_INTERNAL_H
#define GDB_COMPILE_INTERNAL_H

#include "hashtab.h"
#include "gcc-c-interface.h"
#include "common/enum-flags.h"

/* enum-flags wrapper.  */
DEF_ENUM_FLAGS_TYPE (enum gcc_qualifiers, gcc_qualifiers_flags);

/* Debugging flag for the "compile" family of commands.  */

extern int compile_debug;

struct block;

/* An object of this type holds state associated with a given
   compilation job.  */

struct compile_instance
{
  /* The GCC front end.  */

  struct gcc_base_context *fe;

  /* The "scope" of this compilation.  */

  enum compile_i_scope_types scope;

  /* The block in which an expression is being parsed.  */

  const struct block *block;

  /* Specify "-std=gnu11", "-std=gnu++11" or similar.  These options are put
     after CU's DW_AT_producer compilation options to override them.  */

  const char *gcc_target_options;

  /* How to destroy this object.  */

  void (*destroy) (struct compile_instance *);
};

/* A subclass of compile_instance that is specific to the C front
   end.  */
struct compile_c_instance
{
  /* Base class.  Note that the base class vtable actually points to a
     gcc_c_fe_vtable.  */

  struct compile_instance base;

  /* Map from gdb types to gcc types.  */

  htab_t type_map;

  /* Map from gdb symbols to gcc error messages to emit.  */

  htab_t symbol_err_map;
};

/* A helper macro that takes a compile_c_instance and returns its
   corresponding gcc_c_context.  */

#define C_CTX(I) ((struct gcc_c_context *) ((I)->base.fe))

/* Define header and footers for different scopes.  */

/* A simple scope just declares a function named "_gdb_expr", takes no
   arguments and returns no value.  */

#define COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG "__gdb_regs"
#define COMPILE_I_SIMPLE_REGISTER_ARG_NAME "__regs"
#define COMPILE_I_SIMPLE_REGISTER_DUMMY "_dummy"
#define COMPILE_I_PRINT_OUT_ARG_TYPE "void *"
#define COMPILE_I_PRINT_OUT_ARG "__gdb_out_param"
#define COMPILE_I_EXPR_VAL "__gdb_expr_val"
#define COMPILE_I_EXPR_PTR_TYPE "__gdb_expr_ptr_type"

/* Call gdbarch_register_name (GDBARCH, REGNUM) and convert its result
   to a form suitable for the compiler source.  The register names
   should not clash with inferior defined macros.  Returned pointer is
   never NULL.  Returned pointer needs to be deallocated by xfree.  */

extern char *compile_register_name_mangled (struct gdbarch *gdbarch,
					    int regnum);

/* Convert compiler source register name to register number of
   GDBARCH.  Returned value is always >= 0, function throws an error
   for non-matching REG_NAME.  */

extern int compile_register_name_demangle (struct gdbarch *gdbarch,
					   const char *reg_name);

/* Convert a gdb type, TYPE, to a GCC type.  CONTEXT is used to do the
   actual conversion.  The new GCC type is returned.  */

struct type;
extern gcc_type convert_type (struct compile_c_instance *context,
			      struct type *type);

/* A callback suitable for use as the GCC C symbol oracle.  */

extern gcc_c_oracle_function gcc_convert_symbol;

/* A callback suitable for use as the GCC C address oracle.  */

extern gcc_c_symbol_address_function gcc_symbol_address;

/* Instantiate a GDB object holding state for the GCC context FE.  The
   new object is returned.  */

extern struct compile_instance *new_compile_instance (struct gcc_c_context *fe);

/* Emit code to compute the address for all the local variables in
   scope at PC in BLOCK.  Returns a malloc'd vector, indexed by gdb
   register number, where each element indicates if the corresponding
   register is needed to compute a local variable.  */

extern unsigned char *generate_c_for_variable_locations
     (struct compile_c_instance *compiler,
      string_file &stream,
      struct gdbarch *gdbarch,
      const struct block *block,
      CORE_ADDR pc);

/* Get the GCC mode attribute value for a given type size.  */

extern const char *c_get_mode_for_size (int size);

/* Given a dynamic property, return an xmallocd name that is used to
   represent its size.  The result must be freed by the caller.  The
   contents of the resulting string will be the same each time for
   each call with the same argument.  */

struct dynamic_prop;
extern char *c_get_range_decl_name (const struct dynamic_prop *prop);

/* Type used to hold and pass around the source and object file names
   to use for compilation.  */
class compile_file_names
{
public:
  compile_file_names (std::string source_file, std::string object_file)
    : m_source_file (source_file), m_object_file (object_file)
  {}

  /* Provide read-only views only.  Return 'const char *' instead of
     std::string to avoid having to use c_str() everywhere in client
     code.  */

  const char *source_file () const
  { return m_source_file.c_str (); }

  const char *object_file () const
  { return m_object_file.c_str (); }

private:
  /* Storage for the file names.  */
  std::string m_source_file;
  std::string m_object_file;
};

#endif /* GDB_COMPILE_INTERNAL_H */
