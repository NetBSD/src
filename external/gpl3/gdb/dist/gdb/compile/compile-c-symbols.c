/* Convert symbols from GDB to GCC

   Copyright (C) 2014-2016 Free Software Foundation, Inc.

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
#include "symtab.h"
#include "parser-defs.h"
#include "block.h"
#include "objfiles.h"
#include "compile.h"
#include "value.h"
#include "exceptions.h"
#include "gdbtypes.h"
#include "dwarf2loc.h"



/* Object of this type are stored in the compiler's symbol_err_map.  */

struct symbol_error
{
  /* The symbol.  */

  const struct symbol *sym;

  /* The error message to emit.  This is malloc'd and owned by the
     hash table.  */

  char *message;
};

/* Hash function for struct symbol_error.  */

static hashval_t
hash_symbol_error (const void *a)
{
  const struct symbol_error *se = (const struct symbol_error *) a;

  return htab_hash_pointer (se->sym);
}

/* Equality function for struct symbol_error.  */

static int
eq_symbol_error (const void *a, const void *b)
{
  const struct symbol_error *sea = (const struct symbol_error *) a;
  const struct symbol_error *seb = (const struct symbol_error *) b;

  return sea->sym == seb->sym;
}

/* Deletion function for struct symbol_error.  */

static void
del_symbol_error (void *a)
{
  struct symbol_error *se = (struct symbol_error *) a;

  xfree (se->message);
  xfree (se);
}

/* Associate SYMBOL with some error text.  */

static void
insert_symbol_error (htab_t hash, const struct symbol *sym, const char *text)
{
  struct symbol_error e;
  void **slot;

  e.sym = sym;
  slot = htab_find_slot (hash, &e, INSERT);
  if (*slot == NULL)
    {
      struct symbol_error *e = XNEW (struct symbol_error);

      e->sym = sym;
      e->message = xstrdup (text);
      *slot = e;
    }
}

/* Emit the error message corresponding to SYM, if one exists, and
   arrange for it not to be emitted again.  */

static void
error_symbol_once (struct compile_c_instance *context,
		   const struct symbol *sym)
{
  struct symbol_error search;
  struct symbol_error *err;
  char *message;

  if (context->symbol_err_map == NULL)
    return;

  search.sym = sym;
  err = (struct symbol_error *) htab_find (context->symbol_err_map, &search);
  if (err == NULL || err->message == NULL)
    return;

  message = err->message;
  err->message = NULL;
  make_cleanup (xfree, message);
  error (_("%s"), message);
}



/* Compute the name of the pointer representing a local symbol's
   address.  */

static char *
symbol_substitution_name (struct symbol *sym)
{
  return concat ("__", SYMBOL_NATURAL_NAME (sym), "_ptr", (char *) NULL);
}

/* Convert a given symbol, SYM, to the compiler's representation.
   CONTEXT is the compiler instance.  IS_GLOBAL is true if the
   symbol came from the global scope.  IS_LOCAL is true if the symbol
   came from a local scope.  (Note that the two are not strictly
   inverses because the symbol might have come from the static
   scope.)  */

