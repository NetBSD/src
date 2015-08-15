/* Print and select stack frames for GDB, the GNU debugger.

   Copyright (C) 1986-2014 Free Software Foundation, Inc.

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
#include "value.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "language.h"
#include "frame.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "target.h"
#include "source.h"
#include "breakpoint.h"
#include "demangle.h"
#include "inferior.h"
#include "annotate.h"
#include "ui-out.h"
#include "block.h"
#include "stack.h"
#include "dictionary.h"
#include "exceptions.h"
#include "reggroups.h"
#include "regcache.h"
#include "solib.h"
#include "valprint.h"
#include "gdbthread.h"
#include "cp-support.h"
#include "disasm.h"
#include "inline-frame.h"
#include "linespec.h"
#include "cli/cli-utils.h"

#include "gdb_assert.h"
#include <ctype.h>
#include <string.h>

#include "symfile.h"
#include "python/python.h"

void (*deprecated_selected_frame_level_changed_hook) (int);

/* The possible choices of "set print frame-arguments", and the value
   of this setting.  */

static const char *const print_frame_arguments_choices[] =
  {"all", "scalars", "none", NULL};
static const char *print_frame_arguments = "scalars";

/* If non-zero, don't invoke pretty-printers for frame arguments.  */
static int print_raw_frame_arguments;

/* The possible choices of "set print entry-values", and the value
   of this setting.  */

const char print_entry_values_no[] = "no";
const char print_entry_values_only[] = "only";
const char print_entry_values_preferred[] = "preferred";
const char print_entry_values_if_needed[] = "if-needed";
const char print_entry_values_both[] = "both";
const char print_entry_values_compact[] = "compact";
const char print_entry_values_default[] = "default";
static const char *const print_entry_values_choices[] =
{
  print_entry_values_no,
  print_entry_values_only,
  print_entry_values_preferred,
  print_entry_values_if_needed,
  print_entry_values_both,
  print_entry_values_compact,
  print_entry_values_default,
  NULL
};
const char *print_entry_values = print_entry_values_default;

/* Prototypes for local functions.  */

static void print_frame_local_vars (struct frame_info *, int,
				    struct ui_file *);

static void print_frame (struct frame_info *frame, int print_level,
			 enum print_what print_what,  int print_args,
			 struct symtab_and_line sal);

static void set_last_displayed_sal (int valid,
				    struct program_space *pspace,
				    CORE_ADDR addr,
				    struct symtab *symtab,
				    int line);

/* Zero means do things normally; we are interacting directly with the
   user.  One means print the full filename and linenumber when a
   frame is printed, and do so in a format emacs18/emacs19.22 can
   parse.  Two means print similar annotations, but in many more
   cases and in a slightly different syntax.  */

int annotation_level = 0;

/* These variables hold the last symtab and line we displayed to the user.
 * This is where we insert a breakpoint or a skiplist entry by default.  */
static int last_displayed_sal_valid = 0;
static struct program_space *last_displayed_pspace = 0;
static CORE_ADDR last_displayed_addr = 0;
static struct symtab *last_displayed_symtab = 0;
static int last_displayed_line = 0;


/* Return 1 if we should display the address in addition to the location,
   because we are in the middle of a statement.  */

static int
frame_show_address (struct frame_info *frame,
		    struct symtab_and_line sal)
{
  /* If there is a line number, but no PC, then there is no location
     information associated with this sal.  The only way that should
     happen is for the call sites of inlined functions (SAL comes from
     find_frame_sal).  Otherwise, we would have some PC range if the
     SAL came from a line table.  */
  if (sal.line != 0 && sal.pc == 0 && sal.end == 0)
    {
      if (get_next_frame (frame) == NULL)
	gdb_assert (inline_skipped_frames (inferior_ptid) > 0);
      else
	gdb_assert (get_frame_type (get_next_frame (frame)) == INLINE_FRAME);
      return 0;
    }

  return get_frame_pc (frame) != sal.pc;
}

/* Show or print a stack frame FRAME briefly.  The output is format
   according to PRINT_LEVEL and PRINT_WHAT printing the frame's
   relative level, function name, argument list, and file name and
   line number.  If the frame's PC is not at the beginning of the
   source line, the actual PC is printed at the beginning.  */

void
print_stack_frame (struct frame_info *frame, int print_level,
		   enum print_what print_what,
		   int set_current_sal)
{
  volatile struct gdb_exception e;

  /* For mi, alway print location and address.  */
  if (ui_out_is_mi_like_p (current_uiout))
    print_what = LOC_AND_ADDRESS;

  TRY_CATCH (e, RETURN_MASK_ERROR)
    {
      int center = (print_what == SRC_LINE || print_what == SRC_AND_LOC);

      print_frame_info (frame, print_level, print_what, 1 /* print_args */,
			set_current_sal);
      if (set_current_sal)
	set_current_sal_from_frame (frame, center);
    }
}

/* Print nameless arguments of frame FRAME on STREAM, where START is
   the offset of the first nameless argument, and NUM is the number of
   nameless arguments to print.  FIRST is nonzero if this is the first
   argument (not just the first nameless argument).  */

static void
print_frame_nameless_args (struct frame_info *frame, long start, int num,
			   int first, struct ui_file *stream)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
  int i;
  CORE_ADDR argsaddr;
  long arg_value;

  for (i = 0; i < num; i++)
    {
      QUIT;
      argsaddr = get_frame_args_address (frame);
      if (!argsaddr)
	return;
      arg_value = read_memory_integer (argsaddr + start,
				       sizeof (int), byte_order);
      if (!first)
	fprintf_filtered (stream, ", ");
      fprintf_filtered (stream, "%ld", arg_value);
      first = 0;
      start += sizeof (int);
    }
}

/* Print single argument of inferior function.  ARG must be already
   read in.

   Errors are printed as if they would be the parameter value.  Use zeroed ARG
   iff it should not be printed accoring to user settings.  */

static void
print_frame_arg (const struct frame_arg *arg)
{
  struct ui_out *uiout = current_uiout;
  volatile struct gdb_exception except;
  struct cleanup *old_chain;
  struct ui_file *stb;

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  gdb_assert (!arg->val || !arg->error);
  gdb_assert (arg->entry_kind == print_entry_values_no
	      || arg->entry_kind == print_entry_values_only
	      || (!ui_out_is_mi_like_p (uiout)
		  && arg->entry_kind == print_entry_values_compact));

  annotate_arg_begin ();

  make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
  fprintf_symbol_filtered (stb, SYMBOL_PRINT_NAME (arg->sym),
			   SYMBOL_LANGUAGE (arg->sym), DMGL_PARAMS | DMGL_ANSI);
  if (arg->entry_kind == print_entry_values_compact)
    {
      /* It is OK to provide invalid MI-like stream as with
	 PRINT_ENTRY_VALUE_COMPACT we never use MI.  */
      fputs_filtered ("=", stb);

      fprintf_symbol_filtered (stb, SYMBOL_PRINT_NAME (arg->sym),
			       SYMBOL_LANGUAGE (arg->sym),
			       DMGL_PARAMS | DMGL_ANSI);
    }
  if (arg->entry_kind == print_entry_values_only
      || arg->entry_kind == print_entry_values_compact)
    fputs_filtered ("@entry", stb);
  ui_out_field_stream (uiout, "name", stb);
  annotate_arg_name_end ();
  ui_out_text (uiout, "=");

  if (!arg->val && !arg->error)
    ui_out_text (uiout, "...");
  else
    {
      if (arg->error)
	except.message = arg->error;
      else
	{
	  /* TRY_CATCH has two statements, wrap it in a block.  */

	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      const struct language_defn *language;
	      struct value_print_options opts;

	      /* Avoid value_print because it will deref ref parameters.  We
		 just want to print their addresses.  Print ??? for args whose
		 address we do not know.  We pass 2 as "recurse" to val_print
		 because our standard indentation here is 4 spaces, and
		 val_print indents 2 for each recurse.  */ 

	      annotate_arg_value (value_type (arg->val));

	      /* Use the appropriate language to display our symbol, unless the
		 user forced the language to a specific language.  */
	      if (language_mode == language_mode_auto)
		language = language_def (SYMBOL_LANGUAGE (arg->sym));
	      else
		language = current_language;

	      get_no_prettyformat_print_options (&opts);
	      opts.deref_ref = 1;
	      opts.raw = print_raw_frame_arguments;

	      /* True in "summary" mode, false otherwise.  */
	      opts.summary = !strcmp (print_frame_arguments, "scalars");

	      common_val_print (arg->val, stb, 2, &opts, language);
	    }
	}
      if (except.message)
	fprintf_filtered (stb, _("<error reading variable: %s>"),
			  except.message);
    }

  ui_out_field_stream (uiout, "value", stb);

  /* Also invoke ui_out_tuple_end.  */
  do_cleanups (old_chain);

  annotate_arg_end ();
}

/* Read in inferior function local SYM at FRAME into ARGP.  Caller is
   responsible for xfree of ARGP->ERROR.  This function never throws an
   exception.  */

void
read_frame_local (struct symbol *sym, struct frame_info *frame,
		  struct frame_arg *argp)
{
  volatile struct gdb_exception except;
  struct value *val = NULL;

  TRY_CATCH (except, RETURN_MASK_ERROR)
    {
      val = read_var_value (sym, frame);
    }

  argp->error = (val == NULL) ? xstrdup (except.message) : NULL;
  argp->sym = sym;
  argp->val = val;
}

/* Read in inferior function parameter SYM at FRAME into ARGP.  Caller is
   responsible for xfree of ARGP->ERROR.  This function never throws an
   exception.  */

void
read_frame_arg (struct symbol *sym, struct frame_info *frame,
	        struct frame_arg *argp, struct frame_arg *entryargp)
{
  struct value *val = NULL, *entryval = NULL;
  char *val_error = NULL, *entryval_error = NULL;
  int val_equal = 0;
  volatile struct gdb_exception except;

  if (print_entry_values != print_entry_values_only
      && print_entry_values != print_entry_values_preferred)
    {
      TRY_CATCH (except, RETURN_MASK_ERROR)
	{
	  val = read_var_value (sym, frame);
	}
      if (!val)
	{
	  val_error = alloca (strlen (except.message) + 1);
	  strcpy (val_error, except.message);
	}
    }

