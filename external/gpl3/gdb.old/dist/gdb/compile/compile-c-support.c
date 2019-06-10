/* C language support for compilation.

   Copyright (C) 2014-2017 Free Software Foundation, Inc.

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

#include "defs.h"
#include "compile-internal.h"
#include "compile.h"
#include "gdb-dlfcn.h"
#include "c-lang.h"
#include "macrotab.h"
#include "macroscope.h"
#include "regcache.h"
#include "common/function-view.h"

/* See compile-internal.h.  */

const char *
c_get_mode_for_size (int size)
{
  const char *mode = NULL;

  switch (size)
    {
    case 1:
      mode = "QI";
      break;
    case 2:
      mode = "HI";
      break;
    case 4:
      mode = "SI";
      break;
    case 8:
      mode = "DI";
      break;
    default:
      internal_error (__FILE__, __LINE__, _("Invalid GCC mode size %d."), size);
    }

  return mode;
}

/* See compile-internal.h.  */

char *
c_get_range_decl_name (const struct dynamic_prop *prop)
{
  return xstrprintf ("__gdb_prop_%s", host_address_to_string (prop));
}



#define STR(x) #x
#define STRINGIFY(x) STR(x)

/* Helper function for c_get_compile_context.  Open the GCC front-end
   shared library and return the symbol specified by the current
   GCC_C_FE_CONTEXT.  */

static gcc_c_fe_context_function *
load_libcc (void)
{
  gcc_c_fe_context_function *func;

   /* gdb_dlopen will call error () on an error, so no need to check
      value.  */
  gdb_dlhandle_up handle = gdb_dlopen (STRINGIFY (GCC_C_FE_LIBCC));
  func = (gcc_c_fe_context_function *) gdb_dlsym (handle,
						  STRINGIFY (GCC_C_FE_CONTEXT));

  if (func == NULL)
    error (_("could not find symbol %s in library %s"),
	   STRINGIFY (GCC_C_FE_CONTEXT),
	   STRINGIFY (GCC_C_FE_LIBCC));

  /* Leave the library open.  */
  handle.release ();
  return func;
}

/* Return the compile instance associated with the current context.
   This function calls the symbol returned from the load_libcc
   function.  This will provide the gcc_c_context.  */

struct compile_instance *
c_get_compile_context (void)
{
  static gcc_c_fe_context_function *func;

  struct gcc_c_context *context;

  if (func == NULL)
    {
      func = load_libcc ();
      gdb_assert (func != NULL);
    }

  context = (*func) (GCC_FE_VERSION_0, GCC_C_FE_VERSION_0);
  if (context == NULL)
    error (_("The loaded version of GCC does not support the required version "
	     "of the API."));

  return new_compile_instance (context);
}



/* Write one macro definition.  */

static void
print_one_macro (const char *name, const struct macro_definition *macro,
		 struct macro_source_file *source, int line,
		 ui_file *file)
{
  /* Don't print command-line defines.  They will be supplied another
     way.  */
  if (line == 0)
    return;

  /* None of -Wno-builtin-macro-redefined, #undef first
     or plain #define of the same value would avoid a warning.  */
  fprintf_filtered (file, "#ifndef %s\n# define %s", name, name);

  if (macro->kind == macro_function_like)
    {
      int i;

      fputs_filtered ("(", file);
      for (i = 0; i < macro->argc; i++)
	{
	  fputs_filtered (macro->argv[i], file);
	  if (i + 1 < macro->argc)
	    fputs_filtered (", ", file);
	}
      fputs_filtered (")", file);
    }

  fprintf_filtered (file, " %s\n#endif\n", macro->replacement);
}

/* Write macro definitions at PC to FILE.  */

static void
write_macro_definitions (const struct block *block, CORE_ADDR pc,
			 struct ui_file *file)
{
  struct macro_scope *scope;

  if (block != NULL)
    scope = sal_macro_scope (find_pc_line (pc, 0));
  else
    scope = default_macro_scope ();
  if (scope == NULL)
    scope = user_macro_scope ();

  if (scope != NULL && scope->file != NULL && scope->file->table != NULL)
    {
      macro_for_each_in_scope (scope->file, scope->line,
			       [&] (const char *name,
				    const macro_definition *macro,
				    macro_source_file *source,
				    int line)
	{
	  print_one_macro (name, macro, source, line, file);
	});
    }
}

