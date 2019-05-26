/* Disassemble support for GDB.

   Copyright (C) 2000-2017 Free Software Foundation, Inc.

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
#include "arch-utils.h"
#include "target.h"
#include "value.h"
#include "ui-out.h"
#include "disasm.h"
#include "gdbcore.h"
#include "gdbcmd.h"
#include "dis-asm.h"
#include "source.h"
#include "safe-ctype.h"
#include <algorithm>

/* Disassemble functions.
   FIXME: We should get rid of all the duplicate code in gdb that does
   the same thing: disassemble_command() and the gdbtk variation.  */

/* This variable is used to hold the prospective disassembler_options value
   which is set by the "set disassembler_options" command.  */
static char *prospective_options = NULL;

/* This structure is used to store line number information for the
   deprecated /m option.
   We need a different sort of line table from the normal one cuz we can't
   depend upon implicit line-end pc's for lines to do the
   reordering in this function.  */

struct deprecated_dis_line_entry
{
  int line;
  CORE_ADDR start_pc;
  CORE_ADDR end_pc;
};

/* This Structure is used to store line number information.
   We need a different sort of line table from the normal one cuz we can't
   depend upon implicit line-end pc's for lines to do the
   reordering in this function.  */

struct dis_line_entry
{
  struct symtab *symtab;
  int line;
};

/* Hash function for dis_line_entry.  */

static hashval_t
hash_dis_line_entry (const void *item)
{
  const struct dis_line_entry *dle = (const struct dis_line_entry *) item;

  return htab_hash_pointer (dle->symtab) + dle->line;
}

/* Equal function for dis_line_entry.  */

static int
eq_dis_line_entry (const void *item_lhs, const void *item_rhs)
{
  const struct dis_line_entry *lhs = (const struct dis_line_entry *) item_lhs;
  const struct dis_line_entry *rhs = (const struct dis_line_entry *) item_rhs;

  return (lhs->symtab == rhs->symtab
	  && lhs->line == rhs->line);
}

/* Create the table to manage lines for mixed source/disassembly.  */

static htab_t
allocate_dis_line_table (void)
{
  return htab_create_alloc (41,
			    hash_dis_line_entry, eq_dis_line_entry,
			    xfree, xcalloc, xfree);
}

/* Add a new dis_line_entry containing SYMTAB and LINE to TABLE.  */

static void
add_dis_line_entry (htab_t table, struct symtab *symtab, int line)
{
  void **slot;
  struct dis_line_entry dle, *dlep;

  dle.symtab = symtab;
  dle.line = line;
  slot = htab_find_slot (table, &dle, INSERT);
  if (*slot == NULL)
    {
      dlep = XNEW (struct dis_line_entry);
      dlep->symtab = symtab;
      dlep->line = line;
      *slot = dlep;
    }
}

/* Return non-zero if SYMTAB, LINE are in TABLE.  */

static int
line_has_code_p (htab_t table, struct symtab *symtab, int line)
{
  struct dis_line_entry dle;

  dle.symtab = symtab;
  dle.line = line;
  return htab_find (table, &dle) != NULL;
}

/* Wrapper of target_read_code.  */

int
gdb_disassembler::dis_asm_read_memory (bfd_vma memaddr, gdb_byte *myaddr,
				       unsigned int len,
				       struct disassemble_info *info)
{
  return target_read_code (memaddr, myaddr, len);
}

/* Wrapper of memory_error.  */

void
gdb_disassembler::dis_asm_memory_error (int err, bfd_vma memaddr,
					struct disassemble_info *info)
{
  gdb_disassembler *self
    = static_cast<gdb_disassembler *>(info->application_data);

  self->m_err_memaddr = memaddr;
}

/* Wrapper of print_address.  */

void
gdb_disassembler::dis_asm_print_address (bfd_vma addr,
					 struct disassemble_info *info)
{
  gdb_disassembler *self
    = static_cast<gdb_disassembler *>(info->application_data);

  print_address (self->arch (), addr, self->stream ());
}