  if (SYMBOL_COMPUTED_OPS (sym) != NULL
      && SYMBOL_COMPUTED_OPS (sym)->read_variable_at_entry != NULL
      && print_entry_values != print_entry_values_no
      && (print_entry_values != print_entry_values_if_needed
	  || !val || value_optimized_out (val)))
    {
      TRY_CATCH (except, RETURN_MASK_ERROR)
	{
	  const struct symbol_computed_ops *ops;

	  ops = SYMBOL_COMPUTED_OPS (sym);
	  entryval = ops->read_variable_at_entry (sym, frame);
	}
      if (!entryval)
	{
	  entryval_error = alloca (strlen (except.message) + 1);
	  strcpy (entryval_error, except.message);
	}

      if (except.error == NO_ENTRY_VALUE_ERROR
	  || (entryval && value_optimized_out (entryval)))
	{
	  entryval = NULL;
	  entryval_error = NULL;
	}

      if (print_entry_values == print_entry_values_compact
	  || print_entry_values == print_entry_values_default)
	{
	  /* For MI do not try to use print_entry_values_compact for ARGP.  */

	  if (val && entryval && !ui_out_is_mi_like_p (current_uiout))
	    {
	      struct type *type = value_type (val);

	      if (!value_optimized_out (val)
		  && value_available_contents_eq (val, 0, entryval, 0,
						  TYPE_LENGTH (type)))
		{
		  /* Initialize it just to avoid a GCC false warning.  */
		  struct value *val_deref = NULL, *entryval_deref;

		  /* DW_AT_GNU_call_site_value does match with the current
		     value.  If it is a reference still try to verify if
		     dereferenced DW_AT_GNU_call_site_data_value does not
		     differ.  */

		  TRY_CATCH (except, RETURN_MASK_ERROR)
		    {
		      struct type *type_deref;

		      val_deref = coerce_ref (val);
		      if (value_lazy (val_deref))
			value_fetch_lazy (val_deref);
		      type_deref = value_type (val_deref);

		      entryval_deref = coerce_ref (entryval);
		      if (value_lazy (entryval_deref))
			value_fetch_lazy (entryval_deref);

		      /* If the reference addresses match but dereferenced
			 content does not match print them.  */
		      if (val != val_deref
			  && value_available_contents_eq (val_deref, 0,
							  entryval_deref, 0,
						      TYPE_LENGTH (type_deref)))
			val_equal = 1;
		    }

		  /* Value was not a reference; and its content matches.  */
		  if (val == val_deref)
		    val_equal = 1;
		  /* If the dereferenced content could not be fetched do not
		     display anything.  */
		  else if (except.error == NO_ENTRY_VALUE_ERROR)
		    val_equal = 1;
		  else if (except.message)
		    {
		      entryval_error = alloca (strlen (except.message) + 1);
		      strcpy (entryval_error, except.message);
		    }

		  if (val_equal)
		    entryval = NULL;
		}
	    }

	  /* Try to remove possibly duplicate error message for ENTRYARGP even
	     in MI mode.  */

	  if (val_error && entryval_error
	      && strcmp (val_error, entryval_error) == 0)
	    {
	      entryval_error = NULL;

	      /* Do not se VAL_EQUAL as the same error message may be shown for
		 the entry value even if no entry values are present in the
		 inferior.  */
	    }
	}
    }

  if (entryval == NULL)
    {
      if (print_entry_values == print_entry_values_preferred)
	{
	  TRY_CATCH (except, RETURN_MASK_ERROR)
	    {
	      val = read_var_value (sym, frame);
	    }
	  if (!val)
	    {
	      val_error = alloca (strlen (except.message) + 1);
	      strcpy (val_error, except.message);
	    }
	}
      if (print_entry_values == print_entry_values_only
	  || print_entry_values == print_entry_values_both
	  || (print_entry_values == print_entry_values_preferred
	      && (!val || value_optimized_out (val))))
	{
	  entryval = allocate_optimized_out_value (SYMBOL_TYPE (sym));
	  entryval_error = NULL;
	}
    }
  if ((print_entry_values == print_entry_values_compact
       || print_entry_values == print_entry_values_if_needed
       || print_entry_values == print_entry_values_preferred)
      && (!val || value_optimized_out (val)) && entryval != NULL)
    {
      val = NULL;
      val_error = NULL;
    }

  argp->sym = sym;
  argp->val = val;
  argp->error = val_error ? xstrdup (val_error) : NULL;
  if (!val && !val_error)
    argp->entry_kind = print_entry_values_only;
  else if ((print_entry_values == print_entry_values_compact
	   || print_entry_values == print_entry_values_default) && val_equal)
    {
      argp->entry_kind = print_entry_values_compact;
      gdb_assert (!ui_out_is_mi_like_p (current_uiout));
    }
  else
    argp->entry_kind = print_entry_values_no;

  entryargp->sym = sym;
  entryargp->val = entryval;
  entryargp->error = entryval_error ? xstrdup (entryval_error) : NULL;
  if (!entryval && !entryval_error)
    entryargp->entry_kind = print_entry_values_no;
  else
    entryargp->entry_kind = print_entry_values_only;
}

/* Print the arguments of frame FRAME on STREAM, given the function
   FUNC running in that frame (as a symbol), where NUM is the number
   of arguments according to the stack frame (or -1 if the number of
   arguments is unknown).  */

/* Note that currently the "number of arguments according to the
   stack frame" is only known on VAX where i refers to the "number of
   ints of arguments according to the stack frame".  */

static void
print_frame_args (struct symbol *func, struct frame_info *frame,
		  int num, struct ui_file *stream)
{
  struct ui_out *uiout = current_uiout;
  int first = 1;
  /* Offset of next stack argument beyond the one we have seen that is
     at the highest offset, or -1 if we haven't come to a stack
     argument yet.  */
  long highest_offset = -1;
  /* Number of ints of arguments that we have printed so far.  */
  int args_printed = 0;
  struct cleanup *old_chain;
  struct ui_file *stb;
  /* True if we should print arguments, false otherwise.  */
  int print_args = strcmp (print_frame_arguments, "none");

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  if (func)
    {
      struct block *b = SYMBOL_BLOCK_VALUE (func);
      struct block_iterator iter;
      struct symbol *sym;

      ALL_BLOCK_SYMBOLS (b, iter, sym)
        {
	  struct frame_arg arg, entryarg;

	  QUIT;

	  /* Keep track of the highest stack argument offset seen, and
	     skip over any kinds of symbols we don't care about.  */

	  if (!SYMBOL_IS_ARGUMENT (sym))
	    continue;

	  switch (SYMBOL_CLASS (sym))
	    {
	    case LOC_ARG:
	    case LOC_REF_ARG:
	      {
		long current_offset = SYMBOL_VALUE (sym);
		int arg_size = TYPE_LENGTH (SYMBOL_TYPE (sym));

		/* Compute address of next argument by adding the size of
		   this argument and rounding to an int boundary.  */
		current_offset =
		  ((current_offset + arg_size + sizeof (int) - 1)
		   & ~(sizeof (int) - 1));

		/* If this is the highest offset seen yet, set
		   highest_offset.  */
		if (highest_offset == -1
		    || (current_offset > highest_offset))
		  highest_offset = current_offset;

		/* Add the number of ints we're about to print to
		   args_printed.  */
		args_printed += (arg_size + sizeof (int) - 1) / sizeof (int);
	      }

	      /* We care about types of symbols, but don't need to
		 keep track of stack offsets in them.  */
	    case LOC_REGISTER:
	    case LOC_REGPARM_ADDR:
	    case LOC_COMPUTED:
	    case LOC_OPTIMIZED_OUT:
	    default:
	      break;
	    }

	  /* We have to look up the symbol because arguments can have
	     two entries (one a parameter, one a local) and the one we
	     want is the local, which lookup_symbol will find for us.
	     This includes gcc1 (not gcc2) on SPARC when passing a
	     small structure and gcc2 when the argument type is float
	     and it is passed as a double and converted to float by
	     the prologue (in the latter case the type of the LOC_ARG
	     symbol is double and the type of the LOC_LOCAL symbol is
	     float).  */
	  /* But if the parameter name is null, don't try it.  Null
	     parameter names occur on the RS/6000, for traceback
	     tables.  FIXME, should we even print them?  */

	  if (*SYMBOL_LINKAGE_NAME (sym))
	    {
	      struct symbol *nsym;

	      nsym = lookup_symbol (SYMBOL_LINKAGE_NAME (sym),
				    b, VAR_DOMAIN, NULL);
	      gdb_assert (nsym != NULL);
	      if (SYMBOL_CLASS (nsym) == LOC_REGISTER
		  && !SYMBOL_IS_ARGUMENT (nsym))
		{
		  /* There is a LOC_ARG/LOC_REGISTER pair.  This means
		     that it was passed on the stack and loaded into a
		     register, or passed in a register and stored in a
		     stack slot.  GDB 3.x used the LOC_ARG; GDB
		     4.0-4.11 used the LOC_REGISTER.

		     Reasons for using the LOC_ARG:

		     (1) Because find_saved_registers may be slow for
		         remote debugging.

		     (2) Because registers are often re-used and stack
		         slots rarely (never?) are.  Therefore using
		         the stack slot is much less likely to print
		         garbage.

		     Reasons why we might want to use the LOC_REGISTER:

		     (1) So that the backtrace prints the same value
		         as "print foo".  I see no compelling reason
		         why this needs to be the case; having the
		         backtrace print the value which was passed
		         in, and "print foo" print the value as
		         modified within the called function, makes
		         perfect sense to me.

		     Additional note: It might be nice if "info args"
		     displayed both values.

		     One more note: There is a case with SPARC
		     structure passing where we need to use the
		     LOC_REGISTER, but this is dealt with by creating
		     a single LOC_REGPARM in symbol reading.  */

		  /* Leave sym (the LOC_ARG) alone.  */
		  ;
		}
	      else
		sym = nsym;
	    }

	  /* Print the current arg.  */
	  if (!first)
	    ui_out_text (uiout, ", ");
	  ui_out_wrap_hint (uiout, "    ");

	  if (!print_args)
	    {
	      memset (&arg, 0, sizeof (arg));
	      arg.sym = sym;
	      arg.entry_kind = print_entry_values_no;
	      memset (&entryarg, 0, sizeof (entryarg));
	      entryarg.sym = sym;
	      entryarg.entry_kind = print_entry_values_no;
	    }
	  else
	    read_frame_arg (sym, frame, &arg, &entryarg);

	  if (arg.entry_kind != print_entry_values_only)
	    print_frame_arg (&arg);

	  if (entryarg.entry_kind != print_entry_values_no)
	    {
	      if (arg.entry_kind != print_entry_values_only)
		{
		  ui_out_text (uiout, ", ");
		  ui_out_wrap_hint (uiout, "    ");
		}

	      print_frame_arg (&entryarg);
	    }

	  xfree (arg.error);
	  xfree (entryarg.error);

	  first = 0;
	}
    }

  /* Don't print nameless args in situations where we don't know
     enough about the stack to find them.  */
  if (num != -1)
    {
      long start;

      if (highest_offset == -1)
	start = gdbarch_frame_args_skip (get_frame_arch (frame));
      else
	start = highest_offset;

      print_frame_nameless_args (frame, start, num - args_printed,
				 first, stream);
    }

  do_cleanups (old_chain);
}