static void
convert_one_symbol (struct compile_c_instance *context,
		    struct block_symbol sym,
		    int is_global,
		    int is_local)
{
  gcc_type sym_type;
  const char *filename = symbol_symtab (sym.symbol)->filename;
  unsigned short line = SYMBOL_LINE (sym.symbol);

  error_symbol_once (context, sym.symbol);

  if (SYMBOL_CLASS (sym.symbol) == LOC_LABEL)
    sym_type = 0;
  else
    sym_type = convert_type (context, SYMBOL_TYPE (sym.symbol));

  if (SYMBOL_DOMAIN (sym.symbol) == STRUCT_DOMAIN)
    {
      /* Binding a tag, so we don't need to build a decl.  */
      C_CTX (context)->c_ops->tagbind (C_CTX (context),
				       SYMBOL_NATURAL_NAME (sym.symbol),
				       sym_type, filename, line);
    }
  else
    {
      gcc_decl decl;
      enum gcc_c_symbol_kind kind;
      CORE_ADDR addr = 0;
      char *symbol_name = NULL;

      switch (SYMBOL_CLASS (sym.symbol))
	{
	case LOC_TYPEDEF:
	  kind = GCC_C_SYMBOL_TYPEDEF;
	  break;

	case LOC_LABEL:
	  kind = GCC_C_SYMBOL_LABEL;
	  addr = SYMBOL_VALUE_ADDRESS (sym.symbol);
	  break;

	case LOC_BLOCK:
	  kind = GCC_C_SYMBOL_FUNCTION;
	  addr = BLOCK_START (SYMBOL_BLOCK_VALUE (sym.symbol));
	  if (is_global && TYPE_GNU_IFUNC (SYMBOL_TYPE (sym.symbol)))
	    addr = gnu_ifunc_resolve_addr (target_gdbarch (), addr);
	  break;

	case LOC_CONST:
	  if (TYPE_CODE (SYMBOL_TYPE (sym.symbol)) == TYPE_CODE_ENUM)
	    {
	      /* Already handled by convert_enum.  */
	      return;
	    }
	  C_CTX (context)->c_ops->build_constant
	    (C_CTX (context),
	     sym_type, SYMBOL_NATURAL_NAME (sym.symbol),
	     SYMBOL_VALUE (sym.symbol),
	     filename, line);
	  return;

	case LOC_CONST_BYTES:
	  error (_("Unsupported LOC_CONST_BYTES for symbol \"%s\"."),
		 SYMBOL_PRINT_NAME (sym.symbol));

	case LOC_UNDEF:
	  internal_error (__FILE__, __LINE__, _("LOC_UNDEF found for \"%s\"."),
			  SYMBOL_PRINT_NAME (sym.symbol));

	case LOC_COMMON_BLOCK:
	  error (_("Fortran common block is unsupported for compilation "
		   "evaluaton of symbol \"%s\"."),
		 SYMBOL_PRINT_NAME (sym.symbol));

	case LOC_OPTIMIZED_OUT:
	  error (_("Symbol \"%s\" cannot be used for compilation evaluation "
		   "as it is optimized out."),
		 SYMBOL_PRINT_NAME (sym.symbol));

	case LOC_COMPUTED:
	  if (is_local)
	    goto substitution;
	  /* Probably TLS here.  */
	  warning (_("Symbol \"%s\" is thread-local and currently can only "
		     "be referenced from the current thread in "
		     "compiled code."),
		   SYMBOL_PRINT_NAME (sym.symbol));
	  /* FALLTHROUGH */
	case LOC_UNRESOLVED:
	  /* 'symbol_name' cannot be used here as that one is used only for
	     local variables from compile_dwarf_expr_to_c.
	     Global variables can be accessed by GCC only by their address, not
	     by their name.  */
	  {
	    struct value *val;
	    struct frame_info *frame = NULL;

	    if (symbol_read_needs_frame (sym.symbol))
	      {
		frame = get_selected_frame (NULL);
		if (frame == NULL)
		  error (_("Symbol \"%s\" cannot be used because "
			   "there is no selected frame"),
			 SYMBOL_PRINT_NAME (sym.symbol));
	      }

	    val = read_var_value (sym.symbol, sym.block, frame);
	    if (VALUE_LVAL (val) != lval_memory)
	      error (_("Symbol \"%s\" cannot be used for compilation "
		       "evaluation as its address has not been found."),
		     SYMBOL_PRINT_NAME (sym.symbol));

	    kind = GCC_C_SYMBOL_VARIABLE;
	    addr = value_address (val);
	  }
	  break;


	case LOC_REGISTER:
	case LOC_ARG:
	case LOC_REF_ARG:
	case LOC_REGPARM_ADDR:
	case LOC_LOCAL:
	substitution:
	  kind = GCC_C_SYMBOL_VARIABLE;
	  symbol_name = symbol_substitution_name (sym.symbol);
	  break;

	case LOC_STATIC:
	  kind = GCC_C_SYMBOL_VARIABLE;
	  addr = SYMBOL_VALUE_ADDRESS (sym.symbol);
	  break;

	case LOC_FINAL_VALUE:
	default:
	  gdb_assert_not_reached ("Unreachable case in convert_one_symbol.");

	}

      /* Don't emit local variable decls for a raw expression.  */
      if (context->base.scope != COMPILE_I_RAW_SCOPE
	  || symbol_name == NULL)
	{
	  decl = C_CTX (context)->c_ops->build_decl
	    (C_CTX (context),
	     SYMBOL_NATURAL_NAME (sym.symbol),
	     kind,
	     sym_type,
	     symbol_name, addr,
	     filename, line);

	  C_CTX (context)->c_ops->bind (C_CTX (context), decl, is_global);
	}

      xfree (symbol_name);
    }
}