static int
compare_lines (const void *mle1p, const void *mle2p)
{
  struct deprecated_dis_line_entry *mle1, *mle2;
  int val;

  mle1 = (struct deprecated_dis_line_entry *) mle1p;
  mle2 = (struct deprecated_dis_line_entry *) mle2p;

  /* End of sequence markers have a line number of 0 but don't want to
     be sorted to the head of the list, instead sort by PC.  */
  if (mle1->line == 0 || mle2->line == 0)
    {
      val = mle1->start_pc - mle2->start_pc;
      if (val == 0)
        val = mle1->line - mle2->line;
    }
  else
    {
      val = mle1->line - mle2->line;
      if (val == 0)
        val = mle1->start_pc - mle2->start_pc;
    }
  return val;
}

/* See disasm.h.  */

int
gdb_pretty_print_disassembler::pretty_print_insn (struct ui_out *uiout,
						  const struct disasm_insn *insn,
						  int flags)
{
  /* parts of the symbolic representation of the address */
  int unmapped;
  int offset;
  int line;
  int size;
  struct cleanup *ui_out_chain;
  char *filename = NULL;
  char *name = NULL;
  CORE_ADDR pc;
  struct gdbarch *gdbarch = arch ();

  ui_out_chain = make_cleanup_ui_out_tuple_begin_end (uiout, NULL);
  pc = insn->addr;

  if (insn->number != 0)
    {
      uiout->field_fmt ("insn-number", "%u", insn->number);
      uiout->text ("\t");
    }

  if ((flags & DISASSEMBLY_SPECULATIVE) != 0)
    {
      if (insn->is_speculative)
	{
	  uiout->field_string ("is-speculative", "?");

	  /* The speculative execution indication overwrites the first
	     character of the PC prefix.
	     We assume a PC prefix length of 3 characters.  */
	  if ((flags & DISASSEMBLY_OMIT_PC) == 0)
	    uiout->text (pc_prefix (pc) + 1);
	  else
	    uiout->text ("  ");
	}
      else if ((flags & DISASSEMBLY_OMIT_PC) == 0)
	uiout->text (pc_prefix (pc));
      else
	uiout->text ("   ");
    }
  else if ((flags & DISASSEMBLY_OMIT_PC) == 0)
    uiout->text (pc_prefix (pc));
  uiout->field_core_addr ("address", gdbarch, pc);

  if (!build_address_symbolic (gdbarch, pc, 0, &name, &offset, &filename,
			       &line, &unmapped))
    {
      /* We don't care now about line, filename and unmapped.  But we might in
	 the future.  */
      uiout->text (" <");
      if ((flags & DISASSEMBLY_OMIT_FNAME) == 0)
	uiout->field_string ("func-name", name);
      uiout->text ("+");
      uiout->field_int ("offset", offset);
      uiout->text (">:\t");
    }
  else
    uiout->text (":\t");

  if (filename != NULL)
    xfree (filename);
  if (name != NULL)
    xfree (name);

  m_insn_stb.clear ();

  if (flags & DISASSEMBLY_RAW_INSN)
    {
      CORE_ADDR end_pc;
      bfd_byte data;
      int err;
      const char *spacer = "";

      /* Build the opcodes using a temporary stream so we can
	 write them out in a single go for the MI.  */
      m_opcode_stb.clear ();

      size = m_di.print_insn (pc);
      end_pc = pc + size;

      for (;pc < end_pc; ++pc)
	{
	  read_code (pc, &data, 1);
	  m_opcode_stb.printf ("%s%02x", spacer, (unsigned) data);
	  spacer = " ";
	}

      uiout->field_stream ("opcodes", m_opcode_stb);
      uiout->text ("\t");
    }
  else
    size = m_di.print_insn (pc);

  uiout->field_stream ("inst", m_insn_stb);
  do_cleanups (ui_out_chain);
  uiout->text ("\n");

  return size;
}

static int
dump_insns (struct gdbarch *gdbarch,
	    struct ui_out *uiout, CORE_ADDR low, CORE_ADDR high,
	    int how_many, int flags, CORE_ADDR *end_pc)
{
  struct disasm_insn insn;
  int num_displayed = 0;

  memset (&insn, 0, sizeof (insn));
  insn.addr = low;

  gdb_pretty_print_disassembler disasm (gdbarch);

  while (insn.addr < high && (how_many < 0 || num_displayed < how_many))
    {
      int size;

      size = disasm.pretty_print_insn (uiout, &insn, flags);
      if (size <= 0)
	break;

      ++num_displayed;
      insn.addr += size;

      /* Allow user to bail out with ^C.  */
      QUIT;
    }

  if (end_pc != NULL)
    *end_pc = insn.addr;

  return num_displayed;
}