/* Set the current source and line to the location given by frame
   FRAME, if possible.  When CENTER is true, adjust so the relevant
   line is in the center of the next 'list'.  */

void
set_current_sal_from_frame (struct frame_info *frame, int center)
{
  struct symtab_and_line sal;

  find_frame_sal (frame, &sal);
  if (sal.symtab)
    {
      if (center)
        sal.line = max (sal.line - get_lines_to_list () / 2, 1);
      set_current_source_symtab_and_line (&sal);
    }
}

/* If ON, GDB will display disassembly of the next source line when
   execution of the program being debugged stops.
   If AUTO (which is the default), or there's no line info to determine
   the source line of the next instruction, display disassembly of next
   instruction instead.  */

static enum auto_boolean disassemble_next_line;

static void
show_disassemble_next_line (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c,
				 const char *value)
{
  fprintf_filtered (file,
		    _("Debugger's willingness to use "
		      "disassemble-next-line is %s.\n"),
                    value);
}

/* Use TRY_CATCH to catch the exception from the gdb_disassembly
   because it will be broken by filter sometime.  */

static void
do_gdb_disassembly (struct gdbarch *gdbarch,
		    int how_many, CORE_ADDR low, CORE_ADDR high)
{
  volatile struct gdb_exception exception;

  TRY_CATCH (exception, RETURN_MASK_ERROR)
    {
      gdb_disassembly (gdbarch, current_uiout, 0,
		       DISASSEMBLY_RAW_INSN, how_many,
		       low, high);
    }
  if (exception.reason < 0)
    {
      /* If an exception was thrown while doing the disassembly, print
	 the error message, to give the user a clue of what happened.  */
      exception_print (gdb_stderr, exception);
    }
}

/* Print information about frame FRAME.  The output is format according
   to PRINT_LEVEL and PRINT_WHAT and PRINT_ARGS.  The meaning of
   PRINT_WHAT is:
   
   SRC_LINE: Print only source line.
   LOCATION: Print only location.
   LOC_AND_SRC: Print location and source line.

   Used in "where" output, and to emit breakpoint or step
   messages.  */

void
print_frame_info (struct frame_info *frame, int print_level,
		  enum print_what print_what, int print_args,
		  int set_current_sal)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct symtab_and_line sal;
  int source_print;
  int location_print;
  struct ui_out *uiout = current_uiout;

  if (get_frame_type (frame) == DUMMY_FRAME
      || get_frame_type (frame) == SIGTRAMP_FRAME
      || get_frame_type (frame) == ARCH_FRAME)
    {
      struct cleanup *uiout_cleanup
	= make_cleanup_ui_out_tuple_begin_end (uiout, "frame");

      annotate_frame_begin (print_level ? frame_relative_level (frame) : 0,
			    gdbarch, get_frame_pc (frame));

      /* Do this regardless of SOURCE because we don't have any source
         to list for this frame.  */
      if (print_level)
        {
          ui_out_text (uiout, "#");
          ui_out_field_fmt_int (uiout, 2, ui_left, "level",
				frame_relative_level (frame));
        }
      if (ui_out_is_mi_like_p (uiout))
        {
          annotate_frame_address ();
          ui_out_field_core_addr (uiout, "addr",
				  gdbarch, get_frame_pc (frame));
          annotate_frame_address_end ();
        }

      if (get_frame_type (frame) == DUMMY_FRAME)
        {
          annotate_function_call ();
          ui_out_field_string (uiout, "func", "<function called from gdb>");
	}
      else if (get_frame_type (frame) == SIGTRAMP_FRAME)
        {
	  annotate_signal_handler_caller ();
          ui_out_field_string (uiout, "func", "<signal handler called>");
        }
      else if (get_frame_type (frame) == ARCH_FRAME)
        {
          ui_out_field_string (uiout, "func", "<cross-architecture call>");
	}
      ui_out_text (uiout, "\n");
      annotate_frame_end ();

      do_cleanups (uiout_cleanup);
      return;
    }

  /* If FRAME is not the innermost frame, that normally means that
     FRAME->pc points to *after* the call instruction, and we want to
     get the line containing the call, never the next line.  But if
     the next frame is a SIGTRAMP_FRAME or a DUMMY_FRAME, then the
     next frame was not entered as the result of a call, and we want
     to get the line containing FRAME->pc.  */
  find_frame_sal (frame, &sal);

  location_print = (print_what == LOCATION 
		    || print_what == LOC_AND_ADDRESS
		    || print_what == SRC_AND_LOC);

  if (location_print || !sal.symtab)
    print_frame (frame, print_level, print_what, print_args, sal);

  source_print = (print_what == SRC_LINE || print_what == SRC_AND_LOC);

  /* If disassemble-next-line is set to auto or on and doesn't have
     the line debug messages for $pc, output the next instruction.  */
  if ((disassemble_next_line == AUTO_BOOLEAN_AUTO
       || disassemble_next_line == AUTO_BOOLEAN_TRUE)
      && source_print && !sal.symtab)
    do_gdb_disassembly (get_frame_arch (frame), 1,
			get_frame_pc (frame), get_frame_pc (frame) + 1);

  if (source_print && sal.symtab)
    {
      int done = 0;
      int mid_statement = ((print_what == SRC_LINE)
			   && frame_show_address (frame, sal));

      if (annotation_level)
	done = identify_source_line (sal.symtab, sal.line, mid_statement,
				     get_frame_pc (frame));
      if (!done)
	{
	  if (deprecated_print_frame_info_listing_hook)
	    deprecated_print_frame_info_listing_hook (sal.symtab, 
						      sal.line, 
						      sal.line + 1, 0);
	  else
	    {
	      struct value_print_options opts;

	      get_user_print_options (&opts);
	      /* We used to do this earlier, but that is clearly
		 wrong.  This function is used by many different
		 parts of gdb, including normal_stop in infrun.c,
		 which uses this to print out the current PC
		 when we stepi/nexti into the middle of a source
		 line.  Only the command line really wants this
		 behavior.  Other UIs probably would like the
		 ability to decide for themselves if it is desired.  */
	      if (opts.addressprint && mid_statement)
		{
		  ui_out_field_core_addr (uiout, "addr",
					  gdbarch, get_frame_pc (frame));
		  ui_out_text (uiout, "\t");
		}

	      print_source_lines (sal.symtab, sal.line, sal.line + 1, 0);
	    }
	}

      /* If disassemble-next-line is set to on and there is line debug
         messages, output assembly codes for next line.  */
      if (disassemble_next_line == AUTO_BOOLEAN_TRUE)
	do_gdb_disassembly (get_frame_arch (frame), -1, sal.pc, sal.end);
    }

  if (set_current_sal)
    {
      CORE_ADDR pc;

      if (get_frame_pc_if_available (frame, &pc))
	set_last_displayed_sal (1, sal.pspace, pc, sal.symtab, sal.line);
      else
	set_last_displayed_sal (0, 0, 0, 0, 0);
    }

  annotate_frame_end ();

  gdb_flush (gdb_stdout);
}

/* Remember the last symtab and line we displayed, which we use e.g.
 * as the place to put a breakpoint when the `break' command is
 * invoked with no arguments.  */

static void
set_last_displayed_sal (int valid, struct program_space *pspace,
			CORE_ADDR addr, struct symtab *symtab,
			int line)
{
  last_displayed_sal_valid = valid;
  last_displayed_pspace = pspace;
  last_displayed_addr = addr;
  last_displayed_symtab = symtab;
  last_displayed_line = line;
  if (valid && pspace == NULL)
    {
      clear_last_displayed_sal ();
      internal_error (__FILE__, __LINE__,
		      _("Trying to set NULL pspace."));
    }
}

/* Forget the last sal we displayed.  */

void
clear_last_displayed_sal (void)
{
  last_displayed_sal_valid = 0;
  last_displayed_pspace = 0;
  last_displayed_addr = 0;
  last_displayed_symtab = 0;
  last_displayed_line = 0;
}

/* Is our record of the last sal we displayed valid?  If not,
 * the get_last_displayed_* functions will return NULL or 0, as
 * appropriate.  */

int
last_displayed_sal_is_valid (void)
{
  return last_displayed_sal_valid;
}

/* Get the pspace of the last sal we displayed, if it's valid.  */

struct program_space *
get_last_displayed_pspace (void)
{
  if (last_displayed_sal_valid)
    return last_displayed_pspace;
  return 0;
}

/* Get the address of the last sal we displayed, if it's valid.  */

CORE_ADDR
get_last_displayed_addr (void)
{
  if (last_displayed_sal_valid)
    return last_displayed_addr;
  return 0;
}

/* Get the symtab of the last sal we displayed, if it's valid.  */

struct symtab*
get_last_displayed_symtab (void)
{
  if (last_displayed_sal_valid)
    return last_displayed_symtab;
  return 0;
}

/* Get the line of the last sal we displayed, if it's valid.  */

int
get_last_displayed_line (void)
{
  if (last_displayed_sal_valid)
    return last_displayed_line;
  return 0;
}

/* Get the last sal we displayed, if it's valid.  */

void
get_last_displayed_sal (struct symtab_and_line *sal)
{
  if (last_displayed_sal_valid)
    {
      sal->pspace = last_displayed_pspace;
      sal->pc = last_displayed_addr;
      sal->symtab = last_displayed_symtab;
      sal->line = last_displayed_line;
    }
  else
    {
      sal->pspace = 0;
      sal->pc = 0;
      sal->symtab = 0;
      sal->line = 0;
    }
}


/* Attempt to obtain the FUNNAME, FUNLANG and optionally FUNCP of the function
   corresponding to FRAME.  FUNNAME needs to be freed by the caller.  */

void
find_frame_funname (struct frame_info *frame, char **funname,
		    enum language *funlang, struct symbol **funcp)
{
  struct symbol *func;

  *funname = NULL;
  *funlang = language_unknown;
  if (funcp)
    *funcp = NULL;