/* Helper function to construct a header scope for a block of code.
   Takes a scope argument which selects the correct header to
   insert into BUF.  */

static void
add_code_header (enum compile_i_scope_types type, struct ui_file *buf)
{
  switch (type)
    {
    case COMPILE_I_SIMPLE_SCOPE:
      fputs_unfiltered ("void "
			GCC_FE_WRAPPER_FUNCTION
			" (struct "
			COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG
			" *"
			COMPILE_I_SIMPLE_REGISTER_ARG_NAME
			") {\n",
			buf);
      break;
    case COMPILE_I_PRINT_ADDRESS_SCOPE:
    case COMPILE_I_PRINT_VALUE_SCOPE:
      /* <string.h> is needed for a memcpy call below.  */
      fputs_unfiltered ("#include <string.h>\n"
			"void "
			GCC_FE_WRAPPER_FUNCTION
			" (struct "
			COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG
			" *"
			COMPILE_I_SIMPLE_REGISTER_ARG_NAME
			", "
			COMPILE_I_PRINT_OUT_ARG_TYPE
			" "
			COMPILE_I_PRINT_OUT_ARG
			") {\n",
			buf);
      break;

    case COMPILE_I_RAW_SCOPE:
      break;
    default:
      gdb_assert_not_reached (_("Unknown compiler scope reached."));
    }
}

/* Helper function to construct a footer scope for a block of code.
   Takes a scope argument which selects the correct footer to
   insert into BUF.  */

static void
add_code_footer (enum compile_i_scope_types type, struct ui_file *buf)
{
  switch (type)
    {
    case COMPILE_I_SIMPLE_SCOPE:
    case COMPILE_I_PRINT_ADDRESS_SCOPE:
    case COMPILE_I_PRINT_VALUE_SCOPE:
      fputs_unfiltered ("}\n", buf);
      break;
    case COMPILE_I_RAW_SCOPE:
      break;
    default:
      gdb_assert_not_reached (_("Unknown compiler scope reached."));
    }
}

/* Generate a structure holding all the registers used by the function
   we're generating.  */

static void
generate_register_struct (struct ui_file *stream, struct gdbarch *gdbarch,
			  const unsigned char *registers_used)
{
  int i;
  int seen = 0;

  fputs_unfiltered ("struct " COMPILE_I_SIMPLE_REGISTER_STRUCT_TAG " {\n",
		    stream);

  if (registers_used != NULL)
    for (i = 0; i < gdbarch_num_regs (gdbarch); ++i)
      {
	if (registers_used[i])
	  {
	    struct type *regtype = check_typedef (register_type (gdbarch, i));
	    char *regname = compile_register_name_mangled (gdbarch, i);
	    struct cleanup *cleanups = make_cleanup (xfree, regname);

	    seen = 1;

	    /* You might think we could use type_print here.  However,
	       target descriptions often use types with names like
	       "int64_t", which may not be defined in the inferior
	       (and in any case would not be looked up due to the
	       #pragma business).  So, we take a much simpler
	       approach: for pointer- or integer-typed registers, emit
	       the field in the most direct way; and for other
	       register types (typically flags or vectors), emit a
	       maximally-aligned array of the correct size.  */

	    fputs_unfiltered ("  ", stream);
	    switch (TYPE_CODE (regtype))
	      {
	      case TYPE_CODE_PTR:
		fprintf_filtered (stream, "__gdb_uintptr %s", regname);
		break;

	      case TYPE_CODE_INT:
		{
		  const char *mode
		    = c_get_mode_for_size (TYPE_LENGTH (regtype));

		  if (mode != NULL)
		    {
		      if (TYPE_UNSIGNED (regtype))
			fputs_unfiltered ("unsigned ", stream);
		      fprintf_unfiltered (stream,
					  "int %s"
					  " __attribute__ ((__mode__(__%s__)))",
					  regname,
					  mode);
		      break;
		    }
		}

		/* Fall through.  */

	      default:
		fprintf_unfiltered (stream,
				    "  unsigned char %s[%d]"
				    " __attribute__((__aligned__("
				    "__BIGGEST_ALIGNMENT__)))",
				    regname,
				    TYPE_LENGTH (regtype));
	      }
	    fputs_unfiltered (";\n", stream);

	    do_cleanups (cleanups);
	  }
      }

  if (!seen)
    fputs_unfiltered ("  char " COMPILE_I_SIMPLE_REGISTER_DUMMY ";\n",
		      stream);

  fputs_unfiltered ("};\n\n", stream);
}