/* The idea here is to present a source-O-centric view of a
   function to the user.  This means that things are presented
   in source order, with (possibly) out of order assembly
   immediately following.

   N.B. This view is deprecated.  */

static void
do_mixed_source_and_assembly_deprecated
  (struct gdbarch *gdbarch, struct ui_out *uiout,
   struct symtab *symtab,
   CORE_ADDR low, CORE_ADDR high,
   int how_many, int flags)
{
  int newlines = 0;
  int nlines;
  struct linetable_entry *le;
  struct deprecated_dis_line_entry *mle;
  struct symtab_and_line sal;
  int i;
  int out_of_order = 0;
  int next_line = 0;
  int num_displayed = 0;
  print_source_lines_flags psl_flags = 0;
  struct cleanup *ui_out_chain;
  struct cleanup *ui_out_tuple_chain = make_cleanup (null_cleanup, 0);
  struct cleanup *ui_out_list_chain = make_cleanup (null_cleanup, 0);

  gdb_assert (symtab != NULL && SYMTAB_LINETABLE (symtab) != NULL);

  nlines = SYMTAB_LINETABLE (symtab)->nitems;
  le = SYMTAB_LINETABLE (symtab)->item;

  if (flags & DISASSEMBLY_FILENAME)
    psl_flags |= PRINT_SOURCE_LINES_FILENAME;

  mle = (struct deprecated_dis_line_entry *)
    alloca (nlines * sizeof (struct deprecated_dis_line_entry));

  /* Copy linetable entries for this function into our data
     structure, creating end_pc's and setting out_of_order as
     appropriate.  */

  /* First, skip all the preceding functions.  */

  for (i = 0; i < nlines - 1 && le[i].pc < low; i++);

  /* Now, copy all entries before the end of this function.  */

  for (; i < nlines - 1 && le[i].pc < high; i++)
    {
      if (le[i].line == le[i + 1].line && le[i].pc == le[i + 1].pc)
	continue;		/* Ignore duplicates.  */

      /* Skip any end-of-function markers.  */
      if (le[i].line == 0)
	continue;

      mle[newlines].line = le[i].line;
      if (le[i].line > le[i + 1].line)
	out_of_order = 1;
      mle[newlines].start_pc = le[i].pc;
      mle[newlines].end_pc = le[i + 1].pc;
      newlines++;
    }

  /* If we're on the last line, and it's part of the function,
     then we need to get the end pc in a special way.  */

  if (i == nlines - 1 && le[i].pc < high)
    {
      mle[newlines].line = le[i].line;
      mle[newlines].start_pc = le[i].pc;
      sal = find_pc_line (le[i].pc, 0);
      mle[newlines].end_pc = sal.end;
      newlines++;
    }

  /* Now, sort mle by line #s (and, then by addresses within lines).  */

  if (out_of_order)
    qsort (mle, newlines, sizeof (struct deprecated_dis_line_entry),
	   compare_lines);

  /* Now, for each line entry, emit the specified lines (unless
     they have been emitted before), followed by the assembly code
     for that line.  */

  ui_out_chain = make_cleanup_ui_out_list_begin_end (uiout, "asm_insns");

  for (i = 0; i < newlines; i++)
    {
      /* Print out everything from next_line to the current line.  */
      if (mle[i].line >= next_line)
	{
	  if (next_line != 0)
	    {
	      /* Just one line to print.  */
	      if (next_line == mle[i].line)
		{
		  ui_out_tuple_chain
		    = make_cleanup_ui_out_tuple_begin_end (uiout,
							   "src_and_asm_line");
		  print_source_lines (symtab, next_line, mle[i].line + 1, psl_flags);
		}
	      else
		{
		  /* Several source lines w/o asm instructions associated.  */
		  for (; next_line < mle[i].line; next_line++)
		    {
		      struct cleanup *ui_out_list_chain_line;
		      struct cleanup *ui_out_tuple_chain_line;
		      
		      ui_out_tuple_chain_line
			= make_cleanup_ui_out_tuple_begin_end (uiout,
							       "src_and_asm_line");
		      print_source_lines (symtab, next_line, next_line + 1,
					  psl_flags);
		      ui_out_list_chain_line
			= make_cleanup_ui_out_list_begin_end (uiout,
							      "line_asm_insn");
		      do_cleanups (ui_out_list_chain_line);
		      do_cleanups (ui_out_tuple_chain_line);
		    }
		  /* Print the last line and leave list open for
		     asm instructions to be added.  */
		  ui_out_tuple_chain
		    = make_cleanup_ui_out_tuple_begin_end (uiout,
							   "src_and_asm_line");
		  print_source_lines (symtab, next_line, mle[i].line + 1, psl_flags);
		}
	    }
	  else
	    {
	      ui_out_tuple_chain
		= make_cleanup_ui_out_tuple_begin_end (uiout,
						       "src_and_asm_line");
	      print_source_lines (symtab, mle[i].line, mle[i].line + 1, psl_flags);
	    }

	  next_line = mle[i].line + 1;
	  ui_out_list_chain
	    = make_cleanup_ui_out_list_begin_end (uiout, "line_asm_insn");
	}

      num_displayed += dump_insns (gdbarch, uiout,
				   mle[i].start_pc, mle[i].end_pc,
				   how_many, flags, NULL);

      /* When we've reached the end of the mle array, or we've seen the last
         assembly range for this source line, close out the list/tuple.  */
      if (i == (newlines - 1) || mle[i + 1].line > mle[i].line)
	{
	  do_cleanups (ui_out_list_chain);
	  do_cleanups (ui_out_tuple_chain);
	  ui_out_tuple_chain = make_cleanup (null_cleanup, 0);
	  ui_out_list_chain = make_cleanup (null_cleanup, 0);
	  uiout->text ("\n");
	}
      if (how_many >= 0 && num_displayed >= how_many)
	break;
    }
  do_cleanups (ui_out_chain);
}