/* Convert a full symbol to its gcc form.  CONTEXT is the compiler to
   use, IDENTIFIER is the name of the symbol, SYM is the symbol
   itself, and DOMAIN is the domain which was searched.  */

static void
convert_symbol_sym (struct compile_c_instance *context, const char *identifier,
		    struct block_symbol sym, domain_enum domain)
{
  const struct block *static_block;
  int is_local_symbol;

  /* If we found a symbol and it is not in the  static or global
     scope, then we should first convert any static or global scope
     symbol of the same name.  This lets this unusual case work:

     int x; // Global.
     int func(void)
     {
     int x;
     // At this spot, evaluate "extern int x; x"
     }
  */

  static_block = block_static_block (sym.block);
  /* STATIC_BLOCK is NULL if FOUND_BLOCK is the global block.  */
  is_local_symbol = (sym.block != static_block && static_block != NULL);
  if (is_local_symbol)
    {
      struct block_symbol global_sym;

      global_sym = lookup_symbol (identifier, NULL, domain, NULL);
      /* If the outer symbol is in the static block, we ignore it, as
	 it cannot be referenced.  */
      if (global_sym.symbol != NULL
	  && global_sym.block != block_static_block (global_sym.block))
	{
	  if (compile_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"gcc_convert_symbol \"%s\": global symbol\n",
				identifier);
	  convert_one_symbol (context, global_sym, 1, 0);
	}
    }

  if (compile_debug)
    fprintf_unfiltered (gdb_stdlog,
			"gcc_convert_symbol \"%s\": local symbol\n",
			identifier);
  convert_one_symbol (context, sym, 0, is_local_symbol);
}

/* Convert a minimal symbol to its gcc form.  CONTEXT is the compiler
   to use and BMSYM is the minimal symbol to convert.  */

static void
convert_symbol_bmsym (struct compile_c_instance *context,
		      struct bound_minimal_symbol bmsym)
{
  struct minimal_symbol *msym = bmsym.minsym;
  struct objfile *objfile = bmsym.objfile;
  struct type *type;
  enum gcc_c_symbol_kind kind;
  gcc_type sym_type;
  gcc_decl decl;
  CORE_ADDR addr;

  addr = MSYMBOL_VALUE_ADDRESS (objfile, msym);

  /* Conversion copied from write_exp_msymbol.  */
  switch (MSYMBOL_TYPE (msym))
    {
    case mst_text:
    case mst_file_text:
    case mst_solib_trampoline:
      type = objfile_type (objfile)->nodebug_text_symbol;
      kind = GCC_C_SYMBOL_FUNCTION;
      break;

    case mst_text_gnu_ifunc:
      /* nodebug_text_gnu_ifunc_symbol would cause:
	 function return type cannot be function  */
      type = objfile_type (objfile)->nodebug_text_symbol;
      kind = GCC_C_SYMBOL_FUNCTION;
      addr = gnu_ifunc_resolve_addr (target_gdbarch (), addr);
      break;

    case mst_data:
    case mst_file_data:
    case mst_bss:
    case mst_file_bss:
      type = objfile_type (objfile)->nodebug_data_symbol;
      kind = GCC_C_SYMBOL_VARIABLE;
      break;

    case mst_slot_got_plt:
      type = objfile_type (objfile)->nodebug_got_plt_symbol;
      kind = GCC_C_SYMBOL_FUNCTION;
      break;

    default:
      type = objfile_type (objfile)->nodebug_unknown_symbol;
      kind = GCC_C_SYMBOL_VARIABLE;
      break;
    }

