/* Header file for GDB compile command and supporting functions.
   Copyright (C) 2014-2023 Free Software Foundation, Inc.

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

#ifndef COMPILE_COMPILE_INTERNAL_H
#define COMPILE_COMPILE_INTERNAL_H

#include "gcc-c-interface.h"
#include "gdbsupport/gdb-hashtab.h"

/* Debugging flag for the "compile" family of commands.  */

extern bool compile_debug;

struct block;

/* An object that maps a gdb type to a gcc type.  */

struct type_map_instance
{
  /* The gdb type.  */

  struct type *type;

  /* The corresponding gcc type handle.  */

  gcc_type gcc_type_handle;
};

/* An object of this type holds state associated with a given
   compilation job.  */

class compile_instance
{
public:
  compile_instance (struct gcc_base_context *gcc_fe, const char *options);

  virtual ~compile_instance ()
  {
    m_gcc_fe->ops->destroy (m_gcc_fe);
  }

  /* Returns the GCC options to be passed during compilation.  */
  const std::string &gcc_target_options () const
  {
    return m_gcc_target_options;
  }

  /* Query the type cache for TYPE, returning the compiler's
     type for it in RET.  */
  bool get_cached_type (struct type *type, gcc_type *ret) const;

  /* Insert GCC_TYPE into the type cache for TYPE.

     It is ok for a given type to be inserted more than once, provided that
     the exact same association is made each time.  */
  void insert_type (struct type *type, gcc_type gcc_type);

  /* Associate SYMBOL with some error text.  */
  void insert_symbol_error (const struct symbol *sym, const char *text);

  /* Emit the error message corresponding to SYM, if one exists, and
     arrange for it not to be emitted again.  */
  void error_symbol_once (const struct symbol *sym);

  /* These currently just forward to the underlying ops
     vtable.  */

  /* Set the plug-in print callback.  */
  void set_print_callback (void (*print_function) (void *, const char *),
			   void *datum);

  /* Return the plug-in's front-end version.  */
  unsigned int version () const;

  /* Set the plug-in's verbosity level.  Nop for GCC_FE_VERSION_0.  */
  void set_verbose (int level);

  /* Set the plug-in driver program.  Nop for GCC_FE_VERSION_0.  */
  void set_driver_filename (const char *filename);

  /* Set the regular expression used to match the configury triplet
     prefix to the compiler.  Nop for GCC_FE_VERSION_0.  */
  void set_triplet_regexp (const char *regexp);

  /* Set compilation arguments.  REGEXP is only used for protocol
     version GCC_FE_VERSION_0.  */
  gdb::unique_xmalloc_ptr<char> set_arguments (int argc, char **argv,
					       const char *regexp = NULL);

  /* Set the filename of the program to compile.  Nop for GCC_FE_VERSION_0.  */
  void set_source_file (const char *filename);

  /* Compile the previously specified source file to FILENAME.
     VERBOSE_LEVEL is only used for protocol version GCC_FE_VERSION_0.  */
  bool compile (const char *filename, int verbose_level = -1);

  /* Set the scope type for this compile.  */
  void set_scope (enum compile_i_scope_types scope)
  {
    m_scope = scope;
  }

  /* Return the scope type.  */
  enum compile_i_scope_types scope () const
  {
    return m_scope;
  }

  /* Set the block to be used for symbol searches.  */
  void set_block (const struct block *block)
  {
    m_block = block;
  }

  /* Return the search block.  */
  const struct block *block () const
  {
    return m_block;
  }

protected:

  /* The GCC front end.  */
  struct gcc_base_context *m_gcc_fe;

  /* The "scope" of this compilation.  */
  enum compile_i_scope_types m_scope;

  /* The block in which an expression is being parsed.  */
  const struct block *m_block;

  /* Specify "-std=gnu11", "-std=gnu++11" or similar.  These options are put
     after CU's DW_AT_producer compilation options to override them.  */
  std::string m_gcc_target_options;

  /* Map from gdb types to gcc types.  */
  htab_up m_type_map;

  /* Map from gdb symbols to gcc error messages to emit.  */
  htab_up m_symbol_err_map;
};

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

/* A "type" to indicate a NULL type.  */

const gcc_type GCC_TYPE_NONE = (gcc_type) -1;

/* Call gdbarch_register_name (GDBARCH, REGNUM) and convert its result
   to a form suitable for the compiler source.  The register names
   should not clash with inferior defined macros. */

extern std::string compile_register_name_mangled (struct gdbarch *gdbarch,
						  int regnum);

/* Convert compiler source register name to register number of
   GDBARCH.  Returned value is always >= 0, function throws an error
   for non-matching REG_NAME.  */

extern int compile_register_name_demangle (struct gdbarch *gdbarch,
					   const char *reg_name);

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

#endif /* COMPILE_COMPILE_INTERNAL_H */