/* The idea here is to present a source-O-centric view of a
   function to the user.  This means that things are presented
   in source order, with (possibly) out of order assembly
   immediately following.  */

static void
do_mixed_source_and_assembly (struct gdbarch *gdbarch,
			      struct ui_out *uiout,
			      struct symtab *main_symtab,
			      CORE_ADDR low, CORE_ADDR high,
			      int how_many, int flags)
{
  const struct linetable_entry *le, *first_le;
  int i, nlines;
  int num_displayed = 0;
  print_source_lines_flags psl_flags = 0;
  struct cleanup *ui_out_chain;
  struct cleanup *ui_out_tuple_chain;
  struct cleanup *ui_out_list_chain;
  CORE_ADDR pc;
  struct symtab *last_symtab;
  int last_line;

  gdb_assert (main_symtab != NULL && SYMTAB_LINETABLE (main_symtab) != NULL);

  /* First pass: collect the list of all source files and lines.
     We do this so that we can only print lines containing code once.
     We try to print the source text leading up to the next instruction,
     but if that text is for code that will be disassembled later, then
     we'll want to defer printing it until later with its associated code.  */

  htab_up dis_line_table (allocate_dis_line_table ());

  pc = low;

  /* The prologue may be empty, but there may still be a line number entry
     for the opening brace which is distinct from the first line of code.
     If the prologue has been eliminated find_pc_line may return the source
     line after the opening brace.  We still want to print this opening brace.
     first_le is used to implement this.  */

  nlines = SYMTAB_LINETABLE (main_symtab)->nitems;
  le = SYMTAB_LINETABLE (main_symtab)->item;
  first_le = NULL;

  /* Skip all the preceding functions.  */
  for (i = 0; i < nlines && le[i].pc < low; i++)
    continue;

  if (i < nlines && le[i].pc < high)
    first_le = &le[i];

  /* Add lines for every pc value.  */
  while (pc < high)
    {
      struct symtab_and_line sal;
      int length;

      sal = find_pc_line (pc, 0);
      length = gdb_insn_length (gdbarch, pc);
      pc += length;

      if (sal.symtab != NULL)
	add_dis_line_entry (dis_line_table.get (), sal.symtab, sal.line);
    }

  /* Second pass: print the disassembly.

     Output format, from an MI perspective:
       The result is a ui_out list, field name "asm_insns", where elements have
       name "src_and_asm_line".
       Each element is a tuple of source line specs (field names line, file,
       fullname), and field "line_asm_insn" which contains the disassembly.
       Field "line_asm_insn" is a list of tuples: address, func-name, offset,
       opcodes, inst.

     CLI output works on top of this because MI ignores ui_out_text output,
     which is where we put file name and source line contents output.

     Cleanup usage:
     ui_out_chain
       Handles the outer "asm_insns" list.
     ui_out_tuple_chain
       The tuples for each group of consecutive disassemblies.
     ui_out_list_chain
       List of consecutive source lines or disassembled insns.  */

  if (flags & DISASSEMBLY_FILENAME)
    psl_flags |= PRINT_SOURCE_LINES_FILENAME;

  ui_out_chain = make_cleanup_ui_out_list_begin_end (uiout, "asm_insns");

  ui_out_tuple_chain = NULL;
  ui_out_list_chain = NULL;

  last_symtab = NULL;
  last_line = 0;
  pc = low;

  while (pc < high)
    {
      struct symtab_and_line sal;
      CORE_ADDR end_pc;
      int start_preceding_line_to_display = 0;
      int end_preceding_line_to_display = 0;
      int new_source_line = 0;

      sal = find_pc_line (pc, 0);

      if (sal.symtab != last_symtab)
	{
	  /* New source file.  */
	  new_source_line = 1;

	  /* If this is the first line of output, check for any preceding
	     lines.  */
	  if (last_line == 0
	      && first_le != NULL
	      && first_le->line < sal.line)
	    {
	      start_preceding_line_to_display = first_le->line;
	      end_preceding_line_to_display = sal.line;
	    }
	}
      else
	{
	  /* Same source file as last time.  */
	  if (sal.symtab != NULL)
	    {
	      if (sal.line > last_line + 1 && last_line != 0)
		{
		  int l;

		  /* Several preceding source lines.  Print the trailing ones
		     not associated with code that we'll print later.  */
		  for (l = sal.line - 1; l > last_line; --l)
		    {
		      if (line_has_code_p (dis_line_table.get (),
					   sal.symtab, l))
			break;
		    }
		  if (l < sal.line - 1)
		    {
		      start_preceding_line_to_display = l + 1;
		      end_preceding_line_to_display = sal.line;
		    }
		}
	      if (sal.line != last_line)
		new_source_line = 1;
	      else
		{
		  /* Same source line as last time.  This can happen, depending
		     on the debug info.  */
		}
	    }
	}

      if (new_source_line)
	{
	  /* Skip the newline if this is the first instruction.  */
	  if (pc > low)
	    uiout->text ("\n");
	  if (ui_out_tuple_chain != NULL)
	    {
	      gdb_assert (ui_out_list_chain != NULL);
	      do_cleanups (ui_out_list_chain);
	      do_cleanups (ui_out_tuple_chain);
	    }
	  if (sal.symtab != last_symtab
	      && !(flags & DISASSEMBLY_FILENAME))
	    {
	      /* Remember MI ignores ui_out_text.
		 We don't have to do anything here for MI because MI
		 output includes the source specs for each line.  */
	      if (sal.symtab != NULL)
		{
		  uiout->text (symtab_to_filename_for_display (sal.symtab));
		}
	      else
		uiout->text ("unknown");
	      uiout->text (":\n");
	    }
	  if (start_preceding_line_to_display > 0)
	    {
	      /* Several source lines w/o asm instructions associated.
		 We need to preserve the structure of the output, so output
		 a bunch of line tuples with no asm entries.  */
	      int l;
	      struct cleanup *ui_out_list_chain_line;
	      struct cleanup *ui_out_tuple_chain_line;

	      gdb_assert (sal.symtab != NULL);
	      for (l = start_preceding_line_to_display;
		   l < end_preceding_line_to_display;
		   ++l)
		{
		  ui_out_tuple_chain_line
		    = make_cleanup_ui_out_tuple_begin_end (uiout,
							   "src_and_asm_line");
		  print_source_lines (sal.symtab, l, l + 1, psl_flags);
		  ui_out_list_chain_line
		    = make_cleanup_ui_out_list_begin_end (uiout,
							  "line_asm_insn");
		  do_cleanups (ui_out_list_chain_line);
		  do_cleanups (ui_out_tuple_chain_line);
		}
	    }
	  ui_out_tuple_chain
	    = make_cleanup_ui_out_tuple_begin_end (uiout, "src_and_asm_line");
	  if (sal.symtab != NULL)
	    print_source_lines (sal.symtab, sal.line, sal.line + 1, psl_flags);
	  else
	    uiout->text (_("--- no source info for this pc ---\n"));
	  ui_out_list_chain
	    = make_cleanup_ui_out_list_begin_end (uiout, "line_asm_insn");
	}
      else
	{
	  /* Here we're appending instructions to an existing line.
	     By construction the very first insn will have a symtab
	     and follow the new_source_line path above.  */
	  gdb_assert (ui_out_tuple_chain != NULL);
	  gdb_assert (ui_out_list_chain != NULL);
	}

      if (sal.end != 0)
	end_pc = std::min (sal.end, high);
      else
	end_pc = pc + 1;
      num_displayed += dump_insns (gdbarch, uiout, pc, end_pc,
				   how_many, flags, &end_pc);
      pc = end_pc;

      if (how_many >= 0 && num_displayed >= how_many)
	break;

      last_symtab = sal.symtab;
      last_line = sal.line;
    }

  do_cleanups (ui_out_chain);
}

