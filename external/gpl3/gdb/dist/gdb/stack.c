/* Print and select stack frames for GDB, the GNU debugger.

   Copyright (C) 1986-2019 Free Software Foundation, Inc.

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
#include "objfiles.h"

#include "symfile.h"
#include "extension.h"
#include "observable.h"
#include "common/def-vector.h"

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

static void print_frame_local_vars (struct frame_info *frame,
				    bool quiet,
				    const char *regexp, const char *t_regexp,
				    int num_tabs, struct ui_file *stream);

static void print_frame (struct frame_info *frame, int print_level,
			 enum print_what print_what,  int print_args,
			 struct symtab_and_line sal);

static void set_last_displayed_sal (int valid,
				    struct program_space *pspace,
				    CORE_ADDR addr,
				    struct symtab *symtab,
				    int line);

static struct frame_info *find_frame_for_function (const char *);
static struct frame_info *find_frame_for_address (CORE_ADDR);

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
	gdb_assert (inline_skipped_frames (inferior_thread ()) > 0);
      else
	gdb_assert (get_frame_type (get_next_frame (frame)) == INLINE_FRAME);
      return 0;
    }

  return get_frame_pc (frame) != sal.pc;
}

/* See frame.h.  */

void
print_stack_frame_to_uiout (struct ui_out *uiout, struct frame_info *frame,
			    int print_level, enum print_what print_what,
			    int set_current_sal)
{
  scoped_restore save_uiout = make_scoped_restore (&current_uiout, uiout);

  print_stack_frame (frame, print_level, print_what, set_current_sal);
}

/* Show or print a stack frame FRAME briefly.  The output is formatted
   according to PRINT_LEVEL and PRINT_WHAT printing the frame's
   relative level, function name, argument list, and file name and
   line number.  If the frame's PC is not at the beginning of the
   source line, the actual PC is printed at the beginning.  */

void
print_stack_frame (struct frame_info *frame, int print_level,
		   enum print_what print_what,
		   int set_current_sal)
{

  /* For mi, alway print location and address.  */
  if (current_uiout->is_mi_like_p ())
    print_what = LOC_AND_ADDRESS;

  TRY
    {
      print_frame_info (frame, print_level, print_what, 1 /* print_args */,
			set_current_sal);
      if (set_current_sal)
	set_current_sal_from_frame (frame);
    }
  CATCH (e, RETURN_MASK_ERROR)
    {
    }
  END_CATCH
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
  const char *error_message = NULL;

  string_file stb;

  gdb_assert (!arg->val || !arg->error);
  gdb_assert (arg->entry_kind == print_entry_values_no
	      || arg->entry_kind == print_entry_values_only
	      || (!uiout->is_mi_like_p ()
		  && arg->entry_kind == print_entry_values_compact));

  annotate_arg_emitter arg_emitter;
  ui_out_emit_tuple tuple_emitter (uiout, NULL);
  fprintf_symbol_filtered (&stb, SYMBOL_PRINT_NAME (arg->sym),
			   SYMBOL_LANGUAGE (arg->sym), DMGL_PARAMS | DMGL_ANSI);
  if (arg->entry_kind == print_entry_values_compact)
    {
      /* It is OK to provide invalid MI-like stream as with
	 PRINT_ENTRY_VALUE_COMPACT we never use MI.  */
      stb.puts ("=");

      fprintf_symbol_filtered (&stb, SYMBOL_PRINT_NAME (arg->sym),
			       SYMBOL_LANGUAGE (arg->sym),
			       DMGL_PARAMS | DMGL_ANSI);
    }
  if (arg->entry_kind == print_entry_values_only
      || arg->entry_kind == print_entry_values_compact)
    stb.puts ("@entry");
  uiout->field_stream ("name", stb, ui_out_style_kind::VARIABLE);
  annotate_arg_name_end ();
  uiout->text ("=");

  if (!arg->val && !arg->error)
    uiout->text ("...");
  else
    {
      if (arg->error)
	error_message = arg->error;
      else
	{
	  TRY
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

	      common_val_print (arg->val, &stb, 2, &opts, language);
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      error_message = except.message;
	    }
	  END_CATCH
	}
      if (error_message != NULL)
	stb.printf (_("<error reading variable: %s>"), error_message);
    }

  uiout->field_stream ("value", stb);
}

/* Read in inferior function local SYM at FRAME into ARGP.  Caller is
   responsible for xfree of ARGP->ERROR.  This function never throws an
   exception.  */

void
read_frame_local (struct symbol *sym, struct frame_info *frame,
		  struct frame_arg *argp)
{
  argp->sym = sym;
  argp->val = NULL;
  argp->error = NULL;

  TRY
    {
      argp->val = read_var_value (sym, NULL, frame);
    }
  CATCH (except, RETURN_MASK_ERROR)
    {
      argp->error = xstrdup (except.message);
    }
  END_CATCH
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

  if (print_entry_values != print_entry_values_only
      && print_entry_values != print_entry_values_preferred)
    {
      TRY
	{
	  val = read_var_value (sym, NULL, frame);
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  val_error = (char *) alloca (strlen (except.message) + 1);
	  strcpy (val_error, except.message);
	}
      END_CATCH
    }