/* Take the source code provided by the user with the 'compile'
   command, and compute the additional wrapping, macro, variable and
   register operations needed.  INPUT is the source code derived from
   the 'compile' command, GDBARCH is the architecture to use when
   computing above, EXPR_BLOCK denotes the block relevant contextually
   to the inferior when the expression was created, and EXPR_PC
   indicates the value of $PC.  */

std::string
c_compute_program (struct compile_instance *inst,
		   const char *input,
		   struct gdbarch *gdbarch,
		   const struct block *expr_block,
		   CORE_ADDR expr_pc)
{
  struct compile_c_instance *context = (struct compile_c_instance *) inst;

  string_file buf;
  string_file var_stream;

  write_macro_definitions (expr_block, expr_pc, &buf);

  /* Do not generate local variable information for "raw"
     compilations.  In this case we aren't emitting our own function
     and the user's code may only refer to globals.  */
  if (inst->scope != COMPILE_I_RAW_SCOPE)
    {
      unsigned char *registers_used;
      int i;

      /* Generate the code to compute variable locations, but do it
	 before generating the function header, so we can define the
	 register struct before the function body.  This requires a
	 temporary stream.  */
      registers_used = generate_c_for_variable_locations (context,
							  var_stream, gdbarch,
							  expr_block, expr_pc);
      make_cleanup (xfree, registers_used);

      buf.puts ("typedef unsigned int"
		" __attribute__ ((__mode__(__pointer__)))"
		" __gdb_uintptr;\n");
      buf.puts ("typedef int"
		" __attribute__ ((__mode__(__pointer__)))"
		" __gdb_intptr;\n");

      /* Iterate all log2 sizes in bytes supported by c_get_mode_for_size.  */
      for (i = 0; i < 4; ++i)
	{
	  const char *mode = c_get_mode_for_size (1 << i);

	  gdb_assert (mode != NULL);
	  buf.printf ("typedef int"
		      " __attribute__ ((__mode__(__%s__)))"
		      " __gdb_int_%s;\n",
		      mode, mode);
	}

      generate_register_struct (&buf, gdbarch, registers_used);
    }

  add_code_header (inst->scope, &buf);

  if (inst->scope == COMPILE_I_SIMPLE_SCOPE
      || inst->scope == COMPILE_I_PRINT_ADDRESS_SCOPE
      || inst->scope == COMPILE_I_PRINT_VALUE_SCOPE)
    {
      buf.write (var_stream.c_str (), var_stream.size ());
      buf.puts ("#pragma GCC user_expression\n");
    }

  /* The user expression has to be in its own scope, so that "extern"
     works properly.  Otherwise gcc thinks that the "extern"
     declaration is in the same scope as the declaration provided by
     gdb.  */
  if (inst->scope != COMPILE_I_RAW_SCOPE)
    buf.puts ("{\n");

  buf.puts ("#line 1 \"gdb command line\"\n");

  switch (inst->scope)
    {
    case COMPILE_I_PRINT_ADDRESS_SCOPE:
    case COMPILE_I_PRINT_VALUE_SCOPE:
      buf.printf (
"__auto_type " COMPILE_I_EXPR_VAL " = %s;\n"
"typeof (%s) *" COMPILE_I_EXPR_PTR_TYPE ";\n"
"memcpy (" COMPILE_I_PRINT_OUT_ARG ", %s" COMPILE_I_EXPR_VAL ",\n"
	 "sizeof (*" COMPILE_I_EXPR_PTR_TYPE "));\n"
			  , input, input,
			  (inst->scope == COMPILE_I_PRINT_ADDRESS_SCOPE
			   ? "&" : ""));
      break;
    default:
      buf.puts (input);
      break;
    }

  buf.puts ("\n");

  /* For larger user expressions the automatic semicolons may be
     confusing.  */
  if (strchr (input, '\n') == NULL)
    buf.puts (";\n");

  if (inst->scope != COMPILE_I_RAW_SCOPE)
    buf.puts ("}\n");

  add_code_footer (inst->scope, &buf);
  return std::move (buf.string ());
}