static void
do_assembly_only (struct gdbarch *gdbarch, struct ui_out *uiout,
		  CORE_ADDR low, CORE_ADDR high,
		  int how_many, int flags)
{
  struct cleanup *ui_out_chain;

  ui_out_chain = make_cleanup_ui_out_list_begin_end (uiout, "asm_insns");

  dump_insns (gdbarch, uiout, low, high, how_many, flags, NULL);

  do_cleanups (ui_out_chain);
}

/* Initialize the disassemble info struct ready for the specified
   stream.  */

static int ATTRIBUTE_PRINTF (2, 3)
fprintf_disasm (void *stream, const char *format, ...)
{
  va_list args;

  va_start (args, format);
  vfprintf_filtered ((struct ui_file *) stream, format, args);
  va_end (args);
  /* Something non -ve.  */
  return 0;
}

gdb_disassembler::gdb_disassembler (struct gdbarch *gdbarch,
				    struct ui_file *file,
				    di_read_memory_ftype read_memory_func)
  : m_gdbarch (gdbarch),
    m_err_memaddr (0)
{
  init_disassemble_info (&m_di, file, fprintf_disasm);
  m_di.flavour = bfd_target_unknown_flavour;
  m_di.memory_error_func = dis_asm_memory_error;
  m_di.print_address_func = dis_asm_print_address;
  /* NOTE: cagney/2003-04-28: The original code, from the old Insight
     disassembler had a local optomization here.  By default it would
     access the executable file, instead of the target memory (there
     was a growing list of exceptions though).  Unfortunately, the
     heuristic was flawed.  Commands like "disassemble &variable"
     didn't work as they relied on the access going to the target.
     Further, it has been supperseeded by trust-read-only-sections
     (although that should be superseeded by target_trust..._p()).  */
  m_di.read_memory_func = read_memory_func;
  m_di.arch = gdbarch_bfd_arch_info (gdbarch)->arch;
  m_di.mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  m_di.endian = gdbarch_byte_order (gdbarch);
  m_di.endian_code = gdbarch_byte_order_for_code (gdbarch);
  m_di.application_data = this;
  m_di.disassembler_options = get_disassembler_options (gdbarch);
  disassemble_init_for_target (&m_di);
}