  func = get_frame_function (frame);
  if (func)
    {
      /* In certain pathological cases, the symtabs give the wrong
         function (when we are in the first function in a file which
         is compiled without debugging symbols, the previous function
         is compiled with debugging symbols, and the "foo.o" symbol
         that is supposed to tell us where the file with debugging
         symbols ends has been truncated by ar because it is longer
         than 15 characters).  This also occurs if the user uses asm()
         to create a function but not stabs for it (in a file compiled
         with -g).

         So look in the minimal symbol tables as well, and if it comes
         up with a larger address for the function use that instead.
         I don't think this can ever cause any problems; there
         shouldn't be any minimal symbols in the middle of a function;
         if this is ever changed many parts of GDB will need to be
         changed (and we'll create a find_pc_minimal_function or some
         such).  */

      struct bound_minimal_symbol msymbol;

      /* Don't attempt to do this for inlined functions, which do not
	 have a corresponding minimal symbol.  */
      if (!block_inlined_p (SYMBOL_BLOCK_VALUE (func)))
	msymbol
	  = lookup_minimal_symbol_by_pc (get_frame_address_in_block (frame));
      else
	memset (&msymbol, 0, sizeof (msymbol));

      if (msymbol.minsym != NULL
	  && (SYMBOL_VALUE_ADDRESS (msymbol.minsym)
	      > BLOCK_START (SYMBOL_BLOCK_VALUE (func))))
	{
	  /* We also don't know anything about the function besides
	     its address and name.  */
	  func = 0;
	  *funname = xstrdup (SYMBOL_PRINT_NAME (msymbol.minsym));
	  *funlang = SYMBOL_LANGUAGE (msymbol.minsym);
	}
      else
	{
	  *funname = xstrdup (SYMBOL_PRINT_NAME (func));
	  *funlang = SYMBOL_LANGUAGE (func);
	  if (funcp)
	    *funcp = func;
	  if (*funlang == language_cplus)
	    {
	      /* It seems appropriate to use SYMBOL_PRINT_NAME() here,
		 to display the demangled name that we already have
		 stored in the symbol table, but we stored a version
		 with DMGL_PARAMS turned on, and here we don't want to
		 display parameters.  So remove the parameters.  */
	      char *func_only = cp_remove_params (*funname);

	      if (func_only)
		{
		  xfree (*funname);
		  *funname = func_only;
		}
	    }
	}
    }
  else
    {
      struct bound_minimal_symbol msymbol;
      CORE_ADDR pc;

      if (!get_frame_address_in_block_if_available (frame, &pc))
	return;

      msymbol = lookup_minimal_symbol_by_pc (pc);
      if (msymbol.minsym != NULL)
	{
	  *funname = xstrdup (SYMBOL_PRINT_NAME (msymbol.minsym));
	  *funlang = SYMBOL_LANGUAGE (msymbol.minsym);
	}
    }
}

static void
print_frame (struct frame_info *frame, int print_level,
	     enum print_what print_what, int print_args,
	     struct symtab_and_line sal)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct ui_out *uiout = current_uiout;
  char *funname = NULL;
  enum language funlang = language_unknown;
  struct ui_file *stb;
  struct cleanup *old_chain, *list_chain;
  struct value_print_options opts;
  struct symbol *func;
  CORE_ADDR pc = 0;
  int pc_p;

  pc_p = get_frame_pc_if_available (frame, &pc);

  stb = mem_fileopen ();
  old_chain = make_cleanup_ui_file_delete (stb);

  find_frame_funname (frame, &funname, &funlang, &func);
  make_cleanup (xfree, funname);

  annotate_frame_begin (print_level ? frame_relative_level (frame) : 0,
			gdbarch, pc);

  list_chain = make_cleanup_ui_out_tuple_begin_end (uiout, "frame");

  if (print_level)
    {
      ui_out_text (uiout, "#");
      ui_out_field_fmt_int (uiout, 2, ui_left, "level",
			    frame_relative_level (frame));
    }
  get_user_print_options (&opts);
  if (opts.addressprint)
    if (!sal.symtab
	|| frame_show_address (frame, sal)
	|| print_what == LOC_AND_ADDRESS)
      {
	annotate_frame_address ();
	if (pc_p)
	  ui_out_field_core_addr (uiout, "addr", gdbarch, pc);
	else
	  ui_out_field_string (uiout, "addr", "<unavailable>");
	annotate_frame_address_end ();
	ui_out_text (uiout, " in ");
      }
  annotate_frame_function_name ();
  fprintf_symbol_filtered (stb, funname ? funname : "??",
			   funlang, DMGL_ANSI);
  ui_out_field_stream (uiout, "func", stb);
  ui_out_wrap_hint (uiout, "   ");
  annotate_frame_args ();
      
  ui_out_text (uiout, " (");
  if (print_args)
    {
      struct gdbarch *gdbarch = get_frame_arch (frame);
      int numargs;
      struct cleanup *args_list_chain;
      volatile struct gdb_exception e;

      if (gdbarch_frame_num_args_p (gdbarch))
	{
	  numargs = gdbarch_frame_num_args (gdbarch, frame);
	  gdb_assert (numargs >= 0);
	}
      else
	numargs = -1;
    
      args_list_chain = make_cleanup_ui_out_list_begin_end (uiout, "args");
      TRY_CATCH (e, RETURN_MASK_ERROR)
	{
	  print_frame_args (func, frame, numargs, gdb_stdout);
	}
      /* FIXME: ARGS must be a list.  If one argument is a string it
	  will have " that will not be properly escaped.  */
      /* Invoke ui_out_tuple_end.  */
      do_cleanups (args_list_chain);
      QUIT;
    }
  ui_out_text (uiout, ")");
  if (sal.symtab)
    {
      const char *filename_display;
      
      filename_display = symtab_to_filename_for_display (sal.symtab);
      annotate_frame_source_begin ();
      ui_out_wrap_hint (uiout, "   ");
      ui_out_text (uiout, " at ");
      annotate_frame_source_file ();
      ui_out_field_string (uiout, "file", filename_display);
      if (ui_out_is_mi_like_p (uiout))
	{
	  const char *fullname = symtab_to_fullname (sal.symtab);

	  ui_out_field_string (uiout, "fullname", fullname);
	}
      annotate_frame_source_file_end ();
      ui_out_text (uiout, ":");
      annotate_frame_source_line ();
      ui_out_field_int (uiout, "line", sal.line);
      annotate_frame_source_end ();
    }

  if (pc_p && (funname == NULL || sal.symtab == NULL))
    {
      char *lib = solib_name_from_address (get_frame_program_space (frame),
					   get_frame_pc (frame));

      if (lib)
	{
	  annotate_frame_where ();
	  ui_out_wrap_hint (uiout, "  ");
	  ui_out_text (uiout, " from ");
	  ui_out_field_string (uiout, "from", lib);
	}
    }

  /* do_cleanups will call ui_out_tuple_end() for us.  */
  do_cleanups (list_chain);
  ui_out_text (uiout, "\n");
  do_cleanups (old_chain);
}


/* Read a frame specification in whatever the appropriate format is
   from FRAME_EXP.  Call error(), printing MESSAGE, if the
   specification is in any way invalid (so this function never returns
   NULL).  When SEPECTED_P is non-NULL set its target to indicate that
   the default selected frame was used.  */

static struct frame_info *
parse_frame_specification_1 (const char *frame_exp, const char *message,
			     int *selected_frame_p)
{
  int numargs;
  struct value *args[4];
  CORE_ADDR addrs[ARRAY_SIZE (args)];

  if (frame_exp == NULL)
    numargs = 0;
  else
    {
      numargs = 0;
      while (1)
	{
	  char *addr_string;
	  struct cleanup *cleanup;
	  const char *p;

	  /* Skip leading white space, bail of EOL.  */
	  frame_exp = skip_spaces_const (frame_exp);
	  if (!*frame_exp)
	    break;

	  /* Parse the argument, extract it, save it.  */
	  for (p = frame_exp;
	       *p && !isspace (*p);
	       p++);
	  addr_string = savestring (frame_exp, p - frame_exp);
	  frame_exp = p;
	  cleanup = make_cleanup (xfree, addr_string);
	  
	  /* NOTE: Parse and evaluate expression, but do not use
	     functions such as parse_and_eval_long or
	     parse_and_eval_address to also extract the value.
	     Instead value_as_long and value_as_address are used.
	     This avoids problems with expressions that contain
	     side-effects.  */
	  if (numargs >= ARRAY_SIZE (args))
	    error (_("Too many args in frame specification"));
	  args[numargs++] = parse_and_eval (addr_string);

	  do_cleanups (cleanup);
	}
    }

  /* If no args, default to the selected frame.  */
  if (numargs == 0)
    {
      if (selected_frame_p != NULL)
	(*selected_frame_p) = 1;
      return get_selected_frame (message);
    }

  /* None of the remaining use the selected frame.  */
  if (selected_frame_p != NULL)
    (*selected_frame_p) = 0;

  /* Assume the single arg[0] is an integer, and try using that to
     select a frame relative to current.  */
  if (numargs == 1)
    {
      struct frame_info *fid;
      int level = value_as_long (args[0]);

      fid = find_relative_frame (get_current_frame (), &level);
      if (level == 0)
	/* find_relative_frame was successful.  */
	return fid;
    }

  /* Convert each value into a corresponding address.  */
  {
    int i;

    for (i = 0; i < numargs; i++)
      addrs[i] = value_as_address (args[i]);
  }

  /* Assume that the single arg[0] is an address, use that to identify
     a frame with a matching ID.  Should this also accept stack/pc or
     stack/pc/special.  */
  if (numargs == 1)
    {
      struct frame_id id = frame_id_build_wild (addrs[0]);
      struct frame_info *fid;

      /* If (s)he specifies the frame with an address, he deserves
	 what (s)he gets.  Still, give the highest one that matches.
	 (NOTE: cagney/2004-10-29: Why highest, or outer-most, I don't
	 know).  */
      for (fid = get_current_frame ();
	   fid != NULL;
	   fid = get_prev_frame (fid))
	{
	  if (frame_id_eq (id, get_frame_id (fid)))
	    {
	      struct frame_info *prev_frame;

	      while (1)
		{
		  prev_frame = get_prev_frame (fid);
		  if (!prev_frame
		      || !frame_id_eq (id, get_frame_id (prev_frame)))
		    break;
		  fid = prev_frame;
		}
	      return fid;
	    }
	}
      }

  /* We couldn't identify the frame as an existing frame, but
     perhaps we can create one with a single argument.  */
  if (numargs == 1)
    return create_new_frame (addrs[0], 0);
  else if (numargs == 2)
    return create_new_frame (addrs[0], addrs[1]);
  else
    error (_("Too many args in frame specification"));
}