  sym_type = convert_type (context, type);
  decl = C_CTX (context)->c_ops->build_decl (C_CTX (context),
					     MSYMBOL_NATURAL_NAME (msym),
					     kind, sym_type, NULL, addr,
					     NULL, 0);
  C_CTX (context)->c_ops->bind (C_CTX (context), decl, 1 /* is_global */);
}

/* See compile-internal.h.  */

void
gcc_convert_symbol (void *datum,
		    struct gcc_c_context *gcc_context,
		    enum gcc_c_oracle_request request,
		    const char *identifier)
{
  struct compile_c_instance *context = (struct compile_c_instance *) datum;
  domain_enum domain;
  int found = 0;

  switch (request)
    {
    case GCC_C_ORACLE_SYMBOL:
      domain = VAR_DOMAIN;
      break;
    case GCC_C_ORACLE_TAG:
      domain = STRUCT_DOMAIN;
      break;
    case GCC_C_ORACLE_LABEL:
      domain = LABEL_DOMAIN;
      break;
    default:
      gdb_assert_not_reached ("Unrecognized oracle request.");
    }

  /* We can't allow exceptions to escape out of this callback.  Safest
     is to simply emit a gcc error.  */
  TRY
    {
      struct block_symbol sym;

      sym = lookup_symbol (identifier, context->base.block, domain, NULL);
      if (sym.symbol != NULL)
	{
	  convert_symbol_sym (context, identifier, sym, domain);
	  found = 1;
	}
      else if (domain == VAR_DOMAIN)
	{
	  struct bound_minimal_symbol bmsym;

	  bmsym = lookup_minimal_symbol (identifier, NULL, NULL);
	  if (bmsym.minsym != NULL)
	    {
	      convert_symbol_bmsym (context, bmsym);
	      found = 1;
	    }
	}
    }

  CATCH (e, RETURN_MASK_ALL)
    {
      C_CTX (context)->c_ops->error (C_CTX (context), e.message);
    }
  END_CATCH

  if (compile_debug && !found)
    fprintf_unfiltered (gdb_stdlog,
			"gcc_convert_symbol \"%s\": lookup_symbol failed\n",
			identifier);
  return;
}

/* See compile-internal.h.  */

gcc_address
gcc_symbol_address (void *datum, struct gcc_c_context *gcc_context,
		    const char *identifier)
{
  struct compile_c_instance *context = (struct compile_c_instance *) datum;
  gcc_address result = 0;
  int found = 0;

  /* We can't allow exceptions to escape out of this callback.  Safest
     is to simply emit a gcc error.  */
  TRY
    {
      struct symbol *sym;

      /* We only need global functions here.  */
      sym = lookup_symbol (identifier, NULL, VAR_DOMAIN, NULL).symbol;
      if (sym != NULL && SYMBOL_CLASS (sym) == LOC_BLOCK)
	{
	  if (compile_debug)
	    fprintf_unfiltered (gdb_stdlog,
				"gcc_symbol_address \"%s\": full symbol\n",
				identifier);
	  result = BLOCK_START (SYMBOL_BLOCK_VALUE (sym));
	  if (TYPE_GNU_IFUNC (SYMBOL_TYPE (sym)))
	    result = gnu_ifunc_resolve_addr (target_gdbarch (), result);
	  found = 1;
	}
      else
	{
	  struct bound_minimal_symbol msym;

	  msym = lookup_bound_minimal_symbol (identifier);
	  if (msym.minsym != NULL)
	    {
	      if (compile_debug)
		fprintf_unfiltered (gdb_stdlog,
				    "gcc_symbol_address \"%s\": minimal "
				    "symbol\n",
				    identifier);
	      result = BMSYMBOL_VALUE_ADDRESS (msym);
	      if (MSYMBOL_TYPE (msym.minsym) == mst_text_gnu_ifunc)
		result = gnu_ifunc_resolve_addr (target_gdbarch (), result);
	      found = 1;
	    }
	}
    }

  CATCH (e, RETURN_MASK_ERROR)
    {
      C_CTX (context)->c_ops->error (C_CTX (context), e.message);
    }
  END_CATCH

  if (compile_debug && !found)
    fprintf_unfiltered (gdb_stdlog,
			"gcc_symbol_address \"%s\": failed\n",
			identifier);
  return result;
}