  if (SYMBOL_COMPUTED_OPS (sym) != NULL
      && SYMBOL_COMPUTED_OPS (sym)->read_variable_at_entry != NULL
      && print_entry_values != print_entry_values_no
      && (print_entry_values != print_entry_values_if_needed
	  || !val || value_optimized_out (val)))
    {
      TRY
	{
	  const struct symbol_computed_ops *ops;

	  ops = SYMBOL_COMPUTED_OPS (sym);
	  entryval = ops->read_variable_at_entry (sym, frame);
	}
      CATCH (except, RETURN_MASK_ERROR)
	{
	  if (except.error != NO_ENTRY_VALUE_ERROR)
	    {
	      entryval_error = (char *) alloca (strlen (except.message) + 1);
	      strcpy (entryval_error, except.message);
	    }
	}
      END_CATCH

      if (entryval != NULL && value_optimized_out (entryval))
	entryval = NULL;

      if (print_entry_values == print_entry_values_compact
	  || print_entry_values == print_entry_values_default)
	{
	  /* For MI do not try to use print_entry_values_compact for ARGP.  */

	  if (val && entryval && !current_uiout->is_mi_like_p ())
	    {
	      struct type *type = value_type (val);

	      if (value_lazy (val))
		value_fetch_lazy (val);
	      if (value_lazy (entryval))
		value_fetch_lazy (entryval);

	      if (value_contents_eq (val, 0, entryval, 0, TYPE_LENGTH (type)))
		{
		  /* Initialize it just to avoid a GCC false warning.  */
		  struct value *val_deref = NULL, *entryval_deref;

		  /* DW_AT_call_value does match with the current
		     value.  If it is a reference still try to verify if
		     dereferenced DW_AT_call_data_value does not differ.  */

		  TRY
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
			  && value_contents_eq (val_deref, 0,
						entryval_deref, 0,
						TYPE_LENGTH (type_deref)))
			val_equal = 1;
		    }
		  CATCH (except, RETURN_MASK_ERROR)
		    {
		      /* If the dereferenced content could not be
			 fetched do not display anything.  */
		      if (except.error == NO_ENTRY_VALUE_ERROR)
			val_equal = 1;
		      else if (except.message != NULL)
			{
			  entryval_error = (char *) alloca (strlen (except.message) + 1);
			  strcpy (entryval_error, except.message);
			}
		    }
		  END_CATCH

		  /* Value was not a reference; and its content matches.  */
		  if (val == val_deref)
		    val_equal = 1;

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
	  gdb_assert (val == NULL);

	  TRY
	    {
	      val = read_var_value (sym, NULL, frame);
	    }
	  CATCH (except, RETURN_MASK_ERROR)
	    {
	      val_error = (char *) alloca (strlen (except.message) + 1);
	      strcpy (val_error, except.message);
	    }
	  END_CATCH
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
      gdb_assert (!current_uiout->is_mi_like_p ());
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
  /* True if we should print arguments, false otherwise.  */
  int print_args = strcmp (print_frame_arguments, "none");

  if (func)
    {
      const struct block *b = SYMBOL_BLOCK_VALUE (func);
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

	      nsym = lookup_symbol_search_name (SYMBOL_SEARCH_NAME (sym),
						b, VAR_DOMAIN).symbol;
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
	    uiout->text (", ");
	  uiout->wrap_hint ("    ");

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
		  uiout->text (", ");
		  uiout->wrap_hint ("    ");
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
}

/* Set the current source and line to the location given by frame
   FRAME, if possible.  When CENTER is true, adjust so the relevant
   line is in the center of the next 'list'.  */

void
set_current_sal_from_frame (struct frame_info *frame)
{
  symtab_and_line sal = find_frame_sal (frame);
  if (sal.symtab != NULL)
    set_current_source_symtab_and_line (sal);
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

  TRY
    {
      gdb_disassembly (gdbarch, current_uiout,
		       DISASSEMBLY_RAW_INSN, how_many,
		       low, high);
    }
  CATCH (exception, RETURN_MASK_ERROR)
    {
      /* If an exception was thrown while doing the disassembly, print
	 the error message, to give the user a clue of what happened.  */
      exception_print (gdb_stderr, exception);
    }
  END_CATCH
}

/* Print information about frame FRAME.  The output is format according
   to PRINT_LEVEL and PRINT_WHAT and PRINT_ARGS.  The meaning of
   PRINT_WHAT is:
   
   SRC_LINE: Print only source line.
   LOCATION: Print only location.
   SRC_AND_LOC: Print location and source line.

   Used in "where" output, and to emit breakpoint or step
   messages.  */

void
print_frame_info (struct frame_info *frame, int print_level,
		  enum print_what print_what, int print_args,
		  int set_current_sal)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  int source_print;
  int location_print;
  struct ui_out *uiout = current_uiout;

  if (get_frame_type (frame) == DUMMY_FRAME
      || get_frame_type (frame) == SIGTRAMP_FRAME
      || get_frame_type (frame) == ARCH_FRAME)
    {
      ui_out_emit_tuple tuple_emitter (uiout, "frame");

      annotate_frame_begin (print_level ? frame_relative_level (frame) : 0,
			    gdbarch, get_frame_pc (frame));

      /* Do this regardless of SOURCE because we don't have any source
         to list for this frame.  */
      if (print_level)
        {
          uiout->text ("#");
          uiout->field_fmt_int (2, ui_left, "level",
				frame_relative_level (frame));
        }
      if (uiout->is_mi_like_p ())
        {
          annotate_frame_address ();
          uiout->field_core_addr ("addr",
				  gdbarch, get_frame_pc (frame));
          annotate_frame_address_end ();
        }

      if (get_frame_type (frame) == DUMMY_FRAME)
        {
          annotate_function_call ();
          uiout->field_string ("func", "<function called from gdb>",
			       ui_out_style_kind::FUNCTION);
	}
      else if (get_frame_type (frame) == SIGTRAMP_FRAME)
        {
	  annotate_signal_handler_caller ();
          uiout->field_string ("func", "<signal handler called>",
			       ui_out_style_kind::FUNCTION);
        }
      else if (get_frame_type (frame) == ARCH_FRAME)
        {
          uiout->field_string ("func", "<cross-architecture call>",
			       ui_out_style_kind::FUNCTION);
	}
      uiout->text ("\n");
      annotate_frame_end ();

      /* If disassemble-next-line is set to auto or on output the next
	 instruction.  */
      if (disassemble_next_line == AUTO_BOOLEAN_AUTO
	  || disassemble_next_line == AUTO_BOOLEAN_TRUE)
	do_gdb_disassembly (get_frame_arch (frame), 1,
			    get_frame_pc (frame), get_frame_pc (frame) + 1);

      return;
    }

  /* If FRAME is not the innermost frame, that normally means that
     FRAME->pc points to *after* the call instruction, and we want to
     get the line containing the call, never the next line.  But if
     the next frame is a SIGTRAMP_FRAME or a DUMMY_FRAME, then the
     next frame was not entered as the result of a call, and we want
     to get the line containing FRAME->pc.  */
  symtab_and_line sal = find_frame_sal (frame);

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
		  uiout->field_core_addr ("addr",
					  gdbarch, get_frame_pc (frame));
		  uiout->text ("\t");
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

symtab_and_line
get_last_displayed_sal ()
{
  symtab_and_line sal;

  if (last_displayed_sal_valid)
    {
      sal.pspace = last_displayed_pspace;
      sal.pc = last_displayed_addr;
      sal.symtab = last_displayed_symtab;
      sal.line = last_displayed_line;
    }

  return sal;
}


/* Attempt to obtain the name, FUNLANG and optionally FUNCP of the function
   corresponding to FRAME.  */

gdb::unique_xmalloc_ptr<char>
find_frame_funname (struct frame_info *frame, enum language *funlang,
		    struct symbol **funcp)
{
  struct symbol *func;
  gdb::unique_xmalloc_ptr<char> funname;

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
	  && (BMSYMBOL_VALUE_ADDRESS (msymbol)
	      > BLOCK_ENTRY_PC (SYMBOL_BLOCK_VALUE (func))))
	{
	  /* We also don't know anything about the function besides
	     its address and name.  */
	  func = 0;
	  funname.reset (xstrdup (MSYMBOL_PRINT_NAME (msymbol.minsym)));
	  *funlang = MSYMBOL_LANGUAGE (msymbol.minsym);
	}
      else
	{
	  const char *print_name = SYMBOL_PRINT_NAME (func);

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
	      funname = cp_remove_params (print_name);
	    }

	  /* If we didn't hit the C++ case above, set *funname
	     here.  */
	  if (funname == NULL)
	    funname.reset (xstrdup (print_name));
	}
    }
  else
    {
      struct bound_minimal_symbol msymbol;
      CORE_ADDR pc;

      if (!get_frame_address_in_block_if_available (frame, &pc))
	return funname;

      msymbol = lookup_minimal_symbol_by_pc (pc);
      if (msymbol.minsym != NULL)
	{
	  funname.reset (xstrdup (MSYMBOL_PRINT_NAME (msymbol.minsym)));
	  *funlang = MSYMBOL_LANGUAGE (msymbol.minsym);
	}
    }

  return funname;
}

static void
print_frame (struct frame_info *frame, int print_level,
	     enum print_what print_what, int print_args,
	     struct symtab_and_line sal)
{
  struct gdbarch *gdbarch = get_frame_arch (frame);
  struct ui_out *uiout = current_uiout;
  enum language funlang = language_unknown;
  struct value_print_options opts;
  struct symbol *func;
  CORE_ADDR pc = 0;
  int pc_p;