int
gdb_disassembler::print_insn (CORE_ADDR memaddr,
			      int *branch_delay_insns)
{
  m_err_memaddr = 0;

  int length = gdbarch_print_insn (arch (), memaddr, &m_di);

  if (length < 0)
    memory_error (TARGET_XFER_E_IO, m_err_memaddr);

  if (branch_delay_insns != NULL)
    {
      if (m_di.insn_info_valid)
	*branch_delay_insns = m_di.branch_delay_insns;
      else
	*branch_delay_insns = 0;
    }
  return length;
}

void
gdb_disassembly (struct gdbarch *gdbarch, struct ui_out *uiout,
		 int flags, int how_many,
		 CORE_ADDR low, CORE_ADDR high)
{
  struct symtab *symtab;
  int nlines = -1;

  /* Assume symtab is valid for whole PC range.  */
  symtab = find_pc_line_symtab (low);

  if (symtab != NULL && SYMTAB_LINETABLE (symtab) != NULL)
    nlines = SYMTAB_LINETABLE (symtab)->nitems;

  if (!(flags & (DISASSEMBLY_SOURCE_DEPRECATED | DISASSEMBLY_SOURCE))
      || nlines <= 0)
    do_assembly_only (gdbarch, uiout, low, high, how_many, flags);

  else if (flags & DISASSEMBLY_SOURCE)
    do_mixed_source_and_assembly (gdbarch, uiout, symtab, low, high,
				  how_many, flags);

  else if (flags & DISASSEMBLY_SOURCE_DEPRECATED)
    do_mixed_source_and_assembly_deprecated (gdbarch, uiout, symtab,
					     low, high, how_many, flags);

  gdb_flush (gdb_stdout);
}