/* A hash function for symbol names.  */

static hashval_t
hash_symname (const void *a)
{
  const struct symbol *sym = (const struct symbol *) a;

  return htab_hash_string (SYMBOL_NATURAL_NAME (sym));
}

/* A comparison function for hash tables that just looks at symbol
   names.  */

static int
eq_symname (const void *a, const void *b)
{
  const struct symbol *syma = (const struct symbol *) a;
  const struct symbol *symb = (const struct symbol *) b;

  return strcmp (SYMBOL_NATURAL_NAME (syma), SYMBOL_NATURAL_NAME (symb)) == 0;
}

/* If a symbol with the same name as SYM is already in HASHTAB, return
   1.  Otherwise, add SYM to HASHTAB and return 0.  */

static int
symbol_seen (htab_t hashtab, struct symbol *sym)
{
  void **slot;

  slot = htab_find_slot (hashtab, sym, INSERT);
  if (*slot != NULL)
    return 1;

  *slot = sym;
  return 0;
}

/* Generate C code to compute the length of a VLA.  */

static void
generate_vla_size (struct compile_c_instance *compiler,
		   struct ui_file *stream,
		   struct gdbarch *gdbarch,
		   unsigned char *registers_used,
		   CORE_ADDR pc,
		   struct type *type,
		   struct symbol *sym)
{
  type = check_typedef (type);

  if (TYPE_CODE (type) == TYPE_CODE_REF)
    type = check_typedef (TYPE_TARGET_TYPE (type));

  switch (TYPE_CODE (type))
    {
    case TYPE_CODE_RANGE:
      {
	if (TYPE_HIGH_BOUND_KIND (type) == PROP_LOCEXPR
	    || TYPE_HIGH_BOUND_KIND (type) == PROP_LOCLIST)
	  {
	    const struct dynamic_prop *prop = &TYPE_RANGE_DATA (type)->high;
	    char *name = c_get_range_decl_name (prop);
	    struct cleanup *cleanup = make_cleanup (xfree, name);

	    dwarf2_compile_property_to_c (stream, name,
					  gdbarch, registers_used,
					  prop, pc, sym);
	    do_cleanups (cleanup);
	  }
      }
      break;

    case TYPE_CODE_ARRAY:
      generate_vla_size (compiler, stream, gdbarch, registers_used, pc,
			 TYPE_INDEX_TYPE (type), sym);
      generate_vla_size (compiler, stream, gdbarch, registers_used, pc,
			 TYPE_TARGET_TYPE (type), sym);
      break;

    case TYPE_CODE_UNION:
    case TYPE_CODE_STRUCT:
      {
	int i;

	for (i = 0; i < TYPE_NFIELDS (type); ++i)
	  if (!field_is_static (&TYPE_FIELD (type, i)))
	    generate_vla_size (compiler, stream, gdbarch, registers_used, pc,
			       TYPE_FIELD_TYPE (type, i), sym);
      }
      break;
    }
}

/* Generate C code to compute the address of SYM.  */

static void
generate_c_for_for_one_variable (struct compile_c_instance *compiler,
				 struct ui_file *stream,
				 struct gdbarch *gdbarch,
				 unsigned char *registers_used,
				 CORE_ADDR pc,
				 struct symbol *sym)
{