  pc_p = get_frame_pc_if_available (frame, &pc);

  gdb::unique_xmalloc_ptr<char> funname
    = find_frame_funname (frame, &funlang, &func);

  annotate_frame_begin (print_level ? frame_relative_level (frame) : 0,
			gdbarch, pc);

  {
    ui_out_emit_tuple tuple_emitter (uiout, "frame");

    if (print_level)
      {
	uiout->text ("#");
	uiout->field_fmt_int (2, ui_left, "level",
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
	    uiout->field_core_addr ("addr", gdbarch, pc);
	  else
	    uiout->field_string ("addr", "<unavailable>",
				 ui_out_style_kind::ADDRESS);
	  annotate_frame_address_end ();
	  uiout->text (" in ");
	}
    annotate_frame_function_name ();

    string_file stb;
    fprintf_symbol_filtered (&stb, funname ? funname.get () : "??",
			     funlang, DMGL_ANSI);
    uiout->field_stream ("func", stb, ui_out_style_kind::FUNCTION);
    uiout->wrap_hint ("   ");
    annotate_frame_args ();

    uiout->text (" (");
    if (print_args)
      {
	int numargs;

	if (gdbarch_frame_num_args_p (gdbarch))
	  {
	    numargs = gdbarch_frame_num_args (gdbarch, frame);
	    gdb_assert (numargs >= 0);
	  }
	else
	  numargs = -1;
    
	{
	  ui_out_emit_list list_emitter (uiout, "args");
	  TRY
	    {
	      print_frame_args (func, frame, numargs, gdb_stdout);
	    }
	  CATCH (e, RETURN_MASK_ERROR)
	    {
	    }
	  END_CATCH

	    /* FIXME: ARGS must be a list.  If one argument is a string it
	       will have " that will not be properly escaped.  */
	    }
	QUIT;
      }
    uiout->text (")");
    if (sal.symtab)
      {
	const char *filename_display;
      
	filename_display = symtab_to_filename_for_display (sal.symtab);
	annotate_frame_source_begin ();
	uiout->wrap_hint ("   ");
	uiout->text (" at ");
	annotate_frame_source_file ();
	uiout->field_string ("file", filename_display, ui_out_style_kind::FILE);
	if (uiout->is_mi_like_p ())
	  {
	    const char *fullname = symtab_to_fullname (sal.symtab);

	    uiout->field_string ("fullname", fullname);
	  }
	annotate_frame_source_file_end ();
	uiout->text (":");
	annotate_frame_source_line ();
	uiout->field_int ("line", sal.line);
	annotate_frame_source_end ();
      }

    if (pc_p && (funname == NULL || sal.symtab == NULL))
      {
	char *lib = solib_name_from_address (get_frame_program_space (frame),
					     get_frame_pc (frame));

	if (lib)
	  {
	    annotate_frame_where ();
	    uiout->wrap_hint ("  ");
	    uiout->text (" from ");
	    uiout->field_string ("from", lib);
	  }
      }
    if (uiout->is_mi_like_p ())
      uiout->field_string ("arch",
			   (gdbarch_bfd_arch_info (gdbarch))->printable_name);
  }

  uiout->text ("\n");
}


/* Completion function for "frame function", "info frame function", and
   "select-frame function" commands.  */

void
frame_selection_by_function_completer (struct cmd_list_element *ignore,
				       completion_tracker &tracker,
				       const char *text, const char *word)
{
  /* This is used to complete function names within a stack.  It would be
     nice if we only offered functions that were actually in the stack.
     However, this would mean unwinding the stack to completion, which
     could take too long, or on a corrupted stack, possibly not end.
     Instead, we offer all symbol names as a safer choice.  */
  collect_symbol_completion_matches (tracker,
				     complete_symbol_mode::EXPRESSION,
				     symbol_name_match_type::EXPRESSION,
				     text, word);
}

/* Core of all the "info frame" sub-commands.  Print information about a
   frame FI.  If SELECTED_FRAME_P is true then the user didn't provide a
   frame specification, they just entered 'info frame'.  If the user did
   provide a frame specification (for example 'info frame 0', 'info frame
   level 1') then SELECTED_FRAME_P will be false.  */

static void
info_frame_command_core (struct frame_info *fi, bool selected_frame_p)
{
  struct symbol *func;
  struct symtab *s;
  struct frame_info *calling_frame_info;
  int numregs;
  const char *funname = 0;
  enum language funlang = language_unknown;
  const char *pc_regname;
  struct gdbarch *gdbarch;
  CORE_ADDR frame_pc;
  int frame_pc_p;
  /* Initialize it to avoid "may be used uninitialized" warning.  */
  CORE_ADDR caller_pc = 0;
  int caller_pc_p = 0;

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
  func = get_frame_function (fi);
  symtab_and_line sal = find_frame_sal (fi);
  s = sal.symtab;
  gdb::unique_xmalloc_ptr<char> func_only;
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
	  func_only = cp_remove_params (funname);

	  if (func_only)
	    funname = func_only.get ();
	}
    }
  else if (frame_pc_p)
    {
      struct bound_minimal_symbol msymbol;

      msymbol = lookup_minimal_symbol_by_pc (frame_pc);
      if (msymbol.minsym != NULL)
	{
	  funname = MSYMBOL_PRINT_NAME (msymbol.minsym);
	  funlang = MSYMBOL_LANGUAGE (msymbol.minsym);
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

  if (!frame_id_p (frame_unwind_caller_id (fi)))
    val_print_not_saved (gdb_stdout);
  else
    {
      TRY
	{
	  caller_pc = frame_unwind_caller_pc (fi);
	  caller_pc_p = 1;
	}
      CATCH (ex, RETURN_MASK_ERROR)
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
      END_CATCH
    }

  if (caller_pc_p)
    fputs_filtered (paddress (gdbarch, caller_pc), gdb_stdout);
  printf_filtered ("\n");

  if (calling_frame_info == NULL)
    {
      enum unwind_stop_reason reason;

      reason = get_frame_unwind_stop_reason (fi);
      if (reason != UNWIND_NO_REASON)
	printf_filtered (_(" Outermost frame: %s\n"),
			 frame_stop_reason_string (fi));
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
    int count;
    int i;
    int need_nl = 1;
    int sp_regnum = gdbarch_sp_regnum (gdbarch);

    /* The sp is special; what's displayed isn't the save address, but
       the value of the previous frame's sp.  This is a legacy thing,
       at one stage the frame cached the previous frame's SP instead
       of its address, hence it was easiest to just display the cached
       value.  */
    if (sp_regnum >= 0)
      {
	struct value *value = frame_unwind_register_value (fi, sp_regnum);
	gdb_assert (value != NULL);

	if (!value_optimized_out (value) && value_entirely_available (value))
	  {
	    if (VALUE_LVAL (value) == not_lval)
	      {
		CORE_ADDR sp;
		enum bfd_endian byte_order = gdbarch_byte_order (gdbarch);
		int sp_size = register_size (gdbarch, sp_regnum);

		sp = extract_unsigned_integer (value_contents_all (value),
					       sp_size, byte_order);

		printf_filtered (" Previous frame's sp is ");
		fputs_filtered (paddress (gdbarch, sp), gdb_stdout);
		printf_filtered ("\n");
	      }
	    else if (VALUE_LVAL (value) == lval_memory)
	      {
		printf_filtered (" Previous frame's sp at ");
		fputs_filtered (paddress (gdbarch, value_address (value)),
				gdb_stdout);
		printf_filtered ("\n");
	      }
	    else if (VALUE_LVAL (value) == lval_register)
	      {
		printf_filtered (" Previous frame's sp in %s\n",
				 gdbarch_register_name (gdbarch,
							VALUE_REGNUM (value)));
	      }

	    release_value (value);
	    need_nl = 0;
	  }
	/* else keep quiet.  */
      }

    count = 0;
    numregs = gdbarch_num_cooked_regs (gdbarch);
    for (i = 0; i < numregs; i++)
      if (i != sp_regnum
	  && gdbarch_register_reggroup_p (gdbarch, i, all_reggroup))
	{
	  enum lval_type lval;
	  int optimized;
	  int unavailable;
	  CORE_ADDR addr;
	  int realnum;

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
}

/* Return the innermost frame at level LEVEL.  */

static struct frame_info *
leading_innermost_frame (int level)
{
  struct frame_info *leading;

  leading = get_current_frame ();

  gdb_assert (level >= 0);

  while (leading != nullptr && level)
    {
      QUIT;
      leading = get_prev_frame (leading);
      level--;
    }

  return leading;
}

/* Return the starting frame needed to handle COUNT outermost frames.  */

static struct frame_info *
trailing_outermost_frame (int count)
{
  struct frame_info *current;
  struct frame_info *trailing;

  trailing = get_current_frame ();

  gdb_assert (count > 0);

  current = trailing;
  while (current != nullptr && count--)
    {
      QUIT;
      current = get_prev_frame (current);
    }

  /* Will stop when CURRENT reaches the top of the stack.
     TRAILING will be COUNT below it.  */
  while (current != nullptr)
    {
      QUIT;
      trailing = get_prev_frame (trailing);
      current = get_prev_frame (current);
    }

  return trailing;
}

/* The core of all the "select-frame" sub-commands.  Just wraps a call to
   SELECT_FRAME.  */

static void
select_frame_command_core (struct frame_info *fi, bool ignored)
{
  struct frame_info *prev_frame = get_selected_frame_if_set ();
  select_frame (fi);
  if (get_selected_frame_if_set () != prev_frame)
    gdb::observers::user_selected_context_changed.notify (USER_SELECTED_FRAME);
}

/* See stack.h.  */

void
select_frame_for_mi (struct frame_info *fi)
{
  select_frame_command_core (fi, FALSE /* Ignored.  */);
}

/* The core of all the "frame" sub-commands.  Select frame FI, and if this
   means we change frame send out a change notification (otherwise, just
   reprint the current frame summary).   */

static void
frame_command_core (struct frame_info *fi, bool ignored)
{
  struct frame_info *prev_frame = get_selected_frame_if_set ();

  select_frame (fi);
  if (get_selected_frame_if_set () != prev_frame)
    gdb::observers::user_selected_context_changed.notify (USER_SELECTED_FRAME);
  else
    print_selected_thread_frame (current_uiout, USER_SELECTED_FRAME);
}

/* The three commands 'frame', 'select-frame', and 'info frame' all have a
   common set of sub-commands that allow a specific frame to be selected.
   All of the sub-command functions are static methods within this class
   template which is then instantiated below.  The template parameter is a
   callback used to implement the functionality of the base command
   ('frame', 'select-frame', or 'info frame').

   In the template parameter FI is the frame being selected.  The
   SELECTED_FRAME_P flag is true if the frame being selected was done by
   default, which happens when the user uses the base command with no
   arguments.  For example the commands 'info frame', 'select-frame',
   'frame' will all cause SELECTED_FRAME_P to be true.  In all other cases
   SELECTED_FRAME_P is false.  */

template <void (*FPTR) (struct frame_info *fi, bool selected_frame_p)>
class frame_command_helper
{
public:

  /* The "frame level" family of commands.  The ARG is an integer that is
     the frame's level in the stack.  */
  static void
  level (const char *arg, int from_tty)
  {
    int level = value_as_long (parse_and_eval (arg));
    struct frame_info *fid
      = find_relative_frame (get_current_frame (), &level);
    if (level != 0)
      error (_("No frame at level %s."), arg);
    FPTR (fid, false);
  }

  /* The "frame address" family of commands.  ARG is a stack-pointer
     address for an existing frame.  This command does not allow new
     frames to be created.  */

  static void
  address (const char *arg, int from_tty)
  {
    CORE_ADDR addr = value_as_address (parse_and_eval (arg));
    struct frame_info *fid = find_frame_for_address (addr);
    if (fid == NULL)
      error (_("No frame at address %s."), arg);
    FPTR (fid, false);
  }

  /* The "frame view" family of commands.  ARG is one or two addresses and
     is used to view a frame that might be outside the current backtrace.
     The addresses are stack-pointer address, and (optional) pc-address.  */

  static void
  view (const char *args, int from_tty)
  {
    struct frame_info *fid;

    if (args == NULL)
    error (_("Missing address argument to view a frame"));

    gdb_argv argv (args);

    if (argv.count () == 2)
      {
	CORE_ADDR addr[2];

	addr [0] = value_as_address (parse_and_eval (argv[0]));
	addr [1] = value_as_address (parse_and_eval (argv[1]));
	fid = create_new_frame (addr[0], addr[1]);
      }
    else
      {
	CORE_ADDR addr = value_as_address (parse_and_eval (argv[0]));
	fid = create_new_frame (addr, false);
      }
    FPTR (fid, false);
  }

  /* The "frame function" family of commands.  ARG is the name of a
     function within the stack, the first function (searching from frame
     0) with that name will be selected.  */

  static void
  function (const char *arg, int from_tty)
  {
    if (arg == NULL)
      error (_("Missing function name argument"));
    struct frame_info *fid = find_frame_for_function (arg);
    if (fid == NULL)
      error (_("No frame for function \"%s\"."), arg);
    FPTR (fid, false);
  }

  /* The "frame" base command, that is, when no sub-command is specified.
     If one argument is provided then we assume that this is a frame's
     level as historically, this was the supported command syntax that was
     used most often.

     If no argument is provided, then the current frame is selected.  */

  static void
  base_command (const char *arg, int from_tty)
  {
    if (arg == NULL)
      FPTR (get_selected_frame (_("No stack.")), true);
    else
      level (arg, from_tty);
  }
};

/* Instantiate three FRAME_COMMAND_HELPER instances to implement the
   sub-commands for 'info frame', 'frame', and 'select-frame' commands.  */

static frame_command_helper <info_frame_command_core> info_frame_cmd;
static frame_command_helper <frame_command_core> frame_cmd;
static frame_command_helper <select_frame_command_core> select_frame_cmd;

/* Print briefly all stack frames or just the innermost COUNT_EXP
   frames.  */

static void
backtrace_command_1 (const char *count_exp, frame_filter_flags flags,
		     int no_filters, int from_tty)
{
  struct frame_info *fi;
  int count;
  int py_start = 0, py_end = 0;
  enum ext_lang_bt_status result = EXT_LANG_BT_ERROR;

  if (!target_has_stack)
    error (_("No stack."));

  if (count_exp)
    {
      count = parse_and_eval_long (count_exp);
      if (count < 0)
	py_start = count;
      else
	{
	  py_start = 0;
	  /* The argument to apply_ext_lang_frame_filter is the number
	     of the final frame to print, and frames start at 0.  */
	  py_end = count - 1;
	}
    }
  else
    {
      py_end = -1;
      count = -1;
    }

  if (! no_filters)
    {
      enum ext_lang_frame_args arg_type;

      flags |= PRINT_LEVEL | PRINT_FRAME_INFO | PRINT_ARGS;
      if (from_tty)
	flags |= PRINT_MORE_FRAMES;

      if (!strcmp (print_frame_arguments, "scalars"))
	arg_type = CLI_SCALAR_VALUES;
      else if (!strcmp (print_frame_arguments, "all"))
	arg_type = CLI_ALL_VALUES;
      else
	arg_type = NO_VALUES;

      result = apply_ext_lang_frame_filter (get_current_frame (), flags,
					    arg_type, current_uiout,
					    py_start, py_end);
    }

  /* Run the inbuilt backtrace if there are no filters registered, or
     "no-filters" has been specified from the command.  */
  if (no_filters ||  result == EXT_LANG_BT_NO_FILTERS)
    {
      struct frame_info *trailing;

      /* The following code must do two things.  First, it must set the
	 variable TRAILING to the frame from which we should start
	 printing.  Second, it must set the variable count to the number
	 of frames which we should print, or -1 if all of them.  */

      if (count_exp != NULL && count < 0)
	{
	  trailing = trailing_outermost_frame (-count);
	  count = -1;
	}
      else
	trailing = get_current_frame ();

      for (fi = trailing; fi && count--; fi = get_prev_frame (fi))
	{
	  QUIT;

	  /* Don't use print_stack_frame; if an error() occurs it probably
	     means further attempts to backtrace would fail (on the other
	     hand, perhaps the code does or could be fixed to make sure
	     the frame->prev field gets set to NULL in that case).  */

	  print_frame_info (fi, 1, LOCATION, 1, 0);
	  if ((flags & PRINT_LOCALS) != 0)
	    {
	      struct frame_id frame_id = get_frame_id (fi);

	      print_frame_local_vars (fi, false, NULL, NULL, 1, gdb_stdout);

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
			     frame_stop_reason_string (trailing));
	}
    }
}

static void
backtrace_command (const char *arg, int from_tty)
{
  bool filters = true;
  frame_filter_flags flags = 0;

  if (arg)
    {
      bool done = false;

      while (!done)
	{
	  const char *save_arg = arg;
	  std::string this_arg = extract_arg (&arg);

	  if (this_arg.empty ())
	    break;

	  if (subset_compare (this_arg.c_str (), "no-filters"))
	    filters = false;
	  else if (subset_compare (this_arg.c_str (), "full"))
	    flags |= PRINT_LOCALS;
	  else if (subset_compare (this_arg.c_str (), "hide"))
	    flags |= PRINT_HIDE;
	  else
	    {
	      /* Not a recognized argument, so stop.  */
	      arg = save_arg;
	      done = true;
	    }
	}

      if (*arg == '\0')
	arg = NULL;
    }

  backtrace_command_1 (arg, flags, !filters /* no frame-filters */, from_tty);
}

/* Iterate over the local variables of a block B, calling CB with
   CB_DATA.  */

static void
iterate_over_block_locals (const struct block *b,
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
	case LOC_OPTIMIZED_OUT:
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
iterate_over_block_local_vars (const struct block *block,
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
  gdb::optional<compiled_regex> preg;
  gdb::optional<compiled_regex> treg;
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
  struct print_variable_and_value_data *p
    = (struct print_variable_and_value_data *) cb_data;
  struct frame_info *frame;

  if (p->preg.has_value ()
      && p->preg->exec (SYMBOL_NATURAL_NAME (sym), 0,
			NULL, 0) != 0)
    return;
  if (p->treg.has_value ()
      && !treg_matches_sym_type_name (*p->treg, sym))
    return;

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

/* Prepares the regular expression REG from REGEXP.
   If REGEXP is NULL, it results in an empty regular expression.  */

static void
prepare_reg (const char *regexp, gdb::optional<compiled_regex> *reg)
{
  if (regexp != NULL)
    {
      int cflags = REG_NOSUB | (case_sensitivity == case_sensitive_off
				? REG_ICASE : 0);
      reg->emplace (regexp, cflags, _("Invalid regexp"));
    }
  else
    reg->reset ();
}

/* Print all variables from the innermost up to the function block of FRAME.
   Print them with values to STREAM indented by NUM_TABS.
   If REGEXP is not NULL, only print local variables whose name
   matches REGEXP.
   If T_REGEXP is not NULL, only print local variables whose type
   matches T_REGEXP.
   If no local variables have been printed and !QUIET, prints a message
   explaining why no local variables could be printed.

   This function will invalidate FRAME.  */

static void
print_frame_local_vars (struct frame_info *frame,
			bool quiet,
			const char *regexp, const char *t_regexp,
			int num_tabs, struct ui_file *stream)
{
  struct print_variable_and_value_data cb_data;
  const struct block *block;
  CORE_ADDR pc;

  if (!get_frame_pc_if_available (frame, &pc))
    {
      if (!quiet)
	fprintf_filtered (stream,
			  _("PC unavailable, cannot determine locals.\n"));
      return;
    }

  block = get_frame_block (frame, 0);
  if (block == 0)
    {
      if (!quiet)
	fprintf_filtered (stream, "No symbol table info available.\n");
      return;
    }

  prepare_reg (regexp, &cb_data.preg);
  prepare_reg (t_regexp, &cb_data.treg);
  cb_data.frame_id = get_frame_id (frame);
  cb_data.num_tabs = 4 * num_tabs;
  cb_data.stream = stream;
  cb_data.values_printed = 0;

  /* Temporarily change the selected frame to the given FRAME.
     This allows routines that rely on the selected frame instead
     of being given a frame as parameter to use the correct frame.  */
  scoped_restore_selected_frame restore_selected_frame;
  select_frame (frame);

  iterate_over_block_local_vars (block,
				 do_print_variable_and_value,
				 &cb_data);

  if (!cb_data.values_printed && !quiet)
    {
      if (regexp == NULL && t_regexp == NULL)
	fprintf_filtered (stream, _("No locals.\n"));
      else
	fprintf_filtered (stream, _("No matching locals.\n"));
    }
}

void
info_locals_command (const char *args, int from_tty)
{
  std::string regexp;
  std::string t_regexp;
  bool quiet = false;

  while (args != NULL
	 && extract_info_print_args (&args, &quiet, &regexp, &t_regexp))
    ;

  if (args != NULL)
    report_unrecognized_option_error ("info locals", args);

  print_frame_local_vars (get_selected_frame (_("No frame selected.")),
			  quiet,
			  regexp.empty () ? NULL : regexp.c_str (),
			  t_regexp.empty () ? NULL : t_regexp.c_str (),
			  0, gdb_stdout);
}

/* Iterate over all the argument variables in block B.  */

void
iterate_over_block_arg_vars (const struct block *b,
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

	  sym2 = lookup_symbol_search_name (SYMBOL_SEARCH_NAME (sym),
					    b, VAR_DOMAIN).symbol;
	  (*cb) (SYMBOL_PRINT_NAME (sym), sym2, cb_data);
	}
    }
}

/* Print all argument variables of the function of FRAME.
   Print them with values to STREAM.
   If REGEXP is not NULL, only print argument variables whose name
   matches REGEXP.
   If T_REGEXP is not NULL, only print argument variables whose type
   matches T_REGEXP.
   If no argument variables have been printed and !QUIET, prints a message
   explaining why no argument variables could be printed.

   This function will invalidate FRAME.  */

static void
print_frame_arg_vars (struct frame_info *frame,
		      bool quiet,
		      const char *regexp, const char *t_regexp,
		      struct ui_file *stream)
{
  struct print_variable_and_value_data cb_data;
  struct symbol *func;
  CORE_ADDR pc;
  gdb::optional<compiled_regex> preg;
  gdb::optional<compiled_regex> treg;

  if (!get_frame_pc_if_available (frame, &pc))
    {
      if (!quiet)
	fprintf_filtered (stream,
			  _("PC unavailable, cannot determine args.\n"));
      return;
    }

  func = get_frame_function (frame);
  if (func == NULL)
    {
      if (!quiet)
	fprintf_filtered (stream, _("No symbol table info available.\n"));
      return;
    }

  prepare_reg (regexp, &cb_data.preg);
  prepare_reg (t_regexp, &cb_data.treg);
  cb_data.frame_id = get_frame_id (frame);
  cb_data.num_tabs = 0;
  cb_data.stream = stream;
  cb_data.values_printed = 0;

  iterate_over_block_arg_vars (SYMBOL_BLOCK_VALUE (func),
			       do_print_variable_and_value, &cb_data);

  /* do_print_variable_and_value invalidates FRAME.  */
  frame = NULL;

  if (!cb_data.values_printed && !quiet)
    {
      if (regexp == NULL && t_regexp == NULL)
	fprintf_filtered (stream, _("No arguments.\n"));
      else
	fprintf_filtered (stream, _("No matching arguments.\n"));
    }
}

void
info_args_command (const char *args, int from_tty)
{
  std::string regexp;
  std::string t_regexp;
  bool quiet = false;

  while (args != NULL
	 && extract_info_print_args (&args, &quiet, &regexp, &t_regexp))
    ;

  if (args != NULL)
    report_unrecognized_option_error ("info args", args);


  print_frame_arg_vars (get_selected_frame (_("No frame selected.")),
			quiet,
			regexp.empty () ? NULL : regexp.c_str (),
			t_regexp.empty () ? NULL : t_regexp.c_str (),
			gdb_stdout);
}

/* Return the symbol-block in which the selected frame is executing.
   Can return zero under various legitimate circumstances.

   If ADDR_IN_BLOCK is non-zero, set *ADDR_IN_BLOCK to the relevant
   code address within the block returned.  We use this to decide
   which macros are in scope.  */

const struct block *
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

/* Select the frame up one or COUNT_EXP stack levels from the
   previously selected frame, and print it briefly.  */

static void
up_silently_base (const char *count_exp)
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
up_silently_command (const char *count_exp, int from_tty)
{
  up_silently_base (count_exp);
}

static void
up_command (const char *count_exp, int from_tty)
{
  up_silently_base (count_exp);
  gdb::observers::user_selected_context_changed.notify (USER_SELECTED_FRAME);
}

/* Select the frame down one or COUNT_EXP stack levels from the previously
   selected frame, and print it briefly.  */

static void
down_silently_base (const char *count_exp)
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
down_silently_command (const char *count_exp, int from_tty)
{
  down_silently_base (count_exp);
}

static void
down_command (const char *count_exp, int from_tty)
{
  down_silently_base (count_exp);
  gdb::observers::user_selected_context_changed.notify (USER_SELECTED_FRAME);
}

void
return_command (const char *retval_exp, int from_tty)
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
      expression_up retval_expr = parse_expression (retval_exp);
      struct type *return_type = NULL;

      /* Compute the return value.  Should the computation fail, this
         call throws an error.  */
      return_value = evaluate_expression (retval_expr.get ());

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
      return_type = check_typedef (return_type);
      return_value = value_cast (return_type, return_value);

      /* Make sure the value is fully evaluated.  It may live in the
         stack frame we're about to pop.  */
      if (value_lazy (return_value))
	value_fetch_lazy (return_value);

      if (thisfun != NULL)
	function = read_var_value (thisfun, NULL, thisframe);

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
	{
	  if (TYPE_NO_RETURN (thisfun->type))
	    warning (_("Function does not return normally to caller."));
	  confirmed = query (_("%sMake %s return now? "), query_prefix,
			     SYMBOL_PRINT_NAME (thisfun));
	}
      if (!confirmed)
	error (_("Not confirmed"));
    }

  /* Discard the selected frame and all frames inner-to it.  */
  frame_pop (get_selected_frame (NULL));

  /* Store RETURN_VALUE in the just-returned register set.  */
  if (return_value != NULL)
    {
      struct type *return_type = value_type (return_value);
      struct gdbarch *cache_arch = get_current_regcache ()->arch ();

      gdb_assert (rv_conv != RETURN_VALUE_STRUCT_CONVENTION
		  && rv_conv != RETURN_VALUE_ABI_RETURNS_ADDRESS);
      gdbarch_return_value (cache_arch, function, return_type,
			    get_current_regcache (), NULL /*read*/,
			    value_contents (return_value) /*write*/);
    }

  /* If we are at the end of a call dummy now, pop the dummy frame
     too.  */
  if (get_frame_type (get_current_frame ()) == DUMMY_FRAME)
    frame_pop (get_current_frame ());

  select_frame (get_current_frame ());
  /* If interactive, print the frame that is now current.  */
  if (from_tty)
    print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
}

/* Find the most inner frame in the current stack for a function called
   FUNCTION_NAME.  If no matching frame is found return NULL.  */

static struct frame_info *
find_frame_for_function (const char *function_name)
{
  /* Used to hold the lower and upper addresses for each of the
     SYMTAB_AND_LINEs found for functions matching FUNCTION_NAME.  */
  struct function_bounds
  {
    CORE_ADDR low, high;
  };
  struct frame_info *frame;
  bool found = false;
  int level = 1;

  gdb_assert (function_name != NULL);

  frame = get_current_frame ();
  std::vector<symtab_and_line> sals
    = decode_line_with_current_source (function_name,
				       DECODE_LINE_FUNFIRSTLINE);
  gdb::def_vector<function_bounds> func_bounds (sals.size ());
  for (size_t i = 0; i < sals.size (); i++)
    {
      if (sals[i].pspace != current_program_space)
	func_bounds[i].low = func_bounds[i].high = 0;
      else if (sals[i].pc == 0
	       || find_pc_partial_function (sals[i].pc, NULL,
					    &func_bounds[i].low,
					    &func_bounds[i].high) == 0)
	func_bounds[i].low = func_bounds[i].high = 0;
    }

  do
    {
      for (size_t i = 0; (i < sals.size () && !found); i++)
	found = (get_frame_pc (frame) >= func_bounds[i].low
		 && get_frame_pc (frame) < func_bounds[i].high);
      if (!found)
	{
	  level = 1;
	  frame = find_relative_frame (frame, &level);
	}
    }
  while (!found && level == 0);

  if (!found)
    frame = NULL;

  return frame;
}

/* Implements the dbx 'func' command.  */

static void
func_command (const char *arg, int from_tty)
{
  if (arg == NULL)
    return;

  struct frame_info *frame = find_frame_for_function (arg);
  if (frame == NULL)
    error (_("'%s' not within current stack frame."), arg);
  if (frame != get_selected_frame (NULL))
    {
      select_frame (frame);
      print_stack_frame (get_selected_frame (NULL), 1, SRC_AND_LOC, 1);
    }
}

/* Apply a GDB command to all stack frames, or a set of identified frames,
   or innermost COUNT frames.
   With a negative COUNT, apply command on outermost -COUNT frames.

   frame apply 3 info frame     Apply 'info frame' to frames 0, 1, 2
   frame apply -3 info frame    Apply 'info frame' to outermost 3 frames.
   frame apply all x/i $pc      Apply 'x/i $pc' cmd to all frames.
   frame apply all -s p local_var_no_idea_in_which_frame
                If a frame has a local variable called
                local_var_no_idea_in_which_frame, print frame
                and value of local_var_no_idea_in_which_frame.
   frame apply all -s -q p local_var_no_idea_in_which_frame
                Same as before, but only print the variable value.
   frame apply level 2-5 0 4-7 -s p i = i + 1
                Adds 1 to the variable i in the specified frames.
                Note that i will be incremented twice in
                frames 4 and 5.  */

/* Apply a GDB command to COUNT stack frames, starting at TRAILING.
   CMD starts with 0 or more qcs flags followed by the GDB command to apply.
   COUNT -1 means all frames starting at TRAILING.  WHICH_COMMAND is used
   for error messages.  */

static void
frame_apply_command_count (const char *which_command,
			   const char *cmd, int from_tty,
			   struct frame_info *trailing, int count)
{
  qcs_flags flags;
  struct frame_info *fi;

  while (cmd != NULL && parse_flags_qcs (which_command, &cmd, &flags))
    ;

  if (cmd == NULL || *cmd == '\0')
    error (_("Please specify a command to apply on the selected frames"));

  /* The below will restore the current inferior/thread/frame.
     Usually, only the frame is effectively to be restored.
     But in case CMD switches of inferior/thread, better restore
     these also.  */
  scoped_restore_current_thread restore_thread;

  for (fi = trailing; fi && count--; fi = get_prev_frame (fi))
    {
      QUIT;

      select_frame (fi);
      TRY
	{
	  std::string cmd_result;
	  {
	    /* In case CMD switches of inferior/thread/frame, the below
	       restores the inferior/thread/frame.  FI can then be
	       set to the selected frame.  */
	    scoped_restore_current_thread restore_fi_current_frame;

	    cmd_result = execute_command_to_string (cmd, from_tty);
	  }
	  fi = get_selected_frame (_("frame apply "
				     "unable to get selected frame."));
	  if (!flags.silent || cmd_result.length () > 0)
	    {
	      if (!flags.quiet)
		print_stack_frame (fi, 1, LOCATION, 0);
	      printf_filtered ("%s", cmd_result.c_str ());
	    }
	}
      CATCH (ex, RETURN_MASK_ERROR)
	{
	  fi = get_selected_frame (_("frame apply "
				     "unable to get selected frame."));
	  if (!flags.silent)
	    {
	      if (!flags.quiet)
		print_stack_frame (fi, 1, LOCATION, 0);
	      if (flags.cont)
		printf_filtered ("%s\n", ex.message);
	      else
		throw_exception (ex);
	    }
	}
      END_CATCH;
    }
}

/* Implementation of the "frame apply level" command.  */

static void
frame_apply_level_command (const char *cmd, int from_tty)
{
  if (!target_has_stack)
    error (_("No stack."));

  bool level_found = false;
  const char *levels_str = cmd;
  number_or_range_parser levels (levels_str);

  /* Skip the LEVEL list to find the flags and command args.  */
  while (!levels.finished ())
    {
      /* Call for effect.  */
      levels.get_number ();

      level_found = true;
      if (levels.in_range ())
	levels.skip_range ();
    }

  if (!level_found)
    error (_("Missing or invalid LEVEL... argument"));

  cmd = levels.cur_tok ();

  /* Redo the LEVELS parsing, but applying COMMAND.  */
  levels.init (levels_str);
  while (!levels.finished ())
    {
      const int level_beg = levels.get_number ();
      int n_frames;

      if (levels.in_range ())
	{
	  n_frames = levels.end_value () - level_beg + 1;
	  levels.skip_range ();
	}
      else
	n_frames = 1;

      frame_apply_command_count ("frame apply level", cmd, from_tty,
				 leading_innermost_frame (level_beg), n_frames);
    }
}

/* Implementation of the "frame apply all" command.  */

static void
frame_apply_all_command (const char *cmd, int from_tty)
{
  if (!target_has_stack)
    error (_("No stack."));

  frame_apply_command_count ("frame apply all", cmd, from_tty,
			     get_current_frame (), INT_MAX);
}

/* Implementation of the "frame apply" command.  */

static void
frame_apply_command (const char* cmd, int from_tty)
{
  int count;
  struct frame_info *trailing;

  if (!target_has_stack)
    error (_("No stack."));

  if (cmd == NULL)
    error (_("Missing COUNT argument."));
  count = get_number_trailer (&cmd, 0);
  if (count == 0)
    error (_("Invalid COUNT argument."));

  if (count < 0)
    {
      trailing = trailing_outermost_frame (-count);
      count = -1;
    }
  else
    trailing = get_current_frame ();

  frame_apply_command_count ("frame apply", cmd, from_tty,
			     trailing, count);
}

/* Implementation of the "faas" command.  */

static void
faas_command (const char *cmd, int from_tty)
{
  std::string expanded = std::string ("frame apply all -s ") + cmd;
  execute_command (expanded.c_str (), from_tty);
}


/* Find inner-mode frame with frame address ADDRESS.  Return NULL if no
   matching frame can be found.  */

static struct frame_info *
find_frame_for_address (CORE_ADDR address)
{
  struct frame_id id;
  struct frame_info *fid;

  id = frame_id_build_wild (address);

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
  return NULL;
}



/* Commands with a prefix of `frame apply'.  */
static struct cmd_list_element *frame_apply_cmd_list = NULL;

/* Commands with a prefix of `frame'.  */
static struct cmd_list_element *frame_cmd_list = NULL;

/* Commands with a prefix of `select frame'.  */
static struct cmd_list_element *select_frame_cmd_list = NULL;

/* Commands with a prefix of `info frame'.  */
static struct cmd_list_element *info_frame_cmd_list = NULL;

void
_initialize_stack (void)
{
  struct cmd_list_element *cmd;

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

  add_prefix_cmd ("frame", class_stack,
                  &frame_cmd.base_command, _("\
Select and print a stack frame.\n\
With no argument, print the selected stack frame.  (See also \"info frame\").\n\
A single numerical argument specifies the frame to select."),
                  &frame_cmd_list, "frame ", 1, &cmdlist);

  add_com_alias ("f", "frame", class_stack, 1);

#define FRAME_APPLY_FLAGS_HELP "\
Prints the frame location information followed by COMMAND output.\n\
FLAG arguments are -q (quiet), -c (continue), -s (silent).\n\
Flag -q disables printing the frame location information.\n\
By default, if a COMMAND raises an error, frame apply is aborted.\n\
Flag -c indicates to print the error and continue.\n\
Flag -s indicates to silently ignore a COMMAND that raises an error\n\
or produces no output."

  add_prefix_cmd ("apply", class_stack, frame_apply_command,
		  _("Apply a command to a number of frames.\n\
Usage: frame apply COUNT [FLAG]... COMMAND\n\
With a negative COUNT argument, applies the command on outermost -COUNT frames.\n"
FRAME_APPLY_FLAGS_HELP),
		  &frame_apply_cmd_list, "frame apply ", 1, &frame_cmd_list);

  add_cmd ("all", class_stack, frame_apply_all_command,
	   _("\
Apply a command to all frames.\n\
\n\
Usage: frame apply all [FLAG]... COMMAND\n"
FRAME_APPLY_FLAGS_HELP),
	   &frame_apply_cmd_list);

  add_cmd ("level", class_stack, frame_apply_level_command,
	   _("\
Apply a command to a list of frames.\n\
\n\
Usage: frame apply level LEVEL... [FLAG]... COMMAND\n\
ID is a space-separated list of LEVELs of frames to apply COMMAND on.\n"
FRAME_APPLY_FLAGS_HELP),
	   &frame_apply_cmd_list);

  add_com ("faas", class_stack, faas_command, _("\
Apply a command to all frames (ignoring errors and empty output).\n\
Usage: faas COMMAND\n\
shortcut for 'frame apply all -s COMMAND'"));


  add_prefix_cmd ("frame", class_stack,
		  &frame_cmd.base_command, _("\
Select and print a stack frame.\n\
With no argument, print the selected stack frame.  (See also \"info frame\").\n\
A single numerical argument specifies the frame to select."),
		  &frame_cmd_list, "frame ", 1, &cmdlist);
  add_com_alias ("f", "frame", class_stack, 1);

  add_cmd ("address", class_stack, &frame_cmd.address,
	   _("\
Select and print a stack frame by stack address\n\
\n\
Usage: frame address STACK-ADDRESS"),
	   &frame_cmd_list);

  add_cmd ("view", class_stack, &frame_cmd.view,
	   _("\
View a stack frame that might be outside the current backtrace.\n\
\n\
Usage: frame view STACK-ADDRESS\n\
       frame view STACK-ADDRESS PC-ADDRESS"),
	   &frame_cmd_list);

  cmd = add_cmd ("function", class_stack, &frame_cmd.function,
	   _("\
Select and print a stack frame by function name.\n\
\n\
Usage: frame function NAME\n\
\n\
The innermost frame that visited function NAME is selected."),
	   &frame_cmd_list);
  set_cmd_completer (cmd, frame_selection_by_function_completer);


  add_cmd ("level", class_stack, &frame_cmd.level,
	   _("\
Select and print a stack frame by level.\n\
\n\
Usage: frame level LEVEL"),
	   &frame_cmd_list);

  cmd = add_prefix_cmd_suppress_notification ("select-frame", class_stack,
		      &select_frame_cmd.base_command, _("\
Select a stack frame without printing anything.\n\
A single numerical argument specifies the frame to select."),
		      &select_frame_cmd_list, "select-frame ", 1, &cmdlist,
		      &cli_suppress_notification.user_selected_context);

  add_cmd_suppress_notification ("address", class_stack,
			 &select_frame_cmd.address, _("\
Select a stack frame by stack address.\n\
\n\
Usage: select-frame address STACK-ADDRESS"),
			 &select_frame_cmd_list,
			 &cli_suppress_notification.user_selected_context);


  add_cmd_suppress_notification ("view", class_stack,
		 &select_frame_cmd.view, _("\
Select a stack frame that might be outside the current backtrace.\n\
\n\
Usage: select-frame view STACK-ADDRESS\n\
       select-frame view STACK-ADDRESS PC-ADDRESS"),
		 &select_frame_cmd_list,
		 &cli_suppress_notification.user_selected_context);

  cmd = add_cmd_suppress_notification ("function", class_stack,
	       &select_frame_cmd.function, _("\
Select a stack frame by function name.\n\
\n\
Usage: select-frame function NAME"),
	       &select_frame_cmd_list,
	       &cli_suppress_notification.user_selected_context);
  set_cmd_completer (cmd, frame_selection_by_function_completer);

  add_cmd_suppress_notification ("level", class_stack,
			 &select_frame_cmd.level, _("\
Select a stack frame by level.\n\
\n\
Usage: select-frame level LEVEL"),
			 &select_frame_cmd_list,
			 &cli_suppress_notification.user_selected_context);

  add_com ("backtrace", class_stack, backtrace_command, _("\
Print backtrace of all stack frames, or innermost COUNT frames.\n\
Usage: backtrace [QUALIFIERS]... [COUNT]\n\
With a negative argument, print outermost -COUNT frames.\n\
Use of the 'full' qualifier also prints the values of the local variables.\n\
Use of the 'no-filters' qualifier prohibits frame filters from executing\n\
on this backtrace."));
  add_com_alias ("bt", "backtrace", class_stack, 0);

  add_com_alias ("where", "backtrace", class_alias, 0);
  add_info ("stack", backtrace_command,
	    _("Backtrace of the stack, or innermost COUNT frames."));
  add_info_alias ("s", "stack", 1);

  add_prefix_cmd ("frame", class_info, &info_frame_cmd.base_command,
		  _("All about the selected stack frame.\n\
With no arguments, displays information about the currently selected stack\n\
frame.  Alternatively a frame specification may be provided (See \"frame\")\n\
the information is then printed about the specified frame."),
		  &info_frame_cmd_list, "info frame ", 1, &infolist);
  add_info_alias ("f", "frame", 1);

  add_cmd ("address", class_stack, &info_frame_cmd.address,
	   _("\
Print information about a stack frame selected by stack address.\n\
\n\
Usage: info frame address STACK-ADDRESS"),
	   &info_frame_cmd_list);

  add_cmd ("view", class_stack, &info_frame_cmd.view,
	   _("\
Print information about a stack frame outside the current backtrace.\n\
\n\
Usage: info frame view STACK-ADDRESS\n\
       info frame view STACK-ADDRESS PC-ADDRESS"),
	   &info_frame_cmd_list);

  cmd = add_cmd ("function", class_stack, &info_frame_cmd.function,
	   _("\
Print information about a stack frame selected by function name.\n\
\n\
Usage: info frame function NAME"),
	   &info_frame_cmd_list);
  set_cmd_completer (cmd, frame_selection_by_function_completer);

  add_cmd ("level", class_stack, &info_frame_cmd.level,
	   _("\
Print information about a stack frame selected by level.\n\
\n\
Usage: info frame level LEVEL"),
	   &info_frame_cmd_list);

  add_info ("locals", info_locals_command,
	    info_print_args_help (_("\
All local variables of current stack frame or those matching REGEXPs.\n\
Usage: info locals [-q] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the local variables of the current stack frame.\n"),
				  _("local variables")));
  add_info ("args", info_args_command,
	    info_print_args_help (_("\
All argument variables of current stack frame or those matching REGEXPs.\n\
Usage: info args [-q] [-t TYPEREGEXP] [NAMEREGEXP]\n\
Prints the argument variables of the current stack frame.\n"),
				  _("argument variables")));

  if (dbx_commands)
    add_com ("func", class_stack, func_command, _("\
Select the stack frame that contains NAME.\n\
Usage: func NAME"));

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