static struct frame_info *
parse_frame_specification (char *frame_exp)
{
  return parse_frame_specification_1 (frame_exp, NULL, NULL);
}

/* Print verbosely the selected frame or the frame at address
   ADDR_EXP.  Absolutely all information in the frame is printed.  */

static void
frame_info (char *addr_exp, int from_tty)
{
  struct frame_info *fi;
  struct symtab_and_line sal;
  struct symbol *func;
  struct symtab *s;
  struct frame_info *calling_frame_info;
  int numregs;
  const char *funname = 0;
  enum language funlang = language_unknown;
  const char *pc_regname;
  int selected_frame_p;
  struct gdbarch *gdbarch;
  struct cleanup *back_to = make_cleanup (null_cleanup, NULL);
  CORE_ADDR frame_pc;
  int frame_pc_p;
  /* Initialize it to avoid "may be used uninitialized" warning.  */
  CORE_ADDR caller_pc = 0;
  volatile struct gdb_exception ex;

  fi = parse_frame_specification_1 (addr_exp, "No stack.", &selected_frame_p);
  gdbarch = get_frame_arch (fi);

  /* Name of the value returned by get_frame_pc().  Per comments, "pc"
     is not a good name.  */
  if (gdbarch_pc_regnum (gdbarch) >= 0)
    /* OK, this is weird.  The gdbarch_pc_regnum hardware register's value can
       easily not match that of the internal value returned by
       get_frame_pc().  */
    pc_regname = gdbarch_register_name (gdbarch, gdbarch_pc_regnum (gdbarch));
  else
    /* But then, this is weird to.  Even without gdbarch_pc_regnum, an
       architectures will often have a hardware register called "pc",
       and that register's value, again, can easily not match
       get_frame_pc().  */
    pc_regname = "pc";

  frame_pc_p = get_frame_pc_if_available (fi, &frame_pc);
  find_frame_sal (fi, &sal);
  func = get_frame_function (fi);
  s = sal.symtab;
  if (func)
    {
      funname = SYMBOL_PRINT_NAME (func);
      funlang = SYMBOL_LANGUAGE (func);
      if (funlang == language_cplus)
	{
	  /* It seems appropriate to use SYMBOL_PRINT_NAME() here,
	     to display the demangled name that we already have
	     stored in the symbol table, but we stored a version
	     with DMGL_PARAMS turned on, and here we don't want to
	     display parameters.  So remove the parameters.  */
	  char *func_only = cp_remove_params (funname);

	  if (func_only)
	    {
	      funname = func_only;
	      make_cleanup (xfree, func_only);
	    }
	}
    }
  else if (frame_pc_p)
    {
      struct bound_minimal_symbol msymbol;

      msymbol = lookup_minimal_symbol_by_pc (frame_pc);
      if (msymbol.minsym != NULL)
	{
	  funname = SYMBOL_PRINT_NAME (msymbol.minsym);
	  funlang = SYMBOL_LANGUAGE (msymbol.minsym);
	}
    }
  calling_frame_info = get_prev_frame (fi);

  if (selected_frame_p && frame_relative_level (fi) >= 0)
    {
      printf_filtered (_("Stack level %d, frame at "),
		       frame_relative_level (fi));
    }
  else
    {
      printf_filtered (_("Stack frame at "));
    }
  fputs_filtered (paddress (gdbarch, get_frame_base (fi)), gdb_stdout);
  printf_filtered (":\n");
  printf_filtered (" %s = ", pc_regname);
  if (frame_pc_p)
    fputs_filtered (paddress (gdbarch, get_frame_pc (fi)), gdb_stdout);
  else
    fputs_filtered ("<unavailable>", gdb_stdout);

  wrap_here ("   ");
  if (funname)
    {
      printf_filtered (" in ");
      fprintf_symbol_filtered (gdb_stdout, funname, funlang,
			       DMGL_ANSI | DMGL_PARAMS);
    }
  wrap_here ("   ");
  if (sal.symtab)
    printf_filtered (" (%s:%d)", symtab_to_filename_for_display (sal.symtab),
		     sal.line);
  puts_filtered ("; ");
  wrap_here ("    ");
  printf_filtered ("saved %s = ", pc_regname);

  TRY_CATCH (ex, RETURN_MASK_ERROR)
    {
      caller_pc = frame_unwind_caller_pc (fi);
    }
  if (ex.reason < 0)
    {
      switch (ex.error)
	{
	case NOT_AVAILABLE_ERROR:
	  val_print_unavailable (gdb_stdout);
	  break;
	case OPTIMIZED_OUT_ERROR:
	  val_print_not_saved (gdb_stdout);
	  break;
	default:
	  fprintf_filtered (gdb_stdout, _("<error: %s>"), ex.message);
	  break;
	}
    }
  else
    fputs_filtered (paddress (gdbarch, caller_pc), gdb_stdout);
  printf_filtered ("\n");

  if (calling_frame_info == NULL)
    {
      enum unwind_stop_reason reason;

      reason = get_frame_unwind_stop_reason (fi);
      if (reason != UNWIND_NO_REASON)
	printf_filtered (_(" Outermost frame: %s\n"),
			 frame_stop_reason_string (reason));
    }
  else if (get_frame_type (fi) == TAILCALL_FRAME)
    puts_filtered (" tail call frame");
  else if (get_frame_type (fi) == INLINE_FRAME)
    printf_filtered (" inlined into frame %d",
		     frame_relative_level (get_prev_frame (fi)));
  else
    {
      printf_filtered (" called by frame at ");
      fputs_filtered (paddress (gdbarch, get_frame_base (calling_frame_info)),
		      gdb_stdout);
    }
  if (get_next_frame (fi) && calling_frame_info)
    puts_filtered (",");
  wrap_here ("   ");
  if (get_next_frame (fi))
    {
      printf_filtered (" caller of frame at ");
      fputs_filtered (paddress (gdbarch, get_frame_base (get_next_frame (fi))),
		      gdb_stdout);
    }
  if (get_next_frame (fi) || calling_frame_info)
    puts_filtered ("\n");

  if (s)
    printf_filtered (" source language %s.\n",
		     language_str (s->language));

  {
    /* Address of the argument list for this frame, or 0.  */
    CORE_ADDR arg_list = get_frame_args_address (fi);
    /* Number of args for this frame, or -1 if unknown.  */
    int numargs;

    if (arg_list == 0)
      printf_filtered (" Arglist at unknown address.\n");
    else
      {
	printf_filtered (" Arglist at ");
	fputs_filtered (paddress (gdbarch, arg_list), gdb_stdout);
	printf_filtered (",");

	if (!gdbarch_frame_num_args_p (gdbarch))
	  {
	    numargs = -1;
	    puts_filtered (" args: ");
	  }
	else
	  {
	    numargs = gdbarch_frame_num_args (gdbarch, fi);
	    gdb_assert (numargs >= 0);
	    if (numargs == 0)
	      puts_filtered (" no args.");
	    else if (numargs == 1)
	      puts_filtered (" 1 arg: ");
	    else
	      printf_filtered (" %d args: ", numargs);
	  }
	print_frame_args (func, fi, numargs, gdb_stdout);
	puts_filtered ("\n");
      }
  }
  {
    /* Address of the local variables for this frame, or 0.  */
    CORE_ADDR arg_list = get_frame_locals_address (fi);

    if (arg_list == 0)
      printf_filtered (" Locals at unknown address,");
    else
      {
	printf_filtered (" Locals at ");
	fputs_filtered (paddress (gdbarch, arg_list), gdb_stdout);
	printf_filtered (",");
      }
  }

  /* Print as much information as possible on the location of all the
     registers.  */
  {
    enum lval_type lval;
    int optimized;
    int unavailable;
    CORE_ADDR addr;
    int realnum;
    int count;
    int i;
    int need_nl = 1;

    /* The sp is special; what's displayed isn't the save address, but
       the value of the previous frame's sp.  This is a legacy thing,
       at one stage the frame cached the previous frame's SP instead
       of its address, hence it was easiest to just display the cached
       value.  */
    if (gdbarch_sp_regnum (gdbarch) >= 0)
      {
	/* Find out the location of the saved stack pointer with out
           actually evaluating it.  */
	frame_register_unwind (fi, gdbarch_sp_regnum (gdbarch),
			       &optimized, &unavailable, &lval, &addr,
			       &realnum, NULL);
	if (!optimized && !unavailable && lval == not_lval)
	  {
	    enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
	    int sp_size = register_size (gdbarch, gdbarch_sp_regnum (gdbarch));
	    gdb_byte value[MAX_REGISTER_SIZE];
	    CORE_ADDR sp;

	    frame_register_unwind (fi, gdbarch_sp_regnum (gdbarch),
				   &optimized, &unavailable, &lval, &addr,
				   &realnum, value);
	    /* NOTE: cagney/2003-05-22: This is assuming that the
               stack pointer was packed as an unsigned integer.  That
               may or may not be valid.  */
	    sp = extract_unsigned_integer (value, sp_size, byte_order);
	    printf_filtered (" Previous frame's sp is ");
	    fputs_filtered (paddress (gdbarch, sp), gdb_stdout);
	    printf_filtered ("\n");
	    need_nl = 0;
	  }
	else if (!optimized && !unavailable && lval == lval_memory)
	  {
	    printf_filtered (" Previous frame's sp at ");
	    fputs_filtered (paddress (gdbarch, addr), gdb_stdout);
	    printf_filtered ("\n");
	    need_nl = 0;
	  }
	else if (!optimized && !unavailable && lval == lval_register)
	  {
	    printf_filtered (" Previous frame's sp in %s\n",
			     gdbarch_register_name (gdbarch, realnum));
	    need_nl = 0;
	  }
	/* else keep quiet.  */
      }

    count = 0;
    numregs = gdbarch_num_regs (gdbarch)
	      + gdbarch_num_pseudo_regs (gdbarch);
    for (i = 0; i < numregs; i++)
      if (i != gdbarch_sp_regnum (gdbarch)
	  && gdbarch_register_reggroup_p (gdbarch, i, all_reggroup))
	{
	  /* Find out the location of the saved register without
             fetching the corresponding value.  */
	  frame_register_unwind (fi, i, &optimized, &unavailable,
				 &lval, &addr, &realnum, NULL);
	  /* For moment, only display registers that were saved on the
	     stack.  */
	  if (!optimized && !unavailable && lval == lval_memory)
	    {
	      if (count == 0)
		puts_filtered (" Saved registers:\n ");
	      else
		puts_filtered (",");
	      wrap_here (" ");
	      printf_filtered (" %s at ",
			       gdbarch_register_name (gdbarch, i));
	      fputs_filtered (paddress (gdbarch, addr), gdb_stdout);
	      count++;
	    }
	}
    if (count || need_nl)
      puts_filtered ("\n");
  }

  do_cleanups (back_to);
}