/* Print the instruction at address MEMADDR in debugged memory,
   on STREAM.  Returns the length of the instruction, in bytes,
   and, if requested, the number of branch delay slot instructions.  */

int
gdb_print_insn (struct gdbarch *gdbarch, CORE_ADDR memaddr,
		struct ui_file *stream, int *branch_delay_insns)
{

  gdb_disassembler di (gdbarch, stream);

  return di.print_insn (memaddr, branch_delay_insns);
}

/* Return the length in bytes of the instruction at address MEMADDR in
   debugged memory.  */

int
gdb_insn_length (struct gdbarch *gdbarch, CORE_ADDR addr)
{
  return gdb_print_insn (gdbarch, addr, &null_stream, NULL);
}

/* fprintf-function for gdb_buffered_insn_length.  This function is a
   nop, we don't want to print anything, we just want to compute the
   length of the insn.  */

static int ATTRIBUTE_PRINTF (2, 3)
gdb_buffered_insn_length_fprintf (void *stream, const char *format, ...)
{
  return 0;
}

/* Initialize a struct disassemble_info for gdb_buffered_insn_length.  */

static void
gdb_buffered_insn_length_init_dis (struct gdbarch *gdbarch,
				   struct disassemble_info *di,
				   const gdb_byte *insn, int max_len,
				   CORE_ADDR addr)
{
  init_disassemble_info (di, NULL, gdb_buffered_insn_length_fprintf);

  /* init_disassemble_info installs buffer_read_memory, etc.
     so we don't need to do that here.
     The cast is necessary until disassemble_info is const-ified.  */
  di->buffer = (gdb_byte *) insn;
  di->buffer_length = max_len;
  di->buffer_vma = addr;

  di->arch = gdbarch_bfd_arch_info (gdbarch)->arch;
  di->mach = gdbarch_bfd_arch_info (gdbarch)->mach;
  di->endian = gdbarch_byte_order (gdbarch);
  di->endian_code = gdbarch_byte_order_for_code (gdbarch);

  di->disassembler_options = get_disassembler_options (gdbarch);
  disassemble_init_for_target (di);
}

/* Return the length in bytes of INSN.  MAX_LEN is the size of the
   buffer containing INSN.  */

int
gdb_buffered_insn_length (struct gdbarch *gdbarch,
			  const gdb_byte *insn, int max_len, CORE_ADDR addr)
{
  struct disassemble_info di;

  gdb_buffered_insn_length_init_dis (gdbarch, &di, insn, max_len, addr);

  return gdbarch_print_insn (gdbarch, addr, &di);
}

char *
get_disassembler_options (struct gdbarch *gdbarch)
{
  char **disassembler_options = gdbarch_disassembler_options (gdbarch);
  if (disassembler_options == NULL)
    return NULL;
  return *disassembler_options;
}