  TRY
    {
      if (is_dynamic_type (SYMBOL_TYPE (sym)))
	{
	  struct ui_file *size_file = mem_fileopen ();
	  struct cleanup *cleanup = make_cleanup_ui_file_delete (size_file);

	  generate_vla_size (compiler, size_file, gdbarch, registers_used, pc,
			     SYMBOL_TYPE (sym), sym);
	  ui_file_put (size_file, ui_file_write_for_put, stream);

	  do_cleanups (cleanup);
	}

      if (SYMBOL_COMPUTED_OPS (sym) != NULL)
	{
	  char *generated_name = symbol_substitution_name (sym);
	  struct cleanup *cleanup = make_cleanup (xfree, generated_name);
	  /* We need to emit to a temporary buffer in case an error
	     occurs in the middle.  */
	  struct ui_file *local_file = mem_fileopen ();

	  make_cleanup_ui_file_delete (local_file);
	  SYMBOL_COMPUTED_OPS (sym)->generate_c_location (sym, local_file,
							  gdbarch,
							  registers_used,
							  pc, generated_name);
	  ui_file_put (local_file, ui_file_write_for_put, stream);

	  do_cleanups (cleanup);
	}
      else
	{
	  switch (SYMBOL_CLASS (sym))
	    {
	    case LOC_REGISTER:
	    case LOC_ARG:
	    case LOC_REF_ARG:
	    case LOC_REGPARM_ADDR:
	    case LOC_LOCAL:
	      error (_("Local symbol unhandled when generating C code."));

	    case LOC_COMPUTED:
	      gdb_assert_not_reached (_("LOC_COMPUTED variable "
					"missing a method."));

	    default:
	      /* Nothing to do for all other cases, as they don't represent
		 local variables.  */
	      break;
	    }
	}
    }

  CATCH (e, RETURN_MASK_ERROR)
    {
      if (compiler->symbol_err_map == NULL)
	compiler->symbol_err_map = htab_create_alloc (10,
						      hash_symbol_error,
						      eq_symbol_error,
						      del_symbol_error,
						      xcalloc,
						      xfree);
      insert_symbol_error (compiler->symbol_err_map, sym, e.message);
    }
  END_CATCH
}

/* See compile-internal.h.  */

unsigned char *
generate_c_for_variable_locations (struct compile_c_instance *compiler,
				   struct ui_file *stream,
				   struct gdbarch *gdbarch,
				   const struct block *block,
				   CORE_ADDR pc)
{
  struct cleanup *cleanup, *outer;
  htab_t symhash;
  const struct block *static_block = block_static_block (block);
  unsigned char *registers_used;

  /* If we're already in the static or global block, there is nothing
     to write.  */
  if (static_block == NULL || block == static_block)
    return NULL;

  registers_used = XCNEWVEC (unsigned char, gdbarch_num_regs (gdbarch));
  outer = make_cleanup (xfree, registers_used);

  /* Ensure that a given name is only entered once.  This reflects the
     reality of shadowing.  */
  symhash = htab_create_alloc (1, hash_symname, eq_symname, NULL,
			       xcalloc, xfree);
  cleanup = make_cleanup_htab_delete (symhash);

  while (1)
    {
      struct symbol *sym;
      struct block_iterator iter;

      /* Iterate over symbols in this block, generating code to
	 compute the location of each local variable.  */
      for (sym = block_iterator_first (block, &iter);
	   sym != NULL;
	   sym = block_iterator_next (&iter))
	{
	  if (!symbol_seen (symhash, sym))
	    generate_c_for_for_one_variable (compiler, stream, gdbarch,
					     registers_used, pc, sym);
	}

      /* If we just finished the outermost block of a function, we're
	 done.  */
      if (BLOCK_FUNCTION (block) != NULL)
	break;
      block = BLOCK_SUPERBLOCK (block);
    }

  do_cleanups (cleanup);
  discard_cleanups (outer);
  return registers_used;
}