/* Print briefly all stack frames or just the innermost COUNT_EXP
   frames.  */

static void
backtrace_command_1 (char *count_exp, int show_locals, int no_filters,
		     int from_tty)
{
  struct frame_info *fi;
  int count;
  int i;
  struct frame_info *trailing;
  int trailing_level, py_start = 0, py_end = 0;
  enum py_bt_status result = PY_BT_ERROR;

  if (!target_has_stack)
    error (_("No stack."));

  /* The following code must do two things.  First, it must set the
     variable TRAILING to the frame from which we should start
     printing.  Second, it must set the variable count to the number
     of frames which we should print, or -1 if all of them.  */
  trailing = get_current_frame ();

  trailing_level = 0;
  if (count_exp)
    {
      count = parse_and_eval_long (count_exp);
      if (count < 0)
	{
	  struct frame_info *current;

	  py_start = count;
	  count = -count;

	  current = trailing;
	  while (current && count--)
	    {
	      QUIT;
	      current = get_prev_frame (current);
	    }

	  /* Will stop when CURRENT reaches the top of the stack.
	     TRAILING will be COUNT below it.  */
	  while (current)
	    {
	      QUIT;
	      trailing = get_prev_frame (trailing);
	      current = get_prev_frame (current);
	      trailing_level++;
	    }

	  count = -1;
	}
      else
	{
	  py_start = 0;
	  py_end = count;
	}
    }
  else
    {
      py_end = -1;
      count = -1;
    }

  if (info_verbose)
    {
      /* Read in symbols for all of the frames.  Need to do this in a
         separate pass so that "Reading in symbols for xxx" messages
         don't screw up the appearance of the backtrace.  Also if
         people have strong opinions against reading symbols for
         backtrace this may have to be an option.  */
      i = count;
      for (fi = trailing; fi != NULL && i--; fi = get_prev_frame (fi))
	{
	  CORE_ADDR pc;

	  QUIT;
	  pc = get_frame_address_in_block (fi);
	  find_pc_sect_symtab_via_partial (pc, find_pc_mapped_section (pc));
	}
    }

  if (! no_filters)
    {
      int flags = PRINT_LEVEL | PRINT_FRAME_INFO | PRINT_ARGS;
      enum py_frame_args arg_type;

      if (show_locals)
	flags |= PRINT_LOCALS;

      if (!strcmp (print_frame_arguments, "scalars"))
	arg_type = CLI_SCALAR_VALUES;
      else if (!strcmp (print_frame_arguments, "all"))
	arg_type = CLI_ALL_VALUES;
      else
	arg_type = NO_VALUES;

      result = apply_frame_filter (get_current_frame (), flags, arg_type,
				   current_uiout, py_start, py_end);

    }
  /* Run the inbuilt backtrace if there are no filters registered, or
     "no-filters" has been specified from the command.  */
  if (no_filters ||  result == PY_BT_NO_FILTERS)
    {
      for (i = 0, fi = trailing; fi && count--; i++, fi = get_prev_frame (fi))
	{
	  QUIT;

	  /* Don't use print_stack_frame; if an error() occurs it probably
	     means further attempts to backtrace would fail (on the other
	     hand, perhaps the code does or could be fixed to make sure
	     the frame->prev field gets set to NULL in that case).  */

	  print_frame_info (fi, 1, LOCATION, 1, 0);
	  if (show_locals)
	    {
	      struct frame_id frame_id = get_frame_id (fi);

	      print_frame_local_vars (fi, 1, gdb_stdout);

	      /* print_frame_local_vars invalidates FI.  */
	      fi = frame_find_by_id (frame_id);
	      if (fi == NULL)
		{
		  trailing = NULL;
		  warning (_("Unable to restore previously selected frame."));
		  break;
		}
	    }

	  /* Save the last frame to check for error conditions.  */
	  trailing = fi;
	}

      /* If we've stopped before the end, mention that.  */
      if (fi && from_tty)
	printf_filtered (_("(More stack frames follow...)\n"));

      /* If we've run out of frames, and the reason appears to be an error
	 condition, print it.  */
      if (fi == NULL && trailing != NULL)
	{
	  enum unwind_stop_reason reason;

	  reason = get_frame_unwind_stop_reason (trailing);
	  if (reason >= UNWIND_FIRST_ERROR)
	    printf_filtered (_("Backtrace stopped: %s\n"),
			     frame_stop_reason_string (reason));
	}
    }
}

static void
backtrace_command (char *arg, int from_tty)
{
  struct cleanup *old_chain = make_cleanup (null_cleanup, NULL);
  int fulltrace_arg = -1, arglen = 0, argc = 0, no_filters  = -1;
  int user_arg = 0;

  if (arg)
    {
      char **argv;
      int i;

      argv = gdb_buildargv (arg);
      make_cleanup_freeargv (argv);
      argc = 0;
      for (i = 0; argv[i]; i++)
	{
	  unsigned int j;

	  for (j = 0; j < strlen (argv[i]); j++)
	    argv[i][j] = tolower (argv[i][j]);

	  if (no_filters < 0 && subset_compare (argv[i], "no-filters"))
	    no_filters = argc;
	  else
	    {
	      if (fulltrace_arg < 0 && subset_compare (argv[i], "full"))
		fulltrace_arg = argc;
	      else
		{
		  user_arg++;
		  arglen += strlen (argv[i]);
		}
	    }
	  argc++;
	}
      arglen += user_arg;
      if (fulltrace_arg >= 0 || no_filters >= 0)
	{
	  if (arglen > 0)
	    {
	      arg = xmalloc (arglen + 1);
	      make_cleanup (xfree, arg);
	      arg[0] = 0;
	      for (i = 0; i < argc; i++)
		{
		  if (i != fulltrace_arg && i != no_filters)
		    {
		      strcat (arg, argv[i]);
		      strcat (arg, " ");
		    }
		}
	    }
	  else
	    arg = NULL;
	}
    }

  backtrace_command_1 (arg, fulltrace_arg >= 0 /* show_locals */,
		       no_filters >= 0 /* no frame-filters */, from_tty);

  do_cleanups (old_chain);
}

static void
backtrace_full_command (char *arg, int from_tty)
{
  backtrace_command_1 (arg, 1 /* show_locals */, 0, from_tty);
}


/* Iterate over the local variables of a block B, calling CB with
   CB_DATA.  */

static void
iterate_over_block_locals (struct block *b,
			   iterate_over_block_arg_local_vars_cb cb,
			   void *cb_data)
{
  struct block_iterator iter;
  struct symbol *sym;

  ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      switch (SYMBOL_CLASS (sym))
	{
	case LOC_LOCAL:
	case LOC_REGISTER:
	case LOC_STATIC:
	case LOC_COMPUTED:
	  if (SYMBOL_IS_ARGUMENT (sym))
	    break;
	  if (SYMBOL_DOMAIN (sym) == COMMON_BLOCK_DOMAIN)
	    break;
	  (*cb) (SYMBOL_PRINT_NAME (sym), sym, cb_data);
	  break;

	default:
	  /* Ignore symbols which are not locals.  */
	  break;
	}
    }
}


/* Same, but print labels.  */

#if 0
/* Commented out, as the code using this function has also been
   commented out.  FIXME:brobecker/2009-01-13: Find out why the code
   was commented out in the first place.  The discussion introducing
   this change (2007-12-04: Support lexical blocks and function bodies
   that occupy non-contiguous address ranges) did not explain why
   this change was made.  */
static int
print_block_frame_labels (struct gdbarch *gdbarch, struct block *b,
			  int *have_default, struct ui_file *stream)
{
  struct block_iterator iter;
  struct symbol *sym;
  int values_printed = 0;

  ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      if (strcmp (SYMBOL_LINKAGE_NAME (sym), "default") == 0)
	{
	  if (*have_default)
	    continue;
	  *have_default = 1;
	}
      if (SYMBOL_CLASS (sym) == LOC_LABEL)
	{
	  struct symtab_and_line sal;
	  struct value_print_options opts;

	  sal = find_pc_line (SYMBOL_VALUE_ADDRESS (sym), 0);
	  values_printed = 1;
	  fputs_filtered (SYMBOL_PRINT_NAME (sym), stream);
	  get_user_print_options (&opts);
	  if (opts.addressprint)
	    {
	      fprintf_filtered (stream, " ");
	      fputs_filtered (paddress (gdbarch, SYMBOL_VALUE_ADDRESS (sym)),
			      stream);
	    }
	  fprintf_filtered (stream, " in file %s, line %d\n",
			    sal.symtab->filename, sal.line);
	}
    }

  return values_printed;
}
#endif

/* Iterate over all the local variables in block B, including all its
   superblocks, stopping when the top-level block is reached.  */

void
iterate_over_block_local_vars (struct block *block,
			       iterate_over_block_arg_local_vars_cb cb,
			       void *cb_data)
{
  while (block)
    {
      iterate_over_block_locals (block, cb, cb_data);
      /* After handling the function's top-level block, stop.  Don't
	 continue to its superblock, the block of per-file
	 symbols.  */
      if (BLOCK_FUNCTION (block))
	break;
      block = BLOCK_SUPERBLOCK (block);
    }
}

/* Data to be passed around in the calls to the locals and args
   iterators.  */

struct print_variable_and_value_data
{
  struct frame_id frame_id;
  int num_tabs;
  struct ui_file *stream;
  int values_printed;
};

/* The callback for the locals and args iterators.  */

static void
do_print_variable_and_value (const char *print_name,
			     struct symbol *sym,
			     void *cb_data)
{
  struct print_variable_and_value_data *p = cb_data;
  struct frame_info *frame;

  frame = frame_find_by_id (p->frame_id);
  if (frame == NULL)
    {
      warning (_("Unable to restore previously selected frame."));
      return;
    }

  print_variable_and_value (print_name, sym, frame, p->stream, p->num_tabs);

  /* print_variable_and_value invalidates FRAME.  */
  frame = NULL;

  p->values_printed = 1;
}

/* Print all variables from the innermost up to the function block of FRAME.
   Print them with values to STREAM indented by NUM_TABS.

   This function will invalidate FRAME.  */