void
set_disassembler_options (char *prospective_options)
{
  struct gdbarch *gdbarch = get_current_arch ();
  char **disassembler_options = gdbarch_disassembler_options (gdbarch);
  const disasm_options_t *valid_options;
  char *options = remove_whitespace_and_extra_commas (prospective_options);
  const char *opt;

  /* Allow all architectures, even ones that do not support 'set disassembler',
     to reset their disassembler options to NULL.  */
  if (options == NULL)
    {
      if (disassembler_options != NULL)
	{
	  free (*disassembler_options);
	  *disassembler_options = NULL;
	}
      return;
    }

  valid_options = gdbarch_valid_disassembler_options (gdbarch);
  if (valid_options  == NULL)
    {
      fprintf_filtered (gdb_stdlog, _("\
'set disassembler-options ...' is not supported on this architecture.\n"));
      return;
    }

  /* Verify we have valid disassembler options.  */
  FOR_EACH_DISASSEMBLER_OPTION (opt, options)
    {
      size_t i;
      for (i = 0; valid_options->name[i] != NULL; i++)
	if (disassembler_options_cmp (opt, valid_options->name[i]) == 0)
	  break;
      if (valid_options->name[i] == NULL)
	{
	  fprintf_filtered (gdb_stdlog,
			    _("Invalid disassembler option value: '%s'.\n"),
			    opt);
	  return;
	}
    }

  free (*disassembler_options);
  *disassembler_options = xstrdup (options);
}

static void
set_disassembler_options_sfunc (char *args, int from_tty,
				struct cmd_list_element *c)
{
  set_disassembler_options (prospective_options);
}

static void
show_disassembler_options_sfunc (struct ui_file *file, int from_tty,
				 struct cmd_list_element *c, const char *value)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const disasm_options_t *valid_options;

  const char *options = get_disassembler_options (gdbarch);
  if (options == NULL)
    options = "";

  fprintf_filtered (file, _("The current disassembler options are '%s'\n"),
		    options);

  valid_options = gdbarch_valid_disassembler_options (gdbarch);

  if (valid_options == NULL)
    return;

  fprintf_filtered (file, _("\n\
The following disassembler options are supported for use with the\n\
'set disassembler-options <option>[,<option>...]' command:\n"));

  if (valid_options->description != NULL)
    {
      size_t i, max_len = 0;

      /* Compute the length of the longest option name.  */
      for (i = 0; valid_options->name[i] != NULL; i++)
	{
	  size_t len = strlen (valid_options->name[i]);
	  if (max_len < len)
	    max_len = len;
	}

      for (i = 0, max_len++; valid_options->name[i] != NULL; i++)
	{
	  fprintf_filtered (file, "  %s", valid_options->name[i]);
	  if (valid_options->description[i] != NULL)
	    fprintf_filtered (file, "%*c %s",
			      (int)(max_len - strlen (valid_options->name[i])), ' ',
			      valid_options->description[i]);
	  fprintf_filtered (file, "\n");
	}
    }
  else
    {
      size_t i;
      fprintf_filtered (file, "  ");
      for (i = 0; valid_options->name[i] != NULL; i++)
	{
	  fprintf_filtered (file, "%s", valid_options->name[i]);
	  if (valid_options->name[i + 1] != NULL)
	    fprintf_filtered (file, ", ");
	  wrap_here ("  ");
	}
      fprintf_filtered (file, "\n");
    }
}

/* A completion function for "set disassembler".  */

static VEC (char_ptr) *
disassembler_options_completer (struct cmd_list_element *ignore,
				const char *text, const char *word)
{
  struct gdbarch *gdbarch = get_current_arch ();
  const disasm_options_t *opts = gdbarch_valid_disassembler_options (gdbarch);

  if (opts != NULL)
    {
      /* Only attempt to complete on the last option text.  */
      const char *separator = strrchr (text, ',');
      if (separator != NULL)
	text = separator + 1;
      text = skip_spaces_const (text);
      return complete_on_enum (opts->name, text, word);
    }
  return NULL;
}


/* Initialization code.  */

/* -Wmissing-prototypes */
extern initialize_file_ftype _initialize_disasm;

void
_initialize_disasm (void)
{
  struct cmd_list_element *cmd;

  /* Add the command that controls the disassembler options.  */
  cmd = add_setshow_string_noescape_cmd ("disassembler-options", no_class,
					 &prospective_options, _("\
Set the disassembler options.\n\
Usage: set disassembler-options <option>[,<option>...]\n\n\
See: 'show disassembler-options' for valid option values.\n"), _("\
Show the disassembler options."), NULL,
					 set_disassembler_options_sfunc,
					 show_disassembler_options_sfunc,
					 &setlist, &showlist);
  set_cmd_completer (cmd, disassembler_options_completer);
}