static void
print_frame_local_vars (struct frame_info *frame, int num_tabs,
			struct ui_file *stream)
{
  struct print_variable_and_value_data cb_data;
  struct block *block;
  CORE_ADDR pc;

  if (!get_frame_pc_if_available (frame, &pc))
    {
      fprintf_filtered (stream,
			_("PC unavailable, cannot determine locals.\n"));
      return;
    }

  block = get_frame_block (frame, 0);
  if (block == 0)
    {
      fprintf_filtered (stream, "No symbol table info available.\n");
      return;
    }

  cb_data.frame_id = get_frame_id (frame);
  cb_data.num_tabs = 4 * num_tabs;
  cb_data.stream = stream;
  cb_data.values_printed = 0;

  iterate_over_block_local_vars (block,
				 do_print_variable_and_value,
				 &cb_data);

  /* do_print_variable_and_value invalidates FRAME.  */
  frame = NULL;

  if (!cb_data.values_printed)
    fprintf_filtered (stream, _("No locals.\n"));
}

void
locals_info (char *args, int from_tty)
{
  print_frame_local_vars (get_selected_frame (_("No frame selected.")),
			  0, gdb_stdout);
}

/* Iterate over all the argument variables in block B.

   Returns 1 if any argument was walked; 0 otherwise.  */

void
iterate_over_block_arg_vars (struct block *b,
			     iterate_over_block_arg_local_vars_cb cb,
			     void *cb_data)
{
  struct block_iterator iter;
  struct symbol *sym, *sym2;

  ALL_BLOCK_SYMBOLS (b, iter, sym)
    {
      /* Don't worry about things which aren't arguments.  */
      if (SYMBOL_IS_ARGUMENT (sym))
	{
	  /* We have to look up the symbol because arguments can have
	     two entries (one a parameter, one a local) and the one we
	     want is the local, which lookup_symbol will find for us.
	     This includes gcc1 (not gcc2) on the sparc when passing a
	     small structure and gcc2 when the argument type is float
	     and it is passed as a double and converted to float by
	     the prologue (in the latter case the type of the LOC_ARG
	     symbol is double and the type of the LOC_LOCAL symbol is
	     float).  There are also LOC_ARG/LOC_REGISTER pairs which
	     are not combined in symbol-reading.  */

	  sym2 = lookup_symbol (SYMBOL_LINKAGE_NAME (sym),
				b, VAR_DOMAIN, NULL);
	  (*cb) (SYMBOL_PRINT_NAME (sym), sym2, cb_data);
	}
    }
}

/* Print all argument variables of the function of FRAME.
   Print them with values to STREAM.

   This function will invalidate FRAME.  */

static void
print_frame_arg_vars (struct frame_info *frame, struct ui_file *stream)
{
  struct print_variable_and_value_data cb_data;
  struct symbol *func;
  CORE_ADDR pc;

  if (!get_frame_pc_if_available (frame, &pc))
    {
      fprintf_filtered (stream, _("PC unavailable, cannot determine args.\n"));
      return;
    }

  func = get_frame_function (frame);
  if (func == NULL)
    {
      fprintf_filtered (stream, _("No symbol table info available.\n"));
      return;
    }

  cb_data.frame_id = get_frame_id (frame);
  cb_data.num_tabs = 0;
  cb_data.stream = gdb_stdout;
  cb_data.values_printed = 0;

  iterate_over_block_arg_vars (SYMBOL_BLOCK_VALUE (func),
			       do_print_variable_and_value, &cb_data);

  /* do_print_variable_and_value invalidates FRAME.  */
  frame = NULL;

  if (!cb_data.values_printed)
    fprintf_filtered (stream, _("No arguments.\n"));
}

void
args_info (char *ignore, int from_tty)
{
  print_frame_arg_vars (get_selected_frame (_("No frame selected.")),
			gdb_stdout);
}


static void
args_plus_locals_info (char *ignore, int from_tty)
{
  args_info (ignore, from_tty);
  locals_info (ignore, from_tty);
}


/* Select frame FRAME.  Also print the stack frame and show the source
   if this is the tui version.  */
static void
select_and_print_frame (struct frame_info *frame)
{
  select_frame (frame);
  if (frame)
    print_stack_frame (frame, 1, SRC_AND_LOC, 1);
}

/* Return the symbol-block in which the selected frame is executing.
   Can return zero under various legitimate circumstances.

   If ADDR_IN_BLOCK is non-zero, set *ADDR_IN_BLOCK to the relevant
   code address within the block returned.  We use this to decide
   which macros are in scope.  */

struct block *
get_selected_block (CORE_ADDR *addr_in_block)
{
  if (!has_stack_frames ())
    return 0;

  return get_frame_block (get_selected_frame (NULL), addr_in_block);
}

/* Find a frame a certain number of levels away from FRAME.
   LEVEL_OFFSET_PTR points to an int containing the number of levels.
   Positive means go to earlier frames (up); negative, the reverse.
   The int that contains the number of levels is counted toward
   zero as the frames for those levels are found.
   If the top or bottom frame is reached, that frame is returned,
   but the final value of *LEVEL_OFFSET_PTR is nonzero and indicates
   how much farther the original request asked to go.  */

struct frame_info *
find_relative_frame (struct frame_info *frame, int *level_offset_ptr)
{
  /* Going up is simple: just call get_prev_frame enough times or
     until the initial frame is reached.  */
  while (*level_offset_ptr > 0)
    {
      struct frame_info *prev = get_prev_frame (frame);

      if (!prev)
	break;
      (*level_offset_ptr)--;
      frame = prev;
    }

  /* Going down is just as simple.  */
  while (*level_offset_ptr < 0)
    {
      struct frame_info *next = get_next_frame (frame);

      if (!next)
	break;
      (*level_offset_ptr)++;
      frame = next;
    }

  return frame;
}

/* The "select_frame" command.  With no argument this is a NOP.
   Select the frame at level LEVEL_EXP if it is a valid level.
   Otherwise, treat LEVEL_EXP as an address expression and select it.

   See parse_frame_specification for more info on proper frame
   expressions.  */

void
select_frame_command (char *level_exp, int from_tty)
{
  select_frame (parse_frame_specification_1 (level_exp, "No stack.", NULL));
}

/* The "frame" command.  With no argument, print the selected frame
   briefly.  With an argument, behave like select_frame and then print
   the selected frame.  */

static void
frame_command (char *level_exp, int from_tty)
{
  select_frame_command (level_exp, from_tty);
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* The XDB Compatibility command to print the current frame.  */

static void
current_frame_command (char *level_exp, int from_tty)
{
  print_stack_frame (get_selected_frame (_("No stack.")), 1, SRC_AND_LOC, 1);
}

/* Select the frame up one or COUNT_EXP stack levels from the
   previously selected frame, and print it briefly.  */

static void
up_silently_base (char *count_exp)
{
  struct frame_info *frame;
  int count = 1;

  if (count_exp)
    count = parse_and_eval_long (count_exp);

  frame = find_relative_frame (get_selected_frame ("No stack."), &count);
  if (count != 0 && count_exp == NULL)
    error (_("Initial frame selected; you cannot go up."));
  select_frame (frame);
}

static void
up_silently_command (char *count_exp, int from_tty)
{
  up_silently_base (count_exp);
}

static void
up_command (char *count_exp, int from_tty)
{
  up_silently_base (count_exp);
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* Select the frame down one or COUNT_EXP stack levels from the previously
   selected frame, and print it briefly.  */

static void
down_silently_base (char *count_exp)
{
  struct frame_info *frame;
  int count = -1;

  if (count_exp)
    count = -parse_and_eval_long (count_exp);

  frame = find_relative_frame (get_selected_frame ("No stack."), &count);
  if (count != 0 && count_exp == NULL)
    {
      /* We only do this if COUNT_EXP is not specified.  That way
         "down" means to really go down (and let me know if that is
         impossible), but "down 9999" can be used to mean go all the
         way down without getting an error.  */

      error (_("Bottom (innermost) frame selected; you cannot go down."));
    }

  select_frame (frame);
}

static void
down_silently_command (char *count_exp, int from_tty)
{
  down_silently_base (count_exp);
}

static void
down_command (char *count_exp, int from_tty)
{
  down_silently_base (count_exp);
  print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}


void
return_command (char *retval_exp, int from_tty)
{
  /* Initialize it just to avoid a GCC false warning.  */
  enum return_value_convention rv_conv = RETURN_VALUE_STRUCT_CONVENTION;
  struct frame_info *thisframe;
  struct gdbarch *gdbarch;
  struct symbol *thisfun;
  struct value *return_value = NULL;
  struct value *function = NULL;
  const char *query_prefix = "";

  thisframe = get_selected_frame ("No selected frame.");
  thisfun = get_frame_function (thisframe);
  gdbarch = get_frame_arch (thisframe);

  if (get_frame_type (get_current_frame ()) == INLINE_FRAME)
    error (_("Can not force return from an inlined function."));

  /* Compute the return value.  If the computation triggers an error,
     let it bail.  If the return type can't be handled, set
     RETURN_VALUE to NULL, and QUERY_PREFIX to an informational
     message.  */
  if (retval_exp)
    {
      struct expression *retval_expr = parse_expression (retval_exp);
      struct cleanup *old_chain = make_cleanup (xfree, retval_expr);
      struct type *return_type = NULL;

      /* Compute the return value.  Should the computation fail, this
         call throws an error.  */
      return_value = evaluate_expression (retval_expr);

      /* Cast return value to the return type of the function.  Should
         the cast fail, this call throws an error.  */
      if (thisfun != NULL)
	return_type = TYPE_TARGET_TYPE (SYMBOL_TYPE (thisfun));
      if (return_type == NULL)
      	{
	  if (retval_expr->elts[0].opcode != UNOP_CAST
	      && retval_expr->elts[0].opcode != UNOP_CAST_TYPE)
	    error (_("Return value type not available for selected "
		     "stack frame.\n"
		     "Please use an explicit cast of the value to return."));
	  return_type = value_type (return_value);
	}
      do_cleanups (old_chain);
      CHECK_TYPEDEF (return_type);
      return_value = value_cast (return_type, return_value);

      /* Make sure the value is fully evaluated.  It may live in the
         stack frame we're about to pop.  */
      if (value_lazy (return_value))
	value_fetch_lazy (return_value);

      if (thisfun != NULL)
	function = read_var_value (thisfun, thisframe);

      rv_conv = RETURN_VALUE_REGISTER_CONVENTION;
      if (TYPE_CODE (return_type) == TYPE_CODE_VOID)
	/* If the return-type is "void", don't try to find the
           return-value's location.  However, do still evaluate the
           return expression so that, even when the expression result
           is discarded, side effects such as "return i++" still
           occur.  */
	return_value = NULL;
      else if (thisfun != NULL)
	{
	  rv_conv = struct_return_convention (gdbarch, function, return_type);
	  if (rv_conv == RETURN_VALUE_STRUCT_CONVENTION
	      || rv_conv == RETURN_VALUE_ABI_RETURNS_ADDRESS)
	    {
	      query_prefix = "The location at which to store the "
		"function's return value is unknown.\n"
		"If you continue, the return value "
		"that you specified will be ignored.\n";
	      return_value = NULL;
	    }
	}
    }

  /* Does an interactive user really want to do this?  Include
     information, such as how well GDB can handle the return value, in
     the query message.  */
  if (from_tty)
    {
      int confirmed;

      if (thisfun == NULL)
	confirmed = query (_("%sMake selected stack frame return now? "),
			   query_prefix);
      else
	confirmed = query (_("%sMake %s return now? "), query_prefix,
			   SYMBOL_PRINT_NAME (thisfun));
      if (!confirmed)
	error (_("Not confirmed"));
    }

  /* Discard the selected frame and all frames inner-to it.  */
  frame_pop (get_selected_frame (NULL));

  /* Store RETURN_VALUE in the just-returned register set.  */
  if (return_value != NULL)
    {
      struct type *return_type = value_type (return_value);
      struct gdbarch *gdbarch = get_regcache_arch (get_current_regcache ());

      gdb_assert (rv_conv != RETURN_VALUE_STRUCT_CONVENTION
		  && rv_conv != RETURN_VALUE_ABI_RETURNS_ADDRESS);
      gdbarch_return_value (gdbarch, function, return_type,
			    get_current_regcache (), NULL /*read*/,
			    value_contents (return_value) /*write*/);
    }

  /* If we are at the end of a call dummy now, pop the dummy frame
     too.  */
  if (get_frame_type (get_current_frame ()) == DUMMY_FRAME)
    frame_pop (get_current_frame ());

  /* If interactive, print the frame that is now current.  */
  if (from_tty)
    frame_command ("0", 1);
  else
    select_frame_command ("0", 0);
}

/* Sets the scope to input function name, provided that the function
   is within the current stack frame.  */

struct function_bounds
{
  CORE_ADDR low, high;
};

static void
func_command (char *arg, int from_tty)
{
  struct frame_info *frame;
  int found = 0;
  struct symtabs_and_lines sals;
  int i;
  int level = 1;
  struct function_bounds *func_bounds = NULL;
  struct cleanup *cleanups;

  if (arg != NULL)
    return;

  frame = parse_frame_specification ("0");
  sals = decode_line_with_current_source (arg, DECODE_LINE_FUNFIRSTLINE);
  cleanups = make_cleanup (xfree, sals.sals);
  func_bounds = (struct function_bounds *) xmalloc (
			      sizeof (struct function_bounds) * sals.nelts);
  make_cleanup (xfree, func_bounds);
  for (i = 0; (i < sals.nelts && !found); i++)
    {
      if (sals.sals[i].pspace != current_program_space)
	func_bounds[i].low = func_bounds[i].high = 0;
      else if (sals.sals[i].pc == 0
	       || find_pc_partial_function (sals.sals[i].pc, NULL,
					    &func_bounds[i].low,
					    &func_bounds[i].high) == 0)
	{
	  func_bounds[i].low = func_bounds[i].high = 0;
	}
    }

  do
    {
      for (i = 0; (i < sals.nelts && !found); i++)
	found = (get_frame_pc (frame) >= func_bounds[i].low
		 && get_frame_pc (frame) < func_bounds[i].high);
      if (!found)
	{
	  level = 1;
	  frame = find_relative_frame (frame, &level);
	}
    }
  while (!found && level == 0);

  do_cleanups (cleanups);

  if (!found)
    printf_filtered (_("'%s' not within current stack frame.\n"), arg);
  else if (frame != get_selected_frame (NULL))
    select_and_print_frame (frame);
}

/* Gets the language of the current frame.  */

enum language
get_frame_language (void)
{
  struct frame_info *frame = deprecated_safe_get_selected_frame ();

  if (frame)
    {
      volatile struct gdb_exception ex;
      CORE_ADDR pc = 0;
      struct symtab *s;

      /* We determine the current frame language by looking up its
         associated symtab.  To retrieve this symtab, we use the frame
         PC.  However we cannot use the frame PC as is, because it
         usually points to the instruction following the "call", which
         is sometimes the first instruction of another function.  So
         we rely on get_frame_address_in_block(), it provides us with
         a PC that is guaranteed to be inside the frame's code
         block.  */

      TRY_CATCH (ex, RETURN_MASK_ERROR)
	{
	  pc = get_frame_address_in_block (frame);
	}
      if (ex.reason < 0)
	{
	  if (ex.error != NOT_AVAILABLE_ERROR)
	    throw_exception (ex);
	}
      else
	{
	  s = find_pc_symtab (pc);
	  if (s != NULL)
	    return s->language;
	}
    }

  return language_unknown;
}


/* Provide a prototype to silence -Wmissing-prototypes.  */
void _initialize_stack (void);

void
_initialize_stack (void)
{
  add_com ("return", class_stack, return_command, _("\
Make selected stack frame return to its caller.\n\
Control remains in the debugger, but when you continue\n\
execution will resume in the frame above the one now selected.\n\
If an argument is given, it is an expression for the value to return."));

  add_com ("up", class_stack, up_command, _("\
Select and print stack frame that called this one.\n\
An argument says how many frames up to go."));
  add_com ("up-silently", class_support, up_silently_command, _("\
Same as the `up' command, but does not print anything.\n\
This is useful in command scripts."));

  add_com ("down", class_stack, down_command, _("\
Select and print stack frame called by this one.\n\
An argument says how many frames down to go."));
  add_com_alias ("do", "down", class_stack, 1);
  add_com_alias ("dow", "down", class_stack, 1);
  add_com ("down-silently", class_support, down_silently_command, _("\
Same as the `down' command, but does not print anything.\n\
This is useful in command scripts."));

  add_com ("frame", class_stack, frame_command, _("\
Select and print a stack frame.\nWith no argument, \
print the selected stack frame.  (See also \"info frame\").\n\
An argument specifies the frame to select.\n\
It can be a stack frame number or the address of the frame.\n\
With argument, nothing is printed if input is coming from\n\
a command file or a user-defined command."));

  add_com_alias ("f", "frame", class_stack, 1);

  if (xdb_commands)
    {
      add_com ("L", class_stack, current_frame_command,
	       _("Print the current stack frame.\n"));
      add_com_alias ("V", "frame", class_stack, 1);
    }
  add_com ("select-frame", class_stack, select_frame_command, _("\
Select a stack frame without printing anything.\n\
An argument specifies the frame to select.\n\
It can be a stack frame number or the address of the frame.\n"));

  add_com ("backtrace", class_stack, backtrace_command, _("\
Print backtrace of all stack frames, or innermost COUNT frames.\n\
With a negative argument, print outermost -COUNT frames.\nUse of the \
'full' qualifier also prints the values of the local variables.\n\
Use of the 'no-filters' qualifier prohibits frame filters from executing\n\
on this backtrace.\n"));
  add_com_alias ("bt", "backtrace", class_stack, 0);
  if (xdb_commands)
    {
      add_com_alias ("t", "backtrace", class_stack, 0);
      add_com ("T", class_stack, backtrace_full_command, _("\
Print backtrace of all stack frames, or innermost COUNT frames\n\
and the values of the local variables.\n\
With a negative argument, print outermost -COUNT frames.\n\
Usage: T <count>\n"));
    }

  add_com_alias ("where", "backtrace", class_alias, 0);
  add_info ("stack", backtrace_command,
	    _("Backtrace of the stack, or innermost COUNT frames."));
  add_info_alias ("s", "stack", 1);
  add_info ("frame", frame_info,
	    _("All about selected stack frame, or frame at ADDR."));
  add_info_alias ("f", "frame", 1);
  add_info ("locals", locals_info,
	    _("Local variables of current stack frame."));
  add_info ("args", args_info,
	    _("Argument variables of current stack frame."));
  if (xdb_commands)
    add_com ("l", class_info, args_plus_locals_info,
	     _("Argument and local variables of current stack frame."));

  if (dbx_commands)
    add_com ("func", class_stack, func_command, _("\
Select the stack frame that contains <func>.\n\
Usage: func <name>\n"));

  add_setshow_enum_cmd ("frame-arguments", class_stack,
			print_frame_arguments_choices, &print_frame_arguments,
			_("Set printing of non-scalar frame arguments"),
			_("Show printing of non-scalar frame arguments"),
			NULL, NULL, NULL, &setprintlist, &showprintlist);

  add_setshow_boolean_cmd ("frame-arguments", no_class,
			   &print_raw_frame_arguments, _("\
Set whether to print frame arguments in raw form."), _("\
Show whether to print frame arguments in raw form."), _("\
If set, frame arguments are printed in raw form, bypassing any\n\
pretty-printers for that value."),
			   NULL, NULL,
			   &setprintrawlist, &showprintrawlist);

  add_setshow_auto_boolean_cmd ("disassemble-next-line", class_stack,
			        &disassemble_next_line, _("\
Set whether to disassemble next source line or insn when execution stops."),
				_("\
Show whether to disassemble next source line or insn when execution stops."),
				_("\
If ON, GDB will display disassembly of the next source line, in addition\n\
to displaying the source line itself.  If the next source line cannot\n\
be displayed (e.g., source is unavailable or there's no line info), GDB\n\
will display disassembly of next instruction instead of showing the\n\
source line.\n\
If AUTO, display disassembly of next instruction only if the source line\n\
cannot be displayed.\n\
If OFF (which is the default), never display the disassembly of the next\n\
source line."),
			        NULL,
			        show_disassemble_next_line,
			        &setlist, &showlist);
  disassemble_next_line = AUTO_BOOLEAN_FALSE;

  add_setshow_enum_cmd ("entry-values", class_stack,
			print_entry_values_choices, &print_entry_values,
			_("Set printing of function arguments at function "
			  "entry"),
			_("Show printing of function arguments at function "
			  "entry"),
			_("\
GDB can sometimes determine the values of function arguments at entry,\n\
in addition to their current values.  This option tells GDB whether\n\
to print the current value, the value at entry (marked as val@entry),\n\
or both.  Note that one or both of these values may be <optimized out>."),
			NULL, NULL, &setprintlist, &showprintlist);
}
